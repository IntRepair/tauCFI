#include <common/opt/passi.h>

#include "address_taken.h"
#include "ca_decoder.h"
#include "calltargets.h"
#include "logging.h"
#include "relative_callsites.h"

#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
        INFO(LOG_FILTER_GENERAL, "GetImage()...");
        BPatch_image *image = as->getImage();

        INFO(LOG_FILTER_GENERAL, "GetModules()...");
        std::vector<BPatch_module *> *mods = image->getModules();

        CADecoder decoder;

        INFO(LOG_FILTER_GENERAL, "Performing address taken analysis...");
        auto taken_addresses = address_taken_analysis(image, mods, as, &decoder);

        INFO(LOG_FILTER_GENERAL, "Performing relative callee analysis...");
        auto calltargets = calltarget_analysis(image, mods, as, &decoder, taken_addresses);

        INFO(LOG_FILTER_GENERAL, "Performing relative callsite analysis...");
        auto callsites = relative_callsite_analysis(image, mods, as, &decoder);

        INFO(LOG_FILTER_GENERAL, "Finished Dyninst setup, returning to target");

        return false;
    }
};
}

char PadynPass::ID = 0;
RegisterPass<PadynPass> MP("padyn", "Padyn Pass");
