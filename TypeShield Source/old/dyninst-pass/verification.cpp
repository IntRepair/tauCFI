#include "verification.h"

#include <fstream>

#include <BPatch_function.h>

namespace verification
{

void pairing(ast::ast const &ast, std::vector<CallTargets> const &ctss,
             std::vector<CallSites> const &csss)
{
    std::ofstream at_file(std::string("verify.") + ast.get_name() + std::string(".at"));
    for (auto const &ast_function : ast.get_functions())
    {
        if (ast_function.get_is_at())
        {
            at_file << "<AT>;" << ast_function.get_name() << ";"
                    << int_to_hex(ast_function.get_address()) << "\n";
        }
    }

#ifdef __PADYN_TYPE_POLICY
    static const std::array<std::string, 3> base_strings{
        "verify.type_exp5.", "verify.type_exp6.", "verify.type_exp7.",
        //"verify.type_exp8.",
    };

#elif defined(__PADYN_COUNT_EXT_POLICY)

    static const std::array<std::string, 4> base_strings{
        //"verify.sources_destr_no_follow.",
        //"verify.sources_destr_follow.",
        "verify.sources_inter_no_follow.", "verify.sources_inter_follow.",
        "verify.sources_union_no_follow.", "verify.sources_union_follow.",
    };

#else
    static const std::array<std::string, 2> base_strings{
        "verify.count_prec", "verify.count_safe",
    };

#endif

    for (auto index = 0; index < ctss.size(); ++index)
    {
        auto base_string = base_strings[index];
        auto cts = ctss[index];
        auto css = csss[index];

        std::ofstream cs_file(base_string + ast.get_name() + std::string(".cs"));
        cs_file << to_string(css);

        std::ofstream ct_file(base_string + ast.get_name() + std::string(".ct"));
        ct_file << to_string(cts);
    }
}
};