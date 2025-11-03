#ifndef _IRA_PARSER_H_
#define _IRA_PARSER_H_

#include <stdint.h>

#include "transpile/ira_lexer.h"


typedef enum {
    IRA_PAR_UNUSED = IRA_LEX_TOKEN_COUNT, 
    IRA_PAR_TYPE,                           // int16, uint16, float16, etc.
    IRA_PAR_TYPE_MODIFIER,                  // static, anon, const
    IRA_PAR_BINARY_OPERATOR,                // arg '+' 2;
    IRA_PAR_UNARY_OPERATOR,                 // "not expr", "- index2"
    IRA_PAR_EXPRESSION,                     // EXPRESSION OPERATOR EXPRESSION;
    IRA_PAR_VARIABLE_DECLARATION,           // var index1;
    IRA_PAR_VARIABLE_ASSIGNMENT,            // index1 = EXPRESSION;
    IRA_PAR_CONST_VARIABLE_DECLARATION,     // const var index1;    <-- can only be declared once, can only ever hold pointers!!!
    IRA_PAR_DEREF_VARIABLE_ASSIGNMENT,      // deref index1 = EXPRESSION;
    IRA_PAR_VARIABLE_FUNCTION_POINTER_ASSIGNMENT,      // ident = .label;
    IRA_PAR_VARIABLE_REF_STRING_ASSIGNMENT, // var = ref string;
    IRA_PAR_IF,                             // if expr .true;
    IRA_PAR_GOTO,                           // goto .label;
    IRA_PAR_CALLPUSHARG,                    // callpusharg index1;
    IRA_PAR_CALLFREEARG,                    // callfreearg 4;
    IRA_PAR_CALL_LABEL,                     // call .label;
    IRA_PAR_CALL_EXPRESSION,                // call expr;
    IRA_PAR_ADDRESS,                        // .address 1234;
    IRA_PAR_INLINE_ASM,                     // asm "mov r0, 0"

    IRA_PAR_TYPE_CAST, 

    IRA_TOKEN_TOTAL_COUNT, 
} IRAParserTokenType_t;

typedef struct IRAParserToken_t {
    IRALexerToken_t token;       // type
    struct IRAParserToken_t* child[8];
    int child_count;
    struct IRAParserToken_t* parent;
} IRAParserToken_t;

extern IRAParserToken_t** ira_parser_parse(IRALexerToken_t* token, long lexer_token_count, long* parser_root_count);

#endif
