#ifndef _CCAN_PARSER_H_
#define _CCAN_PARSER_H_

#include <stdint.h>

#include "compiler/ccan/ccan_lexer.h"


typedef enum {
    CCAN_PAR_UNUSED = CCAN_LEX_TOKEN_COUNT, 
    CCAN_PAR_EXPRESSION, 
    CCAN_PAR_BINARY_OPERATOR, 
    CCAN_PAR_UNARY_OPERATOR, 
    CCAN_PAR_RETURN_STATEMENT, 
    CCAN_PAR_INBUILD_DATATYPE, 
    CCAN_PAR_DATATYPE, 
    CCAN_PAR_FUNCTION_SIGNATURE,
    CCAN_PAR_VARIABLE_SIGNATURE,
    CCAN_PAR_UNDEFINED_VARIABLE_DECLARATION, 
    CCAN_PAR_ASSIGNED_VARIABLE_DECLARATION, 
    CCAN_PAR_VARIABLE_DECLARATION, 
    CCAN_PAR_FUNCTION_PARAMETER, 
    CCAN_PAR_FUNCTION_DECLARATION, 
    CCAN_PAR_STATEMENT, 
    CCAN_PAR_SCOPE_BODY, 
    CCAN_PAR_FUNCTION_DEFINITION, 
    CCAN_PAR_IF_CLAUSE, 
    CCAN_PAR_ELSE_IF_CLAUSE, 
    CCAN_PAR_ELSE_CLAUSE, 
    CCAN_PAR_IF_BRANCH, 
    CCAN_PAR_ELSE_IF_BRANCH, 
    CCAN_PAR_ELSE_BRANCH, 
    CCAN_PAR_IF_CONTROL_FLOW, 
    CCAN_PAR_TYPE_CAST, 
    CCAN_PAR_TYPE_MODIFIER, 

    CCAN_TOKEN_TOTAL_COUNT, 
} CCANParserTokenType_t;

typedef struct CCANParserToken_t {
    CCANLexerToken_t token;       // type
    struct CCANParserToken_t* child[8];
    int child_count;
    struct CCANParserToken_t* parent;
} CCANParserToken_t;

extern void ccan_recursion(CCANParserToken_t* parser_token, int depth);

extern CCANParserToken_t** ccan_parser_parse(CCANLexerToken_t* token, long lexer_token_count, long* parser_root_count);

#endif
