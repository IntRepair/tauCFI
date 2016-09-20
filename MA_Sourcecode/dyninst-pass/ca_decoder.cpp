#include "ca_decoder.h"

#include <dr_api.h>
#include <dr_defines.h>

static void *drcontext = dr_standalone_init();

CADecoder::CADecoder() { set_x86_mode(drcontext, false); }

CADecoder::~CADecoder() {}

void CADecoder::decode(uint64_t addr, Instruction::Ptr iptr)
{
    auto nbytes = iptr->size();
    auto ins_addr = addr;
    for (size_t i = 0; i < nbytes; i++)
    {
        raw_byte[i] = iptr->rawByte(i);
    }
    raw_byte[nbytes] = '\0';
    instr_init(drcontext, &instr);
    decode_from_copy(drcontext, (byte *)raw_byte, (byte *)ins_addr, &instr);
#if 0
	char dis_buf[1024];
	size_t size_of_dis;
	size_of_dis = instr_disassemble_to_buffer(drcontext,&instr,dis_buf,1024);
	dis_buf[size_of_dis] = '\0';
	printf("Dissassemble %x -> %s\n",(unsigned int)ins_addr,dis_buf);
#endif
}

uint64_t CADecoder::get_src_abs_addr()
{
    opnd_t src_opnd;
    int src_idx;
    uint64_t abs_addr;
    int nr_src, i;

    if (instr_is_cti(&instr))
        return 0;

    if (instr_is_mov_constant(&instr, (ptr_int_t *)(&abs_addr)))
        return abs_addr;

    if (instr_get_opcode(&instr) == OP_lea)
    {
        nr_src = instr_num_srcs(&instr);
        for (i = 0; i < nr_src; i++)
        {
            src_opnd = instr_get_src(&instr, i);
            if (opnd_is_abs_addr(src_opnd))
            {
                return (uint64_t)opnd_get_addr(src_opnd);
            }
        }
    }

    // Now src should PC relative if there is address
    abs_addr = 0;
    if (needs_depie())
    {
        src_idx = instr_get_rel_addr_src_idx(&instr);
        if (src_idx != -1)
        {
            src_opnd = instr_get_src(&instr, (uint)src_idx);
            abs_addr = (uint64_t)opnd_get_disp(src_opnd);
        }
    }

    return abs_addr;
}

uint64_t CADecoder::get_src(int index)
{
    auto opnd = instr_get_src(&instr, index);

    if (opnd_is_pc(opnd))
        return reinterpret_cast<uint64_t>(opnd_get_pc(opnd));
    else if (opnd_is_abs_addr(opnd))
        return reinterpret_cast<uint64_t>(opnd_get_addr(opnd));
#if 0
    else if (instr_has_rel_addr_reference(&instr))
    {
        unsigned char *ptr;
        if (instr_get_rel_addr_target(&instr, &ptr))
            return reinterpret_cast<uint64_t>(ptr);
    }
#endif
    return 0;
}

bool CADecoder::is_indirect_call()
{
    return instr_is_call_indirect(&instr); // instr_is_mbr(&instr) && !is_return();
}

bool CADecoder::is_call() { return instr_is_call_direct(&instr); }

bool CADecoder::is_return() { return instr_is_return(&instr); }

bool CADecoder::is_constant_write()
{
    ptr_int_t value;
    return instr_is_mov_constant(&instr, &value);
}

bool CADecoder::needs_depie() { return instr_has_rel_addr_reference(&instr); }

bool CADecoder::is_reg_source(int index, int reg)
{
    auto opnd = instr_get_src(&instr, index);
    if (opnd_is_reg(opnd))
    {
        return opnd_get_reg(opnd) == reg;
    }
    return false;
}

bool CADecoder::is_target_stackpointer_disp(int index)
{
    if (index < instr_num_dsts(&instr))
    {
        auto opnd = instr_get_dst(&instr, index);
        if (opnd_is_base_disp(opnd))
            return opnd_get_base(opnd) == DR_REG_XSP;
    }
    return false;
}

uint64_t CADecoder::get_target_disp(int index)
{
    if (index < instr_num_dsts(&instr))
    {
        auto opnd = instr_get_dst(&instr, index);
        if (opnd_is_base_disp(opnd))
            return opnd_get_disp(opnd);
    }
    return -1;
}

uint64_t CADecoder::get_address_store_address()
{
    if (instr_has_rel_addr_reference(&instr))
    {
        unsigned char *ptr;
        if (instr_get_rel_addr_target(&instr, &ptr))
            return reinterpret_cast<uint64_t>(ptr);
    }
    return 0;
}

void CADecoder::operand_information()
{
    auto src_count = instr_num_srcs(&instr);
    for (auto pos = 0; pos < src_count; ++pos)
    {
        auto opnd = instr_get_src(&instr, pos);
        auto regs = opnd_num_regs_used(opnd);
        for (auto reg_index = 0; reg_index < regs; ++reg_index)
        {
            auto reg = opnd_get_reg_used(opnd, reg_index);
            LOG_INFO(LOG_FILTER_TYPE_ANALYSIS, "Register %s READ [size: %d]",
                     get_register_name(reg), opnd_size_in_bits(reg_get_size(reg)));
        }
    }

    auto dst_count = instr_num_dsts(&instr);
    for (auto pos = 0; pos < dst_count; ++pos)
    {
        auto opnd = instr_get_dst(&instr, pos);
        if (!opnd_is_reg(opnd))
        {
            auto regs = opnd_num_regs_used(opnd);
            for (auto reg_index = 0; reg_index < regs; ++reg_index)
            {
                auto reg = opnd_get_reg_used(opnd, reg_index);
                LOG_INFO(LOG_FILTER_TYPE_ANALYSIS, "Register %s READ [size: %d]",
                         get_register_name(reg), opnd_size_in_bits(reg_get_size(reg)));
            }
        }
        else
        {
            auto reg = opnd_get_reg(opnd);
            LOG_INFO(LOG_FILTER_TYPE_ANALYSIS, "Register %s WRITTEN [size: %d]",
                     get_register_name(reg), opnd_size_in_bits(reg_get_size(reg)));
        }
    }

    for (int reg = DR_REG_RAX; reg <= DR_REG_DIL; ++reg)
    {
        RegisterState state = REGISTER_UNTOUCHED;
        if (instr_writes_to_exact_reg(&instr, reg, DR_QUERY_DEFAULT))
            state = static_cast<RegisterState>(state | REGISTER_WRITE);
        if (state != 0)
            LOG_INFO(LOG_FILTER_TYPE_ANALYSIS, "Register %s %s", get_register_name(reg),
                     to_string(state).c_str());
    }
}

RegisterState CADecoder::__get_register_state(std::size_t reg)
{
    RegisterState state = REGISTER_UNTOUCHED;
    if (instr_writes_to_reg(&instr, reg, DR_QUERY_DEFAULT))
        state = static_cast<RegisterState>(state | REGISTER_WRITE);
    if (instr_reads_from_reg(&instr, reg, DR_QUERY_DEFAULT))
        state = static_cast<RegisterState>(state | REGISTER_READ);
    return state;
}

std::pair<size_t, size_t> CADecoder::get_register_range()
{
    return std::make_pair(DR_REG_RAX, DR_REG_R15);
}
