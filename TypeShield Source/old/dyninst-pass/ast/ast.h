#ifndef __AST_BASIC_H
#define __AST_BASIC_H

#include <string>
#include <vector>

#include "function.h"
#include "parsing/region_data.h"

class BPatch_object;
class CADecoder;

namespace ast
{
class ast
{
  public:
    static ast create(BPatch_object *object, CADecoder *decoder);

    BPatch_object *get_object() const;
    bool get_is_shared() const;
    std::string get_name() const;

    std::vector<function> const &get_functions() const;
    bool is_function(uint64_t address) const;
    const function get_function(uint64_t address) const;

    region_data get_region_data(std::string name) const;

  private:
    ast(BPatch_object *object_, bool is_shared_, std::string name_,
        std::vector<function> functions_);

    BPatch_object *object;
    bool is_shared;
    std::string objname;
    std::vector<function> functions;
};
}

#endif /* __AST_BASIC_H */
