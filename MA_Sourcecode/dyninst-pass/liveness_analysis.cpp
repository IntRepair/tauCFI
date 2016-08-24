#include "liveness_analysis.h"

#include <PatchCFG.h>
#include <BPatch_flowGraph.h>

using Dyninst::PatchAPI::PatchBlock;

template <> liveness_state_t convert_state(RegisterState reg_state)
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

template <> std::string to_string(liveness_state_t &reg_state)
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

LivenessRegisterStateMap liveness_init() { return {}; }

static LivenessRegisterStates merge_horizontal(std::vector<LivenessRegisterStates> states)
{
    LivenessRegisterStates target_state;

    if (states.size() > 0)
    {
        target_state = std::accumulate(
            ++(states.begin()), states.end(), states.front(),
            [](LivenessRegisterStates state, LivenessRegisterStates next) {
                return transform(state, next, [](liveness_state_t current, liveness_state_t delta) {
                    if (current == REGISTER_READ_BEFORE_WRITE && delta == REGISTER_CLEAR)
                        current = REGISTER_CLEAR;
                    else if (current == REGISTER_READ_BEFORE_WRITE &&
                             delta == REGISTER_WRITE_BEFORE_READ)
                        current = REGISTER_WRITE_BEFORE_READ;
                    else if ((current == REGISTER_WRITE_BEFORE_READ && delta == REGISTER_CLEAR) ||
                             (current == REGISTER_CLEAR && delta == REGISTER_WRITE_BEFORE_READ))
                        current = REGISTER_WRITE_BEFORE_READ; // consider both:
                                                              // REGISTER_WRITE_BEFORE_READ |
                                                              // REGISTER_CLEAR
                    return current;
                });
            });
    }
    return target_state;
}

static LivenessRegisterStates merge_vertical(LivenessRegisterStates current,
                                             LivenessRegisterStates delta)
{
    return transform(current, delta, [](liveness_state_t current, liveness_state_t delta) {
        return (current == REGISTER_CLEAR) ? delta : current;
    });
}

LivenessRegisterStates liveness_analysis(CADecoder *decoder, BPatch_image *image,
                                         std::vector<BPatch_basicBlock *> blocks,
                                         LivenessRegisterStateMap &block_states)
{
    std::vector<LivenessRegisterStates> states;

    for (auto block : blocks)
        states.push_back(liveness_analysis(decoder, image, block, block_states));

    return merge_horizontal(states);
}

LivenessRegisterStates liveness_analysis(CADecoder *decoder, BPatch_image *image,
                                         BPatch_function *function,
                                         LivenessRegisterStateMap &block_states)
{
    char funcname[BUFFER_STRING_LEN];
    function->getName(funcname, BUFFER_STRING_LEN);

    Dyninst::Address start, end;
    function->getAddressRange(start, end);

    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\tANALYZING FUNCTION %s [%lx:0x%lx[", funcname, start,
          end);
    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\tFUN+----------------------------------------- %s",
          funcname);

    LivenessRegisterStates state;

    BPatch_flowGraph *cfg = function->getCFG();

    std::vector<BPatch_basicBlock *> entry_blocks;
    if (!cfg->getEntryBasicBlock(entry_blocks))
    {
        TRACE(LOG_FILTER_LIVENESS_ANALYSIS,
              "\tCOULD NOT DETERMINE ENTRY BASIC_BLOCKS FOR FUNCTION");
    }
    else
    {
        TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\tProcessing Entry Blocks");
        state = liveness_analysis(decoder, image, entry_blocks, block_states);
    }

    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\tFUN------------------------------------------ %s",
          funcname);
    return state;
}

LivenessRegisterStates liveness_analysis(CADecoder *decoder, BPatch_image *image,
                                         BPatch_basicBlock *block,
                                         LivenessRegisterStateMap &block_states)
{
    auto bb_start_address = block->getStartAddress();
    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "Processing BasicBlock %lx", bb_start_address);

    auto itr_bool = block_states.insert({bb_start_address, LivenessRegisterStates()});
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
                    auto function = image->findFunction(decoder->get_src(0));

                    auto delta_state = liveness_analysis(decoder, image, function, block_states);
                    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t PREVIOUS STATE %s",
                          to_string(block_state).c_str());
                    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t DELTA STATE %s",
                          to_string(delta_state).c_str());

                    block_state = merge_vertical(block_state, delta_state);
                    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t RESULT STATE %s",
                          to_string(block_state).c_str());
                }
                else if (decoder->is_return())
                {
                    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\tInstruction is a return");
                    change_possible = false;
                }
                else if (decoder->is_constant_write())
                {
                    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\tInstruction is a constant write");

                    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t PREVIOUS STATE %s",
                          to_string(block_state).c_str());

                    auto register_states = decoder->get_register_state();
                    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t DECODER STATE %s",
                          to_string(register_states).c_str());
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
                    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t DELTA STATE %s",
                          to_string(delta_state).c_str());

                    block_state = merge_vertical(block_state, delta_state);
                    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t RESULT STATE %s",
                          to_string(block_state).c_str());

                    change_possible = block_state.state_exists(REGISTER_CLEAR);
                }
                else
                {
                    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t PREVIOUS STATE %s",
                          to_string(block_state).c_str());

                    auto register_states = decoder->get_register_state();
                    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t DECODER STATE %s",
                          to_string(register_states).c_str());

                    auto delta_state = convert_states<liveness_state_t>(register_states);
                    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t DELTA STATE %s",
                          to_string(delta_state).c_str());

                    block_state = merge_vertical(block_state, delta_state);
                    TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t RESULT STATE %s",
                          to_string(block_state).c_str());

                    change_possible = block_state.state_exists(REGISTER_CLEAR);
                }
            }
        }

        if (change_possible)
        {
            BPatch_Vector<BPatch_basicBlock *> targets;
            block->getTargets(targets);

            TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "Final merging for BasicBlock %lx",
                  bb_start_address);

            TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t PREVIOUS STATE %s",
                  to_string(block_state).c_str());

            auto delta_state = liveness_analysis(decoder, image, targets, block_states);

            TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t DELTA STATE %s",
                  to_string(delta_state).c_str());

            block_state = merge_vertical(block_state, delta_state);
            TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t RESULT STATE %s",
                  to_string(block_state).c_str());
        }
    }
    return block_state;
}
