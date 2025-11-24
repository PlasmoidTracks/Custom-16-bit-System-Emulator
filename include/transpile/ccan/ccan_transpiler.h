#ifndef _CCAN_TRANSPILER_H_
#define _CCAN_TRANSPILER_H_

#define CCAN_MAX_IDENT 256
#define CCAN_MAX_DEPTH 16     // at most 16 calls deep

#include <stdint.h>
#include "transpile/ccan/ccan_parser.h"

typedef enum {
    CCANCO_KEEP_COMMENTS = 0001, 
    CCANCO_ADD_VARNAMES = 0002, 
    CCANCO_ADD_AST_COMMENTS = 0010,                 // this option adds the translated CCAN line as a comment before the assembly code                (default: 1)
} CCANCompileOption_t;

typedef enum {
    CCAN_TM_STATIC = 1, 
    CCAN_TM_ANON = 2, 
    CCAN_TM_CONST = 4, 
} CCANTypeModifier_t;

typedef struct CCANIdentifier_t {
    char name[256];
    CCANTypeModifier_t type_modifier;
    int is_stack_variable;
    union {
        int stack_offset;
        int absolute_address;
    };
    int initialized;
    int identifier_index;
} CCANIdentifier_t;


extern CCANIdentifier_t ccan_identifier[CCAN_MAX_DEPTH][CCAN_MAX_IDENT];

extern int ccan_identifier_index[CCAN_MAX_DEPTH];

extern int ccan_identifier_scope_depth;

extern char* ccan_compile(CCANParserToken_t** lexer_token, long lexer_token_count, CCANCompileOption_t options);

#endif
