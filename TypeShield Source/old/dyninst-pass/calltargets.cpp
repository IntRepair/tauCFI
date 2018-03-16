#include "calltargets.h"

#include "ca_decoder.h"
#include "instrumentation.h"
#include "liveness/liveness_analysis.h"
#include "logging.h"
#include "reaching/reaching_analysis.h"
#include "systemv_abi.h"
#include "utils.h"

#include <BPatch_object.h>

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
