#include "relative_callsites.h"

#include "liveness_analysis.h"
#include "reaching_analysis.h"

#include "logging.h"

#include <BPatch.h>
#include <BPatch_addressSpace.h>
#include <BPatch_binaryEdit.h>
#include <BPatch_flowGraph.h>
#include <BPatch_function.h>

#include <AddrLookup.h>
#include <PatchCFG.h>
#include <Symtab.h>

using namespace Dyninst::PatchAPI;
using namespace Dyninst::InstructionAPI;
using namespace Dyninst::SymtabAPI;

std::vector<CallSite> relative_callsite_analysis(BPatch_image *image, std::vector<BPatch_module *> *mods,
                                                 BPatch_addressSpace *as, CADecoder *decoder)
{
    std::vector<CallSite> call_sites;

    auto liveness_states = liveness_init();
    auto reaching_states = reaching_init();

    for (auto module : *mods)
    {
        char modname[BUFFER_STRING_LEN];
        module->getFullName(modname, BUFFER_STRING_LEN);

        if (module->isSharedLib())
        {
            WARNING(LOG_FILTER_CALL_SITE, "Skipping shared library %s", modname);
        }
        else
        {
            INFO(LOG_FILTER_CALL_SITE, "Processing module %s", modname);
            // Instrument module
            std::vector<BPatch_function *> *functions = module->getProcedures(true);

            char funcname[BUFFER_STRING_LEN];
            Dyninst::Address start, end;

            for (auto function : *functions)
            {
                // Instrument function
                function->getName(funcname, BUFFER_STRING_LEN);
                function->getAddressRange(start, end);
                INFO(LOG_FILTER_CALL_SITE, "Processing function [%lx:%lx] %s", start, end, funcname);

                std::set<BPatch_basicBlock *> blocks;
                BPatch_flowGraph *cfg = function->getCFG();
                cfg->getAllBasicBlocks(blocks);

                for (auto block : blocks)
                {
                    // Instrument BasicBlock
                    DEBUG(LOG_FILTER_CALL_SITE, "Processing basic block [%lx:%lx]", block->getStartAddress(),
                          block->getEndAddress());
                    // iterate backwards (PatchAPI restriction)
                    PatchBlock::Insns insns;
                    Dyninst::PatchAPI::convert(block)->getInsns(insns);

                    for (auto &instruction : insns)
                    {
                        // get instruction bytes
                        auto address = reinterpret_cast<uint64_t>(instruction.first);
                        Instruction::Ptr instruction_ptr = instruction.second;

                        decoder->decode(address, instruction_ptr);
                        DEBUG(LOG_FILTER_CALL_SITE, "Processing instruction %lx", address);

                        if (decoder->is_indirect_call())
                        {
                            INFO(LOG_FILTER_CALL_SITE, "Callsite @ basic block [%lx:%lx] Instr %lx",
                                  block->getStartAddress(), block->getEndAddress(), address);

                            auto reaching_state = reaching_analysis(decoder, as, block, address, reaching_states);
                            INFO(LOG_FILTER_CALL_SITE, "\tREGISTER_STATE %s", to_string(reaching_state).c_str());

                            BPatch_Vector<BPatch_basicBlock *> targets;
                            block->getTargets(targets);

                            auto liveness_state = liveness_analysis(decoder, as, targets, liveness_states);
                            INFO(LOG_FILTER_CALL_SITE, "\tRETURN_REGISTER_STATE %s", to_string(liveness_state).c_str());

                            call_sites.emplace_back(CallSite{std::string(modname), start, block->getStartAddress(),
                                                             address, to_string(reaching_state)});
                        }
                    }
                }
            }
        }
    }

    return call_sites;
}

void dump_call_sites(std::vector<CallSite> &call_sites)
{
    for (auto &call_site : call_sites)
        INFO(LOG_FILTER_CALL_SITE, "<CS>%s:%lx:%lx:%lx", call_site.module_name.c_str(), call_site.function_start,
             call_site.block_start, call_site.address);
}
