#ifndef _IR_COMPILER_H_
#define _IR_COMPILER_H_

#define IR_MAX_IDENT 256
#define IR_MAX_DEPTH 16     // at most 16 calls deep

#include <stdint.h>

#include "compiler/ir/ir_parser.h"

typedef enum {
    IRCO_POSITION_INDEPENDENT_CODE = 0001, 
    IRCO_ADD_PREAMBLE = 0002, 
    IRCO_KEEP_COMMENTS = 0100, 
    IRCO_ADD_VARNAMES = 0200, 
} IRCompileOption_t;

typedef enum {
    IR_TM_STATIC = 1, 
    IR_TM_ANON = 2, 
    IR_TM_CONST = 4, 
    IR_TM_VOLATILE = 8, 
} IRTypeModifier_t;

typedef struct IRIdentifier_t {
    char name[256];
    IRTypeModifier_t type_modifier;
    int is_stack_variable;
    union {
        int stack_offset;
        int static_offset;
    };
    int initialized;
    int identifier_index;
} IRIdentifier_t;

typedef enum {
    STACK       = 0001, 
    STATIC      = 0002, 
    VOLATILE    = 0004, 
    REGISTER    = 0010, 
    MMIO        = 0020, 
    TEMP        = 0040, 
    AT          = 0100, 
    PADALIGN    = 0200, 
} VariableModifier_t;

typedef enum {
    PERILOGUE   = 0001, 
    ATOMIC      = 0002, 
    REENTRANT   = 0004, 
    INTERRUPT   = 0010, 
    ADDRESS     = 0020, 
    LOCAL       = 0040, 
    ALIGN       = 0100, 
} FunctionModifier_t;

typedef struct Variable_t {
    IRParserToken_t* token; // hold what? Parent Statement?
    VariableModifier_t modifier;
    int at;
    int padalign;
    char* name;
    int size;
    int scope_index;    // -1 is global
    union {
        int offset;     // for stack variables
        int address;    // for static variables
    };
} Variable_t;

typedef struct {
    IRParserToken_t* token; // hold what? Parent Statement?
    FunctionModifier_t modifier;
    int interrupt;
    int align;
    char* name;
    int scope_index;        // same as the variables scope_index for identification/linking purposes
    // epilogue size (how many bytes in the epilogue ends and function body starts)
    // memory layout...?
} Function_t;

// THIS MIGHT NOT EVEN BE NECESSARY, since we track the scope-index of functions and variables now
// TODO: Delete at earliest convenience
typedef enum {
    CT_FUNCTION_BODY, 
    CT_GLOBAL
} ContextType_t;

typedef struct {
    IRParserToken_t* parent;        // temporary, will change
    ContextType_t context;
} CodeGenContext_t;

typedef struct {
    char* assembly;             // the whole assembly script
    long allocated;             // the amount of allocated memory for 'char* assembly'
    long index;                 // the current "cursor" position
    CodeGenContext_t context;   // preserved context between codegen sections
    int tmp;
} CodeGen_t;


extern IRIdentifier_t ir_identifier[IR_MAX_DEPTH][IR_MAX_IDENT];
extern int ir_identifier_index[IR_MAX_DEPTH];
extern int ir_identifier_scope_depth;


extern int show_error_in_syntax(IRParserToken_t* root, IRParserToken_t* AST);

extern void ir_prepass_variable(IRParserToken_t* AST, Variable_t** variable_list_ptr, int* variable_list_length_ptr, int* scope_index_ptr);

extern void ir_prepass_function(IRParserToken_t* AST, Function_t** function_list_ptr, int* function_list_length_ptr, int* scope_index_ptr);

extern char* ir_codegen(IRParserToken_t** roots, int parser_root_count, const Variable_t* variable_list, const int variable_list_lenght, const Function_t* function_list, const int function_list_lenght);

extern char* ir_compile(char* source, long source_length, const char* const source_identifier, IRCompileOption_t options);

extern char* ir_compile_from_filename(const char* const filename, IRCompileOption_t options);

#endif
