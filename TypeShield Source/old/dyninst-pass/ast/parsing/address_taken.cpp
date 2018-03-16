#include "address_taken.h"

#include <unordered_set>

#include <BPatch_object.h>

#include "ca_decoder.h"
#include "instrumentation.h"
#include "logging.h"
#include "region_data.h"

namespace
{
template <typename window_t, typename analysis_t>
static void analyze_region(region_data const &data, analysis_t analysis)
{
    auto const raw_end = data.raw + data.size - sizeof(window_t);

    for (auto ptr = data.raw; ptr < raw_end; ++ptr)
    {
        auto const raw_value = *reinterpret_cast<window_t const *>(ptr);
        auto const value = static_cast<uint64_t>(raw_value);
        auto const value_address = ptr - data.raw + data.start;

        analysis(value, value_address);
    }
}
};

std::vector<ast::function> address_taken_analysis(BPatch_object *object,
                                                  std::vector<ast::function> functions,
                                                  CADecoder *decoder)
{
    auto const objname = object->name();

    LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS, "Processing object %s", objname.c_str());

    auto const text = region_data::create(".text", object);
    LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS, "TEXT(mem)  %lx -> %lx", text.start, text.end);

    auto const plt = region_data::create(".plt", object);
    LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS, "PLT(mem)  %lx -> %lx", plt.start, plt.end);

    auto const data = region_data::create(".data", object);
    LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS, "DATA(mem)  %lx -> %lx", data.start, data.end);

    auto const rodata = region_data::create(".rodata", object);
    LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS, "RODATA(mem)  %lx -> %lx", rodata.start,
              rodata.end);

    if (!plt.raw || !text.raw)
    {
        LOG_ERROR(LOG_FILTER_TAKEN_ADDRESS,
                  ".plt or .text section not found -> could not process");
        return functions;
    }

    const auto function_lookup = [&functions]() {
        std::unordered_set<uint64_t> lookup;
        for (auto const &function : functions)
        {
            LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS, "Lookup for function %s @%lx",
                      function.get_name().c_str(), function.get_address());
            lookup.insert(function.get_address());
        }
        return lookup;
    }();

    const auto try_set_at_function = [&functions, &function_lookup](uint64_t value) {
        if (function_lookup.count(value))
        {
            (std::find_if(
                 functions.begin(), functions.end(),
                 [&value](ast::function const &fn) { return fn.get_address() == value; }))
                ->update_is_at(true);
            return true;
        }
        return false;
    };

    auto const at_analysis = [&](uint64_t value, uint64_t value_address) {
        LOG_TRACE(LOG_FILTER_TAKEN_ADDRESS,
                  "Looking at Address @ %lx containing value %lx", value_address, value);
        // check if needed
        // if (is_shared)
        //{
        //    value = translate_address(object, value);
        //}

        if (plt.contains_address(value))
        {
            LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS,
                      "Taken Address found @ %lx = %lx points into .plt", value_address,
                      value);

            if (!try_set_at_function(value))
            {
                LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS,
                          "No function found for Taken Address "
                          "found @ %lx containing value %lx points into .plt",
                          value_address, value);
            }
        }
        else if (text.contains_address(value))
        {
            LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS,
                      "Taken Address found @ %lx = %lx points into .text", value_address,
                      value);

            if (!try_set_at_function(value))
            {
                LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS,
                          "No function found for Taken Address "
                          "found @ %lx containing value %lx points into .text",
                          value_address, value);
            }
        }

    };

    LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS,
              "Looking at .data and .rodata section contents...");
    for (auto &&region : {data, rodata})
    {
        analyze_region<uint32_t>(region, at_analysis);
        analyze_region<uint64_t>(region, at_analysis);
    }

    LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS, "Looking at executable content...");
    std::unordered_set<uint64_t> visited_addresses;

    for (auto const &ast_function : functions)
    {
        instrument_function_decoded_unordered(
            ast_function.get_function(), decoder, [&](CADecoder *instr_decoder) {
                auto const address = instr_decoder->get_address();
                if (visited_addresses.count(address) > 0)
                    return;

                visited_addresses.insert(address);

                auto src_addresses = instr_decoder->get_src_abs_addr();

                for (auto &&src_address : src_addresses)
                    at_analysis(src_address, address);
            });
    }

    return functions;
}
