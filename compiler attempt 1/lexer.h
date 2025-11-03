#ifndef _LEXER_H_
#define _LEXER_H_

#include <stdint.h>

typedef enum {
    // Keywords.
    LEX_UNSIGNED_INTEGER_16 = 1,   // u16
    LEX_SIGNED_INTEGER_16,     // s16
    LEX_FLOAT_16,              // f16
    LEX_INT,                   // int
    LEX_LONG,                  // long
    LEX_SHORT,                 // short
    LEX_BYTE,                  // byte
    LEX_FLOAT,                 // float
    LEX_DOUBLE,                // double
    LEX_CHAR,                  // char
    LEX_SIGNED,                // signed
    LEX_UNSIGNED,              // unsigned
    LEX_STRUCT,                // struct
    LEX_ENUM,                  // enum
    LEX_TYPEDEF,               // typedef
    LEX_VOID,                  // void
    LEX_IF,                    // if
    LEX_ELSE,                  // else
    LEX_WHILE,                 // while
    LEX_BREAK,                 // break
    LEX_CONTINUE,              // continue
    LEX_DO,                    // do
    LEX_FOR,                   // for
    LEX_GOTO,                  // goto
    LEX_RETURN,                // return
    LEX_RESTRICT,              // restrict
    LEX_SWITCH,                // switch
    LEX_CASE,                  // case
    LEX_DEFAULT,               // default
    LEX_INLINE,                // inline
    LEX_CONST,                 // const
    LEX_ALIGN,                 // align
    LEX_REGISTER,              // register
    LEX_AUTO,                  // auto
    LEX_EXTERN,                // extern
    LEX_SIZEOF,                // sizeof
    LEX_TYPEOF,                // typeof
    LEX_UNION,                 // union
    LEX_VOLATILE,              // volatile

    // Multi-character tokens.
    LEX_BANG_EQUAL,            // !=
    LEX_EQUAL_EQUAL,           // ==
    LEX_GREATER_EQUAL,         // >=
    LEX_LESS_EQUAL,            // <=
    LEX_LOGICAL_AND,           // &&
    LEX_LOGICAL_OR,            // ||
    LEX_PLUS_PLUS,             // ++
    LEX_MINUS_MINUS,           // --
    LEX_SHIFT_LEFT,            // <<
    LEX_SHIFT_RIGHT,           // >>
    LEX_PLUS_EQUAL,            // +=
    LEX_MINUS_EQUAL,           // -=
    LEX_STAR_EQUAL,            // *=
    LEX_SLASH_EQUAL,           // /=
    LEX_ARROW,                 // ->
    LEX_PERCENT_EQUAL,         // %=
    LEX_ELLIPSIS,              // ...

    // Single-character tokens.
    LEX_LEFT_PAREN,            // (
    LEX_RIGHT_PAREN,           // )
    LEX_LEFT_BRACE,            // {
    LEX_RIGHT_BRACE,           // }
    LEX_LEFT_BRACKET,          // [
    LEX_RIGHT_BRACKET,         // ]
    LEX_COMMA,                 // ,
    LEX_DOT,                   // .
    LEX_MINUS,                 // -
    LEX_PLUS,                  // +
    LEX_SEMICOLON,             // ;
    LEX_SLASH,                 // /
    LEX_STAR,                  // *  (ambiguous: deref or multiply)
    LEX_BANG,                  // !
    LEX_ASSIGN,                // =
    LEX_GREATER,               // >
    LEX_LESS,                  // <
    LEX_AMPERSAND,             // &  (ambiguous: ref or bitwise AND)
    LEX_BITWISE_OR,            // |
    LEX_BITWISE_NOT,           // ~
    LEX_BITWISE_XOR,           // ^
    LEX_COLON,                 // :
    LEX_PERCENT,               // %
    LEX_QUESTIONMARK,          // ?
    LEX_BACKSLASH,             /* \ */

    // Literals.
    LEX_IDENTIFIER,            // name
    LEX_STRING,                // "..."
    LEX_CHAR_LITERAL,          // 'c'
    LEX_NUMBER,                // 123, 0xFF, etc.
    LEX_COMMENT,               // //... or /* ... */
    LEX_PREPROCESSOR,          // #...

    // Other.
    LEX_EOF,                   // End of file/input
    LEX_UNDEFINED,             // Invalid/unrecognized
    LEX_RESERVED,              // Reserved

    LEX_TOKEN_COUNT, 
} LexerTokenType_t;


typedef struct {
    LexerTokenType_t type;       // type
    const char* raw;        // raw form of the token as-is in source
    int length;
    int index;              // total index in file
    int line;               // line
    int column;             // column in line
    int synthetic;          // 1 if it is added after tokenizer step, or in macro expansion
    int is_leaf;            // 1 if its the leaf of the AST, else 0
} LexerToken_t;


extern const char* lexer_token_literal[LEX_TOKEN_COUNT];

extern const int lexer_token_has_fixed_form[LEX_TOKEN_COUNT];

extern LexerToken_t* lexer_parse(char* source, long source_length, long* token_count);


#endif
