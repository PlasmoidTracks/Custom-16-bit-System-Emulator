#ifndef _IR_PARSER_H_
#define _IR_PARSER_H_

#include <stdint.h>

#include "compiler/ir/ir_lexer.h"


typedef enum {
    IR_PAR_UNUSED = IR_LEX_TOKEN_COUNT, 
    IR_PAR_ASSIGN, 
    IR_PAR_EXPRESSION,
    IR_PAR_DEREF_EXPRESSION,
    IR_PAR_REF_EXPRESSION,
    IR_PAR_STATEMENT, 
    IR_PAR_TYPE_DEFINITION,                // "var" / "static anon var" / etc.
    IR_PAR_VARIABLE_DECLARATION,           // var index1;
    IR_PAR_R_VALUE, 
    IR_PAR_L_VALUE, 
    IR_PAR_L_OR_R_VALUE, 
    IR_PAR_SCOPEBEGIN, 
    IR_PAR_SCOPEEND, 
    IR_PAR_RETURN, 
    IR_PAR_FUNCTION_DEFINITION, 
    IR_PAR_OPERATOR, 
    IR_PAR_REQUIRE, 
    IR_PAR_PUSHARG, 
    IR_PAR_FREEARG, 
    IR_PAR_SCOPE_MODIFIER, 
    IR_PAR_FUNCTION_MODIFIER, 
    IR_PAR_UNARY_OPERATOR,
    IR_PAR_ROOT, 

    IR_TOKEN_TOTAL_COUNT, 
} IRParserTokenType_t;

typedef struct IRParserToken_t {
    IRLexerToken_t token;       // type
    struct IRParserToken_t* child[8];
    int child_count;
    struct IRParserToken_t* parent;
    int variant;
} IRParserToken_t;

extern IRParserToken_t** ir_parser_parse(char* source, long source_length, long* parser_root_count);

#endif
