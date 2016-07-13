#ifndef __CA_DECODER_H
#define __CA_DECODER_H

#include "PatchCommon.h"

#include "dr_api.h"
#include "dr_defines.h"
#include "ca_defines.h"

#include "register_states.h"

#include "logging.h"

enum RegisterState
{
    REGISTER_UNTOUCHED = 0x0,
    REGISTER_READ = 0x1,
    REGISTER_WRITE = 0x2,
    REGISTER_READ_WRITE = 0x3,
};

constexpr std::size_t min_register() { return DR_REG_RAX; }
constexpr std::size_t max_register() { return DR_REG_R15; }

template <typename register_state_t>
using register_states_t = __register_states_t<min_register(), max_register(), register_state_t>;

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
    bool is_constant_write();
    bool needs_depie();

    template <std::size_t min_reg = min_register(), std::size_t max_reg = max_register()>
    __register_states_t<min_reg, max_reg, RegisterState> get_register_state()
    {
        __register_states_t<min_reg, max_reg, RegisterState> register_state;
        for (int reg = min_reg; reg <= max_reg; ++reg)
            register_state[reg] = __get_register_state(reg);

        return register_state;
    }

    std::pair<size_t, size_t> get_register_range();

  private:
    RegisterState __get_register_state(std::size_t reg);

    instr_t instr;
    uint8_t raw_byte[MAX_RAW_INSN_SIZE];
};

#endif /*__CA_DECODER_H*/
