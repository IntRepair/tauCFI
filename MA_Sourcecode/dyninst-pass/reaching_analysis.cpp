#include "reaching_analysis.h"

template <> reaching_state_t convert_state(RegisterState reg_state)
{
    switch (reg_state)
    {
    case REGISTER_UNTOUCHED:
        return REGISTER_UNKNOWN;
    case REGISTER_READ:
        return REGISTER_UNKNOWN;
    case REGISTER_WRITE:
        return REGISTER_SET;
    case REGISTER_READ_WRITE:
        return REGISTER_SET;
    }
}

template <> std::string to_string(reaching_state_t reg_state)
{
    switch (reg_state)
    {
    case REGISTER_UNKNOWN:
        return "*";
    case REGISTER_TRASHED:
        return "T";
    case REGISTER_SET:
        return "S";
    }
}
