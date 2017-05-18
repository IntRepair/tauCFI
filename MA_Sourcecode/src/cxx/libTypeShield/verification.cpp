#include "verification.h"

#include <fstream>

#include <BPatch_function.h>

#include "systemv_abi.h"

namespace verification {
#if 0
void pairing(ast::ast const &ast, std::vector<CallTargets> const &ctss,
             std::vector<CallSites> const &csss)
{
    std::ofstream at_file(std::string("verify.") + ast.get_name() + std::string(".at"));
    for (auto const &ast_function : ast.get_functions())
    {
        if (ast_function.get_is_at())
        {
            at_file << "<AT>;" << ast_function.get_name() << ";"
                    << int_to_hex(ast_function.get_address()) << "\n";
        }
    }

#ifdef __PADYN_TYPE_POLICY
    static const std::array<std::string, 3> base_strings{
        "verify.type_exp5.", "verify.type_exp6.", "verify.type_exp7.",
        //"verify.type_exp8.",
    };

#elif defined(__PADYN_COUNT_EXT_POLICY)

    static const std::array<std::string, 4> base_strings{
        //"verify.sources_destr_no_follow.",
        //"verify.sources_destr_follow.",
        "verify.sources_inter_no_follow.", "verify.sources_inter_follow.",
        "verify.sources_union_no_follow.", "verify.sources_union_follow.",
    };

#else
    static const std::array<std::string, 2> base_strings{
        "verify.count_prec", "verify.count_safe",
    };

#endif

    for (auto index = 0; index < ctss.size(); ++index)
    {
        auto base_string = base_strings[index];
        auto cts = ctss[index];
        auto css = csss[index];

        std::ofstream cs_file(base_string + ast.get_name() + std::string(".cs"));
        cs_file << to_string(css);

        std::ofstream ct_file(base_string + ast.get_name() + std::string(".ct"));
        ct_file << to_string(cts);
    }
}
#endif

void output(ast::ast const &ast, TypeShield::analysis::result_t &result) {
  std::ofstream at_file(std::string("verify.") + ast.get_name() +
                        std::string(".at"));
  for (auto const &ast_function : ast.get_functions()) {
    if (ast_function.get_is_at()) {
      at_file << "<AT>;" << ast_function.get_name() << ";"
              << int_to_hex(ast_function.get_address()) << "\n";
    }
  }

  {
    std::ofstream ct_file("verify." + ast.get_name() + std::string(".ct"));
    auto const param_string =
        param_to_string(system_v::calltarget::default_paramlist());

    for (auto const &function : ast.get_functions()) {
      ct_file << "<CT>;";
      ct_file << function.get_name() << ";";
      ct_file << int_to_hex(function.get_address()) << ";";
      ct_file << (function.get_is_at() ? "AT" : "") << ":";
      ct_file << (function.get_is_plt() ? "PLT" : "") << ";";
      ct_file << param_string << "\n";
    }
  }
  for (auto &ct_result : result.calltargets) {
    std::ofstream ct_file("verify." + ct_result.config_name + "." +
                          ast.get_name() + std::string(".ct"));

    for (auto &function : ast.get_functions()) {
      auto const funaddress = function.get_address();

      ct_file << "<CT>;";
      ct_file << function.get_name() << ";";
      ct_file << int_to_hex(funaddress) << ";";
      ct_file << (function.get_is_at() ? "AT" : "") << ":";
      ct_file << (function.get_is_plt() ? "PLT" : "") << ";";
      ct_file << param_to_string(ct_result.param_lookup[funaddress]) << "\n";
    }
  }

  {
    std::ofstream cs_file("verify." + ast.get_name() + std::string(".cs"));
    auto const param_string =
        param_to_string(system_v::callsite::default_paramlist());

    for (auto const &function : ast.get_functions()) {
      if (!function.get_is_plt()) {
        auto const funcname = function.get_name();
        auto const funcaddress = function.get_address();
        auto const function_at = (function.get_is_at() ? "AT" : "");
        auto const function_plt = (function.get_is_plt() ? "PLT" : "");

        for (auto const &block : function.get_basic_blocks()) {
          if (block.get_is_indirect_callsite()) {
            cs_file << "<CS>;";
            cs_file << funcname << ";";
            cs_file << int_to_hex(funcaddress) << ";";
            cs_file << function_at << ":" << function_plt << ";";
            cs_file << param_string << ";";
            cs_file << int_to_hex(block.get_address()) << ";";
            cs_file << "0" /*int_to_hex(block.back()->get_address())*/ << "\n";
          }
        }
      }
    }
  }

  for (auto &cs_result : result.callsites) {
    std::ofstream cs_file("verify." + cs_result.config_name + "." +
                          ast.get_name() + std::string(".cs"));

    for (auto const &function : ast.get_functions()) {
      if (!function.get_is_plt()) {
        auto const funcname = function.get_name();
        auto const funcaddress = function.get_address();
        auto const function_at = (function.get_is_at() ? "AT" : "");
        auto const function_plt = (function.get_is_plt() ? "PLT" : "");

        for (auto const &block : function.get_basic_blocks()) {
          if (block.get_is_indirect_callsite()) {
            auto const bbaddress = block.get_address();
            cs_file << "<CS>;";
            cs_file << funcname << ";";
            cs_file << int_to_hex(funcaddress) << ";";
            cs_file << function_at << ":" << function_plt << ";";
            cs_file << param_to_string(cs_result.param_lookup[bbaddress])
                    << ";";
            cs_file << int_to_hex(bbaddress) << ";";
            cs_file << "0" /*int_to_hex(block.back()->get_address())*/ << "\n";
          }
        }
      }
    }
  }
}
};
