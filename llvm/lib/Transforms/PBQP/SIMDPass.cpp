#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

#define DEBUG_TYPE "SIMD Instruction Inserter by PBQP"

static void addPossibleSIMDInstructions(BasicBlock &BB) {
  for (Instruction &I : BB) {
    errs() << I.getOpcodeName() << '\n';
  }
}

namespace {
struct PBQPSIMD : public FunctionPass {
  static char ID;
  PBQPSIMD() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override {
    for (BasicBlock &BB : F) {
      addPossibleSIMDInstructions(BB);
    }
    return false;
  }
};
}  // namespace

char PBQPSIMD::ID = 0;
static RegisterPass<PBQPSIMD> X("PBQPSIMD",
                                "SIMD Instruction Inserter by PBQP");