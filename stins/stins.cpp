#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

namespace {
  struct HelloPass : public FunctionPass {
     static char ID;
     HelloPass(): FunctionPass(ID) {}
     virtual bool runOnFunction(Function& f) {
        errs() << "Hello: ";
        errs().write_escaped(f.getName()) << '\n';
     return false;
     }
  };
}

char HelloPass::ID = 0;
static llvm::RegisterPass<HelloPass> X("hello", "Hello", false, false);
