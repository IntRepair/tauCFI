#pragma once

#include "ca_defines.h"
#include "PatchCommon.h"

#include <unordered_map>

enum RegisterState
{
    REGISTER_UNTOUCHED = 0x0,
    REGISTER_READ = 0x1,
    REGISTER_WRITE = 0x2,
    REGISTER_READ_WRITE = 0x3,
};

static std::string RegisterStateToString(RegisterState state)
{
    switch (state)
    {
    case REGISTER_UNTOUCHED:
        return "C";
    case REGISTER_READ:
        return "R";
    case REGISTER_WRITE:
        return "W";
    case REGISTER_READ_WRITE:
        return "RW";
    default:
        return "UNKNOWN";
    }
}

using RegisterStates = std::unordered_map<int, RegisterState>;

using namespace Dyninst::InstructionAPI;

class CADecoder
{
public:
    virtual ~CADecoder() {};
    virtual void decode(uint64_t, Instruction::Ptr) = 0;
    virtual uint64_t get_src(int index) = 0;
    virtual uint64_t get_src_abs_addr() = 0;
    virtual bool is_indirect_call() = 0;
    virtual bool is_call() = 0;
    virtual bool is_return() = 0;
    virtual RegisterStates get_register_state() = 0;
    virtual std::pair<size_t, size_t> get_register_range() = 0;
};

template <typename states_t> static std::string RegisterStatesToString(CADecoder *decoder, states_t register_states)
{
    std::string state_string = "[";
    auto range = decoder->get_register_range();

    for (auto reg = range.first; reg <= range.second; ++reg)
    {
        auto reg_state = register_states[reg];
        state_string += RegisterStateToString(reg_state) + ((reg == range.second) ? "]" : ", ");
    }

    return state_string;
}
