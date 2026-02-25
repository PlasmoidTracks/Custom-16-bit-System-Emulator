#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "utils/Log.h"
#include "utils/String.h"

#include "compiler/ir/ir_lexer.h"


const char* ir_lexer_token_literal[IR_LEX_TOKEN_COUNT] = {
    NULL,             // 0 — (no token type zero in use)

    // Keywords.
    "var",            // IR_LEX_VAR
    "ref",            // IR_LEX_REF
    "deref",          // IR_LEX_DEREF
    "if",             // IR_LEX_IF
    "else",           // IR_LEX_ELSE
    "goto",           // IR_LEX_GOTO
    "return",         // IR_LEX_RETURN
    "arg",            // IR_LEX_ARG
    "const",          // IR_LEX_CONST
    "static",        // IR_LEX_STATIC
    "anon",          // IR_LEX_ANON
    "volatile",      // IR_LEX_VOLATILE
    "callpusharg",   // IR_LEX_CALLPUSHARG
    "callfreearg",   // IR_LEX_CALLFREEARG
    "call",          // IR_LEX_CALL
    "scopebegin",    // IR_LEX_SCOPEBEGIN
    "scopeend",      // IR_LEX_SCOPEEND
    "cif",           // IR_LEX_CIF
    "cid",           // IR_LEX_CID
    "cil",           // IR_LEX_CIL
    "cfi",           // IR_LEX_CFI
    "cfd",           // IR_LEX_CFD
    "cfl",           // IR_LEX_CFL
    "cdi",           // IR_LEX_CDI
    "cdf",           // IR_LEX_CDF
    "cdl",           // IR_LEX_CDL
    "cli",           // IR_LEX_CLI
    "clf",           // IR_LEX_CLF
    "cld",           // IR_LEX_CLD
    "cbi",           // IR_LEX_CBI
    ".address",      // IR_LEX_ADDRESS
    "irqbegin",      // IR_LEX_IRQBEGIN
    "irqend",        // IR_LEX_IRQEND
    "asm",           // IR_LEX_ASM

    // Multi-character tokens.
    "i-",             // IR_LEX_INTEGER_MINUS
    "u-",             // IR_LEX_UNSIGNED_INTEGER_MINUS
    "f-",             // IR_LEX_FLOAT_MINUS
    "d-",             // IR_LEX_DOUBLE_MINUS
    "l-",             // IR_LEX_LONG_MINUS
    "i+",             // IR_LEX_INTEGER_PLUS
    "u+",             // IR_LEX_UNSIGNED_INTEGER_PLUS
    "f+",             // IR_LEX_FLOAT_PLUS
    "d+",             // IR_LEX_DOUBLE_PLUS
    "l+",             // IR_LEX_LONG_PLUS
    "i/",             // IR_LEX_INTEGER_SLASH
    "u/",             // IR_LEX_UNSIGNED_INTEGER_SLASH
    "f/",             // IR_LEX_FLOAT_SLASH
    "d/",             // IR_LEX_DOUBLE_SLASH
    "l/",             // IR_LEX_LONG_SLASH
    "i*",             // IR_LEX_INTEGER_STAR
    "u*",             // IR_LEX_UNSIGNED_INTEGER_STAR
    "f*",             // IR_LEX_FLOAT_STAR
    "d*",             // IR_LEX_DOUBLE_STAR
    "l*",             // IR_LEX_LONG_STAR
    "i!=",            // IR_LEX_BANG_EQUAL
    "u!=",            // IR_LEX_BANG_EQUAL
    "f!=",            // IR_LEX_BANG_EQUAL
    "d!=",            // IR_LEX_BANG_EQUAL
    "l!=",            // IR_LEX_BANG_EQUAL
    "i==",            // IR_LEX_EQUAL_EQUAL
    "u==",            // IR_LEX_EQUAL_EQUAL
    "f==",            // IR_LEX_EQUAL_EQUAL
    "d==",            // IR_LEX_EQUAL_EQUAL
    "l==",            // IR_LEX_EQUAL_EQUAL
    "i>=",           // IR_LEX_INTEGER_GREATER_EQUAL
    "u>=",           // IR_LEX_UNSIGNED_INTEGER_GREATER_EQUAL
    "f>=",           // IR_LEX_FLOAT_GREATER_EQUAL
    "d>=",           // IR_LEX_DOUBLE_GREATER_EQUAL
    "l>=",           // IR_LEX_LONG_GREATER_EQUAL
    "u<=",           // IR_LEX_UNSIGNED_INTEGER_LESS_EQUAL
    "i<=",           // IR_LEX_INTEGER_LESS_EQUAL
    "f<=",           // IR_LEX_FLOAT_LESS_EQUAL
    "d<=",           // IR_LEX_DOUBLE_LESS_EQUAL
    "l<=",           // IR_LEX_LONG_LESS_EQUAL
    "i>",            // IR_LEX_INTEGER_GREATER
    "u>",            // IR_LEX_UNSIGNED_INTEGER_GREATER
    "f>",            // IR_LEX_FLOAT_GREATER
    "d>",            // IR_LEX_DOUBLE_GREATER
    "l>",            // IR_LEX_LONG_GREATER
    "i<",            // IR_LEX_INTEGER_LESS
    "u<",            // IR_LEX_UNSIGNED_INTEGER_LESS
    "f<",            // IR_LEX_FLOAT_LESS
    "d<",            // IR_LEX_DOUBLE_LESS
    "l<",            // IR_LEX_LONG_LESS
    "<<",            // IR_LEX_SHIFT_LEFT
    ">>",            // IR_LEX_SHIFT_RIGHT

    // Single-character tokens.
    "[",             // IR_LEX_LEFT_BRACKET
    "]",             // IR_LEX_RIGHT_BRACKET
    ",",             // IR_LEX_COMMA
    ";",             // IR_LEX_SEMICOLON
    "=",             // IR_LEX_ASSIGN
    "&",             // IR_LEX_BITWISE_AND
    "|",             // IR_LEX_BITWISE_OR
    "~",             // IR_LEX_BITWISE_NOT
    "^",             // IR_LEX_BITWISE_XOR
    "!",             // IR_LEX_BANG
    "%",             // IR_LEX_PERCENT

    // Literals.
    NULL,            // IR_LEX_IDENTIFIER
    NULL,            // IR_LEX_LABEL
    NULL,            // IR_LEX_STRING
    NULL,            // IR_LEX_CHAR_LITERAL
    NULL,            // IR_LEX_NUMBER
    NULL,            // IR_LEX_COMMENT

    // Other.
    NULL,            // IR_LEX_EOF
    NULL,            // IR_LEX_UNDEFINED
    NULL             // IR_LEX_RESERVED
};


// legacy, will instead be determined by ir_lexer_token_literal entry being NULL
//const int ir_lexer_token_has_fixed_form[IR_LEX_TOKEN_COUNT];


IRLexerToken_t* ir_lexer_add_token(IRLexerToken_t* lexer_token_list, long* token_count, IRLexerToken_t token) {
    lexer_token_list = realloc(lexer_token_list, sizeof(IRLexerToken_t) * ((*token_count) + 1));
    lexer_token_list[*token_count] = token;
    (*token_count) ++;
    return lexer_token_list;
}

int ir_lexer_compare_keyword(char* source, long index, const char* keyword, long source_length) {
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

extern IRLexerToken_t* ir_lexer_parse(char* source, long source_length, long* token_count) {

    IRLexerToken_t* lexer_token_list = NULL;
    *token_count = 0;

    long line = 1;
    long column = 1;
    int error = 0;
    int error_line = -1;

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
        if (ir_lexer_compare_keyword(source, index, "//", source_length)) {
            long ws_index = index;
            while (source[++ws_index] != '\n' && index < source_length);
            if (ws_index != index) {
                //char word[ws_index - index + 1];
                char* word = calloc(ws_index - index + 1, sizeof(char));
                strncpy(word, &source[index], ws_index - index);
                word[ws_index - index] = '\0';
                //log_msg(LP_SUCCESS, "IR Lexer: LexerToken found: %s - \"%s\"", token_name[IR_LEX_COMMENT], word);
                IRLexerToken_t token = {
                    .type = IR_LEX_COMMENT, 
                    .length = ws_index - index, 
                    .index = index, 
                    .line = line,
                    .column = column, 
                    .raw = word, 
                };
                lexer_token_list = ir_lexer_add_token(lexer_token_list, token_count, token);
                column += ws_index - index;
                index = ws_index;
                found_token = 1;
            }
            continue;
        }

        // check for comments
        if (ir_lexer_compare_keyword(source, index, "/*", source_length)) {
            long ws_index = index;
            while (!ir_lexer_compare_keyword(source, ++ws_index, "*/", source_length) && index + 1 < source_length);
            if (ws_index != index) {
                //char word[ws_index - index + 1];
                char* word = calloc(ws_index - index + 3, sizeof(char));
                strncpy(word, &source[index], ws_index - index + 2);
                word[ws_index - index + 2] = '\0';
                //log_msg(LP_SUCCESS, "IR Lexer: LexerToken found: %s - \"%s\"", token_name[IR_LEX_COMMENT], word);
                IRLexerToken_t token = {
                    .type = IR_LEX_COMMENT, 
                    .length = ws_index - index + 2, 
                    .index = index, 
                    .line = line,
                    .column = column, 
                    .raw = word, 
                };
                lexer_token_list = ir_lexer_add_token(lexer_token_list, token_count, token);
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
                //log_msg(LP_SUCCESS, "IR Lexer: LexerToken found: %s - \"%s\"", token_name[IR_LEX_STRING], word);
                IRLexerToken_t token = {
                    .type = IR_LEX_STRING, 
                    .length = ws_index - index + 1, 
                    .index = index, 
                    .line = line,
                    .column = column, 
                    .raw = word, 
                };
                lexer_token_list = ir_lexer_add_token(lexer_token_list, token_count, token);
                column += ws_index - index;
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
                //log_msg(LP_SUCCESS, "IR Lexer: LexerToken found: %s - \"%s\"", token_name[IR_LEX_CHAR_LITERAL], word);
                IRLexerToken_t token = {
                    .type = IR_LEX_CHAR_LITERAL, 
                    .length = ws_index - index + 1, 
                    .index = index, 
                    .line = line,
                    .column = column, 
                    .raw = word, 
                };
                lexer_token_list = ir_lexer_add_token(lexer_token_list, token_count, token);
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
            if (!contains(source[ws_index], "0123456789.abcdefxobABCDEFl")) break;
            ws_index ++;
        }
        if (ws_index != index) {
            //char word[ws_index - index + 1];
            char* word = calloc(ws_index - index + 1, sizeof(char));
            strncpy(word, &source[index], ws_index - index);
            word[ws_index - index] = '\0';
            if (string_is_immediate(word) || string_is_float(word)) {
                //log_msg(LP_SUCCESS, "IR Lexer: LexerToken found: %s - \"%s\"", token_name[IR_LEX_NUMBER], word);
                IRLexerToken_t token = {
                    .type = IR_LEX_NUMBER, 
                    .length = ws_index - index, 
                    .index = index, 
                    .line = line,
                    .column = column, 
                    .raw = word, 
                };
                lexer_token_list = ir_lexer_add_token(lexer_token_list, token_count, token);
                column += ws_index - index;
                index = ws_index;
                found_token = 1;
                continue;
            }
            free(word);
        }}


        // secondly, go through all the tokens and compare with those that have a static form, like most keywords
        for (int i = 0; i < IR_LEX_TOKEN_COUNT; i++) {
            if (!ir_lexer_token_literal[i]) {continue;} // not in fixed form, so we skip
            if (!ir_lexer_compare_keyword(source, index, ir_lexer_token_literal[i], source_length)) {continue;} // string comparison failed
            if (contains(source[index + strlen(ir_lexer_token_literal[i])], "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")) {
                // keyword cannot be continuing with another alphanumerical
                // Unless the last character of the matching token was not an alphanumeric itself: 
                if (contains(source[index + strlen(ir_lexer_token_literal[i]) - 1], "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")) {
                    continue;
                }
            }
            //log_msg(LP_SUCCESS, "IR Lexer: LexerToken found: %s - \"%s\"", token_name[i], ir_lexer_token_literal[i]);
            IRLexerToken_t token = {
                .type = i, 
                .length = strlen(ir_lexer_token_literal[i]), 
                .index = index, 
                .line = line,
                .column = column, 
                .raw = strdup(ir_lexer_token_literal[i]), 
            };
            lexer_token_list = ir_lexer_add_token(lexer_token_list, token_count, token);
            index += strlen(ir_lexer_token_literal[i]);
            column += strlen(ir_lexer_token_literal[i]);
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
            //log_msg(LP_SUCCESS, "IR Lexer: LexerToken found: %s - \"%s\"", token_name[IR_LEX_LABEL], word);
            IRLexerToken_t token = {
                .type = IR_LEX_LABEL, 
                .length = ws_index - index, 
                .index = index, 
                .line = line,
                .column = column, 
                .raw = word, 
            };
            lexer_token_list = ir_lexer_add_token(lexer_token_list, token_count, token);
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
            //log_msg(LP_SUCCESS, "IR Lexer: LexerToken found: %s - \"%s\"", token_name[IR_LEX_IDENTIFIER], word);
            IRLexerToken_t token = {
                .type = IR_LEX_IDENTIFIER, 
                .length = ws_index - index, 
                .index = index, 
                .line = line,
                .column = column, 
                .raw = word, 
            };
            lexer_token_list = ir_lexer_add_token(lexer_token_list, token_count, token);
            column += ws_index - index;
            index = ws_index;
            found_token = 1;
            continue;
        }}

        if (line != error_line) {
            if (!error) {
                log_msg(LP_ERROR, "IR Lexer: NO MATCH FOUND! [%s:%d]", __FILE__, __LINE__);
                log_msg_inline(LP_INFO, "The following line(s) contain errors: \nline\n");
            }
            int newlines = 0;
            int max_index = (index + 256) >= source_length ? source_length : (index + 256);
            while (index > 0 && source[index - 1] != '\n') {
                index --;
            }
            printf("%-4ld:%-3ld | ", line, column);
            while (newlines < 1 && index < max_index) {
                char c = source[index++];
                if (c == '\n') {
                    newlines++;
                    line ++;
                }
                printf("%c", c);
            }
            error_line = line;
            printf("         | ");
            for (int i = 0; i < column - 4; i++) {
                printf(" ");
            }
            if (column - 3 < 0) {
                for (int i = 0; i < column - 1; i++) {
                    printf("~");
                }
            } else {
                printf("~~~");
            }
            printf("^~~~\n");
        }
        //return NULL;
        error = 1;
        index ++;
    }

    if (error) {
        return NULL;
    }

    return lexer_token_list;
}

/*
First pass: LexerTokenize / throw out whitespaces


*/



