#include "reaching_analysis.h"

#include "instrumentation.h"
#include "systemv_abi.h"

#include <BPatch_flowGraph.h>
#include <PatchCFG.h>

#include <boost/range/adaptor/reversed.hpp>

using Dyninst::PatchAPI::PatchBlock;

template <> reaching::state_t convert_state(RegisterState reg_state)
{
    switch (reg_state)
    {
    case REGISTER_UNTOUCHED:
        return reaching::REGISTER_UNKNOWN;
    case REGISTER_READ:
        return reaching::REGISTER_UNKNOWN;
    case REGISTER_WRITE:
        return reaching::REGISTER_SET;
    case REGISTER_READ_WRITE:
        return reaching::REGISTER_SET;
    }
}

namespace reaching
{
namespace
{
bool has_undefined_param_regs(RegisterStates &reg_state)
{
    bool undefined_regs = false;
    for (auto i = static_cast<size_t>(RegisterStates::min_index);
         i <= static_cast<size_t>(RegisterStates::max_index); ++i)
    {
        if (is_parameter_register(i))
            undefined_regs |= (reg_state[i] == REGISTER_UNKNOWN);
    }
    return undefined_regs;
}

bool has_undefined_return_regs(RegisterStates &reg_state)
{
    bool undefined_regs = false;
    for (auto i = static_cast<size_t>(RegisterStates::min_index);
         i <= static_cast<size_t>(RegisterStates::max_index); ++i)
    {
        if (is_return_register(i))
            undefined_regs |= (reg_state[i] == REGISTER_UNKNOWN);
    }
    return undefined_regs;
}
}

namespace base
{
template <typename merge_fun_t>
static RegisterStates merge_horizontal(std::vector<RegisterStates> states,
                                       merge_fun_t reg_merge_fun)
{
    RegisterStates target_state;
    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t\tMerging states horizontal:");
    for (auto const &state : states)
        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t\t\t%s", to_string(state).c_str());
    if (states.size() == 1)
    {
        target_state = states[0];
    }
    else if (states.size() > 1)
    {
        target_state =
            std::accumulate(++(states.begin()), states.end(), states.front(), reg_merge_fun);
    }
    return target_state;
}
}

namespace calltarget
{

static RegisterStates merge_horizontal(std::vector<RegisterStates> states)
{
    return base::merge_horizontal(states, [](RegisterStates current, RegisterStates delta) {
        return transform(current, delta, [](state_t current, state_t delta) {
            if (delta == REGISTER_TRASHED)
                current = REGISTER_TRASHED;
            if (current == REGISTER_UNKNOWN)
                current = delta;
            return current;
        });
    });
}

static RegisterStates merge_vertical(RegisterStates current, RegisterStates delta)
{
    return transform(current, delta, [](state_t current, state_t delta) {
        return (current == REGISTER_UNKNOWN) ? delta : current;
    });
}

AnalysisConfig init(CADecoder *decoder, BPatch_image *image, BPatch_object *object)
{
    AnalysisConfig config;

    config.decoder = decoder;
    config.image = image;
    config.merge_vertical = &merge_vertical;
    config.merge_horizontal_inter = &merge_horizontal;
    config.merge_horizontal_union = &merge_horizontal;
    config.can_change = has_undefined_return_regs;
    config.follow_return_edges = true;
    config.follow_into_callers = false;

    return config;
}
};

namespace callsite
{
static RegisterStates merge_horizontal_union(std::vector<RegisterStates> states)
{
    return base::merge_horizontal(states, [](RegisterStates current, RegisterStates delta) {
        return transform(current, delta, [](state_t current, state_t delta) {
            if (delta == REGISTER_TRASHED)
                current = REGISTER_TRASHED;
            if (current == REGISTER_UNKNOWN)
                current = delta;
            return current;
        });
    });
}

static RegisterStates merge_horizontal_inter(std::vector<RegisterStates> states)
{
    return base::merge_horizontal(states, [](RegisterStates current, RegisterStates delta) {
        return transform(current, delta, [](state_t current, state_t delta) {

            if (current != delta)
                current = REGISTER_TRASHED;
            return current;
        });
    });
}

static RegisterStates merge_vertical(RegisterStates current, RegisterStates delta)
{
    return transform(current, delta, [](state_t current, state_t delta) {
        return (current == REGISTER_UNKNOWN) ? delta : current;
    });
}

static FunctionCallReverseLookup calculate_block_callers(AnalysisConfig &config,
                                                         BPatch_object *object)
{
    FunctionCallReverseLookup block_callers;

    auto decoder = config.decoder;
    auto image = config.image;

    using Insns = Dyninst::PatchAPI::PatchBlock::Insns;
    instrument_object_basicBlocks_unordered(object, [&](BPatch_basicBlock *block) {
        instrument_basicBlock_instructions(block, [&](Insns::value_type &instruction) {

            auto address = reinterpret_cast<uint64_t>(instruction.first);
            Instruction::Ptr instruction_ptr = instruction.second;
            decoder->decode(address, instruction_ptr);

            if (decoder->is_call())
            {
                auto source_addr = decoder->get_src(0);
                auto function = image->findFunction(source_addr);
                BPatch_flowGraph *cfg = function->getCFG();

                std::vector<BPatch_basicBlock *> entry_blocks;
                if (!cfg->getEntryBasicBlock(entry_blocks))
                {
                    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS,
                              "\tCOULD NOT DETERMINE ENTRY BASIC_BLOCKS FOR FUNCTION");
                }

                BPatch_basicBlock *entry_block = nullptr;
                if (entry_blocks.size() == 1)
                    entry_block = entry_blocks[0];
                else
                {
                    auto itr = std::find_if(std::begin(entry_blocks), std::end(entry_blocks),
                                            [source_addr](BPatch_basicBlock *block) {
                                                return block->getStartAddress() == source_addr;
                                            });
                    if (itr != std::end(entry_blocks))
                    {
                        entry_block = (*itr);
                    }
                    else
                    {
                        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS,
                                  "\tCOULD NOT DETERMINE CORRECT ENTRY BASIC_BLOCK FOR FUNCTION");
                    }
                }

                if (entry_block)
                    block_callers[entry_block].push_back(std::make_pair(block, address));
            }
        });
    });

    return block_callers;
}

static FunctionMinLivenessLookup calculate_min_liveness(CallTargets &targets)
{
    FunctionMinLivenessLookup min_liveness;
    for (auto &target : targets)
    {
        RegisterStates reg_states;

        auto param = target.parameters;
        for (int i = 0; i < 6; ++i)
        {
            if (param[i] == 'x')
                reg_states[get_parameter_register_from_index(i)] = REGISTER_SET;
        }

        auto function = target.function;
        BPatch_flowGraph *cfg = function->getCFG();

        std::vector<BPatch_basicBlock *> entry_blocks;

        if (cfg->getEntryBasicBlock(entry_blocks))
        {
            for (auto block : entry_blocks)
                min_liveness[block] = reg_states;
        }
    }

    return min_liveness;
}

AnalysisConfig init(CADecoder *decoder, BPatch_image *image, BPatch_object *object,
                    CallTargets &targets)
{
    AnalysisConfig config;

    config.decoder = decoder;
    config.image = image;
    config.merge_vertical = &merge_vertical;
    config.merge_horizontal_inter = &merge_horizontal_inter;
    config.merge_horizontal_union = &merge_horizontal_union;
    config.can_change = has_undefined_param_regs;
    config.follow_return_edges = false;
    config.follow_into_callers = true;
    config.block_callers = calculate_block_callers(config, object);
    config.min_liveness = calculate_min_liveness(targets);
    return config;
}
};

static RegisterStates
analysis(AnalysisConfig &config, BPatch_basicBlock *block,
         std::vector<BPatch_basicBlock *> path = std::vector<BPatch_basicBlock *>());

RegisterStates analysis(AnalysisConfig &config, std::vector<BPatch_basicBlock *> blocks,
                        std::vector<BPatch_basicBlock *> path = std::vector<BPatch_basicBlock *>())
{
    std::vector<RegisterStates> states;

    for (auto block : blocks)
        states.push_back(analysis(config, block, path));
    return (*config.merge_horizontal_union)(states);
}

static RegisterStates interfn_analysis(AnalysisConfig &config, std::vector<FnCallReverse> blocks)
{
    std::vector<RegisterStates> states;

    for (auto block : blocks)
        states.push_back(analysis(config, block.first, block.second));
    return (*config.merge_horizontal_inter)(states);
}

static RegisterStates
analyze_sources(AnalysisConfig &config, BPatch_basicBlock *block,
                std::vector<BPatch_basicBlock *> path = std::vector<BPatch_basicBlock *>())
{
    path.push_back(block);

    if (block->isEntryBlock() && config.follow_into_callers)
    {
        auto sources = config.block_callers[block];
        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "EntryBlock %lx : %lx found %lu sources",
                  block->getStartAddress(), block->getEndAddress(), sources.size());

        return (*config.merge_horizontal_union)(std::vector<RegisterStates>{
            interfn_analysis(config, sources), config.min_liveness[block]});
    }
    else
    {
        BPatch_Vector<BPatch_basicBlock *> sources;
        block->getSources(sources);

        for (auto const &path_block : path)
            sources.erase(std::remove(std::begin(sources), std::end(sources), path_block),
                          std::end(sources));

        return analysis(config, sources, path);
    }
}

RegisterStates analysis(AnalysisConfig &config, BPatch_function *function)
{
    char funcname[BUFFER_STRING_LEN];
    function->getName(funcname, BUFFER_STRING_LEN);

    Dyninst::Address start, end;
    function->getAddressRange(start, end);

    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tANALYZING FUNCTION %s [%lx:0x%lx[", funcname, start,
              end);
    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tFUN+----------------------------------------- %s",
              funcname);

    RegisterStates state;

    BPatch_flowGraph *cfg = function->getCFG();

    std::vector<BPatch_basicBlock *> exit_blocks;
    if (!cfg->getExitBasicBlock(exit_blocks))
    {
        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS,
                  "\tCOULD NOT DETERMINE EXIT BASIC_BLOCKS FOR FUNCTION");
    }
    else
    {
        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tProcessing Exit Blocks");

        state = analysis(config, exit_blocks);
    }

    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tFUN------------------------------------------ %s",
              funcname);
    return state;
}

static void process_instruction(AnalysisConfig &config,
                                PatchBlock::Insns::value_type const &instruction,
                                uint64_t end_address, RegisterStates &block_state,
                                bool &change_possible)
{
    auto address = reinterpret_cast<uint64_t>(instruction.first);
    auto decoder = config.decoder;
    auto image = config.image;
    Instruction::Ptr instruction_ptr = instruction.second;
    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tLooking at instruction %lx", address);

    if (change_possible && ((address < end_address) || !end_address))
    {
        decoder->decode(address, instruction_ptr);
        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tProcessing instruction %lx", address);

        if (decoder->is_indirect_call() || (!config.follow_return_edges && decoder->is_call()))
        {

            LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tInstruction is %sdirect call",
                      (decoder->is_call() ? "a " : "an in"));
            auto range = decoder->get_register_range();
            for (int reg_index = range.first; reg_index <= range.second; ++reg_index)
            {
                auto &reg_state = block_state[reg_index];

                // Usually a (in)direct call is at the end of a basic block, however in case it is
                // not, we still want to be sure that we do not overwrite information
                if (reg_state == REGISTER_UNKNOWN)
                {
                    if (is_return_register(reg_index))
                        reg_state = REGISTER_SET;
                    else if (!is_preserved_register(reg_index))
                        reg_state = REGISTER_TRASHED;
                }
            }

            change_possible =
                (*config.can_change)(block_state); // block_state.state_exists(REGISTER_UNKNOWN);
        }
        else if (decoder->is_call() && config.follow_return_edges)
        {
            LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tInstruction is a direct call");

            LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t PREVIOUS STATE %s",
                      to_string(block_state).c_str());

            auto function = image->findFunction(decoder->get_src(0));
            auto state_delta = analysis(config, function);
            LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t DELTA STATE %s",
                      to_string(state_delta).c_str());
            block_state = (*config.merge_vertical)(block_state, state_delta);
            LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t RESULT STATE %s",
                      to_string(block_state).c_str());

            change_possible =
                (*config.can_change)(block_state); // block_state.state_exists(REGISTER_UNKNOWN);
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
                if (is_preserved_register(reg_index))
                    reg_state = REGISTER_UNKNOWN;
            }
            change_possible =
                (*config.can_change)(block_state); // block_state.state_exists(REGISTER_UNKNOWN);
        }
        else
        {
            LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tInstruction is a normal instruction");
            LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t PREVIOUS STATE %s",
                      to_string(block_state).c_str());
            auto register_states = decoder->get_register_state();
            LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t DECODER STATE %s",
                      to_string(register_states).c_str());
            auto state_delta = convert_states<state_t>(register_states);
            LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t DELTA STATE %s",
                      to_string(state_delta).c_str());
            block_state = (*config.merge_vertical)(block_state, state_delta);
            LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t RESULT STATE %s",
                      to_string(block_state).c_str());

            change_possible =
                (*config.can_change)(block_state); // block_state.state_exists(REGISTER_UNKNOWN);
        }
    }
}

static RegisterStates analysis(AnalysisConfig &config, BPatch_basicBlock *block,
                               std::vector<BPatch_basicBlock *> path)
{
    auto bb_start_address = block->getStartAddress();
    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "Processing BasicBlock %lx", bb_start_address);

    auto &block_states = config.block_states;

    auto itr_bool = block_states.insert({bb_start_address, RegisterStates()});
    auto &block_state = itr_bool.first->second;

    if (itr_bool.second)
    {
        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS,
                  "\tBB+----------------------------------------- %lx", bb_start_address);
        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "Information not in Storage");
        PatchBlock::Insns insns;
        Dyninst::PatchAPI::convert(block)->getInsns(insns);

        bool change_possible = true;

        for (auto &instruction : boost::adaptors::reverse(insns))
        {
            process_instruction(config, instruction, 0, block_state, change_possible);
        }
        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "Final merging for BasicBlock %lx",
                  bb_start_address);

        if (change_possible)
        {
            auto delta_state = analyze_sources(config, block, path);

            LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t PREVIOUS STATE %s",
                      to_string(block_state).c_str());

            LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t DELTA STATE %s",
                      to_string(delta_state).c_str());

            block_state = (*config.merge_vertical)(block_state, delta_state);
            LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t RESULT STATE %s",
                      to_string(block_state).c_str());
        }
        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS,
                  "\tBB------------------------------------------ %lx", bb_start_address);
    }
    return block_state;
}

RegisterStates analysis(AnalysisConfig &config, BPatch_basicBlock *block,
                        Dyninst::Address end_address)
{
    auto bb_start_address = block->getStartAddress();
    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS,
              "Processing History of BasicBlock %lx before reaching %lx", bb_start_address,
              end_address);

    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS,
              "BB_History+----------------------------------------- %lx : %lx", bb_start_address,
              end_address);

    RegisterStates block_state;
    PatchBlock::Insns insns;
    Dyninst::PatchAPI::convert(block)->getInsns(insns);

    bool change_possible = true;

    for (auto &instruction : boost::adaptors::reverse(insns))
    {
        process_instruction(config, instruction, end_address, block_state, change_possible);
    }

    if (change_possible)
    {
        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "Final merging for BasicBlock History %lx : %lx",
                  bb_start_address, end_address);
        auto delta_state = analyze_sources(config, block);

        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t PREVIOUS STATE %s",
                  to_string(block_state).c_str());

        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t DELTA STATE %s",
                  to_string(delta_state).c_str());

        block_state = (*config.merge_vertical)(block_state, delta_state);
        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t RESULT STATE %s",
                  to_string(block_state).c_str());
    }
    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS,
              "BB_History------------------------------------------ %lx : %lx", bb_start_address,
              end_address);

    return block_state;
}
};
