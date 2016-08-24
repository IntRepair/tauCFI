#ifndef __CALLTARGETS_H
#define __CALLTARGETS_H

#include <string>
#include <vector>

#include <BPatch.h>
#include <BPatch_object.h>

#include "address_taken.h"
#include "ca_decoder.h"

struct CallTarget
{
    CallTarget(BPatch_object *object_, BPatch_function *function_, std::string register_state_)
        : object(object_), function(function_), register_state(std::move(register_state_))
    {
    }

    BPatch_object *object;
    BPatch_function *function;
    std::string register_state;
};

using CallTargets = std::vector<CallTarget>;

CallTargets calltarget_analysis(BPatch_object *object, BPatch_image *image, CADecoder *decoder,
                                TakenAddresses &taken_addresses);

#endif /* __CALLTARGETS_H */
