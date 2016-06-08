#include "calltargets.h"
#include "utils.h"

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

using BasicBlockState =
    std::unordered_map<int, RegisterState>;

using BasicBlockStates =
    std::unordered_map<uint64_t, BasicBlockState>;

std::string RegisterStatesToString(CADecoder *decoder, BasicBlockState register_states)
{
    std::string state_string = "[";
    auto range = decoder->get_register_range();

    for (auto reg = range.first; reg <= range.second; ++reg)
    {
        auto reg_state = register_states[reg];
        state_string += RegisterStateToString(reg_state) + ((reg == range.second) ? "]" : ", ");
    }

    return state_string;
}

static BasicBlockState liveness_analysis(CADecoder *decoder, BPatch_basicBlock *block, BasicBlockStates& block_states);

static std::vector<BasicBlockState> liveness_analysis(CADecoder *decoder, BPatch_function *function, BasicBlockStates& block_states)
{
    std::vector<BasicBlockState> states;

    BPatch_flowGraph *cfg = function->getCFG();

    std::vector<BPatch_basicBlock*> entry_blocks;
    if (!cfg->getEntryBasicBlock(entry_blocks))
    {
        ERROR(LOG_FILTER_CALL_TARGET, "\tCOULD NOT DETERMINE ENTRY BASIC_BLOCKS FOR FUNCTION");
    }
    else
    {
        for (auto entry_block : entry_blocks)
            states.push_back(liveness_analysis(decoder, entry_block, block_states));
    }

    return states;
}

static BasicBlockState liveness_analysis(CADecoder *decoder, BPatch_basicBlock *block, BasicBlockStates& block_states)
{
    auto &block_state = block_states[block->getStartAddress()];
    if (block_state.empty())
    {
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
                DEBUG(LOG_FILTER_CALL_TARGET, "Processing instruction %lx\n", address);

                if (decoder->is_indirect_call())
                {
                    TRACE(LOG_FILTER_CALL_TARGET, "decoder->is_indirect_call()\n");
                    auto range = decoder->get_register_range();
                    for (int reg_index = range.first; reg_index <= range.second; ++reg_index)
                    {
                        auto& reg_state = block_state[reg_index];

                        if (reg_state == REGISTER_UNTOUCHED)
                            reg_state = REGISTER_WRITE_BEFORE_READ;
                    }

                    change_possible = false;
                }
                else if (decoder->is_call())
                {
                    TRACE(LOG_FILTER_CALL_TARGET, "decoder->is_call()\n");
                    TRACE(LOG_FILTER_CALL_TARGET, "\t0x%lx\n", decoder->get_src(0));
                    
                    BPatch_Vector<BPatch_basicBlock*> targets;
                    block->getTargets(targets);

                    for (auto target : targets)
                    {
                        TRACE(LOG_FILTER_CALL_TARGET, "\t0x%lx\n", target->getStartAddress());
                    }
                    /*TODO: function_based analysis */
                }
                else if (decoder->is_return())
                {
                    change_possible = false;
                }
                else
                {
                    auto change_delta = false;
                    
                    //TODO: underestimate
                    auto register_state = decoder->get_register_state();
                    for (auto&& pair : register_state)
                    {
                        auto& reg_state = block_state[pair.first];
                        
                        if (reg_state == REGISTER_UNTOUCHED)
                        {
                            reg_state = pair.second;
                            change_delta |= (reg_state != REGISTER_UNTOUCHED);
                        }
                    }
                    
                    change_possible &= change_delta;
                }
            }
        }

        if (change_possible)
        {
            BPatch_Vector<BPatch_basicBlock*> targets;
            block->getTargets(targets);

            BasicBlockState target_state;
            for (auto target : targets)
            {
                // [C,R] C SUPER R
                // [C,W] *
                // [R,W] W
                
                auto state = liveness_analysis(decoder, target, block_states);

                if (target_state.empty())
                    target_state = state;
                else
                {
                    for (auto&& pair : state)
                    {
                        auto reg_state = target_state[pair.first];
                        if (reg_state == REGISTER_READ_BEFORE_WRITE && pair.second == REGISTER_UNTOUCHED)
                            reg_state = REGISTER_UNTOUCHED;

                        if (reg_state == REGISTER_READ_BEFORE_WRITE && pair.second == REGISTER_WRITE_BEFORE_READ)
                            reg_state = REGISTER_WRITE_BEFORE_READ;
                        
                        if ((reg_state == REGISTER_WRITE_BEFORE_READ && pair.second == REGISTER_UNTOUCHED) ||
                            (reg_state == REGISTER_UNTOUCHED && pair.second == REGISTER_WRITE_BEFORE_READ))
                            reg_state = REGISTER_WRITE_BEFORE_READ; // consider both: REGISTER_WRITE_BEFORE_READ | REGISTER_UNTOUCHED
                    }
                }
            }

            for (auto&& pair : target_state)
            {
                auto& reg_state = block_state[pair.first];

                if (reg_state == REGISTER_UNTOUCHED)
                    reg_state = pair.second;
            }
        }
    }
    return block_state;
}


std::vector<CallTarget> calltarget_analysis(BPatch_image *image, std::vector<BPatch_module *> *mods,
                                            BPatch_addressSpace *as, CADecoder *decoder,
                                            std::vector<TakenAddress> &taken_addresses)
{
    ERROR(LOG_FILTER_CALL_TARGET, "calltarget_analysis");

    std::vector<CallTarget> call_targets;

    for (auto module : *mods)
    {
        char modname[BUFFER_STRING_LEN];

        module->getFullName(modname, BUFFER_STRING_LEN);

        INFO(LOG_FILTER_CALL_TARGET, "Processing module %s\n", modname);

        // Instrument module
        char funcname[BUFFER_STRING_LEN];
        std::vector<BPatch_function *> *functions = module->getProcedures(true);
        Dyninst::Address start, end;

        for (auto function : *functions) {
            // Instrument function
            function->getName(funcname, BUFFER_STRING_LEN);
            function->getAddressRange(start, end);
            if (std::find_if(std::begin(taken_addresses), std::end(taken_addresses),
                             [start](TakenAddress const &taken_addr) { return start == taken_addr.deref_address; }) !=
                std::end(taken_addresses)) {
                auto params = function->getParams();
                auto ret_type = function->getReturnType();

                INFO(LOG_FILTER_CALL_TARGET, "<CT> Function %lx = (%s) %s\n", start, ret_type ? ret_type->getName() : "UNKNOWN", funcname);

                if (params) {
                    for (auto&& param : *params)
                        INFO(LOG_FILTER_CALL_TARGET, "\t%s (R %d)\n", param->getType()->getName(), param->getRegister());
                }
                else ERROR(LOG_FILTER_CALL_TARGET, "\tCOULD NOT DETERMINE PARAMS OR NO PARAMS");

                BasicBlockStates block_states;
                auto states = liveness_analysis(decoder, function, block_states);

                for (auto&& state : states)
                {
                    INFO(LOG_FILTER_CALL_TARGET, "\tREGISTER_STATE %s\n", RegisterStatesToString(decoder, state).c_str());
                }

                call_targets.emplace_back(CallTarget{std::string(modname), function});
            }
        }
    }

    return call_targets;
}

void dump_calltargets(std::vector<CallTarget> &targets)
{
    char funcname[BUFFER_STRING_LEN];
    Dyninst::Address start, end;

    for (auto &target : targets)
    {
        target.function->getName(funcname, BUFFER_STRING_LEN);
        target.function->getAddressRange(start, end);

        INFO(LOG_FILTER_CALL_TARGET, "<CT> %s: %s %lx -> %lx\n", target.module_name.c_str(), funcname, start, end);
    }
}
