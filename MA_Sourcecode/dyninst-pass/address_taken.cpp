#include "address_taken.h"

#include <AddrLookup.h>
#include <BPatch_module.h>
#include <Symtab.h>
#include <unordered_set>

#include "ca_decoder.h"
#include "instrumentation.h"
#include "logging.h"
#include "region_data.h"
#include "utils.h"

using Dyninst::SymtabAPI::Region;
using util::contains_if;

namespace
{
static bool is_function_start(BPatch_image *image, uint64_t address)
{
    std::vector<BPatch_function *> functions;
    image->findFunction(address, functions);

    return contains_if(functions, [address](BPatch_function *function) {
        Dyninst::Address start, end;
        return function->getAddressRange(start, end) && (start == address);
    });
}

template <typename window_t, typename analysis_t>
static void analyze_region(region_data const &data, analysis_t analysis)
{
    auto const raw_end = data.raw + data.size - sizeof(window_t);

    for (auto ptr = data.raw; ptr < raw_end; ++ptr)
    {
        auto const raw_value = *reinterpret_cast<window_t const *>(ptr);
        auto const value = static_cast<uint64_t>(raw_value);
        auto const value_address = ptr - data.raw + data.start;

        if (sizeof(window_t) == 4)
        {
            LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS,
                      "%" PRIx64 " : %" PRIx32 " -> %" PRIx64 " [%" PRIx8 ", %" PRIx8
                      ", %" PRIx8 ", %" PRIx8 "]",
                      value_address, raw_value, value, ptr[0], ptr[1], ptr[2], ptr[3]);
        }

        if (sizeof(window_t) == 8)
        {
            LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS,
                      "%" PRIx64 " : %" PRIx64 " -> %" PRIx64 " [%" PRIx8 ", %" PRIx8
                      ", %" PRIx8 ", %" PRIx8 ", %" PRIx8 ", %" PRIx8 ", %" PRIx8
                      ", %" PRIx8 "]",
                      value_address, raw_value, value, ptr[0], ptr[1], ptr[2], ptr[3],
                      ptr[4], ptr[5], ptr[6], ptr[7]);
        }

        analysis(value, value_address);
    }
}
};

TakenAddresses address_taken_analysis(BPatch_object *object, BPatch_image *image,
                                      CADecoder *decoder)
{
    LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS, "Processing object %s", object->name().c_str());

    std::unordered_map<uint64_t, std::vector<TakenAddressSource>> taken_addresses;
    auto symtab = Dyninst::SymtabAPI::convert(object);

    Region *region = nullptr;
    auto const found_text = symtab->findRegion(region, ".text");
    auto const text = region_data::create(region);
    LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS, "TEXT(mem)  %lx -> %lx", text.start, text.end);

    region = nullptr;
    auto const found_plt = symtab->findRegion(region, ".plt");
    auto const plt = region_data::create(region);
    LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS, "PLT(mem)  %lx -> %lx", plt.start, plt.end);

    auto const is_shared = [&]() {
        std::vector<BPatch_module *> modules;
        object->modules(modules);
        if (modules.size() != 1)
            return false;
        return modules[0]->isSharedLib();
    }();

    if (!found_plt || !found_text)
    {
        LOG_ERROR(LOG_FILTER_TAKEN_ADDRESS,
                  ".plt or .text section not found -> could not process");
        return taken_addresses;
    }

    for (auto &&region_name : {".data", ".rodata", ".dynsym"})
    {
        auto type_fn = [&](uint64_t value, uint64_t address) {
            if (plt.contains_address(value))
            {
                LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS, "%lx:%lx,.plt", address, value);
                return ".plt";
            }

            if (std::string(region_name) == std::string(".rodata"))
            {
                LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS, "%lx:%lx,.data", address, value);
                return ".data";
            }

            LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS, "%lx:%lx,%s", address, value,
                      region_name);
            return region_name;
        };

        auto const analysis = [&](uint64_t value, uint64_t address) {
            if (!text.contains_address(value) && !plt.contains_address(value))
                return;

            auto const type = type_fn(value, address);

            if (is_shared)
            {
                value = translate_address(object, value);
                address = translate_address(object, address);
            }

            // PLT here means, statically, in data section, pointer point to PLT are
            // found. In this case, dyninst sometimes do not conclude that a plt  entry is
            // a function, so add it no matter what
            if (std::string(type) == std::string(".plt"))
            {
                taken_addresses[value].emplace_back(std::string("plt"), object, address);
                LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS, "<PLT>%lx:%lx", address, value);
            }
            // For the rest case, where pointer stored into data/rodata section
            else if (std::string(type) == std::string(".data"))
            {
                if (is_function_start(image, value))
                {
                    taken_addresses[value].emplace_back(std::string("data"), object,
                                                        address);
                    LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS, "<AT>%lx:%lx", address, value);
                }
            }
        };

        region = nullptr;
        if (!symtab->findRegion(region, region_name))
        {
            LOG_ERROR(LOG_FILTER_TAKEN_ADDRESS, "Could not find region %s", region_name);
        }
        else
        {
            auto const data = region_data::create(region);
            LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS, "%s(mem)  %lx -> %lx", region_name,
                      data.start, data.end);
            analyze_region<uint32_t>(data, analysis);
            analyze_region<uint64_t>(data, analysis);
        }
    }

    std::unordered_set<uint64_t> visited_addresses;

    instrument_object_decoded_unordered(object, decoder, [&](CADecoder *instr_decoder) {
        auto const address = instr_decoder->get_address();
        if (visited_addresses.count(address) > 0)
            return;

        visited_addresses.insert(address);

        auto src_addresses = instr_decoder->get_src_abs_addr();

        auto const address_analysis = [&](uint64_t value) {
            if (value == 0)
                return;

            if (is_shared)
            {
                value = translate_address(object, value);
            }

            if (is_function_start(image, value))
            {
                LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS, "<AT>%lx:%lx", address, value);
                taken_addresses[value].emplace_back(std::string("insns(direct)"), object,
                                                    address);
            }
            if (plt.start <= value && value <= plt.end)
            {
                LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS, "<PLT>%lx:%lx", address, value);
                taken_addresses[value].emplace_back(std::string("insns(plt)"), object,
                                                    address);
            }
        };

        for (auto &&src_address : src_addresses)
        {
            uint64_t const translated_src_address =
                translate_address(object, src_address);
            address_analysis(src_address);
            if (src_address != translated_src_address)
                address_analysis(translated_src_address);
        }
    });

    return taken_addresses;
}
