// Callee Savedなレジスタのリストを作成する
def CSR_LP32 : CalleeSavedRegs<(add RA, SP, FP, S1, (sequence "S%u", 2, 11))
