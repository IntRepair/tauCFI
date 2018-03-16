#ifndef __SYSTEMV_ABI_H
#define __SYSTEMV_ABI_H

#include "liveness/liveness_analysis.h"
#include "reaching/reaching_analysis.h"

#include <array>

namespace system_v
{

inline bool is_parameter_register(std::size_t reg_index)
{
    switch (reg_index)
    {
    case DR_REG_R9:
    case DR_REG_R8:
    case DR_REG_RCX:
    case DR_REG_RDX:
    case DR_REG_RSI:
    case DR_REG_RDI:
        return true;
    default:
        return false;
    }
}

inline bool is_preserved_register(std::size_t reg_index)
{
    switch (reg_index)
    {
    case DR_REG_RBX:
    case DR_REG_RSP:
    case DR_REG_RBP:
    case DR_REG_R12:
    case DR_REG_R13:
    case DR_REG_R14:
    case DR_REG_R15:
        return true;
    default:
        return false;
    }
}

inline bool is_return_register(std::size_t reg_index)
{
    switch (reg_index)
    {
    case DR_REG_RAX:
    //case DR_REG_RDX:
        return true;
    default:
        return false;
    }
}

inline register_states_t<reaching::state_t> get_maximum_reaching_state()
{
    register_states_t<reaching::state_t> state;

    state[DR_REG_R9] = reaching::REGISTER_SET_FULL;
    state[DR_REG_R8] = reaching::REGISTER_SET_FULL;
    state[DR_REG_RCX] = reaching::REGISTER_SET_FULL;
    state[DR_REG_RDX] = reaching::REGISTER_SET_FULL;
    state[DR_REG_RSI] = reaching::REGISTER_SET_FULL;
    state[DR_REG_RDI] = reaching::REGISTER_SET_FULL;

    return state;
}

inline int get_parameter_register_index(std::size_t reg_index)
{
    switch (reg_index)
    {
    case DR_REG_R9:
        return 5;
    case DR_REG_R8:
        return 4;
    case DR_REG_RCX:
        return 3;
    case DR_REG_RDX:
        return 2;
    case DR_REG_RSI:
        return 1;
    case DR_REG_RDI:
        return 0;
    default:
        return -1;
    }
}

inline std::size_t get_parameter_register_from_index(int param_index)
{
    switch (param_index)
    {
    case 5:
        return DR_REG_R9;
    case 4:
        return DR_REG_R8;
    case 3:
        return DR_REG_RCX;
    case 2:
        return DR_REG_RDX;
    case 1:
        return DR_REG_RSI;
    case 0:
        return DR_REG_RDI;
    default:
        return -1;
    }
}

namespace
{

inline int count_args_calltarget(register_states_t<liveness::state_t> state)
{
    if (liveness::is_read_before_write(state[DR_REG_R9]))
        return 6;

    if (liveness::is_read_before_write(state[DR_REG_R8]))
        return 5;

    if (liveness::is_read_before_write(state[DR_REG_RCX]))
        return 4;

    if (liveness::is_read_before_write(state[DR_REG_RDX]))
        return 3;

    if (liveness::is_read_before_write(state[DR_REG_RSI]))
        return 2;

    if (liveness::is_read_before_write(state[DR_REG_RDI]))
        return 1;

    return 0;
}

inline int count_args_callsite(register_states_t<reaching::state_t> state)
{
    if (reaching::is_set(state[DR_REG_R9]) && !reaching::is_trashed(state[DR_REG_R9]))
        return 6;

    if (reaching::is_set(state[DR_REG_R8]) && !reaching::is_trashed(state[DR_REG_R8]))
        return 5;

    if (reaching::is_set(state[DR_REG_RCX]) && !reaching::is_trashed(state[DR_REG_RCX]))
        return 4;

    if (reaching::is_set(state[DR_REG_RDX]) && !reaching::is_trashed(state[DR_REG_RDX]))
        return 3;

    if (reaching::is_set(state[DR_REG_RSI]) && !reaching::is_trashed(state[DR_REG_RSI]))
        return 2;

    if (reaching::is_set(state[DR_REG_RDI]) && !reaching::is_trashed(state[DR_REG_RDI]))
        return 1;

    return 0;
}
};

namespace callsite
{
inline std::array<char, 7>
generate_paramlist(BPatch_function *function, uint64_t address,
                   register_states_t<reaching::state_t> reaching_state,
                   register_states_t<liveness::state_t> liveness_state)
{
    std::array<char, 7> parameters;

    auto const void_ty = [](liveness::state_t state) {
        if (!liveness::is_write_before_read(state))
        {
            if (liveness::is_read_before_write_64(state))
                return '4';
            if (liveness::is_read_before_write_32(state))
                return '3';
            if (liveness::is_read_before_write_16(state))
                return '2';
            if (liveness::is_read_before_write_8(state))
                return '1';
        }
        return '0';
    }(liveness_state[DR_REG_RAX]);

    std::fill(parameters.begin(), parameters.end(), '0');

    static const int param_register_list[] = {DR_REG_RDI, DR_REG_RSI, DR_REG_RDX,
                                              DR_REG_RCX, DR_REG_R8,  DR_REG_R9};

    int args_count = 0;
    for (int index = 0; index < 6; ++index)
    {
        auto const param_ty = [](reaching::state_t state) {
            if (!reaching::is_trashed(state))
            {
                if (reaching::is_set_64(state))
                    return '4';
                if (reaching::is_set_32(state))
                    return '3';
                if (reaching::is_set_16(state))
                    return '2';
                if (reaching::is_set_8(state))
                    return '1';
            }
            return '0';
        }(reaching_state[param_register_list[index]]);

        if (param_ty != '0')
            args_count = index + 1;
        parameters[index] = param_ty;
    }

    // sanity
    for (int index = args_count - 2; index >= 0; --index)
    {
        if (parameters[index] == '0')
            parameters[index] = '4';
    }

    parameters[6] = void_ty;

    LOG_INFO(LOG_FILTER_CALL_SITE, "<CS> Function %s: Instruction %lx provides atmost %d "
                                   "args and %s has return type_index:%c",
             function->getName().c_str(), address, args_count,
             (void_ty != '0' ? "" : "possibly"), void_ty);
    return parameters;
}

}; /* namespace callsite */

namespace calltarget
{
inline std::array<char, 7>
generate_paramlist(BPatch_function *function, uint64_t start,
                   register_states_t<reaching::state_t> reaching_state,
                   register_states_t<liveness::state_t> liveness_state)
{
    std::array<char, 7> parameters;

    auto const void_ty = [](reaching::state_t state) {
        if (!reaching::is_trashed(state))
        {
            if (reaching::is_set_64(state))
                return '4';
            if (reaching::is_set_32(state))
                return '3';
            if (reaching::is_set_16(state))
                return '2';
            if (reaching::is_set_8(state))
                return '1';
        }
        return '0';
    }(reaching_state[DR_REG_RAX]);

    static const int param_register_list[] = {DR_REG_RDI, DR_REG_RSI, DR_REG_RDX,
                                              DR_REG_RCX, DR_REG_R8,  DR_REG_R9};

    int args_count = 0;
    for (int index = 0; index < 6; ++index)
    {
        auto const param_ty = [](liveness::state_t state) {
            if (liveness::is_read_before_write_64(state))
                return '4';
            if (liveness::is_read_before_write_32(state))
                return '3';
            if (liveness::is_read_before_write_16(state))
                return '2';
            if (liveness::is_read_before_write_8(state))
                return '1';
            return '0';
        }(liveness_state[param_register_list[index]]);

        if (param_ty != '0')
            args_count = index + 1;
        parameters[index] = param_ty;
    }

    parameters[6] = void_ty;

    LOG_INFO(LOG_FILTER_CALL_TARGET, "<CT> Function %s[%lx] requires atleast %d args and "
                                     "%s has the return type_index:%c",
             function->getName().c_str(), start, args_count,
             (void_ty == '0' ? "" : "<possibly>"), void_ty);
    return parameters;
}
}; /* namespace calltarget */
}; /* namespace system_v */

#endif /* __SYSTEMV_ABI_H */
