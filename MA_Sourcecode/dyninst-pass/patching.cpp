#include "patching.h"

#include "BPatch_basicBlock.h"
#include "BPatch_function.h"
#include "BPatch_point.h"
#include "BPatch_process.h"
#include "Snippet.h"
#include "PatchMgr.h"
#include "Command.h"

#include "ca_defines.h"
//#include "dynProcess.h"
#include "instrumentation.h"
#include "logging.h"

#if 0
static const uint8_t nop_buffer[] = { NOP, NOP, NOP, NOP, NOP };

class NopSnippet : public Dyninst::PatchAPI::Snippet
{
    public:

    bool generate(Point* point, Buffer &buffer) override
    {
        fprintf(stderr,"calling generate for NopSnippet\n");
        buffer.copy((void*)nop_buffer, 5);
        return true;
    }

};
#endif

// Create and insert a printf snippet
bool create_and_insert_snippet_callsite(BPatch_image *image, std::vector<BPatch_point *> &points)
{
    auto as = image->getAddressSpace();

    std::vector<BPatch_snippet *> snippet_sequence;

    // Create the printf function call snippet
    std::vector<BPatch_snippet *> printfArgs;
    auto fmt = BPatch_constExpr("the address %lx is called [%lx] = %lx\n");
    printfArgs.push_back(&fmt);
    auto target_addr = BPatch_dynamicTargetExpr();
    printfArgs.push_back(&target_addr);
    auto tag_address = BPatch_arithExpr(BPatch_minus, target_addr, BPatch_constExpr(4));
    printfArgs.push_back(&tag_address);
    auto tag_value = BPatch_arithExpr(BPatch_deref, tag_address);
    printfArgs.push_back(&tag_value);

    // Find the printf function
    std::vector<BPatch_function *> printfFuncs;
    image->findFunction("printf", printfFuncs);
    if (printfFuncs.size() == 0)
    {
        LOG_ERROR(LOG_FILTER_BINARY_PATCHING, "Could not find \"printf\"");
        return false;
    }
    // Construct a function call snippet
    snippet_sequence.push_back(new BPatch_funcCallExpr(*(printfFuncs[0]), printfArgs));

    LOG_INFO(LOG_FILTER_BINARY_PATCHING, "attempting to patch via insertSnippet...");

    // Insert the snippet
    if (!as->insertSnippet(BPatch_sequence(snippet_sequence), points, BPatch_callBefore))
    {
        LOG_ERROR(LOG_FILTER_GENERAL, "insertSnippet: FAIL");
        return false;
    }
    LOG_INFO(LOG_FILTER_GENERAL, "insertSnippet: SUCCESS");
    return true;
}

bool create_and_insert_snippet_calltarget(BPatch_image *image, std::vector<BPatch_point *> &points)
{
}

struct LabelBuilder
{
};

void patch_callsites(BPatch_object *object, BPatch_image *image, CADecoder *decoder,
                     CallSites call_sites, LabelBuilder const &label_builder)
{
    std::vector<BPatch_point *> patch_points;
    for (auto call_site : call_sites)
    {
        auto instr_address = call_site.address;

        std::vector<BPatch_point *> points;
        object->findPoints(instr_address, points);

        for (auto point : points)
            patch_points.push_back(point);
    }

    create_and_insert_snippet_callsite(image, patch_points);
}

void patch_calltargets(BPatch_object *object, BPatch_image *image, CADecoder *decoder,
                       CallTargets call_targets, LabelBuilder const &label_builder)
{
    std::vector<BPatch_point *> patch_points;
    for (auto call_target : call_targets)
    {
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

void binary_patching(BPatch_object *object, BPatch_image *image, CADecoder *decoder,
                     CallTargets call_targets, CallSites call_sites)
{
    if (PATCHING_POLICY == POLICY_NONE)
        return;

    auto label_builder = get_label_for_policy(PATCHING_POLICY);

    patch_callsites(object, image, decoder, call_sites, *label_builder);
    patch_calltargets(object, image, decoder, call_targets, *label_builder);

    auto address_space = image->getAddressSpace();
#if 0
    auto nop_snippet = NopSnippet::create(new NopSnippet());

    auto patch_mgr = Dyninst::PatchAPI::convert(address_space);
    Dyninst::PatchAPI::Patcher patcher(patch_mgr);

    instrument_object_functions(object, [&](BPatch_function *function) {
        std::vector <BPatch_point*> points;
        function->getEntryPoints(points);

        char funcname[BUFFER_STRING_LEN];
        function->getName(funcname, BUFFER_STRING_LEN);

        fprintf(stderr,"looking at function %s\n", funcname);

        for (auto point : points)
        {
            fprintf(stderr," patching point for function %s\n", funcname);
            auto patch_point = Dyninst::PatchAPI::convert(point, BPatch_callBefore);
            patcher.add(Dyninst::PatchAPI::PushBackCommand::create(patch_point, nop_snippet));
        }
    });

    patcher.commit();
#endif
    fprintf(stderr, "commited patch\n");

//    auto label_builder = get_label_for_policy(PATCHING_POLICY);
//    patch_callsites(image, decoder, call_sites, *label_builder);

#if 0
    auto process = dynamic_cast<BPatch_process *>(address_space);
    auto llProcess = process->lowlevel_process();

    //instrument_object_functions(object, [&](BPatch_function *function) {
    //    Dyninst::Address start, end;
    //    function->getAddressRange(start, end);
    //    uint8_t buffer[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    //    llProcess->writeTextSpace((void *)(start - 2), 2, buffer);
    //});
#endif
}
