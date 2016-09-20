#ifndef __RELATIVE_CALLSITES_H
#define __RELATIVE_CALLSITES_H

#include <array>
#include <string>
#include <vector>

#include <BPatch.h>
#include <BPatch_function.h>
#include <BPatch_object.h>

#include "ca_defines.h"
#include "calltargets.h"

class CADecoder;

struct CallSite
{
    CallSite(BPatch_function *function_, uint64_t block_start_, uint64_t address_,
             std::array<uint8_t, 7> parameters_)
        : function(function_), block_start(block_start_), address(address_),
          parameters(std::move(parameters_))
    {
    }
    BPatch_function *function;
    uint64_t block_start;
    uint64_t address;
    std::array<uint8_t, 7> parameters;
};

using CallSites = std::vector<CallSite>;

CallSites callsite_analysis(BPatch_object *objectr, BPatch_image *image, CADecoder *decoder,
                            CallTargets &targets);

#include "to_string.h"

template <> std::string to_string(CallSite const &call_site)
{
    char funcname[BUFFER_STRING_LEN];
    Dyninst::Address start, end;

    call_site.function->getName(funcname, BUFFER_STRING_LEN);
    call_site.function->getAddressRange(start, end);

    return "<CS> " + std::string(funcname) + " " + int_to_hex(start) + " " +
           to_string(call_site.parameters) + int_to_hex(call_site.block_start) + " " +
           int_to_hex(call_site.address);
}

#endif /* __RELATIVE_CALLSITES_H */
