#ifndef __INSTRUMENTATION_H
#define __INSTRUMENTATION_H

#include <BPatch.h>
#include <BPatch_basicBlock.h>
#include <BPatch_flowGraph.h>
#include <BPatch_function.h>
#include <PatchCFG.h>

#include "ca_defines.h"
#include "ast/parsing/function_borders.h"
#include "ast/ast.h"
#include "logging.h"
#include "utils.h"

template <typename instr_op_t, typename... arg_ts>
void instrument_function_basicBlocks_unordered(BPatch_function *function,
                                               instr_op_t instr_op, arg_ts &&... args)
{
    std::set<BPatch_basicBlock *> blocks;
    BPatch_flowGraph *cfg = function->getCFG();
    cfg->getAllBasicBlocks(blocks);
    function_borders::apply(function, blocks);

    for (auto block : blocks)
    {
        // Instrument BasicBlock
        LOG_TRACE(LOG_FILTER_INSTRUMENTATION, "Processing basic block %lx",
                  block->getStartAddress());
        instr_op(block, std::forward<arg_ts>(args)...);
    }
}

template <typename instr_op_t, typename... arg_ts>
void instrument_ast_basicBlocks_unordered(ast::ast const &ast, instr_op_t instr_op,
                                          arg_ts &&... args)
{
    util::for_each(ast.get_functions(), [&instr_op,
                                         &args...](ast::function const &ast_function) {
        LOG_TRACE(LOG_FILTER_INSTRUMENTATION, "Processing function %s",
                  ast_function.get_name().c_str());

        instrument_function_basicBlocks_unordered(ast_function.get_function(), instr_op,
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

#endif /* __INSTRUMENTATION_H */
