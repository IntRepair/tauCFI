#include "basic_block.h"

ast::basic_block::basic_block(BPatch_basicBlock *bp_basic_block_,
                              uint64_t address_, uint64_t end_,
                              bool is_indirect_callsite_)
    : bp_basic_block(bp_basic_block_), blockaddress(address_), blockend(end_),
      is_indirect_callsite(is_indirect_callsite_) {}

BPatch_basicBlock *ast::basic_block::get_basic_block() const { return bp_basic_block; }

uint64_t ast::basic_block::get_address() const { return blockaddress; }

uint64_t ast::basic_block::get_end() const { return blockend; }

bool ast::basic_block::get_is_indirect_callsite() const {
  return is_indirect_callsite;
}
