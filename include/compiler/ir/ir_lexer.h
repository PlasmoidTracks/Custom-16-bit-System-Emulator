#ifndef _IR_LEXER_H_
#define _IR_LEXER_H_

#include <stdint.h>

typedef enum {
    // Keywords.
    IR_LEX_REF = 1,                 // ref
    IR_LEX_DEREF,                   // deref
    IR_LEX_IF,                      // if
    IR_LEX_GOTO,                    // goto
    IR_LEX_RETURN,                  // return
    IR_LEX_PARG,                    // __parg
    IR_LEX_STACK,                   // stack
    IR_LEX_STATIC,                  // static
    IR_LEX_VOLATILE,                // volatile
    IR_LEX_REGISTER,                // register
    IR_LEX_PUSHARG,                 // pusharg
    IR_LEX_FREEARG,                 // freearg
    IR_LEX_CALL,                    // call
    IR_LEX_PERILOGUE,               // perilogue
    IR_LEX_ATOMIC,                  // atomic
    IR_LEX_REENTRANT,               // reentrant
    IR_LEX_INTERRUPT,               // interrupt
    IR_LEX_LOCAL,                   // local
    IR_LEX_ALIGN,                   // align
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
    IR_LEX_REQUIRE,                 // require
    IR_LEX_IRQBEGIN,                // irqbegin
    IR_LEX_IRQEND,                  // irqend
    IR_LEX_ASM,                     // asm


    // Multi-character tokens.
    IR_LEX_INTEGER_MINUS_EQUAL,                     // i-=
    IR_LEX_UNSIGNED_INTEGER_MINUS_EQUAL,            // u-=
    IR_LEX_SATURATED_INTEGER_MINUS_EQUAL,           // si-=
    IR_LEX_SATURATED_UNSIGNED_INTEGER_MINUS_EQUAL,  // su-=
    IR_LEX_FLOAT_MINUS_EQUAL,                       // f-=
    IR_LEX_DOUBLE_MINUS_EQUAL,                      // d-=
    IR_LEX_LONG_MINUS_EQUAL,                        // l-=

    IR_LEX_INTEGER_PLUS_EQUAL,                     // i+=
    IR_LEX_UNSIGNED_INTEGER_PLUS_EQUAL,            // u+=
    IR_LEX_SATURATED_INTEGER_PLUS_EQUAL,           // si+=
    IR_LEX_SATURATED_UNSIGNED_INTEGER_PLUS_EQUAL,  // su+=
    IR_LEX_FLOAT_PLUS_EQUAL,                       // f+=
    IR_LEX_DOUBLE_PLUS_EQUAL,                      // d+=
    IR_LEX_LONG_PLUS_EQUAL,                        // l+=

    IR_LEX_INTEGER_STAR_EQUAL,                     // i*=
    IR_LEX_UNSIGNED_INTEGER_STAR_EQUAL,            // u*=
    IR_LEX_SATURATED_INTEGER_STAR_EQUAL,           // si*=
    IR_LEX_SATURATED_UNSIGNED_INTEGER_STAR_EQUAL,  // su*=
    IR_LEX_FLOAT_STAR_EQUAL,                       // f*=
    IR_LEX_DOUBLE_STAR_EQUAL,                      // d*=
    IR_LEX_LONG_STAR_EQUAL,                        // l*=

    IR_LEX_INTEGER_SLASH_EQUAL,                     // i/=
    IR_LEX_UNSIGNED_INTEGER_SLASH_EQUAL,            // u/=
    IR_LEX_FLOAT_SLASH_EQUAL,                       // f/=
    IR_LEX_DOUBLE_SLASH_EQUAL,                      // d/=
    IR_LEX_LONG_SLASH_EQUAL,                        // l/=

    IR_LEX_INTEGER_BANG_EQUAL,                  // i!=
    IR_LEX_FLOAT_BANG_EQUAL,                    // f!=
    IR_LEX_DOUBLE_BANG_EQUAL,                   // d!=
    IR_LEX_LONG_BANG_EQUAL,                     // l!=

    IR_LEX_INTEGER_EQUAL_EQUAL,                 // i==
    IR_LEX_FLOAT_EQUAL_EQUAL,                   // f==
    IR_LEX_DOUBLE_EQUAL_EQUAL,                  // d==
    IR_LEX_LONG_EQUAL_EQUAL,                    // l==

    IR_LEX_INTEGER_GREATER_EQUAL,               // i>=
    IR_LEX_UNSIGNED_INTEGER_GREATER_EQUAL,      // u>=
    IR_LEX_FLOAT_GREATER_EQUAL,                 // f>=
    IR_LEX_DOUBLE_GREATER_EQUAL,                // d>=
    IR_LEX_LONG_GREATER_EQUAL,                  // l>=

    IR_LEX_INTEGER_LESS_EQUAL,                  // i<=
    IR_LEX_UNSIGNED_INTEGER_LESS_EQUAL,         // u<=
    IR_LEX_FLOAT_LESS_EQUAL,                    // f<=
    IR_LEX_DOUBLE_LESS_EQUAL,                   // d<=
    IR_LEX_LONG_LESS_EQUAL,                     // l<=

    IR_LEX_INTEGER_GREATER,                     // i>
    IR_LEX_UNSIGNED_INTEGER_GREATER,            // u>
    IR_LEX_FLOAT_GREATER,                       // f>
    IR_LEX_DOUBLE_GREATER,                      // d>
    IR_LEX_LONG_GREATER,                        // l>

    IR_LEX_INTEGER_LESS,                        // i<
    IR_LEX_UNSIGNED_INTEGER_LESS,               // u<
    IR_LEX_FLOAT_LESS,                          // f<
    IR_LEX_DOUBLE_LESS,                         // d<
    IR_LEX_LONG_LESS,                           // l<

    IR_LEX_SHIFT_LEFT_EQUAL,            // <<=
    IR_LEX_SHIFT_RIGHT_EQUAL,           // >>=
    IR_LEX_BITWISE_AND_EQUAL,           // &=
    IR_LEX_BITWISE_OR_EQUAL,            // |=
    IR_LEX_ASSIGN_8_BIT,                // 8=

    // Single-character tokens.
    IR_LEX_LEFT_CURLY_BRACKET,          // {
    IR_LEX_RIGHT_CURLY_BRACKET,         // }
    IR_LEX_SEMICOLON,                   // ;
    IR_LEX_COLON,                       // :
    IR_LEX_BITWISE_NOT_EQUAL,           // ~
    IR_LEX_BITWISE_XOR_EQUAL,           // ^
    IR_LEX_ASSIGN_16_BIT,               // =
    IR_LEX_STAR,                        // *
    IR_LEX_AMPERSAND,                   // &

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

#define MAX_RAW_LENGTH 128

typedef struct {
    IRLexerTokenType_t type; // token type
    char raw[MAX_RAW_LENGTH];        // raw form of the token as-is in source
    int length;
    int index;              // total index in file
    int line;               // line
    int column;             // column in line
} IRLexerToken_t;


extern const char* ir_lexer_token_literal[IR_LEX_TOKEN_COUNT];

extern IRLexerToken_t* ir_lexer_parse(char* source, long source_length, long* token_count);


#endif
