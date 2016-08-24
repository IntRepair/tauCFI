#ifndef __LIVENESS_ANALYSIS_H
#define __LIVENESS_ANALYSIS_H

#include <unordered_map>
#include <vector>

#include <BPatch.h>
#include <BPatch_basicBlock.h>
#include <BPatch_function.h>
#include <BPatch_image.h>

#include "ca_decoder.h"
#include "register_states.h"

enum liveness_state_t
{
    REGISTER_CLEAR,
    REGISTER_READ_BEFORE_WRITE,
    REGISTER_WRITE_BEFORE_READ,
    REGISTER_WRITE_OR_UNTOUCHED,
};

using LivenessRegisterStates = register_states_t<liveness_state_t>;
using LivenessRegisterStateMap = std::unordered_map<uint64_t, LivenessRegisterStates>;

LivenessRegisterStateMap liveness_init();

LivenessRegisterStates liveness_analysis(CADecoder *decoder, BPatch_image *image,
                                         std::vector<BPatch_basicBlock *> blocks,
                                         LivenessRegisterStateMap &block_states);

LivenessRegisterStates liveness_analysis(CADecoder *decoder, BPatch_image *image,
                                         BPatch_function *function,
                                         LivenessRegisterStateMap &block_states);

LivenessRegisterStates liveness_analysis(CADecoder *decoder, BPatch_image *image,
                                         BPatch_basicBlock *block,
                                         LivenessRegisterStateMap &block_states);

#endif /* __LIVENESS_ANALYSIS_H */
