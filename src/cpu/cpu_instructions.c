#include "cpu/cpu_instructions.h"

const char* cpu_instruction_string[INSTRUCTION_COUNT] = {
    // Data Manipulation
    "nop", "mov", "push", "pop", "pushsr", "popsr", "lea",

    // Jumps and Calls
    "jmp", "jz", "jnz", "jl", "jnl", "jul", "jnul", "jfl", "jnfl", "jbl", "jnbl", "jso", "jnso", "jao", "jnao", "call", "ret",

    // Arithmetic Integer Operations
    "add", "sub", "mul", "div", "neg", "abs", "inc", "dec", 

    // Arithmetic Float Operations
    "addf", "subf", "mulf", "divf",

    // Arithmetic BFloat Operations
    "addbf", "subbf", "mulbf", "divbf",

    // Type Conversion Operations
    "cif", "cib", "cfi", "cfb", "cbi", "cbi", "cbw", 

    // Bitwise Logic
    "bws", "and", "or", "xor", "not",

    // Tests
    "cmp", "tst",

    // Status Bit Manipulation
    "clz", "sez", "cll", "sel", "clul", "seul", "clfl", "sefl", "clbl", "sebl", "clso", "seso", "clao", "seao",
    "clsrc", "sesrc", "clswc", "seswc", "clmi", "semi", 

    // Conditional Operations
    "cmovz", "cmovnz", "cmovl", "cmovnl", "cmovul", "cmovnul", "cmovfl", "cmovnfl", "cmovbl", "cmovnbl", 

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
    // Arithmetic Integer Operations
    2, 2, 2, 2, 1, 1, 1, 1, 
    // Arithmetic Float Operations
    2, 2, 2, 2, 
    // Arithmetic BFloat Operations
    2, 2, 2, 2, 
    // Type Conversion Operations
    1, 1, 1, 1, 1, 1, 1, 
    // Bitwise Logic
    2, 2, 2, 2, 1, 
    // Tests
    2, 1, 
    // Status Bit Manipulation
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 
    // Conditional Operations
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
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
    // Arithmetic Integer Operations
    0, 0, 0, 0, 1, 1, 1, 1, 
    // Arithmetic Float Operations
    0, 0, 0, 0, 
    // Arithmetic BFloat Operations
    0, 0, 0, 0, 
    // Type Conversion Operations
    1, 1, 1, 1, 1, 1, 1, 
    // Bitwise Logic
    0, 0, 0, 0, 1, 
    // Tests
    0, 0, 
    // Status Bit Manipulation
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 
    // Conditional Operations
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    // Cache Operations
    0, 0, 
    // Self Identification and HW-Info Operations
    0, 0, 
    // Other
    0, 0
};


// If an entry is one, it only uses admr, else admx
const int cpu_instruction_is_jump[INSTRUCTION_COUNT] = {
    // Data Manipulation
    0, 0, 0, 0, 0, 0, 0, 
    // Jumps and Calls
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    // Arithmetic Integer Operations
    0, 0, 0, 0, 0, 0, 0, 0, 
    // Arithmetic Float Operations
    0, 0, 0, 0, 
    // Arithmetic BFloat Operations
    0, 0, 0, 0, 
    // Type Conversion Operations
    0, 0, 0, 0, 0, 0, 0, 
    // Bitwise Logic
    0, 0, 0, 0, 0, 
    // Tests
    0, 0, 
    // Status Bit Manipulation
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 
    // Conditional Operations
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
    // Cache Operations
    0, 0, 
    // Self Identification and HW-Info Operations
    0, 0, 
    // Other
    0, 0
};

