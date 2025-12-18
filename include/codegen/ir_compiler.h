#ifndef _IR_COMPILER_H_
#define _IR_COMPILER_H_

#define IR_MAX_IDENT 256
#define IR_MAX_DEPTH 16     // at most 16 calls deep

#include <stdint.h>

typedef enum {
    IRCO_KEEP_COMMENTS = 0001, 
    IRCO_ADD_VARNAMES = 0002, 
    IRCO_ADD_AST_COMMENTS = 0010,                 // this option adds the translated IR line as a comment before the assembly code                (default: 1)
} IRCompileOption_t;

typedef enum {
    IR_TM_STATIC = 1, 
    IR_TM_ANON = 2, 
    IR_TM_CONST = 4, 
} IRTypeModifier_t;

typedef struct IRIdentifier_t {
    char name[256];
    IRTypeModifier_t type_modifier;
    int is_stack_variable;
    union {
        int stack_offset;
        int absolute_address;
    };
    int initialized;
    int identifier_index;
} IRIdentifier_t;


extern IRIdentifier_t ir_identifier[IR_MAX_DEPTH][IR_MAX_IDENT];

extern int ir_identifier_index[IR_MAX_DEPTH];

extern int ir_identifier_scope_depth;

extern char* ir_compile(char* source, long source_length, IRCompileOption_t options);

extern char* ir_compile_from_filename(const char* const filename, IRCompileOption_t options);

#endif
