#include "reaching_analysis.h"

#include "instrumentation.h"
#include "systemv_abi.h"
#include "utils.h"

#include <BPatch_flowGraph.h>
#include <PatchCFG.h>

#include <boost/range/adaptor/reversed.hpp>
#include <boost/optional.hpp>

using Dyninst::PatchAPI::PatchBlock;

template <> reaching::state_t convert_state(RegisterState reg_state)
{
    switch (reg_state)
    {
    case REGISTER_UNTOUCHED:
    case REGISTER_READ:
        return reaching::REGISTER_UNKNOWN;
    case REGISTER_WRITE:
    case REGISTER_READ_WRITE:
        return reaching::REGISTER_SET_FULL;
    default:
        assert(false);
    }
}

template <> reaching::state_t convert_state(RegisterStateEx reg_state)
{
    if (reg_state == REGISTER_EX_UNTOUCHED)
        return reaching::REGISTER_UNKNOWN;

    return (((reg_state & REGISTER_EX_WRITE_64) == REGISTER_EX_WRITE_64)
                ? reaching::REGISTER_SET_64
                : reaching::REGISTER_UNKNOWN) |
           (((reg_state & REGISTER_EX_WRITE_32) == REGISTER_EX_WRITE_32)
                ? reaching::REGISTER_SET_32
                : reaching::REGISTER_UNKNOWN) |
           (((reg_state & REGISTER_EX_WRITE_16) == REGISTER_EX_WRITE_16)
                ? reaching::REGISTER_SET_16
                : reaching::REGISTER_UNKNOWN) |
           (((reg_state & REGISTER_EX_WRITE_8) == REGISTER_EX_WRITE_8)
                ? reaching::REGISTER_SET_8
                : reaching::REGISTER_UNKNOWN);
}

namespace reaching
{

namespace
{
static boost::optional<RegisterStates> block_analysis(
    AnalysisConfig &config, BPatch_basicBlock *block,
    std::vector<BPatch_basicBlock *> path = std::vector<BPatch_basicBlock *>());

static boost::optional<RegisterStates> block_analysis(
    AnalysisConfig &config, BPatch_basicBlock *block, Dyninst::Address end_address,
    std::vector<BPatch_basicBlock *> path = std::vector<BPatch_basicBlock *>());

static boost::optional<RegisterStates> blocks_analysis(
    AnalysisConfig &config, std::vector<BPatch_basicBlock *> blocks,
    std::vector<BPatch_basicBlock *> path = std::vector<BPatch_basicBlock *>())
{
    std::vector<RegisterStates> states;

    for (auto block : blocks)
    {
        auto block_state = block_analysis(config, block, path);
        if (block_state)
            states.push_back(*block_state);
    }

    boost::optional<RegisterStates> register_state;
    if (states.size() > 0)
        register_state = (*config.merge_horizontal_intra)(states);

    return register_state;
}

boost::optional<RegisterStates> interfn_analysis(
    AnalysisConfig &config, std::vector<FnCallReverse> blocks,
    std::vector<BPatch_basicBlock *> path = std::vector<BPatch_basicBlock *>())
{
    std::vector<RegisterStates> states;

    for (auto block : blocks)
    {
        auto block_state = block_analysis(config, block.first, block.second, path);
        if (block_state)
            states.push_back(*block_state);
    }

    boost::optional<RegisterStates> register_state;
    if (states.size() > 0)
        register_state = (*config.merge_horizontal_inter)(states);

    return register_state;
}

static RegisterStates source_analysis(
    AnalysisConfig &config, BPatch_basicBlock *block,
    std::vector<BPatch_basicBlock *> path = std::vector<BPatch_basicBlock *>())
{
    BPatch_Vector<BPatch_basicBlock *> sources;
    block->getSources(sources);

    RegisterStates state;

    // in case we find no sources, we have to assume that the block prepares the maximum
    // amount
    if (block->isEntryBlock())
    {
        state = system_v::get_maximum_reaching_state();
    }

    auto source_state = blocks_analysis(config, sources, path);
    if (source_state)
    {
        state = (*config.merge_horizontal_entry)(util::make_vector(*source_state, state));
    }

    if (block->isEntryBlock())
    {
        auto &min_liveness = config.min_liveness;
        if (min_liveness.count(block) > 0 && config.use_min_liveness)
        {
            state = (*config.merge_horizontal_mlive)(
                util::make_vector(min_liveness[block], state));
        }

        if (config.follow_into_callers && config.block_callers)
        {
            auto call_sources = (*config.block_callers)[block];
            auto call_state = interfn_analysis(config, call_sources, path);
            if (call_state)
            {
                state = (*config.merge_horizontal_entry)(
                    util::make_vector(*call_state, state));
            }
        }
    }

    return state;
}

static RegisterStates function_analysis(
    AnalysisConfig &config, BPatch_function *function,
    std::vector<BPatch_basicBlock *> path = std::vector<BPatch_basicBlock *>())
{
    char funcname[BUFFER_STRING_LEN];
    function->getName(funcname, BUFFER_STRING_LEN);

    Dyninst::Address start, end;
    function->getAddressRange(start, end);

    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tANALYZING FUNCTION %s [%lx:0x%lx[",
              funcname, start, end);
    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS,
              "\tFUN+----------------------------------------- %s", funcname);

    RegisterStates state;

    BPatch_flowGraph *cfg = function->getCFG();

    if (!cfg)
    {
        LOG_ERROR(LOG_FILTER_REACHING_ANALYSIS,
                  "\tCOULD NOT DETERMINE CFG FOR FUNCTION %s", funcname);
    }

    std::vector<BPatch_basicBlock *> exit_blocks;
    if (!cfg->getExitBasicBlock(exit_blocks))
    {
        LOG_ERROR(LOG_FILTER_REACHING_ANALYSIS,
                  "\tCOULD NOT DETERMINE EXIT BASIC_BLOCKS FOR FUNCTION %s", funcname);
    }
    else
    {
        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tProcessing Exit Blocks");

        auto exit_states = blocks_analysis(config, exit_blocks, path);
        if (exit_states)
            state = *exit_states;
    }

    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS,
              "\tFUN------------------------------------------ %s", funcname);
    return state;
}

static void process_instruction(
    AnalysisConfig &config, PatchBlock::Insns::value_type const &instruction,
    uint64_t end_address, RegisterStates &block_state, bool &change_possible,
    std::vector<BPatch_basicBlock *> path = std::vector<BPatch_basicBlock *>())
{
    auto address = reinterpret_cast<uint64_t>(instruction.first);
    auto decoder = config.decoder;
    auto image = config.image;
    Instruction::Ptr instruction_ptr = instruction.second;
    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tLooking at instruction %lx", address);

    if (change_possible && ((address < end_address) || !end_address))
    {
        if (!decoder->decode(address, instruction_ptr))
        {
            LOG_ERROR(LOG_FILTER_REACHING_ANALYSIS, "Could not decode instruction @%lx",
                      address);
            return;
        }
        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tProcessing instruction %lx", address);

        if (decoder->is_indirect_call() ||
            ((!config.follow_return_edges || config.depth == 0) && decoder->is_call()))
        {

            LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tInstruction is %sdirect call",
                      (decoder->is_call() ? "a " : "an in"));
            auto range = decoder->get_register_range();
            for (int reg_index = range.first; reg_index <= range.second; ++reg_index)
            {
                auto &reg_state = block_state[reg_index];

                // Usually a (in)direct call is at the end of a basic block, however in
                // case it is not, we still want to be sure that we do not overwrite
                // information
                if (reg_state == REGISTER_UNKNOWN)
                {
                    if (system_v::is_return_register(reg_index))
                        reg_state = REGISTER_SET_FULL;
                    else if (!system_v::is_preserved_register(reg_index))
                        reg_state = REGISTER_TRASHED;
                }
            }

            change_possible = (*config.can_change)(block_state);
        }
        else if (decoder->is_call() && config.follow_return_edges && config.depth > 0)
        {
            LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tInstruction is a direct call");

            LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t PREVIOUS STATE %s",
                      to_string(block_state).c_str());

            if (auto function = image->findFunction(decoder->get_src(0)))
            {
                config.depth -= 1;
                auto state_delta = function_analysis(config, function, path);
                LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t DELTA STATE %s",
                          to_string(state_delta).c_str());
                block_state = (*config.merge_vertical)(block_state, state_delta);
                LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t RESULT STATE %s",
                          to_string(block_state).c_str());

                config.depth += 1;

                change_possible = (*config.can_change)(block_state);
            }
            else
            {
                auto range = decoder->get_register_range();
                for (int reg_index = range.first; reg_index <= range.second; ++reg_index)
                {
                    auto &reg_state = block_state[reg_index];

                    // Usually a (in)direct call is at the end of a basic block, however
                    // in case it is not, we still want to be sure that we do not
                    // overwrite information
                    if (reg_state == REGISTER_UNKNOWN)
                    {
                        if (system_v::is_return_register(reg_index))
                            reg_state = REGISTER_SET_FULL;
                        else if (!system_v::is_preserved_register(reg_index))
                            reg_state = REGISTER_TRASHED;
                    }
                }

                change_possible = (*config.can_change)(block_state);
            }
        }
        else if (decoder->is_return())
        {
            LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tInstruction is a return");
            auto range = decoder->get_register_range();
            for (int reg_index = range.first; reg_index <= range.second; ++reg_index)
            {
                auto &reg_state = block_state[reg_index];

                // the simple abstraction is that after returning from a function
                // the preserved are restored and the non preserved stay as they are
                if (system_v::is_preserved_register(reg_index))
                    reg_state = REGISTER_UNKNOWN;
            }
            change_possible = (*config.can_change)(block_state);
        }
        else if (!config.ignore_nops || (config.ignore_nops && !decoder->is_nop()))
        {
            LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS,
                      "\tInstruction is a normal instruction");
            LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t PREVIOUS STATE %s",
                      to_string(block_state).c_str());
            auto register_states = decoder->get_register_state_ex();
            LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t DECODER STATE %s",
                      to_string(register_states).c_str());
            auto state_delta = convert_states<state_t>(register_states);
            LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t DELTA STATE %s",
                      to_string(state_delta).c_str());
            block_state = (*config.merge_vertical)(block_state, state_delta);
            LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t RESULT STATE %s",
                      to_string(block_state).c_str());

            change_possible = (*config.can_change)(block_state);
        }
    }
}

static boost::optional<RegisterStates>
block_analysis(AnalysisConfig &config, BPatch_basicBlock *block,
               std::vector<BPatch_basicBlock *> path)
{
    boost::optional<RegisterStates> block_state_opt;

    if (util::contains(path, block))
        return block_state_opt;

    path.push_back(block);

    auto bb_start_address = block->getStartAddress();
    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "Processing BasicBlock %lx",
              bb_start_address);

    auto &block_states = config.block_states;

    auto itr_bool = block_states.insert({bb_start_address, RegisterStates()});
    auto &block_state = itr_bool.first->second;

    if (itr_bool.second)
    {

        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS,
                  "\tBB+----------------------------------------- %lx", bb_start_address);
        bool change_possible = true;
        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "Information not in Storage");
        PatchBlock::Insns insns;
        Dyninst::PatchAPI::convert(block)->getInsns(insns);

        for (auto &instruction : boost::adaptors::reverse(insns))
        {
            process_instruction(config, instruction, 0, block_state, change_possible,
                                path);
        }
        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "Final merging for BasicBlock %lx",
                  bb_start_address);

        block_state_opt = block_state;

        if (change_possible)
        {
            auto delta_state = source_analysis(config, block, path);

            LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t PREVIOUS STATE %s",
                      to_string(*block_state_opt).c_str());

            LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t DELTA STATE %s",
                      to_string(delta_state).c_str());

            block_state_opt = (*config.merge_vertical)(*block_state_opt, delta_state);
            LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t RESULT STATE %s",
                      to_string(*block_state_opt).c_str());
        }
        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS,
                  "\tBB------------------------------------------ %lx", bb_start_address);
    }
    return block_state_opt;
}

static boost::optional<RegisterStates>
block_analysis(AnalysisConfig &config, BPatch_basicBlock *block,
               Dyninst::Address end_address, std::vector<BPatch_basicBlock *> path)
{
    boost::optional<RegisterStates> block_state_opt;

    if (util::contains(path, block))
        return block_state_opt;

    path.push_back(block);

    auto bb_start_address = block->getStartAddress();
    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS,
              "Processing History of BasicBlock %lx before reaching %lx",
              bb_start_address, end_address);

    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS,
              "BB_History+----------------------------------------- %lx : %lx",
              bb_start_address, end_address);

    RegisterStates block_state;
    PatchBlock::Insns insns;
    Dyninst::PatchAPI::convert(block)->getInsns(insns);

    bool change_possible = true;

    for (auto &instruction : boost::adaptors::reverse(insns))
    {
        process_instruction(config, instruction, end_address, block_state,
                            change_possible, path);
    }

    if (change_possible)
    {
        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS,
                  "Final merging for BasicBlock History %lx : %lx", bb_start_address,
                  end_address);
        auto delta_state = source_analysis(config, block, path);

        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t PREVIOUS STATE %s",
                  to_string(block_state).c_str());

        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t DELTA STATE %s",
                  to_string(delta_state).c_str());

        block_state = (*config.merge_vertical)(block_state, delta_state);
        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t RESULT STATE %s",
                  to_string(block_state).c_str());
    }
    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS,
              "BB_History------------------------------------------ %lx : %lx",
              bb_start_address, end_address);

    block_state_opt = block_state;
    return block_state_opt;
}
};

RegisterStates analysis(AnalysisConfig &config, BPatch_function *function)
{
    return function_analysis(config, function);
}

RegisterStates analysis(AnalysisConfig &config, BPatch_basicBlock *block,
                        Dyninst::Address end_address)
{
    return *block_analysis(config, block, end_address);
}
};
