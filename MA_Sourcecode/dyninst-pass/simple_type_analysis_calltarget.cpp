#include "simple_type_analysis.h"

#include <BPatch_flowGraph.h>
#include <PatchCFG.h>

namespace type_analysis
{
namespace simple
{

namespace calltarget
{
using Dyninst::PatchAPI::PatchBlock;

RegisterStateMap init() { return {}; }

static RegisterStates merge_horizontal(std::vector<RegisterStates> states)
{
    RegisterStates target_state;

    if (states.size() > 0)
    {
        target_state =
            std::accumulate(++(states.begin()), states.end(), states.front(),
                            [](RegisterStates state, RegisterStates next) {
                                return transform(state, next, [](state_t current, state_t delta) {
                                    return std::make_pair(current.first || delta.second,
                                                          std::min(current.second, delta.second));
                                    // is this ok with unknown ? maybe fix afterwards with UNKNOWN
                                    // == W0 ? is the
                                    // locked par ok ?
                                    // move unknown to sth like loked ?
                                });
                            });
    }
    return target_state;
}

static RegisterStates merge_vertical(RegisterStates current, RegisterStates delta)
{
    return transform(current, delta, [](state_t current, state_t delta) {
        if (current.first)
            return current;
        if (current.second == SIMPLE_TYPE_UNKNOWN)
            return delta;

        return std::make_pair(delta.first, std::max(current.second, delta.second));
    });
}

RegisterStates analysis(CADecoder *decoder, BPatch_image *image,
                        std::vector<BPatch_basicBlock *> blocks, RegisterStateMap &block_states)
{
    std::vector<RegisterStates> states;

    for (auto block : blocks)
        states.push_back(analysis(decoder, image, block, block_states));

    return merge_horizontal(states);
}

RegisterStates analysis(CADecoder *decoder, BPatch_image *image, BPatch_function *function,
                        RegisterStateMap &block_states)
{
    char funcname[BUFFER_STRING_LEN];
    function->getName(funcname, BUFFER_STRING_LEN);

    Dyninst::Address start, end;
    function->getAddressRange(start, end);

    LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\tANALYZING FUNCTION %s [%lx:0x%lx[", funcname, start,
              end);
    LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\tFUN+----------------------------------------- %s",
              funcname);

    RegisterStates state;

    BPatch_flowGraph *cfg = function->getCFG();

    std::vector<BPatch_basicBlock *> entry_blocks;
    if (!cfg->getEntryBasicBlock(entry_blocks))
    {
        LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS,
                  "\tCOULD NOT DETERMINE ENTRY BASIC_BLOCKS FOR FUNCTION");
    }
    else
    {
        LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\tProcessing Entry Blocks");
        state = analysis(decoder, image, entry_blocks, block_states);
    }

    LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\tFUN------------------------------------------ %s",
              funcname);
    return state;
}

RegisterStates analysis(CADecoder *decoder, BPatch_image *image, BPatch_basicBlock *block,
                        RegisterStateMap &block_states)
{
    auto bb_start_address = block->getStartAddress();
    LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "Processing BasicBlock %lx", bb_start_address);

    auto itr_bool = block_states.insert({bb_start_address, RegisterStates()});
    auto &block_state = itr_bool.first->second;

    if (itr_bool.second)
    {
        LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "Information not in Storage");
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
                LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\tProcessing instruction %lx", address);

                if (decoder->is_indirect_call())
                {
                    LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\tInstruction is an indirect call");

                    /*                    auto range = decoder->get_register_range();
                                        for (int reg_index = range.first; reg_index <= range.second;
                       ++reg_index)
                                        {
                                            auto &reg_state = block_state[reg_index];

                                            if (reg_state == REGISTER_CLEAR)
                                                reg_state = REGISTER_WRITE_BEFORE_READ;
                                        }
                                        */

                    change_possible = false;
                }
                else if (decoder->is_call())
                {
                    LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\tInstruction is a direct call");
                    /*                    auto function = image->findFunction(decoder->get_src(0));

                                        auto delta_state = analysis(decoder, image, function,
                       block_states);
                                        LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\t PREVIOUS STATE
                       %s",
                                              to_string(block_state).c_str());
                                        LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\t DELTA STATE %s",
                                              to_string(delta_state).c_str());

                                        block_state = merge_vertical(block_state, delta_state);
                                        LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\t RESULT STATE
                       %s",
                                              to_string(block_state).c_str());
                    */
                }
                else if (decoder->is_return())
                {
                    LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\tInstruction is a return");
                    change_possible = false;
                }
                /*                else if (decoder->is_constant_write())
                                {
                                    LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\tInstruction is a
                   constant
                   write");

                                    LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\t PREVIOUS STATE %s",
                                          to_string(block_state).c_str());

                                    auto register_states = decoder->get_register_state();
                                    LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\t DECODER STATE %s",
                                          to_string(register_states).c_str());
                                    register_states = transform(register_states, [](RegisterState
                   state) {
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

                                    LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\t SANITIZED DECODER
                   STATE
                   %s",
                                          to_string(register_states).c_str());

                                    auto delta_state =
                   convert_states<state_t>(register_states);
                                    LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\t DELTA STATE %s",
                                          to_string(delta_state).c_str());

                                    block_state = merge_vertical(block_state, delta_state);
                                    LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\t RESULT STATE %s",
                                          to_string(block_state).c_str());

                                    change_possible = block_state.state_exists(REGISTER_CLEAR);
                                }
                                */
                else
                {

                    std::vector<Operand> operands;
                    instruction_ptr->getOperands(operands);
                    for (auto const &operand : operands)
                    {
                        LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "%s",
                                  operand.format(instruction_ptr->getArch()).c_str());
                    }

                    decoder->operand_information();
                    /*                    LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\t PREVIOUS STATE
                       %s",
                                              to_string(block_state).c_str());

                                        auto register_states = decoder->get_register_state();
                                        LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\t DECODER STATE
                       %s",
                                              to_string(register_states).c_str());

                                        auto delta_state =
                       convert_states<state_t>(register_states);
                                        LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\t DELTA STATE %s",
                                              to_string(delta_state).c_str());

                                        block_state = merge_vertical(block_state, delta_state);
                                        LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\t RESULT STATE
                       %s",
                                              to_string(block_state).c_str());
                    */
                    change_possible =
                        block_state.state_exists(std::make_pair(false, SIMPLE_TYPE_UNKNOWN)) ||
                        block_state.state_exists(std::make_pair(true, SIMPLE_TYPE_UNKNOWN));
                }
            }
        }

        if (change_possible)
        {
            BPatch_Vector<BPatch_basicBlock *> targets;
            block->getTargets(targets);

            LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "Final merging for BasicBlock %lx",
                      bb_start_address);

            LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\t PREVIOUS STATE %s",
                      to_string(block_state).c_str());

            auto delta_state = analysis(decoder, image, targets, block_states);

            LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\t DELTA STATE %s",
                      to_string(delta_state).c_str());

            block_state = merge_vertical(block_state, delta_state);
            LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\t RESULT STATE %s",
                      to_string(block_state).c_str());
        }
    }
    return block_state;
}
};
};
};
