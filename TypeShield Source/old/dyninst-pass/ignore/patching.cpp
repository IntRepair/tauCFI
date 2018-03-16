#include "patching.h"

#include "BPatch_basicBlock.h"
#include "BPatch_function.h"
#include "BPatch_point.h"
#include "BPatch_process.h"
#include "BPatch_snippet.h"
#include "Command.h"
#include "Instrumenter.h"
#include "PatchMgr.h"
#include "PatchMgr.h"
#include "Snippet.h"
#include "Snippet.h"
#include "dynC.h"

#include "ca_defines.h"
#include "instrumentation.h"
#include "logging.h"

#include <unordered_set>

#define ENABLE_DEBUG_OUTPUT 0

using Dyninst::PatchAPI::Patcher;
using Dyninst::PatchAPI::PushBackCommand;

// For now this is the offset, as it is unlikely that something is outside of a disp32
// range ...
static const uint32_t ANNOTATION_OFFSET = 5;

// Create and insert a printf snippet
bool create_and_insert_snippet_callsite(BPatch_image *image,
                                        std::vector<BPatch_point *> &points,
                                        std::vector<char> label, uint32_t address,
                                        Patcher &patcher)
{
    auto as = image->getAddressSpace();
    // auto snippet_ptr = Dyninst::PatchAPI::Snippet::create(new CallsiteCheck(image,
    // ANNOTATION_OFFSET, label, address));
    // Insert the snippet


#if ENABLE_DEBUG_OUTPUT == 1
    {
        std::vector<BPatch_function *> print_func;
        image->findFunction("printf", print_func);
        if (print_func.size() < 1)
        {
            LOG_ERROR(LOG_FILTER_GENERAL, "COULD NOT FIND PRINTF");
            return false;
        }
        
        auto snippet_target = BPatch_dynamicTargetExpr();
        std::vector<BPatch_snippet *> args;
        auto pattern = BPatch_constExpr("Calling Address %lx\n");
        args.push_back(&pattern);
        args.push_back(&snippet_target);

        auto snippet_call = BPatch_funcCallExpr(*print_func[0], args);

        for (auto const &point : points)
        {
            patcher.add(PushBackCommand::create(
                Dyninst::PatchAPI::convert(point, BPatch_callBefore),
                Dyninst::PatchAPI::convert(&snippet_call)));
        }
    }
#endif

    auto snippet_target = BPatch_dynamicTargetExpr();
    auto snippet_off = BPatch_constExpr(ANNOTATION_OFFSET);
    auto snippet_target_off = BPatch_arithExpr(BPatch_plus, snippet_off, snippet_target);
    auto snippet = new BPatch_arithExpr(BPatch_deref, snippet_target_off);

    for (auto const &point : points)
    {
        patcher.add(
            PushBackCommand::create(Dyninst::PatchAPI::convert(point, BPatch_callBefore),
                                    Dyninst::PatchAPI::convert(snippet)));
    }

    // LOG_DEBUG(LOG_FILTER_BINARY_PATCHING, "insertSnippet: SUCCESS");
    return true;
}

class LabelBuilder
{
  public:
    virtual std::vector<char> generate_label(CallSite &) const = 0;
    virtual std::vector<char> generate_label(CallTarget &) const = 0;
};

class AddressTakenLabelBuilder : public LabelBuilder
{
  private:
    std::unordered_set<uint64_t> at_addresses;

  protected:
    template <typename struct_has_function>
    bool is_address_taken(struct_has_function const &data) const
    {
        std::vector<char> label;
        auto function = data.function;

        Dyninst::Address start, end;
        function->getAddressRange(start, end);

        return (at_addresses.count(start) > 0);
    }

  public:
    AddressTakenLabelBuilder(TakenAddresses const &taken_addresses)
    {
        for (auto at : taken_addresses)
            at_addresses.insert(at.first);
    }

    virtual std::vector<char> generate_label(CallSite &) const override
    {
        std::vector<char> label(4);
        label[3] = 0x3F;
        label[2] = 0x3F;
        label[1] = 0x3F;
        label[0] = 0x3F;
        return label;
    }

    virtual std::vector<char> generate_label(CallTarget &call_target) const override
    {

        std::vector<char> label(4);

        if (is_address_taken(call_target))
        {
            label[3] = 0x3F;
            label[2] = 0x3F;
            label[1] = 0x3F;
            label[0] = 0x3F;
        }

        return label;
    }
};

class ParamCountLabelBuilder : public AddressTakenLabelBuilder
{
  protected:
    template <int min, int max, typename struct_has_parameters, typename function_t>
    char param_to_bitmask(struct_has_parameters const &data, function_t pred) const
    {
        unsigned char bitmask = 0;

        auto const &parameters = data.parameters;
        for (int i = min; i < max; ++i)
        {
            if (pred(parameters[i]))
                bitmask |= (0x1 << i);
        }
        return static_cast<char>(bitmask);
    }

  public:
    ParamCountLabelBuilder(TakenAddresses const &taken_addresses)
        : AddressTakenLabelBuilder(taken_addresses)
    {
    }

    virtual std::vector<char> generate_label(CallSite &call_site) const override
    {
        std::vector<char> label(4);
        label[1] = param_to_bitmask<0, 6>(call_site, [](char val) { return val == 'x'; });
        label[0] = (0x1 << 6) - 1;
        return label;
    }

    virtual std::vector<char> generate_label(CallTarget &call_target) const override
    {
        std::vector<char> label(4);

        if (is_address_taken(call_target))
        {
            auto bitmask =
                param_to_bitmask<0, 6>(call_target, [](char val) { return val == 'x'; });

            label[0] = ((0x1 << 6) - 1) ^ bitmask;
            label[1] = bitmask;
        }
        return label;
    }
};

class ParamCountRetLabelBuilder : public ParamCountLabelBuilder
{
  public:
    ParamCountRetLabelBuilder(TakenAddresses const &taken_addresses)
        : ParamCountLabelBuilder(taken_addresses)
    {
    }

    virtual std::vector<char> generate_label(CallSite &call_site) const override
    {
        std::vector<char> label = ParamCountLabelBuilder::generate_label(call_site);

        auto ret_bitmask =
            param_to_bitmask<6, 7>(call_site, [](char val) { return val == 'x'; });

        label[0] |= (0x1 << 6) ^ ret_bitmask;
        label[1] |= (0x1 << 6);
        return label;
    }

    virtual std::vector<char> generate_label(CallTarget &call_target) const override
    {
        std::vector<char> label = ParamCountLabelBuilder::generate_label(call_target);

        if (is_address_taken(call_target))
        {
            auto ret_bitmask =
                param_to_bitmask<6, 7>(call_target, [](char val) { return val == 'x'; });

            label[0] |= (0x1 << 6);
            label[1] |= ret_bitmask;
        }
        return label;
    }
};

class ParamTypeLabelBuilder : public ParamCountLabelBuilder
{
  protected:
    template <int min, int max, typename struct_has_parameters>
    void apply_provided(std::vector<char> &label, struct_has_parameters const &data) const
    {
        /* 8 byte width */
        auto width_8byte = [](char val) { return (val == '8'); };
        label[4] |= param_to_bitmask<min, max>(data, width_8byte);
        /* 4 byte width */
        auto width_4byte = [&](char val) { return (val == '4') || width_8byte(val); };
        label[3] |= param_to_bitmask<min, max>(data, width_4byte);
        /* 2 byte width */
        auto width_2byte = [&](char val) { return (val == '2') || width_4byte(val); };
        label[2] |= param_to_bitmask<min, max>(data, width_2byte);
        /* 1 byte width */
        auto width_1byte = [&](char val) { return (val == '1') || width_2byte(val); };
        label[1] |= param_to_bitmask<min, max>(data, width_1byte);
        /* 0 byte width */
        auto width_0byte = [&](char val) { return (val == '0') || width_1byte(val); };
        label[0] |= param_to_bitmask<min, max>(data, width_0byte);
    }

    template <int min, int max, typename struct_has_parameters>
    void apply_required(std::vector<char> &label, struct_has_parameters const &data) const
    {
        /* 0 byte width */
        auto width_0byte = [](char val) { return (val == '0'); };
        label[0] = param_to_bitmask<min, max>(data, width_0byte);
        /* 1 byte width */
        auto width_1byte = [&](char val) { return (val == '1') || width_0byte(val); };
        label[1] = param_to_bitmask<min, max>(data, width_1byte);
        /* 2 byte width */
        auto width_2byte = [&](char val) { return (val == '2') || width_1byte(val); };
        label[2] = param_to_bitmask<min, max>(data, width_2byte);
        /* 4 byte width */
        auto width_4byte = [&](char val) { return (val == '4') || width_2byte(val); };
        label[3] = param_to_bitmask<min, max>(data, width_4byte);
        /* 8 byte width */
        auto width_8byte = [&](char val) { return (val == '8') || width_4byte(val); };
        label[4] = param_to_bitmask<min, max>(data, width_8byte);
    }

  public:
    ParamTypeLabelBuilder(TakenAddresses const &taken_addresses)
        : ParamCountLabelBuilder(taken_addresses)
    {
    }

    virtual std::vector<char> generate_label(CallSite &call_site) const override
    {
        std::vector<char> label(5);
        apply_provided<0, 6>(label, call_site);
        return label;
    }

    virtual std::vector<char> generate_label(CallTarget &call_target) const override
    {
        std::vector<char> label(5);

        if (is_address_taken(call_target))
        {
            apply_required<0, 6>(label, call_target);
        }
        return label;
    }
};

class ParamTypeRetLabelBuilder : public ParamTypeLabelBuilder
{
  public:
    ParamTypeRetLabelBuilder(TakenAddresses const &taken_addresses)
        : ParamTypeLabelBuilder(taken_addresses)
    {
    }

    virtual std::vector<char> generate_label(CallSite &call_site) const override
    {
        std::vector<char> label(5);
        apply_required<6, 7>(label, call_site);
        return label;
    }

    virtual std::vector<char> generate_label(CallTarget &call_target) const override
    {
        std::vector<char> label(5);

        if (is_address_taken(call_target))
        {
            apply_provided<6, 7>(label, call_target);
        }
        return label;
    }
};

void patch_callsites(BPatch_object *object, BPatch_image *image, CADecoder *decoder,
                     CallSites call_sites, LabelBuilder const &label_builder)
{

    auto as = image->getAddressSpace();
    auto mgr = Dyninst::PatchAPI::convert(as);
    Dyninst::PatchAPI::Patcher patcher(mgr);

    for (auto call_site : call_sites)
    {
        LOG_INFO(LOG_FILTER_BINARY_PATCHING, "Patching CallSite %s",
                 to_string(call_site).c_str());
        std::vector<BPatch_point *> patch_points;
        auto instr_address = call_site.address;

        std::vector<BPatch_point *> points;
        object->findPoints(instr_address, points);

        for (auto point : points)
            patch_points.push_back(point);

        auto label = label_builder.generate_label(call_site);
        create_and_insert_snippet_callsite(image, patch_points, label, call_site.address,
                                           patcher);
    }
    patcher.commit();
}

void patch_calltargets(BPatch_object *object, BPatch_image *image, CADecoder *decoder,
                       CallTargets call_targets, LabelBuilder const &label_builder)
{
    std::vector<BPatch_point *> patch_points;
    for (auto call_target : call_targets)
    {
        // if (!call_target.plt)
        {
            LOG_INFO(LOG_FILTER_BINARY_PATCHING, "Patching CallTarget %s",
                     to_string(call_target).c_str());

            auto function = call_target.function;
            function->annotateFunction(label_builder.generate_label(call_target));
        }
    }
}

std::unique_ptr<LabelBuilder> get_label_for_policy(PatchingPolicy policy,
                                                   TakenAddresses const &taken_addresses)
{
    switch (policy)
    {
    case POLICY_ADDRESS_TAKEN:
        return std::unique_ptr<LabelBuilder>(
            new AddressTakenLabelBuilder(taken_addresses));
    case POLICY_PARAM_COUNT:
        return std::unique_ptr<LabelBuilder>(new ParamCountLabelBuilder(taken_addresses));
    case POLICY_PARAM_COUNT_EXT:
        return std::unique_ptr<LabelBuilder>(
            new ParamCountRetLabelBuilder(taken_addresses));
    case POLICY_PARAM_TYPE:
        return std::unique_ptr<LabelBuilder>(new ParamTypeLabelBuilder(taken_addresses));
    case POLICY_PARAM_TYPE_EXT:
        return std::unique_ptr<LabelBuilder>(
            new ParamTypeRetLabelBuilder(taken_addresses));
    case POLICY_NONE:
    default:
        assert(false); // not implemented label
    }
}

static std::vector<BPatch_basicBlock *> getEntryBasicBlocks(BPatch_function *function)
{
    BPatch_flowGraph *cfg = function->getCFG();
    std::vector<BPatch_basicBlock *> entry_blocks;
    if (!cfg->getEntryBasicBlock(entry_blocks))
    {
        char funcname[BUFFER_STRING_LEN];
        function->getName(funcname, BUFFER_STRING_LEN);
        LOG_FATAL(LOG_FILTER_BINARY_PATCHING,
                  "Could not find entry blocks for function %s", funcname);
    }
    return entry_blocks;
}

void binary_patching(BPatch_object *object, BPatch_image *image, CADecoder *decoder,
                     CallTargets const &call_targets, CallSites const &call_sites,
                     TakenAddresses const &taken_addresses)
{
    auto const is_system = [&]() {
        std::vector<BPatch_module *> modules;
        object->modules(modules);
        if (modules.size() != 1)
            return false;
        return modules[0]->isSystemLib();
    }();

    // for now we do not want to touch system libraries !!
    if (is_system)
        return;

#if ENABLE_DEBUG_OUTPUT == 1
    auto as = image->getAddressSpace();
    as->loadLibrary("libc.so.6", true);
#endif
    LOG_INFO(LOG_FILTER_BINARY_PATCHING, "Touching %s", object->pathName().c_str());

    auto label_builder = get_label_for_policy(POLICY_ADDRESS_TAKEN, taken_addresses);

    patch_callsites(object, image, decoder, call_sites, *label_builder);
    patch_calltargets(object, image, decoder, call_targets, *label_builder);
}
