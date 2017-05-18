#include "llvm/Analysis/IndirectCallSiteVisitor.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
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

static bool isIndirCS(ImmutableCallSite &CS) {
  if (!CS.isCall())
    return false;

  if (CS.getCalledFunction() || !CS.getCalledValue())
    return false;

  Instruction const *I = CS.getInstruction();
  if (CallInst const *CI = dyn_cast<CallInst>(I)) {
    if (CI->isInlineAsm())
      return false;
  }
  if (isa<Constant>(CS.getCalledValue()))
    return false;

  return true;
}

struct GroundTruthLTO_Function : public FunctionPass {
  static char ID; // Pass identification, replacement for typeid
  GroundTruthLTO_Function() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override {
    auto const F_name = demangle(F.getName()); 
    errs().write_escaped(F_name) << "; ";
    errs() << ((F.hasAddressTaken()) ? "<LAT>" : "<LFN>") << "; ";
    errs() << "module <unknown>" << "; "; //<< instr.getModule()->getModuleIdentifier() << "; ";
    // Count (and print) the arguments a callee (function) expects
    auto arguments = 0;
    for (auto itr = F.arg_begin(); itr != F.arg_end(); ++itr, ++arguments)
      errs() << (*itr->getType()) << "; ";

    errs() << "parameter_count " << arguments << ";";
    errs() << "return_type " << (*F.getReturnType()) << ";" << '\n';

//    auto callsites = findIndirectCallSites(F);
//    // Count (and print) the arguments a callsite provides
//    for (auto const &instr : callsites) {
    for (auto const& bb : F) {
      for (auto const& instr : bb) {
        ImmutableCallSite cs(&instr);

        if (isIndirCS(cs)) {
          {
            errs().write_escaped(F_name) << "; ";
            errs() << ("<LUCS>") << "; ";
            errs() << "module " << instr.getModule()->getModuleIdentifier() << "; ";

            auto site_arguments = 0;

            for (auto const &arg : cs.args()) {
              errs() << (*arg->getType()) << "; ";
              site_arguments++;
            }

            errs() << "parameter_count " << site_arguments << ";";
            errs() << "return_type " << (*cs.getType()) << ";" << '\n';
          }

          if (!cs.isMustTailCall()) {
            errs().write_escaped(F_name) << "; ";
            errs() << ("<LCS>") << "; ";
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
      }
    }
    return false;
  } 
};

};

char GroundTruthLTO_Function::ID = 0;
static RegisterPass<GroundTruthLTO_Function>
    X_function("ground_truthlto__function", "Function analysis pass to generate "
                                        "the ground truth for callees and call "
                                        "sites during LTO");

Pass *createGroundTruthLTO_FunctionPass() { return new GroundTruthLTO_Function(); }
};

