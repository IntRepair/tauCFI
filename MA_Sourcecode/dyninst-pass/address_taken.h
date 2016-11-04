#ifndef __ADDRESS_TAKEN_H
#define __ADDRESS_TAKEN_H

#include <string>
#include <unordered_map>
#include <vector>

#include <BPatch.h>
#include <BPatch_object.h>

class CADecoder;

struct TakenAddressSource
{
    TakenAddressSource(std::string source_name_, BPatch_object *object_,
                       uint64_t address_)
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

#include "to_string.h"

template <> inline std::string to_string(TakenAddressSource const &address_source)
{
    return "\t[ <" + address_source.source_name + "> " + address_source.object->name() +
           " @ " + int_to_hex(address_source.address) + "]";
}

template <> inline std::string to_string(TakenAddress const &address)
{
    return "<AT> " + int_to_hex(address.first) + ":\n" + to_string(address.second);
}

template <> inline std::string to_string(TakenAddresses const &addresses)
{
    std::stringstream ss;

    for (auto &&address : addresses)
        ss << to_string<TakenAddress>(address) << "\n";

    return ss.str();
}

#endif /* __ADDRESS_TAKEN_H */
