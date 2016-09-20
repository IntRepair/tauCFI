#ifndef __REACHING_ANALYSIS
#define __REACHING_ANALYSIS

#include <unordered_map>
#include <vector>

#include <BPatch.h>
#include <BPatch_basicBlock.h>
#include <BPatch_function.h>
#include <BPatch_image.h>
#include <BPatch_object.h>

#include "ca_decoder.h"
#include "register_states.h"
#include "calltargets.h"

namespace reaching
{

enum state_t
{
    REGISTER_UNKNOWN,
    REGISTER_TRASHED,
    REGISTER_SET,
};

using RegisterStates = register_states_t<state_t>;
using RegisterStateMap = std::unordered_map<uint64_t, RegisterStates>;
using FnCallReverse = std::pair<BPatch_basicBlock *, uint64_t>;
using FunctionCallReverseLookup =
    std::unordered_map<BPatch_basicBlock *, std::vector<FnCallReverse>>;

using FunctionMinLivenessLookup =
    std::unordered_map<BPatch_basicBlock *, RegisterStates>;

struct AnalysisConfig
{
    RegisterStateMap block_states;
    FunctionCallReverseLookup block_callers;
    FunctionMinLivenessLookup min_liveness;

    CADecoder *decoder;
    BPatch_image *image;

    bool follow_return_edges;
    bool follow_into_callers;

    RegisterStates (*merge_horizontal_union)(std::vector<RegisterStates>);
    RegisterStates (*merge_horizontal_inter)(std::vector<RegisterStates>);
    RegisterStates (*merge_vertical)(RegisterStates, RegisterStates);
    bool (*can_change)(RegisterStates &reg_state);
};

namespace calltarget
{
AnalysisConfig init(CADecoder *decoder, BPatch_image *image, BPatch_object *object);
};
namespace callsite
{
AnalysisConfig init(CADecoder *decoder, BPatch_image *image, BPatch_object *object, CallTargets &call_targets);
};

RegisterStates analysis(AnalysisConfig &config, BPatch_function *function);

RegisterStates analysis(AnalysisConfig &config, BPatch_basicBlock *block,
                        Dyninst::Address end_address);
};

#include "to_string.h"

template <> std::string to_string(reaching::state_t const &reg_state)
{
    switch (reg_state)
    {
    case reaching::REGISTER_UNKNOWN:
        return "*";
    case reaching::REGISTER_TRASHED:
        return "T";
    case reaching::REGISTER_SET:
        return "S";
    }
}

#endif /* __REACHING_ANALYSIS_H */
