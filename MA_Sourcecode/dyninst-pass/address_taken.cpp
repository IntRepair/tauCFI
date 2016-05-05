#include "address_taken.h"
#include "utils.h"

#include <boost/range/adaptor/reversed.hpp>

#include <BPatch.h>
#include <BPatch_addressSpace.h>
#include <BPatch_binaryEdit.h>
#include <BPatch_flowGraph.h>
#include <BPatch_function.h>
#include <BPatch_object.h>
#include <BPatch_point.h>
#include <BPatch_process.h>

#include <AddrLookup.h>
#include <PatchCFG.h>
#include <Symtab.h>

using namespace Dyninst::PatchAPI;
using namespace Dyninst::InstructionAPI;
using namespace Dyninst::SymtabAPI;

void dump_starting_functions(BPatch_image *image, uint64_t address)
{
    std::vector<BPatch_function *> functions;
    image->findFunction(address, functions);

    char funcname[BUFFER_STRING_LEN];
    Dyninst::Address start, end;

    for (auto function : functions)
    {
        function->getName(funcname, BUFFER_STRING_LEN);
        function->getAddressRange(start, end);
        LOG_INFO("%s, %lx -> %lx\n", funcname, start, end);
    }
}

bool is_function_start(BPatch_image *image, const uint64_t val)
{
    std::vector<BPatch_function *> functions;
    image->findFunction(val, functions);

    return std::find_if(std::begin(functions), std::end(functions), [val](BPatch_function *function) {
               Dyninst::Address start, end;
               return function->getAddressRange(start, end) && (start == val);
           }) != std::end(functions);
}

// TODO: Implement this without relying on external input
std::map<uint64_t, uint64_t> read_raw_ck(BPatch_image *image, std::vector<TakenAddress> &taken_addresses)
{
    std::map<uint64_t, uint64_t> reference_map;
    char program_name[BUFFER_STRING_LEN];
    image->getProgramName(program_name, BUFFER_STRING_LEN);

    std::string fname(program_name);
    fname += ".raw";

    char *line;
    size_t len = 0;

    char buf[BUFFER_STRING_LEN];

    auto pfile = fopen(fname.c_str(), "r");
    while (getline(&line, &len, pfile) != -1)
    {
        uint64_t ref_addr;
        uint64_t tar_addr;

        sscanf(line, "%lx:%lx,%s\n", &ref_addr, &tar_addr, buf);
        // PLT here means, statically, in data section, pointer point to PLT are found.
        // In this case, dyninst sometimes do not conclude that a plt entry is a function
        // So add it no matter what
        if (strcmp(buf, ".plt") == 0)
        {
            LOG_DEBUG("<PLT>%lx:%lx\n", ref_addr, tar_addr)
            dump_starting_functions(image, tar_addr);
        }

        // Here, the relation plt section will be fill into the got table, and its value
        // is dynamic deceide, so we check if the got offset is dereference later.
        // However we can not ask the value is a function entry since it is static read the binary
        if (strcmp(buf, ".rela.plt") == 0)
        {
            reference_map[ref_addr] = tar_addr;
        }

        // For the rest case, where pointer stored into data/rodata section
        if (is_function_start(image, tar_addr))
        {
            reference_map[ref_addr] = tar_addr;
            if ((strcmp(buf, ".data") == 0) || (strcmp(buf, ".rodata") == 0))
            {
                taken_addresses.emplace_back(TakenAddress{std::string("raw ck"), tar_addr});
                LOG_DEBUG("<DATA>%lx:%lx\n", ref_addr, tar_addr);
            }
        }

        /* TODO: How to handle these cases ?*/
        else if (tar_addr == 0)
            LOG_DEBUG("target is 0 :%s", line);
        else
            LOG_DEBUG("target is not a function:%s", line);

        free(line);
        line = 0;
    }

    return reference_map;
}

std::vector<TakenAddress> address_taken_analysis(BPatch_image *image, std::vector<BPatch_module *> *mods,
                                                 BPatch_addressSpace *as, CADecoder *decoder)
{
    std::vector<TakenAddress> taken_addresses;

    auto reference_map = read_raw_ck(image, taken_addresses);

    for (auto module : *mods)
    {
        char modname[BUFFER_STRING_LEN];
        module->getFullName(modname, BUFFER_STRING_LEN);

        if (module->isSharedLib())
        {
            LOG_WARNING("Skipping shared library %s\n", modname);
        }
        else
        {
            LOG_INFO("Processing module: %s\n", modname);
            // Instrument module
            char funcname[BUFFER_STRING_LEN];
            std::vector<BPatch_function *> *functions = module->getProcedures(true);

            for (auto function : *functions)
            {
                // Instrument function
                function->getName(funcname, BUFFER_STRING_LEN);
                LOG_DEBUG("Processing function %s\n", funcname);

                std::set<BPatch_basicBlock *> blocks;
                BPatch_flowGraph *cfg = function->getCFG();
                cfg->getAllBasicBlocks(blocks);

                for (auto block : boost::adaptors::reverse(blocks))
                {
                    // Instrument BasicBlock
                    LOG_DEBUG("Processing basic block %lx\n", block->getStartAddress());
                    // iterate backwards (PatchAPI restriction)
                    PatchBlock::Insns insns;
                    Dyninst::PatchAPI::convert(block)->getInsns(insns);

                    for (auto &instruction : boost::adaptors::reverse(insns))
                    {
                        // get instruction bytes
                        auto address = reinterpret_cast<uint64_t>(instruction.first);
                        Instruction::Ptr instruction_ptr = instruction.second;

                        decoder->decode(address, instruction_ptr);
                        uint64_t deref_address = decoder->get_src_abs_addr();
                        LOG_DEBUG("Processing instruction %lx : %lx\n", address, deref_address);
                        if (deref_address == 0)
                            continue;

                        if (is_function_start(image, deref_address))
                        {
                            LOG_DEBUG("<AT>%lx:%lx\n", address, deref_address);
                            taken_addresses.emplace_back(TakenAddress{std::string(modname), deref_address});
                        }
                        else if (reference_map.count(deref_address) > 0)
                        {
                            //    if (address!=reference_map[deref_address])
                            auto deref_ref = reference_map[deref_address];
                            LOG_DEBUG("<CK>%lx:%lx\n", address, deref_ref);
                            taken_addresses.emplace_back(TakenAddress{std::string(modname), deref_ref});
                        }
                    }
                }
            }
        }
    }

    return taken_addresses;
}

void dump_taken_addresses(std::vector<TakenAddress> &addresses)
{
    for (auto &address : addresses)
        LOG_INFO("<AT>%s:%lx\n", address.module_name.c_str(), address.deref_address);
}
