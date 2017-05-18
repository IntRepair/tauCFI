#include "analysis.h"

#include <chrono>

#include "calltargets.h"
#include "logging.h"

namespace TypeShield {

namespace analysis {

result_t run(ast::ast const &ast, config_t const &config, CADecoder *decoder) {
  result_t result;
  if (config.flags == NO_ANALYSIS)
    return result;

  /*
      Generate CONFIG Factory based on

      COUNT_ANALYSIS,
      TYPE_ANALYSIS
  config::generator_t config_gen;
  */

  auto const begin_cta = std::chrono::high_resolution_clock::now();
  if ((config.flags & CALLTARGET_ANALYSIS) == CALLTARGET_ANALYSIS) {
    if (config.type == TYPE_ANALYSIS)
      result.calltargets = calltargets::type::analysis(
          ast, decoder, config.ct_liveness_config, config.ct_reaching_config);
    else if (config.type == COUNT_ANALYSIS)
      result.calltargets = calltargets::count::analysis(
          ast, decoder, config.ct_liveness_config, config.ct_reaching_config);
  }
  auto const end_cta = std::chrono::high_resolution_clock::now();

  auto const begin_csa = std::chrono::high_resolution_clock::now();
  if ((config.flags & CALLSITE_ANALYSIS) == CALLSITE_ANALYSIS) {
    if (config.type == TYPE_ANALYSIS)
      result.callsites = callsites::type::analysis(
          ast, decoder, config.cs_liveness_config, config.cs_reaching_config);
    else if (config.type == COUNT_ANALYSIS)
      result.callsites = callsites::count::analysis(
          ast, decoder, config.cs_liveness_config, config.cs_reaching_config);
  }
  auto const end_csa = std::chrono::high_resolution_clock::now();

  LOG_INFO(
      LOG_FILTER_GENERAL, "CallTarget Analysis took place in %ld ms",
      std::chrono::duration_cast<std::chrono::milliseconds>(end_cta - begin_cta)
          .count());
  LOG_INFO(
      LOG_FILTER_GENERAL, "CallSite Analysis took place in %ld ms",
      std::chrono::duration_cast<std::chrono::milliseconds>(end_csa - begin_csa)
          .count());

    return result;
}
}
}