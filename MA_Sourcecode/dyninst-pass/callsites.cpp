#include "callsites.h"

#include "instrumentation.h"
#include "liveness_analysis.h"
#include "logging.h"
#include "reaching_analysis.h"
#include "simple_type_analysis.h"
#include "systemv_abi.h"

CallSites callsite_analysis(BPatch_object *object, BPatch_image *image, CADecoder *decoder)
{
    CallSites call_sites;

    auto liveness_states = liveness_init();
    auto reaching_states = reaching_init();
    auto type_states = simple_type_analysis::callsite::init();

    instrument_object_basicBlocks_unordered(object, [&](BPatch_basicBlock *block) {
        auto start = block->getStartAddress();

        instrument_basicBlock_instructions(
            block, [&](Dyninst::PatchAPI::PatchBlock::Insns::value_type &instruction) {
                // get instruction bytes
                auto address = reinterpret_cast<uint64_t>(instruction.first);
                Instruction::Ptr instruction_ptr = instruction.second;
                TRACE(LOG_FILTER_CALL_SITE, "Processing instruction %lx", address);

                decoder->decode(address, instruction_ptr);

                if (decoder->is_indirect_call())
                {
                    DEBUG(LOG_FILTER_CALL_SITE, "<CS> basic block [%lx:%lx] instruction %lx",
                          block->getStartAddress(), block->getEndAddress(), address);

                    auto reaching_state =
                        reaching_analysis(decoder, image, block, address, reaching_states);
                    DEBUG(LOG_FILTER_CALL_SITE, "\tREGISTER_STATE %s",
                          to_string(reaching_state).c_str());

                    BPatch_Vector<BPatch_basicBlock *> targets;
                    block->getTargets(targets);

                    auto liveness_state =
                        liveness_analysis(decoder, image, targets, liveness_states);
                    DEBUG(LOG_FILTER_CALL_SITE, "\tRETURN_REGISTER_STATE %s",
                          to_string(liveness_state).c_str());

                    simple_type_analysis::callsite::analysis(decoder, image, block, address,
                                                             type_states);

                    INFO(LOG_FILTER_CALL_SITE,
                         "<CS> instruction %lx provides atmost %d args and is %s", address,
                         count_args_callsite(reaching_state),
                         (is_nonvoid_callsite(liveness_state) ? "<non-void>" : "<possibly void>"));

                    call_sites.emplace_back(object, start, block->getStartAddress(), address,
                                            to_string(reaching_state));
                }

            });
    });
    return call_sites;
}

template <> std::string to_string(CallSite &call_site)
{
    std::stringstream ss;
    ss << "<CS> " << call_site.object->name() << ":" << int_to_hex(call_site.function_start) << ":"
       << int_to_hex(call_site.block_start) << ":" << int_to_hex(call_site.address);

    return ss.str();
}
