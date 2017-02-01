#include "llvm/Analysis/IndirectCallSiteVisitor.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {
#define DEBUG_TYPE "callsite_augment"

namespace {
static std::string demangle(std::string const &name) {
  size_t size = 0;
  int status = 0;
  auto demangled_buffer =
      itaniumDemangle(name.c_str(), nullptr, &size, &status);
  if (status == 0) {
    std::string result(demangled_buffer, size);
    free(demangled_buffer);
    return result;
  } else {
    errs() << "Demangle Error" << status << " with symbol " << name << "\n";
    return name;
  }
}

static bool isIndirCS(ImmutableCallSite &CS) {
  if (!CS.isCall()) {
    //    errs() << "cs is not a call\n";
    return false;
  }

  if (CS.getCalledFunction()) {
    //    errs() << "cs has a called function\n";
    return false;
  }

  if (!CS.getCalledValue()) {
    //    errs() << "cs has not a called value\n";
    return false;
  }

  Instruction const *I = CS.getInstruction();
  if (CallInst const *CI = dyn_cast<CallInst>(I)) {
    if (CI->isInlineAsm())
      return false;
  }

  if (isa<Constant>(CS.getCalledValue())) {
    //    errs() << "cs failed isa conversion test\n";
    return false;
  }

  return true;
}

static bool isLLVMInternal(ImmutableCallSite &CS) {
  if (auto fn = CS.getCalledFunction()) {
    auto fn_name = fn->getName();

    if (fn_name == "llvm.dbg.value")
    {
      return true;
    }
    else if (fn_name.find("llvm.dbg.") == 0) {
      errs() << "INTERNAL found callsite with llvm debug function " << fn_name<< "\n";
      return true;
    }
    else if (fn_name.find("llvm.memset") == 0) {return true;}
    else if (fn_name.find("llvm.memcpy") == 0) {return true;}
    else if (fn_name.find("llvm.lifetime") == 0) {return true;
    }
    else if (fn_name.find("llvm.") == 0) {
      errs() << "INTERNAL found callsite with llvm function " << fn_name << "\n";
      return true;
    } 
  }
  return false;
}

struct CallSiteAugment_MachineFunction : public MachineFunctionPass {
  static char ID; // Pass identification, replacement for typeid
  CallSiteAugment_MachineFunction() : MachineFunctionPass(ID) {}

  bool runOnMachineFunction(MachineFunction &MF) override {
    auto F = MF.getFunction();
    // auto const F_name = demangle(F->getName());

    for (auto &mbb : MF) {
      if (auto bb = mbb.getBasicBlock()) {
        for (auto &instr : *bb) {
          auto callsite = ImmutableCallSite(&instr);
          if (callsite.isCall() && !isLLVMInternal(callsite)) {
            auto targetMI = std::find_if(
                mbb.begin(), mbb.end(), [](MachineInstr const &MI) {
                  return MI.isCall() && !MI.getCallSite();
                });
            if (targetMI == mbb.end()) {
              errs().write_escaped(F->getName()) << " PROBLEM: found callsite "
                                                    "in IR but could not match "
                                                    "in MR\n";
            } else {
              errs().write_escaped(F->getName()) << " found callsite ";

              auto site_arguments = 0;

              for (auto const &arg : callsite.args()) {
                errs() << (*arg->getType()) << " ";
                site_arguments++;
              }

              errs() << "parameter_count " << site_arguments << " ";
              errs() << "return_type " << (*callsite.getType()) << " " << '\n';
              targetMI->setCallSite(callsite);
            }
          }
        }
      }
    }
    return true;
  }
};

struct CallSiteAugmentTest_MachineFunction : public MachineFunctionPass {
  static char ID; // Pass identification, replacement for typeid
  const std::string name;
  CallSiteAugmentTest_MachineFunction(std::string name_ = "default")
      : MachineFunctionPass(ID), name(name_) {}

  bool runOnMachineFunction(MachineFunction &MF) override {
    auto F = MF.getFunction();
    // auto const F_name = demangle(F->getName());

    errs().write_escaped(MF.getName());
    errs() << " Run augment test " << name << "\n";

    for (auto &mbb : MF) {
      for (auto &MI : mbb) {
        if ((MI.isCall() && !MI.getCallSite()) ||
            (MI.getCallSite() && !MI.isCall())) {

          if (F)
            errs().write_escaped(F->getName()) << "  F";
          else
            errs().write_escaped(MF.getName()) << " MF";

          errs() << " " << name << " "
                 << " test failed\n";
        }
      }
    }
    return true;
  }
};

enum DumpType {
  NONE,
  IR,
  MR,
};

static const std::array<std::string, 3> type_name = {
    "NONE", "IR", "MR",
};

struct CallSiteDump_MachineFunction : public MachineFunctionPass {
  static char ID; // Pass identification, replacement for typeid
  const DumpType dump_type;
  const std::string name;
  CallSiteDump_MachineFunction(DumpType dump_type_ = NONE,
                               std::string name_ = "default")
      : MachineFunctionPass(ID), dump_type(dump_type_), name(name_) {}

  bool runOnMachineFunction(MachineFunction &MF) override {
    auto F = MF.getFunction();
    // auto const F_name = demangle(F->getName());

    errs().write_escaped(MF.getName());

    errs() << " Run " << type_name[dump_type] << " test " << name << "\n";

    if (MF.getName() == "pg_qsort") {
      for (auto &mbb : MF) {
        if (dump_type == IR) {
          if (auto bb = mbb.getBasicBlock()) {
            errs().write_escaped(MF.getName()) << " IR(BB) " << *bb << "\n";
            for (auto &instr : *bb) {
              errs().write_escaped(MF.getName()) << "\t IR(INSTR) " << instr
                                                 << "\n";
            }
          }
        }

        if (dump_type == MR) {
          errs().write_escaped(MF.getName()) << " MR(BB) " << mbb << "\n";
          for (auto &instr : mbb) {
            errs().write_escaped(MF.getName()) << "\t MR(INSTR) " << instr
                                               << "\n";
          }
        }
      }
    }
    return true;
  }
};
}

char CallSiteAugment_MachineFunction::ID = 0;
static RegisterPass<CallSiteAugment_MachineFunction>
    Augment_function("callsite_augment_machine_function",
                     "Function analysis pass to augment all callsites");

FunctionPass *createMachineCallsiteAugment_FunctionPass() {
  return new CallSiteAugment_MachineFunction();
}

char CallSiteAugmentTest_MachineFunction::ID = 0;
static RegisterPass<CallSiteAugmentTest_MachineFunction> TestAugment_function(
    "callsite_test_augment_machine_function",
    "Function analysis pass to test the augment of all callsites");

FunctionPass *createMachineCallsiteTestAugment_FunctionPass(std::string name) {
  return new CallSiteAugmentTest_MachineFunction(name);
}

char CallSiteDump_MachineFunction::ID = 0;
static RegisterPass<CallSiteDump_MachineFunction>
    DumpData_function("callsite_dump_machine_function",
                      "Function analysis pass to instruction information");

FunctionPass *createMachineCallsiteDumpMRFunctionPass(std::string name) {
  return new CallSiteDump_MachineFunction(IR, name);
}

FunctionPass *createMachineCallsiteDumpIRFunctionPass(std::string name) {
  return new CallSiteDump_MachineFunction(MR, name);
}
};
