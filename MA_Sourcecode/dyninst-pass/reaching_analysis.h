#ifndef __REACHING_ANALYSIS
#define __REACHING_ANALYSIS

#include "ca_decoder.h"
#include "logging.h"

#include <boost/range/adaptor/reversed.hpp>

#include <unordered_map>

#include <BPatch.h>
#include <BPatch_addressSpace.h>
#include <BPatch_binaryEdit.h>
#include <BPatch_flowGraph.h>
#include <BPatch_function.h>
#include <BPatch_object.h>
#include <BPatch_point.h>
#include <BPatch_process.h>

#include <AddrLookup.h>
#include <PatchCFG.h>
#include <Symtab.h>

using namespace Dyninst::PatchAPI;
using namespace Dyninst::InstructionAPI;
using namespace Dyninst::SymtabAPI;

enum reaching_state_t
{
    REGISTER_UNKNOWN,
    REGISTER_TRASHED,
    REGISTER_SET,
};

static std::unordered_map<uint64_t, register_states_t<reaching_state_t>> reaching_init() { return {}; }

static register_states_t<reaching_state_t> merge_horizontal(std::vector<register_states_t<reaching_state_t>> states)
{
    register_states_t<reaching_state_t> target_state;
    if (states.size() > 0)
    {
        target_state =
            std::accumulate(++(states.begin()), states.end(), states.front(),
                            [](register_states_t<reaching_state_t> current, register_states_t<reaching_state_t> delta) {
                                return transform(current, delta, [](reaching_state_t current, reaching_state_t delta) {
                                    if (current == REGISTER_SET && delta != REGISTER_SET)
                                        current =
                                            REGISTER_TRASHED; // Check if it is possible to have REGISTER_UNKNOWN here
                                    else if (current == REGISTER_UNKNOWN && delta == REGISTER_TRASHED)
                                        current = REGISTER_TRASHED; // consider both: REGISTER_TRASHED | REGISTER_SET }
                                    return current;
                                });
                            });
    }
    return target_state;
}

static register_states_t<reaching_state_t> merge_vertical(register_states_t<reaching_state_t> current,
                                                          register_states_t<reaching_state_t> delta)
{
    return transform(current, delta, [](reaching_state_t current, reaching_state_t delta) {
        return (current == REGISTER_UNKNOWN) ? delta : current;
    });
}

static register_states_t<reaching_state_t>
reaching_analysis(CADecoder *decoder, BPatch_addressSpace *as, BPatch_basicBlock *block,
                  std::unordered_map<uint64_t, register_states_t<reaching_state_t>> &block_states);

static register_states_t<reaching_state_t>
reaching_analysis(CADecoder *decoder, BPatch_addressSpace *as, std::vector<BPatch_basicBlock *> blocks,
                  std::unordered_map<uint64_t, register_states_t<reaching_state_t>> &block_states)
{
    std::vector<register_states_t<reaching_state_t>> states;

    for (auto block : blocks)
        states.push_back(reaching_analysis(decoder, as, block, block_states));
    return merge_horizontal(states);
}

static register_states_t<reaching_state_t>
reaching_analysis(CADecoder *decoder, BPatch_addressSpace *as, BPatch_basicBlock *block, Dyninst::Address end_address,
                  std::unordered_map<uint64_t, register_states_t<reaching_state_t>> &block_states);

static register_states_t<reaching_state_t>
reaching_analysis(CADecoder *decoder, BPatch_addressSpace *as, BPatch_function *function,
                  std::unordered_map<uint64_t, register_states_t<reaching_state_t>> &block_states)
{
    char funcname[BUFFER_STRING_LEN];
    function->getName(funcname, BUFFER_STRING_LEN);

    Dyninst::Address start, end;
    function->getAddressRange(start, end);

    TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tANALYZING FUNCTION %s [%lx:0x%lx[", funcname, start, end);
    TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tFUN+----------------------------------------- %s", funcname);

    register_states_t<reaching_state_t> state;

    BPatch_flowGraph *cfg = function->getCFG();

    std::vector<BPatch_basicBlock *> exit_blocks;
    if (!cfg->getExitBasicBlock(exit_blocks))
    {
        TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tCOULD NOT DETERMINE EXIT BASIC_BLOCKS FOR FUNCTION");
    }
    else
    {
        TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tProcessing Exit Blocks");

        std::vector<register_states_t<reaching_state_t>> states;

        for (auto block : exit_blocks)
        {
            PatchBlock::Insns insns;
            Dyninst::PatchAPI::convert(block)->getInsns(insns);

            for (auto &instruction : insns)
            {
                auto address = reinterpret_cast<uint64_t>(instruction.first);
                Instruction::Ptr instruction_ptr = instruction.second;

                decoder->decode(address, instruction_ptr);

                if (decoder->is_return())
                    states.push_back(reaching_analysis(decoder, as, block, address, block_states));
            }
        }

        state = merge_horizontal(states);
    }

    TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tFUN------------------------------------------ %s", funcname);
    return state;
}

static register_states_t<reaching_state_t>
reaching_analysis(CADecoder *decoder, BPatch_addressSpace *as, BPatch_basicBlock *block,
                  std::unordered_map<uint64_t, register_states_t<reaching_state_t>> &block_states)
{
    auto bb_start_address = block->getStartAddress();
    TRACE(LOG_FILTER_REACHING_ANALYSIS, "Processing BasicBlock %lx", bb_start_address);

    auto itr_bool = block_states.insert({bb_start_address, register_states_t<reaching_state_t>()});
    auto &block_state = itr_bool.first->second;

    if (itr_bool.second)
    {
        TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tBB+----------------------------------------- %lx", bb_start_address);
        TRACE(LOG_FILTER_REACHING_ANALYSIS, "Information not in Storage");
        PatchBlock::Insns insns;
        Dyninst::PatchAPI::convert(block)->getInsns(insns);

        bool change_possible = true;

        for (auto &instruction : boost::adaptors::reverse(insns))
        {
            if (change_possible)
            {
                auto address = reinterpret_cast<uint64_t>(instruction.first);
                Instruction::Ptr instruction_ptr = instruction.second;

                decoder->decode(address, instruction_ptr);
                TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tProcessing instruction %lx", address);

                if (decoder->is_indirect_call())
                {
                    TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tInstruction is an indirect call");
                    auto range = decoder->get_register_range();
                    for (int reg_index = range.first; reg_index <= range.second; ++reg_index)
                    {
                        auto &reg_state = block_state[reg_index];

                        if (reg_state == REGISTER_UNKNOWN)
                            reg_state = REGISTER_TRASHED;
                    }

                    change_possible = false;
                }
                else if (decoder->is_call())
                {
                    TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tInstruction is a direct call");
                    auto range = decoder->get_register_range();
                    for (int reg_index = range.first; reg_index <= range.second; ++reg_index)
                    {
                        auto &reg_state = block_state[reg_index];

                        if (reg_state == REGISTER_UNKNOWN)
                            reg_state = REGISTER_TRASHED;
                    }

                    change_possible = false;
                }
                else if (decoder->is_return())
                {
                    TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tInstruction is a return");
                    auto range = decoder->get_register_range();
                    for (int reg_index = range.first; reg_index <= range.second; ++reg_index)
                    {
                        auto &reg_state = block_state[reg_index];

                        if (reg_state == REGISTER_UNKNOWN)
                            reg_state = REGISTER_TRASHED;
                    }

                    change_possible = false;
                }
                else
                {
                    TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t PREVIOUS STATE %s", to_string(block_state).c_str());
                    auto register_states = decoder->get_register_state();
                    TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t DECODER STATE %s", to_string(register_states).c_str());
                    auto state_delta = convert_states<reaching_state_t>(register_states);
                    TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t DELTA STATE %s", to_string(state_delta).c_str());
                    block_state = merge_vertical(block_state, state_delta);
                    TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t RESULT STATE %s", to_string(block_state).c_str());

                    change_possible = block_state.state_exists(REGISTER_UNKNOWN);
                }
            }
        }

        if (change_possible)
        {
            BPatch_Vector<BPatch_basicBlock *> sources;
            block->getSources(sources);

            TRACE(LOG_FILTER_REACHING_ANALYSIS, "Final merging for BasicBlock %lx", bb_start_address);

            TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t PREVIOUS STATE %s", to_string(block_state).c_str());

            auto delta_state = reaching_analysis(decoder, as, sources, block_states);

            TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t DELTA STATE %s", to_string(delta_state).c_str());

            block_state = merge_vertical(block_state, delta_state);
            TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t RESULT STATE %s", to_string(block_state).c_str());
        }
        TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tBB------------------------------------------ %lx", bb_start_address);
    }
    return block_state;
}

static register_states_t<reaching_state_t>
reaching_analysis(CADecoder *decoder, BPatch_addressSpace *as, BPatch_basicBlock *block, Dyninst::Address end_address,
                  std::unordered_map<uint64_t, register_states_t<reaching_state_t>> &block_states)
{
    auto bb_start_address = block->getStartAddress();
    TRACE(LOG_FILTER_REACHING_ANALYSIS, "Processing History of BasicBlock %lx before reaching %lx", bb_start_address,
          end_address);

    TRACE(LOG_FILTER_REACHING_ANALYSIS, "BB_History+----------------------------------------- %lx : %lx",
          bb_start_address, end_address);

    register_states_t<reaching_state_t> block_state;
    PatchBlock::Insns insns;
    Dyninst::PatchAPI::convert(block)->getInsns(insns);

    bool change_possible = true;

    for (auto &instruction : boost::adaptors::reverse(insns))
    {
        auto address = reinterpret_cast<uint64_t>(instruction.first);
        TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tLooking at instruction %lx", address);

        if (change_possible && address < end_address)
        {
            auto address = reinterpret_cast<uint64_t>(instruction.first);
            Instruction::Ptr instruction_ptr = instruction.second;

            decoder->decode(address, instruction_ptr);
            TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tProcessing instruction %lx", address);

            if (decoder->is_indirect_call())
            {
                TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tInstruction is an indirect call");
                auto range = decoder->get_register_range();
                for (int reg_index = range.first; reg_index <= range.second; ++reg_index)
                {
                    auto &reg_state = block_state[reg_index];

                    if (reg_state == REGISTER_UNKNOWN)
                        reg_state = REGISTER_TRASHED;
                }

                change_possible = false;
            }
            else if (decoder->is_call())
            {
                TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tInstruction is a direct call");
                auto range = decoder->get_register_range();
                for (int reg_index = range.first; reg_index <= range.second; ++reg_index)
                {
                    auto &reg_state = block_state[reg_index];

                    if (reg_state == REGISTER_UNKNOWN)
                        reg_state = REGISTER_TRASHED;
                }

                change_possible = false;
            }
            else if (decoder->is_return())
            {
                TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tInstruction is a return");
                auto range = decoder->get_register_range();
                for (int reg_index = range.first; reg_index <= range.second; ++reg_index)
                {
                    auto &reg_state = block_state[reg_index];

                    if (reg_state == REGISTER_UNKNOWN)
                        reg_state = REGISTER_TRASHED;
                }

                change_possible = false;
            }
            else
            {
                TRACE(LOG_FILTER_REACHING_ANALYSIS, "\tInstruction is a normal function");
                TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t PREVIOUS STATE %s", to_string(block_state).c_str());
                auto register_states = decoder->get_register_state();
                TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t DECODER STATE %s", to_string(register_states).c_str());
                auto state_delta = convert_states<reaching_state_t>(register_states);
                TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t DELTA STATE %s", to_string(state_delta).c_str());
                block_state = merge_vertical(block_state, state_delta);
                TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t RESULT STATE %s", to_string(block_state).c_str());

                change_possible = block_state.state_exists(REGISTER_UNKNOWN);
            }
        }
    }

    if (change_possible)
    {
        BPatch_Vector<BPatch_basicBlock *> sources;
        block->getSources(sources);

        TRACE(LOG_FILTER_REACHING_ANALYSIS, "Final merging for BasicBlock History %lx : %lx", bb_start_address,
              end_address);

        TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t PREVIOUS STATE %s", to_string(block_state).c_str());

        auto delta_state = reaching_analysis(decoder, as, sources, block_states);

        TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t DELTA STATE %s", to_string(delta_state).c_str());

        block_state = merge_vertical(block_state, delta_state);
        TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t RESULT STATE %s", to_string(block_state).c_str());
    }
    TRACE(LOG_FILTER_REACHING_ANALYSIS, "BB_History------------------------------------------ %lx : %lx",
          bb_start_address, end_address);

    return block_state;
}

#endif /* __REACHING_ANALYSIS_H */
