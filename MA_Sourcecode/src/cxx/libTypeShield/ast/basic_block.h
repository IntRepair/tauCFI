#ifndef __AST_BASIC_BLOCK_H
#define __AST_BASIC_BLOCK_H

#include "instruction.h"

#include <cinttypes>

class BPatch_basicBlock;

namespace ast {
class basic_block {
public:
  basic_block(BPatch_basicBlock *bp_basic_block_, uint64_t address_,
              uint64_t end_, bool is_indirect_callsite_);

  BPatch_basicBlock *get_basic_block() const;
  uint64_t get_address() const;
  uint64_t get_end() const;

  bool get_is_indirect_callsite() const;
  /*        public:
          static basic_block create(BPatch_basicBlock *basicBlock);

          const std::vector<const instruction> get_instructions() const;
          const instruction get_instruction(uint64_t address) const;

      private:
          std::vector<instruction> instructions;
*/
private:
  BPatch_basicBlock *bp_basic_block;
  uint64_t blockaddress;
  uint64_t blockend;
  bool is_indirect_callsite;
};
}

#endif /* __AST_BASIC_BLOCK_H */
