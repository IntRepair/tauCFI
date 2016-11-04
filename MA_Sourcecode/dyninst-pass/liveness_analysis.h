#ifndef __LIVENESS_ANALYSIS_H
#define __LIVENESS_ANALYSIS_H

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <BPatch.h>
#include <BPatch_basicBlock.h>
#include <BPatch_function.h>
#include <BPatch_image.h>
#include <BPatch_object.h>

#include "ca_decoder.h"
#include "register_states.h"

namespace liveness
{

enum state_t : uint8_t
{
    REGISTER_CLEAR = 0x0,

    REGISTER_READ_BEFORE_WRITE_8 = 0x1,
    REGISTER_READ_BEFORE_WRITE_16 = 0x2,
    REGISTER_READ_BEFORE_WRITE_32 = 0x4,
    REGISTER_READ_BEFORE_WRITE_64 = 0x8,
    REGISTER_READ_BEFORE_WRITE_FULL = 0xF,

    REGISTER_WRITE_BEFORE_READ_8 = 0x10,
    REGISTER_WRITE_BEFORE_READ_16 = 0x20,
    REGISTER_WRITE_BEFORE_READ_32 = 0x40,
    REGISTER_WRITE_BEFORE_READ_64 = 0x80,
    REGISTER_WRITE_BEFORE_READ_FULL = 0xF0,
};

inline state_t operator|(state_t a, state_t b)
{
    return static_cast<state_t>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline state_t operator&(state_t a, state_t b)
{
    return static_cast<state_t>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline state_t &operator|=(state_t &a, state_t b) { return a = a | b; }

inline bool is_read_before_write(state_t state)
{
    return REGISTER_CLEAR != (state & REGISTER_READ_BEFORE_WRITE_FULL);
}

inline bool is_read_before_write_8(state_t state)
{
    return REGISTER_READ_BEFORE_WRITE_8 == (state & REGISTER_READ_BEFORE_WRITE_8);
}

inline bool is_read_before_write_16(state_t state)
{
    return REGISTER_READ_BEFORE_WRITE_16 == (state & REGISTER_READ_BEFORE_WRITE_16);
}

inline bool is_read_before_write_32(state_t state)
{
    return REGISTER_READ_BEFORE_WRITE_32 == (state & REGISTER_READ_BEFORE_WRITE_32);
}

inline bool is_read_before_write_64(state_t state)
{
    return REGISTER_READ_BEFORE_WRITE_64 == (state & REGISTER_READ_BEFORE_WRITE_64);
}

inline bool is_write_before_read(state_t state)
{
    return REGISTER_CLEAR != (state & REGISTER_WRITE_BEFORE_READ_FULL);
}

inline bool is_write_before_read_8(state_t state)
{
    return REGISTER_WRITE_BEFORE_READ_8 == (state & REGISTER_WRITE_BEFORE_READ_8);
}

inline bool is_write_before_read_16(state_t state)
{
    return REGISTER_WRITE_BEFORE_READ_16 == (state & REGISTER_WRITE_BEFORE_READ_16);
}

inline bool is_write_before_read_32(state_t state)
{
    return REGISTER_WRITE_BEFORE_READ_32 == (state & REGISTER_WRITE_BEFORE_READ_32);
}

inline bool is_write_before_read_64(state_t state)
{
    return REGISTER_WRITE_BEFORE_READ_64 == (state & REGISTER_WRITE_BEFORE_READ_64);
}

using RegisterStates = register_states_t<state_t>;
using RegisterStateMap = std::unordered_map<uint64_t, RegisterStates>;
using IgnoreInstructions = std::unordered_set<uint64_t>;

struct AnalysisConfig
{
    std::shared_ptr<IgnoreInstructions> ignore_instructions;
    RegisterStateMap block_states;
    CADecoder *decoder;
    BPatch_image *image;

    RegisterStates (*merge_horizontal)(std::vector<RegisterStates>);
    RegisterStates (*merge_vertical)(RegisterStates, RegisterStates);
    bool (*can_change)(RegisterStates &reg_state);

    bool follow_calls;
    bool ignore_nops;
};

namespace count
{
namespace calltarget
{
AnalysisConfig init(CADecoder *decoder, BPatch_image *image, BPatch_object *object);
};
namespace callsite
{
AnalysisConfig init(CADecoder *decoder, BPatch_image *image, BPatch_object *object);
};
};

namespace count_ext
{
namespace calltarget
{
std::vector<AnalysisConfig> init(CADecoder *decoder, BPatch_image *image, BPatch_object *object);
};
namespace callsite
{
std::vector<AnalysisConfig> init(CADecoder *decoder, BPatch_image *image, BPatch_object *object);
};
};

namespace type
{
namespace calltarget
{
std::vector<AnalysisConfig> init(CADecoder *decoder, BPatch_image *image, BPatch_object *object);
};
namespace callsite
{
std::vector<AnalysisConfig> init(CADecoder *decoder, BPatch_image *image, BPatch_object *object);
};
};

RegisterStates
analysis(AnalysisConfig &config, std::vector<BPatch_basicBlock *> blocks,
         std::vector<BPatch_basicBlock *> path = std::vector<BPatch_basicBlock *>());

RegisterStates analysis(AnalysisConfig &config, BPatch_function *function);

RegisterStates
analysis(AnalysisConfig &config, BPatch_basicBlock *block,
         std::vector<BPatch_basicBlock *> path = std::vector<BPatch_basicBlock *>());
};

#include "to_string.h"

template <> inline std::string to_string(liveness::state_t const &reg_state)
{
    if (reg_state == liveness::REGISTER_CLEAR)
        return "C";

    char result[10];
    result[0] = 'R';
    result[1] = is_read_before_write_64(reg_state) ? 'x' : 'o';
    result[2] = is_read_before_write_32(reg_state) ? 'x' : 'o';
    result[3] = is_read_before_write_16(reg_state) ? 'x' : 'o';
    result[4] = is_read_before_write_8(reg_state) ? 'x' : 'o';

    result[5] = 'W';
    result[6] = is_write_before_read_64(reg_state) ? 'x' : 'o';
    result[7] = is_write_before_read_32(reg_state) ? 'x' : 'o';
    result[8] = is_write_before_read_16(reg_state) ? 'x' : 'o';
    result[9] = is_write_before_read_8(reg_state) ? 'x' : 'o';

    return std::string(result, 10);
}

#endif /* __LIVENESS_ANALYSIS_H */
