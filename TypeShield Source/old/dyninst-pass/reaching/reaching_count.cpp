#include "reaching_analysis.h"

#include <PatchCFG.h>

#include "instrumentation.h"
#include "reaching_impl.h"

namespace reaching
{

namespace count
{

namespace
{
static RegisterStates merge_horizontal(std::vector<RegisterStates> states)
{
    return impl::merge_horizontal(
        states, [](RegisterStates current, RegisterStates delta) {
            return transform(current, delta, [](state_t current, state_t delta) {
                if (delta != current)
                {
                    if (is_set(delta) && is_set(current))
                        current = REGISTER_SET_FULL;
                    else
                        current = REGISTER_TRASHED;
                }
                return current;
            });
        });
}

static RegisterStates merge_horizontal_intersection(std::vector<RegisterStates> states)
{
    return impl::merge_horizontal(
        states, [](RegisterStates current, RegisterStates delta) {
            return transform(current, delta, [](state_t current, state_t delta) {
                if (is_trashed(delta) || is_trashed(current))
                    return REGISTER_TRASHED;
                else if (is_set(delta) && is_set(current))
                    return REGISTER_SET_FULL;
                else
                    return REGISTER_UNKNOWN;
            });
        });
}

static RegisterStates merge_horizontal_union(std::vector<RegisterStates> states)
{
    return impl::merge_horizontal(
        states, [](RegisterStates current, RegisterStates delta) {
            return transform(current, delta, [](state_t current, state_t delta) {
                if (is_trashed(delta) || is_trashed(current))
                    return REGISTER_TRASHED;
                else if (is_set(delta) || is_set(current))
                    return REGISTER_SET_FULL;
                else
                    return REGISTER_UNKNOWN;
            });
        });
}

static RegisterStates merge_vertical(RegisterStates current, RegisterStates delta)
{
    return transform(current, delta, [](state_t current, state_t delta) {
        return (current == REGISTER_UNKNOWN) ? delta : current;
    });
}
};

namespace calltarget
{
std::vector<AnalysisConfig> init(CADecoder *decoder, ast::ast const &ast)
{
    std::vector<AnalysisConfig> configs;
    AnalysisConfig config(ast);
    config.decoder = decoder;
    config.merge_vertical = &merge_vertical;
    config.merge_horizontal_intra = &merge_horizontal;
    config.merge_horizontal_inter = &merge_horizontal;
    config.merge_horizontal_entry = &merge_horizontal;
    config.merge_horizontal_mlive = &merge_horizontal;
    config.can_change = impl::has_undefined_return_regs;
    config.follow_return_edges = false;
    config.follow_into_callers = false;
    config.use_min_liveness = false;
    config.ignore_nops = false;

    configs.push_back(config);
    configs.push_back(config);

    return configs;
}
}; /* namespace calltarget */

namespace callsite
{
namespace
{
static std::shared_ptr<FunctionCallReverseLookup>
calculate_block_callers(AnalysisConfig &config, ast::ast const &ast)
{
    auto block_callers = std::make_shared<FunctionCallReverseLookup>();

    auto decoder = config.decoder;

    using Insns = Dyninst::PatchAPI::PatchBlock::Insns;
    instrument_ast_basicBlocks_unordered(ast, [&](BPatch_basicBlock
                                                                      *block) {
        instrument_basicBlock_instructions(block, [&](Insns::value_type &instruction) {

            auto address = reinterpret_cast<uint64_t>(instruction.first);
            Instruction::Ptr instruction_ptr = instruction.second;
            if (!decoder->decode(address, instruction_ptr))
            {
                LOG_ERROR(LOG_FILTER_REACHING_ANALYSIS,
                          "Could not decode instruction @%lx", address);
                return;
            }

            if (decoder->is_call())
            {
                auto source_addr = decoder->get_src(0);

                if (!config.ast.is_function(source_addr))
                    return;

                auto function = config.ast.get_function(source_addr).get_function();

                BPatch_flowGraph *cfg = function->getCFG();

                std::vector<BPatch_basicBlock *> entry_blocks;
                if (!cfg->getEntryBasicBlock(entry_blocks))
                {
                    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS,
                              "\tCOULD NOT DETERMINE ENTRY BASIC_BLOCKS FOR FUNCTION");
                }

                BPatch_basicBlock *entry_block = nullptr;
                if (entry_blocks.size() == 1)
                    entry_block = entry_blocks[0];
                else
                {
                    auto itr =
                        std::find_if(std::begin(entry_blocks), std::end(entry_blocks),
                                     [source_addr](BPatch_basicBlock *block) {
                                         return block->getStartAddress() == source_addr;
                                     });
                    if (itr != std::end(entry_blocks))
                    {
                        entry_block = (*itr);
                    }
                    else
                    {
                        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS,
                                  "\tCOULD NOT DETERMINE CORRECT ENTRY BASIC_BLOCK FOR "
                                  "FUNCTION");
                    }
                }

                if (entry_block)
                    (*block_callers)[entry_block].push_back(
                        std::make_pair(block, address));
            }
        });
    });

    return block_callers;
}
};

std::vector<AnalysisConfig> init(CADecoder *decoder, ast::ast const &ast,
                                 std::vector<CallTargets> &targets)
{
    std::vector<AnalysisConfig> configs;
    AnalysisConfig init_config(ast);

    init_config.decoder = decoder;
    init_config.merge_vertical = &merge_vertical;
    init_config.can_change = impl::has_undefined_param_regs;
    init_config.follow_return_edges = false;
    init_config.follow_into_callers = true;
    init_config.use_min_liveness = false;
    init_config.ignore_nops = true;

    {
        AnalysisConfig config(init_config);

        config.block_callers = calculate_block_callers(init_config, ast);
        config.merge_horizontal_intra = &merge_horizontal_intersection;
        config.merge_horizontal_inter = &merge_horizontal_intersection;
        config.merge_horizontal_entry = &merge_horizontal_intersection;
        config.merge_horizontal_mlive = &merge_horizontal_intersection;
        config.merge_horizontal_rip = &merge_horizontal_intersection;

        configs.push_back(config);
    }
    {
        AnalysisConfig config(init_config);

        config.block_callers = calculate_block_callers(init_config, ast);
        config.merge_horizontal_intra = &merge_horizontal_union;
        config.merge_horizontal_inter = &merge_horizontal_union;
        config.merge_horizontal_entry = &merge_horizontal_union;
        config.merge_horizontal_mlive = &merge_horizontal_union;
        config.merge_horizontal_rip = &merge_horizontal_union;

        configs.push_back(config);
    }

    return configs;
}
}; /* namespace callsite */
}; /* namespace count */
}; /* namespace reaching */
