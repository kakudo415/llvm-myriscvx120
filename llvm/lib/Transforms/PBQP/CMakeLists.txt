add_llvm_library( LLVMPBQPSIMD MODULE
  SIMDPass.cpp

  PLUGIN_TOOL
  opt
)