#ifndef __RELATIVE_CALLSITES_H
#define __RELATIVE_CALLSITES_H

#include <string>
#include <vector>

#include <BPatch.h>
#include <BPatch_object.h>

#include "ca_decoder.h"

struct CallSite
{
    CallSite(BPatch_object *object_, uint64_t function_start_, uint64_t block_start_,
             uint64_t address_, std::string register_state_)
        : object(object_), function_start(function_start_), block_start(block_start_),
          address(address_), register_state(std::move(register_state_))
    {
    }
    BPatch_object *object;
    uint64_t function_start;
    uint64_t block_start;
    uint64_t address;
    std::string register_state;
};

using CallSites = std::vector<CallSite>;

CallSites callsite_analysis(BPatch_object *objectr, BPatch_image *image, CADecoder *decoder);

#endif /* __RELATIVE_CALLSITES_H */
