#ifndef __LIVENESS_IMPL_H
#define __LIVENESS_IMPL_H

#include "liveness_analysis.h"

#include "ast/ast.h"
#include "register_states.h"
#include "systemv_abi.h"

class CADecoder;

namespace liveness {
namespace impl {

void initialize_variadic_passthrough(AnalysisConfig &config,
                                     ast::ast const &ast);

void initialize_quasi_empty_return_blocks(AnalysisConfig &config,
                                          ast::ast const &ast);

template <typename merge_fun_t>
inline RegisterStates merge_horizontal(std::vector<RegisterStates> states,
                                       merge_fun_t reg_merge_fun) {
  RegisterStates target_state;
  LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t\tMerging states horizontal:");
  for (auto const &state : states)
    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t\t\t%s",
              to_string(state).c_str());
  if (states.size() == 1) {
    target_state = states[0];
  } else if (states.size() > 1) {
    target_state = std::accumulate(++(states.begin()), states.end(),
                                   states.front(), reg_merge_fun);
  }
  return target_state;
}
};
};

#endif /* __LIVENESS_IMPL_H */
