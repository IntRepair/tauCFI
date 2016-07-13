#include "ca_decoder.h"

#include "dr_api.h"
#include "dr_defines.h"

template <> std::string to_string(RegisterState state)
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

static void* drcontext = dr_standalone_init();

CADecoder::CADecoder()
{
	set_x86_mode(drcontext,false);
}

CADecoder::~CADecoder()
{
}

void CADecoder::decode(uint64_t addr, Instruction::Ptr iptr)
{
	auto nbytes = iptr->size();
	auto ins_addr = addr;
	for (size_t i =0; i<nbytes; i++){
		raw_byte[i] = iptr->rawByte(i);
	}
	raw_byte[nbytes] = '\0';
	instr_init(drcontext,&instr);
	decode_from_copy(drcontext, (byte*)raw_byte, (byte*)ins_addr, &instr);
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
	int nr_src,i;
	
	if (instr_is_cti(&instr))
		return 0;

	if (instr_is_mov_constant(&instr,(ptr_int_t*)(&abs_addr)))
		return abs_addr;

	if (instr_get_opcode(&instr) == OP_lea){
		nr_src = instr_num_srcs(&instr);
		for (i=0;i<nr_src;i++){
			src_opnd = instr_get_src(&instr,i);
			if (opnd_is_abs_addr(src_opnd)){
				return (uint64_t)opnd_get_addr(src_opnd);
			}
		}
	}
	
// Now src should PC relative if there is address 
	abs_addr = 0;
	if (needs_depie()){
		src_idx = instr_get_rel_addr_src_idx(&instr);
		if (src_idx!=-1){
			src_opnd = instr_get_src(&instr,(uint)src_idx);
			abs_addr = (uint64_t) opnd_get_disp(src_opnd);
		}
	}
	
	return abs_addr;
}

uint64_t CADecoder::get_src(int index)
{
    auto opnd = instr_get_src(&instr, 0);

    if(opnd_is_pc(opnd))
        return reinterpret_cast<uint64_t>(opnd_get_pc(opnd));
    else if (opnd_is_abs_addr(opnd))
        return reinterpret_cast<uint64_t>(opnd_get_addr(opnd));

    return 0;
}

bool CADecoder::is_indirect_call()
{
    return instr_is_call_indirect(&instr);
}

bool CADecoder::is_call()
{
    return instr_is_call_direct(&instr);
}

bool CADecoder::is_return()
{
    return instr_is_return(&instr);
}

bool CADecoder::is_constant_write()
{
    ptr_int_t value;
    return instr_is_mov_constant(&instr, &value);
}

bool CADecoder::needs_depie()
{
    return instr_has_rel_addr_reference(&instr);
}

RegisterState CADecoder::__get_register_state(std::size_t reg)
{
    RegisterState state = REGISTER_UNTOUCHED;
    if (instr_writes_to_reg(&instr, reg))
        state = static_cast<RegisterState>(state | REGISTER_WRITE);
    if (instr_reads_from_reg(&instr, reg))
        state = static_cast<RegisterState>(state | REGISTER_READ);
    return state;
}

std::pair<size_t, size_t> CADecoder::get_register_range() { return std::make_pair(DR_REG_RAX, DR_REG_R15); }
