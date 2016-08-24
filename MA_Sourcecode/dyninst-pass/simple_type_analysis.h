#ifndef __SIMPLE_TYPE_ANALYSIS
#define __SIMPLE_TYPE_ANALYSIS

#include <unordered_map>
#include <vector>

#include <BPatch.h>
#include <BPatch_basicBlock.h>
#include <BPatch_function.h>
#include <BPatch_image.h>

#include "ca_decoder.h"
#include "register_states.h"

namespace simple_type_analysis
{

enum type_state_t
{
    SIMPLE_TYPE_UNKNOWN = 0,
    SIMPLE_TYPE_W0,
    SIMPLE_TYPE_W8,
    SIMPLE_TYPE_W16,
    SIMPLE_TYPE_W32,
    SIMPLE_TYPE_W64
};

namespace callsite
{
using state_t = type_state_t;
using RegisterStates = register_states_t<state_t>;
using RegisterStateMap = std::unordered_map<uint64_t, RegisterStates>;

RegisterStateMap init();

RegisterStates analysis(CADecoder *decoder, BPatch_image *image,
                        std::vector<BPatch_basicBlock *> blocks, RegisterStateMap &block_states);

RegisterStates analysis(CADecoder *decoder, BPatch_image *image, BPatch_function *function,
                        RegisterStateMap &block_states);

RegisterStates analysis(CADecoder *decoder, BPatch_image *image, BPatch_basicBlock *block,
                        RegisterStateMap &block_states);

RegisterStates analysis(CADecoder *decoder, BPatch_image *image, BPatch_basicBlock *block,
                        Dyninst::Address end_address, RegisterStateMap &block_states);
};

namespace calltarget
{
using state_t = std::pair<bool, type_state_t>;
using RegisterStates = register_states_t<state_t>;
using RegisterStateMap = std::unordered_map<uint64_t, RegisterStates>;

RegisterStateMap init();

RegisterStates analysis(CADecoder *decoder, BPatch_image *image,
                        std::vector<BPatch_basicBlock *> blocks, RegisterStateMap &block_states);

RegisterStates analysis(CADecoder *decoder, BPatch_image *image, BPatch_function *function,
                        RegisterStateMap &block_states);

RegisterStates analysis(CADecoder *decoder, BPatch_image *image, BPatch_basicBlock *block,
                        RegisterStateMap &block_states);
};
};

#endif /* __SIMPLE_TYPE_ANALYSIS */
