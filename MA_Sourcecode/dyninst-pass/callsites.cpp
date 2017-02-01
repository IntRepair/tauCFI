#include "callsites.h"

#include "ca_decoder.h"
#include "instrumentation.h"
#include "liveness_analysis.h"
#include "logging.h"
#include "reaching_analysis.h"
#include "systemv_abi.h"

#include <boost/optional.hpp>
#include <boost/range/adaptor/reversed.hpp>

#if 0
bool is_virtual_callsite(CADecoder *decoder, BPatch_basicBlock *block)
{
    // 400855:       48 8b 45 e8             mov    -0x18(%rbp),%rax
    // 400859:       48 8b 08                mov    (%rax),%rcx
    // 40085c:       48 8b 09                mov    (%rcx),%rcx
    // 40085f:       8b 75 f8                mov    -0x8(%rbp),%esi // parameter from
    // stack (main
    // fkt
    // ?)
    // 400862:       48 89 c7                mov    %rax,%rdi
    // 400865:       ff d1                   callq  *%rcx

    // MOV ? -> %THIS_PTR
    // MOV (%THIS_PTR) -> %ADDR_REG
    // MOV (%ADDR_REG) -> %ADDR_REG
    // PARAMS
    // mov THIS_PTR, %RDI
    // call *%ADDR_REG

    boost::optional<int> addr_REG;
    boost::optional<int> this_REG;
    bool addr_deref = false;

    Dyninst::PatchAPI::PatchBlock::Insns insns;
    Dyninst::PatchAPI::convert(block)->getInsns(insns);
    for (auto &instruction : boost::adaptors::reverse(insns))
    {
        // Instrument Instruction
        auto address = reinterpret_cast<uint64_t>(instruction.first);
        LOG_TRACE(LOG_FILTER_INSTRUMENTATION, "Processing instruction %lx", address);
        // call *%ADDR_REG
        if (addr_REG)
        {
            // mov THIS_PTR, %RDI
            if (this_REG)
            {
                // MOV (%ADDR_REG) -> %ADDR_REG
                if (addr_deref)
                {
                    // MOV (%THIS_PTR) -> %ADDR_REG
                    if (decoder->is_move_to(addr_REG.value()) &&
                        decoder->get_reg_source(0) == this_REG)
                    {
                        return true;
                    }
                }
                else if (decoder->is_move_to(addr_REG.value()))
                {
                    addr_deref = decoder->get_reg_source(0) == addr_REG;
                }
            }
            else if (decoder->is_move_to(system_v::get_parameter_register_from_index(0)))
            {
                this_REG = decoder->get_reg_source(0);
            }
        }
        else if (decoder->is_indirect_call())
        {
            addr_REG = decoder->get_reg_source(0);
        }
    }

    return false;
}
#endif

struct MemoryCallSite
{
    MemoryCallSite(BPatch_function *function_, uint64_t block_start_, uint64_t address_,
                   std::array<char, 7> parameters_,
                   reaching::RegisterStates reaching_state_)
        : function(function_), block_start(block_start_), address(address_),
          parameters(parameters_), reaching_state(reaching_state_)
    {
    }

    BPatch_function *function;
    uint64_t block_start;
    uint64_t address;
    std::array<char, 7> parameters;
    reaching::RegisterStates reaching_state;
};

template <> inline std::string to_string(MemoryCallSite const &call_site)
{
    Dyninst::Address start, end;

    auto funcname = [&]() {
        auto typed_name = call_site.function->getTypedName();
        if (typed_name.empty())
            return call_site.function->getName();
        return typed_name;
    }();

    call_site.function->getAddressRange(start, end);

    return "<CS*>;" + funcname + ";" + ":" + ";" + int_to_hex(start) + ";" +
           param_to_string(call_site.parameters) + " | " +
           to_string(call_site.reaching_state) + ";" + int_to_hex(call_site.block_start) +
           ";" + int_to_hex(call_site.address);
}

std::vector<CallSites> callsite_analysis(BPatch_object *object, BPatch_image *image,
                                         CADecoder *decoder,
                                         std::vector<CallTargets> &targets)
{
#ifdef __PADYN_TYPE_POLICY
    auto liveness_configs = liveness::type::callsite::init(decoder, image, object);
    auto reaching_configs =
        reaching::type::callsite::init(decoder, image, object, targets);

#elif defined(__PADYN_COUNT_EXT_POLICY)
    auto liveness_configs = liveness::count_ext::callsite::init(decoder, image, object);
    auto reaching_configs =
        reaching::count_ext::callsite::init(decoder, image, object, targets);

#else
    auto liveness_configs = liveness::count::callsite::init(decoder, image, object);
    auto reaching_configs =
        reaching::count::callsite::init(decoder, image, object, targets);
#endif

    std::vector<CallSites> call_sites_vector;

    for (auto index = 0; index < liveness_configs.size(); ++index)
    {
        auto liveness_config = liveness_configs[index];
        auto reaching_config = reaching_configs[index];

        CallSites call_sites;

    std::unordered_map<uint64_t, std::vector<MemoryCallSite>> memory_callsites;

    instrument_object_functions(object, [&](BPatch_function *function) {
        instrument_function_basicBlocks_unordered(function, [&](BPatch_basicBlock
                                                                    *block) {
            instrument_basicBlock_decoded(block, decoder, [&](CADecoder *instr_decoder) {
                // get instruction bytes
                auto const address = instr_decoder->get_address();

                if (instr_decoder->is_indirect_call())
                {
                    auto store_address = instr_decoder->get_address_store_address();

                    LOG_INFO(LOG_FILTER_CALL_SITE,
                             "<CS> basic block [%lx:%lx] instruction %lx",
                             block->getStartAddress(), block->getEndAddress(), address);

                    reaching::reset_state(reaching_config);
                    auto reaching_state =
                        reaching::analysis(reaching_config, block, address);
                    LOG_INFO(LOG_FILTER_CALL_SITE, "\tREGISTER_STATE %s",
                             to_string(reaching_state).c_str());

                    BPatch_Vector<BPatch_basicBlock *> targets;
                    block->getTargets(targets);

                    auto liveness_state = liveness::analysis(liveness_config, targets);
                    LOG_INFO(LOG_FILTER_CALL_SITE, "\tRETURN_REGISTER_STATE %s",
                             to_string(liveness_state).c_str());

                    // is_virtual_callsite(instr_decoder, block);

                    auto parameters = system_v::callsite::generate_paramlist(
                        function, address, reaching_state, liveness_state);

                    // if (store_address != 0)
                    //{
                    //    LOG_INFO(LOG_FILTER_CALL_SITE,
                    //             "<CS*> instruction %lx is using fptr stored at %lx",
                    //             address, store_address);
                    //
                    //    memory_callsites[store_address].emplace_back(
                    //        function, block->getStartAddress(), address, parameters,
                    //        reaching_state);
                    //}
                    // else
                    {
                        call_sites.emplace_back(function, block->getStartAddress(),
                                                address, parameters);
                    }
                }
            });
        });
    });

//    LOG_INFO(LOG_FILTER_CALL_SITE, "fptr storage analysis");

// for (auto &&addr_cs_pairs : memory_callsites)
//{
//    LOG_INFO(LOG_FILTER_CALL_SITE, "%lx -> %s", addr_cs_pairs.first,
//              to_string(addr_cs_pairs.second).c_str());
//
//    std::vector<reaching::RegisterStates> reaching_states;
//    for (auto const &memory_callsite : addr_cs_pairs.second)
//    {
//        reaching_states.push_back(memory_callsite.reaching_state);
//    }
//
//    auto reaching_state = reaching_config.merge_horizontal_rip(reaching_states);
//
//    static const int param_register_list[] = {DR_REG_RDI, DR_REG_RSI, DR_REG_RDX,
//                                              DR_REG_RCX, DR_REG_R8,  DR_REG_R9};
//
//    for (auto const &memory_callsite : addr_cs_pairs.second)
//    {
//        auto parameters = memory_callsite.parameters;
//        for (int index = 0; index < 6; ++index)
//        {
//            parameters[index] = [](reaching::state_t state) {
//                if (!reaching::is_trashed(state))
//                {
//                    if (reaching::is_set_64(state))
//                        return '4';
//                    if (reaching::is_set_32(state))
//                        return '3';
//                    if (reaching::is_set_16(state))
//                        return '2';
//                    if (reaching::is_set_8(state))
//                        return '1';
//                }
//                return '0';
//            }(reaching_state[param_register_list[index]]);
//        }
//
//        call_sites.emplace_back(memory_callsite.function,
//        memory_callsite.block_start,
//                                memory_callsite.address,
//                                memory_callsite.parameters);
//    }
//}

        liveness_config.block_states.clear();
        reaching_config.block_states.clear();

        call_sites_vector.push_back(call_sites);
    }

    return call_sites_vector;
}
