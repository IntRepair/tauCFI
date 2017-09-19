#include "plt_resolver.h"

#include <string>
#include <unordered_map>

#include <BPatch_object.h>

#include "ca_decoder.h"
#include "instrumentation.h"
#include "logging.h"
#include "region_data.h"

// std::string plt_resolver::get_name(uint64_t address) const
//{
//    if (plt_names.count(address) > 0)
//        return plt_names.at(address);
//
//    return "";
//}

std::vector<ast::function> resolve_plt_names(BPatch_object *object,
                                             std::vector<ast::function> functions,
                                             CADecoder *decoder)
{
    std::unordered_map<uint64_t, std::string> got_lookup =
        region_data::get_got_strings(".rela.plt", object);

    auto const plt = region_data::create(".plt", object);
    LOG_DEBUG(LOG_FILTER_AST, "PLT(mem)  %lx -> %lx", plt.start, plt.end);

    for (auto &function : functions)
    {
        auto addr = function.get_address();

        LOG_DEBUG(LOG_FILTER_AST, "Looking at function %s @ %lx",
                  function.get_name().c_str(), addr);

        if (plt.contains_address(addr))
        {
            LOG_DEBUG(LOG_FILTER_AST, "function %s @ %lx is contained in PLT",
                      function.get_name().c_str(), addr);
            /*
                1) get .plt instruction @ $address [jmpq *disp(%rip)]
                1a) get result of disp(%rip)
                1b) save result to got_addr
            */
            uint64_t got_addr = 0;
            // TODO: change this (no longer rely on instrumentation.h)
            instrument_function_decoded_unordered(
                function.get_function(), decoder,
                [addr, &got_addr](CADecoder *instr_decoder) {
                    LOG_DEBUG(LOG_FILTER_AST,
                              "Looking for instruction %lx currently at %lx", addr,
                              instr_decoder->get_address());

                    if (instr_decoder->get_address() == addr)
                        got_addr = instr_decoder->get_address_store_address();
                });

            if (got_addr)
            {
                LOG_DEBUG(LOG_FILTER_AST,
                          "Updating function name for %s [%lx -> %lx] to %s",
                          function.get_name().c_str(), addr, got_addr,
                          got_lookup[got_addr].c_str());

                function.update_name(got_lookup[got_addr]);
            }
        }
    }
    return functions;
}
