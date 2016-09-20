#ifndef __PATCHING_H
#define __PATCHING_H

#include <string>
#include <vector>

#include <BPatch.h>
#include <BPatch_addressSpace.h>
#include <BPatch_object.h>

#include "address_taken.h"
#include "callsites.h"
#include "calltargets.h"

class CADecoder;

enum PatchingPolicy
{
    POLICY_NONE,
    POLICY_ADDRESS_TAKEN,
    POLICY_PARAM_COUNT,
    POLICY_PARAM_COUNT_EXT,
    POLICY_PARAM_TYPE,
    POLICY_PARAM_TYPE_EXT
};

static const constexpr PatchingPolicy PATCHING_POLICY = POLICY_NONE;

void binary_patching(BPatch_object *object, BPatch_image* image, CADecoder *decoder, CallTargets calltargets,
                     CallSites callsites);

#endif /* __PATCHING_H */
