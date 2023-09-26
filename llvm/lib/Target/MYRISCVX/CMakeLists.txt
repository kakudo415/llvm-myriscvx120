add_llvm_component_group(MYRISCVX)
set(LLVM_TARGET_DEFINITIONS MYRISCVX.td)

# MYRISCVXGenRegisterInfo.incを生成する

# tablegenを利用して、MYRISCVXRegisterInfo.tdからMYRISCVXGenRegisterInfo.incが生成される
tablegen(LLVM MYRISCVXGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM MYRISCVXGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM MYRISCVXGenSubtargetInfo.inc -gen-subtarget)
tablegen(LLVM MYRISCVXGenMCPseudoLowering.inc -gen-pseudo-lowering)

# tablegenを利用して、MYRISCVXCallingConv.tdからMYRISCVXGenCallingConv.incを生成される
tablegen(LLVM MYRISCVXGenCallingConv.inc -gen-callingconv)

add_public_tablegen_target(MYRISCVXCommonTableGen)

add_llvm_target(MYRISCVXCodeGen
  MYRISCVXTargetMachine.cpp
)

add_subdirectory(TargetInfo)
add_subdirectory(MCTargetDesc)
