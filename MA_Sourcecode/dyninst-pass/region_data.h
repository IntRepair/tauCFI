#ifndef __REGION_DATA_H
#define __REGION_DATA_H

#include <BPatch_object.h>
#include <Region.h>

inline uint64_t translate_address(BPatch_object *object, uint64_t address)
{
    auto const translated = object->fileOffsetToAddr(address);
    if (static_cast<uint64_t>(translated) == static_cast<uint64_t>(-1)) // already translated ?
        return address;

    return translated;
}

class region_data
{
    region_data(uint64_t start_, uint64_t size_, uint64_t end_, const uint8_t *raw_)
        : start(start_), size(size_), end(end_), raw(raw_)
    {
    }

  public:
    static region_data create(Dyninst::SymtabAPI::Region *region);

    bool contains_address(uint64_t address) const
    {
        return (start <= address) && (address < end);
    }

    const uint64_t start;
    const uint64_t size;
    const uint64_t end;
    const uint8_t *raw;
};

#endif /* __REGION_DATA_H */
