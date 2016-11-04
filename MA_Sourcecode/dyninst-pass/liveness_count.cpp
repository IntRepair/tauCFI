#include "liveness_analysis.h"

#include <PatchCFG.h>

#include "instrumentation.h"
#include "liveness_impl.h"

namespace liveness
{

namespace count
{

namespace
{
static RegisterStates merge_vertical(RegisterStates current, RegisterStates delta)
{
    return transform(current, delta,
                     [](liveness::state_t current, liveness::state_t delta) {
                         return (current == REGISTER_CLEAR) ? delta : current;
                     });
}

static RegisterStates merge_horizontal(std::vector<RegisterStates> states)
{
    return impl::merge_horizontal(
        states, [](RegisterStates current, RegisterStates delta) {
            return transform(current, delta, [](state_t current, state_t delta) {
                if (is_read_before_write(current) && delta == REGISTER_CLEAR)
                    current = REGISTER_CLEAR;
                else if (is_write_before_read(current) || is_write_before_read(delta))
                    current = REGISTER_WRITE_BEFORE_READ_FULL;
                return current;
            });
        });
}

static RegisterStates merge_horizontal_union(std::vector<RegisterStates> states)
{
    return impl::merge_horizontal(
        states, [](RegisterStates current, RegisterStates delta) {
            return transform(current, delta, [](state_t current, state_t delta) {
                if (is_write_before_read(current) || is_write_before_read(delta))
                    current = REGISTER_WRITE_BEFORE_READ_FULL;
                else if (is_read_before_write(current) || is_read_before_write(delta))
                    current = REGISTER_READ_BEFORE_WRITE_FULL;
                return current;
            });
        });
}

static bool is_xmm_passthrough_block(CADecoder *decoder, BPatch_basicBlock *block)
{
    auto prev = -1;
    auto disp = 0ul;
    boost::optional<int> stack_reg;

    using Insns = Dyninst::PatchAPI::PatchBlock::Insns;
    instrument_basicBlock_instructions(block, [&](Insns::value_type &instruction) {
        auto address = reinterpret_cast<uint64_t>(instruction.first);
        Instruction::Ptr instruction_ptr = instruction.second;

        if (!decoder->decode(address, instruction_ptr))
        {
            LOG_ERROR(LOG_FILTER_LIVENESS_ANALYSIS, "Could not decode instruction @%lx",
                      address);
            return;
        }

        // PROBLEM: ALIASING CAN PREVENT THIS !
        // if (!decoder->is_target_stackpointer_disp(0))
        if (!decoder->is_target_register_disp(0))
        {
            LOG_ERROR(LOG_FILTER_LIVENESS_ANALYSIS, "FAIL A XMM @%lx prev:%d", address,
                      prev);
            prev = -2;
            return;
        }
        else if (prev == -1 && decoder->is_reg_source(0, DR_REG_XMM0))
        {
            LOG_ERROR(LOG_FILTER_LIVENESS_ANALYSIS, "STAGE 1 XMM @%lx", address);
            prev++;
            disp = decoder->get_target_disp(0);
            stack_reg = decoder->get_reg_disp_target_register(0);
        }
        else if (!stack_reg || stack_reg != decoder->get_reg_disp_target_register(0))
        {
            LOG_ERROR(LOG_FILTER_LIVENESS_ANALYSIS, "FAIL D XMM @%lx prev:%d", address,
                      prev);
            prev = -2;
            return;
        }
        else if ((disp += 0x10) != decoder->get_target_disp(0))
        {
            LOG_ERROR(LOG_FILTER_LIVENESS_ANALYSIS, "FAIL B XMM @%lx prev:%d", address,
                      prev);
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
            LOG_ERROR(LOG_FILTER_LIVENESS_ANALYSIS, "FAIL C XMM @%lx prev:%d", address,
                      prev);
            prev = -2;
            return;
        }
    });

    return (prev == 7);
}

static void calculate_ignore_instructions(AnalysisConfig &config,
                                          BPatch_basicBlock *init_block)
{
    auto decoder = config.decoder;

    auto reg_index = DR_REG_R9; // todo use system_abi for this
    auto done = false;

    using Insns = Dyninst::PatchAPI::PatchBlock::Insns;
    instrument_basicBlock_instructions(init_block, [&](Insns::value_type &instruction) {
        if (done)
            return;

        auto address = reinterpret_cast<uint64_t>(instruction.first);
        Instruction::Ptr instruction_ptr = instruction.second;

        if (!decoder->decode(address, instruction_ptr))
        {
            LOG_ERROR(LOG_FILTER_LIVENESS_ANALYSIS, "Could not decode instruction @%lx",
                      address);
            return;
        }

        // PROBLEM: ALIASING CAN PREVENT THIS !
        // if (decoder->is_target_stackpointer_disp(0) && decoder->is_reg_source(0,
        // reg_index))
        if (decoder->is_target_register_disp(0) && decoder->is_reg_source(0, reg_index))
        {
            LOG_INFO(LOG_FILTER_LIVENESS_ANALYSIS, "Ignore instruction %lx", address);
            config.ignore_instructions->insert(address);

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
            {
                done = true;
                return;
            }
            default:
                break;
            }
        }
        else if (reg_index != DR_REG_R9)
            done = true;
    });
}

static void initialize_variadic_passthrough(AnalysisConfig &config, BPatch_object *object)
{
    config.ignore_instructions = std::make_shared<IgnoreInstructions>();

    instrument_object_basicBlocks_unordered(object, [&](BPatch_basicBlock *block) {
        if (is_xmm_passthrough_block(config.decoder, block))
        {
            LOG_INFO(LOG_FILTER_LIVENESS_ANALYSIS,
                     "BasicBlock [%lx : %lx[ is an xmm passthrough block",
                     block->getStartAddress(), block->getEndAddress());

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
                        LOG_INFO(
                            LOG_FILTER_LIVENESS_ANALYSIS,
                            "BasicBlock [%lx : %lx[ is an varargs passthrough reg block",
                            init_block->getStartAddress(), init_block->getEndAddress());

                        calculate_ignore_instructions(config, reg_block);
                    }
                }
            }
        }
    });
}
};

namespace calltarget
{
AnalysisConfig init(CADecoder *decoder, BPatch_image *image, BPatch_object *object)
{
    AnalysisConfig config;

    config.decoder = decoder;
    config.image = image;
    config.merge_vertical = &merge_vertical;
    config.merge_horizontal = &merge_horizontal_union;
    config.can_change = &impl::has_clear_param_regs;

    config.follow_calls = true;
    config.ignore_nops = true;

    // this will take care of ooptimized versions
    initialize_variadic_passthrough(config, object);

    return config;
}
}; /* namespace calltarget */

namespace callsite
{

AnalysisConfig init(CADecoder *decoder, BPatch_image *image, BPatch_object *object)
{
    AnalysisConfig config;

    config.decoder = decoder;
    config.image = image;
    config.merge_vertical = &merge_vertical;
    config.merge_horizontal = &merge_horizontal;
    config.can_change = &impl::has_clear_return_regs;

    config.follow_calls = true;
    config.ignore_nops = false;

    return config;
}
}; /* namespace callsite */

}; /* namespace count */
}; /* namespace liveness */