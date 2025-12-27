#ifndef _CPU_VIRTUAL_INSTRUCTIONS_H_
#define _CPU_VIRTUAL_INSTRUCTIONS_H_

#include "cpu_instructions.h"

#include <stdint.h>

// need an instruction to just push and pop the status register

typedef enum CPU_VIRTUAL_INSTRUCTION_MNEMONIC {
    RESERVED = INSTRUCTION_COUNT, 
    SWP,        // swp dest, src    :: pushsr; xor src, dest; xor dest, srx; xor src, dest; popsr;
    MOD,        // mod dest, src    :: push rX; mov rX, dest; div rX, src; mul rX, src; sub dest, rX; pop rX
    PUSHALL,    // pushall          :: push r0; push r1; push r2; push r3; pushsr
    POPALL,     // popall           :: popsr; pop r3; pop r2; pop r1; pop r0

    VIRTUAL_INSTRUCTION_COUNT // Number of defined instructions
} CPU_VIRTUAL_INSTRUCTION_MNEMONIC_t;


extern const char* cpu_virtual_instruction_string[INSTRUCTION_COUNT];

extern const int cpu_virtual_instruction_argument_count[INSTRUCTION_COUNT];

#endif // _CPU_VIRTUAL_INSTRUCTIONS_H_
