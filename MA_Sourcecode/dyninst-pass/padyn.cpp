#include <common/opt/passi.h>

#include "address_taken.h"
#include "ca_decoder.h"
#include "ca_decoder_dynamo_rio.h"
#include "calltargets.h"
#include "relative_callsites.h"
#include "utils.h"

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
    PadynPass() : ModulePass(ID)
    {
    }

    virtual bool runOnModule(void *M)
    {
        BPatch_addressSpace *as = (BPatch_addressSpace *)M;

        /* here we go! */
        LOG_INFO("GetImage()...\n");
        BPatch_image *image = as->getImage();

        LOG_INFO("GetModules()...\n");
        std::vector<BPatch_module *> *mods = image->getModules();

        CADecoder *decoder = new CADecoderDynamoRIO();

        LOG_INFO("Performing address taken analysis...\n");
        auto taken_addresses = address_taken_analysis(image, mods, as, decoder);

        LOG_INFO("Performing relative callee analysis...\n");
        auto calltargets = calltarget_analysis(image, mods, as, decoder, taken_addresses);

        LOG_INFO("Performing relative callsite analysis...\n");
        auto callsites = relative_callsite_analysis(image, mods, as, decoder);

        delete decoder;

        LOG_INFO("Finished Dyninst setup, returning to target\n\n");

        return false;
    }
};
}

char PadynPass::ID = 0;
RegisterPass<PadynPass> MP("padyn", "Padyn Pass");
