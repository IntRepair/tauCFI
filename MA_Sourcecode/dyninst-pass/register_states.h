#ifndef __REGISTER_STATES_H
#define __REGISTER_STATES_H

#include <algorithm>
#include <array>
#include <string>

#include "logging.h"
#include "to_string.h"

std::string register_name(std::size_t reg_index);

template <std::size_t min, std::size_t max, typename state_t> struct __register_states_t
{
    using container_t = std::array<state_t, 1 + max - min>;

  public:
    enum
    {
        min_index = min,
        max_index = max
    };

    using this_t = __register_states_t<min, max, state_t>;

    inline state_t &operator[](std::size_t index) { return _storage[index - min]; }
    inline state_t const &operator[](std::size_t index) const { return _storage[index - min]; }

    bool state_exists(state_t state) const { return end() != std::find(begin(), end(), state); }

    template <typename unary_fun_t> friend this_t transform(this_t current, unary_fun_t unary_fun)
    {
        std::transform(current.begin(), current.end(), current.begin(), unary_fun);
        return current;
    }

    template <typename binary_fun_t>
    friend this_t transform(this_t current, this_t delta, binary_fun_t binary_fun)
    {
        std::transform(current.begin(), current.end(), delta.begin(), current.begin(), binary_fun);
        return current;
    }

    inline typename container_t::iterator begin() { return _storage.begin(); }
    inline typename container_t::iterator end() { return _storage.end(); }

    inline typename container_t::const_iterator begin() const { return _storage.cbegin(); }
    inline typename container_t::const_iterator end() const { return _storage.cend(); }

  private:
    container_t _storage{};
};

template <typename new_state_t, typename old_state_t> new_state_t convert_state(old_state_t);

template <typename new_state_t, std::size_t min, std::size_t max, typename old_state_t>
__register_states_t<min, max, new_state_t>
convert_states(__register_states_t<min, max, old_state_t> register_states)
{
    __register_states_t<min, max, new_state_t> conv_states;
    std::transform(register_states.begin(), register_states.end(), conv_states.begin(),
                   [](old_state_t state) { return convert_state<new_state_t>(state); });
    return conv_states;
}

template <std::size_t min, std::size_t max, typename register_state_t>
std::string to_string(__register_states_t<min, max, register_state_t> const &register_states)
{
    std::stringstream ss;
    ss << "[";

    for (auto reg = min; reg <= max; ++reg)
    {
        auto reg_state = register_states[reg];
        ss << register_name(reg) << "(" << to_string(reg_state) << ")"
           << ((reg == max) ? ']' : ' ');
    }

    return ss.str();
}

#endif /* __REGISTER_STATES_H */
