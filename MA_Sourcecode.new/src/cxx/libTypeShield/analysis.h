#ifndef __ANALYSIS_H
#define __ANALYSIS_H

#include <cinttypes>
#include <vector>

#include "ast/ast.h"
#include "calltargets.h"
#include "callsites.h"

class CADecoder;

namespace TypeShield {

namespace analysis {

enum flags_t {
  NO_ANALYSIS = 0,
  CALLTARGET_ANALYSIS = (1 << 0),
  CALLSITE_ANALYSIS = (1 << 1),
};

enum type_t { COUNT_ANALYSIS, TYPE_ANALYSIS };

struct config_t {
  config_t()
      : flags(NO_ANALYSIS), type(COUNT_ANALYSIS), ct_liveness_config(-1),
        ct_reaching_config(-1), cs_liveness_config(-1), cs_reaching_config(-1) {
  }

  flags_t flags;
  type_t type;
  int ct_liveness_config;
  int ct_reaching_config;
  int cs_liveness_config;
  int cs_reaching_config;
};

struct result_t {
  std::vector<calltargets::result_t> calltargets;
  std::vector<callsites::result_t> callsites;
};

result_t run(ast::ast const &ast, config_t const &config,
                      CADecoder *decoder);
}
}
#endif /* __ANALYSIS_H */
