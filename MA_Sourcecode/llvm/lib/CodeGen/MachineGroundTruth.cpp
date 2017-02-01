#include "llvm/Analysis/IndirectCallSiteVisitor.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Demangle/Demangle.h"

namespace llvm {
#define DEBUG_TYPE "ground_truth"

namespace {
static std::string demangle(std::string const& name)
{
  size_t size = 0;
  int status = 0;
  auto demangled_buffer = itaniumDemangle(name.c_str(), nullptr, &size, &status);
  if (status == 0)
  {
    std::string result(demangled_buffer, size);
    free(demangled_buffer);
    return result;
  }
  else
  {
    errs() << "Demangle Error" << status << " with symbol " << name << "\n";
    return name;
  }
}

static bool isIndirCS(ImmutableCallSite &CS, std::string F_name) {
  if (!CS.isCall()) {
    errs().write_escaped(F_name) << " cs is not a call\n";
    return false;
  }

  if (auto const callee = CS.getCalledFunction()) {
    errs().write_escaped(F_name) << " cs has a called function ";
    errs().write_escaped(callee->getName()) << "\n";
    return false;
  }

  if (!CS.getCalledValue()) {
    errs().write_escaped(F_name) << " cs has not a called value\n";
    return false;
  }

  Instruction const *I = CS.getInstruction();
  if (CallInst const *CI = dyn_cast<CallInst>(I)) {
    if (CI->isInlineAsm())
      return false;
  }

  if (isa<Constant>(CS.getCalledValue())) {
    errs().write_escaped(F_name) << " cs failed isa conversion test\n";
    return false;
  }

  return true;
}

struct GroundTruth_MachineFunction : public MachineFunctionPass {
  static char ID; // Pass identification, replacement for typeid
  GroundTruth_MachineFunction() : MachineFunctionPass(ID) {}

  bool runOnMachineFunction(MachineFunction &MF) override {
    auto F = MF.getFunction();
    auto const F_name = demangle(F->getName());
    errs().write_escaped(F_name) << "; ";
    errs() << ((F->hasAddressTaken()) ? "<MAT>" : "<MFN>") << "; ";
    errs() << "module <unknown>" << "; "; //<< instr.getModule()->getModuleIdentifier() << "; ";
    // Count (and print) the arguments a callee (function) expects
    auto arguments = 0;
    for (auto itr = F->arg_begin(); itr != F->arg_end(); ++itr, ++arguments)
      errs() << (*itr->getType()) << "; ";

    errs() << "parameter_count " << arguments << ";";
    errs() << "return_type " << (*F->getReturnType()) << ";" << '\n';

    // Count (and print) the arguments a callsite provides
    for (auto const &mbb : MF) {
      errs().write_escaped(F_name) << " BB Start\n";

      for (auto const &MI : mbb) {
        if (MI.isCall())
        {
          auto cs = MI.getCallSite();
          if (cs && cs.isCall())
          {
            if (isIndirCS(cs, F_name + " AUGMENT ")/* && !cs.isMustTailCall()*/) {
                errs().write_escaped(F_name) << "; ";
                errs() << ("<AMCS>") << "; ";
                errs() << "module <unknown>; ";

                auto site_arguments = 0;

                for (auto const &arg : cs.args()) {
                  errs() << (*arg->getType()) << "; ";
                  site_arguments++;
                }

                errs() << "parameter_count " << site_arguments << ";";
                errs() << "return_type " << (*cs.getType()) << ";" << '\n';
            }
            else {

                errs().write_escaped(F_name) << "; ";
                errs() << ("<DMCS>") << "; ";
                errs() << "module <unknown>; ";

                auto site_arguments = 0;

                for (auto const &arg : cs.args()) {
                  errs() << (*arg->getType()) << "; ";
                  site_arguments++;
                }

                errs() << "parameter_count " << site_arguments << ";";
                errs() << "return_type " << (*cs.getType()) << ";" << '\n';
            }
          }
          else
          {
            errs() << "Problem regarding callsite augmentation arose in func ";
            errs().write_escaped(F_name) << "; ";
            errs() << "\n";
          }
        }
      }

      if (auto bb = mbb.getBasicBlock()) {
        for (auto const &instr : *bb) {
          ImmutableCallSite cs(&instr);

          if (isIndirCS(cs, F_name)) {
            bool is_call = false;
            for (auto const &MI : mbb) {
              if (MI.isCall()) {
                is_call = true;
              }
            }

            if (!cs.isMustTailCall()) {
              if (is_call) {
                errs().write_escaped(F_name) << "; ";
                errs() << ("<MCS>") << "; ";
                errs() << "module " << instr.getModule()->getModuleIdentifier() << "; ";

                auto site_arguments = 0;

                for (auto const &arg : cs.args()) {
                  errs() << (*arg->getType()) << "; ";
                  site_arguments++;
                }

                errs() << "parameter_count " << site_arguments << ";";
                errs() << "return_type " << (*cs.getType()) << ";" << '\n';
              }

              errs().write_escaped(F_name) << "; ";
              errs() << ("<TMCS>") << "; ";
              errs() << "module " << instr.getModule()->getModuleIdentifier() << "; ";

              auto site_arguments = 0;

              for (auto const &arg : cs.args()) {
                errs() << (*arg->getType()) << "; ";
                site_arguments++;
              }

              errs() << "parameter_count " << site_arguments << ";";
              errs() << "return_type " << (*cs.getType()) << ";" << '\n';
            }
            if (is_call) {
            errs().write_escaped(F_name) << "; ";
              errs() << ("<CMCS>") << "; ";
              errs() << "module " << instr.getModule()->getModuleIdentifier() << "; ";

              auto site_arguments = 0;

              for (auto const &arg : cs.args()) {
                errs() << (*arg->getType()) << "; ";
                site_arguments++;
              }

              errs() << "parameter_count " << site_arguments << ";";
              errs() << "return_type " << (*cs.getType()) << ";" << '\n';
            }
            errs().write_escaped(F_name) << "; ";
            errs() << ("<UMCS>") << "; ";
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
            errs().write_escaped(F_name) << "; ";
            errs() << ("<MCS*>") << "; ";
            errs() << minstr;
            errs() << "\n";
          }
        }
      }

      errs().write_escaped(F_name) << " BB End\n";
    }
    return false;
  }
};
}

char GroundTruth_MachineFunction::ID = 0;
static RegisterPass<GroundTruth_MachineFunction>
    XX_function("ground_truth_machine_function",
                "Function analysis pass to generate "
                "the ground truth for callees and call "
                "sites based on machine functions");

FunctionPass *createMachineGroundTruth_FunctionPass() {
  return new GroundTruth_MachineFunction();
}
};
