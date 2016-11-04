#ifndef __INSTRUMENTATION_H
#define __INSTRUMENTATION_H

#include <BPatch.h>
#include <BPatch_basicBlock.h>
#include <PatchCFG.h>
#include <BPatch_flowGraph.h>
#include <BPatch_function.h>
#include <BPatch_module.h>
#include <BPatch_object.h>
#include <PatchCFG.h>

#include "ca_defines.h"
#include "logging.h"

template <typename instr_op_t, typename... arg_ts>
void instrument_image_objects(BPatch_image *image, instr_op_t instr_op, arg_ts &&... args)
{
    std::vector<BPatch_object *> objects;
    image->getObjects(objects);
    for (auto object : objects)
    {
        // Instrument object
        LOG_TRACE(LOG_FILTER_INSTRUMENTATION, "Processing object %s",
                  object->name().c_str());

        instr_op(object, std::forward<arg_ts>(args)...);
    }
}

template <typename instr_op_t, typename... arg_ts>
void instrument_object_functions(BPatch_object *object, instr_op_t instr_op,
                                 arg_ts &&... args)
{
    std::vector<BPatch_module *> modules;
    object->modules(modules);

    for (auto module : modules)
    {
        // Instrument module
        char modname[BUFFER_STRING_LEN];
        module->getFullName(modname, BUFFER_STRING_LEN);

        LOG_TRACE(LOG_FILTER_INSTRUMENTATION, "Processing module%s: %s",
                  (module->isSharedLib() ? "[shared library]" : ""), modname);

        std::vector<BPatch_function *> functions;
        module->getProcedures(functions, true);
        for (auto function : functions)
        {
            // Instrument function
            char funcname[BUFFER_STRING_LEN];
            function->getName(funcname, BUFFER_STRING_LEN);
            LOG_TRACE(LOG_FILTER_INSTRUMENTATION, "Processing function %s", funcname);
            instr_op(function, std::forward<arg_ts>(args)...);
        }
    }
}

template <typename instr_op_t, typename... arg_ts>
void instrument_function_basicBlocks_unordered(BPatch_function *function,
                                               instr_op_t instr_op, arg_ts &&... args)
{
    std::set<BPatch_basicBlock *> blocks;
    BPatch_flowGraph *cfg = function->getCFG();
    cfg->getAllBasicBlocks(blocks);

    for (auto block : blocks)
    {
        // Instrument BasicBlock
        LOG_TRACE(LOG_FILTER_INSTRUMENTATION, "Processing basic block %lx",
                  block->getStartAddress());
        instr_op(block, std::forward<arg_ts>(args)...);
    }
}

template <typename instr_op_t, typename... arg_ts>
void instrument_object_basicBlocks_unordered(BPatch_object *object, instr_op_t instr_op,
                                             arg_ts &&... args)
{
    instrument_object_functions(object, [&instr_op, &args...](BPatch_function *function) {
        instrument_function_basicBlocks_unordered(function, instr_op,
                                                  std::forward<arg_ts>(args)...);
    });
}

template <typename instr_op_t, typename... arg_ts>
void instrument_basicBlock_instructions(BPatch_basicBlock *block, instr_op_t instr_op,
                                        arg_ts &&... args)
{
    Dyninst::PatchAPI::PatchBlock::Insns insns;
    auto patch_block = Dyninst::PatchAPI::convert(block);
    patch_block->getInsns(insns);
    for (auto &instruction : insns)
    {
        // Instrument Instruction
        auto address = reinterpret_cast<uint64_t>(instruction.first);
        LOG_TRACE(LOG_FILTER_INSTRUMENTATION, "Processing instruction %lx", address);
        instr_op(instruction, std::forward<arg_ts>(args)...);
    }
}

template <typename instr_op_t, typename decoder_t, typename... arg_ts>
void instrument_basicBlock_decoded(BPatch_basicBlock *block, decoder_t *decoder,
                                   instr_op_t instr_op, arg_ts &&... args)
{

    Dyninst::PatchAPI::PatchBlock::Insns insns;
    auto patch_block = Dyninst::PatchAPI::convert(block);
    patch_block->getInsns(insns);
    for (auto &instruction : insns)
    {
        auto const address = reinterpret_cast<uint64_t>(instruction.first);
        Dyninst::InstructionAPI::Instruction::Ptr instruction_ptr = instruction.second;

        LOG_TRACE(LOG_FILTER_INSTRUMENTATION, "Processing instruction %lx", address);
        if (!decoder->decode(address, instruction_ptr))
        {
            LOG_ERROR(LOG_FILTER_INSTRUMENTATION, "Could not decode instruction %lx",
                      address);
        }
        else
        {
            instr_op(decoder, std::forward<arg_ts>(args)...);
        }
    }
}

template <typename instr_op_t, typename decoder_t, typename... arg_ts>
void instrument_function_decoded_unordered(BPatch_function *function, decoder_t *decoder,
                                           instr_op_t instr_op, arg_ts &&... args)
{
    instrument_function_basicBlocks_unordered(
        function, [&instr_op, &decoder, &args...](BPatch_basicBlock *block) {
            instrument_basicBlock_decoded(block, decoder, instr_op,
                                          std::forward<arg_ts>(args)...);
        });
}

template <typename instr_op_t, typename decoder_t, typename... arg_ts>
void instrument_object_decoded_unordered(BPatch_object *object, decoder_t *decoder,
                                         instr_op_t instr_op, arg_ts &&... args)
{
    instrument_object_functions(
        object, [&instr_op, &decoder, &args...](BPatch_function *function) {
            instrument_function_decoded_unordered(function, decoder, instr_op,
                                                  std::forward<arg_ts>(args)...);
        });
}

#endif /* __INSTRUMENTATION_H */
