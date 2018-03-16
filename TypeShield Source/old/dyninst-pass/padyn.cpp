#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <vector>

#include <BPatch.h>
#include <BPatch_image.h>
#include <BPatch_module.h>
#include <BPatch_object.h>

#include <common/opt/passi.h>

#include "ast/ast.h"
#include "ca_decoder.h"
#include "callsites.h"
#include "calltargets.h"
#include "instrumentation.h"
#include "logging.h"
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

        auto const is_system = [](BPatch_object *object) {
            std::vector<BPatch_module *> modules;
            object->modules(modules);
            if (modules.size() != 1)
                return false;
            return modules[0]->isSystemLib();
        };

        auto const is_excluded_library = [](BPatch_object *object) {
            const std::array<std::string, 16> excluded_libraries{
                "libdyninstAPI_RT.so", "libpthread.so", "libz.so",    "libdl.so",
                "libcrypt.so",         "libnsl.so",     "libm.so",    "libstdc++.so",
                "libgcc_s.so",         "libc.so",       "libpcre.so", "libcrypto.so",
                "libcap.so",           "libpthread-",   "libdl-",     "libc-"};

            auto const library_name = object->name();
            for (auto const &exclude : excluded_libraries)
            {
                if (exclude.length() <= library_name.length())
                    if (std::equal(exclude.begin(), exclude.end(), library_name.begin()))
                        return true;
            }
            return false;
        };

        std::vector<BPatch_object *> objects;
        image->getObjects(objects);
        for (auto object : objects)
        {
            LOG_TRACE(LOG_FILTER_GENERAL, "Processing object %s", object->name().c_str());

            if (!(is_system(object) || is_excluded_library(object)))
            {
                LOG_INFO(LOG_FILTER_GENERAL, "Processing object %s",
                         object->name().c_str());

                auto const ast = ast::ast::create(object, &decoder);

                LOG_INFO(LOG_FILTER_GENERAL, "Performing relative callee analysis...");
                auto calltargets = calltarget_analysis(ast, &decoder);

                LOG_INFO(LOG_FILTER_GENERAL, "Performing relative callsite analysis...");
                auto callsites = callsite_analysis(ast, &decoder, calltargets);

                auto taken_address_count =
                    util::count_if(ast.get_functions(), [](ast::function const &fn) {
                        return fn.get_is_at();
                    });

                LOG_INFO(LOG_FILTER_GENERAL,
                         "Found %lu taken_addresses, %lu calltargets and %lu callsites",
                         taken_address_count, calltargets.size(), callsites.size());

                verification::pairing(ast, calltargets, callsites);
            }
        }

        LOG_INFO(LOG_FILTER_GENERAL, "Finished Dyninst setup, returning to target");

        return true;
    }
};
}

char PadynPass::ID = 0;
RegisterPass<PadynPass> MP("padyn", "Padyn Pass");
