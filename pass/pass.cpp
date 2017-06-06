#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace std;

static cl::list<string> sfParam("sf", cl::desc("Sensitive functions"), cl::OneOrMore);
static cl::opt<int> clParam("cl", cl::desc("Connectivity level"), cl::Required);

namespace {
   struct ResultCheckingPass: public ModulePass {
      static char ID;
      ResultCheckingPass(): ModulePass(ID) {}

      void checkParameters(Module& M) {
        if(clParam < 1 || clParam > 10) {
           fprintf(stderr, "Connectivity level should be between 1 and 10. Exiting.\n");
           exit(1);
        }
        for(string func: sfParam) {
           if(M.getFunction(func) == nullptr) {
              fprintf(stderr, "Function %s is not present in the module. Exiting.\n", func.c_str());
              exit(1);
           }
        }
      }
      virtual bool runOnModule(Module& M) {
         checkParameters(M);
         Module::FunctionListType& functions = M.getFunctionList(); 
         vector<string> not_sens_funcs;
         bool found = false;
         for(Function& f: functions) {
           string name = f.getName();
           for(string sens_func: sfParam) {
              if(name == sens_func) {
                 found = true; 
              }
           }
           if(found == false) {
              not_sens_funcs.push_back(name);               
           }
           found = false;
         }
         for(string func: not_sens_funcs) {
            errs() << func << '\n';
         }
      
      }
   };
}

char ResultCheckingPass::ID = 0;

static RegisterPass<ResultCheckingPass> X("instr", "Result checking instrumentation pass", false, false);
