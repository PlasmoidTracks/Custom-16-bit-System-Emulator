#include <stdlib.h>
#include <string.h>

#include "utils/Log.h"
#include "utils/String.h"

#include "transpile/ira_lexer.h"


const char* ira_lexer_token_literal[IRA_LEX_TOKEN_COUNT] = {
    NULL,             // 0 — (no token type zero in use)

    // Keywords.
    "int16",          // IRA_LEX_INT16
    "uint16",         // IRA_LEX_UINT16
    "int8",           // IRA_LEX_INT8
    "uint8",          // IRA_LEX_UINT8
    "int32",          // IRA_LEX_INT32
    "uint32",         // IRA_LEX_UINT32
    "float16",        // IRA_LEX_FLOAT16
    "ref",            // IRA_LEX_REF
    "deref",          // IRA_LEX_DEREF
    "if",             // IRA_LEX_IF
    "else",           // IRA_LEX_ELSE
    "goto",           // IRA_LEX_GOTO
    "return",         // IRA_LEX_RETURN
    "arg",            // IRA_LEX_ARG
    "const",          // IRA_LEX_CONST
    "static",        // IRA_LEX_STATIC
    "anon",          // IRA_LEX_ANON
    "callpusharg",   // IRA_LEX_CALLPUSHARG
    "callfreearg",   // IRA_LEX_CALLFREEARG
    "call",          // IRA_LEX_CALL
    "scopebegin",    // IRA_LEX_SCOPEBEGIN
    "scopeend",      // IRA_LEX_SCOPEEND
    ".address",      // IRA_LEX_ADDRESS
    "irqbegin",      // IRA_LEX_IRQBEGIN
    "irqend",        // IRA_LEX_IRQEND
    "asm",           // IRA_LEX_ASM

    // Multi-character tokens.
    "!=",            // IRA_LEX_BANG_EQUAL
    "==",            // IRA_LEX_EQUAL_EQUAL
    ">=",            // IRA_LEX_GREATER_EQUAL
    "<=",            // IRA_LEX_LESS_EQUAL
    "<<",            // IRA_LEX_SHIFT_LEFT
    ">>",            // IRA_LEX_SHIFT_RIGHT

    // Single-character tokens.
    "-",             // IRA_LEX_MINUS
    "+",             // IRA_LEX_PLUS
    "/",             // IRA_LEX_SLASH
    "*",             // IRA_LEX_STAR
    ">",             // IRA_LEX_GREATER
    "<",             // IRA_LEX_LESS
    "(",             // IRA_LEX_LEFT_PARENTH
    ")",             // IRA_LEX_RIGHT_PARENTH
    "[",             // IRA_LEX_LEFT_BRACKET
    "]",             // IRA_LEX_RIGHT_BRACKET
    ",",             // IRA_LEX_COMMA
    ";",             // IRA_LEX_SEMICOLON
    "=",             // IRA_LEX_ASSIGN
    "&",             // IRA_LEX_BITWISE_AND
    "|",             // IRA_LEX_BITWISE_OR
    "~",             // IRA_LEX_BITWISE_NOT
    "^",             // IRA_LEX_BITWISE_XOR
    "!",             // IRA_LEX_BANG
    "%%",            // IRA_LEX_PERCENT

    // Literals.
    NULL,            // IRA_LEX_IDENTIFIER
    NULL,            // IRA_LEX_LABEL
    NULL,            // IRA_LEX_STRING
    NULL,            // IRA_LEX_CHAR_LITERAL
    NULL,            // IRA_LEX_NUMBER
    NULL,            // IRA_LEX_COMMENT

    // Other.
    NULL,            // IRA_LEX_EOF
    NULL,            // IRA_LEX_UNDEFINED
    NULL             // IRA_LEX_RESERVED
};


const int ira_lexer_token_has_fixed_form[IRA_LEX_TOKEN_COUNT] = {
    0,  // 0 — unused

    // Keywords.
    1,   // IRA_LEX_INT16
    1,   // IRA_LEX_UINT16
    1,   // IRA_LEX_INT8
    1,   // IRA_LEX_UINT8
    1,   // IRA_LEX_INT32
    1,   // IRA_LEX_UINT32
    1,   // IRA_LEX_FLOAT16
    1,   // IRA_LEX_REF
    1,   // IRA_LEX_DEREF
    1,  // IRA_LEX_IF
    1,  // IRA_LEX_ELSE
    1,  // IRA_LEX_GOTO
    1,  // IRA_LEX_RETURN
    1,  // IRA_LEX_ARG
    1,  // IRA_LEX_CONST
    1,  // IRA_LEX_CALLPUSHARG
    1,  // IRA_LEX_CALLFREEARG
    1,  // IRA_LEX_CALL
    1,  // IRA_LEX_SCOPEBEGIN
    1,  // IRA_LEX_SCOPEEND
    1,  // IRA_LEX_ADDRESS
    1,  // IRA_LEX_IRQBEGIN
    1,  // IRA_LEX_IRQEND
    1,  // IRA_LEX_ASM

    // Multi-character tokens.
    1,  // IRA_LEX_BANG_EQUAL
    1,  // IRA_LEX_EQUAL_EQUAL
    1,  // IRA_LEX_GREATER_EQUAL
    1,  // IRA_LEX_LESS_EQUAL
    1,  // IRA_LEX_SHIFT_LEFT
    1,  // IRA_LEX_SHIFT_RIGHT

    // Single-character tokens.
    1,  // IRA_LEX_MINUS
    1,  // IRA_LEX_PLUS
    1,  // IRA_LEX_SLASH
    1,  // IRA_LEX_STAR
    1,  // IRA_LEX_GREATER
    1,  // IRA_LEX_LESS
    1,  // IRA_LEX_LEFT_BRACKET
    1,  // IRA_LEX_RIGHT_BRACKET
    1,  // IRA_LEX_COMMA
    1,  // IRA_LEX_SEMICOLON
    1,  // IRA_LEX_ASSIGN
    1,  // IRA_LEX_BITWISE_AND
    1,  // IRA_LEX_BITWISE_OR
    1,  // IRA_LEX_BITWISE_NOT
    1,  // IRA_LEX_BITWISE_XOR
    1,  // IRA_LEX_BANG
    1,  // IRA_LEX_PERCENT

    // Literals.
    0,  // IRA_LEX_IDENTIFIER
    0,  // IRA_LEX_LABEL
    0,  // IRA_LEX_STRING
    0,  // IRA_LEX_CHAR_LITERAL
    0,  // IRA_LEX_NUMBER
    0,  // IRA_LEX_COMMENT

    // Other.
    0,  // IRA_LEX_EOF
    0,  // IRA_LEX_UNDEFINED
    0   // IRA_LEX_RESERVED
};



IRALexerToken_t* ira_lexer_add_token(IRALexerToken_t* ira_lexer_token_list, long* token_count, IRALexerToken_t token) {
    ira_lexer_token_list = realloc(ira_lexer_token_list, sizeof(IRALexerToken_t) * ((*token_count) + 1));
    ira_lexer_token_list[*token_count] = token;
    (*token_count) ++;
    return ira_lexer_token_list;
}

int ira_lexer_compare_keyword(char* source, long index, const char* keyword, long source_length) {
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

extern IRALexerToken_t* ira_lexer_parse(char* source, long source_length, long* token_count) {

    IRALexerToken_t* ira_lexer_token_list = NULL;
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
        if (ira_lexer_compare_keyword(source, index, "//", source_length)) {
            long ws_index = index;
            while (source[++ws_index] != '\n' && index < source_length);
            if (ws_index != index) {
                //char word[ws_index - index + 1];
                char* word = calloc(ws_index - index + 1, sizeof(char));
                strncpy(word, &source[index], ws_index - index);
                word[ws_index - index] = '\0';
                //log_msg(LP_SUCCESS, "LexerToken found: %s - \"%s\"", token_name[IRA_LEX_COMMENT], word);
                IRALexerToken_t token = {
                    .type = IRA_LEX_COMMENT, 
                    .length = ws_index - index, 
                    .index = index, 
                    .line = line,
                    .column = column, 
                    .raw = word, 
                };
                ira_lexer_token_list = ira_lexer_add_token(ira_lexer_token_list, token_count, token);
                column += ws_index - index;
                index = ws_index;
                found_token = 1;
            }
            continue;
        }

        // check for comments
        if (ira_lexer_compare_keyword(source, index, "/*", source_length)) {
            long ws_index = index;
            while (!ira_lexer_compare_keyword(source, ++ws_index, "*/", source_length) && index + 1 < source_length);
            if (ws_index != index) {
                //char word[ws_index - index + 1];
                char* word = calloc(ws_index - index + 3, sizeof(char));
                strncpy(word, &source[index], ws_index - index + 2);
                word[ws_index - index + 2] = '\0';
                //log_msg(LP_SUCCESS, "LexerToken found: %s - \"%s\"", token_name[IRA_LEX_COMMENT], word);
                IRALexerToken_t token = {
                    .type = IRA_LEX_COMMENT, 
                    .length = ws_index - index + 2, 
                    .index = index, 
                    .line = line,
                    .column = column, 
                    .raw = word, 
                };
                ira_lexer_token_list = ira_lexer_add_token(ira_lexer_token_list, token_count, token);
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
                //log_msg(LP_SUCCESS, "LexerToken found: %s - \"%s\"", token_name[IRA_LEX_STRING], word);
                IRALexerToken_t token = {
                    .type = IRA_LEX_STRING, 
                    .length = ws_index - index + 1, 
                    .index = index, 
                    .line = line,
                    .column = column, 
                    .raw = word, 
                };
                ira_lexer_token_list = ira_lexer_add_token(ira_lexer_token_list, token_count, token);
                column += ws_index - index + 1;
                index = ws_index;
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
                //log_msg(LP_SUCCESS, "LexerToken found: %s - \"%s\"", token_name[IRA_LEX_CHAR_LITERAL], word);
                IRALexerToken_t token = {
                    .type = IRA_LEX_CHAR_LITERAL, 
                    .length = ws_index - index + 1, 
                    .index = index, 
                    .line = line,
                    .column = column, 
                    .raw = word, 
                };
                ira_lexer_token_list = ira_lexer_add_token(ira_lexer_token_list, token_count, token);
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
                //log_msg(LP_SUCCESS, "LexerToken found: %s - \"%s\"", token_name[IRA_LEX_NUMBER], word);
                IRALexerToken_t token = {
                    .type = IRA_LEX_NUMBER, 
                    .length = ws_index - index, 
                    .index = index, 
                    .line = line,
                    .column = column, 
                    .raw = word, 
                };
                ira_lexer_token_list = ira_lexer_add_token(ira_lexer_token_list, token_count, token);
                column += ws_index - index;
                index = ws_index;
                found_token = 1;
                continue;
            }
            free(word);
        }}


        // secondly, go through all the tokens and compare with those that have a static form, like most keywords
        for (int i = 0; i < IRA_LEX_TOKEN_COUNT; i++) {
            if (!ira_lexer_token_has_fixed_form[i]) {continue;} // not in fixed form, so we skip
            if (!ira_lexer_compare_keyword(source, index, ira_lexer_token_literal[i], source_length)) {continue;} // string comparison failed
            //log_msg(LP_SUCCESS, "LexerToken found: %s - \"%s\"", token_name[i], ira_lexer_token_literal[i]);
            IRALexerToken_t token = {
                .type = i, 
                .length = strlen(ira_lexer_token_literal[i]), 
                .index = index, 
                .line = line,
                .column = column, 
                .raw = strdup(ira_lexer_token_literal[i]), 
            };
            ira_lexer_token_list = ira_lexer_add_token(ira_lexer_token_list, token_count, token);
            index += strlen(ira_lexer_token_literal[i]);
            column += strlen(ira_lexer_token_literal[i]);
            found_token = 1;
            break;
        }
        if (found_token) continue;

        // now check for labels
        {long ws_index = index;
        if (source[ws_index] == '.') {
            ws_index++;
            while (ws_index < source_length) {
                if (!contains(source[ws_index], "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_")) break;
                ws_index ++;
            }
        }
        if (ws_index != index) {
            //char word[ws_index - index + 1];
            char* word = calloc(ws_index - index + 1, sizeof(char));
            strncpy(word, &source[index], ws_index - index);
            word[ws_index - index] = '\0';
            //log_msg(LP_SUCCESS, "LexerToken found: %s - \"%s\"", token_name[IRA_LEX_LABEL], word);
            IRALexerToken_t token = {
                .type = IRA_LEX_LABEL, 
                .length = ws_index - index, 
                .index = index, 
                .line = line,
                .column = column, 
                .raw = word, 
            };
            ira_lexer_token_list = ira_lexer_add_token(ira_lexer_token_list, token_count, token);
            column += ws_index - index;
            index = ws_index;
            found_token = 1;
            continue;
        }}

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
            //log_msg(LP_SUCCESS, "LexerToken found: %s - \"%s\"", token_name[IRA_LEX_IDENTIFIER], word);
            IRALexerToken_t token = {
                .type = IRA_LEX_IDENTIFIER, 
                .length = ws_index - index, 
                .index = index, 
                .line = line,
                .column = column, 
                .raw = word, 
            };
            ira_lexer_token_list = ira_lexer_add_token(ira_lexer_token_list, token_count, token);
            column += ws_index - index;
            index = ws_index;
            found_token = 1;
            continue;
        }}

        log_msg(LP_ERROR, "NO MATCH FOUND!");
        log_msg(LP_INFO, "The following text contains the error: \n=====================\n%s\n=====================", &source[index]);
        exit(1);
    }


    return ira_lexer_token_list;
}

/*
First pass: LexerTokenize / throw out whitespaces


*/



