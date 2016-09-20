#include "callsites.h"

#include "ca_decoder.h"
#include "instrumentation.h"
#include "liveness_analysis.h"
#include "logging.h"
#include "reaching_analysis.h"
#include "simple_type_analysis.h"
#include "systemv_abi.h"

CallSites callsite_analysis(BPatch_object *object, BPatch_image *image, CADecoder *decoder,
                            CallTargets &targets)
{
    CallSites call_sites;

    auto liveness_config = liveness::callsite::init(decoder, image, object);
    auto reaching_config = reaching::callsite::init(decoder, image, object, targets);
    auto type_states = type_analysis::simple::callsite::init();

    std::unordered_map<uint64_t, std::vector<CallSite>> memory_callsites;

    using Insns = Dyninst::PatchAPI::PatchBlock::Insns;
    instrument_object_functions(object, [&](BPatch_function *function) {
        instrument_function_basicBlocks_unordered(function, [&](BPatch_basicBlock *block) {
            instrument_basicBlock_instructions(block, [&](Insns::value_type &instruction) {
                // get instruction bytes
                auto address = reinterpret_cast<uint64_t>(instruction.first);
                Instruction::Ptr instruction_ptr = instruction.second;
                LOG_TRACE(LOG_FILTER_CALL_SITE, "Processing instruction %lx", address);

                decoder->decode(address, instruction_ptr);

                if (decoder->is_indirect_call())
                {
                    auto store_address = decoder->get_address_store_address();

                    LOG_DEBUG(LOG_FILTER_CALL_SITE, "<CS> basic block [%lx:%lx] instruction %lx",
                              block->getStartAddress(), block->getEndAddress(), address);

                    auto reaching_state = reaching::analysis(reaching_config, block, address);
                    LOG_DEBUG(LOG_FILTER_CALL_SITE, "\tREGISTER_STATE %s",
                              to_string(reaching_state).c_str());

                    BPatch_Vector<BPatch_basicBlock *> targets;
                    block->getTargets(targets);

                    auto liveness_state = liveness::analysis(liveness_config, targets);
                    LOG_DEBUG(LOG_FILTER_CALL_SITE, "\tRETURN_REGISTER_STATE %s",
                              to_string(liveness_state).c_str());

                    type_analysis::simple::callsite::analysis(decoder, image, block, address,
                                                              type_states);

                    std::array<uint8_t, 7> parameters;
                    std::fill(parameters.begin(), parameters.end(), '0');
                    std::fill_n(parameters.begin(), count_args_callsite(reaching_state), 'x');
                    parameters[6] = is_nonvoid_callsite(liveness_state) ? 'x' : '0';

                    if (store_address != 0)
                    {
                        LOG_INFO(LOG_FILTER_CALL_SITE, "<CS*> instruction %lx provides "
                                                       "atmost %d args and is %s [using "
                                                       "fptr stored at %lx]",
                                 address, count_args_callsite(reaching_state),
                                 (is_nonvoid_callsite(liveness_state) ? "<non-void>"
                                                                      : "<possibly void>"),
                                 store_address);

                        memory_callsites[store_address].emplace_back(
                            function, block->getStartAddress(), address, parameters);
                    }
                    else
                    {
                        LOG_INFO(LOG_FILTER_CALL_SITE,
                                 "<CS> instruction %lx provides atmost %d args and is %s", address,
                                 count_args_callsite(reaching_state),
                                 (is_nonvoid_callsite(liveness_state) ? "<non-void>"
                                                                      : "<possibly void>"));
                    }
                    call_sites.emplace_back(function, block->getStartAddress(), address,
                                            parameters);
                }
            });
        });
    });

    for (auto &&addr_cs_pairs : memory_callsites)
    {
        LOG_DEBUG(LOG_FILTER_CALL_SITE, "%lx -> %s", addr_cs_pairs.first,
                  to_string(addr_cs_pairs.second).c_str());
    }

    return call_sites;
}
