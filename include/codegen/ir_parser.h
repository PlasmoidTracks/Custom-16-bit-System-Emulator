#ifndef _IR_PARSER_H_
#define _IR_PARSER_H_

#include <stdint.h>

#include "codegen/ir_lexer.h"


typedef enum {
    IR_PAR_UNUSED = IR_LEX_TOKEN_COUNT, 
    IR_PAR_TYPE_DEFINITION,                // "var" / "static anon var" / etc.
    IR_PAR_BINARY_OPERATOR,                // arg '+' 2;
    IR_PAR_UNARY_OPERATOR,                 // "not expr", "- index2"
    IR_PAR_EXPRESSION,                     // EXPRESSION OPERATOR EXPRESSION;
    IR_PAR_VARIABLE_DECLARATION,           // var index1;
    IR_PAR_VARIABLE_ASSIGNMENT,            // index1 = EXPRESSION;
    IR_PAR_CONST_VARIABLE_DECLARATION,     // const var index1;    <-- can only be declared once, can only ever hold pointers!!!
    IR_PAR_DEREF_VARIABLE_ASSIGNMENT,      // deref index1 = EXPRESSION;
    IR_PAR_VARIABLE_FUNCTION_POINTER_ASSIGNMENT,      // ident = .label;
    IR_PAR_VARIABLE_REF_STRING_ASSIGNMENT, // var = ref string;
    IR_PAR_IF,                             // if expr .true;
    IR_PAR_GOTO,                           // goto .label;
    IR_PAR_CALLPUSHARG,                    // callpusharg index1;
    IR_PAR_CALLFREEARG,                    // callfreearg 4;
    IR_PAR_CALL_LABEL,                     // call .label;
    IR_PAR_CALL_EXPRESSION,                // call expr;
    IR_PAR_ADDRESS,                        // .address 1234;
    IR_PAR_INLINE_ASM,                     // asm "mov r0, 0"

    IR_TOKEN_TOTAL_COUNT, 
} IRParserTokenType_t;

typedef struct IRParserToken_t {
    IRLexerToken_t token;       // type
    struct IRParserToken_t* child[8];
    int child_count;
    struct IRParserToken_t* parent;
} IRParserToken_t;

extern IRParserToken_t** ir_parser_parse(IRLexerToken_t* token, long lexer_token_count, long* parser_root_count);

#endif
