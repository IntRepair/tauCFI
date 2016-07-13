#include "calltargets.h"

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

std::vector<CallTarget> calltarget_analysis(BPatch_image *image, std::vector<BPatch_module *> *mods,
                                            BPatch_addressSpace *as, CADecoder *decoder,
                                            std::vector<TakenAddress> &taken_addresses)
{
    std::vector<CallTarget> call_targets;

    auto liveness_states = liveness_init();
    auto reaching_states = reaching_init();

    for (auto module : *mods)
    {
        char modname[BUFFER_STRING_LEN];

        module->getFullName(modname, BUFFER_STRING_LEN);

        INFO(LOG_FILTER_CALL_TARGET, "Processing module %s", modname);

        // Instrument module
        char funcname[BUFFER_STRING_LEN];
        std::vector<BPatch_function *> *functions = module->getProcedures(true);
        Dyninst::Address start, end;

        for (auto function : *functions)
        {
            // Instrument function
            function->getName(funcname, BUFFER_STRING_LEN);
            function->getAddressRange(start, end);
            if (std::find_if(std::begin(taken_addresses), std::end(taken_addresses),
                             [start](TakenAddress const &taken_addr) { return start == taken_addr.deref_address; }) !=
                std::end(taken_addresses))
            {
                auto params = function->getParams();
                auto ret_type = function->getReturnType();

                INFO(LOG_FILTER_CALL_TARGET, "<CT> Function %lx = (%s) %s", start,
                     ret_type ? ret_type->getName() : "UNKNOWN", funcname);

                if (params)
                {
                    for (auto &&param : *params)
                        INFO(LOG_FILTER_CALL_TARGET, "\t%s (R %d)", param->getType()->getName(), param->getRegister());
                }
                else
                    ERROR(LOG_FILTER_CALL_TARGET, "\tCOULD NOT DETERMINE PARAMS OR NO PARAMS");

                auto liveness_state = liveness_analysis(decoder, as, function, liveness_states);
                INFO(LOG_FILTER_CALL_TARGET, "\tREGISTER_STATE %s", to_string(liveness_state).c_str());

                auto reaching_state = reaching_analysis(decoder, as, function, reaching_states);
                INFO(LOG_FILTER_CALL_TARGET, "\tRETURN_REGISTER_STATE %s", to_string(reaching_state).c_str());

                call_targets.emplace_back(CallTarget{std::string(modname), function, to_string(liveness_state)});
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

        INFO(LOG_FILTER_CALL_TARGET, "<CT> %s: %s %lx -> %lx", target.module_name.c_str(), funcname, start, end);
    }
}
