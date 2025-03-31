#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include "arcana/gino/core/AutotunerDoallFilter.hpp"
#include "arcana/gino/core/AutotunerSearchSpace.hpp"
#include "arcana/gino/core/EnablersManager.hpp"
#include "arcana/gino/core/HeuristicsPass.hpp"
#include "arcana/gino/core/Inliner.hpp"
#include "arcana/gino/core/InputOutput.hpp"
#include "arcana/gino/core/Planner.hpp"
#include "arcana/gino/core/Parallelizer.hpp"
#include "arcana/gino/tools/PlanInfo.hpp"
#include "arcana/gino/tools/TimeSaved.hpp"

namespace arcana::gino {

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return { LLVM_PLUGIN_API_VERSION,
           "Gino",
           LLVM_VERSION_STRING,
           [](llvm::PassBuilder &PB) {
             // Register module transformation passes
             PB.registerPipelineParsingCallback(
                 [](llvm::StringRef name,
                    llvm::ModulePassManager &MPM,
                    llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
                   if (name == "autotunerdoallfilter") {
                     MPM.addPass(AutotunerDoallFilter());
                     return true;
                   }
                   if (name == "autotunersearchspace") {
                     MPM.addPass(AutotunerSearchSpace());
                     return true;
                   }
                   if (name == "enablers") {
                     MPM.addPass(EnablersManager());
                     return true;
                   }
                   if (name == "inliner") {
                     MPM.addPass(Inliner());
                     return true;
                   }
                   if (name == "inputoutput") {
                     MPM.addPass(InputOutput());
                     return true;
                   }
                   if (name == "planner") {
                     MPM.addPass(Planner());
                     return true;
                   }
                   if (name == "parallelizer") {
                     MPM.addPass(Parallelizer());
                     return true;
                   }

                   if (name == "TimeSaved") {
                     MPM.addPass(TimeSaved());
                     return true;
                   }
                   if (name == "ParallelizerPlanInfo") {
                     MPM.addPass(PlanInfo());
                     return true;
                   }
                   return false;
                 });

             // Register function transformation passes
             PB.registerPipelineParsingCallback(
                 [](llvm::StringRef name,
                    llvm::FunctionPassManager &FPM,
                    llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
                   return false;
                 });

             // Register module analyses
             PB.registerAnalysisRegistrationCallback(
                 [](llvm::ModuleAnalysisManager &MAM) {
                   MAM.registerPass([&] { return HeuristicsPass(); });
                 });

             // Register function analyses
             PB.registerAnalysisRegistrationCallback(
                 [](llvm::FunctionAnalysisManager &FAM) {

                 });
           } };
}

} // namespace arcana::gino
