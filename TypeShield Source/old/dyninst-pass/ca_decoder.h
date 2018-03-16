#ifndef __CA_DECODER_H
#define __CA_DECODER_H

#include "PatchCommon.h"

#include <dr_api.h>
#include <dr_defines.h>

#include "ca_defines.h"
#include "logging.h"

#include "register_states.h"

#include <boost/optional.hpp>

enum RegisterState
{
    REGISTER_UNTOUCHED = 0x0,
    REGISTER_READ = 0x1,
    REGISTER_WRITE = 0x2,
    REGISTER_READ_WRITE = 0x3,
};

enum RegisterStateEx : uint8_t
{
    REGISTER_EX_UNTOUCHED = 0x0,
    REGISTER_EX_READ_8 = 0x1,
    REGISTER_EX_READ_16 = 0x2,
    REGISTER_EX_READ_32 = 0x4,
    REGISTER_EX_READ_64 = 0x8,
    REGISTER_EX_READ = 0xF,
    REGISTER_EX_WRITE_8 = 0x10,
    REGISTER_EX_WRITE_16 = 0x20,
    REGISTER_EX_WRITE_32 = 0x40,
    REGISTER_EX_WRITE_64 = 0x80,
    REGISTER_EX_WRITE = 0xF0,
};

inline RegisterStateEx operator|(RegisterStateEx a, RegisterStateEx b)
{
    return static_cast<RegisterStateEx>(static_cast<uint8_t>(a) |
                                        static_cast<uint8_t>(b));
}

inline RegisterStateEx operator&(RegisterStateEx a, RegisterStateEx b)
{
    return static_cast<RegisterStateEx>(static_cast<uint8_t>(a) &
                                        static_cast<uint8_t>(b));
}

inline RegisterStateEx &operator|=(RegisterStateEx &a, RegisterStateEx b)
{
    return a = a | b;
}

constexpr std::size_t min_register() { return DR_REG_RAX; }
constexpr std::size_t max_register() { return DR_REG_R15; }

template <typename register_state_t>
using register_states_t =
    __register_states_t<min_register(), max_register(), register_state_t>;

using namespace Dyninst::InstructionAPI;

class CADecoder
{
  public:
    CADecoder();
    ~CADecoder();
    bool decode(uint64_t, Instruction::Ptr);

    uint64_t get_address() const { return address; }

    uint64_t get_src(int index);
    std::vector<uint64_t> get_src_abs_addr();

    bool is_indirect_call();
    bool is_possible_direct_tailcall();
    bool is_possible_indirect_tailcall();
    bool is_call();
    bool is_return();
    bool is_constant_write();
    bool ignore_direct_reads();
    ptr_int_t get_constant_write();
    bool is_nop();

    bool is_reg_source(int index, int reg);
    boost::optional<int> get_reg_source(int index);
    boost::optional<int> get_par_reg_source(int index);
    boost::optional<int> get_reg_target(int index);
    boost::optional<int> get_par_reg_target(int index);
    
    bool is_target_stackpointer_disp(int index);
    bool is_target_register_disp(int index);
    boost::optional<int> get_reg_disp_target_register(int index);
    bool is_move_to(int reg);
    uint64_t get_target_disp(int index);
    uint64_t get_address_store_address();

    __register_states_t<min_register(), max_register(), RegisterStateEx>
    get_register_state_ex();

    template <std::size_t min_reg = min_register(), std::size_t max_reg = max_register()>
    __register_states_t<min_reg, max_reg, RegisterState> get_register_state()
    {
        __register_states_t<min_reg, max_reg, RegisterState> register_state;
        auto reg_state_ex = get_register_state_ex();
        for (int reg = min_reg; reg <= max_reg; ++reg)
        {
            register_state[reg] = __get_register_state(reg);
            if (((register_state[reg] & REGISTER_READ) == REGISTER_READ) !=
                ((reg_state_ex[reg] & REGISTER_EX_READ) > 0))
            {
                LOG_ERROR(LOG_FILTER_TYPE_ANALYSIS, "READ DISCREPANCY @ %lx with %lx %s",
                          get_address(),
                          static_cast<uint64_t>(reg_state_ex[reg] & REGISTER_EX_READ),
                          get_register_name(reg));
            }

            if (((register_state[reg] & REGISTER_WRITE) == REGISTER_WRITE) !=
                ((reg_state_ex[reg] & REGISTER_EX_WRITE) > 0))
            {
                LOG_ERROR(LOG_FILTER_TYPE_ANALYSIS, "WRITE DISCREPANCY @ %lx with %lx %s",
                          get_address(),
                          static_cast<uint64_t>(reg_state_ex[reg] & REGISTER_EX_WRITE),
                          get_register_name(reg));
            }
        }

        return register_state;
    }

    std::pair<size_t, size_t> get_register_range();

  private:
    RegisterState __get_register_state(std::size_t reg);

    uint64_t address;
    bool instr_initialized;
    instr_t instr;
    uint8_t raw_byte[MAX_RAW_INSN_SIZE];
};

#include "to_string.h"

template <> inline std::string to_string(RegisterState const &state)
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

template <> inline std::string to_string(RegisterStateEx const &state)
{
    if (state == REGISTER_EX_UNTOUCHED)
        return "C";

    char result[10];
    result[0] = 'R';
    result[1] = ((state & REGISTER_EX_READ_64) == REGISTER_EX_READ_64) ? 'x' : 'o';
    result[2] = ((state & REGISTER_EX_READ_32) == REGISTER_EX_READ_32) ? 'x' : 'o';
    result[3] = ((state & REGISTER_EX_READ_16) == REGISTER_EX_READ_16) ? 'x' : 'o';
    result[4] = ((state & REGISTER_EX_READ_8) == REGISTER_EX_READ_8) ? 'x' : 'o';

    result[5] = 'W';
    result[6] = ((state & REGISTER_EX_WRITE_64) == REGISTER_EX_WRITE_64) ? 'x' : 'o';
    result[7] = ((state & REGISTER_EX_WRITE_32) == REGISTER_EX_WRITE_32) ? 'x' : 'o';
    result[8] = ((state & REGISTER_EX_WRITE_16) == REGISTER_EX_WRITE_16) ? 'x' : 'o';
    result[9] = ((state & REGISTER_EX_WRITE_8) == REGISTER_EX_WRITE_8) ? 'x' : 'o';

    return std::string(result, 10);
}

#endif /*__CA_DECODER_H*/
