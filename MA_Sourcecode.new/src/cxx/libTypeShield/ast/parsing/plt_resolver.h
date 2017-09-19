#ifndef __PLT_RESOLVER_H
#define __PLT_RESOLVER_H

#include <vector>

#include "ast/function.h"

class BPatch_object;
class CADecoder;

std::vector<ast::function> resolve_plt_names(BPatch_object *object,
                                             std::vector<ast::function> functions,
                                             CADecoder *decoder);

#endif /* __PLT_RESOLVER_H */