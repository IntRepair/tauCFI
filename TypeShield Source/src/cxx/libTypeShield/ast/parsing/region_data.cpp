#include "region_data.h"

// this include is special and needs to be before all other Dyninst #includes
#include <Symtab.h>

#include <AddrLookup.h>
#include <BPatch_object.h>
#include <Region.h>

#include "logging.h"

uint64_t region_data::translate_address(BPatch_object *object, uint64_t address)
{
    auto const translated = object->fileOffsetToAddr(address);
    if (static_cast<uint64_t>(translated) ==
        static_cast<uint64_t>(-1)) // already translated ?
        return address;

    return translated;
}

region_data region_data::create(std::string name, BPatch_object *object)
{
    auto symtab = Dyninst::SymtabAPI::convert(object);

    Dyninst::SymtabAPI::Region *region = nullptr;
    symtab->findRegion(region, name);

    if (region)
    {
        auto const start = region->getMemOffset();
        // translate_address(object, region->getFileOffset());
        auto const size = region->getMemSize(); // region->getDiskSize() ?
        auto const end = start + size;
        auto const raw = static_cast<uint8_t *>(region->getPtrToRawData());
        return region_data(start, size, end, raw);
    }
    else
    {
        // TODO emit error
        return region_data(0, 0, 0, nullptr);
    }
}

std::unordered_map<uint64_t, std::string>
region_data::get_got_strings(std::string name, BPatch_object *object)
{
    std::unordered_map<uint64_t, std::string> got_lookup;

    auto symtab = Dyninst::SymtabAPI::convert(object);
    Dyninst::SymtabAPI::Region *region = nullptr;

    if (!symtab->findRegion(region, name))
        LOG_ERROR(LOG_FILTER_AST, "Could not load %s", name.c_str());

    auto relocs = region->getRelocations();
    for (auto const &reloc : relocs)
    {
        LOG_TRACE(LOG_FILTER_AST, "%s -> %lx %lx -> %s", name.c_str(),
                  reloc.target_addr(), reloc.rel_addr(), reloc.name().c_str());
        got_lookup[reloc.rel_addr()] = reloc.name();
    }

    return got_lookup;
}