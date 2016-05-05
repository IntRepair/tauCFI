#include "relative_callsites.h"

#include "utils.h"

#include <boost/range/adaptor/reversed.hpp>

#include <BPatch.h>
#include <BPatch_addressSpace.h>
#include <BPatch_binaryEdit.h>
#include <BPatch_flowGraph.h>
#include <BPatch_function.h>
#include <BPatch_object.h>
#include <BPatch_point.h>
#include <BPatch_process.h>

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

    for (auto module : *mods)
    {
        char modname[BUFFER_STRING_LEN];
        module->getFullName(modname, BUFFER_STRING_LEN);

        if (module->isSharedLib())
        {
            LOG_WARNING("Skipping shared library %s\n", modname);
        }
        else
        {
            LOG_INFO("Processing module %s\n", modname);
            // Instrument module
            std::vector<BPatch_function *> *functions = module->getProcedures(true);

            char funcname[BUFFER_STRING_LEN];
            Dyninst::Address start, end;

            for (auto function : *functions)
            {
                // Instrument function
                function->getName(funcname, BUFFER_STRING_LEN);
                function->getAddressRange(start, end);
                LOG_INFO("Processing function [%lx:%lx] %s\n", start, end, funcname);

                std::set<BPatch_basicBlock *> blocks;
                BPatch_flowGraph *cfg = function->getCFG();
                cfg->getAllBasicBlocks(blocks);

                for (auto block : boost::adaptors::reverse(blocks))
                {
                    // Instrument BasicBlock
                    LOG_DEBUG("Processing basic block %lx\n", block->getStartAddress());
                    // iterate backwards (PatchAPI restriction)
                    PatchBlock::Insns insns;
                    Dyninst::PatchAPI::convert(block)->getInsns(insns);

                    for (auto &instruction : boost::adaptors::reverse(insns))
                    {
                        // get instruction bytes
                        auto address = reinterpret_cast<uint64_t>(instruction.first);
                        Instruction::Ptr instruction_ptr = instruction.second;

                        decoder->decode(address, instruction_ptr);
                        LOG_DEBUG("Processing instruction %lx\n", address);

                        if (decoder->isCall_indirect())
                        {
                            LOG_INFO("<CS>%lx:%lx\n", block->getStartAddress(), address);
                            call_sites.emplace_back(
                                CallSite{std::string(modname), start, block->getStartAddress(), address});
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
        LOG_INFO("<CS>%s:%lx:%lx:%lx\n", call_site.module_name.c_str(), call_site.function_start, call_site.block_start,
             call_site.address);
}
