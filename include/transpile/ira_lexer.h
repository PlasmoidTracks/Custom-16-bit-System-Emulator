#ifndef _IRA_LEXER_H_
#define _IRA_LEXER_H_

#include <stdint.h>

typedef enum {
    // Keywords.
    IRA_LEX_INT16 = 1,              // int16
    IRA_LEX_UINT16,                 // uint16
    IRA_LEX_INT8,                   // int8
    IRA_LEX_UINT8,                  // uint8
    IRA_LEX_INT32,                  // int32
    IRA_LEX_UINT32,                 // uint32
    IRA_LEX_FLOAT16,                // float16
    IRA_LEX_REF,                    // ref
    IRA_LEX_DEREF,                  // deref
    IRA_LEX_IF,                     // if
    IRA_LEX_ELSE,                   // else
    IRA_LEX_GOTO,                   // goto
    IRA_LEX_RETURN,                 // return
    IRA_LEX_ARG,                    // arg
    IRA_LEX_CONST,                  // const
    IRA_LEX_STATIC,                 // static
    IRA_LEX_ANON,                   // anon
    IRA_LEX_CALLPUSHARG,            // callpusharg
    IRA_LEX_CALLFREEARG,            // callfreearg
    IRA_LEX_CALL,                   // call
    IRA_LEX_SCOPEBEGIN,             // scopebegin
    IRA_LEX_SCOPEEND,               // scopeend
    IRA_LEX_ADDRESS,                // .address
    IRA_LEX_IRQBEGIN,               // irqbegin
    IRA_LEX_IRQEND,                 // irqend
    IRA_LEX_ASM,                    // asm


    // Multi-character tokens.
    IRA_LEX_BANG_EQUAL,            // !=
    IRA_LEX_EQUAL_EQUAL,           // ==
    IRA_LEX_GREATER_EQUAL,         // >=
    IRA_LEX_LESS_EQUAL,            // <=
    IRA_LEX_SHIFT_LEFT,            // <<
    IRA_LEX_SHIFT_RIGHT,           // >>

    // Single-character tokens.
    IRA_LEX_MINUS,                 // -
    IRA_LEX_PLUS,                  // +
    IRA_LEX_SLASH,                 // /
    IRA_LEX_STAR,                  // *
    IRA_LEX_GREATER,               // >
    IRA_LEX_LESS,                  // <
    IRA_LEX_LEFT_PARENTH,          // (
    IRA_LEX_RIGHT_PARENTH,         // )
    IRA_LEX_LEFT_BRACKET,          // [
    IRA_LEX_RIGHT_BRACKET,         // ]
    IRA_LEX_COMMA,                 // ,
    IRA_LEX_SEMICOLON,             // ;
    IRA_LEX_ASSIGN,                // =
    IRA_LEX_BITWISE_AND,           // &
    IRA_LEX_BITWISE_OR,            // |
    IRA_LEX_BITWISE_NOT,           // ~
    IRA_LEX_BITWISE_XOR,           // ^
    IRA_LEX_BANG,                  // !
    IRA_LEX_PERCENT,               // %

    // Literals.
    IRA_LEX_IDENTIFIER,            // name
    IRA_LEX_LABEL,                 // .label
    IRA_LEX_STRING,                // "..."
    IRA_LEX_CHAR_LITERAL,          // 'c'
    IRA_LEX_NUMBER,                // 123, 0xFF, etc.
    IRA_LEX_COMMENT,               // //... or /* ... */

    // Other.
    IRA_LEX_EOF,                   // End of file/input
    IRA_LEX_UNDEFINED,             // Invalid/unrecognized
    IRA_LEX_RESERVED,              // Reserved

    IRA_LEX_TOKEN_COUNT, 
} IRALexerTokenType_t;


typedef struct {
    IRALexerTokenType_t type;  // token type
    const char* raw;        // raw form of the token as-is in source
    int length;
    int index;              // total index in file
    int line;               // line
    int column;             // column in line
} IRALexerToken_t;


extern const char* ira_lexer_token_literal[IRA_LEX_TOKEN_COUNT];

extern const int ira_lexer_token_has_fixed_form[IRA_LEX_TOKEN_COUNT];

extern IRALexerToken_t* ira_lexer_parse(char* source, long source_length, long* token_count);


#endif
