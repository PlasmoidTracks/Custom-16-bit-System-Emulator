#ifndef _OPTIMIZER_H_
#define _OPTIMIZER_H_

#include "compiler/asm/assembler.h"

typedef struct {
    int is_instruction;
    char* raw;
    Instruction_t instruction;
} OptimizerToken_t;

extern char* optimizer_compile(char* content);

extern char* optimizer_compile_from_file(const char* filename);

#endif

