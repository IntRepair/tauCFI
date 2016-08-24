#ifndef __REACHING_ANALYSIS
#define __REACHING_ANALYSIS

#include <unordered_map>
#include <vector>

#include <BPatch.h>
#include <BPatch_basicBlock.h>
#include <BPatch_function.h>
#include <BPatch_image.h>

#include "ca_decoder.h"
#include "register_states.h"

enum reaching_state_t
{
    REGISTER_UNKNOWN,
    REGISTER_TRASHED,
    REGISTER_SET,
};

using ReachingRegisterStates = register_states_t<reaching_state_t>;
using ReachingRegisterStateMap = std::unordered_map<uint64_t, ReachingRegisterStates>;

ReachingRegisterStateMap reaching_init();

ReachingRegisterStates reaching_analysis(CADecoder *decoder, BPatch_image *image,
                                         std::vector<BPatch_basicBlock *> blocks,
                                         ReachingRegisterStateMap &block_states);

ReachingRegisterStates reaching_analysis(CADecoder *decoder, BPatch_image *image,
                                         BPatch_function *function,
                                         ReachingRegisterStateMap &block_states);

ReachingRegisterStates reaching_analysis(CADecoder *decoder, BPatch_image *image,
                                         BPatch_basicBlock *block,
                                         ReachingRegisterStateMap &block_states);

ReachingRegisterStates reaching_analysis(CADecoder *decoder, BPatch_image *image,
                                         BPatch_basicBlock *block, Dyninst::Address end_address,
                                         ReachingRegisterStateMap &block_states);

#endif /* __REACHING_ANALYSIS_H */
