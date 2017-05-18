#include "ast/ast.h"

#include <string>
#include <unordered_map>
#include <vector>

#include <BPatch_function.h>
#include <BPatch_module.h>
#include <BPatch_object.h>

#include "address_taken.h"
#include "ca_decoder.h"
#include "ca_defines.h"
#include "function_borders.h"
#include "instrumentation.h"
#include "logging.h"
#include "plt_resolver.h"

ast::ast ast::ast::create(BPatch_object *object, CADecoder *decoder) {
  std::vector<function> functions;
  std::vector<BPatch_module *> modules;
  object->modules(modules);

  std::unordered_map<std::string, region_data> regions;
  auto const plt = region_data::create(".plt", object);
  regions[".plt"] = plt;
  regions[".text"] = region_data::create(".text", object);
  regions[".rodata"] = region_data::create(".rodata", object);
  regions[".data"] = region_data::create(".data", object);
  regions[".bss"] = region_data::create(".bss", object);

  auto const is_shared = [&modules]() {
    if (modules.size() != 1)
      return false;
    return modules[0]->isSharedLib();
  }();

  std::string const objname = object->name();

  LOG_INFO(LOG_FILTER_AST, "Creating AST for object%s %s...",
           is_shared ? "[shared]" : "", objname.c_str());

  auto const is_plt = [&object, &plt, &is_shared](uint64_t start) {
/*    if (is_shared) {
      auto const plt_start = region_data::translate_address(object, plt.start);
      auto const plt_end = plt_start + plt.size;
      auto const address = region_data::translate_address(object, start);

      return (plt_start <= address) && (address < plt_end);
    }*/

    return plt.contains_address(start);
  };

  for (auto module : modules) {
    char modname[BUFFER_STRING_LEN];
    module->getFullName(modname, BUFFER_STRING_LEN);

    LOG_TRACE(LOG_FILTER_AST, "Processing module%s: %s",
              (module->isSharedLib() ? "[shared library]" : ""), modname);

    std::vector<BPatch_function *> bp_functions;
    module->getProcedures(bp_functions, true);
    for (auto bp_function : bp_functions) {
      uint64_t start = 0;
      uint64_t end = 0;
      bp_function->getAddressRange(start, end);

      auto funcname = [&]() {
        auto typed_name = bp_function->getTypedName();
        if (typed_name.empty())
          return bp_function->getName();
        return typed_name;
      }();

      LOG_DEBUG(LOG_FILTER_AST, "Processing function %s [%lx -> %lx]",
                funcname.c_str(), start, end);

      functions.emplace_back(bp_function, is_plt(start), funcname, start, end);
    }
  }

  LOG_INFO(LOG_FILTER_AST, "Initializing function borders...");
  function_borders::initialize(functions);

  // here we should process BBs for the functions (remove the need for
  // function_borders
  // outside of here);

  LOG_INFO(LOG_FILTER_AST, "Setting Basic Blocks...");
  for (auto &ast_function : functions) {
    auto function = ast_function.get_function();
    // TODO REMOVE (HACKFIX)
    std::set<BPatch_basicBlock *> blocks;
    BPatch_flowGraph *cfg = function->getCFG();
    cfg->getAllBasicBlocks(blocks);
    function_borders::apply(function, blocks);

    std::vector<basic_block> basic_blocks;
    for (auto block : blocks) {
      bool is_indirect_callsite = false;
      instrument_basicBlock_decoded(
          block, decoder, [&](CADecoder *instr_decoder) {
            // get instruction bytes
            auto const address = instr_decoder->get_address();

            if (instr_decoder->is_indirect_call())
              is_indirect_callsite = true;
          });

      basic_blocks.emplace_back(block, block->getStartAddress(),
                                block->getEndAddress(), is_indirect_callsite);
    }

    ast_function.update_basic_blocks(basic_blocks);
  }

  LOG_INFO(LOG_FILTER_AST, "Resolving function names of plt functions...");
  functions = resolve_plt_names(object, functions, decoder);

  LOG_INFO(LOG_FILTER_AST, "Performing address taken analysis...");
  functions = address_taken_analysis(object, functions, decoder);

  return ast(is_shared, objname, functions, regions);
}
