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
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetLoweringObjectFile.h"
#include "llvm/IR/Mangler.h"

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
#if DEBUG
    errs() << "DEBUG ";
    errs() << "Demangle Error" << status << " with symbol " << name << "\n";
#endif // DEBUG
    return name;
  }
}

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

static bool isIndirCS(ImmutableCallSite const& CS, std::string const& fn_name) {
  if (!CS.isCall()) {
#if DEBUG
    errs() << "DEBUG ";
    errs().write_escaped(fn_name) << " cs is not a call\n";
#endif // DEBUG
    return false;
  }

  if (auto const callee = CS.getCalledFunction()) {
#if DEBUG
    errs() << "DEBUG ";
    errs().write_escaped(fn_name) << " cs has a called function ";
    errs().write_escaped(callee->getName()) << "\n";
#endif // DEBUG
    return false;
  }

  if (!CS.getCalledValue()) {
#if DEBUG
    errs() << "DEBUG ";
    errs().write_escaped(fn_name) << " cs has not a called value\n";
#endif // DEBUG
    return false;
  }

  Instruction const *I = CS.getInstruction();
  if (CallInst const *CI = dyn_cast<CallInst>(I)) {
    if (CI->isInlineAsm())
      return false;
  }

  if (isa<Constant>(CS.getCalledValue())) {
#if DEBUG
    errs() << "DEBUG ";
    errs().write_escaped(fn_name) << " cs failed isa conversion test\n";
#endif // DEBUG
    return false;
  }

  return true;
}

/*
    <MFNX>; $fn_name; $demang_fn_name; $current_path/$module; $flags; $arg_types; $arg_count; $ret_type;
    <MFN>; $fn_name; $demang_fn_name; $current_path/$module; $flags; $arg_types; $arg_count; $ret_type;
*/

static void output_object(Function const& obj, Function const& fn, std::string tag, std::string const& module_path, Module const& module)
{
  auto const fn_name = get_name_from_llvm(fn.getName());
  if (!fn_name.empty()) {
    auto const demang_fn_name = demangle(fn_name);
    errs() << tag << ";";
    errs().write_escaped(fn_name) << ";";
    errs().write_escaped(demang_fn_name) << ";";
    errs() << module_path;
    errs().write_escaped(module.getModuleIdentifier()) << ";";

    errs() << (obj.hasAddressTaken() ? "AT" : "");
    errs() << ";";

    for (auto const &arg : obj.args())
      errs() << (*arg.getType()) << "|";
    errs() << ";";

    errs() << (*obj.getReturnType()) << ";" << '\n';
  } else {
    errs() << "ERROR ";
    errs().write_escaped(fn.getName()) << " is not a valid function name!\n";
  }
}

/*
    <AMCS>; $fn_name; $demang_fn_name; $current_path/$module; $flags; $arg_types; $arg_count; $ret_type;
*/

static void output_object(ImmutableCallSite const& obj, Function const& fn, std::string tag, std::string const& module_path, Module const& module)
{
  auto const fn_name = get_name_from_llvm(fn.getName());
  if (!fn_name.empty()) {
    auto const demang_fn_name = demangle(fn_name);

    auto const is_indir = isIndirCS(obj, fn_name + " AUGMENT ");

    errs() << tag << ";";
    errs().write_escaped(fn_name) << ";";
    errs().write_escaped(demang_fn_name) << ";";
    errs() << module_path;
    errs().write_escaped(module.getModuleIdentifier()) << ";";

    errs() << (is_indir ? "ICS" : "");
    errs() << ";";

    for (auto const &arg : obj.args())
      errs() << (*arg->getType()) << "|";
    errs() << ";"; 

    errs() << *obj.getType() << ";" << '\n';
  } else {
    errs() << "ERROR ";
    errs().write_escaped(fn.getName()) << " is not a valid function name!\n";
  }
}

struct GroundTruth_MachineFunction : public MachineFunctionPass {
  static char ID; // Pass identification, replacement for typeid
  GroundTruth_MachineFunction() : MachineFunctionPass(ID) {}

  bool runOnMachineFunction(MachineFunction &MF) override {
    auto function = MF.getFunction();
    auto const fn_name = /*demangle*/std::string(function->getName());
    auto const mfn_name = std::string(function->getName());

    SmallString<128> current_path_sstr;
    sys::fs::current_path(current_path_sstr);

    auto module =
        (MF.begin())->getBasicBlock()->getModule();

    auto const module_path = current_path_sstr.str().str() + std::string("/");

    // Count (and print) the arguments a callsite provides
    for (auto const &mbb : MF) {
#if DEBUG
      errs() << "DEBUG ";
      errs().write_escaped(fn_name) << " BB Start\n";
#endif // DEBUG
      for (auto const &MI : mbb) {
        if (MI.isCall()) {
          auto cs = MI.getCallSite();
          if (cs && cs.isCall()) {
            output_object(cs, *function, std::string("<AMCS>"), module_path, *module);
          } else {
            errs() << "ERROR ";
            errs() << "Problem regarding callsite augmentation arose in func ";
            errs().write_escaped(fn_name) << ";";
            errs() << "\n";
          }
        }
      }
#if DEBUG
      errs() << "DEBUG ";
      errs().write_escaped(fn_name) << " BB End\n";
#endif // DEBUG
    }

    output_object(*function, *function, std::string("<MFNX>"), module_path, *module);
    return false;
  }
};

struct GroundTruth_Module : public ModulePass {
  static char ID; // Pass identification, replacement for typeid
  GroundTruth_Module() : ModulePass(ID) {}

  bool runOnModule(Module &module) override {
    SmallString<128> current_path_sstr;
    sys::fs::current_path(current_path_sstr);
    auto const module_name = module.getModuleIdentifier();
    auto const module_path = current_path_sstr.str().str() + std::string("/");

    // Print all functions that are *known* within the module
    for (auto const &function : module.functions()) 
      output_object(function, function, std::string("<MFN>"), module_path, module);

    // Indicate the module was compiled
    errs().write_escaped(module_name) << ";";
    errs() << "<MMD>" << ";";
    errs() << current_path_sstr.str() << ";" << '\n';

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
