#include "register_states.h"

#include <dr_api.h>
#include <dr_ir_opnd.h>

#include <unordered_map>

std::string register_name(std::size_t reg_index)
{
    auto static string_table = []() {
        using str_table_t = std::unordered_map<std::size_t, std::string>;
        str_table_t table({{DR_REG_RAX, "RAX"},
                           {DR_REG_RCX, "RCX"},
                           {DR_REG_RDX, "RDX"},
                           {DR_REG_RBX, "RBX"},
                           {DR_REG_RSP, "RSP"},
                           {DR_REG_RBP, "RBP"},
                           {DR_REG_RSI, "RSI"},
                           {DR_REG_RDI, "RDI"},
                           {DR_REG_R8, "R8"},
                           {DR_REG_R9, "R9"},
                           {DR_REG_R10, "R10"},
                           {DR_REG_R11, "R11"},
                           {DR_REG_R12, "R12"},
                           {DR_REG_R13, "R13"},
                           {DR_REG_R14, "R14"},
                           {DR_REG_R15, "R15"}});
        return table;
    }();

    return string_table[reg_index];
}
