#include "relative_callsites.h"

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

enum register_state_t
{
    REGISTER_UNKNOWN,
    REGISTER_TRASHED,
    REGISTER_SET,
};

static register_state_t convert_state(RegisterState reg_state)
{
    switch (reg_state)
    {
    case REGISTER_UNTOUCHED:
        return REGISTER_UNKNOWN;
    case REGISTER_READ:
        return REGISTER_UNKNOWN;
    case REGISTER_WRITE:
        return REGISTER_SET;
    case REGISTER_READ_WRITE:
        return REGISTER_SET;
    }
}

static std::string RegisterStateToString(register_state_t reg_state)
{
    switch (reg_state)
    {
    case REGISTER_UNKNOWN:
        return "*";
    case REGISTER_TRASHED:
        return "T";
    case REGISTER_SET:
        return "S";
    }
}

using instruction_register_state_t = std::unordered_map<int, register_state_t>;

static instruction_register_state_t convert_states(RegisterStates register_states)
{
    instruction_register_state_t state_delta;
    for (auto &&reg_pair : register_states)
        state_delta[reg_pair.first] = convert_state(reg_pair.second);

    return state_delta;
}

using BasicBlockState = instruction_register_state_t;
using BasicBlockStates = std::unordered_map<uint64_t, BasicBlockState>;

static instruction_register_state_t merge_horizontal(std::vector<BasicBlockState> states)
{
    instruction_register_state_t target_state;
    if (states.size() > 0)
    {
        target_state = std::accumulate(
            ++(states.begin()), states.end(), states.front(), [](BasicBlockState state, BasicBlockState next) {
                for (auto &&pair : next)
                {
                    auto &reg_state = state[pair.first];

                    if (reg_state == REGISTER_SET && pair.second != REGISTER_SET)
                        reg_state = REGISTER_TRASHED; // Check if it is possible to have REGISTER_UNKNOWN here
                    else if (reg_state == REGISTER_UNKNOWN && pair.second == REGISTER_TRASHED)
                        reg_state = REGISTER_TRASHED; // consider both: REGISTER_TRASHED | REGISTER_SET
                }
                return state;
            });
    }
    return target_state;
}

static BasicBlockState merge_vertical(BasicBlockState current, instruction_register_state_t delta)
{
    for (auto &&pair : delta)
    {
        auto &reg_state = current[pair.first];

        if (reg_state == REGISTER_UNKNOWN)
            reg_state = pair.second;
    }

    return current;
}

static BasicBlockState reaching_analysis(CADecoder *decoder, BPatch_addressSpace *as, BPatch_basicBlock *block,
                                         BasicBlockStates &block_states);

static BasicBlockState reaching_analysis(CADecoder *decoder, BPatch_addressSpace *as,
                                         std::vector<BPatch_basicBlock *> blocks, BasicBlockStates &block_states)
{
    std::vector<BasicBlockState> states;

    for (auto block : blocks)
        states.push_back(reaching_analysis(decoder, as, block, block_states));
    return merge_horizontal(states);
}

static BasicBlockState reaching_analysis(CADecoder *decoder, BPatch_addressSpace *as, BPatch_basicBlock *block,
                                         BasicBlockStates &block_states)
{
    auto bb_start_address = block->getStartAddress();
    TRACE(LOG_FILTER_CALL_SITE, "Processing BasicBlock %lx\n", bb_start_address);

    auto &block_state = block_states[bb_start_address];
    if (block_state.empty())
    {
        TRACE(LOG_FILTER_CALL_SITE, "\tBB+----------------------------------------- %lx \n", bb_start_address);
        TRACE(LOG_FILTER_CALL_SITE, "Information not in Storage\n");
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
                TRACE(LOG_FILTER_CALL_SITE, "\tProcessing instruction %lx\n", address);

                if (decoder->is_indirect_call())
                {
                    TRACE(LOG_FILTER_CALL_SITE, "\tInstruction is an indirect call\n");
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
                    TRACE(LOG_FILTER_CALL_SITE, "\tInstruction is a direct call\n");
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
                    TRACE(LOG_FILTER_CALL_SITE, "\tInstruction is a return");
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
                    TRACE(LOG_FILTER_CALL_SITE, "\t PREVIOUS STATE %s\n",
                          RegisterStatesToString(decoder, block_state).c_str());
                    auto register_states = decoder->get_register_state();
                    TRACE(LOG_FILTER_CALL_SITE, "\t DECODER STATE %s\n",
                          RegisterStatesToString(decoder, register_states).c_str());
                    auto state_delta = convert_states(register_states);
                    TRACE(LOG_FILTER_CALL_SITE, "\t DELTA STATE %s\n",
                          RegisterStatesToString(decoder, state_delta).c_str());
                    block_state = merge_vertical(block_state, state_delta);
                    TRACE(LOG_FILTER_CALL_SITE, "\t RESULT STATE %s\n",
                          RegisterStatesToString(decoder, block_state).c_str());

                    change_possible = (std::end(block_state) !=
                                       std::find_if(std::begin(block_state), std::end(block_state),
                                                    [](std::pair<const uint64_t, register_state_t> const &reg_state) {
                                                        return reg_state.second == REGISTER_UNKNOWN;
                                                    }));
                }
            }
        }

        if (change_possible)
        {
            BPatch_Vector<BPatch_basicBlock *> sources;
            block->getSources(sources);

            TRACE(LOG_FILTER_CALL_SITE, "Final merging for BasicBlock %lx\n", bb_start_address);

            TRACE(LOG_FILTER_CALL_SITE, "\t PREVIOUS STATE %s\n", RegisterStatesToString(decoder, block_state).c_str());

            auto delta_state = reaching_analysis(decoder, as, sources, block_states);

            TRACE(LOG_FILTER_CALL_SITE, "\t DELTA STATE %s\n", RegisterStatesToString(decoder, delta_state).c_str());

            block_state = merge_vertical(block_state, delta_state);
            TRACE(LOG_FILTER_CALL_SITE, "\t RESULT STATE %s\n", RegisterStatesToString(decoder, block_state).c_str());
        }
        TRACE(LOG_FILTER_CALL_SITE, "\tBB------------------------------------------ %lx \n", bb_start_address);
    }
    return block_state;
}

static BasicBlockState reaching_analysis(CADecoder *decoder, BPatch_addressSpace *as, BPatch_basicBlock *block,
                                         Dyninst::Address end_address, BasicBlockStates &block_states)
{
    auto bb_start_address = block->getStartAddress();
    TRACE(LOG_FILTER_CALL_SITE, "Processing History of BasicBlock %lx before reaching %lx\n", bb_start_address,
          end_address);

    TRACE(LOG_FILTER_CALL_SITE, "BB_History+----------------------------------------- %lx : %lx \n", bb_start_address,
          end_address);

    BasicBlockState block_state;
    PatchBlock::Insns insns;
    Dyninst::PatchAPI::convert(block)->getInsns(insns);

    bool change_possible = true;

    for (auto &instruction : boost::adaptors::reverse(insns))
    {
        auto address = reinterpret_cast<uint64_t>(instruction.first);
        TRACE(LOG_FILTER_CALL_SITE, "\tLooking at instruction %lx\n", address);

        if (change_possible && address < end_address)
        {
            auto address = reinterpret_cast<uint64_t>(instruction.first);
            Instruction::Ptr instruction_ptr = instruction.second;

            decoder->decode(address, instruction_ptr);
            TRACE(LOG_FILTER_CALL_SITE, "\tProcessing instruction %lx\n", address);

            if (decoder->is_indirect_call())
            {
                TRACE(LOG_FILTER_CALL_SITE, "\tInstruction is an indirect call\n");
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
                TRACE(LOG_FILTER_CALL_SITE, "\tInstruction is a direct call\n");
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
                TRACE(LOG_FILTER_CALL_SITE, "\tInstruction is a return");
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
                TRACE(LOG_FILTER_CALL_SITE, "\t PREVIOUS STATE %s\n",
                      RegisterStatesToString(decoder, block_state).c_str());
                auto register_states = decoder->get_register_state();
                TRACE(LOG_FILTER_CALL_SITE, "\t DECODER STATE %s\n",
                      RegisterStatesToString(decoder, register_states).c_str());
                auto state_delta = convert_states(register_states);
                TRACE(LOG_FILTER_CALL_SITE, "\t DELTA STATE %s\n",
                      RegisterStatesToString(decoder, state_delta).c_str());
                block_state = merge_vertical(block_state, state_delta);
                TRACE(LOG_FILTER_CALL_SITE, "\t RESULT STATE %s\n",
                      RegisterStatesToString(decoder, block_state).c_str());

                change_possible = (std::end(block_state) !=
                                   std::find_if(std::begin(block_state), std::end(block_state),
                                                [](std::pair<const uint64_t, register_state_t> const &reg_state) {
                                                    return reg_state.second == REGISTER_UNKNOWN;
                                                }));
            }
        }
    }

    if (change_possible)
    {
        BPatch_Vector<BPatch_basicBlock *> sources;
        block->getSources(sources);

        TRACE(LOG_FILTER_CALL_SITE, "Final merging for BasicBlock History %lx : %lx \n", bb_start_address, end_address);

        TRACE(LOG_FILTER_CALL_SITE, "\t PREVIOUS STATE %s\n", RegisterStatesToString(decoder, block_state).c_str());

        auto delta_state = reaching_analysis(decoder, as, sources, block_states);

        TRACE(LOG_FILTER_CALL_SITE, "\t DELTA STATE %s\n", RegisterStatesToString(decoder, delta_state).c_str());

        block_state = merge_vertical(block_state, delta_state);
        TRACE(LOG_FILTER_CALL_SITE, "\t RESULT STATE %s\n", RegisterStatesToString(decoder, block_state).c_str());
    }
    TRACE(LOG_FILTER_CALL_SITE, "BB_History------------------------------------------ %lx : %lx \n", bb_start_address,
          end_address);

    return block_state;
}

std::vector<CallSite> relative_callsite_analysis(BPatch_image *image, std::vector<BPatch_module *> *mods,
                                                 BPatch_addressSpace *as, CADecoder *decoder)
{
    std::vector<CallSite> call_sites;

    BasicBlockStates states;

    for (auto module : *mods)
    {
        char modname[BUFFER_STRING_LEN];
        module->getFullName(modname, BUFFER_STRING_LEN);

        if (module->isSharedLib())
        {
            WARNING(LOG_FILTER_CALL_SITE, "Skipping shared library %s\n", modname);
        }
        else
        {
            INFO(LOG_FILTER_CALL_SITE, "Processing module %s\n", modname);
            // Instrument module
            std::vector<BPatch_function *> *functions = module->getProcedures(true);

            char funcname[BUFFER_STRING_LEN];
            Dyninst::Address start, end;

            for (auto function : *functions)
            {
                // Instrument function
                function->getName(funcname, BUFFER_STRING_LEN);
                function->getAddressRange(start, end);
                INFO(LOG_FILTER_CALL_SITE, "Processing function [%lx:%lx] %s\n", start, end, funcname);

                std::set<BPatch_basicBlock *> blocks;
                BPatch_flowGraph *cfg = function->getCFG();
                cfg->getAllBasicBlocks(blocks);

                for (auto block : blocks)
                {
                    // Instrument BasicBlock
                    DEBUG(LOG_FILTER_CALL_SITE, "Processing basic block %lx\n", block->getStartAddress());
                    // iterate backwards (PatchAPI restriction)
                    PatchBlock::Insns insns;
                    Dyninst::PatchAPI::convert(block)->getInsns(insns);

                    for (auto &instruction : insns)
                    {
                        // get instruction bytes
                        auto address = reinterpret_cast<uint64_t>(instruction.first);
                        Instruction::Ptr instruction_ptr = instruction.second;

                        decoder->decode(address, instruction_ptr);
                        DEBUG(LOG_FILTER_CALL_SITE, "Processing instruction %lx\n", address);

                        if (decoder->is_indirect_call())
                        {
                            INFO(LOG_FILTER_CALL_SITE, "Callsite Block %lx Instr %lx\n", block->getStartAddress(), address);

                            auto state = reaching_analysis(decoder, as, block, address, states);
                            call_sites.emplace_back(
                                CallSite{std::string(modname), start, block->getStartAddress(), address, RegisterStatesToString(decoder, state)});
                        }
                    }
                }
            }
        }
    }

    return call_sites;
}

void dump_call_sites(std::vector<CallSite> &call_sites)
{
    for (auto &call_site : call_sites)
        INFO(LOG_FILTER_CALL_SITE, "<CS>%s:%lx:%lx:%lx\n", call_site.module_name.c_str(), call_site.function_start, call_site.block_start,
             call_site.address);
}
