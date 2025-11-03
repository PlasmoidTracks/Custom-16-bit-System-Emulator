#include <stdlib.h>
#include <string.h>

#include "utils/Log.h"
#include "utils/String.h"

#include "compiler/lexer.h"
#include "compiler/token_name_table.h"


const char* lexer_token_literal[LEX_TOKEN_COUNT] = {
    // Keywords.
    "???", 
    "u16", "s16", "f16", "int", "long", "short", "byte", "float", "double", "char",
    "signed", "unsigned", "struct", "enum", "typedef", "void", "if", "else", "while",
    "break", "continue", "do", "for", "goto", "return", "restrict", "switch", "case",
    "default", "inline", "const", "align", "register", "auto", "extern", "sizeof",
    "typeof", "union", "volatile",

    // Multi-character tokens.
    "!=", "==", ">=", "<=", "&&", "||", "++", "--", "<<", ">>", "+=", "-=", "*=", "/=", "->", "%=", "...",

    // Single-character tokens.
    "(", ")", "{", "}", "[", "]", ",", ".", "-", "+", ";", "/", "*", "!", "=", ">", "<", "&", "|", "~", "^", ":", "%", "?", "\\",

    // Literals.
    NULL, NULL, NULL, NULL, NULL, NULL,

    // Other.
    NULL, NULL, NULL
};

const int lexer_token_has_fixed_form[LEX_TOKEN_COUNT] = {
    // Keywords.
    0, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1,

    // Multi-character tokens.
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,

    // Single-character tokens.
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,

    // Literals.
    0, 0, 0, 0, 0, 0,

    // Other.
    0, 0, 0
};


LexerToken_t* lexer_add_token(LexerToken_t* lexer_token_list, long* token_count, LexerToken_t token) {
    lexer_token_list = realloc(lexer_token_list, sizeof(LexerToken_t) * ((*token_count) + 1));
    lexer_token_list[*token_count] = token;
    (*token_count) ++;
    return lexer_token_list;
}

int lexer_compare_keyword(char* source, long index, const char* keyword, long source_length) {
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

extern LexerToken_t* lexer_parse(char* source, long source_length, long* token_count) {

    LexerToken_t* lexer_token_list = NULL;
    *token_count = 0;

    long line = 1;
    long column = 1;

    long index = 0;
    while (index < source_length) {
        //log_msg(LP_DEBUG, "%d / %d", index, source_length);
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
        if (lexer_compare_keyword(source, index, "//", source_length)) {
            long ws_index = index;
            while (source[++ws_index] != '\n' && index < source_length);
            if (ws_index != index) {
                //char word[ws_index - index + 1];
                char* word = calloc(ws_index - index + 1, sizeof(char));
                strncpy(word, &source[index], ws_index - index);
                word[ws_index - index] = '\0';
                LexerToken_t token = {
                    .type = LEX_COMMENT, 
                    .length = ws_index - index, 
                    .index = index, 
                    .line = line,
                    .column = column, 
                    .raw = word, 
                    .synthetic = 0, 
                    .is_leaf = 1, 
                };
                lexer_token_list = lexer_add_token(lexer_token_list, token_count, token);
                column += ws_index - index;
                index = ws_index;
                found_token = 1;
            }
            continue;
        }

        // check for comments
        if (lexer_compare_keyword(source, index, "/*", source_length)) {
            long ws_index = index;
            while (!lexer_compare_keyword(source, ++ws_index, "*/", source_length) && index + 1 < source_length);
            if (ws_index != index) {
                //char word[ws_index - index + 1];
                char* word = calloc(ws_index - index + 3, sizeof(char));
                strncpy(word, &source[index], ws_index - index + 2);
                word[ws_index - index + 2] = '\0';
                LexerToken_t token = {
                    .type = LEX_COMMENT, 
                    .length = ws_index - index + 2, 
                    .index = index, 
                    .line = line,
                    .column = column, 
                    .raw = word, 
                    .synthetic = 0, 
                    .is_leaf = 1, 
                };
                lexer_token_list = lexer_add_token(lexer_token_list, token_count, token);
                column += ws_index - index + 2;
                index = ws_index + 2;
                found_token = 1;
            }
            continue;
        }

        // check for string
        if (source[index] == '"') {
            long ws_index = index + 1;
            while (source[ws_index++] != '"' && index < source_length);
            if (ws_index != index) {
                //char word[ws_index - index + 1];
                char* word = calloc(ws_index - index + 1, sizeof(char));
                strncpy(word, &source[index], ws_index - index);
                word[ws_index - index] = '\0';
                LexerToken_t token = {
                    .type = LEX_STRING, 
                    .length = ws_index - index + 1, 
                    .index = index, 
                    .line = line,
                    .column = column, 
                    .raw = word, 
                    .synthetic = 0, 
                    .is_leaf = 1, 
                };
                lexer_token_list = lexer_add_token(lexer_token_list, token_count, token);
                column += ws_index - index + 1;
                index = ws_index + 1;
                found_token = 1;
            }
            continue;
        }

        // check for character
        if (source[index] == '\'') {
            long ws_index = index + 1;
            while (source[ws_index++] != '\'' && index < source_length);
            if (ws_index != index) {
                //char word[ws_index - index + 1];
                char* word = calloc(ws_index - index + 1, sizeof(char));
                strncpy(word, &source[index], ws_index - index);
                word[ws_index - index] = '\0';
                LexerToken_t token = {
                    .type = LEX_CHAR_LITERAL, 
                    .length = ws_index - index + 1, 
                    .index = index, 
                    .line = line,
                    .column = column, 
                    .raw = word, 
                    .synthetic = 0, 
                    .is_leaf = 1, 
                };
                lexer_token_list = lexer_add_token(lexer_token_list, token_count, token);
                column += ws_index - index + 1;
                index = ws_index + 1;
                found_token = 1;
            }
            continue;
        }


        // first check if its a number literal
        // first get a copy of the next "word" until the next whitespace, we just need the index of the next whitespace
        {long ws_index = index;
        while (ws_index < source_length) {
            if (!contains(source[ws_index], "0123456789.fxob")) break;
            ws_index ++;
        }
        if (ws_index != index) {
            //char word[ws_index - index + 1];
            char* word = calloc(ws_index - index + 1, sizeof(char));
            strncpy(word, &source[index], ws_index - index);
            word[ws_index - index] = '\0';
            if (string_is_immediate(word) || string_is_float(word)) {
                //log_msg(LP_SUCCESS, "LexerToken found: %s - \"%s\"", token_name[LEX_NUMBER], word);
                LexerToken_t token = {
                    .type = LEX_NUMBER, 
                    .length = ws_index - index, 
                    .index = index, 
                    .line = line,
                    .column = column, 
                    .raw = word, 
                    .synthetic = 0, 
                    .is_leaf = 1, 
                };
                lexer_token_list = lexer_add_token(lexer_token_list, token_count, token);
                column += ws_index - index;
                index = ws_index;
                found_token = 1;
                continue;
            }
            free(word);
        }}


        // secondly, go through all the tokens and compare with those that have a static form, like most keywords
        for (int i = 0; i < LEX_TOKEN_COUNT; i++) {
            if (!lexer_token_has_fixed_form[i]) {continue;} // not in fixed form, so we skip
            if (!lexer_compare_keyword(source, index, lexer_token_literal[i], source_length)) {continue;} // string comparison failed
            //log_msg(LP_SUCCESS, "LexerToken found: %s - \"%s\"", token_name[i], token_literal[i]);
            LexerToken_t token = {
                .type = i, 
                .length = strlen(lexer_token_literal[i]), 
                .index = index, 
                .line = line,
                .column = column, 
                .raw = strdup(lexer_token_literal[i]), 
                .synthetic = 0, 
                .is_leaf = 1, 
            };
            lexer_token_list = lexer_add_token(lexer_token_list, token_count, token);
            index += strlen(lexer_token_literal[i]);
            column += strlen(lexer_token_literal[i]);
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
            //log_msg(LP_SUCCESS, "LexerToken found: %s - \"%s\"", token_name[LEX_IDENTIFIER], word);
            LexerToken_t token = {
                .type = LEX_IDENTIFIER, 
                .length = ws_index - index, 
                .index = index, 
                .line = line,
                .column = column, 
                .raw = word, 
                .synthetic = 0, 
                .is_leaf = 1, 
            };
            lexer_token_list = lexer_add_token(lexer_token_list, token_count, token);
            column += ws_index - index;
            index = ws_index;
            found_token = 1;
            continue;
        }}

        log_msg(LP_ERROR, "NO MATCH FOUND!");
        log_msg(LP_INFO, "The following text contains the error: \n=====================\n%s\n=====================", &source[index]);
        exit(1);
    }


    return lexer_token_list;
}

/*
First pass: LexerTokenize / throw out whitespaces


*/



