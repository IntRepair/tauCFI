#include "liveness_analysis.h"

#include <PatchCFG.h>

#include "instrumentation.h"
#include "liveness_impl.h"

namespace liveness {

namespace count {

namespace {

inline bool has_clear_param_regs(RegisterStates &reg_state) {
  bool undefined_regs = false;
  for (auto i = static_cast<size_t>(RegisterStates::min_index);
       i <= static_cast<size_t>(RegisterStates::max_index); ++i) {
    if (system_v::is_parameter_register(i))
      undefined_regs |= (reg_state[i] == REGISTER_CLEAR);
  }
  return undefined_regs;
}

inline bool has_clear_return_regs(RegisterStates &reg_state) {
  bool undefined_regs = false;
  for (auto i = static_cast<size_t>(RegisterStates::min_index);
       i <= static_cast<size_t>(RegisterStates::max_index); ++i) {
    if (system_v::is_return_register(i))
      undefined_regs |= (reg_state[i] == REGISTER_CLEAR);
  }
  return undefined_regs;
}

static RegisterStates merge_vertical(RegisterStates current,
                                     RegisterStates delta) {
  return transform(current, delta,
                   [](liveness::state_t current, liveness::state_t delta) {
                     return (current == REGISTER_CLEAR) ? delta : current;
                   });
}

static RegisterStates
merge_horizontal_destructive(std::vector<RegisterStates> states) {
  return impl::merge_horizontal(
      states, [](RegisterStates current, RegisterStates delta) {
        return transform(current, delta, [](state_t current, state_t delta) {
          if (is_read_before_write(current) && delta == REGISTER_CLEAR)
            current = REGISTER_WRITE_BEFORE_READ_FULL;
          else if (is_write_before_read(current) || is_write_before_read(delta))
            current = REGISTER_WRITE_BEFORE_READ_FULL;
          return current;
        });
      });
}

static RegisterStates
merge_horizontal_intersection(std::vector<RegisterStates> states) {
  return impl::merge_horizontal(
      states, [](RegisterStates current, RegisterStates delta) {
        return transform(current, delta, [](state_t current, state_t delta) {
          if (is_read_before_write(current) && delta == REGISTER_CLEAR)
            current = REGISTER_CLEAR;
          else if (is_write_before_read(current) || is_write_before_read(delta))
            current = REGISTER_WRITE_BEFORE_READ_FULL;
          return current;
        });
      });
}

static RegisterStates
merge_horizontal_union(std::vector<RegisterStates> states) {
  return impl::merge_horizontal(
      states, [](RegisterStates current, RegisterStates delta) {
        return transform(current, delta, [](state_t current, state_t delta) {
          if (is_write_before_read(current) || is_write_before_read(delta))
            current = REGISTER_WRITE_BEFORE_READ_FULL;
          else if (is_read_before_write(current) || is_read_before_write(delta))
            current = REGISTER_READ_BEFORE_WRITE_FULL;
          return current;
        });
      });
}

}; /* namespace */

namespace calltarget {
std::vector<AnalysisConfig> init(CADecoder *decoder, ast::ast const &ast) {
  std::vector<AnalysisConfig> configs;

  AnalysisConfig config(ast);
  config.decoder = decoder;
  config.can_change = has_clear_param_regs;
  config.merge_vertical = &merge_vertical;
  impl::initialize_variadic_passthrough(config, ast);

  /* 0 - 1 */
  config.merge_horizontal = &merge_horizontal_intersection;
  config.follow_calls = false;
  configs.push_back(config);
  config.follow_calls = true;
  configs.push_back(config);

  /* 2 - 3 */
  config.merge_horizontal = &merge_horizontal_destructive;
  config.follow_calls = false;
  configs.push_back(config);
  config.follow_calls = true;
  configs.push_back(config);

  /* 4 - 5 */
  config.merge_horizontal = &merge_horizontal_union;
  config.follow_calls = false;
  configs.push_back(config);
  config.follow_calls = true;
  configs.push_back(config);

  return configs;
}
}; /* namespace calltarget */

namespace callsite {

std::vector<AnalysisConfig> init(CADecoder *decoder, ast::ast const &ast) {
  std::vector<AnalysisConfig> configs;

  AnalysisConfig config(ast);
  config.decoder = decoder;
  config.merge_vertical = &merge_vertical;
  config.can_change = &has_clear_return_regs;
  impl::initialize_variadic_passthrough(config, ast);

  config.merge_horizontal = &merge_horizontal_intersection;
  config.follow_calls = false;
  configs.push_back(config);
  config.follow_calls = true;
  configs.push_back(config);

  config.merge_horizontal = &merge_horizontal_union;
  config.follow_calls = false;
  configs.push_back(config);
  config.follow_calls = true;
  configs.push_back(config);

  config.merge_horizontal = &merge_horizontal_union;
  config.follow_calls = false;
  configs.push_back(config);
  config.follow_calls = true;
  configs.push_back(config);

  return configs;
}
}; /* namespace callsite */

}; /* namespace count */
}; /* namespace liveness */
