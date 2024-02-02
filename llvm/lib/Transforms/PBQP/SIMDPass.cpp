#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

#define DEBUG_TYPE "SIMD Instruction Inserter by PBQP"

namespace {
struct PBQPSIMD : public FunctionPass {
  static char ID;
  PBQPSIMD() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override {
    errs() << "Function: " << F.getName() << '\n';
    for (Function::iterator bb = F.begin(); bb != F.end(); bb++) {
      for (BasicBlock::iterator i = bb->begin(); i != bb->end(); i++) {
        errs() << i->getOpcodeName() << '\n';
      }
    }
    return false;
  }
};
}  // namespace

char PBQPSIMD::ID = 0;
static RegisterPass<PBQPSIMD> X("PBQPSIMD",
                                "SIMD Instruction Inserter by PBQP");