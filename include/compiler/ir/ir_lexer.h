#ifndef _IR_LEXER_H_
#define _IR_LEXER_H_

#include <stdint.h>

typedef enum {
    // Keywords.
    IR_LEX_VAR = 1,                 // var
    IR_LEX_REF,                     // ref
    IR_LEX_DEREF,                   // deref
    IR_LEX_IF,                      // if
    IR_LEX_ELSE,                    // else
    IR_LEX_GOTO,                    // goto
    IR_LEX_RETURN,                  // return
    IR_LEX_ARG,                     // arg
    IR_LEX_CONST,                   // const
    IR_LEX_STATIC,                  // static
    IR_LEX_ANON,                    // anon
    IR_LEX_VOLATILE,                // volatile
    IR_LEX_CALLPUSHARG,             // callpusharg
    IR_LEX_CALLFREEARG,             // callfreearg
    IR_LEX_CALL,                    // call
    IR_LEX_SCOPEBEGIN,              // scopebegin
    IR_LEX_SCOPEEND,                // scopeend
    IR_LEX_CIF,                     // cif
    IR_LEX_CID,                     // cid
    IR_LEX_CIL,                     // cil
    IR_LEX_CFI,                     // cfi
    IR_LEX_CFD,                     // cfd
    IR_LEX_CFL,                     // cfl
    IR_LEX_CDI,                     // cdi
    IR_LEX_CDF,                     // cdf
    IR_LEX_CDL,                     // cdl
    IR_LEX_CLI,                     // cli
    IR_LEX_CLF,                     // clf
    IR_LEX_CLD,                     // cld
    IR_LEX_CBI,                     // cbi
    IR_LEX_ADDRESS,                 // .address
    IR_LEX_IRQBEGIN,                // irqbegin
    IR_LEX_IRQEND,                  // irqend
    IR_LEX_ASM,                     // asm


    // Multi-character tokens.
    IR_LEX_INTEGER_MINUS,                       // i-
    IR_LEX_FLOAT_MINUS,                         // f-
    IR_LEX_DOUBLE_MINUS,                        // d-
    IR_LEX_LONG_MINUS,                          // l-
    IR_LEX_INTEGER_PLUS,                        // i+
    IR_LEX_FLOAT_PLUS,                          // f+
    IR_LEX_DOUBLE_PLUS,                         // d+
    IR_LEX_LONG_PLUS,                           // l+
    IR_LEX_INTEGER_SLASH,                       // i/
    IR_LEX_FLOAT_SLASH,                         // f/
    IR_LEX_DOUBLE_SLASH,                        // d/
    IR_LEX_LONG_SLASH,                          // l/
    IR_LEX_INTEGER_STAR,                        // i*
    IR_LEX_FLOAT_STAR,                          // f*
    IR_LEX_DOUBLE_STAR,                         // d*
    IR_LEX_LONG_STAR,                           // l*
    IR_LEX_BANG_EQUAL,                          // !=
    IR_LEX_EQUAL_EQUAL,                         // ==
    IR_LEX_UNSIGNED_INTEGER_GREATER_EQUAL,      // u>=
    IR_LEX_INTEGER_GREATER_EQUAL,               // i>=
    IR_LEX_FLOAT_GREATER_EQUAL,                 // f>=
    IR_LEX_DOUBLE_GREATER_EQUAL,                // d>=
    IR_LEX_LONG_GREATER_EQUAL,                  // l>=
    IR_LEX_UNSIGNED_INTEGER_LESS_EQUAL,         // u<=
    IR_LEX_INTEGER_LESS_EQUAL,                  // i<=
    IR_LEX_FLOAT_LESS_EQUAL,                    // f<=
    IR_LEX_DOUBLE_LESS_EQUAL,                   // d<=
    IR_LEX_LONG_LESS_EQUAL,                     // l<=
    IR_LEX_UNSIGNED_INTEGER_GREATER,            // u>
    IR_LEX_INTEGER_GREATER,                     // i>
    IR_LEX_FLOAT_GREATER,                       // f>
    IR_LEX_DOUBLE_GREATER,                      // d>
    IR_LEX_LONG_GREATER,                        // l>
    IR_LEX_UNSIGNED_INTEGER_LESS,               // u<
    IR_LEX_INTEGER_LESS,                        // i<
    IR_LEX_FLOAT_LESS,                          // f<
    IR_LEX_DOUBLE_LESS,                         // d<
    IR_LEX_LONG_LESS,                           // l<
    IR_LEX_SHIFT_LEFT,                          // <<
    IR_LEX_SHIFT_RIGHT,                         // >>

    // Single-character tokens.
    IR_LEX_LEFT_BRACKET,          // [
    IR_LEX_RIGHT_BRACKET,         // ]
    IR_LEX_COMMA,                 // ,
    IR_LEX_SEMICOLON,             // ;
    IR_LEX_ASSIGN,                // =
    IR_LEX_BITWISE_AND,           // &
    IR_LEX_BITWISE_OR,            // |
    IR_LEX_BITWISE_NOT,           // ~
    IR_LEX_BITWISE_XOR,           // ^
    IR_LEX_BANG,                  // !
    IR_LEX_PERCENT,               // %

    // Literals.
    IR_LEX_IDENTIFIER,            // name
    IR_LEX_LABEL,                 // .label
    IR_LEX_STRING,                // "..."
    IR_LEX_CHAR_LITERAL,          // 'c'
    IR_LEX_NUMBER,                // 123, 0xFF, etc.
    IR_LEX_COMMENT,               // //... or /* ... */

    // Other.
    IR_LEX_EOF,                   // End of file/input
    IR_LEX_UNDEFINED,             // Invalid/unrecognized
    IR_LEX_RESERVED,              // Reserved

    IR_LEX_TOKEN_COUNT, 
} IRLexerTokenType_t;


typedef struct {
    IRLexerTokenType_t type; // token type
    const char* raw;        // raw form of the token as-is in source
    int length;
    int index;              // total index in file
    int line;               // line
    int column;             // column in line
} IRLexerToken_t;


extern const char* ir_lexer_token_literal[IR_LEX_TOKEN_COUNT];

extern const int ir_lexer_token_has_fixed_form[IR_LEX_TOKEN_COUNT];

extern IRLexerToken_t* ir_lexer_parse(char* source, long source_length, long* token_count);


#endif
