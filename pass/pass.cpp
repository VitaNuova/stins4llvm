#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include <cstdlib>

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
      
      vector<string> findNonSensitive(Module& M) {
         Module::FunctionListType& functions = M.getFunctionList(); 
         vector<string> not_sens_funcs;
         for(Function& f: functions) {
           string name = f.getName();
           if(find(sfParam.begin(), sfParam.end(), name) == sfParam.end()) {
              not_sens_funcs.push_back(name);
           }
         }
         return not_sens_funcs;
      }
 
      vector<string> findProtectingFuncs(string sens_func, vector<string> not_sens_funcs) {
         vector<string> protecting_funcs;
         int rand_num = 0;
         while(!(protecting_funcs.size() == clParam)) {
            rand_num = rand() % not_sens_funcs.size(); 
            if(find(protecting_funcs.begin(), protecting_funcs.end(), not_sens_funcs[rand_num]) == protecting_funcs.end()) {
               protecting_funcs.push_back(not_sens_funcs[rand_num]);
            }
            errs() << sens_func << " will be protected by " << not_sens_funcs[rand_num] << ".\n";
         }           
         return protecting_funcs;
      }

      virtual bool runOnModule(Module& M) {
         checkParameters(M);
         srand(time(0));
         vector<string> not_sens_funcs = findNonSensitive(M);
         for(string sens_func: sfParam) {
            vector<string> protecting_funcs = findProtectingFuncs(sens_func, not_sens_funcs);
         }      
      }
   };
}

char ResultCheckingPass::ID = 0;

static RegisterPass<ResultCheckingPass> X("instr", "Result checking instrumentation pass", false, false);
