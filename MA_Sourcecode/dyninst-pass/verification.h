#ifndef __VERIFICATION
#define __VERIFICATION

#include "address_taken.h"
#include "callsites.h"
#include "calltargets.h"

#include <BPatch.h>
#include <BPatch_image.h>
#include <BPatch_object.h>

namespace verification
{
void pairing(BPatch_object *object, BPatch_image *image, TakenAddresses const &ats,
             CallTargets const &cts, CallSites const &css);
};
#endif /* __VERIFICATION */
