#ifndef __CALLTARGETS_H
#define __CALLTARGETS_H

#include <BPatch.h>
#include <BPatch_addressSpace.h>
#include <BPatch_addressSpace.h>
#include <BPatch_module.h>

#include "address_taken.h"
#include "ca_decoder.h"

#include <string>
#include <vector>

struct CallTarget
{
    std::string module_name;
    BPatch_function *function;
    std::string register_state;
};

std::vector<CallTarget> calltarget_analysis(BPatch_image *image, std::vector<BPatch_module *> *mods,
                                            BPatch_addressSpace *as, CADecoder *decoder,
                                            std::vector<TakenAddress> &taken_addresses);

void dump_calltargets(std::vector<CallTarget> &targets);

#endif /* __CALLTARGETS_H */
