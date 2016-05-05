#include "calltargets.h"
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

std::vector<CallTarget> calltarget_analysis(BPatch_image *image, std::vector<BPatch_module *> *mods,
                                            BPatch_addressSpace *as, CADecoder *decoder,
                                            std::vector<TakenAddress> &taken_addresses)
{
    std::vector<CallTarget> call_targets;

    for (auto module : *mods)
    {
        char modname[BUFFER_STRING_LEN];

        module->getFullName(modname, BUFFER_STRING_LEN);

        LOG_INFO("Processing module %s\n", modname);

        // Instrument module
        char funcname[BUFFER_STRING_LEN];
        std::vector<BPatch_function *> *functions = module->getProcedures(true);
        Dyninst::Address start, end;

        for (auto function : *functions)
        {
            // Instrument function
            function->getName(funcname, BUFFER_STRING_LEN);
            function->getAddressRange(start, end);
            LOG_INFO("Processing function %s\n", funcname);

            if (std::find_if(std::begin(taken_addresses), std::end(taken_addresses),
                             [start](TakenAddress const &taken_addr) { return start == taken_addr.deref_address; }) !=
                std::end(taken_addresses))
            {
                LOG_INFO("<CT> Function %lx = %s\n", start, funcname);
                call_targets.emplace_back(CallTarget{std::string(modname), function});
            }
        }
    }

    return call_targets;
}

void dump_calltargets(std::vector<CallTarget> &targets)
{
    char funcname[BUFFER_STRING_LEN];
    Dyninst::Address start, end;

    for (auto &target : targets)
    {
        target.function->getName(funcname, BUFFER_STRING_LEN);
        target.function->getAddressRange(start, end);

        LOG_INFO("<CT> %s: %s %lx -> %lx\n", target.module_name.c_str(), funcname, start, end);
    }
}
