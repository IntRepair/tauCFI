#ifndef __RELATIVE_CALLSITES_H
#define __RELATIVE_CALLSITES_H

#include <BPatch.h>
#include <BPatch_addressSpace.h>
#include <BPatch_addressSpace.h>
#include <BPatch_module.h>

#include "ca_decoder.h"

#include <string>
#include <vector>

struct CallSite
{
    std::string module_name;
    uint64_t function_start;
    uint64_t block_start;
    uint64_t address;
};

std::vector<CallSite> relative_callsite_analysis(BPatch_image *image, std::vector<BPatch_module *> *mods,
                                                 BPatch_addressSpace *as, CADecoder *decoder);

void dump_call_sites(std::vector<CallSite> &call_sites);

#endif /* __RELATIVE_CALLSITES_H */
