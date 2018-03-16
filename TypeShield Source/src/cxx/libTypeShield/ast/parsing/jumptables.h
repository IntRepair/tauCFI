#ifndef __JUMPTABLES_H
#define __JUMPTABLES_H

#include <vector>

#include "ast/function.h"

class BPatch_object;
class CADecoder;

std::vector<ast::function> resolve_jumptables(BPatch_object *object,
                                              std::vector<ast::function> functions,
                                              CADecoder *decoder);

#endif /* __JUMPTABLES_H */
