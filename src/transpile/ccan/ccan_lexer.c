#include <stdlib.h>
#include <string.h>

#include "utils/Log.h"
#include "utils/String.h"

#include "transpile/ccan/ccan_lexer.h"


const char* ccan_lexer_token_literal[CCAN_LEX_TOKEN_COUNT] = {
    NULL,                               // 0 — unused (tokens start at 1)

    // Keywords.
    "if",                               // CCAN_LEX_IF
    "else",                             // CCAN_LEX_ELSE
    "for",                              // CCAN_LEX_FOR
    "while",                            // CCAN_LEX_WHILE
    "continue",                         // CCAN_LEX_CONTINUE
    "break",                            // CCAN_LEX_BREAK
    "return",                           // CCAN_LEX_RETURN
    "void",                             // CCAN_LEX_VOID
    "int",                              // CCAN_LEX_INT
    "short",                            // CCAN_LEX_SHORT
    "char",                             // CCAN_LEX_CHAR
    "long",                             // CCAN_LEX_LONG
    "float",                            // CCAN_LEX_FLOAT
    "double",                           // CCAN_LEX_DOUBLE
    "unsigned",                         // CCAN_LEX_UNSIGNED
    "signed",                           // CCAN_LEX_SIGNED
    "struct",                           // CCAN_LEX_STRUCT
    "union",                            // CCAN_LEX_UNION
    "typedef",                          // CCAN_LEX_TYPEDEF
    "enum",                             // CCAN_LEX_ENUM
    "sizeof",                           // CCAN_LEX_SIZEOF
    "static",                           // CCAN_LEX_STATIC
    "const",                            // CCAN_LEX_CONST
    "extern",                           // CCAN_LEX_EXTERN
    "volatile",                         // CCAN_LEX_VOLATILE
    "auto",                             // CCAN_LEX_AUTO
    "inline",                           // CCAN_LEX_INLINE
    "register",                         // CCAN_LEX_REGISTER
    "restrict",                         // CCAN_LEX_RESTRICT
    "goto",                             // CCAN_LEX_GOTO
    "switch",                           // CCAN_LEX_SWITCH
    "case",                             // CCAN_LEX_CASE
    "default",                          // CCAN_LEX_DEFAULT
    "do",                               // CCAN_LEX_DO

    // Multi-character tokens.
    "<<=",                              // CCAN_LEX_SHIFT_LEFT_EQUAL
    ">>=",                              // CCAN_LEX_SHIFT_RIGHT_EQUAL
    "!=",                               // CCAN_LEX_BANG_EQUAL
    "==",                               // CCAN_LEX_EQUAL_EQUAL
    ">=",                               // CCAN_LEX_GREATER_EQUAL
    "<=",                               // CCAN_LEX_LESS_EQUAL
    "<<",                               // CCAN_LEX_SHIFT_LEFT
    ">>",                               // CCAN_LEX_SHIFT_RIGHT
    "&&",                               // CCAN_LEX_LOGICAL_AND
    "||",                               // CCAN_LEX_LOGICAL_OR
    "+=",                               // CCAN_LEX_PLUS_EQUAL
    "-=",                               // CCAN_LEX_MINUS_EQUAL
    "*=",                               // CCAN_LEX_STAR_EQUAL
    "/=",                               // CCAN_LEX_SLASH_EQUAL
    "%=",                               // CCAN_LEX_MOD_EQUAL
    "&=",                               // CCAN_LEX_AND_EQUAL
    "|=",                               // CCAN_LEX_OR_EQUAL
    "^=",                               // CCAN_LEX_XOR_EQUAL
    "++",                               // CCAN_LEX_INCREMENT
    "--",                               // CCAN_LEX_DECREMENT
    "->",                               // CCAN_LEX_ARROW

    // Single-character tokens.
    ">",                                // CCAN_LEX_GREATER
    "<",                                // CCAN_LEX_LESS
    "-",                                // CCAN_LEX_MINUS
    "+",                                // CCAN_LEX_PLUS
    "/",                                // CCAN_LEX_SLASH
    "*",                                // CCAN_LEX_STAR
    "(",                                // CCAN_LEX_LEFT_PARENTHESES
    ")",                                // CCAN_LEX_RIGHT_PARENTHESES
    "{",                                // CCAN_LEX_LEFT_BRACE
    "}",                                // CCAN_LEX_RIGHT_BRACE
    "[",                                // CCAN_LEX_LEFT_BRACKET
    "]",                                // CCAN_LEX_RIGHT_BRACKET
    ",",                                // CCAN_LEX_COMMA
    ":",                                // CCAN_LEX_COLON
    ".",                                // CCAN_LEX_POINT
    ";",                                // CCAN_LEX_SEMICOLON
    "=",                                // CCAN_LEX_ASSIGN
    "&",                                // CCAN_LEX_BITWISE_AND
    "|",                                // CCAN_LEX_BITWISE_OR
    "~",                                // CCAN_LEX_BITWISE_NOT
    "^",                                // CCAN_LEX_BITWISE_XOR
    "!",                                // CCAN_LEX_BANG
    "%",                                // CCAN_LEX_PERCENT
    "?",                                // CCAN_LEX_QUESTION_MARK
    "\\",                               // CCAN_LEX_BACKSLASH

    // Literals.
    NULL,                               // CCAN_LEX_IDENTIFIER
    NULL,                               // CCAN_LEX_STRING
    NULL,                               // CCAN_LEX_CHAR_LITERAL
    NULL,                               // CCAN_LEX_NUMBER
    NULL,                               // CCAN_LEX_COMMENT

    // Other.
    NULL,                               // CCAN_LEX_EOF
    NULL,                               // CCAN_LEX_UNDEFINED
    NULL,                               // CCAN_LEX_RESERVED
};



const int ccan_lexer_token_has_fixed_form[CCAN_LEX_TOKEN_COUNT] = {
    0,  // 0 — unused (tokens start at 1)

    // Keywords.
    1,  // CCAN_LEX_IF
    1,  // CCAN_LEX_ELSE
    1,  // CCAN_LEX_FOR
    1,  // CCAN_LEX_WHILE
    1,  // CCAN_LEX_CONTINUE
    1,  // CCAN_LEX_BREAK
    1,  // CCAN_LEX_RETURN
    1,  // CCAN_LEX_VOID
    1,  // CCAN_LEX_INT
    1,  // CCAN_LEX_SHORT
    1,  // CCAN_LEX_CHAR
    1,  // CCAN_LEX_LONG
    1,  // CCAN_LEX_FLOAT
    1,  // CCAN_LEX_DOUBLE
    1,  // CCAN_LEX_UNSIGNED
    1,  // CCAN_LEX_SIGNED
    1,  // CCAN_LEX_STRUCT
    1,  // CCAN_LEX_UNION
    1,  // CCAN_LEX_TYPEDEF
    1,  // CCAN_LEX_ENUM
    1,  // CCAN_LEX_SIZEOF
    1,  // CCAN_LEX_STATIC
    1,  // CCAN_LEX_CONST
    1,  // CCAN_LEX_EXTERN
    1,  // CCAN_LEX_VOLATILE
    1,  // CCAN_LEX_AUTO
    1,  // CCAN_LEX_INLINE
    1,  // CCAN_LEX_REGISTER
    1,  // CCAN_LEX_RESTRICT
    1,  // CCAN_LEX_GOTO
    1,  // CCAN_LEX_SWITCH
    1,  // CCAN_LEX_CASE
    1,  // CCAN_LEX_DEFAULT
    1,  // CCAN_LEX_DO

    // Multi-character tokens.
    1,  // CCAN_LEX_SHIFT_LEFT_EQUAL
    1,  // CCAN_LEX_SHIFT_RIGHT_EQUAL
    1,  // CCAN_LEX_BANG_EQUAL
    1,  // CCAN_LEX_EQUAL_EQUAL
    1,  // CCAN_LEX_GREATER_EQUAL
    1,  // CCAN_LEX_LESS_EQUAL
    1,  // CCAN_LEX_SHIFT_LEFT
    1,  // CCAN_LEX_SHIFT_RIGHT
    1,  // CCAN_LEX_LOGICAL_AND
    1,  // CCAN_LEX_LOGICAL_OR
    1,  // CCAN_LEX_PLUS_EQUAL
    1,  // CCAN_LEX_MINUS_EQUAL
    1,  // CCAN_LEX_STAR_EQUAL
    1,  // CCAN_LEX_SLASH_EQUAL
    1,  // CCAN_LEX_MOD_EQUAL
    1,  // CCAN_LEX_AND_EQUAL
    1,  // CCAN_LEX_OR_EQUAL
    1,  // CCAN_LEX_XOR_EQUAL
    1,  // CCAN_LEX_INCREMENT
    1,  // CCAN_LEX_DECREMENT
    1,  // CCAN_LEX_ARROW

    // Single-character tokens.
    1,  // CCAN_LEX_GREATER
    1,  // CCAN_LEX_LESS
    1,  // CCAN_LEX_MINUS
    1,  // CCAN_LEX_PLUS
    1,  // CCAN_LEX_SLASH
    1,  // CCAN_LEX_STAR
    1,  // CCAN_LEX_LEFT_PARENTHESES
    1,  // CCAN_LEX_RIGHT_PARENTHESES
    1,  // CCAN_LEX_LEFT_BRACE
    1,  // CCAN_LEX_RIGHT_BRACE
    1,  // CCAN_LEX_LEFT_BRACKET
    1,  // CCAN_LEX_RIGHT_BRACKET
    1,  // CCAN_LEX_COMMA
    1,  // CCAN_LEX_COLON
    1,  // CCAN_LEX_POINT
    1,  // CCAN_LEX_SEMICOLON
    1,  // CCAN_LEX_ASSIGN
    1,  // CCAN_LEX_BITWISE_AND
    1,  // CCAN_LEX_BITWISE_OR
    1,  // CCAN_LEX_BITWISE_NOT
    1,  // CCAN_LEX_BITWISE_XOR
    1,  // CCAN_LEX_BANG
    1,  // CCAN_LEX_PERCENT
    1,  // CCAN_LEX_QUESTION_MARK
    1,  // CCAN_LEX_BACKSLASH

    // Literals.
    0,  // CCAN_LEX_IDENTIFIER
    0,  // CCAN_LEX_STRING
    0,  // CCAN_LEX_CHAR_LITERAL
    0,  // CCAN_LEX_NUMBER
    0,  // CCAN_LEX_COMMENT

    // Other.
    0,  // CCAN_LEX_EOF
    0,  // CCAN_LEX_UNDEFINED
    0   // CCAN_LEX_RESERVED
};




CCANLexerToken_t* ccan_lexer_add_token(CCANLexerToken_t* lexer_token_list, long* token_count, CCANLexerToken_t token) {
    lexer_token_list = realloc(lexer_token_list, sizeof(CCANLexerToken_t) * ((*token_count) + 1));
    lexer_token_list[*token_count] = token;
    (*token_count) ++;
    return lexer_token_list;
}

int ccan_lexer_compare_keyword(char* source, long index, const char* keyword, long source_length) {
    int strl = strlen(keyword);
    char control[strl + 1];
    strncpy(control, &source[index], strl);
    control[strl] = '\0';
    if (source_length - index < strl) {
        // cant match, cause too close to the end
        return 0;
    }
    return strcmp(control, keyword) == 0;
}

extern CCANLexerToken_t* ccan_lexer_parse(char* source, long source_length, long* token_count) {

    CCANLexerToken_t* lexer_token_list = NULL;
    *token_count = 0;

    long line = 1;
    long column = 1;

    long index = 0;
    while (index < source_length) {
        ////log_msg(LP_DEBUG, "%d / %d", index, source_length);
        int found_token = 0;

        // wkip whitespaces
        if (source[index] == '\n') {
            index ++;
            line ++;
            column = 1;
            continue;
        }
        if (contains(source[index], " \t\b")) {
            index ++;
            column ++;
            continue;
        }

        // check for comments
        if (ccan_lexer_compare_keyword(source, index, "//", source_length)) {
            long ws_index = index;
            while (source[++ws_index] != '\n' && index < source_length);
            if (ws_index != index) {
                //char word[ws_index - index + 1];
                char* word = calloc(ws_index - index + 1, sizeof(char));
                strncpy(word, &source[index], ws_index - index);
                word[ws_index - index] = '\0';
                //log_msg(LP_SUCCESS, "LexerToken found: %s - \"%s\"", token_name[CCAN_LEX_COMMENT], word);
                CCANLexerToken_t token = {
                    .type = CCAN_LEX_COMMENT, 
                    .length = ws_index - index, 
                    .index = index, 
                    .line = line,
                    .column = column, 
                    .raw = word, 
                };
                lexer_token_list = ccan_lexer_add_token(lexer_token_list, token_count, token);
                column += ws_index - index;
                index = ws_index;
                found_token = 1;
            }
            continue;
        }

        // check for comments
        if (ccan_lexer_compare_keyword(source, index, "/*", source_length)) {
            long ws_index = index;
            while (!ccan_lexer_compare_keyword(source, ++ws_index, "*/", source_length) && index + 1 < source_length);
            if (ws_index != index) {
                //char word[ws_index - index + 1];
                char* word = calloc(ws_index - index + 3, sizeof(char));
                strncpy(word, &source[index], ws_index - index + 2);
                word[ws_index - index + 2] = '\0';
                //log_msg(LP_SUCCESS, "LexerToken found: %s - \"%s\"", token_name[CCAN_LEX_COMMENT], word);
                CCANLexerToken_t token = {
                    .type = CCAN_LEX_COMMENT, 
                    .length = ws_index - index + 2, 
                    .index = index, 
                    .line = line,
                    .column = column, 
                    .raw = word, 
                };
                lexer_token_list = ccan_lexer_add_token(lexer_token_list, token_count, token);
                column += ws_index - index + 2;
                index = ws_index + 2;
                found_token = 1;
            }
            continue;
        }

        // check for string
        if (source[index] == '"') {
            long ws_index = index + 1;
            while (1) {
                while (source[ws_index++] != '"' && index < source_length);
                if (source[ws_index - 2] != '\\') { // so that '\"' does not trigger end-of-string condition
                    break;
                }
            }
            if (ws_index != index) {
                //char word[ws_index - index + 1];
                char* word = calloc(ws_index - index + 1, sizeof(char));
                strncpy(word, &source[index], ws_index - index);
                word[ws_index - index] = '\0';
                //log_msg(LP_SUCCESS, "LexerToken found: %s - \"%s\"", token_name[CCAN_LEX_STRING], word);
                CCANLexerToken_t token = {
                    .type = CCAN_LEX_STRING, 
                    .length = ws_index - index + 1, 
                    .index = index, 
                    .line = line,
                    .column = column, 
                    .raw = word, 
                };
                lexer_token_list = ccan_lexer_add_token(lexer_token_list, token_count, token);
                column += ws_index - index + 1;
                index = ws_index;
                found_token = 1;
            }
            continue;
        }

        // check for character
        if (source[index] == '\'') {
            long ws_index = index + 1;
            while (1) {
                while (source[ws_index++] != '\'' && index < source_length);
                if (source[ws_index - 2] != '\\') { // so that '\"' does not trigger end-of-string condition
                    break;
                }
            }
            if (ws_index != index) {
                //char word[ws_index - index + 1];
                char* word = calloc(ws_index - index + 1, sizeof(char));
                strncpy(word, &source[index], ws_index - index);
                word[ws_index - index] = '\0';
                //log_msg(LP_SUCCESS, "LexerToken found: %s - \"%s\"", token_name[CCAN_LEX_CHAR_LITERAL], word);
                CCANLexerToken_t token = {
                    .type = CCAN_LEX_CHAR_LITERAL, 
                    .length = ws_index - index + 1, 
                    .index = index, 
                    .line = line,
                    .column = column, 
                    .raw = word, 
                };
                lexer_token_list = ccan_lexer_add_token(lexer_token_list, token_count, token);
                column += ws_index - index + 1;
                index = ws_index;
                found_token = 1;
            }
            continue;
        }


        // first check if its a number literal
        // first get a copy of the next "word" until the next whitespace, we just need the index of the next whitespace
        {long ws_index = index;
        while (ws_index < source_length) {
            if (!contains(source[ws_index], "0123456789.abcdefxob")) break;
            ws_index ++;
        }
        if (ws_index != index) {
            //char word[ws_index - index + 1];
            char* word = calloc(ws_index - index + 1, sizeof(char));
            strncpy(word, &source[index], ws_index - index);
            word[ws_index - index] = '\0';
            if (string_is_immediate(word) || string_is_float(word)) {
                //log_msg(LP_SUCCESS, "LexerToken found: %s - \"%s\"", token_name[CCAN_LEX_NUMBER], word);
                CCANLexerToken_t token = {
                    .type = CCAN_LEX_NUMBER, 
                    .length = ws_index - index, 
                    .index = index, 
                    .line = line,
                    .column = column, 
                    .raw = word, 
                };
                lexer_token_list = ccan_lexer_add_token(lexer_token_list, token_count, token);
                column += ws_index - index;
                index = ws_index;
                found_token = 1;
                continue;
            }
            free(word);
        }}


        // secondly, go through all the tokens and compare with those that have a static form, like most keywords
        for (int i = 0; i < CCAN_LEX_TOKEN_COUNT; i++) {
            if (!ccan_lexer_token_has_fixed_form[i]) {continue;} // not in fixed form, so we skip
            if (!ccan_lexer_compare_keyword(source, index, ccan_lexer_token_literal[i], source_length)) {continue;} // string comparison failed
            if (contains(source[index + strlen(ccan_lexer_token_literal[i])], "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")) {
                // keyword cannot be continuing with another alphanumerical
                // Unless the last character of the matching token was not an alphanumeric itself: 
                if (contains(source[index + strlen(ccan_lexer_token_literal[i]) - 1], "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")) {
                    continue;
                }
            }
            //log_msg(LP_SUCCESS, "LexerToken found: %s - \"%s\"", token_name[i], ccan_lexer_token_literal[i]);
            CCANLexerToken_t token = {
                .type = i, 
                .length = strlen(ccan_lexer_token_literal[i]), 
                .index = index, 
                .line = line,
                .column = column, 
                .raw = strdup(ccan_lexer_token_literal[i]), 
            };
            lexer_token_list = ccan_lexer_add_token(lexer_token_list, token_count, token);
            index += strlen(ccan_lexer_token_literal[i]);
            column += strlen(ccan_lexer_token_literal[i]);
            found_token = 1;
            break;
        }
        if (found_token) continue;

        // lastly, check for varname and strings
        {long ws_index = index;
        while (ws_index < source_length) {
            if (!contains(source[ws_index], "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_")) break;
            ws_index ++;
        }
        if (ws_index != index) {
            //char word[ws_index - index + 1];
            char* word = calloc(ws_index - index + 1, sizeof(char));
            strncpy(word, &source[index], ws_index - index);
            word[ws_index - index] = '\0';
            //log_msg(LP_SUCCESS, "LexerToken found: %s - \"%s\"", token_name[CCAN_LEX_IDENTIFIER], word);
            CCANLexerToken_t token = {
                .type = CCAN_LEX_IDENTIFIER, 
                .length = ws_index - index, 
                .index = index, 
                .line = line,
                .column = column, 
                .raw = word, 
            };
            lexer_token_list = ccan_lexer_add_token(lexer_token_list, token_count, token);
            column += ws_index - index;
            index = ws_index;
            found_token = 1;
            continue;
        }}

        log_msg(LP_ERROR, "CCAN Lexer: NO MATCH FOUND! [%s:%d]", __FILE__, __LINE__);
        log_msg(LP_INFO, "The following text contains the error: \n=====================\n%s\n=====================", &source[index]);
        log_msg(LP_INFO, "More context: \n=====================\n%s\n=====================", &source[index - 16]);
        exit(1);
    }


    return lexer_token_list;
}




