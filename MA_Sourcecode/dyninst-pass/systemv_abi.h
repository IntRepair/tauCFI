#ifndef __SYSTEMV_ABI_H
#define __SYSTEMV_ABI_H

#include "liveness_analysis.h"
#include "reaching_analysis.h"

static int count_args_calltarget(register_states_t<liveness::state_t> state)
{
    if (state[DR_REG_R9] == liveness::REGISTER_READ_BEFORE_WRITE)
        return 6;

    if (state[DR_REG_R8] == liveness::REGISTER_READ_BEFORE_WRITE)
        return 5;

    if (state[DR_REG_RCX] == liveness::REGISTER_READ_BEFORE_WRITE)
        return 4;

    if (state[DR_REG_RDX] == liveness::REGISTER_READ_BEFORE_WRITE)
        return 3;

    if (state[DR_REG_RSI] == liveness::REGISTER_READ_BEFORE_WRITE)
        return 2;

    if (state[DR_REG_RDI] == liveness::REGISTER_READ_BEFORE_WRITE)
        return 1;

    return 0;
}

static int count_args_callsite(register_states_t<reaching::state_t> state)
{
    if (state[DR_REG_R9] == reaching::REGISTER_SET)
        return 6;

    if (state[DR_REG_R8] == reaching::REGISTER_SET)
        return 5;

    if (state[DR_REG_RCX] == reaching::REGISTER_SET)
        return 4;

    if (state[DR_REG_RDX] == reaching::REGISTER_SET)
        return 3;

    if (state[DR_REG_RSI] == reaching::REGISTER_SET)
        return 2;

    if (state[DR_REG_RDI] == reaching::REGISTER_SET)
        return 1;

    return 0;
}

static int get_parameter_register_index(std::size_t reg_index)
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

static std::size_t get_parameter_register_from_index(int param_index)
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

static bool is_parameter_register(std::size_t reg_index)
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

static bool is_preserved_register(std::size_t reg_index)
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

static bool is_return_register(std::size_t reg_index)
{
    switch (reg_index)
    {
    case DR_REG_RAX:
    case DR_REG_RDX:
        return true;
    default:
        return false;
    }
}

static bool is_void_calltarget(register_states_t<reaching::state_t> state)
{
    return (state[DR_REG_RAX] == reaching::REGISTER_TRASHED);
}

static bool is_nonvoid_callsite(register_states_t<liveness::state_t> state)
{
    return (state[DR_REG_RAX] == liveness::REGISTER_READ_BEFORE_WRITE);
}

#endif /* __SYSTEMV_ABI_H */
