#include "liveness_analysis.h"

template<> liveness_state_t convert_state(RegisterState reg_state)
{
    switch (reg_state)
    {
    case REGISTER_UNTOUCHED:
        return REGISTER_CLEAR;
    case REGISTER_READ:
        return REGISTER_READ_BEFORE_WRITE;
    case REGISTER_WRITE:
        return REGISTER_WRITE_BEFORE_READ;
    case REGISTER_READ_WRITE:
        return REGISTER_WRITE_BEFORE_READ;
    }
}

template <> std::string to_string(liveness_state_t reg_state)
{
    switch (reg_state)
    {
    case REGISTER_CLEAR:
        return "C";
    case REGISTER_READ_BEFORE_WRITE:
        return "R";
    case REGISTER_WRITE_BEFORE_READ:
        return "W";
    case REGISTER_WRITE_OR_UNTOUCHED:
        return "*";
    }
}

