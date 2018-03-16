#ifndef __CALLTARGETS_H
#define __CALLTARGETS_H

#include <array>
#include <cinttypes>
#include <unordered_map>
#include <vector>

#include "ast/ast.h"

class CADecoder;

namespace TypeShield {
namespace calltargets {
struct result_t {
  std::string config_name;
  std::string description;

  std::unordered_map<uint64_t, std::array<char, 7>> param_lookup;
};

namespace count {
std::vector<result_t> analysis(ast::ast const &ast, CADecoder *decoder,
                               int liveness_cfg, int reaching_cfg);
}
namespace type {
std::vector<result_t> analysis(ast::ast const &ast, CADecoder *decoder,
                               int liveness_cfg, int reaching_cfg);
}
}
}

#endif /* __CALLTARGETS_H */
