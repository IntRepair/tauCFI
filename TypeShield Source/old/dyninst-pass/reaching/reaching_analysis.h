#ifndef __REACHING_ANALYSIS
#define __REACHING_ANALYSIS

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <BPatch.h>
#include <BPatch_basicBlock.h>
#include <BPatch_function.h>

#include "ast/ast.h"
#include "ca_decoder.h"
#include "calltargets.h"
#include "register_states.h"

namespace reaching
{

enum state_t : uint8_t
{
    REGISTER_UNKNOWN = 0x0,

    REGISTER_SET_8 = 0x1,
    REGISTER_SET_16 = 0x2,
    REGISTER_SET_32 = 0x4,
    REGISTER_SET_64 = 0x8,
    REGISTER_SET_FULL = 0x0F,

    REGISTER_TRASHED = 0xF0,
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

inline bool is_set(state_t state)
{
    return REGISTER_UNKNOWN != (state & REGISTER_SET_FULL);
}

inline bool is_set_8(state_t state) { return REGISTER_SET_8 == (state & REGISTER_SET_8); }

inline bool is_set_16(state_t state)
{
    return REGISTER_SET_16 == (state & REGISTER_SET_16);
}

inline bool is_set_32(state_t state)
{
    return REGISTER_SET_32 == (state & REGISTER_SET_32);
}

inline bool is_set_64(state_t state)
{
    return REGISTER_SET_64 == (state & REGISTER_SET_64);
}

inline bool is_trashed(state_t state)
{
    return REGISTER_UNKNOWN != (state & REGISTER_TRASHED);
}

using RegisterStates = register_states_t<state_t>;
using RegisterStateMap = std::unordered_map<uint64_t, RegisterStates>;
using FnCallReverse = std::pair<BPatch_basicBlock *, uint64_t>;
using FunctionCallReverseLookup =
    std::unordered_map<BPatch_basicBlock *, std::vector<FnCallReverse>>;

using FunctionMinLivenessLookup = std::unordered_map<BPatch_basicBlock *, RegisterStates>;

using IgnoreInstructions = std::unordered_set<uint64_t>;

struct AnalysisConfig
{
    AnalysisConfig(ast::ast const &ast_) : ast(ast_) {}

    RegisterStateMap block_states;
    std::shared_ptr<FunctionCallReverseLookup> block_callers;
    std::shared_ptr<IgnoreInstructions> ignore_instructions;
    FunctionMinLivenessLookup min_liveness;

    CADecoder *decoder;
    ast::ast const &ast;

    bool follow_return_edges;
    bool follow_into_callers;
    bool use_min_liveness;
    bool ignore_nops;
    uint32_t depth;

    RegisterStates (*merge_horizontal_intra)(std::vector<RegisterStates>);
    RegisterStates (*merge_horizontal_inter)(std::vector<RegisterStates>);
    RegisterStates (*merge_horizontal_entry)(std::vector<RegisterStates>);
    RegisterStates (*merge_horizontal_mlive)(std::vector<RegisterStates>);
    RegisterStates (*merge_horizontal_rip)(std::vector<RegisterStates>);
    RegisterStates (*merge_vertical)(RegisterStates, RegisterStates);
    bool (*can_change)(RegisterStates &reg_state);
};

void reset_state(AnalysisConfig &config);

namespace count
{
namespace calltarget
{
std::vector<AnalysisConfig> init(CADecoder *decoder, ast::ast const &ast);
};
namespace callsite
{
std::vector<AnalysisConfig> init(CADecoder *decoder, ast::ast const &ast,
                                 std::vector<CallTargets> &call_targets);
};
};

namespace count_ext
{
namespace calltarget
{
std::vector<AnalysisConfig> init(CADecoder *decoder, ast::ast const &ast);
};
namespace callsite
{
std::vector<AnalysisConfig> init(CADecoder *decoder, ast::ast const &ast,
                                 std::vector<CallTargets> &call_targets);
};
};

namespace type
{
namespace calltarget
{
std::vector<AnalysisConfig> init(CADecoder *decoder, ast::ast const &ast);
};
namespace callsite
{
std::vector<AnalysisConfig> init(CADecoder *decoder, ast::ast const &ast,
                                 std::vector<CallTargets> &call_targets);
};
};

RegisterStates analysis(AnalysisConfig &config, BPatch_function *function);

RegisterStates analysis(AnalysisConfig &config, BPatch_basicBlock *block,
                        Dyninst::Address end_address);
};

#include "to_string.h"

template <> inline std::string to_string(reaching::state_t const &reg_state)
{
    if (reg_state == reaching::REGISTER_UNKNOWN)
        return "C";

    char result[10];
    result[0] = 'S';
    result[1] = (is_set_64(reg_state)) ? 'x' : 'o';
    result[2] = (is_set_32(reg_state)) ? 'x' : 'o';
    result[3] = (is_set_16(reg_state)) ? 'x' : 'o';
    result[4] = (is_set_8(reg_state)) ? 'x' : 'o';

    result[5] = 'T';
    result[6] = (reaching::is_trashed(reg_state)) ? 'x' : 'o';
    result[7] = (reaching::is_trashed(reg_state)) ? 'x' : 'o';
    result[8] = (reaching::is_trashed(reg_state)) ? 'x' : 'o';
    result[9] = (reaching::is_trashed(reg_state)) ? 'x' : 'o';

    return std::string(result, 10);
}

#endif /* __REACHING_ANALYSIS_H */
