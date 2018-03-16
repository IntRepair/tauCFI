#include "jumptables.h"

#include "ast/basic_block.h"

#include "ca_decoder.h"
#include "instrumentation.h"
#include "logging.h"

std::vector<ast::function> resolve_jumptables(BPatch_object *object,
                                              std::vector<ast::function> functions,
                                              CADecoder *decoder)
{
  LOG_INFO(LOG_FILTER_AST, "Setting Basic Blocks...");
  for (auto &ast_function : functions) {
    std::vector<ast::basic_block> basic_blocks;
    for (auto&& ast_block : ast_function.get_basic_blocks()) {
      if (ast_block.get_is_indirect_callsite())
      {
        bool potential_switch_table_mov = false;
        boost::optional<int> reg_target;
        uint64_t switch_table = 0;
        bool potential_switch_table = false;

        instrument_basicBlock_decoded(
            ast_block.get_basic_block(), decoder, [&](CADecoder *instr_decoder) {
              // get instruction bytes
              //auto const address = instr_decoder->get_address();
              if (instr_decoder->is_indirect_call())
              {
                if (potential_switch_table_mov)
                {
                  if (instr_decoder->get_reg_source(0) == reg_target)
                  {
                    LOG_INFO(LOG_FILTER_GENERAL, "Potential switch case table for %lx @ %lx", instr_decoder->get_address(), switch_table);
                    potential_switch_table = true;
                  }
                }
              }

              if ((switch_table = instr_decoder->get_potential_switch_table_address()))
              {
                potential_switch_table_mov = true;
                reg_target = instr_decoder->get_reg_target(0);
              }
              else
              {
                potential_switch_table_mov = false;
              }
           });
        basic_blocks.emplace_back(ast_block.get_basic_block(),
                                  ast_block.get_address(),
                                  ast_block.get_end(),
                                  false,
                                  potential_switch_table);
      }
      else {
        basic_blocks.emplace_back(ast_block);
      }
/*      basic_blocks.emplace_back(block, block->getStartAddress(),
                                block->getEndAddress(), is_indirect_callsite);
*/    }

//    ast_function.update_basic_blocks(basic_blocks);
  }


  return functions;
}

#if 0
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
#endif 

