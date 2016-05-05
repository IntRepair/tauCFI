#ifndef __ADDRESS_TAKEN_H
#define __ADDRESS_TAKEN_H

#include <BPatch.h>
#include <BPatch_addressSpace.h>
#include <BPatch_addressSpace.h>
#include <BPatch_module.h>

#include "ca_decoder.h"

#include <string>
#include <vector>

struct TakenAddress
{
    std::string module_name;
    uint64_t deref_address;
};

std::vector<TakenAddress> address_taken_analysis(BPatch_image *image, std::vector<BPatch_module *> *mods,
                                                 BPatch_addressSpace *as, CADecoder *decoder);

void dump_taken_addresses(std::vector<TakenAddress> &addresses);

#endif /* __ADDRESS_TAKEN_H */
