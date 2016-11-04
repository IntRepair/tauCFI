#include "region_data.h"

region_data region_data::create(Dyninst::SymtabAPI::Region *region)
{
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
        return region_data(0, 0, 0, nullptr);
}
