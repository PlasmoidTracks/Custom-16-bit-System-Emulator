#ifndef _PARSER_H_
#define _PARSER_H_

#include <stdint.h>

#include "lexer.h"


typedef enum {
    PAR_UNUSED = LEX_TOKEN_COUNT, 
    PAR_EXPRESSION, 
    PAR_BINARY_OPERATOR, 
    PAR_UNARY_OPERATOR, 
    PAR_TOKEN_COUNT, 
    PAR_DEREFERENCE, 
    PAR_TYPE_DECLARATION, 
    PAR_VARIABLE_DECLARATION, 

    TOKEN_TOTAL_COUNT, 
} ParserTokenType_t;

typedef struct ParserToken_t {
    LexerToken_t token;       // type
    struct ParserToken_t* child[8];
    int child_count;
    struct ParserToken_t* parent;
} ParserToken_t;

typedef struct AbstractSyntaxTree_t {
    struct AbstractSyntaxTree_t* child;
    struct AbstractSyntaxTree_t* parent;
    int child_count;
    ParserToken_t* parser_token;
} AbstractSyntaxTree_t;


extern ParserToken_t** parser_parse(LexerToken_t* token, long lexer_token_count, long* parser_root_count);

#endif
