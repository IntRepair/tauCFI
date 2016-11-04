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
#if (not defined(__PADYN_COUNT_EXT_POLICY)) && (not (defined (__PADYN_TYPE_POLICY)))
void pairing(BPatch_object *object, BPatch_image *image, TakenAddresses const &ats,
             CallTargets const &cts, CallSites const &css);
#else
void pairing(BPatch_object *object, BPatch_image *image, TakenAddresses const &ats,
             std::vector<CallTargets> const &cts, std::vector<CallSites> const &css);
#endif
};
#endif /* __VERIFICATION */
