#ifndef __CALLTARGETS_H
#define __CALLTARGETS_H

#include <array>
#include <string>
#include <vector>

#include <BPatch.h>
#include <BPatch_function.h>

#include "ast/ast.h"
#include "ca_defines.h"

class CADecoder;

struct CallTarget
{
    CallTarget(BPatch_function *function_, bool address_taken_, bool plt_,
               std::array<char, 7> parameters_)
        : function(function_), address_taken(address_taken_), plt(plt_),
          parameters(parameters_)
    {
    }

    BPatch_function *function;
    bool address_taken;
    bool plt;
    std::array<char, 7> parameters;
};

using CallTargets = std::vector<CallTarget>;

std::vector<CallTargets> calltarget_analysis(ast::ast const &ast, CADecoder *decoder);

#include "to_string.h"

template <> inline std::string to_string(CallTarget const &call_target)
{
    Dyninst::Address start, end;

    auto funcname = [&]() {
        auto typed_name = call_target.function->getTypedName();
        if (typed_name.empty())
            return call_target.function->getName();
        return typed_name;
    }();

    call_target.function->getAddressRange(start, end);

    return "<CT>;" + funcname + ";" + int_to_hex(start) + ";" +
           (call_target.address_taken ? "AT" : "") + ":" +
           (call_target.plt ? "PLT" : "") + ";" + param_to_string(call_target.parameters);
}

#endif /* __CALLTARGETS_H */
