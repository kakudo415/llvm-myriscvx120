#include "ScheduleDAGSDNodes.h"
#include "SelectionDAGBuilder.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/BranchProbabilityInfo.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/Analysis/EHPersonalities.h"
#include "llvm/Analysis/LazyBlockFrequencyInfo.h"
#include "llvm/Analysis/LegacyDivergenceAnalysis.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/Analysis/ProfileSummaryInfo.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/AssignmentTrackingAnalysis.h"
#include "llvm/CodeGen/CodeGenCommonISel.h"
#include "llvm/CodeGen/FastISel.h"
#include "llvm/CodeGen/FunctionLoweringInfo.h"
#include "llvm/CodeGen/GCMetadata.h"
#include "llvm/CodeGen/ISDOpcodes.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/MachinePassRegistry.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SchedulerRegistry.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/CodeGen/SelectionDAGNodes.h"
#include "llvm/CodeGen/StackMaps.h"
#include "llvm/CodeGen/StackProtector.h"
#include "llvm/CodeGen/SwiftErrorValueTracking.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/CodeGen/ValueTypes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/IntrinsicsWebAssembly.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Statepoint.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/User.h"
#include "llvm/IR/Value.h"
#include "llvm/InitializePasses.h"
#include "llvm/MC/MCInstrDesc.h"
#include "llvm/Pass.h"
#include "llvm/Support/BranchProbability.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/KnownBits.h"
#include "llvm/Support/MachineValueType.h"
#include "llvm/Support/Timer.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetIntrinsicInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace llvm;

#define DEBUG_TYPE "isel"

class ChoiceNode;
class PsuedoNode;

class ChoiceNode {
public:
  std::vector<PsuedoNode *> Choices;
};

class PsuedoNode {
public:
  unsigned Opcode;
  std::vector<ChoiceNode *> Operands;
};

bool IsLeaf(SelectionDAG *SDAG, std::set<SDNode *> Ignore, SDNode *Node);
std::set<SDNode *> FindLeaves(SelectionDAG *SDAG, std::set<SDNode *> Ignore);

bool IsLeaf(std::set<ChoiceNode *> Ignore, ChoiceNode *Node);
std::set<ChoiceNode *> FindLeaves(std::vector<ChoiceNode *> ChoiceDAG,
                                  std::set<ChoiceNode *> Ignore);

void SelectionDAGISel::InsertSIMDInstructions() {
  std::vector<ChoiceNode *> ChoiceDAG;
  std::vector<PsuedoNode *> PsuedoDAG;

  std::map<SDNode *, PsuedoNode *> SDNodeToPsuedoNode;
  std::map<PsuedoNode *, ChoiceNode *> PsuedoNodeToChoiceNode;

  // Create PsuedoDAG and ChoiceDAG
  std::set<SDNode *> UnreachedSDNodes;
  std::set<SDNode *> SeenSDNodes;

  while (true) {
    std::set<SDNode *> Leaves = FindLeaves(CurDAG, SeenSDNodes);
    UnreachedSDNodes.insert(Leaves.begin(), Leaves.end());

    if (SeenSDNodes.size() == CurDAG->allnodes_size()) {
      break;
    }

    SDNode *CurSDNode = *UnreachedSDNodes.begin();
    UnreachedSDNodes.erase(CurSDNode);
    SeenSDNodes.insert(CurSDNode);

    PsuedoNode *CopyOfCurSDNode = new PsuedoNode();
    CopyOfCurSDNode->Opcode = CurSDNode->getOpcode();
    for (const SDUse &Operand : CurSDNode->ops()) {
      CopyOfCurSDNode->Operands.push_back(
          PsuedoNodeToChoiceNode[SDNodeToPsuedoNode[Operand.get().getNode()]]);
    }
    PsuedoDAG.push_back(CopyOfCurSDNode);
    SDNodeToPsuedoNode[CurSDNode] = CopyOfCurSDNode;

    ChoiceNode *CurChoiceNode = new ChoiceNode();
    CurChoiceNode->Choices.push_back(NULL); // Nop
    CurChoiceNode->Choices.push_back(CopyOfCurSDNode);
    ChoiceDAG.push_back(CurChoiceNode);
    PsuedoNodeToChoiceNode[CopyOfCurSDNode] = CurChoiceNode;
  }

  // Insert SIMD instructions
  std::set<ChoiceNode *> Unreached;
  std::set<ChoiceNode *> Seen;

  while (true) {
    std::set<ChoiceNode *> Leaves = FindLeaves(ChoiceDAG, Seen);
    Unreached.insert(Leaves.begin(), Leaves.end());

    if (Seen.size() == ChoiceDAG.size()) {
      return;
    }

    ChoiceNode *CurChoiceNode = *Unreached.begin();
    Unreached.erase(CurChoiceNode);
    Seen.insert(CurChoiceNode);

    errs() << "CurChoiceNode\n";
    errs() << CurChoiceNode->Choices[1]->Opcode << "\n";
  }
}

std::set<SDNode *> FindLeaves(SelectionDAG *SDAG, std::set<SDNode *> Ignore) {
  std::set<SDNode *> Leaves;
  for (SDNode &Node : SDAG->allnodes()) {
    if (IsLeaf(SDAG, Ignore, &Node)) {
      Leaves.insert(&Node);
    }
  }
  return Leaves;
}

bool IsLeaf(SelectionDAG *SDAG, std::set<SDNode *> Ignore, SDNode *Node) {
  if (Ignore.find(Node) != Ignore.end()) {
    return false;
  }
  for (const SDUse &Operand : Node->ops()) {
    if (Ignore.find(Operand.get().getNode()) == Ignore.end()) {
      return false;
    }
  }
  return true;
}

std::set<ChoiceNode *> FindLeaves(std::vector<ChoiceNode *> ChoiceDAG,
                                  std::set<ChoiceNode *> Ignore) {
  std::set<ChoiceNode *> Leaves;
  for (ChoiceNode *Node : ChoiceDAG) {
    if (IsLeaf(Ignore, Node)) {
      Leaves.insert(Node);
    }
  }
  return Leaves;
}

bool IsLeaf(std::set<ChoiceNode *> Ignore, ChoiceNode *Node) {
  if (Ignore.find(Node) != Ignore.end()) {
    return false;
  }
  for (PsuedoNode *Choice : Node->Choices) {
    if (Choice == NULL) {
      continue;
    }
    for (ChoiceNode *Operand : Choice->Operands) {
      if (Ignore.find(Operand) == Ignore.end()) {
        return false;
      }
    }
  }
  return true;
}
