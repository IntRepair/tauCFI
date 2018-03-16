#ifndef __RELATIVE_CALLSITES_H
#define __RELATIVE_CALLSITES_H

#include <array>
#include <string>
#include <vector>

#include <BPatch.h>
#include <BPatch_function.h>

#include "ast/ast.h"
#include "ca_defines.h"
#include "calltargets.h"

class CADecoder;

struct CallSite
{
    CallSite(BPatch_function *function_, uint64_t block_start_, uint64_t address_,
             std::array<char, 7> parameters_)
        : function(function_), block_start(block_start_), address(address_),
          parameters(parameters_)
    {
    }
    BPatch_function *function;
    uint64_t block_start;
    uint64_t address;
    std::array<char, 7> parameters;
};

using CallSites = std::vector<CallSite>;

std::vector<CallSites> callsite_analysis(ast::ast const &ast, CADecoder *decoder,
                                         std::vector<CallTargets> &targets);

#include "to_string.h"

template <> inline std::string to_string(CallSite const &call_site)
{
    Dyninst::Address start, end;

    auto funcname = [&]() {
        auto typed_name = call_site.function->getTypedName();
        if (typed_name.empty())
            return call_site.function->getName();
        return typed_name;
    }();

    call_site.function->getAddressRange(start, end);

    return "<CS>;" + funcname + ";" + ":" + ";" + int_to_hex(start) + ";" +
           param_to_string(call_site.parameters) + ";" +
           int_to_hex(call_site.block_start) + ";" + int_to_hex(call_site.address);
}

#endif /* __RELATIVE_CALLSITES_H */
