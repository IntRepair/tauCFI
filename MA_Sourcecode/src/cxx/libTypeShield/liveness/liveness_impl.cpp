#include "liveness_impl.h"

#include <algorithm>
#include <cinttypes>
#include <memory>
#include <vector>

#include "ca_decoder.h"
#include "ast/ast.h"
#include "instrumentation.h"

namespace liveness {

namespace impl {
static bool is_xmm_passthrough_block(CADecoder *decoder, BPatch_basicBlock *block) {
  auto prev = -1;
  auto disp = 0ul;
  boost::optional<int> stack_reg;

  using Insns = Dyninst::PatchAPI::PatchBlock::Insns;
  instrument_basicBlock_instructions(
      block, [&](Insns::value_type &instruction) {
        auto address = reinterpret_cast<uint64_t>(instruction.first);
        Instruction::Ptr instruction_ptr = instruction.second;

        if (!decoder->decode(address, instruction_ptr)) {
          LOG_ERROR(LOG_FILTER_LIVENESS_ANALYSIS,
                    "Could not decode instruction @%lx", address);
          return;
        }

        // PROBLEM: ALIASING CAN PREVENT THIS !
        // if (!decoder->is_target_stackpointer_disp(0))
        if (!decoder->is_target_register_disp(0)) {
          LOG_ERROR(LOG_FILTER_LIVENESS_ANALYSIS, "FAIL A XMM @%lx prev:%d",
                    address, prev);
          prev = -2;
          return;
        } else if (prev == -1 && decoder->is_reg_source(0, DR_REG_XMM0)) {
          LOG_ERROR(LOG_FILTER_LIVENESS_ANALYSIS, "STAGE 1 XMM @%lx", address);
          prev++;
          disp = decoder->get_target_disp(0);
          stack_reg = decoder->get_reg_disp_target_register(0);
        } else if (!stack_reg ||
                   stack_reg != decoder->get_reg_disp_target_register(0)) {
          LOG_ERROR(LOG_FILTER_LIVENESS_ANALYSIS, "FAIL D XMM @%lx prev:%d",
                    address, prev);
          prev = -2;
          return;
        } else if ((disp += 0x10) != decoder->get_target_disp(0)) {
          LOG_ERROR(LOG_FILTER_LIVENESS_ANALYSIS, "FAIL B XMM @%lx prev:%d",
                    address, prev);
          prev = -2;
          return;
        } else if (prev == 0 && decoder->is_reg_source(0, DR_REG_XMM1))
          prev++;
        else if (prev == 1 && decoder->is_reg_source(0, DR_REG_XMM2))
          prev++;
        else if (prev == 2 && decoder->is_reg_source(0, DR_REG_XMM3))
          prev++;
        else if (prev == 3 && decoder->is_reg_source(0, DR_REG_XMM4))
          prev++;
        else if (prev == 4 && decoder->is_reg_source(0, DR_REG_XMM5))
          prev++;
        else if (prev == 5 && decoder->is_reg_source(0, DR_REG_XMM6))
          prev++;
        else if (prev == 6 && decoder->is_reg_source(0, DR_REG_XMM7))
          prev++;
        else {
          LOG_ERROR(LOG_FILTER_LIVENESS_ANALYSIS, "FAIL C XMM @%lx prev:%d",
                    address, prev);
          prev = -2;
          return;
        }
      });

  return (prev == 7);
}

static void calculate_ignore_instructions(AnalysisConfig &config,
                                   BPatch_basicBlock *init_block) {
  struct passthrough_t {
    passthrough_t(uint64_t offset_, uint32_t reg_, uint64_t address_)
        : offset(offset_), reg(reg_), address(address_) {}

    uint64_t offset;
    uint32_t reg;
    uint64_t address;
  };

  std::vector<passthrough_t> passthroughs;

  auto decoder = config.decoder;

  using Insns = Dyninst::PatchAPI::PatchBlock::Insns;
  instrument_basicBlock_instructions(
      init_block, [&](Insns::value_type &instruction) {
        auto address = reinterpret_cast<uint64_t>(instruction.first);
        Instruction::Ptr instruction_ptr = instruction.second;

        if (!decoder->decode(address, instruction_ptr)) {
          LOG_ERROR(LOG_FILTER_LIVENESS_ANALYSIS,
                    "Could not decode instruction @%lx", address);
          return;
        }

        if (decoder->is_target_register_disp(0)) {
          auto disp = decoder->get_target_disp(0);
          if (decoder->is_reg_source(0, DR_REG_R9))
            passthroughs.emplace_back(disp, DR_REG_R9, address);
          else if (decoder->is_reg_source(0, DR_REG_R8))
            passthroughs.emplace_back(disp, DR_REG_R8, address);
          else if (decoder->is_reg_source(0, DR_REG_RCX))
            passthroughs.emplace_back(disp, DR_REG_RCX, address);
          else if (decoder->is_reg_source(0, DR_REG_RDX))
            passthroughs.emplace_back(disp, DR_REG_RDX, address);
          else if (decoder->is_reg_source(0, DR_REG_RSI))
            passthroughs.emplace_back(disp, DR_REG_RSI, address);
          else if (decoder->is_reg_source(0, DR_REG_RDI))
            passthroughs.emplace_back(disp, DR_REG_RDI, address);
        }
      });

  std::sort(passthroughs.begin(), passthroughs.end(),
            [](passthrough_t const &left, passthrough_t const &right) {
              return left.offset >= right.offset;
            });

  auto current_reg = DR_REG_R9;
  auto done = false;
  for (auto const &pt : passthroughs) {
    if (pt.reg == current_reg && !done) {
      config.ignore_instructions->insert(pt.address);
      current_reg = [current_reg, &done]() {
        switch (current_reg) {
        case DR_REG_R9:
          return DR_REG_R8;
        case DR_REG_R8:
          return DR_REG_RCX;
        case DR_REG_RCX:
          return DR_REG_RDX;
        case DR_REG_RDX:
          return DR_REG_RSI;
        case DR_REG_RSI:
          return DR_REG_RDI;
        case DR_REG_RDI:
        default:
          done = true;
          return DR_REG_NULL;
        }
      }();
    }
  }
}

void initialize_variadic_passthrough(AnalysisConfig &config,
                                     ast::ast const &ast) {
  config.ignore_instructions = std::make_shared<AddressSet>();

  instrument_ast_basicBlocks_unordered(ast, [&](BPatch_basicBlock *block) {
    if (is_xmm_passthrough_block(config.decoder, block)) {
      LOG_INFO(LOG_FILTER_LIVENESS_ANALYSIS,
               "BasicBlock [%lx : %lx[ is an xmm passthrough block",
               block->getStartAddress(), block->getEndAddress());

      BPatch_Vector<BPatch_basicBlock *> xmm_targets;
      block->getTargets(xmm_targets);

      BPatch_Vector<BPatch_basicBlock *> xmm_sources;
      block->getSources(xmm_sources);

      if (xmm_targets.size() == 1 && xmm_sources.size() == 1) {
        auto init_block = xmm_sources[0];
        auto reg_block = xmm_targets[0];

        LOG_INFO(LOG_FILTER_LIVENESS_ANALYSIS,
                 "BasicBlock [%lx : %lx[ is an varargs passthrough init block",
                 init_block->getStartAddress(), init_block->getEndAddress());

        BPatch_Vector<BPatch_basicBlock *> init_targets;
        init_block->getTargets(init_targets);

        if (init_targets.size() == 2) {
          if (init_targets[0] == reg_block || init_targets[1] == reg_block) {
            LOG_INFO(
                LOG_FILTER_LIVENESS_ANALYSIS,
                "BasicBlock [%lx : %lx[ is an varargs passthrough reg block",
                init_block->getStartAddress(), init_block->getEndAddress());

            calculate_ignore_instructions(config, reg_block);
          }
        }
      }
    }
  });
}

static bool is_quasi_empty_return_block(CADecoder *decoder, BPatch_basicBlock *block) {
  bool is_quasi_empty = true;
  bool is_return = false;

  using Insns = Dyninst::PatchAPI::PatchBlock::Insns;
  instrument_basicBlock_instructions(
      block, [&](Insns::value_type &instruction) {
        auto address = reinterpret_cast<uint64_t>(instruction.first);
        Instruction::Ptr instruction_ptr = instruction.second;

        if (!decoder->decode(address, instruction_ptr)) {
          LOG_ERROR(LOG_FILTER_LIVENESS_ANALYSIS,
                    "Could not decode instruction @%lx", address);
          return;
        }

        if (!decoder->is_nop()) {
          if (decoder->is_return())
            is_return = true;
          else {
            auto pop_info = decoder->get_pop();
            if (!pop_info.first)
              is_quasi_empty = false;
          }
        }
      });

  return (is_quasi_empty && is_return);
}

void initialize_quasi_empty_return_blocks(AnalysisConfig &config,
                                                 ast::ast const &ast) {
  config.quasi_empty_return_blocks = std::make_shared<AddressSet>();

  instrument_ast_basicBlocks_unordered(ast, [&](BPatch_basicBlock *block) {
    if (is_quasi_empty_return_block(config.decoder, block))
      config.quasi_empty_return_blocks->insert(block->getStartAddress());
  });
}

}; /* namespace impl */

}; /* namespace liveness */
