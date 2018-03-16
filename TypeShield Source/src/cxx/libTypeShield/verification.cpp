#include "verification.h"

#include <fstream>

#include <BPatch_function.h>

#include "systemv_abi.h"

namespace verification {
void output(ast::ast const &ast, TypeShield::analysis::result_t &result) {

  // <FN>; $fn_name; $demang_fn_name; $current_path/$module; $flags; $arg_types; $arg_count; $ret_type;

  {
    std::ofstream ct_file("verify." + ast.get_name() + std::string(".ct"));
    auto const param_string =
        param_to_string(system_v::calltarget::default_paramlist());

    for (auto const &function : ast.get_functions()) {
      ct_file << "<FN>;";
      ct_file << function.get_name_mang() << ";";
      ct_file << function.get_name_demang() << ";";
      ct_file << "" << ";";
      ct_file << (function.get_is_at() ? "AT" : "") << "|";
      ct_file << (function.get_is_plt() ? "PLT" : "") << ";";
      ct_file << param_string << ";";
      ct_file << int_to_hex(function.get_address()) << ";" << '\n';
    }
  }
  for (auto &ct_result : result.calltargets) {
    std::ofstream ct_file("verify." + ct_result.config_name + "." +
                          ast.get_name() + std::string(".ct"));

    for (auto &function : ast.get_functions()) {
      auto const funaddress = function.get_address();

      ct_file << "<FN>;";
      ct_file << function.get_name_mang() << ";";
      ct_file << function.get_name_demang() << ";";
      ct_file << "" << ";";
      ct_file << (function.get_is_at() ? "AT" : "") << "|";
      ct_file << (function.get_is_plt() ? "PLT" : "") << ";";
      ct_file << param_to_string(ct_result.param_lookup[funaddress]) << ";";
      ct_file << int_to_hex(funaddress) << ";" << '\n';
    }
  }

  // <CS>; $fn_name; $demang_fn_name; $current_path/$module; $flags; $arg_types; $arg_count; $ret_type;

  {
    std::ofstream cs_file("verify." + ast.get_name() + std::string(".cs"));
    auto const param_string =
        param_to_string(system_v::callsite::default_paramlist());

    for (auto const &function : ast.get_functions()) {
      if (!function.get_is_plt()) {
        auto const funcname_mang = function.get_name_mang();
        auto const funcname_demang = function.get_name_demang();
        auto const funcaddress = function.get_address();

        for (auto const &block : function.get_basic_blocks()) {
          if (block.get_is_indirect_callsite()) {
            cs_file << "<CS>;";
            cs_file << funcname_mang << ";";
            cs_file << funcname_demang << ";";
            cs_file << "" << ";";
            cs_file << "ICS" << ";";
            cs_file << param_string << ";";
            cs_file << int_to_hex(block.get_address()) << ";";
            cs_file << int_to_hex(funcaddress) << ";";
            cs_file << "0" /*int_to_hex(block.back()->get_address())*/ << ";" << '\n';
          }
        }
      }
    }
  }

  // <CS>; $fn_name; $demang_fn_name; $current_path/$module; $flags; $arg_types; $arg_count; $ret_type;

  for (auto &cs_result : result.callsites) {
    std::ofstream cs_file("verify." + cs_result.config_name + "." +
                          ast.get_name() + std::string(".cs"));

    for (auto const &function : ast.get_functions()) {
      if (!function.get_is_plt()) {
        auto const funcname_mang = function.get_name_mang();
        auto const funcname_demang = function.get_name_demang();
        auto const funcaddress = function.get_address();

        for (auto const &block : function.get_basic_blocks()) {
          if (block.get_is_indirect_callsite()) {
            auto const bbaddress = block.get_address();
            cs_file << "<CS>;";
            cs_file << funcname_mang << ";";
            cs_file << funcname_demang << ";";
            cs_file << "" << ";";
            cs_file << "ICS" << ";";
            cs_file << param_to_string(cs_result.param_lookup[bbaddress])
                    << ";";
            cs_file << int_to_hex(bbaddress) << ";";
            cs_file << int_to_hex(funcaddress) << ";";
            cs_file << "0" /*int_to_hex(block.back()->get_address())*/ << ";" << '\n';
          }
        }
      }
    }
  }
}
};
