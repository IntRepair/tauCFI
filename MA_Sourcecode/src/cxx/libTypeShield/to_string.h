#ifndef __TO_STRING_H
#define __TO_STRING_H

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include <type_traits>

#include "mpl.h"

#include <BPatch.h>
#include <BPatch_basicBlock.h>

template <typename int_t> std::string int_to_hex(int_t value)
{
    std::stringstream ss;
    ss << "0x" << std::setfill('0') << std::setw(sizeof(int_t) * 2) << std::hex << value;
    return ss.str();
}

template <typename object_t> std::string to_string(object_t const &object);

template <typename object_t> std::string to_string(std::vector<object_t> const &objects)
{
    std::stringstream ss;

    for (auto const &object : objects)
        ss << to_string<typename mpl::unqualified_type<object_t>::type>(object) << "\n";

    return ss.str();
}

static inline std::string param_to_string(std::array<char, 7> const &params)
{
    std::stringstream ss;

    for (auto const &param : params)
        ss << param << " ";

    return ss.str();
}

template <> inline std::string to_string(BPatch_basicBlock *const &block)
{
    std::stringstream ss;

    ss << "BasicBlock Path element" << std::hex << block->getStartAddress();

    return ss.str();
}

template <> inline std::string to_string(std::string const &string) { return string; }

#endif /* __TO_STRING_H */
