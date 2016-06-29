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

using BasicBlockState = std::pair<instruction_register_state_t, std::vector<instruction_register_state_t>>;
using BasicBlockStates = std::unordered_map<uint64_t, BasicBlockState>;

template <> std::string RegisterStatesToString(CADecoder *decoder, BasicBlockState register_states)
{
    RegisterStatesToString(decoder, register_states.first);
    //TODO: show history too ?
}

static instruction_register_state_t merge_horizontal(std::vector<BasicBlockState> states)
{
    BasicBlockState target_state;
    if (states.size() > 0)
    {
        target_state =
            std::accumulate(++(states.begin()), states.end(), states.front(), [](BasicBlockState state,
                                                                                 BasicBlockState next) {
                for (auto &&pair : next.first)
                {
                    auto &reg_state = state.first[pair.first];

                    if (reg_state == REGISTER_SET && pair.second != REGISTER_SET)
                        reg_state = REGISTER_TRASHED; // Check if it is possible to have REGISTER_UNKNOWN here
                    else if (reg_state == REGISTER_UNKNOWN && pair.second == REGISTER_TRASHED)
                        reg_state = REGISTER_TRASHED; // consider both: REGISTER_TRASHED | REGISTER_SET
                }
                return state;
            });
    }
    return target_state.first;
}

static instruction_register_state_t merge_vertical(instruction_register_state_t current,
                                                   instruction_register_state_t delta)
{
    for (auto &&pair : delta)
    {
        auto &reg_state = current[pair.first];

        if (reg_state == REGISTER_UNKNOWN)
            reg_state = pair.second;
    }

    return current;
}

static BasicBlockState merge_vertical(BasicBlockState current, instruction_register_state_t delta)
{
    current.first = merge_vertical(current.first, delta);
    for (auto& state: current.second)
        state = merge_vertical(state, delta);
    return current;
}

static BasicBlockState reaching_analysis(CADecoder *decoder, BPatch_addressSpace *as, BPatch_basicBlock *block,
                                         BasicBlockStates &block_states, bool further = true);


static instruction_register_state_t reaching_analysis(CADecoder *decoder, BPatch_addressSpace *as,
                                         std::vector<BPatch_basicBlock *> blocks, BasicBlockStates &block_states)
{
    std::vector<BasicBlockState> states;

    for (auto block : blocks)
        states.push_back(reaching_analysis(decoder, as, block, block_states));

    return merge_horizontal(states);
}

static instruction_register_state_t reaching_analysis(CADecoder *decoder, BPatch_addressSpace *as, BPatch_function *function,
                                         BasicBlockStates &block_states)
{
    char funcname[BUFFER_STRING_LEN];
    function->getName(funcname, BUFFER_STRING_LEN);

    Dyninst::Address start, end;
    function->getAddressRange(start, end);

    TRACE(LOG_FILTER_CALL_SITE, "\tFUN+----------------------------------------- %s \n", funcname);
    TRACE(LOG_FILTER_CALL_SITE, "\t %s 0x%lx -> 0x%lx\n", funcname, start, end);

	instruction_register_state_t state;

    BPatch_flowGraph *cfg = function->getCFG();

    std::vector<BPatch_basicBlock *> entry_blocks;
    if (!cfg->getEntryBasicBlock(entry_blocks))
    {
        TRACE(LOG_FILTER_CALL_SITE, "\tCOULD NOT DETERMINE ENTRY BASIC_BLOCKS FOR FUNCTION");
    }
    else
    {
        TRACE(LOG_FILTER_CALL_SITE, "\tProcessing Entry Blocks\n");
        state = reaching_analysis(decoder, as, entry_blocks, block_states);
    }

    TRACE(LOG_FILTER_CALL_SITE, "\tFUN------------------------------------------ %s \n", funcname);
    return state;
}

static BasicBlockState reaching_analysis(CADecoder *decoder, BPatch_addressSpace *as, BPatch_basicBlock *block,
                                         BasicBlockStates &block_states, bool further)
{
    auto bb_start_address = block->getStartAddress();
    TRACE(LOG_FILTER_CALL_SITE, "Processing BasicBlock %lx\n", bb_start_address);

    auto &block_state = block_states[bb_start_address];
    if (block_state.first.empty() || block_state.second.empty())
    {
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
                    /*
                                        auto range = decoder->get_register_range();
                                        for (int reg_index = range.first; reg_index <= range.second; ++reg_index)
                                        {
                                            auto &reg_state = block_state[reg_index];

                                            if (reg_state == REGISTER_CLEAR)
                                                reg_state = REGISTER_WRITE_BEFORE_READ;
                                        }

                                        change_possible = false;
                    */
                }
                else if (decoder->is_call())
                {
	                TRACE(LOG_FILTER_CALL_SITE, "\tInstruction is a direct call\n");
                    /*
                                        auto function = as->findFunctionByAddr((void *)decoder->get_src(0));

                                        auto delta_state = reaching_analysis(decoder, as, function, block_states);
                                        TRACE(LOG_FILTER_CALL_SITE, "\t PREVIOUS STATE %s\n",
                                              RegisterStatesToString(decoder, block_state).c_str());
                                        TRACE(LOG_FILTER_CALL_SITE, "\t DELTA STATE %s\n",
                                              RegisterStatesToString(decoder, delta_state).c_str());

                                        block_state = merge_vertical(block_state, delta_state);
                                        TRACE(LOG_FILTER_CALL_SITE, "\t RESULT STATE %s\n",
                                              RegisterStatesToString(decoder, block_state).c_str());
                    */
                }
                else if (decoder->is_return())
                {
	                TRACE(LOG_FILTER_CALL_SITE, "\tInstruction is a return");
                    /*
                                        change_possible = false;
                    */
                }
                else
                {
                    /*
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
                                                                        [](std::pair<const uint64_t, register_state_t>
                       const &reg_state) {
                                                                            return reg_state.second == REGISTER_CLEAR;
                                                                        }));
                    */
                }
            }
        }

        if (change_possible && further)
        {
            BPatch_Vector<BPatch_basicBlock *> sources;
            block->getSources(sources);

            for (auto source : sources)
            {
                auto source_start = source->getStartAddress();
                auto source_end = source->getEndAddress();

                TRACE(LOG_FILTER_CALL_SITE, "BasicBlock %lx, SOURCE: [%lx : %lx[\n",
                    bb_start_address, source_start, source_end);
            }

            for (auto source : sources)
            {
                TRACE(LOG_FILTER_CALL_SITE, "\tBB+----------------------------------------- %lx \n", bb_start_address);
                reaching_analysis(decoder, as, source, block_states, false);
                TRACE(LOG_FILTER_CALL_SITE, "\tBB------------------------------------------ %lx \n", bb_start_address);
            }
/*
            TRACE(LOG_FILTER_CALL_SITE, "Final merging for BasicBlock %lx\n", bb_start_address);

            TRACE(LOG_FILTER_CALL_SITE, "\t PREVIOUS STATE %s\n", RegisterStatesToString(decoder, block_state).c_str());

            auto delta_state = reaching_analysis(decoder, as, sources, block_states);
            TRACE(LOG_FILTER_CALL_SITE, "\t DELTA STATE %s\n", RegisterStatesToString(decoder, delta_state).c_str());

            block_state = merge_vertical(block_state, delta_state);
            TRACE(LOG_FILTER_CALL_SITE, "\t RESULT STATE %s\n", RegisterStatesToString(decoder, block_state).c_str());
*/
        }
    }
    return block_state;
}

std::vector<CallSite> relative_callsite_analysis(BPatch_image *image, std::vector<BPatch_module *> *mods,
                                                 BPatch_addressSpace *as, CADecoder *decoder)
{
    std::vector<CallSite> call_sites;

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

                            BasicBlockStates states;
                            reaching_analysis(decoder, as, block, states);
                            call_sites.emplace_back(
                                CallSite{std::string(modname), start, block->getStartAddress(), address});
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
