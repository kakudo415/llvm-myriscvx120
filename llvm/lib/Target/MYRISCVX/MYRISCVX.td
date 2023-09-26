//===-- MYRISCVX.td - Describe the MYRISCVX Target Machine -*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// This is the top level entry point for the MYRISCVX target.
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

//===----------------------------------------------------------------------===//
// Target-dependent interfaces
//===----------------------------------------------------------------------===//

def RV64 : HwMode<"+64bit">; // MYRISCVX64用のスイッチを定義
def RV32 : HwMode<"-64bit">; // MYRISCVX32用のスイッチを定義

include "MYRISCVXRegisterInfo.td" // レジスタ定義を含んだtdファイル
include "MYRISCVXInstrInfo.td"    // 命令定義を含んだtdファイル

def MYRISCVXInstrInfo : InstrInfo;

// MYRISCVXターゲットアーキテクチャを作成する
def MYRISCVX : Target {
  // MYRISCVXInstrInfoは上記で定義したもの。InstrInfoを継承している。
  let InstructionSet = MYRISCVXInstrInfo;
}

def FeatureRV64 : SubtargetFeature<"64bit", "HasRV64", "true", "RV64 support">;

// CPUモデルcpu-rv32/cpu-rv64
// cpu-rv64のときは、FeatureRV64が有効になる
def : ProcessorModel<"cpu-rv32", NoSchedModel, []>;
def : ProcessorModel<"cpu-rv64", NoSchedModel, [FeatureRV64]>;