#include "calltargets.h"

#include "liveness/liveness_analysis.h"
#include "reaching/reaching_analysis.h"
#include <systemv_abi.h>

namespace TypeShield {

namespace calltargets {

static std::vector<result_t>
_analysis_single(ast::ast const &ast, CADecoder *decoder,
                 std::vector<liveness::AnalysisConfig> &liveness_configs,
                 std::vector<reaching::AnalysisConfig> &reaching_configs,
                 std::string type, int liveness_index) {
  std::vector<result_t> results;

  auto &liveness_config = liveness_configs[liveness_index];

  std::unordered_map<uint64_t, liveness::RegisterStates> liveness_lookup;

  for (auto const &ast_function : ast.get_functions()) {
    auto function = ast_function.get_function();

    auto const funcname = ast_function.get_name();
    auto const start = ast_function.get_address();
#if 0
        if (ast_function.get_is_at())
#endif
    {
      LOG_DEBUG(LOG_FILTER_CALL_TARGET, "Looking at Function %s[%lx]",
                funcname.c_str(), start);

      liveness_config.block_states = liveness::RegisterStateMap();
      auto liveness_state = liveness::analysis(liveness_config, function);
      LOG_DEBUG(LOG_FILTER_CALL_TARGET, "\tREGISTER_STATE %s",
                to_string(liveness_state).c_str());

      liveness_lookup[start] = liveness_state;
    }
  }

  for (size_t index = 0; index < reaching_configs.size(); ++index) {
    result_t result;
    result.config_name =
        std::to_string(liveness_index) + type + std::to_string(index);

    auto &reaching_config = reaching_configs[index];

    for (auto const &ast_function : ast.get_functions()) {
      auto function = ast_function.get_function();

      auto const funcname = ast_function.get_name();
      auto const start = ast_function.get_address();
#if 0
        if (ast_function.get_is_at())
#endif
      {
        LOG_DEBUG(LOG_FILTER_CALL_TARGET, "Looking at Function %s[%lx]",
                  funcname.c_str(), start);

        reaching_config.block_states = reaching::RegisterStateMap();
        auto reaching_state = reaching::analysis(reaching_config, function);
        LOG_DEBUG(LOG_FILTER_CALL_TARGET, "\tRETURN_REGISTER_STATE %s",
                  to_string(reaching_state).c_str());

        result.param_lookup[start] = system_v::calltarget::generate_paramlist(
            function, start, reaching_state, liveness_lookup[start]);
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

  for (size_t index = 0; index < liveness_configs.size(); ++index) {
    result_t result;
    result.config_name = std::to_string(index) + type;

    auto &liveness_config = liveness_configs[index];

    for (auto const &ast_function : ast.get_functions()) {
      auto function = ast_function.get_function();

      auto const funcname = ast_function.get_name();
      auto const start = ast_function.get_address();
#if 0
        if (ast_function.get_is_at())
#endif
//        if (ast_function.get_name() == "buffer_append_string_encoded")
      {
        LOG_INFO(LOG_FILTER_GENERAL, "Looking at Function %s[%lx]",
                  funcname.c_str(), start);

        auto liveness_state = liveness::analysis(liveness_config, function);
        LOG_INFO(LOG_FILTER_GENERAL, "\tREGISTER_STATE %s",
                  to_string(liveness_state).c_str());

        auto reaching_state = reaching::RegisterStates();

        result.param_lookup[start] = system_v::calltarget::generate_paramlist(
            function, start, reaching_state, liveness_state);
      }
    }

    results.push_back(result);
  }

  return results;
}

/*

static std::vector<result_t>
_analysis(ast::ast const &ast, CADecoder *decoder,
          std::vector<liveness::AnalysisConfig> const &liveness_configs,
          std::vector<reaching::AnalysisConfig> const &reaching_configs,
          std::string type) {
  std::vector<result_t> results;

  for (auto index = 0; index < liveness_configs.size(); ++index) {
    result_t result;
    result.config_name = std::to_string(index) + type;

    auto &liveness_config = liveness_configs[index];
    // auto &reaching_config = reaching_configs[index];

    for (auto const &ast_function : ast.get_functions()) {
      auto function = ast_function.get_function();
      // Instrument function

      auto const funcname = ast_function.get_name();
      auto const start = ast_function.get_address();
#if 0
        if (ast_function.get_is_at())
#endif
      // if (funcname == "xfer_exit_ev")
      {
        LOG_DEBUG(LOG_FILTER_CALL_TARGET, "Looking at Function %s[%lx]",
                  funcname.c_str(), start);

        liveness_config.block_states = liveness::RegisterStateMap();
        auto liveness_state = liveness::analysis(liveness_config, function);
        LOG_DEBUG(LOG_FILTER_CALL_TARGET, "\tREGISTER_STATE %s",
                  to_string(liveness_state).c_str());

        // reaching_config.block_states = reaching::RegisterStateMap();
        auto reaching_state = reaching::RegisterStates();
        // reaching::analysis(reaching_config, function);
        // LOG_DEBUG(LOG_FILTER_CALL_TARGET, "\tRETURN_REGISTER_STATE %s",
        //          to_string(reaching_state).c_str());

        result.param_lookup[start] = system_v::calltarget::generate_paramlist(
            function, start, reaching_state, liveness_state);
      }
    }

    liveness_config.block_states.clear();
    // reaching_config.block_states.clear();

    results.push_back(result);
  }

  return results;
}
*/

std::vector<result_t> count::analysis(ast::ast const &ast, CADecoder *decoder,
                                      int liveness_cfg, int reaching_cfg) {
  auto liveness_configs = liveness::count::calltarget::init(decoder, ast);
  auto reaching_configs = reaching::count::calltarget::init(decoder, ast);

  if (liveness_cfg < 0)
    return _analysis(ast, decoder, liveness_configs, reaching_configs, "count");
  else
    return _analysis_single(ast, decoder, liveness_configs, reaching_configs,
                            "count", liveness_cfg);
}
std::vector<result_t> type::analysis(ast::ast const &ast, CADecoder *decoder,
                                     int liveness_cfg, int reaching_cfg) {
  auto liveness_configs = liveness::type::calltarget::init(decoder, ast);
  auto reaching_configs = reaching::type::calltarget::init(decoder, ast);

  if (liveness_cfg < 0)
    return _analysis(ast, decoder, liveness_configs, reaching_configs, "type");
  else
    return _analysis_single(ast, decoder, liveness_configs, reaching_configs,
                            "type", liveness_cfg);
}
}
}

#if 0
std::vector<CallTargets> calltarget_analysis(ast::ast const &ast, CADecoder *decoder)


{
    auto const plt = ast.get_region_data(".plt");
    LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS, "PLT(mem)  %lx -> %lx", plt.start, plt.end);

#ifdef __PADYN_TYPE_POLICY
    auto liveness_configs = liveness::type::calltarget::init(decoder, ast);
// auto reaching_configs = reaching::type::calltarget::init(decoder, ast);

#elif defined(__PADYN_COUNT_EXT_POLICY)
    auto liveness_configs = liveness::count_ext::calltarget::init(decoder, ast);
// auto reaching_configs = reaching::count_ext::calltarget::init(decoder, ast);

#else
    auto liveness_configs = liveness::count::calltarget::init(decoder, ast);
// auto reaching_configs = reaching::count::calltarget::init(decoder, ast);
#endif
    std::vector<CallTargets> call_targets_vector;

    for (auto index = 0; index < liveness_configs.size(); ++index)
    {
        auto &liveness_config = liveness_configs[index];
        // auto& reaching_config = reaching_configs[index];

        CallTargets call_targets;

        for (auto const &ast_function : ast.get_functions())
        {
            auto function = ast_function.get_function();
            // Instrument function
            Dyninst::Address start, end;
            char funcname[BUFFER_STRING_LEN];

            function->getName(funcname, BUFFER_STRING_LEN);
            function->getAddressRange(start, end);

#if 0
        std::string fname(funcname);
        if (fname != "v8::internal::StringStream::PrintByteArray")
            return;
#endif

// disabled for now, we simply tag the calltarget
#if 0
        if (ast_function.get_is_at())
#endif
            {
                LOG_DEBUG(LOG_FILTER_CALL_TARGET, "Looking at Function %s[%lx]", funcname,
                          start);

                liveness_config.block_states = liveness::RegisterStateMap();
                auto liveness_state = liveness::analysis(liveness_config, function);
                LOG_DEBUG(LOG_FILTER_CALL_TARGET, "\tREGISTER_STATE %s",
                          to_string(liveness_state).c_str());

                // reaching_config.block_states = reaching::RegisterStateMap();
                auto reaching_state = reaching::RegisterStates();
                // reaching::analysis(reaching_config, function);
                // LOG_DEBUG(LOG_FILTER_CALL_TARGET, "\tRETURN_REGISTER_STATE %s",
                //          to_string(reaching_state).c_str());

                auto parameters = system_v::calltarget::generate_paramlist(
                    function, start, reaching_state, liveness_state);

                auto const is_plt = ast_function.get_is_plt();

                call_targets.emplace_back(function, ast_function.get_is_at(),
                                          is_plt, parameters);
            }
        }

        liveness_config.block_states.clear();
        // reaching_config.block_states.clear();

        call_targets_vector.push_back(call_targets);
    }

    return call_targets_vector;
}
#endif