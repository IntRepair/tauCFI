#include "patching.h"

#include "BPatch_basicBlock.h"
#include "BPatch_function.h"
#include "BPatch_point.h"
#include "BPatch_process.h"
#include "Command.h"
#include "PatchMgr.h"
#include "Snippet.h"
#include "dynC.h"
#include "PatchMgr.h"
#include "Instrumenter.h"
#include "Snippet.h"


#include "ca_defines.h"
#include "instrumentation.h"
#include "logging.h"

#include <unordered_set>

using Dyninst::PatchAPI::Patcher;
using Dyninst::PatchAPI::PushBackCommand;

// For now this is the offset, as it is unlikely that something is outside of a disp32
// range ...
static const uint32_t ANNOTATION_OFFSET = 5;

// Create and insert a printf snippet
bool create_and_insert_snippet_callsite(BPatch_image *image,
                                        std::vector<BPatch_point *> &points,
                                        std::vector<char> label, uint32_t address, Patcher &patcher)
{
    auto as = image->getAddressSpace();

    //std::vector<BPatch_snippet *> snippet_sequence;

    //// Create the printf function call snippet
    //std::vector<BPatch_snippet *> printfArgs;
    //auto fmt = BPatch_constExpr(
    //    "[%lx | %lx] the address %lx is called [%lx] = %lx (expected %lx)\n");
    //printfArgs.push_back(&fmt);
    //
    //auto callsite_addr_code = BPatch_constExpr(address);
    //printfArgs.push_back(&callsite_addr_code);
    //auto callsite_addr = BPatch_originalAddressExpr();
    //printfArgs.push_back(&callsite_addr);
    //auto target_addr = BPatch_dynamicTargetExpr();
    //printfArgs.push_back(&target_addr);

    //auto tag_address =
    //    BPatch_arithExpr(BPatch_plus, target_addr, BPatch_constExpr(ANNOTATION_OFFSET));
    //printfArgs.push_back(&tag_address);
    //auto tag_value = BPatch_arithExpr(BPatch_deref, tag_address);
    //printfArgs.push_back(&tag_value);

    //uint32_t label_val = 0x000000FF & static_cast<uint32_t>(label[3]);
    //label_val = label_val << 8 | (0x000000FF & static_cast<uint32_t>(label[2]));
    //label_val = label_val << 8 | (0x000000FF & static_cast<uint32_t>(label[1]));
    //label_val = label_val << 8 | (0x000000FF & static_cast<uint32_t>(label[0]));

    //auto expected_tag_value = BPatch_constExpr(label_val);
    //
    //auto result = BPatch_arithExpr(BPatch_bit_and, tag_value, expected_tag_value);
    //auto result_16 = BPatch_arithExpr(BPatch_times, tag_value, BPatch_constExpr(0x10000));
    //auto result_16_0 = BPatch_arithExpr(BPatch_bit_or, tag_value, result_16);
    //auto result_24_8 = BPatch_arithExpr(BPatch_times, result_16_0, BPatch_constExpr(0x100));
    //auto result_32_24_16_8 = BPatch_arithExpr(BPatch_bit_or, result_24_8, result_16_0);

    //printfArgs.push_back(&result_32_24_16_8);

    //// Find the printf function
    //std::vector<BPatch_function *> printfFuncs;
    //image->findFunction("printf", printfFuncs);
    //if (printfFuncs.size() == 0)
    //{
    //    LOG_ERROR(LOG_FILTER_BINARY_PATCHING, "Could not find \"printf\"");
    //    return false;
    //}
    // Construct a function call snippet
    //snippet_sequence.push_back(new BPatch_funcCallExpr(*(printfFuncs[0]), printfArgs));

    std::stringstream snippet_stream;

    uint32_t label_val = 0x000000FF & static_cast<uint32_t>(label[3]);
    label_val = label_val << 8 | (0x000000FF & static_cast<uint32_t>(label[2]));
    label_val = label_val << 8 | (0x000000FF & static_cast<uint32_t>(label[1]));
    label_val = label_val << 8 | (0x000000FF & static_cast<uint32_t>(label[0]));

    uint32_t label_extra = (0x000000FF & static_cast<uint32_t>(label[4])) << 24;

    //snippet_stream << "long tag_address = dyninst`dynamic_target + " << ANNOTATION_OFFSET << ";\n";
    snippet_stream << "int tag_address = dyninst`dynamic_target;\n";
    //snippet_stream << "int tag_address = " << 0xEEFFC0 << ";\n";
    //snippet_stream << "int tag_value = dyninst`dynamic_target [ " << ANNOTATION_OFFSET << " ] ;\n";
    //snippet_stream << "int tag_value = " << 0x3F3F3F3F << ";\n";
    //snippet_stream << "tag_value &= " << 0x3F3F3F3F << ";\n"; // insert mask
    //snippet_stream << "tag_value |= " << 0x3F000000 << ";\n"; // insert mask
    //snippet_stream << "int tag_value_a = tag_value * " << (1 << 16) << ";\n";
    //snippet_stream << "tag_value |= tag_value_a;\n";
    //snippet_stream << "int tag_value_b = tag_value * " << (1 << 8) << ";\n";
    //snippet_stream << "tag_value |= tag_value_b;\n";
    //snippet_stream << "tag_value &= " << 0xFF000000 << ";\n";
//    snippet_stream << "func`printf(\"Function call in %s to %lx, which has the following tag value %lx\\n\", dyninst`function_name, dyninst`dynamic_target, tag_value);\n";
    //snippet_stream << "if (tag_value != " << 0x3F000000 << ") { dyninst`break(); } \n";

    //DYNC_EXPORT std::map<BPatch_point *, BPatch_snippet *> *createSnippet(std::string str, std::vector<BPatch_point *> points);
    auto snippet_map = dynC_API::createSnippet(snippet_stream.str().c_str(), *as); // points);

    //LOG_DEBUG(LOG_FILTER_BINARY_PATCHING, "attempting to patch via insertSnippet...");

    // Insert the snippet
    //for (auto const& point_snippet_pair : *snippet_map)
    //{
    for (auto const& point : points)
    {
        //patcher.add(PushBackCommand::create(Dyninst::PatchAPI::convert(point_snippet_pair.first, BPatch_callBefore), Dyninst::PatchAPI::convert(point_snippet_pair.second)));
        patcher.add(PushBackCommand::create(Dyninst::PatchAPI::convert(point, BPatch_callBefore), Dyninst::PatchAPI::convert(snippet_map)));

        //if (!as->insertSnippet(*point_snippet_pair.second, *point_snippet_pair.first, BPatch_callBefore))
        //{
        //    LOG_ERROR(LOG_FILTER_BINARY_PATCHING, "insertSnippet: FAIL");
        //    return false;
        //}
    }

    //LOG_DEBUG(LOG_FILTER_BINARY_PATCHING, "insertSnippet: SUCCESS");
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
        create_and_insert_snippet_callsite(image, patch_points, label, call_site.address, patcher);
    }
    patcher.commit();
}

void patch_calltargets(BPatch_object *object, BPatch_image *image, CADecoder *decoder,
                       CallTargets call_targets, LabelBuilder const &label_builder)
{
    std::vector<BPatch_point *> patch_points;
    for (auto call_target : call_targets)
    {
        //if (!call_target.plt)
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

    LOG_INFO(LOG_FILTER_BINARY_PATCHING, "Touching %s", object->pathName().c_str());

    auto label_builder = get_label_for_policy(POLICY_ADDRESS_TAKEN, taken_addresses);

    patch_callsites(object, image, decoder, call_sites, *label_builder);
    //patch_calltargets(object, image, decoder, call_targets, *label_builder);
}
