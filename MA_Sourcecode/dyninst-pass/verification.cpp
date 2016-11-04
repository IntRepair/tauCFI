#include "verification.h"

#include <fstream>

namespace verification
{

#if (not defined(__PADYN_COUNT_EXT_POLICY)) && (not (defined (__PADYN_TYPE_POLICY)))
void pairing(BPatch_object *object, BPatch_image *image, TakenAddresses const &ats,
             CallTargets const &cts, CallSites const &css)
#else
void pairing(BPatch_object *object, BPatch_image *image, TakenAddresses const &ats,
             std::vector<CallTargets> const &ctss, std::vector<CallSites> const &csss)
#endif
{
#ifdef __PADYN_TYPE_POLICY
    static const std::array<std::string, 6> base_strings {
        "verify.type_exp1",
        "verify.type_exp2",
        "verify.type_exp3",
        "verify.type_exp4",
    };

    for (auto index = 0; index <  ctss.size(); ++index)
    {
        auto base_string = base_strings[index];
        auto cts = ctss[index];
        auto css = csss[index % csss.size()];

#elif defined(__PADYN_COUNT_EXT_POLICY)

    static const std::array<std::string, 4> base_strings {
        //"verify.sources_destr_no_follow.",
        //"verify.sources_destr_follow.",
        "verify.sources_inter_no_follow.",
        "verify.sources_inter_follow.",
        "verify.sources_union_no_follow.",
        "verify.sources_union_follow.",
    };

    for (auto index = 0; index <  ctss.size(); ++index)
    {
        auto base_string = base_strings[index];
        auto cts = ctss[index];
        auto css = csss[index];

#else
    std::string base_string("verify.");
#endif

    std::ofstream at_file(base_string + object->name() + std::string(".at"));
    std::ofstream at_file_debug(base_string + object->name() + std::string(".at.debug"));
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
    auto funcname = [&]() {
        auto typed_name = functions[0]->getTypedName();
        if (typed_name.empty())
            return functions[0]->getName();
        return typed_name;
    }();
    
            at_file << "<AT>;" << funcname << ";" << int_to_hex(fn_addr) << "\n";
        }
    }

    std::ofstream cs_file(base_string + object->name() + std::string(".cs"));
    cs_file << to_string(css);

    std::ofstream ct_file(base_string + object->name() + std::string(".ct"));
    ct_file << to_string(cts);

#if defined(__PADYN_COUNT_EXT_POLICY) || defined(__PADYN_TYPE_POLICY)
    }
#endif

}
};