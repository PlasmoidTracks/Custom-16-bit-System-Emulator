#include "cpu/cpu_instructions.h"


const CPU_INSTRUCTION_ENCODING_t instruction_encoding[INSTRUCTION_COUNT] = {
    // Data Manipulation
    [NOP] = {NOP, "nop", 0, 0, 0, 0},
    [MOV] = {MOV, "mov", 2, 0, 0, 0},
    [PUSH] = {PUSH, "push", 1, 0, 0, 0},
    [POP] = {POP, "pop", 1, 1, 0, 0},
    [PUSHSR] = {PUSHSR, "pushsr", 0, 0, 0, 0},
    [POPSR] = {POPSR, "popsr", 0, 0, 0, 0},
    [LEA] = {LEA, "lea", 2, 0, 0, 0},

    // Jumps and Calls
    [JMP] = {JMP, "jmp", 1, 0, 1, 0},
    [JZ] = {JZ, "jz", 1, 0, 1, 0},
    [JNZ] = {JNZ, "jnz", 1, 0, 1, 0},
    [JFZ] = {JFZ, "jfz", 1, 0, 1, 0},
    [JNFZ] = {JNFZ, "jnfz", 1, 0, 1, 0},
    [JL] = {JL, "jl", 1, 0, 1, 0},
    [JNL] = {JNL, "jnl", 1, 0, 1, 0},
    [JUL] = {JUL, "jul", 1, 0, 1, 0},
    [JNUL] = {JNUL, "jnul", 1, 0, 1, 0},
    [JFL] = {JFL, "jfl", 1, 0, 1, 0},
    [JNFL] = {JNFL, "jnfl", 1, 0, 1, 0},
    [JDL] = {JDL, "jdl", 1, 0, 1, 0},
    [JNDL] = {JNDL, "jndl", 1, 0, 1, 0},
    [JLL] = {JLL, "jll", 1, 0, 1, 0},
    [JNLL] = {JNLL, "jnll", 1, 0, 1, 0},
    [JAO] = {JAO, "jao", 1, 0, 1, 0},
    [JNAO] = {JNAO, "jnao", 1, 0, 1, 0},
    [CALL] = {CALL, "call", 1, 0, 1, 0},
    [RET] = {RET, "ret", 0, 0, 1, 0},

    // Relative Jumps and Calls
    [RJMP] = {RJMP, "rjmp", 1, 0, 1, 1},
    [RJZ] = {RJZ, "rjz", 1, 0, 1, 1},
    [RJNZ] = {RJNZ, "rjnz", 1, 0, 1, 1},
    [RJFZ] = {RJFZ, "rjfz", 1, 0, 1, 1},
    [RJNFZ] = {RJNFZ, "rjnfz", 1, 0, 1, 1},
    [RJL] = {RJL, "rjl", 1, 0, 1, 1},
    [RJNL] = {RJNL, "rjnl", 1, 0, 1, 1},
    [RJUL] = {RJUL, "rjul", 1, 0, 1, 1},
    [RJNUL] = {RJNUL, "rjnul", 1, 0, 1, 1},
    [RJFL] = {RJFL, "rjfl", 1, 0, 1, 1},
    [RJNFL] = {RJNFL, "rjnfl", 1, 0, 1, 1},
    [RJDL] = {RJDL, "rjdl", 1, 0, 1, 1},
    [RJNDL] = {RJNDL, "rjndl", 1, 0, 1, 1},
    [RJLL] = {RJLL, "rjll", 1, 0, 1, 1},
    [RJNLL] = {RJNLL, "rjnll", 1, 0, 1, 1},
    [RJAO] = {RJAO, "rjao", 1, 0, 1, 1},
    [RJNAO] = {RJNAO, "rjnao", 1, 0, 1, 1},
    [RCALL] = {RCALL, "rcall", 1, 0, 1, 1},

    // Arithmetic Integer Operations
    [ADD] = {ADD, "add", 2, 0, 0, 0},
    [ADC] = {ADC, "adc", 2, 0, 0, 0},
    [SUB] = {SUB, "sub", 2, 0, 0, 0},
    [SBC] = {SBC, "sbc", 2, 0, 0, 0},
    [MUL] = {MUL, "mul", 2, 0, 0, 0},
    [DIV] = {DIV, "div", 2, 0, 0, 0},
    [NEG] = {NEG, "neg", 1, 1, 0, 0},
    [ABS] = {ABS, "abs", 1, 1, 0, 0},
    [INC] = {INC, "inc", 1, 1, 0, 0},
    [DEC] = {DEC, "dec", 1, 1, 0, 0},

    // Saturated Arithmetic Signed Integer Operations
    [SSA] = {SSA, "ssa", 2, 0, 0, 0},
    [SSS] = {SSS, "sss", 2, 0, 0, 0},
    [SSM] = {SSM, "ssm", 2, 0, 0, 0},

    // Saturated Arithmetic Unsigned Integer Operations
    [USA] = {USA, "usa", 2, 0, 0, 0},
    [USS] = {USS, "uss", 2, 0, 0, 0},
    [USM] = {USM, "usm", 2, 0, 0, 0},

    // Arithmetic Float Operations
    [ADDF] = {ADDF, "addf", 2, 0, 0, 0},
    [SUBF] = {SUBF, "subf", 2, 0, 0, 0},
    [MULF] = {MULF, "mulf", 2, 0, 0, 0},
    [DIVF] = {DIVF, "divf", 2, 0, 0, 0},

    // Arithmetic Double Operations
    [ADDD] = {ADDD, "addd", 2, 0, 0, 0},
    [SUBD] = {SUBD, "subd", 2, 0, 0, 0},
    [MULD] = {MULD, "muld", 2, 0, 0, 0},
    [DIVD] = {DIVD, "divd", 2, 0, 0, 0},

    // Arithmetic Long Operations
    [ADDL] = {ADDL, "addl", 2, 0, 0, 0},
    [SUBL] = {SUBL, "subl", 2, 0, 0, 0},
    [MULL] = {MULL, "mull", 2, 0, 0, 0},
    [DIVL] = {DIVL, "divl", 2, 0, 0, 0},

    // Type Conversion Operations
    [CIF] = {CIF, "cif", 1, 1, 0, 0},
    [CID] = {CID, "cid", 1, 1, 0, 0},
    [CIL] = {CIL, "cil", 1, 1, 0, 0},
    [CFI] = {CFI, "cfi", 1, 1, 0, 0},
    [CFD] = {CFD, "cfd", 1, 1, 0, 0},
    [CFL] = {CFL, "cfl", 1, 1, 0, 0},
    [CDI] = {CDI, "cdi", 1, 1, 0, 0},
    [CDF] = {CDF, "cdf", 1, 1, 0, 0},
    [CDL] = {CDL, "cdl", 1, 1, 0, 0},
    [CLI] = {CLI, "cli", 1, 1, 0, 0},
    [CLF] = {CLF, "clf", 1, 1, 0, 0},
    [CLD] = {CLD, "cld", 1, 1, 0, 0},
    [CBI] = {CBI, "cbi", 1, 1, 0, 0},

    // Bitwise Logic
    [UBS] = {UBS, "ubs", 2, 0, 0, 0},
    [SBS] = {SBS, "sbs", 2, 0, 0, 0},
    [AND] = {AND, "and", 2, 0, 0, 0},
    [OR] = {OR, "or", 2, 0, 0, 0},
    [XOR] = {XOR, "xor", 2, 0, 0, 0},
    [NOT] = {NOT, "not", 1, 1, 0, 0},

    // Tests
    [CMP] = {CMP, "cmp", 2, 0, 0, 0},
    [TST] = {TST, "tst", 1, 0, 0, 0},

    // Status Bit Manipulation
    [CLZ] = {CLZ, "clz", 0, 0, 0, 0},
    [SEZ] = {SEZ, "sez", 0, 0, 0, 0},
    [CLFZ] = {CLFZ, "clfz", 0, 0, 0, 0},
    [SEFZ] = {SEFZ, "sefz", 0, 0, 0, 0},
    [CLL] = {CLL, "cll", 0, 0, 0, 0},
    [SEL] = {SEL, "sel", 0, 0, 0, 0},
    [CLUL] = {CLUL, "clul", 0, 0, 0, 0},
    [SEUL] = {SEUL, "seul", 0, 0, 0, 0},
    [CLFL] = {CLFL, "clfl", 0, 0, 0, 0},
    [SEFL] = {SEFL, "sefl", 0, 0, 0, 0},
    [CLDL] = {CLDL, "cldl", 0, 0, 0, 0},
    [SEDL] = {SEDL, "sedl", 0, 0, 0, 0},
    [CLLL] = {CLLL, "clll", 0, 0, 0, 0},
    [SELL] = {SELL, "sell", 0, 0, 0, 0},
    [CLAO] = {CLAO, "clao", 0, 0, 0, 0},
    [SEAO] = {SEAO, "seao", 0, 0, 0, 0},
    [CLMI] = {CLMI, "clmi", 0, 0, 0, 0},
    [SEMI] = {SEMI, "semi", 0, 0, 0, 0},

    // Conditional Operations
    [CMOVZ] = {CMOVZ, "cmovz", 2, 0, 0, 0},
    [CMOVNZ] = {CMOVNZ, "cmovnz", 2, 0, 0, 0},
    [CMOVFZ] = {CMOVFZ, "cmovfz", 2, 0, 0, 0},
    [CMOVNFZ] = {CMOVNFZ, "cmovnfz", 2, 0, 0, 0},
    [CMOVL] = {CMOVL, "cmovl", 2, 0, 0, 0},
    [CMOVNL] = {CMOVNL, "cmovnl", 2, 0, 0, 0},
    [CMOVUL] = {CMOVUL, "cmovul", 2, 0, 0, 0},
    [CMOVNUL] = {CMOVNUL, "cmovnul", 2, 0, 0, 0},
    [CMOVFL] = {CMOVFL, "cmovfl", 2, 0, 0, 0},
    [CMOVNFL] = {CMOVNFL, "cmovnfl", 2, 0, 0, 0},
    [CMOVDL] = {CMOVDL, "cmovdl", 2, 0, 0, 0},
    [CMOVNDL] = {CMOVNDL, "cmovndl", 2, 0, 0, 0},
    [CMOVLL] = {CMOVLL, "cmovll", 2, 0, 0, 0},
    [CMOVNLL] = {CMOVNLL, "cmovnll", 2, 0, 0, 0},
    [CMOVAO] = {CMOVAO, "cmovao", 2, 0, 0, 0},
    [CMOVNAO] = {CMOVNAO, "cmovnao", 2, 0, 0, 0},
    [CMOVMI] = {CMOVMI, "cmovmi", 2, 0, 0, 0},
    [CMOVNMI] = {CMOVNMI, "cmovnmi", 2, 0, 0, 0},

    // Cache Operations
    [INV] = {INV, "inv", 0, 0, 0, 0},
    [FTC] = {FTC, "ftc", 1, 0, 0, 0},

    // Self Identification and HW-Info Operations
    [HWCLOCK] = {HWCLOCK, "hwclock", 0, 0, 0, 0},
    [HWINSTR] = {HWINSTR, "hwinstr", 0, 0, 0, 0},

    // Other
    [INT] = {INT, "int", 1, 0, 0, 0},
    [HLT] = {HLT, "hlt", 0, 0, 0, 0},

    // Extension
    [EXT] = {EXT, "ext", 0, 0, 0, 0},

    // Extended Instructions
    [EXTNOP] = {EXTNOP, "extnop", 0, 0, 0, 0},
    [EXTNOP2] = {EXTNOP2, "extnop2", 0, 0, 0, 0},
};

