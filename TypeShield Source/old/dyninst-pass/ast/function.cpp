#include "function.h"

#include "logging.h"

#include <BPatch_function.h>

ast::function::function(BPatch_function *function_, bool is_plt_, std::string funcname_,
                        uint64_t funcaddress_, uint64_t funcend_)
    : bp_function(function_), is_plt(is_plt_), is_at(false), funcname(funcname_),
      funcaddress(funcaddress_), funcend(funcend_)
{
    // TODO implement reading of BBs and Instructions ...
}

BPatch_function *ast::function::get_function() const { return bp_function; }

bool ast::function::get_is_plt() const { return is_plt; }

bool ast::function::get_is_at() const { return is_at; }

void ast::function::update_is_at(bool is_at_) { is_at = is_at_; }

std::string ast::function::get_name() const { return funcname; }

void ast::function::update_name(std::string funcname_) { funcname = funcname_; }

uint64_t ast::function::get_address() const { return funcaddress; }

uint64_t ast::function::get_end() const { return funcend; }

void ast::function::update_end(uint64_t funcend_) { funcend = funcend_; }

std::vector<ast::basic_block> const &ast::function::get_basic_blocks() const
{
    return basic_blocks;
}

const ast::basic_block ast::function::get_basic_block(uint64_t address) const
{
    auto itr = std::find_if(
        basic_blocks.begin(), basic_blocks.end(),
        [address](basic_block const &bb) { return bb.get_address() == address; });

    if (itr == basic_blocks.end())
    {
        LOG_ERROR(LOG_FILTER_GENERAL, "Could not find basic block for address %lx",
                  address);
        // TODO: throw exception
    }

    return *itr;
}
