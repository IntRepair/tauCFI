#ifndef __AST_BASIC_BLOCK_H
#define __AST_BASIC_BLOCK_H

#include "instruction.h"

#include <cinttypes>

namespace ast
{
    class basic_block
    {
        public:
            uint64_t get_address() const;
/*        public:
            static basic_block create(BPatch_basicBlock *basicBlock);

            const std::vector<const instruction> get_instructions() const;
            const instruction get_instruction(uint64_t address) const;

        private:
            std::vector<instruction> instructions;
*/
    };
}

#endif /* __AST_BASIC_BLOCK_H */
