#ifndef __REGION_DATA_H
#define __REGION_DATA_H

#include <cinttypes>
#include <unordered_map>

class BPatch_object;

class region_data
{
  public:
    region_data(uint64_t start_, uint64_t size_, uint64_t end_, const uint8_t *raw_)
        : start(start_), size(size_), end(end_), raw(raw_)
    {
    }

    region_data()
        : start(0), size(0), end(0), raw(nullptr)
    {
    }

    static uint64_t translate_address(BPatch_object *object, uint64_t address);

    static region_data create(std::string name, BPatch_object *object);

    static std::unordered_map<uint64_t, std::string>
    get_got_strings(std::string name, BPatch_object *object);

    bool contains_address(uint64_t address) const
    {
        return (start <= address) && (address < end);
    }

    uint64_t start;
    uint64_t size;
    uint64_t end;
    uint8_t const *raw;
};

#endif /* __REGION_DATA_H */
