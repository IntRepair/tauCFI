#include "function_borders.h"

#include <unordered_map>

#include <BPatch_basicBlock.h>
#include <BPatch_function.h>

#include "instrumentation.h"
#include "logging.h"

namespace function_borders
{

// ugly implementation (need to remove global vars!)
namespace
{
/*
using fn_bb_points_t = std::unordered_map<BPatch_function *, std::vector<BPatch_basicBlock
*>>;

static fn_bb_points_t start_points;
static fn_bb_points_t exit_points;

using dictionary_t = std::unordered_map<BPatch_basicBlock *, BPatch_function *>;

static dictionary_t dictionary;
static dictionary_t start_dictionary;
*/

using borders_t = std::pair<uint64_t, uint64_t>;
using border_dictionary_t = std::unordered_map<BPatch_function *, borders_t>;

using bb_fn_dictionary_t = std::unordered_map<BPatch_basicBlock *, BPatch_function *>;

static bb_fn_dictionary_t bb_fn_dictionary;
};

// bool possible_end(BPatch_basicBlock *prev, BPatch_basicBlock *block)
//{
//    // check if the last block of a function is a padded block (possible) with a call
//    // (possible) to a possibly non returning function (no idea of yet...)
//
//    if ((block.getStartAddress() % 0x10) == 0)
//    {
//        // TODO if calls a function and is full of nops
//        return true;
//    }
//    return false;
//}

void initialize(std::vector<ast::function> const &functions)
{

    std::set<std::pair<uint64_t, BPatch_function *>> border_set;
    for (auto const &ast_function : functions)
    {
        auto function = ast_function.get_function();
        auto cfg = function->getCFG();

        std::vector<BPatch_basicBlock *> entry_blocks;
        if (cfg->getEntryBasicBlock(entry_blocks))
        {
            if (entry_blocks.empty())
                return;

            auto element = std::min_element(
                std::begin(entry_blocks), std::end(entry_blocks),
                [](BPatch_basicBlock *left, BPatch_basicBlock *right) {
                    return left->getStartAddress() < right->getStartAddress();
                });

            border_set.insert(std::make_pair((*element)->getStartAddress(), function));
        }
    }

    for (auto itr = std::begin(border_set); itr != std::end(border_set);)
    {
        auto current = itr++;
        auto function = current->second;
        auto fn_start = current->first;
        auto cfg = function->getCFG();

        std::set<BPatch_basicBlock *> basic_blocks;
        if (cfg->getAllBasicBlocks(basic_blocks))
        {
            if (itr == std::end(border_set))
            {
                for (auto &block : basic_blocks)
                {
                    if (block->getStartAddress() >= fn_start)
                        bb_fn_dictionary[block] = function;
                }
            }
            else
            {
                auto fn_end = itr->first;
                for (auto &block : basic_blocks)
                {
                    if (block->getStartAddress() >= fn_start &&
                        block->getStartAddress() < fn_end)
                        bb_fn_dictionary[block] = function;
                }
            }
        }
    }

    // LOG_TRACE(LOG_FILTER_GENERAL, "function borders:\n");
    // for (auto const &bb_fn_pair : bb_fn_dictionary)
    //{
    //    auto function = bb_fn_pair.second;
    //
    //    LOG_TRACE(LOG_FILTER_GENERAL, "\t(%lx, %s)\n",
    //    bb_fn_pair.first->getStartAddress(),
    //             (function ? function->getName().c_str() : "<unknown>"));
    //}
}

bool is_valid(BPatch_function *source, BPatch_basicBlock *target)
{
    return source == bb_fn_dictionary[target];
}

bool is_valid(BPatch_basicBlock *source, BPatch_basicBlock *target)
{
    return bb_fn_dictionary[source] == bb_fn_dictionary[target];
}

// bool check(BPatch_function *function, BPatch_basicBlock *block)
//{
//    return function == dictionary[block];
//}
//
// void apply(BPatch_function *function, std::set<BPatch_basicBlock *> &blocks)
//{
//    for (auto itr = blocks.begin(); itr != blocks.end();)
//    {
//        if (!check(function, *itr))
//        {
//            itr = blocks.erase(it);
//        }
//        else
//        {
//            ++itr;
//        }
//    }
//}
};
