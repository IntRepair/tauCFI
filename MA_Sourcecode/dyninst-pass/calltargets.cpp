#include "calltargets.h"
#include "logging.h"

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
    REGISTER_CLEAR,
    REGISTER_READ_BEFORE_WRITE,
    REGISTER_WRITE_BEFORE_READ,
    REGISTER_WRITE_OR_UNTOUCHED,
};

static register_state_t convert_state(RegisterState reg_state)
{
    switch (reg_state)
    {
    case REGISTER_UNTOUCHED:
        return REGISTER_CLEAR;
    case REGISTER_READ:
        return REGISTER_READ_BEFORE_WRITE;
    case REGISTER_WRITE:
        return REGISTER_WRITE_BEFORE_READ;
    case REGISTER_READ_WRITE:
        return REGISTER_WRITE_BEFORE_READ;
    }
}

static std::string RegisterStateToString(register_state_t reg_state)
{
    switch (reg_state)
    {
    case REGISTER_CLEAR:
        return "C";
    case REGISTER_READ_BEFORE_WRITE:
        return "R";
    case REGISTER_WRITE_BEFORE_READ:
        return "W";
    case REGISTER_WRITE_OR_UNTOUCHED:
        return "*";
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
                    if (reg_state == REGISTER_READ_BEFORE_WRITE && pair.second == REGISTER_CLEAR)
                        reg_state = REGISTER_CLEAR;
                    if (reg_state == REGISTER_READ_BEFORE_WRITE && pair.second == REGISTER_WRITE_BEFORE_READ)
                        reg_state = REGISTER_WRITE_BEFORE_READ;
                    if ((reg_state == REGISTER_WRITE_BEFORE_READ && pair.second == REGISTER_CLEAR) ||
                        (reg_state == REGISTER_CLEAR && pair.second == REGISTER_WRITE_BEFORE_READ))
                        reg_state =
                            REGISTER_WRITE_BEFORE_READ; // consider both: REGISTER_WRITE_BEFORE_READ | REGISTER_CLEAR
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

        if (reg_state == REGISTER_CLEAR)
            reg_state = pair.second;
    }

    return current;
}

static BasicBlockState liveness_analysis(CADecoder *decoder, BPatch_addressSpace *as, BPatch_basicBlock *block,
                                         BasicBlockStates &block_states);

static BasicBlockState liveness_analysis(CADecoder *decoder, BPatch_addressSpace *as,
                                         std::vector<BPatch_basicBlock *> blocks, BasicBlockStates &block_states)
{
    std::vector<BasicBlockState> states;

    for (auto block : blocks)
        states.push_back(liveness_analysis(decoder, as, block, block_states));

    return merge_horizontal(states);
}

static BasicBlockState liveness_analysis(CADecoder *decoder, BPatch_addressSpace *as, BPatch_function *function,
                                         BasicBlockStates &block_states)
{
    char funcname[BUFFER_STRING_LEN];
    function->getName(funcname, BUFFER_STRING_LEN);

    Dyninst::Address start, end;
    function->getAddressRange(start, end);

    TRACE(LOG_FILTER_CALL_TARGET, "\tANALYZING FUNCTION %s [%lx:0x%lx[\n", funcname, start, end);
    TRACE(LOG_FILTER_CALL_TARGET, "\tFUN+----------------------------------------- %s \n", funcname);

    BasicBlockState state;

    BPatch_flowGraph *cfg = function->getCFG();

    std::vector<BPatch_basicBlock *> entry_blocks;
    if (!cfg->getEntryBasicBlock(entry_blocks))
    {
        TRACE(LOG_FILTER_CALL_TARGET, "\tCOULD NOT DETERMINE ENTRY BASIC_BLOCKS FOR FUNCTION");
    }
    else
    {
        TRACE(LOG_FILTER_CALL_TARGET, "\tProcessing Entry Blocks\n");
        state = liveness_analysis(decoder, as, entry_blocks, block_states);
    }

    TRACE(LOG_FILTER_CALL_TARGET, "\tFUN------------------------------------------ %s \n", funcname);
    return state;
}

static BasicBlockState liveness_analysis(CADecoder *decoder, BPatch_addressSpace *as, BPatch_basicBlock *block,
                                         BasicBlockStates &block_states)
{
    auto bb_start_address = block->getStartAddress();
    TRACE(LOG_FILTER_CALL_TARGET, "Processing BasicBlock %lx\n", bb_start_address);

    auto &block_state = block_states[bb_start_address];
    if (block_state.empty())
    {
        TRACE(LOG_FILTER_CALL_TARGET, "Information not in Storage\n");
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
                TRACE(LOG_FILTER_CALL_TARGET, "\tProcessing instruction %lx\n", address);

                if (decoder->is_indirect_call())
                {
                    TRACE(LOG_FILTER_CALL_TARGET, "\tInstruction is an indirect call\n");
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
                    TRACE(LOG_FILTER_CALL_TARGET, "\tInstruction is a direct call\n");
                    auto function = as->findFunctionByAddr((void *)decoder->get_src(0));

                    auto delta_state = liveness_analysis(decoder, as, function, block_states);
                    TRACE(LOG_FILTER_CALL_TARGET, "\t PREVIOUS STATE %s\n",
                          RegisterStatesToString(decoder, block_state).c_str());
                    TRACE(LOG_FILTER_CALL_TARGET, "\t DELTA STATE %s\n",
                          RegisterStatesToString(decoder, delta_state).c_str());

                    block_state = merge_vertical(block_state, delta_state);
                    TRACE(LOG_FILTER_CALL_TARGET, "\t RESULT STATE %s\n",
                          RegisterStatesToString(decoder, block_state).c_str());
                }
                else if (decoder->is_return())
                {
                    TRACE(LOG_FILTER_CALL_TARGET, "\t return");
                    change_possible = false;
                }
                else
                {
                    TRACE(LOG_FILTER_CALL_TARGET, "\t PREVIOUS STATE %s\n",
                          RegisterStatesToString(decoder, block_state).c_str());

                    auto register_states = decoder->get_register_state();
                    TRACE(LOG_FILTER_CALL_TARGET, "\t DECODER STATE %s\n",
                          RegisterStatesToString(decoder, register_states).c_str());

                    auto delta_state = convert_states(register_states);
                    TRACE(LOG_FILTER_CALL_TARGET, "\t DELTA STATE %s\n",
                          RegisterStatesToString(decoder, delta_state).c_str());

                    block_state = merge_vertical(block_state, delta_state);
                    TRACE(LOG_FILTER_CALL_TARGET, "\t RESULT STATE %s\n",
                          RegisterStatesToString(decoder, block_state).c_str());

                    change_possible = (std::end(block_state) !=
                                       std::find_if(std::begin(block_state), std::end(block_state),
                                                    [](std::pair<const uint64_t, register_state_t> const &reg_state) {
                                                        return reg_state.second == REGISTER_CLEAR;
                                                    }));
                }
            }
        }

        if (change_possible)
        {
            BPatch_Vector<BPatch_basicBlock *> targets;
            block->getTargets(targets);

            TRACE(LOG_FILTER_CALL_TARGET, "Final merging for BasicBlock %lx\n", bb_start_address);

            TRACE(LOG_FILTER_CALL_TARGET, "\t PREVIOUS STATE %s\n",
                  RegisterStatesToString(decoder, block_state).c_str());

            auto delta_state = liveness_analysis(decoder, as, targets, block_states);

            TRACE(LOG_FILTER_CALL_TARGET, "\t DELTA STATE %s\n", RegisterStatesToString(decoder, delta_state).c_str());

            block_state = merge_vertical(block_state, delta_state);
            TRACE(LOG_FILTER_CALL_TARGET, "\t RESULT STATE %s\n", RegisterStatesToString(decoder, block_state).c_str());
        }
    }
    return block_state;
}

std::vector<CallTarget> calltarget_analysis(BPatch_image *image, std::vector<BPatch_module *> *mods,
                                            BPatch_addressSpace *as, CADecoder *decoder,
                                            std::vector<TakenAddress> &taken_addresses)
{
    ERROR(LOG_FILTER_CALL_TARGET, "calltarget_analysis\n");

    std::vector<CallTarget> call_targets;

    BasicBlockStates block_states;

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
                else ERROR(LOG_FILTER_CALL_TARGET, "\tCOULD NOT DETERMINE PARAMS OR NO PARAMS\n");

                auto state = liveness_analysis(decoder, as, function, block_states);
                INFO(LOG_FILTER_CALL_TARGET, "\tREGISTER_STATE %s\n", RegisterStatesToString(decoder, state).c_str());

                call_targets.emplace_back(CallTarget{std::string(modname), function, RegisterStatesToString(decoder, state)});
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
