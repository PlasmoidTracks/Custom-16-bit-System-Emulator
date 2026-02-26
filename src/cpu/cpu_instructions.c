#include "cpu/cpu_instructions.h"

const char* cpu_instruction_string[INSTRUCTION_COUNT] = {
    // Data Manipulation
    [NOP] = "nop", [MOV] = "mov", [PUSH] = "push", [POP] = "pop", [PUSHSR] = "pushsr", [POPSR] = "popsr", [LEA] = "lea",

    // Jumps and Calls
    [JMP] = "jmp", [JZ] = "jz", [JNZ] = "jnz", [JFZ] = "jfz", [JNFZ] = "jnfz", [JL] = "jl", [JNL] = "jnl", [JUL] = "jul", [JNUL] = "jnul", [JFL] = "jfl", [JNFL] = "jnfl", [JDL] = "jdl", [JNDL] = "jndl", [JLL] = "jll", [JNLL] = "jnll", [JAO] = "jao", [JNAO] = "jnao", [CALL] = "call", [RET] = "ret",

    // Relative Jumps and Calls
    [RJMP] = "rjmp", [RJZ] = "rjz", [RJNZ] = "rjnz", [RJFZ] = "rjfz", [RJNFZ] = "rjnfz", [RJL] = "rjl", [RJNL] = "rjnl", [RJUL] = "rjul", [RJNUL] = "rjnul", [RJFL] = "rjfl", [RJNFL] = "rjnfl", [RJDL] = "rjdl", [RJNDL] = "rjndl", [RJLL] = "rjll", [RJNLL] = "rjnll", [RJAO] = "rjao", [RJNAO] = "rjnao", [RCALL] = "rcall",

    // Arithmetic Integer Operations
    [ADD] = "add", [ADC] = "adc", [SUB] = "sub", [SBC] = "sbc", [MUL] = "mul", [DIV] = "div", [NEG] = "neg", [ABS] = "abs", [INC] = "inc", [DEC] = "dec",

    // Saturated Arithmetic Signed Integer Operations
    [SSA] = "ssa", [SSS] = "sss", [SSM] = "ssm",

    // Saturated Arithmetic Unsigned Integer Operations
    [USA] = "usa", [USS] = "uss", [USM] = "usm",

    // Arithmetic Float Operations
    [ADDF] = "addf", [SUBF] = "subf", [MULF] = "mulf", [DIVF] = "divf",

    // Arithmetic Double Operations
    [ADDD] = "addd", [SUBD] = "subd", [MULD] = "muld", [DIVD] = "divd",

    // Arithmetic Long Operations
    [ADDL] = "addl", [SUBL] = "subl", [MULL] = "mull", [DIVL] = "divl",

    // Type Conversion Operations
    [CIF] = "cif", [CID] = "cid", [CIL] = "cil", [CFI] = "cfi", [CFD] = "cfd", [CFL] = "cfl", [CDF] = "cdf", [CDI] = "cdi", [CDL] = "cdl", [CLI] = "cli", [CLF] = "clf", [CLD] = "cld", [CBI] = "cbi",

    // Bitwise Logic
    [UBS] = "ubs", [SBS] = "sbs", [AND] = "and", [OR] = "or", [XOR] = "xor", [NOT] = "not",

    // Tests
    [CMP] = "cmp", [TST] = "tst",

    // Status Bit Manipulation
    [CLZ] = "clz", [SEZ] = "sez", [CLFZ] = "clfz", [SEFZ] = "sefz", [CLL] = "cll", [SEL] = "sel", [CLUL] = "clul", [SEUL] = "seul",
    [CLFL] = "clfl", [SEFL] = "sefl", [CLDL] = "cldl", [SEDL] = "sedl", [CLLL] = "clll", [SELL] = "sell", [CLAO] = "clao", [SEAO] = "seao", [CLMI] = "clmi", [SEMI] = "semi",

    // Conditional Operations
    [CMOVZ] = "cmovz", [CMOVNZ] = "cmovnz", [CMOVFZ] = "cmovfz", [CMOVNFZ] = "cmovnfz", [CMOVL] = "cmovl", [CMOVNL] = "cmovnl", [CMOVUL] = "cmovul", [CMOVNUL] = "cmovnul",
    [CMOVFL] = "cmovfl", [CMOVNFL] = "cmovnfl", [CMOVDL] = "cmovdl", [CMOVNDL] = "cmovndl", [CMOVLL] = "cmovll", [CMOVNLL] = "cmovnll", [CMOVAO] = "cmovao", [CMOVNAO] = "cmovnao", [CMOVMI] = "cmovmi", [CMOVNMI] = "cmovnmi",

    // Cache Operations
    [INV] = "inv", [FTC] = "ftc",

    // Self Identification and HW-Info Operations
    [HWCLOCK] = "hwclock", [HWINSTR] = "hwinstr",

    // Other
    [INT] = "int", [HLT] = "hlt",

    // Extension
    [EXT] = "ext",

    // Extended Instructions
    [EXTNOP] = "extnop",
    [EXTNOP2] = "extnop2",
};


const int cpu_instruction_argument_count[INSTRUCTION_COUNT] = {
    // Data Manipulation
    [NOP] = 0, [MOV] = 2, [PUSH] = 1, [POP] = 1, [PUSHSR] = 0, [POPSR] = 0, [LEA] = 2, 
    // Jumps and Calls
    [JMP] = 1, [JZ] = 1, [JNZ] = 1, [JFZ] = 1, [JNFZ] = 1, [JL] = 1, [JNL] = 1, [JUL] = 1, [JNUL] = 1, [JFL] = 1, [JNFL] = 1, [JDL] = 1, [JNDL] = 1, [JLL] = 1, [JNLL] = 1, [JAO] = 1, [JNAO] = 1, [CALL] = 1, [RET] = 0, 
    // Relative Jumps
    [RJMP] = 1, [RJZ] = 1, [RJNZ] = 1, [RJFZ] = 1, [RJNFZ] = 1, [RJL] = 1, [RJNL] = 1, [RJUL] = 1, 
    [RJNUL] = 1, [RJFL] = 1, [RJNFL] = 1, [RJDL] = 1, [RJNDL] = 1, [RJLL] = 1, [RJNLL] = 1, [RJAO] = 1, [RJNAO] = 1, [RCALL] = 1, 
    // Arithmetic Integer Operations
    [ADD] = 2, [ADC] = 2, [SUB] = 2, [SBC] = 2, [MUL] = 2, [DIV] = 2, [NEG] = 1, [ABS] = 1, [INC] = 1, [DEC] = 1, 
    // Saturated Arithmetic Signed Integer Operations
    [SSA] = 2, [SSS] = 2, [SSM] = 2, 
    // Saturated Arithmetic Unsigned Integer Operations
    [USA] = 2, [USS] = 2, [USM] = 2, 
    // Arithmetic Float Operations
    [ADDF] = 2, [SUBF] = 2, [MULF] = 2, [DIVF] = 2, 
    // Arithmetic Double Operations
    [ADDD] = 2, [SUBD] = 2, [MULD] = 2, [DIVD] = 2, 
    // Arithmetic Long Operations
    [ADDL] = 2, [SUBL] = 2, [MULL] = 2, [DIVL] = 2, 
    // Type Conversion Operations
    [CIF] = 1, [CID] = 1, [CIL] = 1, [CFI] = 1, [CFD] = 1, [CFL] = 1, [CDF] = 1, [CDI] = 1, [CDL] = 1, [CLI] = 1, [CLF] = 1, [CLD] = 1, [CBI] = 1, 
    // Bitwise Logic
    [UBS] = 2, [SBS] = 2, [AND] = 2, [OR] = 2, [XOR] = 2, [NOT] = 1, 
    // Tests
    [CMP] = 2, [TST] = 1, 
    // Status Bit Manipulation
    [CLZ] = 0, [SEZ] = 0, [CLFZ] = 0, [SEFZ] = 0, [CLL] = 0, [SEL] = 0, [CLUL] = 0, [SEUL] = 0, 
    [CLFL] = 0, [SEFL] = 0, [CLDL] = 0, [SEDL] = 0, [CLLL] = 0, [SELL] = 0, [CLAO] = 0, [SEAO] = 0, [CLMI] = 0, [SEMI] = 0, 
    // Conditional Operations
    [CMOVZ] = 2, [CMOVNZ] = 2, [CMOVFZ] = 2, [CMOVNFZ] = 2, [CMOVL] = 2, [CMOVNL] = 2, [CMOVUL] = 2, [CMOVNUL] = 2, 
    [CMOVFL] = 2, [CMOVNFL] = 2, [CMOVDL] = 2, [CMOVNDL] = 2, [CMOVLL] = 2, [CMOVNLL] = 2, [CMOVAO] = 2, [CMOVNAO] = 2, [CMOVMI] = 2, [CMOVNMI] = 2, 
    // Cache Operations
    [INV] = 0, [FTC] = 1, 
    // Self Identification and HW-Info Operations
    [HWCLOCK] = 0, [HWINSTR] = 0, 
    // Other
    [INT] = 1, [HLT] = 0, 
    // Extension
    [EXT] = 0, 
    // Extended Instructions
    [EXTNOP] = 0, [EXTNOP2] = 0, 
};


// If an entry is one, it only uses admr, else admx
const int cpu_instruction_single_operand_writeback[INSTRUCTION_COUNT] = {
    // Data Manipulation
    [NOP] = 0, [MOV] = 0, [PUSH] = 0, [POP] = 1, [PUSHSR] = 0, [POPSR] = 0, [LEA] = 0, 
    // Jumps and Calls
    [JMP] = 0, [JZ] = 0, [JNZ] = 0, [JFZ] = 0, [JNFZ] = 0, [JL] = 0, [JNL] = 0, [JUL] = 0, [JNUL] = 0, [JFL] = 0, [JNFL] = 0, [JDL] = 0, [JNDL] = 0, [JLL] = 0, [JNLL] = 0, [JAO] = 0, [JNAO] = 0, [CALL] = 0, [RET] = 0, 
    // Relative Jumps
    [RJMP] = 0, [RJZ] = 0, [RJNZ] = 0, [RJFZ] = 0, [RJNFZ] = 0, [RJL] = 0, [RJNL] = 0, [RJUL] = 0, 
    [RJNUL] = 0, [RJFL] = 0, [RJNFL] = 0, [RJDL] = 0, [RJNDL] = 0, [RJLL] = 0, [RJNLL] = 0, [RJAO] = 0, [RJNAO] = 0, [RCALL] = 0, 
    // Arithmetic Integer Operations
    [ADD] = 0, [ADC] = 0, [SUB] = 0, [SBC] = 0, [MUL] = 0, [DIV] = 0, [NEG] = 1, [ABS] = 1, [INC] = 1, [DEC] = 1, 
    // Saturated Arithmetic Signed Integer Operations
    [SSA] = 0, [SSS] = 0, [SSM] = 0, 
    // Saturated Arithmetic Unsigned Integer Operations
    [USA] = 0, [USS] = 0, [USM] = 0, 
    // Arithmetic Float Operations
    [ADDF] = 0, [SUBF] = 0, [MULF] = 0, [DIVF] = 0, 
    // Arithmetic Double Operations
    [ADDD] = 0, [SUBD] = 0, [MULD] = 0, [DIVD] = 0, 
    // Arithmetic Long Operations
    [ADDL] = 0, [SUBL] = 0, [MULL] = 0, [DIVL] = 0, 
    // Type Conversion Operations
    [CIF] = 1, [CID] = 1, [CIL] = 1, [CFI] = 1, [CFD] = 1, [CFL] = 1, [CDF] = 1, [CDI] = 1, [CDL] = 1, [CLI] = 1, [CLF] = 1, [CLD] = 1, [CBI] = 1, 
    // Bitwise Logic
    [UBS] = 0, [SBS] = 0, [AND] = 0, [OR] = 0, [XOR] = 0, [NOT] = 1, 
    // Tests
    [CMP] = 0, [TST] = 0, 
    // Status Bit Manipulation
    [CLZ] = 0, [SEZ] = 0, [CLFZ] = 0, [SEFZ] = 0, [CLL] = 0, [SEL] = 0, [CLUL] = 0, [SEUL] = 0, 
    [CLFL] = 0, [SEFL] = 0, [CLDL] = 0, [SEDL] = 0, [CLLL] = 0, [SELL] = 0, [CLAO] = 0, [SEAO] = 0, [CLMI] = 0, [SEMI] = 0, 
    // Conditional Operations
    [CMOVZ] = 0, [CMOVNZ] = 0, [CMOVFZ] = 0, [CMOVNFZ] = 0, [CMOVL] = 0, [CMOVNL] = 0, [CMOVUL] = 0, [CMOVNUL] = 0, 
    [CMOVFL] = 0, [CMOVNFL] = 0, [CMOVDL] = 0, [CMOVNDL] = 0, [CMOVLL] = 0, [CMOVNLL] = 0, [CMOVAO] = 0, [CMOVNAO] = 0, [CMOVMI] = 0, [CMOVNMI] = 0, 
    // Cache Operations
    [INV] = 0, [FTC] = 0, 
    // Self Identification and HW-Info Operations
    [HWCLOCK] = 0, [HWINSTR] = 0, 
    // Other
    [INT] = 0, [HLT] = 0, 
    // Extension
    [EXT] = 0, 
    // Extended Instructions
    [EXTNOP] = 0, [EXTNOP2] = 0, 
};


// If an entry is one, it is a jump instruction
const int cpu_instruction_is_jump[INSTRUCTION_COUNT] = {
    // Data Manipulation
    [NOP] = 0, [MOV] = 0, [PUSH] = 0, [POP] = 0, [PUSHSR] = 0, [POPSR] = 0, [LEA] = 0, 
    // Jumps and Calls
    [JMP] = 1, [JZ] = 1, [JNZ] = 1, [JFZ] = 1, [JNFZ] = 1, [JL] = 1, [JNL] = 1, [JUL] = 1, [JNUL] = 1, [JFL] = 1, [JNFL] = 1, [JDL] = 1, [JNDL] = 1, [JLL] = 1, [JNLL] = 1, [JAO] = 1, [JNAO] = 1, [CALL] = 1, [RET] = 1, 
    // Relative Jumps
    [RJMP] = 1, [RJZ] = 1, [RJNZ] = 1, [RJFZ] = 1, [RJNFZ] = 1, [RJL] = 1, [RJNL] = 1, [RJUL] = 1, 
    [RJNUL] = 1, [RJFL] = 1, [RJNFL] = 1, [RJDL] = 1, [RJNDL] = 1, [RJLL] = 1, [RJNLL] = 1, [RJAO] = 1, [RJNAO] = 1, [RCALL] = 1, 
    // Arithmetic Integer Operations
    [ADD] = 0, [ADC] = 0, [SUB] = 0, [SBC] = 0, [MUL] = 0, [DIV] = 0, [NEG] = 0, [ABS] = 0, [INC] = 0, [DEC] = 0, 
    // Saturated Arithmetic Signed Integer Operations
    [SSA] = 0, [SSS] = 0, [SSM] = 0, 
    // Saturated Arithmetic Unsigned Integer Operations
    [USA] = 0, [USS] = 0, [USM] = 0, 
    // Arithmetic Float Operations
    [ADDF] = 0, [SUBF] = 0, [MULF] = 0, [DIVF] = 0, 
    // Arithmetic Double Operations
    [ADDD] = 0, [SUBD] = 0, [MULD] = 0, [DIVD] = 0, 
    // Arithmetic Long Operations
    [ADDL] = 0, [SUBL] = 0, [MULL] = 0, [DIVL] = 0, 
    // Type Conversion Operations
    [CIF] = 0, [CID] = 0, [CIL] = 0, [CFI] = 0, [CFD] = 0, [CFL] = 0, [CDF] = 0, [CDI] = 0, [CDL] = 0, [CLI] = 0, [CLF] = 0, [CLD] = 0, [CBI] = 0, 
    // Bitwise Logic
    [UBS] = 0, [SBS] = 0, [AND] = 0, [OR] = 0, [XOR] = 0, [NOT] = 0, 
    // Tests
    [CMP] = 0, [TST] = 0, 
    // Status Bit Manipulation
    [CLZ] = 0, [SEZ] = 0, [CLFZ] = 0, [SEFZ] = 0, [CLL] = 0, [SEL] = 0, [CLUL] = 0, [SEUL] = 0, 
    [CLFL] = 0, [SEFL] = 0, [CLDL] = 0, [SEDL] = 0, [CLLL] = 0, [SELL] = 0, [CLAO] = 0, [SEAO] = 0, [CLMI] = 0, [SEMI] = 0, 
    // Conditional Operations
    [CMOVZ] = 0, [CMOVNZ] = 0, [CMOVFZ] = 0, [CMOVNFZ] = 0, [CMOVL] = 0, [CMOVNL] = 0, [CMOVUL] = 0, [CMOVNUL] = 0, 
    [CMOVFL] = 0, [CMOVNFL] = 0, [CMOVDL] = 0, [CMOVNDL] = 0, [CMOVLL] = 0, [CMOVNLL] = 0, [CMOVAO] = 0, [CMOVNAO] = 0, [CMOVMI] = 0, [CMOVNMI] = 0, 
    // Cache Operations
    [INV] = 0, [FTC] = 0, 
    // Self Identification and HW-Info Operations
    [HWCLOCK] = 0, [HWINSTR] = 0, 
    // Other
    [INT] = 0, [HLT] = 0, 
    // Extension
    [EXT] = 0, 
    // Extended Instructions
    [EXTNOP] = 0, [EXTNOP2] = 0, 
};


// If an entry is one, it is a relative jump instruction and will treat labels differently during assembly
const int cpu_instruction_is_relative_jump[INSTRUCTION_COUNT] = {
    // Data Manipulation
    [NOP] = 0, [MOV] = 0, [PUSH] = 0, [POP] = 0, [PUSHSR] = 0, [POPSR] = 0, [LEA] = 0, 
    // Jumps and Calls
    [JMP] = 0, [JZ] = 0, [JNZ] = 0, [JFZ] = 0, [JNFZ] = 0, [JL] = 0, [JNL] = 0, [JUL] = 0, [JNUL] = 0, [JFL] = 0, [JNFL] = 0, [JDL] = 0, [JNDL] = 0, [JLL] = 0, [JNLL] = 0, [JAO] = 0, [JNAO] = 0, [CALL] = 0, [RET] = 0, 
    // Relative Jumps
    [RJMP] = 1, [RJZ] = 1, [RJNZ] = 1, [RJFZ] = 1, [RJNFZ] = 1, [RJL] = 1, [RJNL] = 1, [RJUL] = 1, 
    [RJNUL] = 1, [RJFL] = 1, [RJNFL] = 1, [RJDL] = 1, [RJNDL] = 1, [RJLL] = 1, [RJNLL] = 1, [RJAO] = 1, [RJNAO] = 1, [RCALL] = 1, 
    // Arithmetic Integer Operations
    [ADD] = 0, [ADC] = 0, [SUB] = 0, [SBC] = 0, [MUL] = 0, [DIV] = 0, [NEG] = 0, [ABS] = 0, [INC] = 0, [DEC] = 0, 
    // Saturated Arithmetic Signed Integer Operations
    [SSA] = 0, [SSS] = 0, [SSM] = 0, 
    // Saturated Arithmetic Unsigned Integer Operations
    [USA] = 0, [USS] = 0, [USM] = 0, 
    // Arithmetic Float Operations
    [ADDF] = 0, [SUBF] = 0, [MULF] = 0, [DIVF] = 0, 
    // Arithmetic Double Operations
    [ADDD] = 0, [SUBD] = 0, [MULD] = 0, [DIVD] = 0, 
    // Arithmetic Long Operations
    [ADDL] = 0, [SUBL] = 0, [MULL] = 0, [DIVL] = 0, 
    // Type Conversion Operations
    [CIF] = 0, [CID] = 0, [CIL] = 0, [CFI] = 0, [CFD] = 0, [CFL] = 0, [CDF] = 0, [CDI] = 0, [CDL] = 0, [CLI] = 0, [CLF] = 0, [CLD] = 0, [CBI] = 0, 
    // Bitwise Logic
    [UBS] = 0, [SBS] = 0, [AND] = 0, [OR] = 0, [XOR] = 0, [NOT] = 0, 
    // Tests
    [CMP] = 0, [TST] = 0, 
    // Status Bit Manipulation
    [CLZ] = 0, [SEZ] = 0, [CLFZ] = 0, [SEFZ] = 0, [CLL] = 0, [SEL] = 0, [CLUL] = 0, [SEUL] = 0, 
    [CLFL] = 0, [SEFL] = 0, [CLDL] = 0, [SEDL] = 0, [CLLL] = 0, [SELL] = 0, [CLAO] = 0, [SEAO] = 0, [CLMI] = 0, [SEMI] = 0, 
    // Conditional Operations
    [CMOVZ] = 0, [CMOVNZ] = 0, [CMOVFZ] = 0, [CMOVNFZ] = 0, [CMOVL] = 0, [CMOVNL] = 0, [CMOVUL] = 0, [CMOVNUL] = 0, 
    [CMOVFL] = 0, [CMOVNFL] = 0, [CMOVDL] = 0, [CMOVNDL] = 0, [CMOVLL] = 0, [CMOVNLL] = 0, [CMOVAO] = 0, [CMOVNAO] = 0, [CMOVMI] = 0, [CMOVNMI] = 0, 
    // Cache Operations
    [INV] = 0, [FTC] = 0, 
    // Self Identification and HW-Info Operations
    [HWCLOCK] = 0, [HWINSTR] = 0, 
    // Other
    [INT] = 0, [HLT] = 0, 
    // Extension
    [EXT] = 0, 
    // Extended Instructions
    [EXTNOP] = 0, [EXTNOP2] = 0, 
};

