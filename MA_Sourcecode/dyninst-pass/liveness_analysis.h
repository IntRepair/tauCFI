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

enum state_t
{
    REGISTER_CLEAR,
    REGISTER_READ_BEFORE_WRITE,
    REGISTER_WRITE_BEFORE_READ,
    REGISTER_WRITE_OR_UNTOUCHED,
};

using RegisterStates = register_states_t<state_t>;
using RegisterStateMap = std::unordered_map<uint64_t, RegisterStates>;
using IgnoreInstructions = std::unordered_set<uint64_t>;

struct AnalysisConfig
{
    IgnoreInstructions ignore_instructions;
    RegisterStateMap block_states;
    CADecoder *decoder;
    BPatch_image *image;

    RegisterStates (*merge_horizontal)(std::vector<RegisterStates>);
    RegisterStates (*merge_vertical)(RegisterStates, RegisterStates);
    //    bool (*can_change)(RegisterStates &reg_state);
};

namespace calltarget
{
AnalysisConfig init(CADecoder *decoder, BPatch_image *image, BPatch_object *object);
};
namespace callsite
{
AnalysisConfig init(CADecoder *decoder, BPatch_image *image, BPatch_object *object);
}

RegisterStates analysis(AnalysisConfig &config, std::vector<BPatch_basicBlock *> blocks);

RegisterStates analysis(AnalysisConfig &config, BPatch_function *function);

RegisterStates analysis(AnalysisConfig &config, BPatch_basicBlock *block);
};

#include "to_string.h"

template <> std::string to_string(liveness::state_t const &reg_state)
{
    switch (reg_state)
    {
    case liveness::REGISTER_CLEAR:
        return "C";
    case liveness::REGISTER_READ_BEFORE_WRITE:
        return "R";
    case liveness::REGISTER_WRITE_BEFORE_READ:
        return "W";
    case liveness::REGISTER_WRITE_OR_UNTOUCHED:
        return "*";
    }
}

#endif /* __LIVENESS_ANALYSIS_H */
