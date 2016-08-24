#include "patching.h"

#include "BPatch_basicBlock.h"
#include "BPatch_function.h"
#include "BPatch_point.h"

// Create and insert a printf snippet
bool create_and_insert_snippet_callsite(BPatch_image *image, std::vector<BPatch_point *> &points)
{
    auto as = image->getAddressSpace();

    std::vector<BPatch_snippet *> snippet_sequence;

    // Create the printf function call snippet
    std::vector<BPatch_snippet *> printfArgs;
    BPatch_snippet *fmt = new BPatch_constExpr("the address %lx is called\n");
    printfArgs.push_back(fmt);

    BPatch_snippet *target_addr = new BPatch_dynamicTargetExpr();
    printfArgs.push_back(target_addr);

    // Find the printf function
    std::vector<BPatch_function *> printfFuncs;
    image->findFunction("printf", printfFuncs);
    if (printfFuncs.size() == 0)
    {
        ERROR(LOG_FILTER_BINARY_PATCHING, "Could not find \"printf\"");
        return false;
    }
    // Construct a function call snippet
    snippet_sequence.push_back(new BPatch_funcCallExpr(*(printfFuncs[0]), printfArgs));

    std::vector<BPatch_snippet *> printfArgs_check;
    fmt = new BPatch_constExpr("Status of address %lx is <UNKNOWN> ");
    printfArgs_check.push_back(fmt);
    BPatch_funcCallExpr success_message(*(printfFuncs[0]), printfArgs_check);

    INFO(LOG_FILTER_BINARY_PATCHING, "attempting to patch via insertSnippet...");

    // Insert the snippet
    if (!as->insertSnippet(BPatch_sequence(snippet_sequence), points, BPatch_callBefore))
    {
        ERROR(LOG_FILTER_BINARY_PATCHING, "insertSnippet: FAIL");
        return false;
    }
    INFO(LOG_FILTER_BINARY_PATCHING, "insertSnippet: SUCCESS");
    return true;
}

bool create_and_insert_snippet_calltarget(BPatch_image *image, std::vector<BPatch_point *> &points)
{
}

struct LabelBuilder
{
};

void patch_callsites(BPatch_image *image, CADecoder *decoder, CallSites call_sites,
                     LabelBuilder const &label_builder)
{
    std::vector<BPatch_point *> patch_points;
    for (auto call_site : call_sites)
    {
        auto object = call_site.object;
        auto instr_address = call_site.address;

        std::vector<BPatch_point *> points;
        object->findPoints(instr_address, points);

        for (auto point : points)
            patch_points.push_back(point);
    }

    create_and_insert_snippet_callsite(image, patch_points);
}

void patch_calltargets(BPatch_image *image, CADecoder *decoder, CallTargets call_targets,
                  LabelBuilder const &label_builder)
{
    std::vector<BPatch_point *> patch_points;
    for (auto call_target : call_targets)
    {
        auto object = call_target.object;
        auto function = call_target.function;

        Dyninst::Address start, end;
        function->getAddressRange(start, end);

        std::vector<BPatch_point *> points;
        object->findPoints(start, points);

        for (auto point : points)
            patch_points.push_back(point);
    }

    create_and_insert_snippet_calltarget(image, patch_points);
}

std::unique_ptr<LabelBuilder> get_label_for_policy(PatchingPolicy policy)
{
    return std::unique_ptr<LabelBuilder>(new LabelBuilder());
}

void binary_patching(BPatch_image *image, CADecoder *decoder, CallTargets call_targets,
                     CallSites call_sites)
{
    if (PATCHING_POLICY == POLICY_NONE)
        return;

    auto label_builder = get_label_for_policy(PATCHING_POLICY);

    patch_callsites(image, decoder, call_sites, *label_builder);
    patch_calltargets(image, decoder, call_targets, *label_builder);
}
