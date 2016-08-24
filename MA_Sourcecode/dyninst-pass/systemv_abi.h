#ifndef __SYSTEMV_ABI_H
#define __SYSTEMV_ABI_H

#include "liveness_analysis.h"
#include "reaching_analysis.h"

static int count_args_calltarget(register_states_t<liveness_state_t> state)
{
    if (state[DR_REG_R9] == REGISTER_READ_BEFORE_WRITE)
        return 6;

    if (state[DR_REG_R8] == REGISTER_READ_BEFORE_WRITE)
        return 5;

    if (state[DR_REG_RCX] == REGISTER_READ_BEFORE_WRITE)
        return 4;

    if (state[DR_REG_RDX] == REGISTER_READ_BEFORE_WRITE)
        return 3;

    if (state[DR_REG_RSI] == REGISTER_READ_BEFORE_WRITE)
        return 2;

    if (state[DR_REG_RDI] == REGISTER_READ_BEFORE_WRITE)
        return 1;

    return 0;
}

static int count_args_callsite(register_states_t<reaching_state_t> state)
{
    if (state[DR_REG_R9] == REGISTER_SET)
        return 6;

    if (state[DR_REG_R8] == REGISTER_SET)
        return 5;

    if (state[DR_REG_RCX] == REGISTER_SET)
        return 4;

    if (state[DR_REG_RDX] == REGISTER_SET)
        return 3;

    if (state[DR_REG_RSI] == REGISTER_SET)
        return 2;

    if (state[DR_REG_RDI] == REGISTER_SET)
        return 1;

    return 0;
}

static bool is_void_calltarget(register_states_t<reaching_state_t> state)
{
    return (state[DR_REG_RAX] != REGISTER_SET);
}

static bool is_nonvoid_callsite(register_states_t<liveness_state_t> state)
{
    return (state[DR_REG_RAX] == REGISTER_READ_BEFORE_WRITE);
}

#endif /* __SYSTEMV_ABI_H */
