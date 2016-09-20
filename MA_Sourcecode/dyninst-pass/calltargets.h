#ifndef __CALLTARGETS_H
#define __CALLTARGETS_H

#include <array>
#include <string>
#include <vector>

#include <BPatch.h>
#include <BPatch_function.h>
#include <BPatch_object.h>

#include "address_taken.h"
#include "ca_defines.h"

class CADecoder;

struct CallTarget
{
    CallTarget(BPatch_function *function_, std::array<uint8_t, 7> parameters_)
        : function(function_), parameters(std::move(parameters_))
    {
    }

    BPatch_function *function;
    std::array<uint8_t, 7> parameters;
};

using CallTargets = std::vector<CallTarget>;

CallTargets calltarget_analysis(BPatch_object *object, BPatch_image *image, CADecoder *decoder,
                                TakenAddresses &taken_addresses);

#include "to_string.h"

template <> std::string to_string(CallTarget const &call_target)
{
    char funcname[BUFFER_STRING_LEN];
    Dyninst::Address start, end;

    call_target.function->getName(funcname, BUFFER_STRING_LEN);
    call_target.function->getAddressRange(start, end);

    return "<CT> " + std::string(funcname) + " " + int_to_hex(start) + " " +
           to_string(call_target.parameters);
}

#endif /* __CALLTARGETS_H */
