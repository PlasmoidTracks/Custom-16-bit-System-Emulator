#ifndef _CCAN_PARSER_H_
#define _CCAN_PARSER_H_

#include <stdint.h>

#include "transpile/ccan/ccan_lexer.h"


typedef enum {
    CCAN_PAR_UNUSED = CCAN_LEX_TOKEN_COUNT, 
    CCAN_PAR_EXPRESSION, 
    CCAN_PAR_BINARY_OPERATOR, 
    CCAN_PAR_UNARY_OPERATOR, 
    CCAN_PAR_RETURN,                // "return;" / "return expr;"
    CCAN_PAR_DATATYPE, 
    CCAN_PAR_FUNCTION_SIGNATURE,    // "int foo ("
    CCAN_PAR_VARIABLE_SIGNATURE,    // "int x"
    CCAN_PAR_UNDEFINED_VARIABLE_DECLARATION, 
    CCAN_PAR_ASSIGNED_VARIABLE_DECLARATION, 
    CCAN_PAR_VARIABLE_DECLARATION, 
    CCAN_PAR_FUNCTION_PARAMETER, 
    CCAN_PAR_FUNCTION_DECLARATION, 

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
