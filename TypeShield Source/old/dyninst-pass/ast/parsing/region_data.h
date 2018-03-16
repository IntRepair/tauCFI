#ifndef __REGION_DATA_H
#define __REGION_DATA_H

#include <cinttypes>
#include <unordered_map>

#include <BPatch_object.h>

inline uint64_t translate_address(BPatch_object *object, uint64_t address)
{
    auto const translated = object->fileOffsetToAddr(address);
    if (static_cast<uint64_t>(translated) ==
        static_cast<uint64_t>(-1)) // already translated ?
        return address;

    return translated;
}

class region_data
{
  public:
    region_data(uint64_t start_, uint64_t size_, uint64_t end_, const uint8_t *raw_)
        : start(start_), size(size_), end(end_), raw(raw_)
    {
    }

    static region_data create(std::string name, BPatch_object *object);

    static std::unordered_map<uint64_t, std::string>
    get_got_strings(std::string name, BPatch_object *object);

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
