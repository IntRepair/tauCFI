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

namespace type_analysis
{
namespace simple
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
};

#include "to_string.h"

template <> std::string to_string(type_analysis::simple::type_state_t const &reg_state)
{
    using type_analysis::simple::type_state_t;
    switch (reg_state)
    {
    case type_state_t::SIMPLE_TYPE_UNKNOWN:
        return "XX";
    case type_state_t::SIMPLE_TYPE_W0:
        return "00";
    case type_state_t::SIMPLE_TYPE_W8:
        return "08";
    case type_state_t::SIMPLE_TYPE_W16:
        return "16";
    case type_state_t::SIMPLE_TYPE_W32:
        return "32";
    case type_state_t::SIMPLE_TYPE_W64:
        return "64";
    }
}

template <> std::string to_string(std::pair<bool, type_analysis::simple::type_state_t> const &state)
{
    return (state.first ? "<locked>" : "<unlocked>") + to_string(state.second);
}
#endif /* __SIMPLE_TYPE_ANALYSIS */
