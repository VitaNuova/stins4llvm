#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
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
           Function::BasicBlockListType& list = f.getBasicBlockList();
           string name = f.getName();
           if(find(sfParam.begin(), sfParam.end(), name) == sfParam.end() && !list.empty()) {
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
         LLVMContext& ctx = M.getContext();
         IRBuilder<> builder(ctx);
         for(string sens_func: sfParam) {
            Function* sf_ptr = M.getFunction(sens_func);
            vector<string> protecting_funcs = findProtectingFuncs(sens_func, not_sens_funcs);
            Constant* sens_func_const = M.getOrInsertFunction(sens_func, (*sf_ptr).getFunctionType());
            for(string protecting_func: protecting_funcs) {
               Function* protector = M.getFunction(protecting_func);
               protector -> dump();
               BasicBlock& last = (*protector).back(); 
               Instruction& last_inst = last.back();
               errs() << "before split" << '\n';
               last_inst.dump();
               BasicBlock* split = last.splitBasicBlock(&last_inst);
               Instruction& new_last_inst = last.back();
               errs() << "after split" << '\n';
               new_last_inst.dump();
               builder.SetInsertPoint(&new_last_inst);
               Constant* arg = ConstantInt::get(Type::getInt32Ty(ctx), 2);
               Value* args[] = {arg};
               CallInst* call_inst = builder.CreateCall(sens_func_const, arg);
               
               AllocaInst* al_inst = builder.CreateAlloca((*sf_ptr).getReturnType());
               StoreInst* store_inst = builder.CreateStore(call_inst, al_inst);
               
               LoadInst* load_inst = builder.CreateLoad(al_inst, "returnval");
               Value* inEqualsOut = builder.CreateICmpEQ(load_inst, ConstantInt::get(Type::getInt32Ty(ctx), 2), "tmp");
               
               BasicBlock* true_bl = BasicBlock::Create(ctx, "true_bl", protector);
               BasicBlock* false_bl = BasicBlock::Create(ctx, "false_bl", protector);
               BranchInst* br_inst = builder.CreateCondBr(inEqualsOut, true_bl, false_bl);
               
               Constant* exit_func = M.getOrInsertFunction("exit", Type::getVoidTy(ctx), Type::getInt32Ty(ctx), NULL);
               Constant* arg_to_exit = ConstantInt::get(Type::getInt32Ty(ctx), 1);
               Value* args_to_exit[] = {arg_to_exit}; 
               
               builder.SetInsertPoint(true_bl);
               builder.CreateBr(split);
               
               builder.SetInsertPoint(false_bl);
               builder.CreateCall(exit_func, args_to_exit);
               builder.CreateBr(split);
               
               Instruction& newest_last_inst = last.back();
               errs() << "newnewlast" << '\n';
               newest_last_inst.dump();
               newest_last_inst.eraseFromParent();
               protector -> dump();         
            }
         }      
      }
   };
}

char ResultCheckingPass::ID = 0;

static RegisterPass<ResultCheckingPass> X("instr", "Result checking instrumentation pass", false, false);
