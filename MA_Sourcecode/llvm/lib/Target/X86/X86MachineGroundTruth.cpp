#include "X86.h"
#include "X86InstrInfo.h"
#include "X86Subtarget.h"

#include "llvm/Analysis/IndirectCallSiteVisitor.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {
#define DEBUG_TYPE "ground_truth"

/*
    if (CS.getCalledFunction() || !CS.getCalledValue())
      return;
    Instruction *I = CS.getInstruction();
    if (CallInst *CI = dyn_cast<CallInst>(I)) {
      if (CI->isInlineAsm())
        return;
    }
    if (isa<Constant>(CS.getCalledValue()))
      return;
    IndirectCallInsts.push_back(I);
*/

namespace {
static bool callsiteOpcodeCheck(MachineInstr const& MI)
{
  switch(MI.getOpcode())
  {
    case X86::CALLpcrel32: errs() << "X86::CALLpcrel32:\n"; break;
    case X86::CALLpcrel16: errs() << "X86::CALLpcrel16:\n"; break;
    case X86::CALL16r: errs() << "X86::CALL16r:\n"; break;
    case X86::CALL16m: errs() << "X86::CALL16m:\n"; break;
    case X86::CALL32r: errs() << "X86::CALL32r:\n"; break;
    case X86::CALL32m: errs() << "X86::CALL32m:\n"; break;
    case X86::FARCALL16i: errs() << "X86::FARCALL16i:\n"; break;
    case X86::FARCALL32i: errs() << "X86::FARCALL32i:\n"; break;
    case X86::FARCALL16m: errs() << "X86::FARCALL16m:\n"; break;
    case X86::FARCALL32m: errs() << "X86::FARCALL32m:\n"; break;
    case X86::CALL64pcrel32: errs() << "X86::CALL64pcrel32:\n"; break;
    case X86::CALL64r: errs() << "X86::CALL64r:\n"; break;
    case X86::CALL64m: errs() << "X86::CALL64m:\n"; break;
    case X86::FARCALL64: errs() << "X86::FARCALL64:\n"; break;
   default:
      return false;
  }
  return true;
}

static bool isIndirCS(ImmutableCallSite &CS, bool ext_log) {
  if (!CS.isCall()) {
    if (ext_log)
      errs() << "cs is not a call\n";
    return false;
  }

  if (CS.getCalledFunction()) {
    if (ext_log)
      errs() << "cs has a called function\n";
    return false;
  }

  if (!CS.getCalledValue()) {
    if (ext_log)
      errs() << "cs has not a called value\n";
    return false;
  }

  Instruction const *I = CS.getInstruction();
  if (CallInst const *CI = dyn_cast<CallInst>(I)) {
    if (CI->isInlineAsm())
      return false;
  }

  if (isa<Constant>(CS.getCalledValue())) {
     if (ext_log)
         errs() << "cs failed isa conversion test\n";
    return false;
  }

  return true;
}

struct GroundTruth_X86MachineFunction : public MachineFunctionPass {
  static char ID; // Pass identification, replacement for typeid
  GroundTruth_X86MachineFunction() : MachineFunctionPass(ID) {}

  bool runOnMachineFunction(MachineFunction &MF) override {
    auto F = MF.getFunction();
    errs().write_escaped(F->getName()) << "; ";
    errs() << ((F->hasAddressTaken()) ? "<M86AT>" : "<M86FN>") << "; ";
    // Count (and print) the arguments a callee (function) expects
    auto arguments = 0;
    for (auto itr = F->arg_begin(); itr != F->arg_end(); ++itr, ++arguments)
      errs() << (*itr->getType()) << "; ";

    errs() << "parameter_count " << arguments << ";";
    errs() << "return_type " << (*F->getReturnType()) << ";" << '\n';

    bool extended_log = (std::string(F->getName()) == std::string("ngx_http_range_header_filter"));

    // Count (and print) the arguments a callsite provides
    for (auto const &mbb : MF) {
      if (extended_log) {
        errs() <<  mbb << "\n";

        for (auto const &MI : mbb) {
          if (MI.isCall()) {
            if (callsiteOpcodeCheck(MI))
            {
              errs() << "Opcode " << MI.getOpcode() << "\n";
              if (auto bb = mbb.getBasicBlock())
                errs() << (*bb) << "\n";
              else
                errs() << "no IR BB found\n";
            }
          }
        }
      }

      if (auto bb = mbb.getBasicBlock()) {
        for (auto const &instr : *bb) {
          ImmutableCallSite cs(&instr);

          if (isIndirCS(cs, extended_log)) {
            bool is_call = false;
            for (auto const &MI : mbb) {
              if (MI.isCall()) {
                is_call = callsiteOpcodeCheck(MI);
              }
            }

            if (!cs.isMustTailCall()) {
              if (is_call) {
                errs().write_escaped(F->getName()) << "; ";
                errs() << ("<M86CS>") << "; ";
                errs() << "module " << instr.getModule()->getModuleIdentifier() << "; ";

                auto site_arguments = 0;

                for (auto const &arg : cs.args()) {
                  errs() << (*arg->getType()) << "; ";
                  site_arguments++;
                }

                errs() << "parameter_count " << site_arguments << ";";
                errs() << "return_type " << (*cs.getType()) << ";" << '\n';
              }

              {
                errs().write_escaped(F->getName()) << "; ";
                errs() << ("<TM86CS>") << "; ";
                errs() << "module " << instr.getModule()->getModuleIdentifier() << "; ";

                auto site_arguments = 0;

                for (auto const &arg : cs.args()) {
                  errs() << (*arg->getType()) << "; ";
                  site_arguments++;
                }

                errs() << "parameter_count " << site_arguments << ";";
                errs() << "return_type " << (*cs.getType()) << ";" << '\n';
              }
            }

            if (is_call) {
              errs().write_escaped(F->getName()) << "; ";
              errs() << ("<CM86CS>") << "; ";
              errs() << "module " << instr.getModule()->getModuleIdentifier() << "; ";

              auto site_arguments = 0;

              for (auto const &arg : cs.args()) {
                errs() << (*arg->getType()) << "; ";
                site_arguments++;
              }

              errs() << "parameter_count " << site_arguments << ";";
              errs() << "return_type " << (*cs.getType()) << ";" << '\n';
            }

            errs().write_escaped(F->getName()) << "; ";
            errs() << ("<UM86CS>") << "; ";
            errs() << "module " << instr.getModule()->getModuleIdentifier() << "; ";

            auto site_arguments = 0;

            for (auto const &arg : cs.args()) {
              errs() << (*arg->getType()) << "; ";
              site_arguments++;
            }

            errs() << "parameter_count " << site_arguments << ";";
            errs() << "return_type " << (*cs.getType()) << ";" << '\n';
          }
        }
      } else {
        for (auto const &minstr : mbb) {
          if (minstr.isCall()) {
            errs().write_escaped(F->getName()) << "; ";
            errs() << ("<M86CS*>") << "; ";
            errs() << minstr;
            errs() << "\n";
          }
        }
      }
    }
    return false;
  }
};
}

char GroundTruth_X86MachineFunction::ID = 0;
static RegisterPass<GroundTruth_X86MachineFunction>
    XX_function("x86_ground_truth_machine_function",
                "Function analysis pass to generate "
                "the ground truth for callees and call "
                "sites based on X86 machine functions");

FunctionPass *createX86MachineGroundTruth_FunctionPass() {
  return new GroundTruth_X86MachineFunction();
}
};
