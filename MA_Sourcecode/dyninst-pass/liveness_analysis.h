#ifndef __LIVENESS_ANALYSIS_H
#define __LIVENESS_ANALYSIS_H

#include "ca_decoder.h"
#include "logging.h"

#include <unordered_map>

#include <BPatch.h>
#include <BPatch_addressSpace.h>
#include <BPatch_flowGraph.h>
#include <BPatch_function.h>
#include <BPatch_object.h>

#include <AddrLookup.h>
#include <PatchCFG.h>
#include <Symtab.h>

using namespace Dyninst::PatchAPI;
using namespace Dyninst::InstructionAPI;
using namespace Dyninst::SymtabAPI;

enum liveness_state_t
{
    REGISTER_CLEAR,
    REGISTER_READ_BEFORE_WRITE,
    REGISTER_WRITE_BEFORE_READ,
    REGISTER_WRITE_OR_UNTOUCHED,
};

static std::unordered_map<uint64_t, register_states_t<liveness_state_t>> liveness_init() { return {}; }

static register_states_t<liveness_state_t> merge_horizontal(std::vector<register_states_t<liveness_state_t>> states)
{
    register_states_t<liveness_state_t> target_state;

    if (states.size() > 0)
    {
        target_state = std::accumulate(
            ++(states.begin()), states.end(), states.front(),
            [](register_states_t<liveness_state_t> state, register_states_t<liveness_state_t> next) {
                return transform(state, next, [](liveness_state_t current, liveness_state_t delta) {
                    if (current == REGISTER_READ_BEFORE_WRITE && delta == REGISTER_CLEAR)
                        current = REGISTER_CLEAR;
                    else if (current == REGISTER_READ_BEFORE_WRITE && delta == REGISTER_WRITE_BEFORE_READ)
                        current = REGISTER_WRITE_BEFORE_READ;
                    else if ((current == REGISTER_WRITE_BEFORE_READ && delta == REGISTER_CLEAR) ||
                             (current == REGISTER_CLEAR && delta == REGISTER_WRITE_BEFORE_READ))
                        current =
                            REGISTER_WRITE_BEFORE_READ; // consider both: REGISTER_WRITE_BEFORE_READ | REGISTER_CLEAR
                    return current;
                });
            });
    }
    return target_state;
}

static register_states_t<liveness_state_t> merge_vertical(register_states_t<liveness_state_t> current,
                                                          register_states_t<liveness_state_t> delta)
{
    return transform(current, delta, [](liveness_state_t current, liveness_state_t delta) {
        return (current == REGISTER_CLEAR) ? delta : current;
    });
}

static register_states_t<liveness_state_t>
liveness_analysis(CADecoder *decoder, BPatch_addressSpace *as, BPatch_basicBlock *block,
                  std::unordered_map<uint64_t, register_states_t<liveness_state_t>> &block_states);

static register_states_t<liveness_state_t>
liveness_analysis(CADecoder *decoder, BPatch_addressSpace *as, std::vector<BPatch_basicBlock *> blocks,
                  std::unordered_map<uint64_t, register_states_t<liveness_state_t>> &block_states)
{
    std::vector<register_states_t<liveness_state_t>> states;

    for (auto block : blocks)
        states.push_back(liveness_analysis(decoder, as, block, block_states));

    return merge_horizontal(states);
}

static register_states_t<liveness_state_t>
liveness_analysis(CADecoder *decoder, BPatch_addressSpace *as, BPatch_function *function,
                  std::unordered_map<uint64_t, register_states_t<liveness_state_t>> &block_states)
{
    char funcname[BUFFER_STRING_LEN];
    function->getName(funcname, BUFFER_STRING_LEN);

    Dyninst::Address start, end;
    function->getAddressRange(start, end);

    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\tANALYZING FUNCTION %s [%lx:0x%lx[", funcname, start, end);
    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\tFUN+----------------------------------------- %s", funcname);

    register_states_t<liveness_state_t> state;

    BPatch_flowGraph *cfg = function->getCFG();

    std::vector<BPatch_basicBlock *> entry_blocks;
    if (!cfg->getEntryBasicBlock(entry_blocks))
    {
        TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\tCOULD NOT DETERMINE ENTRY BASIC_BLOCKS FOR FUNCTION");
    }
    else
    {
        TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\tProcessing Entry Blocks");
        state = liveness_analysis(decoder, as, entry_blocks, block_states);
    }

    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\tFUN------------------------------------------ %s", funcname);
    return state;
}

static register_states_t<liveness_state_t>
liveness_analysis(CADecoder *decoder, BPatch_addressSpace *as, BPatch_basicBlock *block,
                  std::unordered_map<uint64_t, register_states_t<liveness_state_t>> &block_states)
{
    auto bb_start_address = block->getStartAddress();
    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "Processing BasicBlock %lx", bb_start_address);

    auto itr_bool = block_states.insert({bb_start_address, register_states_t<liveness_state_t>()});
    auto &block_state = itr_bool.first->second;

    if (itr_bool.second)
    {
        TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "Information not in Storage");
        PatchBlock::Insns insns;
        Dyninst::PatchAPI::convert(block)->getInsns(insns);

        bool change_possible = true;

        for (auto &instruction : insns)
        {
            if (change_possible)
            {
                auto address = reinterpret_cast<uint64_t>(instruction.first);
                Instruction::Ptr instruction_ptr = instruction.second;

                decoder->decode(address, instruction_ptr);
                TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\tProcessing instruction %lx", address);

                if (decoder->is_indirect_call())
                {
                    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\tInstruction is an indirect call");
                    auto range = decoder->get_register_range();
                    for (int reg_index = range.first; reg_index <= range.second; ++reg_index)
                    {
                        auto &reg_state = block_state[reg_index];

                        if (reg_state == REGISTER_CLEAR)
                            reg_state = REGISTER_WRITE_BEFORE_READ;
                    }

                    change_possible = false;
                }
                else if (decoder->is_call())
                {
                    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\tInstruction is a direct call");
                    auto function = as->findFunctionByAddr((void *)decoder->get_src(0));

                    auto delta_state = liveness_analysis(decoder, as, function, block_states);
                    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t PREVIOUS STATE %s", to_string(block_state).c_str());
                    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t DELTA STATE %s", to_string(delta_state).c_str());

                    block_state = merge_vertical(block_state, delta_state);
                    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t RESULT STATE %s", to_string(block_state).c_str());
                }
                else if (decoder->is_return())
                {
                    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\tInstruction is a return");
                    change_possible = false;
                }
                else if (decoder->is_constant_write())
                {
                    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\tInstruction is a constant write");

                    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t PREVIOUS STATE %s", to_string(block_state).c_str());

                    auto register_states = decoder->get_register_state();
                    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t DECODER STATE %s", to_string(register_states).c_str());
                    register_states = transform(register_states, [](RegisterState state) {
                        switch (state)
                        {
                        case REGISTER_UNTOUCHED:
                        case REGISTER_READ:
                            return REGISTER_UNTOUCHED;
                        case REGISTER_WRITE:
                        case REGISTER_READ_WRITE:
                            return REGISTER_WRITE;
                        }
                    });

                    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t SANITIZED DECODER STATE %s",
                          to_string(register_states).c_str());

                    auto delta_state = convert_states<liveness_state_t>(register_states);
                    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t DELTA STATE %s", to_string(delta_state).c_str());

                    block_state = merge_vertical(block_state, delta_state);
                    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t RESULT STATE %s", to_string(block_state).c_str());

                    change_possible = block_state.state_exists(REGISTER_CLEAR);
                }
                else
                {
                    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t PREVIOUS STATE %s", to_string(block_state).c_str());

                    auto register_states = decoder->get_register_state();
                    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t DECODER STATE %s", to_string(register_states).c_str());

                    auto delta_state = convert_states<liveness_state_t>(register_states);
                    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t DELTA STATE %s", to_string(delta_state).c_str());

                    block_state = merge_vertical(block_state, delta_state);
                    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t RESULT STATE %s", to_string(block_state).c_str());

                    change_possible = block_state.state_exists(REGISTER_CLEAR);
                }
            }
        }

        if (change_possible)
        {
            BPatch_Vector<BPatch_basicBlock *> targets;
            block->getTargets(targets);

            TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "Final merging for BasicBlock %lx", bb_start_address);

            TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t PREVIOUS STATE %s", to_string(block_state).c_str());

            auto delta_state = liveness_analysis(decoder, as, targets, block_states);

            TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t DELTA STATE %s", to_string(delta_state).c_str());

            block_state = merge_vertical(block_state, delta_state);
            TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t RESULT STATE %s", to_string(block_state).c_str());
        }
    }
    return block_state;
}

#endif /* __LIVENESS_ANALYSIS_H */
