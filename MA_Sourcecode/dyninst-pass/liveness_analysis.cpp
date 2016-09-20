#include "liveness_analysis.h"

#include "instrumentation.h"

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
        return liveness::REGISTER_READ_BEFORE_WRITE;
    case REGISTER_WRITE:
        return liveness::REGISTER_WRITE_BEFORE_READ;
    case REGISTER_READ_WRITE:
        return liveness::REGISTER_WRITE_BEFORE_READ;
    }
}

namespace liveness
{

static RegisterStates merge_horizontal(std::vector<RegisterStates> states)
{
    RegisterStates target_state;

    if (states.size() > 0)
    {
        target_state = std::accumulate(
            ++(states.begin()), states.end(), states.front(),
            [](RegisterStates state, RegisterStates next) {
                return transform(state, next, [](liveness::state_t current,
                                                 liveness::state_t delta) {
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

static RegisterStates merge_vertical(RegisterStates current, RegisterStates delta)
{
    return transform(current, delta, [](liveness::state_t current, liveness::state_t delta) {
        return (current == REGISTER_CLEAR) ? delta : current;
    });
}

static bool is_xmm_passthrough_block(CADecoder *decoder, BPatch_basicBlock *block,
                                     uint64_t init_disp)
{
    auto prev = -1;
    auto disp = 0ul;

    using Insns = Dyninst::PatchAPI::PatchBlock::Insns;
    instrument_basicBlock_instructions(block, [&](Insns::value_type &instruction) {
        auto address = reinterpret_cast<uint64_t>(instruction.first);
        Instruction::Ptr instruction_ptr = instruction.second;

        decoder->decode(address, instruction_ptr);
        if (!decoder->is_target_stackpointer_disp(0))
        {
            prev = -2;
            return;
        }
        else if (prev == -1 && decoder->is_reg_source(0, DR_REG_XMM0))
        {
            prev++;
            disp = decoder->get_target_disp(0);
            init_disp = disp;
        }
        else if ((disp += 0x10) != decoder->get_target_disp(0))
        {
            prev = -2;
            return;
        }
        else if (prev == 0 && decoder->is_reg_source(0, DR_REG_XMM1))
            prev++;
        else if (prev == 1 && decoder->is_reg_source(0, DR_REG_XMM2))
            prev++;
        else if (prev == 2 && decoder->is_reg_source(0, DR_REG_XMM3))
            prev++;
        else if (prev == 3 && decoder->is_reg_source(0, DR_REG_XMM4))
            prev++;
        else if (prev == 4 && decoder->is_reg_source(0, DR_REG_XMM5))
            prev++;
        else if (prev == 5 && decoder->is_reg_source(0, DR_REG_XMM6))
            prev++;
        else if (prev == 6 && decoder->is_reg_source(0, DR_REG_XMM7))
            prev++;
        else
        {
            prev = -2;
            return;
        }
    });

    return (prev == 7);
}

static std::size_t calculate_ignore_instructions(AnalysisConfig &config,
                                                 BPatch_basicBlock *init_block, uint64_t init_disp)
{

    auto decoder = config.decoder;
    auto &ignore_instructions = config.ignore_instructions;

    auto reg_index = DR_REG_R9; // todo use system_abi for this
    auto done = false;

    using Insns = Dyninst::PatchAPI::PatchBlock::Insns;
    instrument_basicBlock_instructions(init_block, [&](Insns::value_type &instruction) {
        if (done)
            return;

        auto address = reinterpret_cast<uint64_t>(instruction.first);
        Instruction::Ptr instruction_ptr = instruction.second;

        decoder->decode(address, instruction_ptr);

        if (decoder->is_target_stackpointer_disp(0) && decoder->is_reg_source(0, reg_index))
        {

            LOG_INFO(LOG_FILTER_LIVENESS_ANALYSIS, "Ignore instruction %lx", address);
            ignore_instructions.insert(address);

            switch (reg_index)
            {
            case DR_REG_R9:
            {
                reg_index = DR_REG_R8;
                break;
            }
            case DR_REG_R8:
            {
                reg_index = DR_REG_RCX;
                break;
            }
            case DR_REG_RCX:
            {
                reg_index = DR_REG_RDX;
                break;
            }
            case DR_REG_RDX:
            {
                reg_index = DR_REG_RSI;
                break;
            }
            case DR_REG_RSI:
            {
                reg_index = DR_REG_RDI;
                break;
            }
            case DR_REG_RDI:
                done = true;
                return;
            }
        }
        else if (reg_index != DR_REG_R9)
            done = true;
    });
}

static void initialize_variadic_passthrough(AnalysisConfig &config, BPatch_object *object)
{
    instrument_object_basicBlocks_unordered(object, [&](BPatch_basicBlock *block) {
        uint64_t init_disp;
        if (is_xmm_passthrough_block(config.decoder, block, init_disp))
        {
            LOG_INFO(LOG_FILTER_LIVENESS_ANALYSIS,
                     "BasicBlock [%lx : %lx[ is an xmm passthrough block", block->getStartAddress(),
                     block->getEndAddress());

            BPatch_Vector<BPatch_basicBlock *> xmm_targets;
            block->getTargets(xmm_targets);

            BPatch_Vector<BPatch_basicBlock *> xmm_sources;
            block->getSources(xmm_sources);

            if (xmm_targets.size() == 1 && xmm_sources.size() == 1)
            {
                auto init_block = xmm_sources[0];
                auto reg_block = xmm_targets[0];

                LOG_INFO(LOG_FILTER_LIVENESS_ANALYSIS,
                         "BasicBlock [%lx : %lx[ is an varargs passthrough init block",
                         init_block->getStartAddress(), init_block->getEndAddress());

                BPatch_Vector<BPatch_basicBlock *> init_targets;
                init_block->getTargets(init_targets);

                if (init_targets.size() == 2)
                {
                    if (init_targets[0] == reg_block || init_targets[1] == reg_block)
                    {
                        LOG_INFO(LOG_FILTER_LIVENESS_ANALYSIS,
                                 "BasicBlock [%lx : %lx[ is an varargs passthrough reg block",
                                 init_block->getStartAddress(), init_block->getEndAddress());

                        calculate_ignore_instructions(config, reg_block, init_disp);
                    }
                }
            }
        }
    });
}

namespace calltarget
{
AnalysisConfig init(CADecoder *decoder, BPatch_image *image, BPatch_object *object)
{
    AnalysisConfig config;

    config.decoder = decoder;
    config.image = image;
    config.merge_vertical = &merge_vertical;
    config.merge_horizontal = &merge_horizontal;

    initialize_variadic_passthrough(config, object);

    return config;
}
};

namespace callsite
{
AnalysisConfig init(CADecoder *decoder, BPatch_image *image, BPatch_object *object)
{
    AnalysisConfig config;

    config.decoder = decoder;
    config.image = image;
    config.merge_vertical = &merge_vertical;
    config.merge_horizontal = &merge_horizontal;

    return config;
}
};
RegisterStates analysis(AnalysisConfig &config, std::vector<BPatch_basicBlock *> blocks)
{
    std::vector<RegisterStates> states;

    for (auto block : blocks)
        states.push_back(analysis(config, block));

    return (*config.merge_horizontal)(states);
}

RegisterStates analysis(AnalysisConfig &config, BPatch_function *function)
{
    char funcname[BUFFER_STRING_LEN];
    function->getName(funcname, BUFFER_STRING_LEN);

    Dyninst::Address start, end;
    function->getAddressRange(start, end);

    LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\tANALYZING FUNCTION %s [%lx:0x%lx[", funcname, start,
              end);
    LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\tFUN+----------------------------------------- %s",
              funcname);

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

    LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\tFUN------------------------------------------ %s",
              funcname);
    return state;
}

RegisterStates analysis(AnalysisConfig &config, BPatch_basicBlock *block)
{
    auto bb_start_address = block->getStartAddress();
    LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "Processing BasicBlock %lx", bb_start_address);

    auto &block_states = config.block_states;
    auto itr_bool = block_states.insert({bb_start_address, RegisterStates()});
    auto &block_state = itr_bool.first->second;
    auto &ignore_instructions = config.ignore_instructions;

    if (itr_bool.second)
    {
        LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "Information not in Storage");
        PatchBlock::Insns insns;
        Dyninst::PatchAPI::convert(block)->getInsns(insns);

        bool change_possible = true;
        auto decoder = config.decoder;

        for (auto &instruction : insns)
        {
            auto address = reinterpret_cast<uint64_t>(instruction.first);
            Instruction::Ptr instruction_ptr = instruction.second;

            if (ignore_instructions.count(address) > 0)
            {
                LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\tIgnoring instruction %lx", address);
            }
            else if (change_possible)
            {

                decoder->decode(address, instruction_ptr);
                LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\tProcessing instruction %lx", address);

                if (decoder->is_indirect_call())
                {
                    // TODO: We need to add the read from the indirect call to the
                    // liveness set
                    LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\tInstruction is an indirect call");
                    auto range = decoder->get_register_range();
                    for (int reg_index = range.first; reg_index <= range.second; ++reg_index)
                    {
                        auto &reg_state = block_state[reg_index];

                        if (reg_state == REGISTER_CLEAR)
                            reg_state = REGISTER_WRITE_BEFORE_READ;
                    }

                    change_possible = false;
                }
                /* TODO: should we follow direct calls ? (if we should, how should we
                   proceed)
                    without -> no problematic call_target analysis results
                    with -> problematic call_target analysis results
                    probably more advanced analysis (aka, which register depends on
                   which (?))
                                else if (decoder->is_call())
                                {
                                    LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS,
                   "\tInstruction is a direct call");
                                    auto function =
                   image->findFunction(decoder->get_src(0));

                                    auto delta_state = analysis(config, function);
                                    LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t PREVIOUS
                   STATE %s",
                                              to_string(block_state).c_str());
                                    LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t DELTA
                   STATE %s",
                                              to_string(delta_state).c_str());

                                    block_state = (*config.merge_vertical)(block_state,
                   delta_state);
                                    LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t RESULT
                   STATE %s",
                                              to_string(block_state).c_str());
                                }*/
                else if (decoder->is_return())
                {
                    LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\tInstruction is a return");
                    change_possible = false;
                }
                else if (decoder->is_constant_write())
                {
                    LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\tInstruction is a constant write");

                    LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t PREVIOUS STATE %s",
                              to_string(block_state).c_str());

                    auto register_states = decoder->get_register_state();
                    LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t DECODER STATE %s",
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

                    LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t SANITIZED DECODER STATE %s",
                              to_string(register_states).c_str());

                    auto delta_state = convert_states<liveness::state_t>(register_states);
                    LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t DELTA STATE %s",
                              to_string(delta_state).c_str());

                    block_state = (*config.merge_vertical)(block_state, delta_state);
                    LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t RESULT STATE %s",
                              to_string(block_state).c_str());

                    change_possible = block_state.state_exists(REGISTER_CLEAR);
                }
                else
                {
                    LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t PREVIOUS STATE %s",
                              to_string(block_state).c_str());

                    auto register_states = decoder->get_register_state();
                    LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t DECODER STATE %s",
                              to_string(register_states).c_str());

                    auto delta_state = convert_states<liveness::state_t>(register_states);
                    LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t DELTA STATE %s",
                              to_string(delta_state).c_str());

                    block_state = (*config.merge_vertical)(block_state, delta_state);
                    LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t RESULT STATE %s",
                              to_string(block_state).c_str());

                    change_possible = block_state.state_exists(REGISTER_CLEAR);
                }
            }
        }

        if (change_possible)
        {
            BPatch_Vector<BPatch_basicBlock *> targets;
            block->getTargets(targets);

            LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "Final merging for BasicBlock %lx",
                      bb_start_address);

            LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t PREVIOUS STATE %s",
                      to_string(block_state).c_str());

            auto delta_state = analysis(config, targets);

            LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t DELTA STATE %s",
                      to_string(delta_state).c_str());

            block_state = (*config.merge_vertical)(block_state, delta_state);
            LOG_TRACE(LOG_FILTER_LIVENESS_ANALYSIS, "\t RESULT STATE %s",
                      to_string(block_state).c_str());
        }
    }
    return block_state;
}
};