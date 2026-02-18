#include "cpu/cpu_instructions.h"

const char* cpu_instruction_string[INSTRUCTION_COUNT] = {
    // Data Manipulation
    "nop", "mov", "push", "pop", "pushsr", "popsr", "lea",

    // Jumps and Calls
    "jmp", "jz", "jnz", "jfz", "jnfz", "jl", "jnl", "jul", "jnul", "jfl", "jnfl", "jbl", "jnbl", "jao", "jnao", "call", "ret",

    // Relative Jumps and Calls
    "rjmp", "rjz", "rjnz", "rjfz", "rjnfz", "rjl", "rjnl", "rjul", "rjnul", "rjfl", "rjnfl", "rjbl", "rjnbl", "rjao", "rjnao", "rcall", 

    // Arithmetic Integer Operations
    "add", "adc", "sub", "sbc", "mul", "div", "neg", "abs", "inc", "dec", 

    // Saturated Arithmetic Signed Integer Operations
    "ssa", "sss", "ssm",

    // Saturated Arithmetic Unsigned Integer Operations
    "usa", "uss", "usm",

    // Arithmetic Float Operations
    "addf", "subf", "mulf", "divf",

    // Arithmetic Double Operations
    "addd", "subd", "muld", "divd",

    // Arithmetic Long Operations
    "addl", "subl", "mull", "divl",

    // Type Conversion Operations
    "cif", "cid", "cil", "cfi", "cfd", "cfl", "cdf", "cdi", "cdl", "cli", "clf", "cld", "cbi", 

    // Bitwise Logic
    "bws", "and", "or", "xor", "not",

    // Tests
    "cmp", "tst",

    // Status Bit Manipulation
    "clz", "sez", "clfz", "sefz", "cll", "sel", "clul", "seul", 
    "clfl", "sefl", "clbl", "sebl", "clao", "seao", "clmi", "semi", 

    // Conditional Operations
    "cmovz", "cmovnz", "cmovfz", "cmovnfz", "cmovl", "cmovnl", "cmovul", "cmovnul", 
    "cmovfl", "cmovnfl", "cmovbl", "cmovnbl", "cmovao", "cmovnao", "cmovmi", "cmovnmi", 

    // Cache Operations
    "inv", "ftc",

    // Self Identification and HW-Info Operations
    "hwclock", "hwinstr", 

    // Other
    "int", "hlt"
};


const int cpu_instruction_argument_count[INSTRUCTION_COUNT] = {
    // Data Manipulation
    0, 2, 1, 1, 0, 0, 2, 
    // Jumps and Calls
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 
    // Relative Jumps
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    // Arithmetic Integer Operations
    2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 
    // Saturated Arithmetic Signed Integer Operations
    2, 2, 2, 
    // Saturated Arithmetic Unsigned Integer Operations
    2, 2, 2,
    // Arithmetic Float Operations
    2, 2, 2, 2, 
    // Arithmetic Double Operations
    2, 2, 2, 2, 
    // Arithmetic Long Operations
    2, 2, 2, 2, 
    // Type Conversion Operations
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    // Bitwise Logic
    2, 2, 2, 2, 1, 
    // Tests
    2, 1, 
    // Status Bit Manipulation
    0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 
    // Conditional Operations
    2, 2, 2, 2, 2, 2, 2, 2, 
    2, 2, 2, 2, 2, 2, 2, 2, 
    // Cache Operations
    0, 1, 
    // Self Identification and HW-Info Operations
    0, 0, 
    // Other
    1, 0
};

// If an entry is one, it only uses admr, else admx
const int cpu_instruction_single_operand_writeback[INSTRUCTION_COUNT] = {
    // Data Manipulation
    0, 0, 0, 1, 0, 0, 0, 
    // Jumps and Calls
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    // Relative Jumps
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    // Arithmetic Integer Operations
    0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 
    // Saturated Arithmetic Signed Integer Operations
    0, 0, 0, 
    // Saturated Arithmetic Unsigned Integer Operations
    0, 0, 0,
    // Arithmetic Float Operations
    0, 0, 0, 0, 
    // Arithmetic Double Operations
    0, 0, 0, 0, 
    // Arithmetic Long Operations
    0, 0, 0, 0, 
    // Type Conversion Operations
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    // Bitwise Logic
    0, 0, 0, 0, 1, 
    // Tests
    0, 0, 
    // Status Bit Manipulation
    0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 
    // Conditional Operations
    0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 
    // Cache Operations
    0, 0, 
    // Self Identification and HW-Info Operations
    0, 0, 
    // Other
    0, 0
};


// If an entry is one, it is a jump instruction
const int cpu_instruction_is_jump[INSTRUCTION_COUNT] = {
    // Data Manipulation
    0, 0, 0, 0, 0, 0, 0, 
    // Jumps and Calls
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    // Relative Jumps
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    // Arithmetic Integer Operations
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    // Saturated Arithmetic Signed Integer Operations
    0, 0, 0, 
    // Saturated Arithmetic Unsigned Integer Operations
    0, 0, 0,
    // Arithmetic Float Operations
    0, 0, 0, 0, 
    // Arithmetic Double Operations
    0, 0, 0, 0, 
    // Arithmetic Long Operations
    0, 0, 0, 0, 
    // Type Conversion Operations
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    // Bitwise Logic
    0, 0, 0, 0, 0, 
    // Tests
    0, 0, 
    // Status Bit Manipulation
    0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 
    // Conditional Operations
    0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 
    // Cache Operations
    0, 0, 
    // Self Identification and HW-Info Operations
    0, 0, 
    // Other
    0, 0
};


// If an entry is one, it is a relative jump instruction and will treat labels differently during assembly
const int cpu_instruction_is_relative_jump[INSTRUCTION_COUNT] = {
    // Data Manipulation
    0, 0, 0, 0, 0, 0, 0, 
    // Jumps and Calls
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    // Relative Jumps
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    // Arithmetic Integer Operations
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    // Saturated Arithmetic Signed Integer Operations
    0, 0, 0, 
    // Saturated Arithmetic Unsigned Integer Operations
    0, 0, 0,
    // Arithmetic Float Operations
    0, 0, 0, 0, 
    // Arithmetic Double Operations
    0, 0, 0, 0, 
    // Arithmetic Long Operations
    0, 0, 0, 0, 
    // Type Conversion Operations
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    // Bitwise Logic
    0, 0, 0, 0, 0, 
    // Tests
    0, 0, 
    // Status Bit Manipulation
    0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 
    // Conditional Operations
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 
    // Cache Operations
    0, 0, 
    // Self Identification and HW-Info Operations
    0, 0, 
    // Other
    0, 0
};

