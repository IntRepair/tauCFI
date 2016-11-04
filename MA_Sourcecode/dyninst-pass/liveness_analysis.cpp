#include "liveness_analysis.h"

#include "instrumentation.h"
#include "systemv_abi.h"

#include <BPatch_flowGraph.h>
#include <PatchCFG.h>

using Dyninst::PatchAPI::PatchBlock;

template <> liveness::state_t convert_state(RegisterState reg_state)
{
    switch (reg_state)
    {
    case REGISTER_UNTOUCHED:
        return liveness::REGISTER_CLEAR;
    case REGISTER_READ:
        return liveness::REGISTER_READ_BEFORE_WRITE_FULL;
    case REGISTER_WRITE:
    case REGISTER_READ_WRITE:
        return liveness::REGISTER_WRITE_BEFORE_READ_FULL;
    default:
        assert(false);
    }
}

template <> liveness::state_t convert_state(RegisterStateEx reg_state)
{
    if (reg_state == REGISTER_EX_UNTOUCHED)
        return liveness::REGISTER_CLEAR;

    liveness::state_t state =
        (((reg_state & REGISTER_EX_WRITE_64) == REGISTER_EX_WRITE_64)
             ? liveness::REGISTER_WRITE_BEFORE_READ_64
             : liveness::REGISTER_CLEAR) |
        (((reg_state & REGISTER_EX_WRITE_32) == REGISTER_EX_WRITE_32)
             ? liveness::REGISTER_WRITE_BEFORE_READ_32
             : liveness::REGISTER_CLEAR) |
        (((reg_state & REGISTER_EX_WRITE_16) == REGISTER_EX_WRITE_16)
             ? liveness::REGISTER_WRITE_BEFORE_READ_16
             : liveness::REGISTER_CLEAR) |
        (((reg_state & REGISTER_EX_WRITE_8) == REGISTER_EX_WRITE_8)
             ? liveness::REGISTER_WRITE_BEFORE_READ_8
             : liveness::REGISTER_CLEAR);

    if (state == liveness::REGISTER_CLEAR)
    {
        state = (((reg_state & REGISTER_EX_READ_64) == REGISTER_EX_READ_64)
                     ? liveness::REGISTER_READ_BEFORE_WRITE_64
                     : liveness::REGISTER_CLEAR) |
                (((reg_state & REGISTER_EX_READ_32) == REGISTER_EX_READ_32)
                     ? liveness::REGISTER_READ_BEFORE_WRITE_32
                     : liveness::REGISTER_CLEAR) |
                (((reg_state & REGISTER_EX_READ_16) == REGISTER_EX_READ_16)
                     ? liveness::REGISTER_READ_BEFORE_WRITE_16
                     : liveness::REGISTER_CLEAR) |
                (((reg_state & REGISTER_EX_READ_8) == REGISTER_EX_READ_8)
                     ? liveness::REGISTER_READ_BEFORE_WRITE_8
                     : liveness::REGISTER_CLEAR);
    }

    return state;
}

namespace liveness
{

RegisterStates analysis(AnalysisConfig &config, std::vector<BPatch_basicBlock *> blocks,
                        std::vector<BPatch_basicBlock *> path)
{
    std::vector<RegisterStates> states;

    for (auto const &path_block : path)
        blocks.erase(std::remove(std::begin(blocks), std::end(blocks), path_block),
                     std::end(blocks));

    for (auto block : blocks)
        states.push_back(analysis(config, block, path));

    return (*config.merge_horizontal)(states);
}

RegisterStates analysis(AnalysisConfig &config, BPatch_function *function)
{
    char funcname[BUFFER_STRING_LEN];
    function->getName(funcname, BUFFER_STRING_LEN);

    Dyninst::Address start, end;
    function->getAddressRange(start, end);

    LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\tANALYZING FUNCTION %s [%lx:0x%lx[",
              funcname, start, end);
    LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS,
              "\tFUN+----------------------------------------- %s", funcname);

    RegisterStates state;

    BPatch_flowGraph *cfg = function->getCFG();

    std::vector<BPatch_basicBlock *> entry_blocks;
    if (!cfg->getEntryBasicBlock(entry_blocks))
    {
        LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS,
                  "\tCOULD NOT DETERMINE ENTRY BASIC_BLOCKS FOR FUNCTION");
    }
    else
    {
        LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\tProcessing Entry Blocks");
        state = analysis(config, entry_blocks);
    }

    LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS,
              "\tFUN------------------------------------------ %s", funcname);
    return state;
}

RegisterStates analysis(AnalysisConfig &config, BPatch_basicBlock *block,
                        std::vector<BPatch_basicBlock *> path)
{
    auto bb_start_address = block->getStartAddress();
    LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "Processing BasicBlock %lx",
              bb_start_address);

    auto &block_states = config.block_states;
    auto itr_bool = block_states.insert({bb_start_address, RegisterStates()});
    auto &block_state = itr_bool.first->second;

    auto ignore_instructions = config.ignore_instructions;

    path.push_back(block);

    if (itr_bool.second)
    {
        LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "Information not in Storage");

        bool change_possible = true;
        auto decoder = config.decoder;

        LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS,
                  "\tBB+----------------------------------------- %lx", bb_start_address);

        using Insns = Dyninst::PatchAPI::PatchBlock::Insns;
        instrument_basicBlock_instructions(block, [&](Insns::value_type &instruction) {
            auto address = reinterpret_cast<uint64_t>(instruction.first);
            Instruction::Ptr instruction_ptr = instruction.second;

            if (ignore_instructions && ignore_instructions->count(address) > 0)
            {
                LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\tIgnoring instruction %lx",
                          address);
                return;
            }

            if (!change_possible)
                return;

            if (!decoder->decode(address, instruction_ptr))
            {
                LOG_ERROR(LOG_FILTER_LIVENESS_ANALYSIS,
                          "Could not decode instruction @%lx", address);
                return;
            }

            if (decoder->is_indirect_call() ||
                (!config.follow_calls && decoder->is_call()))
            {
                LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS,
                          "\tInstruction is an indirect call");

                // We need to add the read from the indirect call to the liveness set
                LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t PREVIOUS STATE %s",
                          to_string(block_state).c_str());

                auto register_states = decoder->get_register_state_ex();
                LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t DECODER STATE %s",
                          to_string(register_states).c_str());

                auto delta_state = convert_states<liveness::state_t>(register_states);
                LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t DELTA STATE %s",
                          to_string(delta_state).c_str());

                block_state = (*config.merge_vertical)(block_state, delta_state);
                LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t INTERMEDIATE RESULT STATE %s",
                          to_string(block_state).c_str());

                // Now handle the indirect call itself
                auto range = decoder->get_register_range();
                for (int reg_index = range.first; reg_index <= range.second; ++reg_index)
                {
                    auto &reg_state = block_state[reg_index];

                    if (reg_state == REGISTER_CLEAR)
                        reg_state = REGISTER_WRITE_BEFORE_READ_FULL;
                }
                LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t RESULT STATE %s",
                          to_string(block_state).c_str());

                change_possible = (*config.can_change)(block_state);
            }
            /* TODO: should we follow direct calls ? (if we should, how should we
               proceed)
                without -> no problematic call_target analysis results
                with -> problematic call_target analysis results
                probably more advanced analysis (aka, which register depends on
               which (?))*/
            else if (config.follow_calls && decoder->is_call())
            {
                LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\tInstruction is a direct call");
                auto function = config.image->findFunction(decoder->get_src(0));

                // We need to add the read from the indirect call to the liveness set
                LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t PREVIOUS STATE %s",
                          to_string(block_state).c_str());

                auto register_states = decoder->get_register_state_ex();
                LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t DECODER STATE %s",
                          to_string(register_states).c_str());

                auto delta_state = convert_states<liveness::state_t>(register_states);
                LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t DELTA STATE %s",
                          to_string(delta_state).c_str());

                block_state = (*config.merge_vertical)(block_state, delta_state);
                LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t INTERMEDIATE RESULT STATE %s",
                          to_string(block_state).c_str());

                // Now handle the actual call itself
                if (function)
                {
                    delta_state = analysis(config, function);
                    LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t DELTA STATE %s",
                              to_string(delta_state).c_str());

                    block_state = (*config.merge_vertical)(block_state, delta_state);
                    LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t RESULT STATE %s",
                              to_string(block_state).c_str());
                }
                else
                {
                    auto range = decoder->get_register_range();
                    for (int reg_index = range.first; reg_index <= range.second;
                         ++reg_index)
                    {
                        auto &reg_state = block_state[reg_index];

                        if (reg_state == REGISTER_CLEAR)
                            reg_state = REGISTER_WRITE_BEFORE_READ_FULL;
                    }
                    LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t RESULT STATE %s",
                              to_string(block_state).c_str());
                }
                change_possible = (*config.can_change)(block_state);
            }
            else if (decoder->is_return())
            {
                LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\tInstruction is a return");
                change_possible = false;
            }
            else if (decoder->is_constant_write())
            {
                LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS,
                          "\tInstruction is a constant write");

                LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t PREVIOUS STATE %s",
                          to_string(block_state).c_str());

                auto register_states = decoder->get_register_state_ex();
                LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t DECODER STATE %s",
                          to_string(register_states).c_str());
                register_states = transform(register_states, [](RegisterStateEx state) {
                    return state & REGISTER_EX_WRITE; // Essentially remove all reads
                });

                LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t SANITIZED DECODER STATE %s",
                          to_string(register_states).c_str());

                auto delta_state = convert_states<liveness::state_t>(register_states);
                LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t DELTA STATE %s",
                          to_string(delta_state).c_str());

                block_state = (*config.merge_vertical)(block_state, delta_state);
                LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t RESULT STATE %s",
                          to_string(block_state).c_str());

                change_possible = (*config.can_change)(block_state);
            }
            else if (!config.ignore_nops || (config.ignore_nops && !decoder->is_nop()))
            {
                LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\tProcessing instruction %lx",
                          address);

                LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t PREVIOUS STATE %s",
                          to_string(block_state).c_str());

                auto register_states = decoder->get_register_state_ex();
                LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t DECODER STATE %s",
                          to_string(register_states).c_str());

                auto delta_state = convert_states<liveness::state_t>(register_states);
                LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t DELTA STATE %s",
                          to_string(delta_state).c_str());

                block_state = (*config.merge_vertical)(block_state, delta_state);
                LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t RESULT STATE %s",
                          to_string(block_state).c_str());

                change_possible = (*config.can_change)(block_state);
            }
        });
        LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS,
                  "\tBB|----------------------------------------- %lx", bb_start_address);

        if (change_possible)
        {
            BPatch_Vector<BPatch_basicBlock *> targets;
            block->getTargets(targets);

            LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "Final merging for BasicBlock %lx",
                      bb_start_address);

            LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t PREVIOUS STATE %s",
                      to_string(block_state).c_str());

            auto delta_state = analysis(config, targets, path);

            LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t DELTA STATE %s",
                      to_string(delta_state).c_str());

            block_state = (*config.merge_vertical)(block_state, delta_state);
            LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t RESULT STATE %s",
                      to_string(block_state).c_str());
        }
        LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS,
                  "\tBB------------------------------------------ %lx", bb_start_address);
    }

    return block_state;
}
};
