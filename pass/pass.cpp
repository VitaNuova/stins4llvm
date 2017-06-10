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
#include <iostream>
#include <fstream>
#include <sstream>

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
  
     void getAnalysisUsage(AnalysisUsage& AU) const override {
        AU.addRequired<CallGraphWrapperPass>();
        AU.setPreservesAll();
     }
  
     bool recurse(const CallGraphNode* node, string candidate) {
        if((*((*node).getFunction())).getBasicBlockList().size() == 0) {
           return false;
        }
        if((*((*node).getFunction())).getName() == candidate) {
           return true;
        }
        bool overall_res = false;
        for(unsigned i = 0; i < (*node).size(); i++) {
           bool res = recurse((*node)[i], candidate);
           overall_res = overall_res || res;
        }
        return overall_res;
     }

     bool checkForCycles(Module& M, string sens_func, string candidate) {
        AnalysisUsage analysis;
        getAnalysisUsage(analysis);
        CallGraphWrapperPass* pass = getAnalysisIfAvailable<CallGraphWrapperPass>();
        const CallGraph& graph = (*pass).getCallGraph();
        Function* fsens = M.getFunction(sens_func);
        const CallGraphNode* sens_func_graph = graph[fsens];
        bool res = recurse(sens_func_graph, candidate);
        return res;
      } 
 
      vector<string> findProtectingFuncs(Module& M, string sens_func, vector<string> not_sens_funcs) {
         vector<string> protecting_funcs;
         int rand_num = 0;
         while(!(protecting_funcs.size() == clParam)) {
            rand_num = rand() % not_sens_funcs.size(); 
            bool cycles = checkForCycles(M, sens_func, not_sens_funcs[rand_num]);
            if(find(protecting_funcs.begin(), protecting_funcs.end(), not_sens_funcs[rand_num]) == protecting_funcs.end() && !cycles) {
               protecting_funcs.push_back(not_sens_funcs[rand_num]);
               errs() << sens_func << " will be protected by " << not_sens_funcs[rand_num] << ".\n";
            }
         }           
         return protecting_funcs;
      }
   
      void checkData(vector<string> tokens, Module& M) {
         if(tokens.size() != 3) {
            fprintf(stderr,"Broken data line. Exiting");
            exit(1);
         }
         if(M.getFunction(tokens[0]) == nullptr) {
            fprintf(stderr, "Function %s is not present in the module. Exiting.\n", (tokens[0]).c_str());
            exit(1);
         }
         try {
           stoi(tokens[1]);
           stoi(tokens[2]);
         } 
         catch(invalid_argument& ex) {
            errs() << "One of the input-output pair is not an integer as required by " << tokens[0] << " .Exiting." << '\n';
            exit(1);  
         } 
      }
      
      Constant* prepareArg(Function* sf_ptr, string input) {
         Type* first_type = (*sf_ptr).getArgumentList().front().getType();
         Constant* arg;
         if((*first_type).isIntegerTy()) {
            arg = ConstantInt::get(first_type, stoi(input)); 
         } else {
            errs() << "Non-integer input types are not yet supported\n";
            exit(1);
         }
         return arg;
      }

     Constant* prepareExpOutput(Function* sf_ptr, string output) {
         Type* return_type = (*sf_ptr).getReturnType();
         Constant* exp_out_val;
         if((*return_type).isIntegerTy()) {
            exp_out_val = ConstantInt::get(return_type, stoi(output)); 
         } else {
            errs() << "Non-integer return types are not yet supported\n";
            exit(1);
         }
         return exp_out_val;
     }
  
      void insertInstrumentation(Module& M, Function* protector, string sens_func, Constant* arg, Constant* exp_out_val) {
         LLVMContext& ctx = M.getContext();
         IRBuilder<> builder(ctx);
        
         Function* sf_ptr = M.getFunction(sens_func);
        
         BasicBlock& last = (*protector).back(); 
         Instruction& last_inst = last.back();
               
         BasicBlock* split = last.splitBasicBlock(&last_inst);
         Instruction& new_last_inst = last.back();
               
         builder.SetInsertPoint(&new_last_inst);
         Value* args[] = {arg};
         Constant* sens_func_const = M.getOrInsertFunction(sens_func, (*sf_ptr).getFunctionType());    
         CallInst* call_inst = builder.CreateCall(sens_func_const, arg);
         AllocaInst* al_inst = builder.CreateAlloca((*sf_ptr).getReturnType());
         StoreInst* store_inst = builder.CreateStore(call_inst, al_inst);
               
         LoadInst* load_inst = builder.CreateLoad(al_inst, "returnval");
         Value* inEqualsOut = builder.CreateICmpEQ(load_inst, exp_out_val, "tmp");
               
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
         newest_last_inst.eraseFromParent();
            
      }

      virtual bool runOnModule(Module& M) {
         checkParameters(M);
         srand(time(0));
         vector<string> not_sens_funcs = findNonSensitive(M);
         string line;
         ifstream data("data.dat");
         if(data.is_open()) {
            while(getline(data, line)) {
               stringstream ss(line);
               vector<string> tokens;
               string buf;
               while(ss >> buf) {
                  tokens.push_back(buf);
               } 
               checkData(tokens, M);      
               string sens_func = tokens[0];
               Function* sf_ptr = M.getFunction(sens_func);
               Constant* arg = prepareArg(sf_ptr, tokens[1]);
               Constant* exp_out = prepareExpOutput(sf_ptr, tokens[2]);
               vector<string> protecting_funcs = findProtectingFuncs(M, sens_func, not_sens_funcs);     
               for(string protecting_func: protecting_funcs) {
                  Function* protector = M.getFunction(protecting_func);
                  protector -> dump();
                  insertInstrumentation(M, protector, sens_func, arg, exp_out);
                  protector -> dump(); 
               }        
            } 
            data.close();          
              
         } else {
            errs() << "Could not open file with data. Exiting.\n";
         }      
      }
   };
}

char ResultCheckingPass::ID = 0;

static RegisterPass<ResultCheckingPass> X("instr", "Result checking instrumentation pass", false, false);
