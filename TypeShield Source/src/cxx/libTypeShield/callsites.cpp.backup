#include "callsites.h"

#include "ca_decoder.h"
#include "instrumentation.h"
#include "liveness/liveness_analysis.h"
#include "logging.h"
#include "reaching/reaching_analysis.h"
#include "systemv_abi.h"

#include <boost/optional.hpp>
#include <boost/range/adaptor/reversed.hpp>

namespace TypeShield {

namespace callsites {

/*
struct MemoryCallSite {
  MemoryCallSite(BPatch_function *function_, uint64_t block_start_,
                 uint64_t address_, std::array<char, 7> parameters_,
                 reaching::RegisterStates reaching_state_, size_t index_)
      : function(function_), block_start(block_start_), address(address_),
        parameters(parameters_), reaching_state(reaching_state_),
        index(index_) {}

  BPatch_function *function;
  uint64_t block_start;
  uint64_t address;
  std::array<char, 7> parameters;
  reaching::RegisterStates reaching_state;
  size_t index;
};template <> inline std::string to_string(MemoryCallSite const &call_site)
{
    Dyninst::Address start, end;

    auto funcname = [&]() {
        auto typed_name = call_site.function->getTypedName();
        if (typed_name.empty())
            return call_site.function->getName();
        return typed_name;
    }();

    call_site.function->getAddressRange(start, end);

    return "<CS*>;" + funcname + ";" + ":" + ";" + int_to_hex(start) + ";" +
           param_to_string(call_site.parameters) + " | " +
           to_string(call_site.reaching_state) + ";" +
int_to_hex(call_site.block_start) +
           ";" + int_to_hex(call_site.address);
}*/

struct memory_cs_t {
  memory_cs_t(BPatch_function *function_, uint64_t address_,
              uint64_t block_address_, liveness::RegisterStates liveness_,
              reaching::RegisterStates reaching_)
      : function(function_), address(address_), block_address(block_address_),
        liveness(liveness_), reaching(reaching_) {}

  BPatch_function *function;
  uint64_t address;
  uint64_t block_address;
  liveness::RegisterStates liveness;
  reaching::RegisterStates reaching;
};

static std::vector<result_t>
_analysis_single(ast::ast const &ast, CADecoder *decoder,
                 std::vector<liveness::AnalysisConfig> &liveness_configs,
                 std::vector<reaching::AnalysisConfig> &reaching_configs,
                 std::string type, int reaching_index) {
  std::unordered_map<uint64_t, reaching::RegisterStates> reaching_lookup;

  auto &reaching_config = reaching_configs[reaching_index];

  std::unordered_map<uint64_t, std::vector<memory_cs_t>> memory_callsites;

  for (auto const &ast_function : ast.get_functions()) {
    if (!ast_function.get_is_plt()) {
      auto const funcname = ast_function.get_name();

      for (auto const &block : ast_function.get_basic_blocks()) {
        if (block.get_is_indirect_callsite()) {
          instrument_basicBlock_decoded(
              block.get_basic_block(), decoder, [&](CADecoder *instr_decoder) {

                auto const address = instr_decoder->get_address();
                if (!instr_decoder->is_indirect_call())
                  return;

                auto store_address = instr_decoder->get_address_store_address();

                LOG_INFO(LOG_FILTER_CALL_SITE,
                         "<CS> basic block [%lx:%lx] instruction %lx",
                         block.get_address(), block.get_end(), address);

                auto reaching_state = reaching::analysis(
                    reaching_config, block.get_basic_block(), address);

                LOG_INFO(LOG_FILTER_CALL_SITE, "\tREGISTER_STATE %s",
                         to_string(reaching_state).c_str());

                auto liveness_state = liveness::RegisterStates();

                if (store_address != 0) {
                  LOG_INFO(LOG_FILTER_CALL_SITE,
                           "<CS*> instruction %lx is using fptr stored at %lx",
                           address, store_address);

                  memory_callsites[store_address].emplace_back(
                      ast_function.get_function(), address, block.get_address(),
                      liveness_state, reaching_state);
                } else {
                  reaching_lookup[block.get_address()] = reaching_state;
                }
              });
        }
      }
    }
  }

  LOG_INFO(LOG_FILTER_CALL_SITE, "fptr storage analysis");

  for (auto &&addr_cs_pairs : memory_callsites) {
    // LOG_INFO(LOG_FILTER_CALL_SITE, "%lx -> %s", addr_cs_pairs.first,
    //         to_string(addr_cs_pairs.second).c_str());

    std::vector<reaching::RegisterStates> reaching_states;
    for (auto const &memory_callsite : addr_cs_pairs.second) {
      reaching_states.push_back(memory_callsite.reaching);
    }

    auto reaching_state = reaching_config.merge_horizontal_rip(reaching_states);

    for (auto const &memory_callsite : addr_cs_pairs.second) {
      reaching_lookup[memory_callsite.block_address] = reaching_state;
    }
  }

  std::vector<result_t> results;

  for (size_t index = 0; index < liveness_configs.size(); ++index) {
    result_t result;
    result.config_name = std::to_string(reaching_index) + type + std::to_string(index);

    auto &liveness_config = liveness_configs[index];

    for (auto const &ast_function : ast.get_functions()) {
      if (!ast_function.get_is_plt()) {
        auto const funcname = ast_function.get_name();

        for (auto const &block : ast_function.get_basic_blocks()) {
          if (block.get_is_indirect_callsite()) {
            instrument_basicBlock_decoded(
                block.get_basic_block(), decoder,
                [&](CADecoder *instr_decoder) {

                  auto const address = instr_decoder->get_address();
                  if (!instr_decoder->is_indirect_call())
                    return;

                  LOG_INFO(LOG_FILTER_CALL_SITE,
                           "<CS> basic block [%lx:%lx] instruction %lx",
                           block.get_address(), block.get_end(), address);

                  auto reaching_state = reaching_lookup[block.get_address()];

                  BPatch_Vector<BPatch_basicBlock *> targets;
                  block.get_basic_block()->getTargets(targets);
                  function_borders::apply(block.get_basic_block(), targets);
                  auto liveness_state =
                      liveness::analysis(liveness_config, targets);

                  result.param_lookup[block.get_address()] =
                      system_v::callsite::generate_paramlist(
                          ast_function.get_function(), address, reaching_state,
                          liveness_state);

                });
          }
        }
      }
    }
    results.push_back(result);
  }

  return results;
}

static std::vector<result_t>
_analysis(ast::ast const &ast, CADecoder *decoder,
          std::vector<liveness::AnalysisConfig> &liveness_configs,
          std::vector<reaching::AnalysisConfig> &reaching_configs,
          std::string type) {

  std::vector<result_t> results;

  for (size_t index = 0; index < reaching_configs.size(); ++index) {
    result_t result;
    result.config_name = std::to_string(index) + type;

    auto &reaching_config = reaching_configs[index];

    std::unordered_map<uint64_t, std::vector<memory_cs_t>> memory_callsites;

    for (auto const &ast_function : ast.get_functions()) {
      if (!ast_function.get_is_plt()) {
        auto const funcname = ast_function.get_name();

        for (auto const &block : ast_function.get_basic_blocks()) {
          if (block.get_is_indirect_callsite()) {
            instrument_basicBlock_decoded(
                block.get_basic_block(), decoder,
                [&](CADecoder *instr_decoder) {

                  auto const address = instr_decoder->get_address();
                  if (!instr_decoder->is_indirect_call())
                    return;

                  auto store_address =
                      instr_decoder->get_address_store_address();

                  LOG_INFO(LOG_FILTER_CALL_SITE,
                           "<CS> basic block [%lx:%lx] instruction %lx",
                           block.get_address(), block.get_end(), address);

                  reaching_config.block_states = reaching::RegisterStateMap();
                  auto reaching_state = reaching::analysis(
                      reaching_config, block.get_basic_block(), address);
                  reaching_config.block_states.clear();

                  LOG_INFO(LOG_FILTER_CALL_SITE, "\tREGISTER_STATE %s",
                           to_string(reaching_state).c_str());

                  auto liveness_state = liveness::RegisterStates();

                  if (store_address != 0) {
                    LOG_INFO(
                        LOG_FILTER_CALL_SITE,
                        "<CS*> instruction %lx is using fptr stored at %lx",
                        address, store_address);

                    memory_callsites[store_address].emplace_back(
                        ast_function.get_function(), address,
                        block.get_address(), liveness_state, reaching_state);
                  } else {
                    result.param_lookup[block.get_address()] =
                        system_v::callsite::generate_paramlist(
                            ast_function.get_function(), address,
                            reaching_state, liveness_state);
                  }
                });
          }
        }
      }
    }

    LOG_INFO(LOG_FILTER_CALL_SITE, "fptr storage analysis");

    for (auto &&addr_cs_pairs : memory_callsites) {
      // LOG_INFO(LOG_FILTER_CALL_SITE, "%lx -> %s", addr_cs_pairs.first,
      //         to_string(addr_cs_pairs.second).c_str());

      std::vector<reaching::RegisterStates> reaching_states;
      for (auto const &memory_callsite : addr_cs_pairs.second) {
        reaching_states.push_back(memory_callsite.reaching);
      }

      auto reaching_state =
          reaching_config.merge_horizontal_rip(reaching_states);

      for (auto const &memory_callsite : addr_cs_pairs.second) {
        result.param_lookup[memory_callsite.block_address] =
            system_v::callsite::generate_paramlist(
                memory_callsite.function, memory_callsite.address,
                reaching_state, memory_callsite.liveness);
      }
    }

    results.push_back(result);
  }

  return results;
}

std::vector<result_t> count::analysis(ast::ast const &ast, CADecoder *decoder,
                                      int liveness_cfg, int reaching_cfg) {
  auto liveness_configs = liveness::count::callsite::init(decoder, ast);
  auto reaching_configs = reaching::count::callsite::init(decoder, ast);

  if (reaching_cfg < 0)
    return _analysis(ast, decoder, liveness_configs, reaching_configs, "count");
  else
    return _analysis_single(ast, decoder, liveness_configs, reaching_configs,
                            "count", reaching_cfg);
}

std::vector<result_t> type::analysis(ast::ast const &ast, CADecoder *decoder,
                                     int liveness_cfg, int reaching_cfg) {
  auto liveness_configs = liveness::type::callsite::init(decoder, ast);
  auto reaching_configs = reaching::type::callsite::init(decoder, ast);

  if (reaching_cfg < 0)
    return _analysis(ast, decoder, liveness_configs, reaching_configs, "type");
  else
    return _analysis_single(ast, decoder, liveness_configs, reaching_configs,
                            "type", reaching_cfg);
}
}
}

#if 0
bool is_virtual_callsite(CADecoder *decoder, BPatch_basicBlock *block)
{
    // 400855:       48 8b 45 e8             mov    -0x18(%rbp),%rax
    // 400859:       48 8b 08                mov    (%rax),%rcx
    // 40085c:       48 8b 09                mov    (%rcx),%rcx
    // 40085f:       8b 75 f8                mov    -0x8(%rbp),%esi // parameter from
    // stack (main
    // fkt
    // ?)
    // 400862:       48 89 c7                mov    %rax,%rdi
    // 400865:       ff d1                   callq  *%rcx

    // MOV ? -> %THIS_PTR
    // MOV (%THIS_PTR) -> %ADDR_REG
    // MOV (%ADDR_REG) -> %ADDR_REG
    // PARAMS
    // mov THIS_PTR, %RDI
    // call *%ADDR_REG

    boost::optional<int> addr_REG;
    boost::optional<int> this_REG;
    bool addr_deref = false;

    Dyninst::PatchAPI::PatchBlock::Insns insns;
    Dyninst::PatchAPI::convert(block)->getInsns(insns);
    for (auto &instruction : boost::adaptors::reverse(insns))
    {
        // Instrument Instruction
        auto address = reinterpret_cast<uint64_t>(instruction.first);
        LOG_TRACE(LOG_FILTER_INSTRUMENTATION, "Processing instruction %lx", address);
        // call *%ADDR_REG
        if (addr_REG)
        {
            // mov THIS_PTR, %RDI
            if (this_REG)
            {
                // MOV (%ADDR_REG) -> %ADDR_REG
                if (addr_deref)
                {
                    // MOV (%THIS_PTR) -> %ADDR_REG
                    if (decoder->is_move_to(addr_REG.value()) &&
                        decoder->get_reg_source(0) == this_REG)
                    {
                        return true;
                    }
                }
                else if (decoder->is_move_to(addr_REG.value()))
                {
                    addr_deref = decoder->get_reg_source(0) == addr_REG;
                }
            }
            else if (decoder->is_move_to(system_v::get_parameter_register_from_index(0)))
            {
                this_REG = decoder->get_reg_source(0);
            }
        }
        else if (decoder->is_indirect_call())
        {
            addr_REG = decoder->get_reg_source(0);
        }
    }

    return false;
}
#endif
#if 0
struct MemoryCallSite
{
    MemoryCallSite(BPatch_function *function_, uint64_t block_start_, uint64_t address_,
                   std::array<char, 7> parameters_,
                   reaching::RegisterStates reaching_state_, size_t index_)
        : function(function_), block_start(block_start_), address(address_),
          parameters(parameters_), reaching_state(reaching_state_), index(index_)
    {
    }

    BPatch_function *function;
    uint64_t block_start;
    uint64_t address;
    std::array<char, 7> parameters;
    reaching::RegisterStates reaching_state;
    size_t index;
};

template <> inline std::string to_string(MemoryCallSite const &call_site)
{
    Dyninst::Address start, end;

    auto funcname = [&]() {
        auto typed_name = call_site.function->getTypedName();
        if (typed_name.empty())
            return call_site.function->getName();
        return typed_name;
    }();

    call_site.function->getAddressRange(start, end);

    return "<CS*>;" + funcname + ";" + ":" + ";" + int_to_hex(start) + ";" +
           param_to_string(call_site.parameters) + " | " +
           to_string(call_site.reaching_state) + ";" + int_to_hex(call_site.block_start) +
           ";" + int_to_hex(call_site.address);
}

std::vector<CallSites> callsite_analysis(ast::ast const &ast, CADecoder *decoder)
{
#ifdef __PADYN_TYPE_POLICY
    // auto liveness_configs = liveness::type::callsite::init(decoder, ast);
    auto reaching_configs = reaching::type::callsite::init(decoder, ast);

#elif defined(__PADYN_COUNT_EXT_POLICY)
    // auto liveness_configs = liveness::count_ext::callsite::init(decoder, ast);
    auto reaching_configs = reaching::count_ext::callsite::init(decoder, ast);

#else
    // auto liveness_configs = liveness::count::callsite::init(decoder, ast);
    auto reaching_configs = reaching::count::callsite::init(decoder, ast);
#endif

    std::vector<CallSites> call_sites_vector;

    for (auto index = 0; index < reaching_configs.size(); ++index)
    {
        // auto liveness_config = liveness_configs[index];
        auto reaching_config = reaching_configs[index];

        CallSites call_sites;

        std::unordered_map<uint64_t, std::vector<MemoryCallSite>> memory_callsites;

        util::for_each(ast.get_functions(), [&](ast::function const &ast_function) {
            auto function = ast_function.get_function();

            instrument_function_basicBlocks_unordered(function, [&](BPatch_basicBlock
                                                                        *block) {
                instrument_basicBlock_decoded(
                    block, decoder, [&](CADecoder *instr_decoder) {
                        // get instruction bytes
                        auto const address = instr_decoder->get_address();

                        if (instr_decoder->is_indirect_call())
                        {
                            auto store_address =
                                instr_decoder->get_address_store_address();

                            LOG_INFO(LOG_FILTER_CALL_SITE,
                                     "<CS> basic block [%lx:%lx] instruction %lx",
                                     block->getStartAddress(), block->getEndAddress(),
                                     address);

                            // reaching_config.block_states =
                            // reaching::RegisterStateMap();
                            auto reaching_state =
                                reaching::analysis(reaching_config, block, address);
                            LOG_INFO(LOG_FILTER_CALL_SITE, "\tREGISTER_STATE %s",
                                     to_string(reaching_state).c_str());

                            BPatch_Vector<BPatch_basicBlock *> targets;
                            block->getTargets(targets);

                            // liveness_config.block_states =
                            // liveness::RegisterStateMap();
                            auto liveness_state = liveness::RegisterStates();
                            //    liveness::analysis(liveness_config, targets);
                            // LOG_INFO(LOG_FILTER_CALL_SITE,
                            //         "\tRETURN_REGISTER_STATE %s",
                            //         to_string(liveness_state).c_str());

                            // is_virtual_callsite(instr_decoder, block);

                            auto parameters = system_v::callsite::generate_paramlist(
                                function, address, reaching_state, liveness_state);

                            if (store_address != 0)
                            {
                                LOG_INFO(
                                    LOG_FILTER_CALL_SITE,
                                    "<CS*> instruction %lx is using fptr stored at %lx",
                                    address, store_address);

                                auto iterator = call_sites.emplace(
                                    call_sites.end(), function, block->getStartAddress(),
                                    address, parameters);
                                auto element_index = iterator - call_sites.begin();

                                memory_callsites[store_address].emplace_back(
                                    function, block->getStartAddress(), address,
                                    parameters, reaching_state, element_index);
                            }
                            else
                            {
                                call_sites.emplace_back(function,
                                                        block->getStartAddress(), address,
                                                        parameters);
                            }
                        }
                    });
            });
        });

        LOG_INFO(LOG_FILTER_CALL_SITE, "fptr storage analysis");

        for (auto &&addr_cs_pairs : memory_callsites)
        {
            LOG_INFO(LOG_FILTER_CALL_SITE, "%lx -> %s", addr_cs_pairs.first,
                     to_string(addr_cs_pairs.second).c_str());

            std::vector<reaching::RegisterStates> reaching_states;
            for (auto const &memory_callsite : addr_cs_pairs.second)
            {
                reaching_states.push_back(memory_callsite.reaching_state);
            }

            auto reaching_state = reaching_config.merge_horizontal_rip(reaching_states);

            for (auto const &memory_callsite : addr_cs_pairs.second)
            {
                call_sites[memory_callsite.index].parameters =
                    system_v::callsite::generate_paramlist(
                        memory_callsite.function, memory_callsite.address, reaching_state,
                        liveness::RegisterStates());
            }
        }

        // liveness_config.block_states.clear();
        reaching_config.block_states.clear();

        call_sites_vector.push_back(call_sites);
    }

    return call_sites_vector;
}
#endif
