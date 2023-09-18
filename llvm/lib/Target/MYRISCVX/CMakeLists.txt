add_llvm_component_group(MYRISCVX)
set(LLVM_TARGET_DEFINITIONS MYRISCVX.td)

tablegen(LLVM MYRISCVXGenRegisterInfo.inc -gen-register-info)

add_public_tablegen_target(MYRISCVXCommonTableGen)

add_llvm_target(MYRISCVXCodeGen
  MYRISCVXTargetMachine.cpp
)

add_subdirectory(TargetInfo)
add_subdirectory(MCTargetDesc)