#include "ast.h"

#include "logging.h"
#include "utils.h"

#include <BPatch_object.h>

using util::contains_if;

std::string ast::ast::get_name() const { return objname; }

bool ast::ast::get_is_shared() const { return is_shared; }

BPatch_object *ast::ast::get_object() const { return object; }

std::vector<ast::function> const &ast::ast::get_functions() const { return functions; }

bool ast::ast::is_function(uint64_t address) const
{
    return contains_if(
        functions, [address](function const &fn) { return fn.get_address() == address; });
}

const ast::function ast::ast::get_function(uint64_t address) const
{
    auto itr =
        std::find_if(functions.begin(), functions.end(), [address](function const &fn) {
            return fn.get_address() == address;
        });

    if (itr == functions.end())
    {
        int x = *((int *)NULL);
        LOG_ERROR(LOG_FILTER_AST, "Could not find function for address %lx %x", address,
                  x);
        // TODO: throw exception
    }

    return *itr;
}

region_data ast::ast::get_region_data(std::string name) const
{
    return region_data::create(name, get_object());
}

ast::ast::ast(BPatch_object *object_, bool is_shared_, std::string name_,
              std::vector<function> functions_)
    : object(object_), is_shared(is_shared_), objname(name_), functions(functions_)
{
}
