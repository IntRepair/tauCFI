#include "calltargets.h"

#include "ca_decoder.h"
#include "instrumentation.h"
#include "liveness_analysis.h"
#include "logging.h"
#include "reaching_analysis.h"
#include "simple_type_analysis.h"
#include "systemv_abi.h"

CallTargets calltarget_analysis(BPatch_object *object, BPatch_image *image, CADecoder *decoder,
                                TakenAddresses &taken_addresses)
{
    std::vector<CallTarget> call_targets;

    auto liveness_config = liveness::calltarget::init(decoder, image, object);
    auto reaching_config = reaching::calltarget::init(decoder, image, object);
    auto type_states = type_analysis::simple::calltarget::init();

    instrument_object_functions(object, [&](BPatch_function *function) {
        // Instrument function
        Dyninst::Address start, end;
        char funcname[BUFFER_STRING_LEN];

        function->getName(funcname, BUFFER_STRING_LEN);
        function->getAddressRange(start, end);

#if 0
        if (taken_addresses.count(start) > 0)
#endif
        {
            auto params = function->getParams();
            auto ret_type = function->getReturnType();

            LOG_DEBUG(LOG_FILTER_CALL_TARGET, "<CT> Function %s[%lx]", funcname, start);

            auto liveness_state = liveness::analysis(liveness_config, function);
            LOG_DEBUG(LOG_FILTER_CALL_TARGET, "\tREGISTER_STATE %s",
                      to_string(liveness_state).c_str());

            auto reaching_state = reaching::analysis(reaching_config, function);
            LOG_DEBUG(LOG_FILTER_CALL_TARGET, "\tRETURN_REGISTER_STATE %s",
                      to_string(reaching_state).c_str());

            type_analysis::simple::calltarget::analysis(decoder, image, function, type_states);

            LOG_INFO(LOG_FILTER_CALL_TARGET,
                     "<CT> Function %s[%lx] requires atleast %d args and is %s", funcname, start,
                     count_args_calltarget(liveness_state),
                     (is_void_calltarget(reaching_state) ? "<void>" : "<possibly non-void>"));

            std::array<uint8_t, 7> parameters;
            std::fill(parameters.begin(), parameters.end(), '0');
            std::fill_n(parameters.begin(), count_args_calltarget(liveness_state), 'x');
            parameters[6] = is_void_calltarget(reaching_state) ? '0' : 'x';
            call_targets.emplace_back(function, parameters);
        }
    });

    return call_targets;
}
