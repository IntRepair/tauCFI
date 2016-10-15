#include "llvm/Transforms/GroundTruth.h"
#include "llvm/Analysis/IndirectCallSiteVisitor.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include <fstream>
#include <mutex>

using namespace llvm;

#define DEBUG_TYPE "ground_truth"

namespace {
struct GroundTruth_Function : public FunctionPass {
  static char ID; // Pass identification, replacement for typeid
  GroundTruth_Function() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override {
    errs().write_escaped(F.getName()) << "; ";
    errs() << ((F.hasAddressTaken()) ? "<AT>" : "<FN>") << "; ";
    // Count (and print) the arguments a callee (function) expects
    auto arguments = 0;
    for (auto itr = F.arg_begin(); itr != F.arg_end(); ++itr, ++arguments)
      errs() << (*itr->getType()) << "; ";

    for (auto i = arguments; i < 6; ++i)
      errs() << "; ";

    errs() << "parameter_count " << arguments << ";";
    errs() << "return_type " << (*F.getReturnType()) << ";" << '\n';

    auto callsites = findIndirectCallSites(F);
    // Count (and print) the arguments a callsite provides
    for (auto const &instr : callsites) {
      errs().write_escaped(F.getName()) << "; ";
      errs() << ("<CS>") << "; ";
      CallSite cs(instr);

      auto site_arguments = 0;

      for (auto const &arg : cs.args()) {
        errs() << (*arg->getType()) << "; ";
        site_arguments++;
      }
      for (auto i = site_arguments; i < 6; ++i)
        errs() << "; ";

      errs() << "parameter_count " << site_arguments << ";";
      errs() << "return_type " << (*cs.getType()) << ";" << '\n';
    }

    return false;
  }
};

struct GroundTruth_Module : public ModulePass {
  static char ID; // Pass identification, replacement for typeid

  GroundTruth_Module() : ModulePass(ID) {}

  bool runOnModule(Module &module) override { return false; }
};
}

char GroundTruth_Function::ID = 0;
char GroundTruth_Module::ID = 1;
static RegisterPass<GroundTruth_Function>
    X_function("ground_truth_function", "Function analysis pass to generate "
                                        "the ground truth for callees and call "
                                        "sites");

static RegisterPass<GroundTruth_Module>
    X_module("ground_truth_module", "Module analysis pass to generate the "
                                    "ground truth for callees and call sites");

namespace llvm
{
FunctionPass * createGroundTruth_FunctionPass ()
{
  return new GroundTruth_Function()
}
};
/*
static void loadPass(const PassManagerBuilder &Builder,
                     legacy::PassManagerBase &PM) {
  PM.add(new GroundTruth_Function());
  PM.add(new GroundTruth_Module());
}

static RegisterStandardPasses
    clangtoolLoader_Ox(PassManagerBuilder::EP_OptimizerLast, loadPass);
static RegisterStandardPasses
    clangtoolLoader_O0(PassManagerBuilder::EP_EnabledOnOptLevel0, loadPass);
*/