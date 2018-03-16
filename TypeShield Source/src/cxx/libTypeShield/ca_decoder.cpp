#include "ca_decoder.h"

#include <dr_api.h>
#include <dr_defines.h>

static void *drcontext = dr_standalone_init();

CADecoder::CADecoder() : instr_initialized(false) {
  set_x86_mode(drcontext, false);
}

CADecoder::~CADecoder() {}

bool CADecoder::decode(uint64_t addr, Instruction::Ptr iptr) {
  auto nbytes = iptr->size();
  auto ins_addr = addr;
  address = addr;
  for (size_t i = 0; i < nbytes; i++) {
    raw_byte[i] = iptr->rawByte(i);
  }
  raw_byte[nbytes] = '\0';
  if (instr_initialized)
    instr_reset(drcontext, &instr);
  else {
    instr_init(drcontext, &instr);
    instr_initialized = true;
  }
  return NULL != decode_from_copy(drcontext, (byte *)raw_byte, (byte *)ins_addr,
                                  &instr);
#if 0
	char dis_buf[1024];
	size_t size_of_dis;
	size_of_dis = instr_disassemble_to_buffer(drcontext,&instr,dis_buf,1024);
	dis_buf[size_of_dis] = '\0';
	printf("Dissassemble %x -> %s\n",(unsigned int)ins_addr,dis_buf);
#endif
}

std::vector<uint64_t> CADecoder::get_src_abs_addr() {
  std::vector<uint64_t> addresses;

  if (instr_is_cti(&instr) && !is_indirect_call())
    return addresses;

  uint64_t abs_addr;
  if (instr_is_mov_constant(&instr, (ptr_int_t *)(&abs_addr))) {
    addresses.push_back(abs_addr);
    return addresses;
  }

  if (instr_get_opcode(&instr) == OP_lea)
  {
      auto const nr_src = instr_num_srcs(&instr);
      for (int i = 0; i < nr_src; i++)
      {
          auto src_opnd = instr_get_src(&instr, i);
          if (opnd_is_abs_addr(src_opnd))
          {
              addresses.push_back(reinterpret_cast<uint64_t>(opnd_get_addr(src_opnd)));
          }
      }
  }

  if (instr_get_opcode(&instr) == OP_push_imm)
  {
      auto const nr_src = instr_num_srcs(&instr);
      for (int i = 0; i < nr_src; i++)
      {
          auto src_opnd = instr_get_src(&instr, i);
          if (opnd_is_immed_int(src_opnd))
          {
              addresses.push_back(static_cast<uint64_t>(opnd_get_immed_int(src_opnd)));
          } 
      }
  }


    // check for RIP based addresses displacement(rip)
    if (instr_has_rel_addr_reference(&instr))
    {
        if (instr_get_rel_addr_src_idx(&instr) != -1)
        {
            unsigned char *ptr;
            if (instr_get_rel_addr_target(&instr, &ptr))
                addresses.push_back(reinterpret_cast<uint64_t>(ptr));
        }
    }

  return addresses;
}

uint64_t CADecoder::get_src(int index) {
  auto const nr_src = instr_num_srcs(&instr);
  if (index < nr_src) {
    auto opnd = instr_get_src(&instr, index);

    if (opnd_is_pc(opnd))
      return reinterpret_cast<uint64_t>(opnd_get_pc(opnd));
    else if (opnd_is_abs_addr(opnd))
      return reinterpret_cast<uint64_t>(opnd_get_addr(opnd));
  }
  return 0;
}

bool CADecoder::is_indirect_call() { // return instr_is_call_indirect(&instr);
  if (instr_is_mbr(&instr)) {
    if (instr_is_call(&instr))
      return true;

    if (instr_is_return(&instr))
      return false;

    auto const nr_src = instr_num_srcs(&instr);
    if (0 < nr_src) {
      auto opnd = instr_get_src(&instr, 0);
      if (opnd_is_base_disp(opnd))
        if (opnd_get_base(opnd) == DR_REG_NULL)
          return false;
    }

    return true;
  }
  return false;
}

bool CADecoder::is_possible_direct_tailcall() { return instr_is_ubr(&instr); }

bool CADecoder::is_possible_indirect_tailcall() { return instr_is_ubr(&instr); }

bool CADecoder::is_call() { return instr_is_call_direct(&instr); }

bool CADecoder::is_return() { return instr_is_return(&instr); }

bool CADecoder::is_constant_write() {
  ptr_int_t value;
  return instr_is_mov_constant(&instr, &value);
}

bool CADecoder::ignore_direct_reads() {
  if (is_constant_write())
    return true;

  if (instr_get_opcode(&instr) == OP_sbb) {
    // LOG_TRACE(LOG_FILTER_CALL_TARGET, "\t SBB opcode %s addr %lx",
    //    decode_opcode_name(instr_get_opcode(&instr)), get_address());
    auto reg_source = get_reg_source(0);
    auto reg_target = get_reg_target(0);

    if (reg_source && reg_target) {
      // LOG_TRACE(LOG_FILTER_CALL_TARGET, "\t SBB opcode %s addr %lx %u %u",
      //    decode_opcode_name(instr_get_opcode(&instr)), get_address(),
      //    *reg_source, *reg_target);
      return *reg_source == *reg_target;
    }
  }

  if (instr_get_opcode(&instr) == OP_sub) {
    // LOG_TRACE(LOG_FILTER_CALL_TARGET, "\t SUB opcode %s addr %lx",
    //    decode_opcode_name(instr_get_opcode(&instr)), get_address());
    auto reg_source = get_reg_source(0);
    auto reg_target = get_reg_target(0);

    if (reg_source && reg_target) {
      // LOG_TRACE(LOG_FILTER_CALL_TARGET, "\t SUB opcode %s addr %lx %u %u",
      //    decode_opcode_name(instr_get_opcode(&instr)), get_address(),
      //    *reg_source, *reg_target);
      return *reg_source == *reg_target;
    }
  }

  return false;
}

ptr_int_t CADecoder::get_constant_write() {
  ptr_int_t value;

  if (instr_is_mov_constant(&instr, &value))
    return value;

  return 0;
}

bool CADecoder::is_nop() { return instr_is_nop(&instr); }

std::pair<bool, int> CADecoder::get_pop() {
  if (instr_get_opcode(&instr) == OP_pop) {
    return std::make_pair(true, *get_reg_target(0));
  }
  return std::make_pair(false, 0);
}

std::pair<bool, int> CADecoder::get_push() {
  if (instr_get_opcode(&instr) == OP_push) {
    return std::make_pair(true, *get_reg_source(0));
  }
  return std::make_pair(false, 0);
}

bool CADecoder::is_reg_source(int index, int reg) {
  auto opnd = instr_get_src(&instr, index);
  if (opnd_is_reg(opnd)) {
    return opnd_get_reg(opnd) == reg;
  }
  return false;
}

boost::optional<int> CADecoder::get_reg_source(int index) {
  boost::optional<int> result;

  auto opnd = instr_get_src(&instr, index);
  if (opnd_is_reg(opnd)) {
    result = opnd_get_reg(opnd);
  }

  return result;
}

static reg_id_t get_parent_register(reg_id_t reg) {
  return reg_resize_to_opsz(reg, OPSZ_8);
}

boost::optional<int> CADecoder::get_par_reg_source(int index) {
  auto reg = get_reg_source(index);
  if (reg) {
    reg = get_parent_register(*reg);
  }
  return reg;
}

boost::optional<int> CADecoder::get_reg_target(int index) {
  boost::optional<int> result;

  if (index < instr_num_dsts(&instr)) {
    auto opnd = instr_get_dst(&instr, index);
    if (opnd_is_reg(opnd)) {
      result = opnd_get_reg(opnd);
    }
  }

  return result;
}

boost::optional<int> CADecoder::get_par_reg_target(int index) {
  auto reg = get_reg_target(index);
  if (reg) {
    reg = get_parent_register(*reg);
  }
  return reg;
}

bool CADecoder::is_target_stackpointer_disp(int index) {
  if (index < instr_num_dsts(&instr)) {
    auto opnd = instr_get_dst(&instr, index);
    if (opnd_is_base_disp(opnd))
      return opnd_get_base(opnd) == DR_REG_XSP;
  }
  return false;
}

bool CADecoder::is_target_register_disp(int index) {
  if (index < instr_num_dsts(&instr)) {
    auto opnd = instr_get_dst(&instr, index);
    return opnd_is_base_disp(opnd);
  }
  return false;
}

boost::optional<int> CADecoder::get_reg_disp_target_register(int index) {
  boost::optional<int> result;

  if (index < instr_num_dsts(&instr)) {
    auto opnd = instr_get_dst(&instr, index);
    if (opnd_is_base_disp(opnd))
      result = opnd_get_base(opnd);
  }

  return result;
}

bool CADecoder::is_move_to(int reg) {
  if (instr_is_mov(&instr) && 0 < instr_num_dsts(&instr)) {
    auto opnd = instr_get_dst(&instr, 0);
    return opnd_get_reg(opnd) == reg;
  }
  return false;
}

uint64_t CADecoder::get_target_disp(int index) {
  if (index < instr_num_dsts(&instr)) {
    auto opnd = instr_get_dst(&instr, index);
    if (opnd_is_base_disp(opnd))
      return opnd_get_disp(opnd);
  }
  return -1;
}

uint64_t CADecoder::get_address_store_address() {
  if (instr_has_rel_addr_reference(&instr)) {
    unsigned char *ptr;
    if (instr_get_rel_addr_target(&instr, &ptr))
      return reinterpret_cast<uint64_t>(ptr);
  }
  return 0;
}

uint64_t CADecoder::get_potential_switch_table_address() {
  auto const nr_src = instr_num_srcs(&instr);
  if (0 < nr_src) {
    auto opnd = instr_get_src(&instr, 0);
    if (opnd_is_base_disp(opnd)) {
      if (opnd_get_scale(opnd) == 8 && opnd_get_base(opnd) == DR_REG_NULL && opnd_get_index(opnd) != DR_REG_NULL) {
        return opnd_get_disp(opnd);
      }
    }
  }
  return 0;
}

static RegisterStateEx calculate_read_change(reg_id_t reg) {
  // These registers are occupying the upper half of the 16bit register, thus we
  // set the
  // REGISTER_EX_READ_16 flag, however they do not touch the lowever 8 bits, we
  // do not
  // set the REGISTER_EX_READ_8 flag
  switch (reg) {
  case DR_REG_AH: /**< The "ah" register. */
  case DR_REG_CH: /**< The "ch" register. */
  case DR_REG_DH: /**< The "dh" register. */
  case DR_REG_BH: /**< The "bh" register. */
    return REGISTER_EX_READ_16;
  }

  RegisterStateEx state = REGISTER_EX_UNTOUCHED;
  switch (opnd_size_in_bits(reg_get_size(reg))) {
  case 64:
    state |= REGISTER_EX_READ_64;
  case 32:
    state |= REGISTER_EX_READ_32;
  case 16:
    state |= REGISTER_EX_READ_16;
  case 8:
    state |= REGISTER_EX_READ_8;
  }

  return state;
}

static RegisterStateEx calculate_write_change(reg_id_t reg) {
  // These registers are occupying the upper half of the 16bit register, thus we
  // set the
  // REGISTER_EX_WRITE_16 flag, however they do not touch the lowever 8 bits, we
  // do not
  // set the REGISTER_EX_WRITE_8 flag
  switch (reg) {
  case DR_REG_AH: /**< The "ah" register. */
  case DR_REG_CH: /**< The "ch" register. */
  case DR_REG_DH: /**< The "dh" register. */
  case DR_REG_BH: /**< The "bh" register. */
    return REGISTER_EX_WRITE_16;
  }

  RegisterStateEx state = REGISTER_EX_UNTOUCHED;
  switch (opnd_size_in_bits(reg_get_size(reg))) {
  case 64:
    state |= REGISTER_EX_WRITE_64;
  case 32:
    state |= REGISTER_EX_WRITE_32;
  case 16:
    state |= REGISTER_EX_WRITE_16;
  case 8:
    state |= REGISTER_EX_WRITE_8;
  }

  return state;
}

__register_states_t<min_register(), max_register(), RegisterStateEx>
CADecoder::get_register_state_ex() {
  __register_states_t<min_register(), max_register(), RegisterStateEx>
      reg_state;

  if (is_nop())
    return reg_state;

  if (instr_get_opcode(&instr) == OP_movsx ||
      instr_get_opcode(&instr) == OP_movzx) {
    // LOG_TRACE(LOG_FILTER_CALL_TARGET, "\t SBB opcode %s addr %lx",
    //    decode_opcode_name(instr_get_opcode(&instr)), get_address());
    auto reg_source = get_reg_source(0);
    auto reg_target = get_reg_target(0);

    if (reg_source && reg_target) {
      auto src_count = instr_num_srcs(&instr);
      for (auto pos = 0; pos < src_count; ++pos) {
        auto opnd = instr_get_src(&instr, pos);
        auto regs = opnd_num_regs_used(opnd);
        for (auto reg_index = 0; reg_index < regs; ++reg_index) {
          auto reg = opnd_get_reg_used(opnd, reg_index);
          if (reg_is_gpr(reg)) {
            if (get_parent_register(*reg_source) == get_parent_register(reg))
              reg_state[get_parent_register(*reg_target)] |=
                  calculate_write_change(reg);
            reg_state[get_parent_register(reg)] |= calculate_read_change(reg);
          }
        }
      }

      auto dst_count = instr_num_dsts(&instr);
      for (auto pos = 0; pos < dst_count; ++pos) {
        auto opnd = instr_get_dst(&instr, pos);
        if (!opnd_is_reg(opnd)) {
          auto regs = opnd_num_regs_used(opnd);
          for (auto reg_index = 0; reg_index < regs; ++reg_index) {
            auto reg = opnd_get_reg_used(opnd, reg_index);
            if (reg_is_gpr(reg))
              reg_state[get_parent_register(reg)] |= calculate_read_change(reg);
          }
        } else {
          auto reg = opnd_get_reg(opnd);
          if (reg_is_gpr(reg) &&
              get_parent_register(reg) != get_parent_register(*reg_target) &&
              instr_writes_to_reg(&instr, reg, DR_QUERY_INCLUDE_ALL)) {
            reg_state[get_parent_register(reg)] |= calculate_write_change(reg);
          }
        }
      }

      for (reg_id_t reg = DR_REG_RAX; reg <= DR_REG_DIL; ++reg) {
        if (instr_writes_to_exact_reg(&instr, reg, DR_QUERY_INCLUDE_ALL) &&
            get_parent_register(reg) != get_parent_register(*reg_target)) {
          if (reg_is_gpr(reg))
            reg_state[get_parent_register(reg)] |= calculate_write_change(reg);
        }
      }

      // LOG_INFO(LOG_FILTER_CALL_TARGET,
      //         "\t OP_MOVX opcode %s addr %lx  REGISTER_STATE %s",
      //         decode_opcode_name(instr_get_opcode(&instr)), get_address(),
      //         to_string(reg_state).c_str());
      return reg_state;
    }
  }

  auto src_count = instr_num_srcs(&instr);
#if 1
  if (!ignore_direct_reads())
#endif
  {
    for (auto pos = 0; pos < src_count; ++pos) {
      auto opnd = instr_get_src(&instr, pos);
      auto regs = opnd_num_regs_used(opnd);
      for (auto reg_index = 0; reg_index < regs; ++reg_index) {
        auto reg = opnd_get_reg_used(opnd, reg_index);
        if (reg_is_gpr(reg))
          reg_state[get_parent_register(reg)] |= calculate_read_change(reg);
      }
    }
  }

  auto dst_count = instr_num_dsts(&instr);
  for (auto pos = 0; pos < dst_count; ++pos) {
    auto opnd = instr_get_dst(&instr, pos);
    if (!opnd_is_reg(opnd)) {
      auto regs = opnd_num_regs_used(opnd);
      for (auto reg_index = 0; reg_index < regs; ++reg_index) {
        auto reg = opnd_get_reg_used(opnd, reg_index);
        if (reg_is_gpr(reg))
          reg_state[get_parent_register(reg)] |= calculate_read_change(reg);
      }
    } else {
      auto reg = opnd_get_reg(opnd);
      if (reg_is_gpr(reg) &&
          instr_writes_to_reg(&instr, reg, DR_QUERY_INCLUDE_ALL)) {
        reg_state[get_parent_register(reg)] |= calculate_write_change(reg);
      }
    }
  }

  for (reg_id_t reg = DR_REG_RAX; reg <= DR_REG_DIL; ++reg) {
    if (instr_writes_to_exact_reg(&instr, reg, DR_QUERY_INCLUDE_ALL)) {
      if (reg_is_gpr(reg))
        reg_state[get_parent_register(reg)] |= calculate_write_change(reg);
    }
  }

  if (instr_get_opcode(&instr) == OP_lea) {
    if (auto reg_target = get_reg_target(0)) {
      auto const max_read = reg_state[get_parent_register(*reg_target)];

        RegisterStateEx limit = REGISTER_EX_UNTOUCHED;
            limit |= REGISTER_EX_WRITE_64;
            limit |= REGISTER_EX_WRITE_32;
            limit |= REGISTER_EX_WRITE_16;
            limit |= REGISTER_EX_WRITE_8;

        if (max_read & REGISTER_EX_WRITE_8)
            limit |= REGISTER_EX_READ_8;

        if (max_read & REGISTER_EX_WRITE_16)
            limit |= REGISTER_EX_READ_16;

        if (max_read & REGISTER_EX_WRITE_32)
            limit |= REGISTER_EX_READ_32;

        if (max_read & REGISTER_EX_WRITE_64)
            limit |= REGISTER_EX_READ_64;

        for (auto& state : reg_state)
            state = state & limit;
    }
  }

#if 0
    if (src_count > 0 && dst_count > 0 && !is_constant_write())
    {
        auto opnd_src = instr_get_src(&instr, 0);
        auto opnd_dst = instr_get_dst(&instr, 0);

        if (opnd_is_reg(opnd_src) && opnd_is_reg(opnd_dst))
        {
            auto reg_src = opnd_get_reg(opnd_src);
            auto reg_dest = opnd_get_reg(opnd_dst);

            if (reg_src == reg_dest)
                LOG_INFO(LOG_FILTER_CALL_TARGET, "\t SAMEREG opcode %s addr %lx  REGISTER_STATE %s",
                     decode_opcode_name(instr_get_opcode(&instr)), get_address(), to_string(reg_state).c_str());
        }
    }
#endif

  return reg_state;
}

RegisterState CADecoder::__get_register_state(std::size_t reg) {
  RegisterState state = REGISTER_UNTOUCHED;
  if (instr_writes_to_reg(&instr, reg, DR_QUERY_DEFAULT))
    state = static_cast<RegisterState>(state | REGISTER_WRITE);
  if (instr_reads_from_reg(&instr, reg, DR_QUERY_DEFAULT))
    state = static_cast<RegisterState>(state | REGISTER_READ);
  return state;
}

std::pair<size_t, size_t> CADecoder::get_register_range() {
  return std::make_pair(DR_REG_RAX, DR_REG_R15);
}
