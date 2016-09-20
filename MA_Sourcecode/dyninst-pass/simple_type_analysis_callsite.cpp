#include "simple_type_analysis.h"

#include <BPatch_flowGraph.h>
#include <PatchCFG.h>

#include <boost/range/adaptor/reversed.hpp>

namespace type_analysis
{
namespace simple
{
namespace callsite
{
using Dyninst::PatchAPI::PatchBlock;

RegisterStateMap init() { return {}; }

static RegisterStates merge_horizontal(std::vector<RegisterStates> states)
{
    RegisterStates target_state;
    if (states.size() > 0)
    {
        target_state = std::accumulate(++(states.begin()), states.end(), states.front(),
                                       [](RegisterStates current, RegisterStates delta) {
                                           return transform(current, delta,
                                                            [](state_t current, state_t delta) {
                                                                return std::max(current, delta);
                                                            });
                                       });
    }
    return target_state;
}

static RegisterStates merge_vertical(RegisterStates current, RegisterStates delta)
{
    return transform(current, delta, [](state_t current, state_t delta) {
        return (current == SIMPLE_TYPE_UNKNOWN) ? delta : current;
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

    std::vector<BPatch_basicBlock *> exit_blocks;
    if (!cfg->getExitBasicBlock(exit_blocks))
    {
        LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\tCOULD NOT DETERMINE EXIT BASIC_BLOCKS FOR FUNCTION");
    }
    else
    {
        LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\tProcessing Exit Blocks");

        std::vector<RegisterStates> states;

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
                    states.push_back(analysis(decoder, image, block, address, block_states));
            }
        }

        state = merge_horizontal(states);
    }

    LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\tFUN------------------------------------------ %s",
              funcname);
    return state;
}

static void process_instruction(CADecoder *decoder,
                                PatchBlock::Insns::value_type const &instruction,
                                uint64_t end_address, RegisterStates &block_state,
                                bool &change_possible)
{
    auto address = reinterpret_cast<uint64_t>(instruction.first);
    Instruction::Ptr instruction_ptr = instruction.second;
    LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\tLooking at instruction %lx", address);

    if (change_possible && ((address < end_address) || !end_address))
    {
        decoder->decode(address, instruction_ptr);
        LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\tProcessing instruction %lx", address);

        if (decoder->is_indirect_call())
        {
            LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\tInstruction is an indirect call");
            change_possible = false; // No need to implement trashing, as this type analysis is a
                                     // rather simplistic implementation
        }
        else if (decoder->is_call())
        {
            LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\tInstruction is a direct call");
            change_possible = false; // No need to implement trashing, as this type analysis is a
                                     // rather simplistic implementation
        }
        else if (decoder->is_return())
        {
            LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\tInstruction is a return");
            change_possible = false; // No need to implement trashing, as this type analysis is a
                                     // rather simplistic implementation
        }
        else
        {
            LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\tInstruction is not a special case");

            std::vector<Operand> operands;
            instruction_ptr->getOperands(operands);
            for (auto const &operand : operands)
            {
                LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "%s",
                          operand.format(instruction_ptr->getArch()).c_str());
            }

            decoder->operand_information();

            /*
                        LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\t PREVIOUS STATE %s",
                              to_string(block_state).c_str());
                        auto register_states = decoder->get_register_state();
                        LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\t DECODER STATE %s",
                              to_string(register_states).c_str());
                        auto state_delta = convert_states<reaching_state_t>(register_states);
                        LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\t DELTA STATE %s",
                              to_string(state_delta).c_str());
                        block_state = merge_vertical(block_state, state_delta);
                        LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\t RESULT STATE %s",
                              to_string(block_state).c_str());
            */
            change_possible = block_state.state_exists(SIMPLE_TYPE_UNKNOWN);
        }
    }
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
        LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\tBB+----------------------------------------- %lx",
                  bb_start_address);
        LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "Information not in Storage");
        PatchBlock::Insns insns;
        Dyninst::PatchAPI::convert(block)->getInsns(insns);

        bool change_possible = true;

        for (auto &instruction : boost::adaptors::reverse(insns))
        {
            process_instruction(decoder, instruction, 0, block_state, change_possible);
        }

        if (change_possible)
        {
            BPatch_Vector<BPatch_basicBlock *> sources;
            block->getSources(sources);

            LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "Final merging for BasicBlock %lx",
                      bb_start_address);

            LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\t PREVIOUS STATE %s",
                      to_string(block_state).c_str());

            auto delta_state = analysis(decoder, image, sources, block_states);

            LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\t DELTA STATE %s",
                      to_string(delta_state).c_str());

            block_state = merge_vertical(block_state, delta_state);
            LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\t RESULT STATE %s",
                      to_string(block_state).c_str());
        }
        LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\tBB------------------------------------------ %lx",
                  bb_start_address);
    }
    return block_state;
}

RegisterStates analysis(CADecoder *decoder, BPatch_image *image, BPatch_basicBlock *block,
                        Dyninst::Address end_address, RegisterStateMap &block_states)
{
    auto bb_start_address = block->getStartAddress();
    LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "Processing History of BasicBlock %lx before reaching %lx",
              bb_start_address, end_address);

    LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS,
              "BB_History+----------------------------------------- %lx : %lx", bb_start_address,
              end_address);

    RegisterStates block_state;
    PatchBlock::Insns insns;
    Dyninst::PatchAPI::convert(block)->getInsns(insns);

    bool change_possible = true;

    for (auto &instruction : boost::adaptors::reverse(insns))
    {
        process_instruction(decoder, instruction, end_address, block_state, change_possible);
    }

    if (change_possible)
    {
        BPatch_Vector<BPatch_basicBlock *> sources;
        block->getSources(sources);

        LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "Final merging for BasicBlock History %lx : %lx",
                  bb_start_address, end_address);

        LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\t PREVIOUS STATE %s", to_string(block_state).c_str());

        auto delta_state = analysis(decoder, image, sources, block_states);

        LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\t DELTA STATE %s", to_string(delta_state).c_str());

        block_state = merge_vertical(block_state, delta_state);
        LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS, "\t RESULT STATE %s", to_string(block_state).c_str());
    }
    LOG_TRACE(LOG_FILTER_TYPE_ANALYSIS,
              "BB_History------------------------------------------ %lx : %lx", bb_start_address,
              end_address);

    return block_state;
}
};
};
};
