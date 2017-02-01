#include "calltargets.h"

#include <Symtab.h>

#include "ca_decoder.h"
#include "instrumentation.h"
#include "liveness_analysis.h"
#include "logging.h"
#include "reaching_analysis.h"
#include "region_data.h"
#include "systemv_abi.h"
#include "utils.h"

std::vector<CallTargets> calltarget_analysis(BPatch_object *object, BPatch_image *image,
                                             CADecoder *decoder,
                                             TakenAddresses &taken_addresses)
{

    auto symtab = Dyninst::SymtabAPI::convert(object);

    Dyninst::SymtabAPI::Region *region = nullptr;
    symtab->findRegion(region, ".plt");
    auto const plt = region_data::create(region);

    auto const is_shared = [&]() {
        std::vector<BPatch_module *> modules;
        object->modules(modules);
        if (modules.size() != 1)
            return false;
        return modules[0]->isSharedLib();
    }();

    LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS, "PLT(mem)  %lx -> %lx", plt.start, plt.end);

#ifdef __PADYN_TYPE_POLICY
    auto liveness_configs = liveness::type::calltarget::init(decoder, image, object);
    auto reaching_configs = reaching::type::calltarget::init(decoder, image, object);

#elif defined(__PADYN_COUNT_EXT_POLICY)
    auto liveness_configs = liveness::count_ext::calltarget::init(decoder, image, object);
    auto reaching_configs = reaching::count_ext::calltarget::init(decoder, image, object);

#else
    auto liveness_configs = liveness::count::calltarget::init(decoder, image, object);
    auto reaching_configs = reaching::count::calltarget::init(decoder, image, object);
#endif
    std::vector<CallTargets> call_targets_vector;

    for (auto index = 0; index < liveness_configs.size(); ++index)
    {
        auto& liveness_config = liveness_configs[index];
        auto& reaching_config = reaching_configs[index];

        CallTargets call_targets;

    instrument_object_functions(object, [&](BPatch_function *function) {
        // Instrument function
        Dyninst::Address start, end;
        char funcname[BUFFER_STRING_LEN];

        function->getName(funcname, BUFFER_STRING_LEN);
        function->getAddressRange(start, end);

// disabled for now, we simply tag the calltarget
#if 0
        if (taken_addresses.count(start) > 0)
#endif
        {
            LOG_DEBUG(LOG_FILTER_CALL_TARGET, "Looking at Function %s[%lx]", funcname,
                      start);

            auto liveness_state = liveness::analysis(liveness_config, function);
            LOG_DEBUG(LOG_FILTER_CALL_TARGET, "\tREGISTER_STATE %s",
                      to_string(liveness_state).c_str());

            auto reaching_state = reaching::analysis(reaching_config, function);
            LOG_DEBUG(LOG_FILTER_CALL_TARGET, "\tRETURN_REGISTER_STATE %s",
                      to_string(reaching_state).c_str());

            auto parameters = system_v::calltarget::generate_paramlist(
                function, start, reaching_state, liveness_state);

            auto const is_plt = [&]() {
                if (is_shared)
                {
                    auto const plt_start = translate_address(object, plt.start);
                    auto const plt_end = plt_start + plt.size;
                    auto const address = translate_address(object, start);

                    return (plt_start <= address) && (address < plt_end);
                }

                return plt.contains_address(start);
            }();

            call_targets.emplace_back(function, taken_addresses.count(start) > 0, is_plt,
                                      parameters);
        }
    });

        liveness_config.block_states.clear();
        reaching_config.block_states.clear();

        call_targets_vector.push_back(call_targets);
    }

    return call_targets_vector;
}
