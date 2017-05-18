#include "llvm/Analysis/IndirectCallSiteVisitor.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {
#define DEBUG_TYPE "ground_truth"

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

static bool isIndirCS(ImmutableCallSite &CS, std::string fn_name) {
  if (!CS.isCall()) {
    errs().write_escaped(fn_name) << " cs is not a call\n";
    return false;
  }

  if (auto const callee = CS.getCalledFunction()) {
    errs().write_escaped(fn_name) << " cs has a called function ";
    errs().write_escaped(callee->getName()) << "\n";
    return false;
  }

  if (!CS.getCalledValue()) {
    errs().write_escaped(fn_name) << " cs has not a called value\n";
    return false;
  }

  Instruction const *I = CS.getInstruction();
  if (CallInst const *CI = dyn_cast<CallInst>(I)) {
    if (CI->isInlineAsm())
      return false;
  }

  if (isa<Constant>(CS.getCalledValue())) {
    errs().write_escaped(fn_name) << " cs failed isa conversion test\n";
    return false;
  }

  return true;
}

struct GroundTruth_MachineFunction : public MachineFunctionPass {
  static char ID; // Pass identification, replacement for typeid
  GroundTruth_MachineFunction() : MachineFunctionPass(ID) {}

  bool runOnMachineFunction(MachineFunction &MF) override {
    auto function = MF.getFunction();
    auto const fn_name = demangle(function->getName());

    SmallString<128> current_path;
    sys::fs::current_path(current_path);

    auto module =
        (MF.begin())->getBasicBlock()->getModule()->getModuleIdentifier();

    errs().write_escaped(fn_name) << "; ";
    errs() << ((function->hasAddressTaken()) ? "<MATX>" : "<MFNX>") << "; ";
    errs() << "module " << current_path << "/"
           << module << "; ";

    // Count (and print) the arguments a callee (function) expects
    auto arguments = 0;
    for (auto itr = function->arg_begin(); itr != function->arg_end(); ++itr, ++arguments) {
      errs() << (*itr->getType()) << "; ";

      errs() << "parameter_count " << arguments << ";";
      errs() << "return_type " << (*function->getReturnType()) << ";" << '\n';
    }

    // Count (and print) the arguments a callsite provides
    for (auto const &mbb : MF) {
      errs().write_escaped(fn_name) << " BB Start\n";

      for (auto const &MI : mbb) {
        if (MI.isCall()) {
          auto cs = MI.getCallSite();
          if (cs && cs.isCall()) {
            if (isIndirCS(cs,
                          fn_name + " AUGMENT ") /* && !cs.isMustTailCall()*/) {
              errs().write_escaped(fn_name) << "; ";
              errs() << ("<AMCS>") << "; ";
              errs() << "module " << current_path << "/" << module << ";";

              auto site_arguments = 0;

              for (auto const &arg : cs.args()) {
                errs() << (*arg->getType()) << "; ";
                site_arguments++;
              }

              errs() << "parameter_count " << site_arguments << ";";
              errs() << "return_type " << (*cs.getType()) << ";" << '\n';
            } else {
              errs().write_escaped(fn_name) << "; ";
              errs() << ("<DMCS>") << "; ";
              errs() << "module " << current_path << "/" << module << ";";

              auto site_arguments = 0;

              for (auto const &arg : cs.args()) {
                errs() << (*arg->getType()) << "; ";
                site_arguments++;
              }

              errs() << "parameter_count " << site_arguments << ";";
              errs() << "return_type " << (*cs.getType()) << ";" << '\n';
            }
          } else {
            errs() << "Problem regarding callsite augmentation arose in func ";
            errs().write_escaped(fn_name) << "; ";
            errs() << "\n";
          }
        }
      }

      if (auto bb = mbb.getBasicBlock()) {
        for (auto const &instr : *bb) {
          ImmutableCallSite cs(&instr);

          if (isIndirCS(cs, fn_name)) {
            bool is_call = false;
            for (auto const &MI : mbb) {
              if (MI.isCall()) {
                is_call = true;
              }
            }

            auto output_cs_info = [&](std::string tag) {
              errs().write_escaped(fn_name) << "; ";
              errs() << tag << "; ";
              errs() << "module " << current_path << "/" << module << ";";

              auto site_arguments = 0;

              for (auto const &arg : cs.args()) {
                errs() << (*arg->getType()) << "; ";
                site_arguments++;
              }

              errs() << "parameter_count " << site_arguments << ";";
              errs() << "return_type " << (*cs.getType()) << ";" << '\n';
            };

            if (!cs.isMustTailCall()) {
              if (is_call)
                output_cs_info("<MCS>");
              output_cs_info("<TMCS>");
            }
            if (is_call)
              output_cs_info("<CMCS>");
            output_cs_info("<UMCS>");
          }
        }
      } else {
        for (auto const &minstr : mbb) {
          if (minstr.isCall()) {
            errs().write_escaped(fn_name) << "; ";
            errs() << ("<MCS*>") << "; ";
            errs() << minstr;
            errs() << "\n";
          }
        }
      }

      errs().write_escaped(fn_name) << " BB End\n";
    }
    return false;
  }
};

static std::string get_name_from_llvm(std::string fn_name)
{
    if (fn_name.find("llvm.") == 0)
    {
        if (0 == fn_name.find("llvm.memset"))
            return "memset";
	else if (0 == fn_name.find("llvm.memcmp"))
            return "memcmp";
        else if (0 == fn_name.find("llvm.memcpy"))
            return "memcpy";
        else if (0 == fn_name.find("llvm.memmove"))
            return "memmove";

        return "";
    }

    return fn_name;
}

struct GroundTruth_Module : public ModulePass {
  static char ID; // Pass identification, replacement for typeid
  GroundTruth_Module() : ModulePass(ID) {}

  bool runOnModule(Module &module) override {
    SmallString<128> current_path;
    sys::fs::current_path(current_path);

    auto const module_name = module.getModuleIdentifier();

    // Indicate the module was compiled
    errs().write_escaped(module_name) << "; ";
    errs() << "<MMD>" << "; ";
    errs() << current_path << ";" << '\n';

    // Print all functions that are *known* within the module
    for (auto const &function : module.functions()) {
      auto fn_name = get_name_from_llvm(function.getName());
      if (!fn_name.empty()) {
        fn_name = demangle(fn_name);

        errs().write_escaped(fn_name) << "; ";
        errs() << ((function.hasAddressTaken()) ? "<MAT>" : "<MFN>") << "; ";
        errs() << "module " << current_path << "/"
               << module.getModuleIdentifier() << "; ";

        // Count (and print) the arguments a callee (function) expects
        auto arguments = 0;
        for (auto itr = function.arg_begin(); itr != function.arg_end();
             ++itr, ++arguments)
          errs() << (*itr->getType()) << "; ";

        errs() << "parameter_count " << arguments << ";";
        errs() << "return_type " << (*function.getReturnType()) << ";" << '\n';
      }
    }

    return false;
  }
};
}

char GroundTruth_MachineFunction::ID = 0;
char GroundTruth_Module::ID = 0;
static RegisterPass<GroundTruth_MachineFunction>
    XX_function("ground_truth_machine_function",
                "Function analysis pass to generate "
                "the ground truth for callees and call "
                "sites based on machine functions");

static RegisterPass<GroundTruth_MachineFunction>
    XX_module("ground_truth_module", "Module pass to generate "
                                     "the ground truth for address taken");

FunctionPass *createMachineGroundTruth_FunctionPass() {
  return new GroundTruth_MachineFunction();
}

ModulePass *GroundTruth_ModulePass() { return new GroundTruth_Module(); }
}
