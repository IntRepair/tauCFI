#include "calltargets.h"

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

    auto liveness_states = liveness_init();
    auto reaching_states = reaching_init();
    auto type_states = simple_type_analysis::calltarget::init();

    instrument_object_functions(object, [&](BPatch_function *function) {
        // Instrument function
        Dyninst::Address start, end;
        char funcname[BUFFER_STRING_LEN];

        function->getName(funcname, BUFFER_STRING_LEN);
        function->getAddressRange(start, end);

        if (taken_addresses.count(start) > 0)
        {
            auto params = function->getParams();
            auto ret_type = function->getReturnType();

            DEBUG(LOG_FILTER_CALL_TARGET, "<CT> Function %s[%lx]", funcname, start);

            auto liveness_state = liveness_analysis(decoder, image, function, liveness_states);
            DEBUG(LOG_FILTER_CALL_TARGET, "\tREGISTER_STATE %s", to_string(liveness_state).c_str());

            auto reaching_state = reaching_analysis(decoder, image, function, reaching_states);
            DEBUG(LOG_FILTER_CALL_TARGET, "\tRETURN_REGISTER_STATE %s",
                  to_string(reaching_state).c_str());

            simple_type_analysis::calltarget::analysis(decoder, image, function, type_states);

            INFO(LOG_FILTER_CALL_TARGET, "<CT> Function %s[%lx] requires atleast %d args and is %s",
                 funcname, start, count_args_calltarget(liveness_state),
                 (is_void_calltarget(reaching_state) ? "<void>" : "<possibly non-void>"));

            call_targets.emplace_back(object, function, to_string(liveness_state));
        }
    });

    return call_targets;
}

template <> std::string to_string(CallTarget &call_target)
{
    char funcname[BUFFER_STRING_LEN];
    Dyninst::Address start, end;

    call_target.function->getName(funcname, BUFFER_STRING_LEN);
    call_target.function->getAddressRange(start, end);

    return "<CT> " + call_target.object->name() + ": " + std::string(funcname) + " " +
           int_to_hex(start) + " -> " + int_to_hex(end);
}
