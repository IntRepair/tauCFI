#ifndef __VERIFICATION
#define __VERIFICATION

#include "callsites.h"
#include "calltargets.h"

#include "ast/ast.h"

namespace verification
{
void pairing(ast::ast const &ast, std::vector<CallTargets> const &cts, std::vector<CallSites> const &css);
};
#endif /* __VERIFICATION */
