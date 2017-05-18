#ifndef __REACHING_IMPL_H
#define __REACHING_IMPL_H

#include "reaching_analysis.h"

#include "systemv_abi.h"
#include "register_states.h"

namespace reaching
{
namespace impl
{
inline bool has_undefined_param_regs(RegisterStates &reg_state)
{
    bool undefined_regs = false;
    for (auto i = static_cast<size_t>(RegisterStates::min_index);
         i <= static_cast<size_t>(RegisterStates::max_index); ++i)
    {
        if (system_v::is_parameter_register(i))
            undefined_regs |= (reg_state[i] == REGISTER_UNKNOWN);
    }
    return undefined_regs;
}

inline bool has_undefined_return_regs(RegisterStates &reg_state)
{
    bool undefined_regs = false;
    for (auto i = static_cast<size_t>(RegisterStates::min_index);
         i <= static_cast<size_t>(RegisterStates::max_index); ++i)
    {
        if (system_v::is_return_register(i))
            undefined_regs |= (reg_state[i] == REGISTER_UNKNOWN);
    }
    return undefined_regs;
}

template <typename merge_fun_t>
inline RegisterStates merge_horizontal(std::vector<RegisterStates> states,
                                       merge_fun_t reg_merge_fun)
{
    RegisterStates target_state;
    LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t\tMerging states horizontal:");
    for (auto const &state : states)
        LOG_TRACE(LOG_FILTER_REACHING_ANALYSIS, "\t\t\t%s", to_string(state).c_str());
    if (states.size() == 1)
    {
        target_state = states[0];
    }
    else if (states.size() > 1)
    {
        target_state = std::accumulate(++(states.begin()), states.end(), states.front(),
                                       reg_merge_fun);
    }
    return target_state;
}
};
};

#endif /* __REACHING_IMPL_H */
