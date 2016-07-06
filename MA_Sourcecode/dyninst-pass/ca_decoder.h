#pragma once

#include "dr_defines.h"
#include "ca_defines.h"

#include "PatchCommon.h"

#include <array>
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
/*
template <std::size_t min, std::size_t max, typename element_t>
struct __register_state_t
{
public:
    enum {
        min_size = min,
        max_size = max,
    };

    inline element_t& operator[](std::size_t index)
    {
        return _storage[index - min];
    }

    inline element_t const& operator[](std::size_t index) const
    {
        return _storage[index - min];
    }

private:
    std::array<element_t, 1 + max - min> _storage;
};
*/

using RegisterStates = std::unordered_map<int, RegisterState>;

using namespace Dyninst::InstructionAPI;

class CADecoder
{
public:
    CADecoder();
    ~CADecoder();
    void decode(uint64_t, Instruction::Ptr);
    uint64_t get_src(int index);
    uint64_t get_src_abs_addr();
    bool is_indirect_call();
    bool is_call();
    bool is_return();
	bool needs_depie();

    static constexpr std::size_t min_register();
    static constexpr std::size_t max_register();
    //using RegisterStates = __register_state_t<min_register(), max_register(), RegisterState>;
    RegisterStates get_register_state();

    std::pair<size_t, size_t> get_register_range();
private:
	instr_t instr;
	uint8_t raw_byte[MAX_RAW_INSN_SIZE];
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
