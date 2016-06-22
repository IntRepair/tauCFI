#pragma once

#include "ca_defines.h"
#include "PatchCommon.h"

enum RegisterState {
    REGISTER_UNTOUCHED,
    REGISTER_READ_BEFORE_WRITE,
    REGISTER_WRITE_BEFORE_READ,
};

static std::string RegisterStateToString(RegisterState state)
{
    switch(state)
    {
        case REGISTER_UNTOUCHED:
            return "C";
        case REGISTER_READ_BEFORE_WRITE:
            return "R";
        case REGISTER_WRITE_BEFORE_READ:
            return "W";
        default:
            return "UNKNOWN";
    }
}

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
    virtual std::vector<std::pair<int, RegisterState>> get_register_state() = 0;
    virtual std::pair<size_t, size_t> get_register_range() = 0;
};
