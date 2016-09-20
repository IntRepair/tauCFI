#include "address_taken.h"

#include <Symtab.h>

#include "ca_decoder.h"
#include "instrumentation.h"
#include "logging.h"

using Dyninst::SymtabAPI::Region;
using Dyninst::PatchAPI::PatchBlock;

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
        LOG_INFO(LOG_FILTER_TAKEN_ADDRESS, "%s, %lx -> %lx", funcname, start, end);
    }
}

bool is_function_start(BPatch_image *image, const uint64_t val)
{
    std::vector<BPatch_function *> functions;
    image->findFunction(val, functions);

    return std::find_if(std::begin(functions), std::end(functions),
                        [val](BPatch_function *function) {
                            Dyninst::Address start, end;
                            return function->getAddressRange(start, end) && (start == val);
                        }) != std::end(functions);
}

TakenAddresses address_taken_analysis(BPatch_object *object, BPatch_image *image,
                                      CADecoder *decoder)
{
    std::map<uint64_t, uint64_t> reference_map;
    std::unordered_map<uint64_t, std::vector<TakenAddressSource>> taken_addresses;
    auto symtab = Dyninst::SymtabAPI::convert(object);

    LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS, "Processing object %s", object->name().c_str());

    Region *region = nullptr;
    auto const found_text = symtab->findRegion(region, ".text");
    auto const text_start = region->getMemOffset();
    auto const text_end = text_start + region->getMemSize();
    LOG_TRACE(LOG_FILTER_TAKEN_ADDRESS, "TEXT(mem)  %lx -> %lx", text_start, text_end);

    auto const found_plt = symtab->findRegion(region, ".plt");
    auto const plt_start = region->getMemOffset();
    auto const plt_end = plt_start + region->getMemSize();
    LOG_TRACE(LOG_FILTER_TAKEN_ADDRESS, "PLT(mem)  %lx -> %lx", plt_start, plt_end);

// TODO: Only if we want to secure PLT and GOT (requires runtime module)
#if 0
    if (!symtab->findRegion(region, ".got"))
        return false;
    auto const got_start = region->getMemOffset();
    auto const got_size = region->getMemSize();
        LOG_TRACE(LOG_FILTER_TAKEN_ADDRESS, "GOT(mem)  %lx -> %lx", got_start, got_end);

    if (!symtab->findRegion(region, ".data"))
        return false;
    auto const data_start = region->getMemOffset();
    auto const data_size = region->getMemSize();
        LOG_TRACE(LOG_FILTER_TAKEN_ADDRESS, "DATA(mem)  %lx -> %lx", data_start, data_end);
#endif

    if (!found_plt || !found_text)
    {
        LOG_ERROR(LOG_FILTER_TAKEN_ADDRESS, ".plt or .text section not found -> could not process");
    }
    else
    {
        for (auto &&region_name : {".data", ".rodata", ".dynsym"})
        {
            if (symtab->findRegion(region, region_name))
            {
                auto const region_data_begin = static_cast<uint8_t *>(region->getPtrToRawData());
                auto const region_offset = region->getMemOffset();
                auto const region_size = region->getMemSize();
                auto const region_data_end = region_data_begin + region_size - sizeof(uint64_t);
                for (auto value_ptr = region_data_begin; value_ptr < region_data_end; ++value_ptr)
                {
                    uint64_t value = *reinterpret_cast<uint64_t *>(value_ptr);
                    uint64_t value_offset = value_ptr - region_data_begin + region_offset;

                    auto type_fn = [plt_start, plt_end, value_offset,
                                    &region_name](uint64_t value) {
                        if (plt_start <= value && value <= plt_end)
                        {
                            LOG_TRACE(LOG_FILTER_TAKEN_ADDRESS, "%lx:%lx,.plt", value_offset,
                                      value);
                            return ".plt";
                        }

                        if (region_name == ".rodata")
                        {
                            LOG_TRACE(LOG_FILTER_TAKEN_ADDRESS, "%lx:%lx,.data", value_offset,
                                      value);
                            return ".data";
                        }

                        LOG_TRACE(LOG_FILTER_TAKEN_ADDRESS, "%lx:%lx,%s", value_offset, value,
                                  region_name);
                        return region_name;
                    };

                    if ((text_start <= value && value <= text_end) ||
                        (plt_start <= value && value <= plt_end))
                    {
                        auto type = type_fn(value);
                        // PLT here means, statically, in data section, pointer point to PLT are
                        // found. In this case, dyninst sometimes do not conclude that a plt entry
                        // is a function So add it no matter what
                        if (type == ".plt")
                        {
                            taken_addresses[value].emplace_back(std::string("plt"), object,
                                                                value_offset);
                            LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS, "<PLT>%lx:%lx", value_offset,
                                      value);
                        }
                        // For the rest case, where pointer stored into data/rodata section
                        else if (is_function_start(image, value))
                        {
                            reference_map[value_offset] = value;

                            if (type == ".data")
                                taken_addresses[value].emplace_back(std::string("data"), object,
                                                                    value_offset);

                            LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS, "<DATA>%lx:%lx", value_offset,
                                      value);
                        }
                    }
                }
            }
        }
    }

// TODO: Only if we want to secure GOT and PLT (requires runtime module)
#if 0
    /*
    def read_plt_entries():
        global PLT_ENTRY

        f = open("./plt.log","r");
        d = f.readlines();
        f.close();

        for l in d:
            addr = int(l.split(" ")[0],16);
            name = l.split("<")[1].split("@plt>:")[0];
            PLT_ENTRY[name] = addr;
        return;

    def search_plt_entries(ename):
        global PLT_ENTRY

        strip_name = ename;
        if (ename.find(" ")!=-1):
            strip_name = ename.split(" ")[0];

        if strip_name in PLT_ENTRY.keys():
            return PLT_ENTRY[strip_name];

        return 0;
    */

    // is an array of struct Elf64_Rela objects (see /usr/include/elf.h)
    //    typedef struct
    //    {
    //      Elf64_Addr	    r_offset;		/* Address */
    //      Elf64_Xword	    r_info;			/* Relocation type and symbol index */
    //      Elf64_Sxword	r_addend;		/* Addend */
    //    } Elf64_Rela;
    struct elf64_rela
    {
        uint64 address;
        uint64 info;
        uint64 addend;
    };

    using rela_entry_t = elf64_rela;

    if (symtab->findRegion(region, ".rela.dyn"))
    {
        auto const relocs = region->getRelocations();

        LOG_TRACE(LOG_FILTER_TAKEN_ADDRESS, ".rela.dyn %lu entries:", relocs.size());

        for (auto &&reloc : relocs)
        {
            LOG_TRACE(LOG_FILTER_TAKEN_ADDRESS, "\t tar: %lu rel: %lu add: %lu", reloc.target_addr(),
                  reloc.rel_addr(), reloc.addend());
        }
        /*
              #read rela.dyn table
              k = 0;
              while (k<len(d))and(d[k].find("Relocation section '.rela.dyn'")==-1):
                  k += 1;
              if (k >= len(d)):
                  return;
              d = d[k+2:]
              rela_dyn_dict = dict();
              for l in d:
                  if (l == "\n"):
                      break;
                  items = read_rela_section_line(l);
                  offset = int(items[0],16);
                  val = int(items[3],16);
                  name = items[4];
                  if (name != ""):
                      rela_dyn_dict[name] = offset;

                  if ((val >= _TEXT_START)and(val <= _TEXT_END)or((val >= _PLT_START)and(val <=
             _PLT_END))):
                      print "%x:%x,.got"%(offset,val);
          */
    }

    if (symtab->findRegion(region, ".rela.plt"))
    {
        auto const relocs = region->getRelocations();

        LOG_TRACE(LOG_FILTER_TAKEN_ADDRESS, ".rela.plt %lu entries:", relocs.size());
        for (auto &&reloc : relocs)
        {
            LOG_TRACE(LOG_FILTER_TAKEN_ADDRESS, "\t tar: %lu rel: %lu add: %lu", reloc.target_addr(),
                  reloc.rel_addr(), reloc.addend());
        }
        /*
            #read rela.plt table
            k = 0;
            while (k<len(d))and(d[k].find("Relocation section '.rela.plt'")==-1):
                k += 1;
            if (k >= len(d)):
                return;
            d = d[k+2:]
            rela_plt_dict = dict();
            for l in d:
                if (l == "\n"):
                    break;
                items = read_rela_section_line(l);
                offset = int(items[0],16);
                val = int(items[3],16);
                name = items[4];
                if (val==0):
                    val = search_plt_entries(name);
                if (name in rela_dyn_dict.keys()):
                    #means the .rela.dyn have same entry for this plt entry, but the offset may
           differ, so add both
                    if ((rela_dyn_dict[name]>=_DATA_START)and(rela_dyn_dict[name]<=_DATA_END)):
                        print "%x:%x,.data"%(rela_dyn_dict[name],val);
                    else:
                        print "%x:%x,.rela.plt"%(rela_dyn_dict[name],val);

                print "%x:%x,.rela.plt"%(offset,val);
        */
    }
#endif

    instrument_object_instructions_unordered(
        object, [&](PatchBlock::Insns::value_type &instruction) {
            // get instruction bytes
            auto const address = reinterpret_cast<uint64_t>(instruction.first);
            Instruction::Ptr instruction_ptr = instruction.second;

            decoder->decode(address, instruction_ptr);
            uint64_t const deref_address = decoder->get_src_abs_addr();
            LOG_TRACE(LOG_FILTER_INSTRUMENTATION, "Processing instruction %lx : %lx", address,
                      deref_address);

            if (deref_address == 0)
                return;

            if (is_function_start(image, deref_address))
            {
                LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS, "<AT>%lx:%lx", address, deref_address);
                taken_addresses[deref_address].emplace_back(std::string("insns(direct)"), object,
                                                            address);
            }

            if (reference_map.count(deref_address) > 0)
            {
                //    if (address!=reference_map[deref_address])
                auto deref_ref = reference_map[deref_address];
                LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS, "<CK>%lx:%lx", address, deref_ref);
                taken_addresses[deref_ref].emplace_back(std::string("insns(indir)"), object,
                                                        address);
            }

            if (plt_start <= deref_address && deref_address <= plt_end)
            {
                LOG_DEBUG(LOG_FILTER_TAKEN_ADDRESS, "<PLT>%lx:%lx", address, deref_address);
                taken_addresses[deref_address].emplace_back(std::string("insns(plt)"), object,
                                                            address);
            }
        });

    return taken_addresses;
}
