// clang -c -emit-llvm -O2 --target=riscv64-unknown-elf <source>.c
// llc -march=riscv64 <source>.bc

#include "RISCV.h"
#include "llvm/CodeGen/MachineFunctionPass.h"

using namespace llvm;

#define RISCV_BB_PRINTER_PASS_NAME "RISC-V Basic Block Printer Pass"

namespace {
class RISCVBBPrinter : public MachineFunctionPass {
 public:
  static char ID;

  RISCVBBPrinter() : MachineFunctionPass(ID) {
    initializeRISCVBBPrinterPass(*PassRegistry::getPassRegistry());
  }

  bool runOnMachineFunction(MachineFunction &MF) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  StringRef getPassName() const override { return RISCV_BB_PRINTER_PASS_NAME; }
};
}  // namespace

char RISCVBBPrinter::ID = 0;
INITIALIZE_PASS(RISCVBBPrinter, RISCV_BB_PRINTER_PASS_NAME,
                "riscv-basic-block-printer", true, true)

FunctionPass *llvm::createRISCVBBPrinterPass() { return new RISCVBBPrinter(); }

bool RISCVBBPrinter::runOnMachineFunction(MachineFunction &MF) {
  for (auto &MBB : MF) {
    outs() << MBB << "\n";
  }
  return false;
}