#ifndef _CCAN_LEXER_H_
#define _CCAN_LEXER_H_

#include <stdint.h>

typedef enum {
    // Keywords.
    CCAN_LEX_IF = 1,                // if
    CCAN_LEX_ELSE,                  // else
    CCAN_LEX_FOR,                   // for
    CCAN_LEX_WHILE,                 // while
    CCAN_LEX_CONTINUE,              // continue
    CCAN_LEX_BREAK,                 // break
    CCAN_LEX_RETURN,                // return
    CCAN_LEX_VOID,                  // void
    CCAN_LEX_INT,                   // int
    CCAN_LEX_SHORT,                 // short
    CCAN_LEX_CHAR,                  // char
    CCAN_LEX_LONG,                  // long
    CCAN_LEX_FLOAT,                 // float
    CCAN_LEX_DOUBLE,                // double
    CCAN_LEX_UNSIGNED,              // unsigned
    CCAN_LEX_SIGNED,                // signed
    CCAN_LEX_STRUCT,                // struct
    CCAN_LEX_UNION,                 // union
    CCAN_LEX_TYPEDEF,               // typedef
    CCAN_LEX_ENUM,                  // enum
    CCAN_LEX_SIZEOF,                // sizeof
    CCAN_LEX_STATIC,                // static
    CCAN_LEX_CONST,                 // const
    CCAN_LEX_EXTERN,                // extern
    CCAN_LEX_VOLATILE,              // volatile
    CCAN_LEX_AUTO,                  // auto
    CCAN_LEX_INLINE,                // inline
    CCAN_LEX_REGISTER,              // register
    CCAN_LEX_RESTRICT,              // restrict
    CCAN_LEX_GOTO,                  // goto
    CCAN_LEX_SWITCH,                // switch
    CCAN_LEX_CASE,                  // case
    CCAN_LEX_DEFAULT,               // default
    CCAN_LEX_DO,                    // do

    // Multi-character tokens.
    CCAN_LEX_SHIFT_LEFT_EQUAL,      // <<=
    CCAN_LEX_SHIFT_RIGHT_EQUAL,     // >>=
    CCAN_LEX_BANG_EQUAL,            // !=
    CCAN_LEX_EQUAL_EQUAL,           // ==
    CCAN_LEX_GREATER_EQUAL,         // >=
    CCAN_LEX_LESS_EQUAL,            // <=
    CCAN_LEX_SHIFT_LEFT,            // <<
    CCAN_LEX_SHIFT_RIGHT,           // >>
    CCAN_LEX_LOGICAL_AND,           // &&
    CCAN_LEX_LOGICAL_OR,            // ||
    CCAN_LEX_PLUS_EQUAL,            // +=
    CCAN_LEX_MINUS_EQUAL,           // -=
    CCAN_LEX_STAR_EQUAL,            // *=
    CCAN_LEX_SLASH_EQUAL,           // /=
    CCAN_LEX_MOD_EQUAL,             // %=
    CCAN_LEX_AND_EQUAL,             // &=
    CCAN_LEX_OR_EQUAL,              // |=
    CCAN_LEX_XOR_EQUAL,             // ^=
    CCAN_LEX_INCREMENT,             // ++
    CCAN_LEX_DECREMENT,             // --
    CCAN_LEX_ARROW,                 // ->

    // Single-character tokens.
    CCAN_LEX_GREATER,               // >
    CCAN_LEX_LESS,                  // <
    CCAN_LEX_MINUS,                 // -
    CCAN_LEX_PLUS,                  // +
    CCAN_LEX_SLASH,                 // /
    CCAN_LEX_STAR,                  // *
    CCAN_LEX_LEFT_PARENTHESES,      // (
    CCAN_LEX_RIGHT_PARENTHESES,     // )
    CCAN_LEX_LEFT_BRACE,            // {
    CCAN_LEX_RIGHT_BRACE,           // }
    CCAN_LEX_LEFT_BRACKET,          // [
    CCAN_LEX_RIGHT_BRACKET,         // ]
    CCAN_LEX_COMMA,                 // ,
    CCAN_LEX_COLON,                 // :
    CCAN_LEX_POINT,                 // .
    CCAN_LEX_SEMICOLON,             // ;
    CCAN_LEX_ASSIGN,                // =
    CCAN_LEX_BITWISE_AND,           // &
    CCAN_LEX_BITWISE_OR,            // |
    CCAN_LEX_BITWISE_NOT,           // ~
    CCAN_LEX_BITWISE_XOR,           // ^
    CCAN_LEX_BANG,                  // !
    CCAN_LEX_PERCENT,               // %
    CCAN_LEX_QUESTION_MARK,         // ?
    CCAN_LEX_BACKSLASH,             // \ 

    // Literals.
    CCAN_LEX_IDENTIFIER,            // name
    CCAN_LEX_STRING,                // "..."
    CCAN_LEX_CHAR_LITERAL,          // 'c'
    CCAN_LEX_NUMBER,                // 123, 0xFF, etc.
    CCAN_LEX_COMMENT,               // //... or /* ... */

    // Other.
    CCAN_LEX_EOF,                   // End of file/input
    CCAN_LEX_UNDEFINED,             // Invalid/unrecognized
    CCAN_LEX_RESERVED,              // Reserved

    CCAN_LEX_TOKEN_COUNT, 
} CCANLexerTokenType_t;


typedef struct {
    CCANLexerTokenType_t type; // token type
    const char* raw;        // raw form of the token as-is in source
    int length;
    int index;              // total index in file
    int line;               // line
    int column;             // column in line
} CCANLexerToken_t;


extern const char* ccan_lexer_token_literal[CCAN_LEX_TOKEN_COUNT];

extern const int ccan_lexer_token_has_fixed_form[CCAN_LEX_TOKEN_COUNT];

extern CCANLexerToken_t* ccan_lexer_parse(char* source, long source_length, long* token_count);


#endif
