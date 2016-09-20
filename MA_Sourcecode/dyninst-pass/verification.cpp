#include "verification.h"

#include <fstream>

namespace verification
{
void pairing(BPatch_object *object, BPatch_image *image, TakenAddresses const &ats,
             CallTargets const &cts, CallSites const &css)
{
    std::ofstream at_file(std::string("verify.") + object->name() + std::string(".at"));
    std::ofstream at_file_debug(std::string("verify.") + object->name() + std::string(".at.debug"));
    for (auto const &at : ats)
    {
        at_file_debug << to_string(at.second);

        uint64_t const fn_addr = at.first;

        std::vector<BPatch_function *> functions;
        image->findFunction(fn_addr, functions);

        if (functions.size() != 1)
        {
            // TODO: Print ERROR HERE
        }
        else
        {
            char funcname[BUFFER_STRING_LEN];
            functions[0]->getName(funcname, BUFFER_STRING_LEN);

            at_file << "<AT> " << std::string(funcname) << " " << int_to_hex(fn_addr) << "\n";
        }
    }

    std::ofstream cs_file(std::string("verify.") + object->name() + std::string(".cs"));
    cs_file << to_string(css);

    std::ofstream ct_file(std::string("verify.") + object->name() + std::string(".ct"));
    ct_file << to_string(cts);
}
};