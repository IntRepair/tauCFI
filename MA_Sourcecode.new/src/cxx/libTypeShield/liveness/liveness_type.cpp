#include "liveness_analysis.h"

#include <PatchCFG.h>

#include "instrumentation.h"
#include "liveness_impl.h"

namespace liveness {

namespace type {
namespace {
inline bool has_unwritten_param_regs(RegisterStates &reg_state) {
  bool undefined_regs = false;
  for (auto i = static_cast<size_t>(RegisterStates::min_index);
       i <= static_cast<size_t>(RegisterStates::max_index); ++i) {
    if (system_v::is_parameter_register(i))
      undefined_regs |= !is_write_before_read(reg_state[i]);
  }
  return undefined_regs;
}

inline bool has_unwritten_return_regs(RegisterStates &reg_state) {
  bool undefined_regs = false;
  for (auto i = static_cast<size_t>(RegisterStates::min_index);
       i <= static_cast<size_t>(RegisterStates::max_index); ++i) {
    if (system_v::is_return_register(i))
      undefined_regs |= !is_write_before_read(reg_state[i]);
  }
  return undefined_regs;
}

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

static RegisterStates merge_vertical_first(RegisterStates current,
                                           RegisterStates delta) {
  return transform(current, delta,
                   [](liveness::state_t current, liveness::state_t delta) {
                     return (current == REGISTER_CLEAR) ? delta : current;
                   });
}

static RegisterStates merge_vertical_inter(RegisterStates current,
                                           RegisterStates delta) {
  return transform(current, delta,
                   [](liveness::state_t current, liveness::state_t delta) {
                     if (current == REGISTER_CLEAR)
                       return delta;
                     if (is_write_before_read(current))
                       return current;
                     if (is_write_before_read(delta))
                       current |= REGISTER_WRITE_BEFORE_READ_FULL;
                     if (is_read_before_write(delta))
                       return current & delta;
                     return current;
                   });
}

static RegisterStates merge_vertical_union(RegisterStates current,
                                           RegisterStates delta) {
  return transform(current, delta,
                   [](liveness::state_t current, liveness::state_t delta) {
                     if (current == REGISTER_CLEAR)
                       return delta;
                     if (is_write_before_read(current))
                       return current;
                     return current | delta;
                   });
}

static RegisterStates
merge_horizontal_union_first(std::vector<RegisterStates> states) {
  return impl::merge_horizontal(
      states, [](RegisterStates current, RegisterStates delta) {
        return transform(current, delta, [](state_t current, state_t delta) {
          if (is_write_before_read(current) || is_write_before_read(delta))
            return REGISTER_WRITE_BEFORE_READ_FULL;
          else if (is_read_before_write(current) || is_read_before_write(delta))
            return current | delta;
          return current;
        });
      });
}

static RegisterStates
merge_horizontal_union(std::vector<RegisterStates> states) {
  return impl::merge_horizontal(states, [](RegisterStates current,
                                           RegisterStates delta) {
    return transform(current, delta, [](state_t current, state_t delta) {
      if ((is_write_before_read(current) && !is_read_before_write(current)) ||
          (is_write_before_read(delta) && !is_read_before_write(delta)))
        return REGISTER_WRITE_BEFORE_READ_FULL;
      return current | delta;
    });
  });
}
};

namespace calltarget {
std::vector<AnalysisConfig> init(CADecoder *decoder, ast::ast const &ast) {
  std::vector<AnalysisConfig> configs;
  AnalysisConfig config(ast);
  config.decoder = decoder;
  impl::initialize_variadic_passthrough(config, ast);

  /* 0 -> 7 */
  config.merge_vertical = &merge_vertical_first;
  config.merge_horizontal = &merge_horizontal_union_first;
  config.can_change = &has_clear_param_regs;
  //config.follow_calls = false;
  //configs.push_back(config);
  config.follow_calls = true;
  configs.push_back(config);

  config.can_change = &has_unwritten_param_regs;
  //config.follow_calls = false;
  //configs.push_back(config);
  config.follow_calls = true;
  configs.push_back(config);

  config.merge_horizontal = &merge_horizontal_union;
  config.can_change = &has_clear_param_regs;
  //config.follow_calls = false;
  //configs.push_back(config);
  config.follow_calls = true;
  configs.push_back(config);

  config.can_change = &has_unwritten_param_regs;
  //config.follow_calls = false;
  //configs.push_back(config);
  config.follow_calls = true;
  configs.push_back(config);

  /* 8 -> 15 */
  config.merge_vertical = &merge_vertical_inter;
  config.merge_horizontal = &merge_horizontal_union_first;
  config.can_change = &has_clear_param_regs;
  //config.follow_calls = false;
  //configs.push_back(config);
  config.follow_calls = true;
  configs.push_back(config);

  config.can_change = &has_unwritten_param_regs;
  //config.follow_calls = false;
  //configs.push_back(config);
  config.follow_calls = true;
  configs.push_back(config);

  config.merge_horizontal = &merge_horizontal_union;
  config.can_change = &has_clear_param_regs;
  //config.follow_calls = false;
  //configs.push_back(config);
  config.follow_calls = true;
  configs.push_back(config);

  config.can_change = &has_unwritten_param_regs;
  //config.follow_calls = false;
  //configs.push_back(config);
  config.follow_calls = true;
  configs.push_back(config);

  /* 16 -> 23 */
  config.merge_vertical = &merge_vertical_union;
  config.merge_horizontal = &merge_horizontal_union_first;
  config.can_change = &has_clear_param_regs;
  //config.follow_calls = false;
  //configs.push_back(config);
  config.follow_calls = true;
  configs.push_back(config);

  config.can_change = &has_unwritten_param_regs;
  //config.follow_calls = false;
  //configs.push_back(config);
  config.follow_calls = true;
  configs.push_back(config);

  config.merge_horizontal = &merge_horizontal_union;
  config.can_change = &has_clear_param_regs;
  //config.follow_calls = false;
  //configs.push_back(config);
  config.follow_calls = true;
  configs.push_back(config);

  config.can_change = &has_unwritten_param_regs;
  //config.follow_calls = false;
  //configs.push_back(config);
  config.follow_calls = true;
  configs.push_back(config);

  /*
      config.follow_calls = true;
      {
          AnalysisConfig config(init_config);
          config.merge_vertical = &merge_vertical_first;
          config.merge_horizontal = &merge_horizontal_union_first;
          config.can_change = &has_clear_param_regs;

          configs.push_back(config);
      }
      {
          AnalysisConfig config(init_config);
          config.merge_vertical = &merge_vertical_inter;
          config.merge_horizontal = &merge_horizontal_union;
          config.can_change = &has_unwritten_param_regs;

          configs.push_back(config);
      }
      {
          AnalysisConfig config(init_config);
          config.merge_vertical = &merge_vertical_union;
          config.merge_horizontal = &merge_horizontal_union;
          config.can_change = &has_unwritten_param_regs;

          configs.push_back(config);
      }
      */

  //{
  //    AnalysisConfig config(init_config);
  //    config.merge_vertical = &merge_vertical;
  //    config.merge_horizontal = &merge_horizontal;
  //    config.can_change = &has_unwritten_param_regs;
  //    config.follow_calls = false;
  //
  //    configs.push_back(config);
  //}

  return configs;
}
}; /* namespace calltarget */

namespace callsite {

static RegisterStates merge_vertical(RegisterStates current,
                                     RegisterStates delta) {
  return transform(current, delta,
                   [](liveness::state_t current, liveness::state_t delta) {
                     if (is_write_before_read(current))
                       return current;
                     return current | delta;
                   });
}

static RegisterStates merge_horizontal(std::vector<RegisterStates> states) {
  return impl::merge_horizontal(
      states, [](RegisterStates current, RegisterStates delta) {
        return transform(current, delta, [](state_t current, state_t delta) {
          return current | delta;
        });
      });
}

std::vector<AnalysisConfig> init(CADecoder *decoder, ast::ast const &ast) {
  std::vector<AnalysisConfig> configs;
  AnalysisConfig config(ast);
  config.decoder = decoder;
  impl::initialize_variadic_passthrough(config, ast);

  /* 0 -> 7 */
  config.merge_vertical = &merge_vertical_first;
  config.merge_horizontal = &merge_horizontal_union_first;
  config.can_change = &has_clear_return_regs;
  config.follow_calls = false;
  configs.push_back(config);
  //config.follow_calls = true;
  //configs.push_back(config);

  config.can_change = &has_unwritten_return_regs;
  config.follow_calls = false;
  configs.push_back(config);
  //config.follow_calls = true;
  //configs.push_back(config);

  config.merge_horizontal = &merge_horizontal_union;
  config.can_change = &has_clear_return_regs;
  config.follow_calls = false;
  configs.push_back(config);
  //config.follow_calls = true;
  //configs.push_back(config);

  config.can_change = &has_unwritten_return_regs;
  config.follow_calls = false;
  configs.push_back(config);
  //config.follow_calls = true;
  //configs.push_back(config);

  /* 8 -> 15 */
  config.merge_vertical = &merge_vertical_inter;
  config.merge_horizontal = &merge_horizontal_union_first;
  config.can_change = &has_clear_return_regs;
  config.follow_calls = false;
  configs.push_back(config);
  //config.follow_calls = true;
  //configs.push_back(config);

  config.can_change = &has_unwritten_return_regs;
  config.follow_calls = false;
  configs.push_back(config);
  //config.follow_calls = true;
  //configs.push_back(config);

  config.merge_horizontal = &merge_horizontal_union;
  config.can_change = &has_clear_return_regs;
  config.follow_calls = false;
  configs.push_back(config);
  //config.follow_calls = true;
  //configs.push_back(config);

  config.can_change = &has_unwritten_return_regs;
  config.follow_calls = false;
  configs.push_back(config);
  //config.follow_calls = true;
  //configs.push_back(config);

  /* 16 -> 23 */
  config.merge_vertical = &merge_vertical_union;
  config.merge_horizontal = &merge_horizontal_union_first;
  config.can_change = &has_clear_return_regs;
  config.follow_calls = false;
  configs.push_back(config);
  //config.follow_calls = true;
  //configs.push_back(config);

  config.can_change = &has_unwritten_return_regs;
  config.follow_calls = false;
  configs.push_back(config);
  //config.follow_calls = true;
  //configs.push_back(config);

  config.merge_horizontal = &merge_horizontal_union;
  config.can_change = &has_clear_return_regs;
  config.follow_calls = false;
  configs.push_back(config);
  //config.follow_calls = true;
  //configs.push_back(config);

  config.can_change = &has_unwritten_return_regs;
  config.follow_calls = false;
  configs.push_back(config);
  //config.follow_calls = true;
  //configs.push_back(config);

/*
  std::vector<AnalysisConfig> configs;
  AnalysisConfig config(ast);

  config.decoder = decoder;
  config.merge_vertical = &merge_vertical;
  config.merge_horizontal = &merge_horizontal;
  config.can_change = &has_clear_return_regs;

  config.follow_calls = false;

  configs.push_back(config);
  configs.push_back(config);
  configs.push_back(config);
  // configs.push_back(config);
*/
  return configs;
}
}; /* namespace callsite */

}; /* namespace count */
}; /* namespace liveness */
