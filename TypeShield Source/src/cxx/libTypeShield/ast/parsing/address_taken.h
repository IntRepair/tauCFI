#ifndef __ADDRESS_TAKEN_H
#define __ADDRESS_TAKEN_H

#include <vector>

#include "ast/function.h"

class BPatch_object;
class CADecoder;

std::vector<ast::function> address_taken_analysis(BPatch_object *object,
                                                  std::vector<ast::function> functions,
                                                  CADecoder *decoder);

#endif /* __ADDRESS_TAKEN_H */
