#ifndef __TO_STRING_H
#define __TO_STRING_H

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "mpl.h"

template <typename int_t> std::string int_to_hex(int_t value)
{
    std::stringstream ss;
    ss << "0x" << std::setfill('0') << std::setw(sizeof(int_t) * 2) << std::hex << value;
    return ss.str();
}

template <typename object_t> static std::string to_string(object_t const &object);

template <> std::string to_string(uint8_t const &object)
{
    std::stringstream ss;
    ss << object;
    return ss.str();
}

template <typename object_t> std::string to_string(std::vector<object_t> const &objects)
{
    std::stringstream ss;

    for (auto const &object : objects)
        ss << to_string<typename mpl::unqualified_type<object_t>::type>(object) << "\n";

    return ss.str();
}

template <typename object_t, size_t count>
std::string to_string(std::array<object_t, count> const &objects)
{
    std::stringstream ss;

    for (auto const &object : objects)
        ss << to_string<typename mpl::unqualified_type<object_t>::type>(object) << " ";

    return ss.str();
}

template <> std::string to_string(std::string const &string) { return string; }

#endif /* __TO_STRING_H */