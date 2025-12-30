#include <stdlib.h>
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
    "callpusharg",   // IR_LEX_CALLPUSHARG
    "callfreearg",   // IR_LEX_CALLFREEARG
    "call",          // IR_LEX_CALL
    "scopebegin",    // IR_LEX_SCOPEBEGIN
    "scopeend",      // IR_LEX_SCOPEEND
    "cfi",           // IR_LEX_CFI
    "cfb",           // IR_LEX_CFB
    "cif",           // IR_LEX_CIF
    "cib",           // IR_LEX_CIB
    "cbf",           // IR_LEX_CBF
    "cbi",           // IR_LEX_CBI
    "cbw",           // IR_LEX_CBW
    ".address",      // IR_LEX_ADDRESS
    "irqbegin",      // IR_LEX_IRQBEGIN
    "irqend",        // IR_LEX_IRQEND
    "asm",           // IR_LEX_ASM

    // Multi-character tokens.
    "i-",             // IR_LEX_INTEGER_MINUS
    "f-",             // IR_LEX_FLOAT_MINUS
    "bf-",             // IR_LEX_BFLOAT_MINUS
    "i+",             // IR_LEX_INTEGER_PLUS
    "f+",             // IR_LEX_FLOAT_PLUS
    "bf+",             // IR_LEX_BFLOAT_PLUS
    "i/",             // IR_LEX_INTEGER_SLASH
    "f/",             // IR_LEX_FLOAT_SLASH
    "bf/",             // IR_LEX_BFLOAT_SLASH
    "i*",             // IR_LEX_INTEGER_STAR
    "f*",             // IR_LEX_FLOAT_STAR
    "bf*",             // IR_LEX_BFLOAT_STAR
    "!=",            // IR_LEX_BANG_EQUAL
    "==",            // IR_LEX_EQUAL_EQUAL
    "u>=",           // IR_LEX_UNSIGNED_INTEGER_GREATER_EQUAL
    "i>=",           // IR_LEX_INTEGER_GREATER_EQUAL
    "f>=",           // IR_LEX_FLOAT_GREATER_EQUAL
    "bf>=",           // IR_LEX_BFLOAT_GREATER_EQUAL
    "u<=",           // IR_LEX_UNSIGNED_INTEGER_LESS_EQUAL
    "i<=",           // IR_LEX_INTEGER_LESS_EQUAL
    "f<=",           // IR_LEX_FLOAT_LESS_EQUAL
    "bf<=",           // IR_LEX_BFLOAT_LESS_EQUAL
    "u>",            // IR_LEX_UNSIGNED_INTEGER_GREATER
    "i>",            // IR_LEX_INTEGER_GREATER
    "f>",            // IR_LEX_FLOAT_GREATER
    "bf>",            // IR_LEX_BFLOAT_GREATER
    "u<",            // IR_LEX_UNSIGNED_INTEGER_LESS
    "i<",            // IR_LEX_INTEGER_LESS
    "f<",            // IR_LEX_FLOAT_LESS
    "bf<",            // IR_LEX_BFLOAT_LESS
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


const int ir_lexer_token_has_fixed_form[IR_LEX_TOKEN_COUNT] = {
    0,  // 0 — unused

    // Keywords.
    1,  // IR_LEX_VAR
    1,  // IR_LEX_REF
    1,  // IR_LEX_DEREF
    1,  // IR_LEX_IF
    1,  // IR_LEX_ELSE
    1,  // IR_LEX_GOTO
    1,  // IR_LEX_RETURN
    1,  // IR_LEX_ARG
    1,  // IR_LEX_CONST
    1,  // IR_LEX_CALLPUSHARG
    1,  // IR_LEX_CALLFREEARG
    1,  // IR_LEX_CALL
    1,  // IR_LEX_SCOPEBEGIN
    1,  // IR_LEX_SCOPEEND
    1,  // IR_LEX_CFI
    1,  // IR_LEX_CFB
    1,  // IR_LEX_CIF
    1,  // IR_LEX_CIB
    1,  // IR_LEX_CBI
    1,  // IR_LEX_CBF
    1,  // IR_LEX_CBW
    1,  // IR_LEX_ADDRESS
    1,  // IR_LEX_IRQBEGIN
    1,  // IR_LEX_IRQEND
    1,  // IR_LEX_ASM

    // Multi-character tokens.
    1,  // IR_LEX_INTEGER_MINUS
    1,  // IR_LEX_FLOAT_MINUS
    1,  // IR_LEX_BFLOAT_MINUS
    1,  // IR_LEX_INTEGER_PLUS
    1,  // IR_LEX_FLOAT_PLUS
    1,  // IR_LEX_BFLOAT_PLUS
    1,  // IR_LEX_INTEGER_SLASH
    1,  // IR_LEX_FLOAT_SLASH
    1,  // IR_LEX_BFLOAT_SLASH
    1,  // IR_LEX_INTEGER_STAR
    1,  // IR_LEX_FLOAT_STAR
    1,  // IR_LEX_BFLOAT_STAR
    1,  // IR_LEX_BANG_EQUAL
    1,  // IR_LEX_EQUAL_EQUAL
    1,  // IR_LEX_UNSIGNED_INTEGER_GREATER_EQUAL
    1,  // IR_LEX_INTEGER_GREATER_EQUAL
    1,  // IR_LEX_FLOAT_GREATER_EQUAL
    1,  // IR_LEX_BFLOAT_GREATER_EQUAL
    1,  // IR_LEX_UNSIGNED_INTEGER_GREATER_EQUAL
    1,  // IR_LEX_INTEGER_LESS_EQUAL
    1,  // IR_LEX_FLOAT_LESS_EQUAL
    1,  // IR_LEX_BFLOAT_LESS_EQUAL
    1,  // IR_LEX_UNSIGNED_INTEGER_GREATER_EQUAL
    1,  // IR_LEX_INTEGER_GREATER
    1,  // IR_LEX_FLOAT_GREATER
    1,  // IR_LEX_BFLOAT_GREATER
    1,  // IR_LEX_UNSIGNED_INTEGER_GREATER_EQUAL
    1,  // IR_LEX_INTEGER_LESS
    1,  // IR_LEX_FLOAT_LESS
    1,  // IR_LEX_BFLOAT_LESS
    1,  // IR_LEX_SHIFT_LEFT
    1,  // IR_LEX_SHIFT_RIGHT

    // Single-character tokens.
    1,  // IR_LEX_LEFT_BRACKET
    1,  // IR_LEX_RIGHT_BRACKET
    1,  // IR_LEX_COMMA
    1,  // IR_LEX_SEMICOLON
    1,  // IR_LEX_ASSIGN
    1,  // IR_LEX_BITWISE_AND
    1,  // IR_LEX_BITWISE_OR
    1,  // IR_LEX_BITWISE_NOT
    1,  // IR_LEX_BITWISE_XOR
    1,  // IR_LEX_BANG
    1,  // IR_LEX_PERCENT

    // Literals.
    0,  // IR_LEX_IDENTIFIER
    0,  // IR_LEX_LABEL
    0,  // IR_LEX_STRING
    0,  // IR_LEX_CHAR_LITERAL
    0,  // IR_LEX_NUMBER
    0,  // IR_LEX_COMMENT

    // Other.
    0,  // IR_LEX_EOF
    0,  // IR_LEX_UNDEFINED
    0   // IR_LEX_RESERVED
};



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
            if (!contains(source[ws_index], "0123456789.abcdefxobABCDEF")) break;
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
            if (!ir_lexer_token_has_fixed_form[i]) {continue;} // not in fixed form, so we skip
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

        log_msg(LP_ERROR, "IR Lexer: NO MATCH FOUND! [%s:%d]", __FILE__, __LINE__);
        log_msg(LP_INFO, "The following text contains the error: \n=====================\n%s\n=====================", &source[index]);
        //log_msg(LP_INFO, "More context: \n=====================\n%s\n=====================", &source[((index - 16) < 0) ? 0 : (index - 16)]);
        exit(1);
    }


    return lexer_token_list;
}

/*
First pass: LexerTokenize / throw out whitespaces


*/



