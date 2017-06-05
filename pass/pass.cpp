#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace std;

static cl::list<string> sfParam("sf", cl::desc("Sensitive functions"), cl::OneOrMore);
static cl::opt<int> clParam("cl", cl::desc("Connectivity level"), cl::Required);

namespace {
   struct ResultCheckingPass: public FunctionPass {
      static char ID;
      ResultCheckingPass(): FunctionPass(ID) {}
      
      virtual bool runOnFunction(Function& F) {
         errs() << "clParam " << clParam << '\n';
         for(string sens_func: sfParam) {
            errs() << "sensFunc " << sens_func << '\n';
         }
      }
   };
}

char ResultCheckingPass::ID = 0;

static RegisterPass<ResultCheckingPass> X("instr", "Result checking instrumentation pass", false, false);
