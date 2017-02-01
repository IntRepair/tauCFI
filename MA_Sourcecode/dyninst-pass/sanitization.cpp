#include <sanitization.h>

#include <unordered_map>
#include <BPatch_function.h>
#include <BPatch_basicBlock.h>
#include <BPatch_object.h>
#include <BPatch_image.h>

#include "instrumentation.h"

namespace sanitization
{

// ugly implementation (need to remove global vars!)
namespace
{

using fn_bb_points_t = std::unordered_map<BPatch_function *, std::vector<BPatch_basicBlock *>>;

static fn_bb_points_t start_points;
static fn_bb_points_t exit_points;

using dictionary_t = std::unordered_map<BPatch_basicBlock *, BPatch_function *>;

static dictionary_t dictionary;
static dictionary_t start_dictionary;
};

bool possible_end(BPatch_basicBlock *prev, BPatch_basicBlock *block)
{
    // check if the last block of a function is a padded block (possible) with a call
    // (possible) to a possibly non returning function (no idea of yet...)

    if ((block.getStartAddress() % 0x10) == 0)
    {
        // TODO if calls a function and is full of nops
        return true;
    }
    return false;
}

void initialize(BPatch_image *image)
{
    instrument_image_objects(image, [](BPatch_object *object) {
        instrument_object_functions(object, [](BPatch_function *function) {
            BPatch_flowGraph *cfg = function->getCFG();

            std::vector<BPatch_basicBlock *> entry_blocks;
            if (cfg->getEntryBasicBlock(entry_blocks))
            {
                for (auto &&entry_block : entry_blocks)
                {
                    start_points[function].push_back(block);
                    dictionary[block] = function;
                    start_dictionary[block] = function;
                }
            }
        }

        instrument_object_functions(object, [](BPatch_function *function) {
            BPatch_flowGraph *cfg = function->getCFG();

            std::set<BPatch_basicBlock *> blocks;
            cfg->getAllBasicBlocks(blocks);

            BPatch_basicBlock prev = nullptr;
            for (auto &&block : blocks)
            {
                if (dictionary.count(block) > 0 && start_dictionary.count(block) > 0)
                {
                    if (start_dictionary[block] != function)
                    {
                        if (possible_end(prev, block))
                            end_points[function] = prev;
                        return;
                    }
                }

                dictionary[block] = function;
                prev = block;
            }
        });

        instrument_object_functions(object, [](BPatch_function *function) {
            BPatch_flowGraph *cfg = function->getCFG();

            std::vector<BPatch_basicBlock *> exit_blocks;
            if (cfg->getExitBasicBlock(exit_blocks))
            {
                for (auto &&exit_block : exit_blocks)
                {
                    if (dictionary[exit_block] == function)
                    {
                        exit_points[function].push_back(block);
                    }
                }
            }
        }
    });
}

bool check(BPatch_function *function, BPatch_basicBlock *block)
{
    return function == dictionary[block];
}

void apply(BPatch_function *function, std::set<BPatch_basicBlock *> &blocks)
{
    for (auto itr = blocks.begin(); itr != blocks.end();)
    {
        if (!check(function, *itr))
        {
            itr = blocks.erase(it);
        }
        else
        {
            ++itr;
        }
    }
}
};
