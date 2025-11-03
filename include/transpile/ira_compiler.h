#ifndef _IRA_COMPILER_H_
#define _IRA_COMPILER_H_

#define IR_MAX_IDENT 256
#define IR_MAX_DEPTH 16     // at most 16 calls deep

#include <stdint.h>
#include "transpile/ira_parser.h"

typedef enum {
    IRACO_KEEP_COMMENTS = 0001, 
    IRACO_ADD_VARNAMES = 0002, 
    IRACO_ADD_AST_COMMENTS = 0010,                 // this option adds the translated IR line as a comment before the assembly code                (default: 1)
} IRACompileOption_t;

typedef enum {
    IRA_TM_STATIC = 1, 
    IRA_TM_ANON = 2, 
    IRA_TM_CONST = 4, 
} IRATypeModifier_t;

typedef enum {
    IRA_T_INT16 = 1, 
    IRA_T_UINT16, 
    IRA_T_FLOAT16, 
    IRA_T_INT8, 
    IRA_T_UINT8, 
    IRA_T_INT32, 
    IRA_T_UINT32, 
} IRAType_t;

typedef struct IRAIdentifier_t {
    char name[256];
    IRAType_t type;
    IRATypeModifier_t type_modifier;
} IRAIdentifier_t;


extern IRAIdentifier_t ira_identifier[IR_MAX_DEPTH][IR_MAX_IDENT];
extern int ira_identifier_index[IR_MAX_DEPTH];
extern int ira_identifier_scope_depth;

extern char* ira_compile(IRAParserToken_t** lexer_token, long lexer_token_count, IRACompileOption_t options);

#endif
