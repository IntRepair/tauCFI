#ifndef __ADDRESS_TAKEN_H
#define __ADDRESS_TAKEN_H

#include <string>
#include <unordered_map>
#include <vector>

#include <BPatch.h>
#include <BPatch_object.h>

#include "ca_decoder.h"

struct TakenAddressSource
{
    TakenAddressSource(std::string source_name_, BPatch_object *object_, uint64_t address_)
        : source_name(std::move(source_name_)), object(object_), address(address_)
    {
    }

    std::string source_name;
    BPatch_object *object;
    uint64_t address;
};

using TakenAddresses = std::unordered_map<uint64_t, std::vector<TakenAddressSource>>;

using TakenAddress = TakenAddresses::value_type;

TakenAddresses address_taken_analysis(BPatch_object *object, BPatch_image *image,
                                      CADecoder *decoder);

void dump_taken_addresses(TakenAddresses const &addresses);

#endif /* __ADDRESS_TAKEN_H */
