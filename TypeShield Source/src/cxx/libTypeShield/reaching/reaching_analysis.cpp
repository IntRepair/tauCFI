#include "reaching_analysis.h"

#include "instrumentation.h"
#include "systemv_abi.h"
#include "utils.h"

#include <BPatch_flowGraph.h>
#include <PatchCFG.h>

#include <boost/optional.hpp>
#include <boost/range/adaptor/reversed.hpp>

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

void reset_state(AnalysisConfig &config) { config.block_states = RegisterStateMap(); }

namespace
{
static boost::optional<RegisterStates>
block_analysis(AnalysisConfig &config, BPatch_basicBlock *block,
               std::vector<BPatch_basicBlock *> path = std::vector<BPatch_basicBlock *>(),
               RegisterStates path_state = RegisterStates());

static boost::optional<RegisterStates>
block_analysis(AnalysisConfig &config, BPatch_basicBlock *block,
               Dyninst::Address end_address,
               std::vector<BPatch_basicBlock *> path = std::vector<BPatch_basicBlock *>(),
               RegisterStates path_state = RegisterStates());

static boost::optional<RegisterStates> blocks_analysis(
    AnalysisConfig &config, std::vector<BPatch_basicBlock *> blocks,
    std::vector<BPatch_basicBlock *> path = std::vector<BPatch_basicBlock *>(),
    RegisterStates path_state = RegisterStates())
{
    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "CALL blocks_analysis(AnalysisConfig "
                                            "&config, std::vector<BPatch_basicBlock *> "
                                            "blocks,...)");
    std::vector<RegisterStates> states;

    for (auto block : blocks)
    {
        auto block_state = block_analysis(config, block, path, path_state);
        if (block_state)
            states.push_back(*block_state);
    }

    boost::optional<RegisterStates> register_state;
    if (states.size() > 0)
        register_state = (*config.merge_horizontal_intra)(states);

    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "RETURN blocks_analysis(AnalysisConfig "
                                            "&config, std::vector<BPatch_basicBlock *> "
                                            "blocks,...)");
    return register_state;
}

boost::optional<RegisterStates> interfn_analysis(
    AnalysisConfig &config, std::vector<FnCallReverse> blocks,
    std::vector<BPatch_basicBlock *> path = std::vector<BPatch_basicBlock *>(),
    RegisterStates path_state = RegisterStates())
{
    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "CALL interfn_analysis(AnalysisConfig "
                                            "&config, std::vector<FnCallReverse> "
                                            "blocks,...)");
    std::vector<RegisterStates> states;

    for (auto block : blocks)
    {
        auto block_state =
            block_analysis(config, block.first, block.second, path, path_state);
        if (block_state)
            states.push_back(*block_state);
    }

    boost::optional<RegisterStates> register_state;
    if (states.size() > 0)
        register_state = (*config.merge_horizontal_inter)(states);

    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "RETURN interfn_analysis(AnalysisConfig "
                                            "&config, std::vector<FnCallReverse> "
                                            "blocks,...)");
    return register_state;
}

static RegisterStates source_analysis(
    AnalysisConfig &config, BPatch_basicBlock *block,
    std::vector<BPatch_basicBlock *> path = std::vector<BPatch_basicBlock *>(),
    RegisterStates path_state = RegisterStates())
{
    LOG_TRACE(
        LOG_FILTER_REACHING_ANALYSIS,
        "CALL source_analysis(AnalysisConfig &config, BPatch_basicBlock *block,...)");

    BPatch_Vector<BPatch_basicBlock *> sources;
    block->getSources(sources);
    function_borders::apply(block, sources);

    RegisterStates state;

    // in case we find no sources, we have to assume that the block prepares the maximum
    // amount

    // previously
    //    auto source_state = blocks_analysis(config, sources, path);
    //    if (block->isEntryBlock() || !source_state)
    //    {
    //        state = system_v::get_maximum_reaching_state();
    //    }
    //
    //    if (source_state)
    //    {
    //        state = (*config.merge_horizontal_entry)(util::make_vector(*source_state,
    //        state));
    //    }

    auto source_state = blocks_analysis(config, sources, path, path_state);
    if (!source_state)
    {
        state = system_v::get_maximum_reaching_state();
    }
    else
    {
        state = *source_state;
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
            auto call_state = interfn_analysis(config, call_sources, path, path_state);
            if (call_state)
            {
                state = (*config.merge_horizontal_entry)(
                    util::make_vector(*call_state, state));
            }
        }
    }

    LOG_TRACE(
        LOG_FILTER_REACHING_ANALYSIS,
        "RETURN source_analysis(AnalysisConfig &config, BPatch_basicBlock *block,...)");
    return state;
}

static RegisterStates function_analysis(
    AnalysisConfig &config, BPatch_function *function,
    std::vector<BPatch_basicBlock *> path = std::vector<BPatch_basicBlock *>(),
    RegisterStates path_state = RegisterStates())
{
    LOG_TRACE(
        LOG_FILTER_REACHING_ANALYSIS,
        "CALL function_analysis(AnalysisConfig &config, BPatch_function *function,...)");

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

        function_borders::apply(function, exit_blocks);
        auto exit_states = blocks_analysis(config, exit_blocks, path, path_state);
        if (exit_states)
            state = *exit_states;
    }

    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS,
              "\tFUN------------------------------------------ %s", funcname);

    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "RETURN function_analysis(AnalysisConfig "
                                            "&config, BPatch_function *function,...)");
    return state;
}

static void process_instruction(
    AnalysisConfig &config, PatchBlock::Insns::value_type const &instruction,
    uint64_t end_address, RegisterStates &block_state, bool &change_possible,
    std::vector<BPatch_basicBlock *> path = std::vector<BPatch_basicBlock *>(),
    RegisterStates path_state = RegisterStates())
{
    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "CALL process_instruction(AnalysisConfig "
                                            "&config, PatchBlock::Insns::value_type "
                                            "const &instruction,...)");

    auto address = reinterpret_cast<uint64_t>(instruction.first);
    auto decoder = config.decoder;
    Instruction::Ptr instruction_ptr = instruction.second;
    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tLooking at instruction %lx", address);

    if (end_address && address == end_address)
    {
        if (decoder->is_indirect_call())
        {
            LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS,
                      "\tInstruction is an indirect call at the end of history");

            if (auto possible_reg_index = decoder->get_reg_source(0))
            {
                auto range = decoder->get_register_range();
                for (auto reg_index = range.first; reg_index <= range.second; ++reg_index)
                {
                    if (*possible_reg_index == reg_index)
                        block_state[reg_index] = REGISTER_TRASHED;
                }
            }
            change_possible = (*config.can_change)(block_state);
        }
    }

    if (change_possible && ((address < end_address) || !end_address))
    {
        if (!decoder->decode(address, instruction_ptr))
        {
            LOG_ERROR(LOG_FILTER_REACHING_ANALYSIS, "Could not decode instruction @%lx",
                      address);

            LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS,
                      "RETURN process_instruction(AnalysisConfig &config, "
                      "PatchBlock::Insns::value_type const &instruction,...)");
            return;
        }
        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tProcessing instruction %lx", address);

        if (decoder->is_indirect_call() ||
            ((!config.follow_return_edges || config.depth == 0) && decoder->is_call()))
        {

            LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tInstruction is %sdirect call",
                      (decoder->is_call() ? "a " : "an in"));
            auto range = decoder->get_register_range();
            for (auto reg_index = range.first; reg_index <= range.second; ++reg_index)
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
            LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS,
                      "\tInstruction is a direct call depth: %u", config.depth);

            LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t PREVIOUS STATE %s",
                      to_string(block_state).c_str());

            if (config.ast.is_function(decoder->get_src(0)))
            {
                auto function =
                    config.ast.get_function(decoder->get_src(0)).get_function();
                config.depth -= 1;

                auto state_delta = function_analysis(config, function, path, path_state);
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
                for (auto reg_index = range.first; reg_index <= range.second; ++reg_index)
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
            for (auto reg_index = range.first; reg_index <= range.second; ++reg_index)
            {
                auto &reg_state = block_state[reg_index];

                // the simple abstraction is that after returning from a function
                // the preserved are restored and the non preserved stay as they are
                if (system_v::is_preserved_register(reg_index))
                    reg_state = REGISTER_UNKNOWN;
            }
            change_possible = (*config.can_change)(block_state);
        }
        else if (!decoder->is_nop())
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

            auto reg_index = decoder->get_par_reg_target(0);
            if (reg_index && decoder->is_constant_write())
            {
                auto value = decoder->get_constant_write();
                if (value == 0)
                {
                    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS,
                              "\t Instruction @%lx writes the value %lx, which is a "
                              "rodata value -> assuming 64bit write",
                              address, value);

                    state_delta[*reg_index] |= REGISTER_SET_FULL;

                    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t DELTA STATE %s",
                              to_string(state_delta).c_str());
                }

                auto const rodata = config.ast.get_region_data(".rodata");
                if (rodata.raw)
                {
                    auto value = decoder->get_constant_write();
                    if (rodata.contains_address(value))
                    {
                        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS,
                                  "\t Instruction @%lx writes the value %lx, which is a "
                                  "rodata value -> assuming 64bit write",
                                  address, value);

                        state_delta[*reg_index] |= REGISTER_SET_FULL;

                        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t DELTA STATE %s",
                                  to_string(state_delta).c_str());
                    }
                }
                auto const data = config.ast.get_region_data(".data");
                if (data.raw)
                {
                    auto value = decoder->get_constant_write();
                    if (data.contains_address(value))
                    {
                        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS,
                                  "\t Instruction @%lx writes the value %lx, which is a "
                                  "data value -> assuming 64bit write",
                                  address, value);

                        state_delta[*reg_index] |= REGISTER_SET_FULL;

                        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t DELTA STATE %s",
                                  to_string(state_delta).c_str());
                    }
                }
                auto const bss = config.ast.get_region_data(".bss");
                if (bss.raw)
                {
                    auto value = decoder->get_constant_write();
                    if (bss.contains_address(value))
                    {
                        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS,
                                  "\t Instruction @%lx writes the value %lx, which is a "
                                  "bss value -> assuming 64bit write",
                                  address, value);

                        state_delta[*reg_index] |= REGISTER_SET_FULL;

                        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t DELTA STATE %s",
                                  to_string(state_delta).c_str());
                    }
                }
            }

            block_state = (*config.merge_vertical)(block_state, state_delta);
            LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t RESULT STATE %s",
                      to_string(block_state).c_str());

            change_possible = (*config.can_change)(block_state);
        }
    }

    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "RETURN process_instruction(AnalysisConfig "
                                            "&config, PatchBlock::Insns::value_type "
                                            "const &instruction,...)");
}

static boost::optional<RegisterStates>
block_analysis(AnalysisConfig &config, BPatch_basicBlock *block,
               std::vector<BPatch_basicBlock *> path, RegisterStates path_state)
{
    LOG_TRACE(
        LOG_FILTER_REACHING_ANALYSIS,
        "CALL block_analysis(AnalysisConfig &config, BPatch_basicBlock *block,...)");

    boost::optional<RegisterStates> block_state_opt;

    if (util::contains(path, block) || path.size() >= 9)
    {
        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "RETURN block_analysis(AnalysisConfig "
                                                "&config, BPatch_basicBlock *block,...)");
        return block_state_opt;
    }

    path.push_back(block);

    auto bb_start_address = block->getStartAddress();
    LOG_DEBUG(LOG_FILTER_REACHING_ANALYSIS, "Processing BasicBlock %lx path length %lu",
             bb_start_address, path.size());

    auto &block_states = config.block_states;

    auto itr_bool = block_states.insert({bb_start_address, RegisterStates()});
    auto &block_state = itr_bool.first->second;

    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS,
              "\tBB+----------------------------------------- %lx", bb_start_address);

    if (itr_bool.second)
    {
        bool change_possible = true;
        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "Information not in Storage");
        PatchBlock::Insns insns;
        Dyninst::PatchAPI::convert(block)->getInsns(insns);

        for (auto &instruction : boost::adaptors::reverse(insns))
        {
            process_instruction(config, instruction, 0, block_state, change_possible,
                                path, path_state);
        }
    }

    block_state_opt = block_state;
    path_state = (*config.merge_vertical)(path_state, *block_state_opt);

    if ((*config.can_change)(path_state))
    {
        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "Final merging for BasicBlock %lx",
                  bb_start_address);
        auto delta_state = source_analysis(config, block, path, path_state);

        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t PREVIOUS STATE %s",
                  to_string(*block_state_opt).c_str());

        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t DELTA STATE %s",
                  to_string(delta_state).c_str());

        block_state_opt = (*config.merge_vertical)(*block_state_opt, delta_state);
    }
    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t RESULT STATE %s",
              to_string(*block_state_opt).c_str());
    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS,
              "\tBB------------------------------------------ %lx", bb_start_address);

    LOG_TRACE(
        LOG_FILTER_REACHING_ANALYSIS,
        "RETURN block_analysis(AnalysisConfig &config, BPatch_basicBlock *block,...)");
    return block_state_opt;
}

static boost::optional<RegisterStates>
block_analysis(AnalysisConfig &config, BPatch_basicBlock *block,
               Dyninst::Address end_address, std::vector<BPatch_basicBlock *> path,
               RegisterStates path_state)
{
    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "CALL block_analysis(AnalysisConfig &config, "
                                            "BPatch_basicBlock *block, Dyninst::Address "
                                            "end_address,...)");
    boost::optional<RegisterStates> block_state_opt;

    if (util::contains(path, block))
    {
        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "RETURN block_analysis(AnalysisConfig "
                                                "&config, BPatch_basicBlock *block, "
                                                "Dyninst::Address end_address,...)");
        return block_state_opt;
    }

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

    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "RETURN block_analysis(AnalysisConfig "
                                            "&config, BPatch_basicBlock *block, "
                                            "Dyninst::Address end_address,...)");
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
