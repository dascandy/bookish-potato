
#ifdef __aarch64__

void mmu_init() {
asm volatile (
"  MRS    X0, S3_1_C15_C2_1;"
"  ORR    X0, X0, #(0x1 << 6);"
"  MSR    S3_1_C15_C2_1,   X0;"
"  MRS    X0, SCTLR_EL3;"
"  ORR    X0, X0, #0x1       ;"
"  ORR    X0, X0, #(1<<2)       ;"
"  ORR    X0, X0, #(1<<12)       ;"
"  MSR    SCTLR_EL3, X0 ;"
"  DSB    SY ;"
"  ISB");
}

#endif


