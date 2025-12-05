#ifndef _CPU_FEATURES_H_
#define _CPU_FEATURES_H_


// NOP is always available
#define DCFF_CORE_BASE              // MOV, PUSH/POP, PUSHSR/POPSR, JMP/JZ/JNZ, CALL/RET, CMP/TST, AND/OR/XOR/NOT, BWS, INT, HLT
#define DCFF_CORE_LEA               // LEA
#define DCFF_INT_ARITH              // ADD, SUB, NEG, ABS, INC, DEC
#define DCFF_INT_SIGNED_SAT_ARITH   // SAD, SSB, SML
#define DCFF_INT_UNSIGNED_SAT_ARITH // USD, USS, USM
#define DCFF_INT_SAT_ARITH          // SAD, SSB, SML
#define DCFF_INT_ARITH_EXT          // MUL, DIV
#define DCFF_INT_CARRY_EXT          // ADC, SBC
#define DCFF_LOGIC_EXT              // JL/JNL, all flags, CL*/SE* ops
#define DCFF_CMOV_EXT               // CMOVZ…CMOVNBL
#define DCFF_BYTE_EXT               // CBW
#define DCFF_FLOAT16                // ADDF, SUBF, MULF, DIVF
#define DCFF_BFLOAT16               // ADDBF, SUBBF, MULBF, DIVBF
#define DCFF_FLOAT_CONVERT          // CIF, CIB, CFI, CFB, CBF, CBI
#define DCFF_CACHE_EXT              // INV, FTC
#define DCFF_HW_INFO                // HWCLOCK, HWINSTR


typedef enum CpuFeatureFlag_t {
    CFF_BASE                    = 0x0001,   // MOV, PUSH/POP, PUSHSR/POPSR, LEA, JMP/JZ/JNZ, CALL/RET, CMP/TST, AND/OR/XOR/NOT, BWS, INT, HLT
    CFF_INT_ARITH               = 0x0002,   // ADD, SUB, NEG, ABS, INC, DEC
    CFF_INT_SIGNED_SAT_ARITH    = 0x0004,   // SAD, SSB, SML
    CFF_INT_UNSIGNED_SAT_ARITH  = 0x0008,   // USD, USS, USM
    CFF_INT_ARITH_EXT           = 0x0010,   // MUL, DIV
    CFF_INT_CARRY_EXT           = 0x0020,   // ADC, SBC
    CFF_LOGIC_EXT               = 0x0040,   // JL/JNL, all flags, CL*/SE* ops
    CFF_CMOV_EXT                = 0x0080,   // CMOVZ…CMOVNBL
    CFF_BYTE_EXT                = 0x0100,   // CBW
    CFF_FLOAT16                 = 0x0200,   // ADDF, SUBF, MULF, DIVF
    CFF_BFLOAT16                = 0x0400,   // ADDBF, SUBBF, MULBF, DIVBF
    CFF_FLOAT_CONVERT           = 0x0800,   // CIF, CIB, CFI, CFB, CBF, CBI
    CFF_CACHE_EXT               = 0x1000,   // INV, FTC
    CFF_HW_INFO                 = 0x2000,   // HWCLOCK, HWINSTR, HWFFLAG
} CpuFeatureFlag_t;


extern const char* cpu_feature_flag_name[16];

#endif
