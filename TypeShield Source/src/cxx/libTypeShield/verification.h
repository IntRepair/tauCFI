#ifndef __VERIFICATION
#define __VERIFICATION

#include "callsites.h"
#include "calltargets.h"

#include "analysis.h"

#include "ast/ast.h"

namespace verification
{
//void pairing(ast::ast const &ast, std::vector<CallTargets> const &cts, std::vector<CallSites> const &css);

void output(ast::ast const &ast, TypeShield::analysis::result_t  &result);
};
#endif /* __VERIFICATION */
