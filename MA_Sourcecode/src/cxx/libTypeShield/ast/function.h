#ifndef __AST_FUNCTION_H
#define __AST_FUNCTION_H

#include "basic_block.h"

#include <cinttypes>
#include <string>
#include <vector>

class BPatch_function;

namespace ast
{
class function
{
  public:
    function(BPatch_function *function_, bool is_plt, std::string funcname_,
             uint64_t address_, uint64_t funcend_);

    std::vector<basic_block> const& get_basic_blocks() const;
    const basic_block get_basic_block(uint64_t address) const;
    void update_basic_blocks(std::vector<basic_block> basic_blocks_);

    bool get_is_plt() const;
    bool get_is_at() const;
    void update_is_at(bool is_at_);

    BPatch_function *get_function() const;
    std::string get_name() const;
    void update_name(std::string funcname_);

    uint64_t get_address() const;
    uint64_t get_end() const;
    void update_end(uint64_t funcend_);

  private:
    BPatch_function *bp_function;

    bool const is_plt;
    bool is_at;
    std::string funcname;
    uint64_t funcaddress;
    uint64_t funcend;

    std::vector<basic_block> basic_blocks;
};
}

#endif /* __AST_FUNCTION_H */
