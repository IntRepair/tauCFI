#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <common/opt/passi.h>

#include "address_taken.h"
#include "ca_decoder.h"
#include "callsites.h"
#include "calltargets.h"
#include "instrumentation.h"
#include "logging.h"
#include "patching.h"
#include "verification.h"

void handler(int sig)
{
    void *array[10];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace(array, 10);

    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
}

using namespace Dyninst::PatchAPI;
using namespace Dyninst::InstructionAPI;
using namespace Dyninst::SymtabAPI;

PASS_ONCE();

namespace
{
class PadynPass : public ModulePass
{

  public:
    static char ID;
    PadynPass() : ModulePass(ID) {}

    virtual bool runOnModule(void *M)
    {
        signal(SIGSEGV, handler); // install our handler

        BPatch_addressSpace *as = (BPatch_addressSpace *)M;

        /* here we go! */
        LOG_INFO(LOG_FILTER_GENERAL, "GetImage()...");
        BPatch_image *image = as->getImage();

        CADecoder decoder;

        instrument_image_objects(image, [&decoder, image](BPatch_object *object) {
            LOG_INFO(LOG_FILTER_GENERAL, "Processing object %s", object->name().c_str());
            if ((object->name() != "binops.u8.u8.u8") && (object->name() != "nginx"))
                return;

            LOG_INFO(LOG_FILTER_GENERAL, "Performing address taken analysis...");
            auto taken_addresses = address_taken_analysis(object, image, &decoder);

            LOG_INFO(LOG_FILTER_GENERAL, "Performing relative callee analysis...");
            auto calltargets = calltarget_analysis(object, image, &decoder, taken_addresses);

            LOG_INFO(LOG_FILTER_GENERAL, "Performing relative callsite analysis...");
            auto callsites = callsite_analysis(object, image, &decoder, calltargets);

            LOG_INFO(LOG_FILTER_GENERAL,
                     "Found %lu taken_addresses, %lu calltargets and %lu callsites",
                     taken_addresses.size(), calltargets.size(), callsites.size());

            LOG_INFO(LOG_FILTER_GENERAL, "Performing binary patching...");
            binary_patching(object, image, &decoder, calltargets, callsites);

            verification::pairing(object, image, taken_addresses, calltargets, callsites);

        });

        LOG_INFO(LOG_FILTER_GENERAL, "Finished Dyninst setup, returning to target");

        return false;
    }
};
}

char PadynPass::ID = 0;
RegisterPass<PadynPass> MP("padyn", "Padyn Pass");
