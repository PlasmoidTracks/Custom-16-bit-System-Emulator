#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "utils/Log.h"
#include "utils/String.h"

#include "compiler/ir/ir_lexer.h"


const char* ir_lexer_token_literal[IR_LEX_TOKEN_COUNT] = {
    NULL,             // 0 — (no token type zero in use)

    // Keywords.
    [IR_LEX_REF] = "ref",                               // IR_LEX_REF
    [IR_LEX_DEREF] = "deref",                           // IR_LEX_DEREF
    [IR_LEX_IF] = "if",                                 // IR_LEX_IF
    [IR_LEX_GOTO] = "goto",                             // IR_LEX_GOTO
    [IR_LEX_RETURN] = "return",                         // IR_LEX_RETURN
    [IR_LEX_PARG] = "__parg",                           // IR_LEX_PARG
    [IR_LEX_STACK] = "stack",                           // IR_LEX_STACK
    [IR_LEX_STATIC] = "static",                         // IR_LEX_STATIC
    [IR_LEX_VOLATILE] = "volatile",                     // IR_LEX_VOLATILE
    [IR_LEX_REGISTER] = "register",                     // IR_LEX_REGISTER
    [IR_LEX_PUSHARG] = "pusharg",                       // IR_LEX_CALLPUSHARG
    [IR_LEX_FREEARG] = "freearg",                       // IR_LEX_CALLFREEARG
    [IR_LEX_CALL] = "call",                             // IR_LEX_CALL
    [IR_LEX_PERILOGUE] = "perilogue",                   // IR_LEX_PERILOGUE
    [IR_LEX_ATOMIC] = "atomic",                         // IR_LEX_ATOMIC
    [IR_LEX_REENTRANT] = "reentrant",                   // IR_LEX_REENTRANT
    [IR_LEX_INTERRUPT] = "interrupt",                   // IR_LEX_INTERRUPT
    [IR_LEX_LOCAL] = "local",                           // IR_LEX_LOCAL
    [IR_LEX_ALIGN] = "align",                           // IR_LEX_ALIGN
    [IR_LEX_SCOPEBEGIN] = "scopebegin",                 // IR_LEX_SCOPEBEGIN
    [IR_LEX_SCOPEEND] = "scopeend",                     // IR_LEX_SCOPEEND
    [IR_LEX_CIF] = "cif",                               // IR_LEX_CIF
    [IR_LEX_CID] = "cid",                               // IR_LEX_CID
    [IR_LEX_CIL] = "cil",                               // IR_LEX_CIL
    [IR_LEX_CFI] = "cfi",                               // IR_LEX_CFI
    [IR_LEX_CFD] = "cfd",                               // IR_LEX_CFD
    [IR_LEX_CFL] = "cfl",                               // IR_LEX_CFL
    [IR_LEX_CDI] = "cdi",                               // IR_LEX_CDI
    [IR_LEX_CDF] = "cdf",                               // IR_LEX_CDF
    [IR_LEX_CDL] = "cdl",                               // IR_LEX_CDL
    [IR_LEX_CLI] = "cli",                               // IR_LEX_CLI
    [IR_LEX_CLF] = "clf",                               // IR_LEX_CLF
    [IR_LEX_CLD] = "cld",                               // IR_LEX_CLD
    [IR_LEX_CBI] = "cbi",                               // IR_LEX_CBI
    [IR_LEX_REQUIRE] = "require",                       // IR_LEX_REQUIRE
    [IR_LEX_IRQBEGIN] = "irqbegin",                     // IR_LEX_IRQBEGIN
    [IR_LEX_IRQEND] = "irqend",                         // IR_LEX_IRQEND
    [IR_LEX_ASM] = "asm",                               // IR_LEX_ASM

    // Multi-character tokens.
    [IR_LEX_INTEGER_MINUS_EQUAL] = "i-=",                // IR_LEX_INTEGER_MINUS_EQUAL
    [IR_LEX_UNSIGNED_INTEGER_MINUS_EQUAL] = "u-=",       // IR_LEX_UNSIGNED_INTEGER_MINUS_EQUAL
    [IR_LEX_SATURATED_INTEGER_MINUS_EQUAL] = "si-=",     // IR_LEX_SATURATED_INTEGER_MINUS_EQUAL
    [IR_LEX_SATURATED_UNSIGNED_INTEGER_MINUS_EQUAL] = "su-=", // IR_LEX_SATURATED_UNSIGNED_INTEGER_MINUS_EQUAL
    [IR_LEX_FLOAT_MINUS_EQUAL] = "f-=",                  // IR_LEX_FLOAT_MINUS_EQUAL
    [IR_LEX_DOUBLE_MINUS_EQUAL] = "d-=",                 // IR_LEX_DOUBLE_MINUS_EQUAL
    [IR_LEX_LONG_MINUS_EQUAL] = "l-=",                   // IR_LEX_LONG_MINUS_EQUAL

    [IR_LEX_INTEGER_PLUS_EQUAL] = "i+=",                 // IR_LEX_INTEGER_PLUS_EQUAL
    [IR_LEX_UNSIGNED_INTEGER_PLUS_EQUAL] = "u+=",        // IR_LEX_UNSIGNED_INTEGER_PLUS_EQUAL
    [IR_LEX_SATURATED_INTEGER_PLUS_EQUAL] = "si+=",      // IR_LEX_SATURATED_INTEGER_PLUS_EQUAL
    [IR_LEX_SATURATED_UNSIGNED_INTEGER_PLUS_EQUAL] = "su+=", // IR_LEX_SATURATED_UNSIGNED_INTEGER_PLUS_EQUAL
    [IR_LEX_FLOAT_PLUS_EQUAL] = "f+=",                   // IR_LEX_FLOAT_PLUS_EQUAL
    [IR_LEX_DOUBLE_PLUS_EQUAL] = "d+=",                  // IR_LEX_DOUBLE_PLUS_EQUAL
    [IR_LEX_LONG_PLUS_EQUAL] = "l+=",                    // IR_LEX_LONG_PLUS_EQUAL

    [IR_LEX_INTEGER_STAR_EQUAL] = "i*=",                 // IR_LEX_INTEGER_STAR_EQUAL
    [IR_LEX_UNSIGNED_INTEGER_STAR_EQUAL] = "u*=",        // IR_LEX_UNSIGNED_INTEGER_STAR_EQUAL
    [IR_LEX_SATURATED_INTEGER_STAR_EQUAL] = "si*=",      // IR_LEX_SATURATED_INTEGER_STAR_EQUAL
    [IR_LEX_SATURATED_UNSIGNED_INTEGER_STAR_EQUAL] = "su*=", // IR_LEX_SATURATED_UNSIGNED_INTEGER_STAR_EQUAL
    [IR_LEX_FLOAT_STAR_EQUAL] = "f*=",                   // IR_LEX_FLOAT_STAR_EQUAL
    [IR_LEX_DOUBLE_STAR_EQUAL] = "d*=",                  // IR_LEX_DOUBLE_STAR_EQUAL
    [IR_LEX_LONG_STAR_EQUAL] = "l*=",                    // IR_LEX_LONG_STAR_EQUAL

    [IR_LEX_INTEGER_SLASH_EQUAL] = "i/=",                // IR_LEX_INTEGER_SLASH_EQUAL
    [IR_LEX_UNSIGNED_INTEGER_SLASH_EQUAL] = "u/=",       // IR_LEX_UNSIGNED_INTEGER_SLASH_EQUAL
    [IR_LEX_FLOAT_SLASH_EQUAL] = "f/=",                  // IR_LEX_FLOAT_SLASH_EQUAL
    [IR_LEX_DOUBLE_SLASH_EQUAL] = "d/=",                 // IR_LEX_DOUBLE_SLASH_EQUAL
    [IR_LEX_LONG_SLASH_EQUAL] = "l/=",                   // IR_LEX_LONG_SLASH_EQUAL

    [IR_LEX_INTEGER_BANG_EQUAL] = "i!=",                 // IR_LEX_INTEGER_BANG_EQUAL
    [IR_LEX_FLOAT_BANG_EQUAL] = "f!=",                   // IR_LEX_FLOAT_BANG_EQUAL
    [IR_LEX_DOUBLE_BANG_EQUAL] = "d!=",                  // IR_LEX_DOUBLE_BANG_EQUAL
    [IR_LEX_LONG_BANG_EQUAL] = "l!=",                    // IR_LEX_LONG_BANG_EQUAL

    [IR_LEX_INTEGER_EQUAL_EQUAL] = "i==",                // IR_LEX_INTEGER_EQUAL_EQUAL
    [IR_LEX_FLOAT_EQUAL_EQUAL] = "f==",                  // IR_LEX_FLOAT_EQUAL_EQUAL
    [IR_LEX_DOUBLE_EQUAL_EQUAL] = "d==",                 // IR_LEX_DOUBLE_EQUAL_EQUAL
    [IR_LEX_LONG_EQUAL_EQUAL] = "l==",                   // IR_LEX_LONG_EQUAL_EQUAL

    [IR_LEX_INTEGER_GREATER_EQUAL] = "i>=",              // IR_LEX_INTEGER_GREATER_EQUAL
    [IR_LEX_UNSIGNED_INTEGER_GREATER_EQUAL] = "u>=",     // IR_LEX_UNSIGNED_INTEGER_GREATER_EQUAL
    [IR_LEX_FLOAT_GREATER_EQUAL] = "f>=",                // IR_LEX_FLOAT_GREATER_EQUAL
    [IR_LEX_DOUBLE_GREATER_EQUAL] = "d>=",               // IR_LEX_DOUBLE_GREATER_EQUAL
    [IR_LEX_LONG_GREATER_EQUAL] = "l>=",                 // IR_LEX_LONG_GREATER_EQUAL

    [IR_LEX_INTEGER_LESS_EQUAL] = "i<=",                 // IR_LEX_INTEGER_LESS_EQUAL
    [IR_LEX_UNSIGNED_INTEGER_LESS_EQUAL] = "u<=",        // IR_LEX_UNSIGNED_INTEGER_LESS_EQUAL
    [IR_LEX_FLOAT_LESS_EQUAL] = "f<=",                   // IR_LEX_FLOAT_LESS_EQUAL
    [IR_LEX_DOUBLE_LESS_EQUAL] = "d<=",                  // IR_LEX_DOUBLE_LESS_EQUAL
    [IR_LEX_LONG_LESS_EQUAL] = "l<=",                    // IR_LEX_LONG_LESS_EQUAL

    [IR_LEX_INTEGER_GREATER] = "i>",                     // IR_LEX_INTEGER_GREATER
    [IR_LEX_UNSIGNED_INTEGER_GREATER] = "u>",            // IR_LEX_UNSIGNED_INTEGER_GREATER
    [IR_LEX_FLOAT_GREATER] = "f>",                       // IR_LEX_FLOAT_GREATER
    [IR_LEX_DOUBLE_GREATER] = "d>",                      // IR_LEX_DOUBLE_GREATER
    [IR_LEX_LONG_GREATER] = "l>",                        // IR_LEX_LONG_GREATER

    [IR_LEX_INTEGER_LESS] = "i<",                        // IR_LEX_INTEGER_LESS
    [IR_LEX_UNSIGNED_INTEGER_LESS] = "u<",               // IR_LEX_UNSIGNED_INTEGER_LESS
    [IR_LEX_FLOAT_LESS] = "f<",                          // IR_LEX_FLOAT_LESS
    [IR_LEX_DOUBLE_LESS] = "d<",                         // IR_LEX_DOUBLE_LESS
    [IR_LEX_LONG_LESS] = "l<",                           // IR_LEX_LONG_LESS

    [IR_LEX_SHIFT_LEFT_EQUAL] = "<<=",                   // IR_LEX_SHIFT_LEFT_EQUAL
    [IR_LEX_SHIFT_RIGHT_EQUAL] = ">>=",                  // IR_LEX_SHIFT_RIGHT_EQUAL
    [IR_LEX_BITWISE_AND_EQUAL] = "&=",                   // IR_LEX_BITWISE_AND_EQUAL
    [IR_LEX_BITWISE_OR_EQUAL] = "|=",                    // IR_LEX_BITWISE_OR_EQUAL
    [IR_LEX_ASSIGN_8_BIT] = "8=",                        // IR_LEX_ASSIGN_8_BIT

    // Single-character tokens.
    [IR_LEX_LEFT_CURLY_BRACKET] = "{",                   // IR_LEX_LEFT_CURLY_BRACKET
    [IR_LEX_RIGHT_CURLY_BRACKET] = "}",                  // IR_LEX_RIGHT_CURLY_BRACKET
    [IR_LEX_SEMICOLON] = ";",                            // IR_LEX_SEMICOLON
    [IR_LEX_COLON] = ":",                                // IR_LEX_COLON
    [IR_LEX_BITWISE_NOT_EQUAL] = "~",                    // IR_LEX_BITWISE_NOT_EQUAL
    [IR_LEX_BITWISE_XOR_EQUAL] = "^",                    // IR_LEX_BITWISE_XOR_EQUAL
    [IR_LEX_ASSIGN_16_BIT] = "=",                        // IR_LEX_ASSIGN_16_BIT
    [IR_LEX_STAR] = "*",                                 // IR_LEX_STAR
    [IR_LEX_AMPERSAND] = "&",                            // IR_LEX_AMPERSAND

    // Literals.
    [IR_LEX_IDENTIFIER] = NULL,                          // IR_LEX_IDENTIFIER
    [IR_LEX_LABEL] = NULL,                               // IR_LEX_LABEL
    [IR_LEX_STRING] = NULL,                              // IR_LEX_STRING
    [IR_LEX_CHAR_LITERAL] = NULL,                        // IR_LEX_CHAR_LITERAL
    [IR_LEX_NUMBER] = NULL,                              // IR_LEX_NUMBER
    [IR_LEX_COMMENT] = NULL,                             // IR_LEX_COMMENT

    // Other.
    [IR_LEX_EOF] = NULL,                                 // IR_LEX_EOF
    [IR_LEX_UNDEFINED] = NULL,                           // IR_LEX_UNDEFINED
    [IR_LEX_RESERVED] = NULL,                            // IR_LEX_RESERVED
};

// legacy, will instead be determined by ir_lexer_token_literal entry being NULL
//const int ir_lexer_token_has_fixed_form[IR_LEX_TOKEN_COUNT];

static inline int imin(int a, int b) {
    return (a < b) ? a : b;
}


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
            while (source[++ws_index] != '\n' && index < source_length - 1);
            if (ws_index != index) {
                //char word[ws_index - index + 1];
                //log_msg(LP_SUCCESS, "IR Lexer: LexerToken found: %s - \"%s\"", token_name[IR_LEX_COMMENT], word);
                IRLexerToken_t token = {
                    .type = IR_LEX_COMMENT, 
                    .length = ws_index - index, 
                    .index = index, 
                    .line = line,
                    .column = column, 
                    //.raw = word, 
                };
                strncpy(token.raw, &source[index], imin(MAX_RAW_LENGTH - 1, ws_index - index));
                token.raw[imin(MAX_RAW_LENGTH - 1, ws_index - index)] = '\0';
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
                //log_msg(LP_SUCCESS, "IR Lexer: LexerToken found: %s - \"%s\"", token_name[IR_LEX_COMMENT], word);
                IRLexerToken_t token = {
                    .type = IR_LEX_COMMENT, 
                    .length = ws_index - index + 2, 
                    .index = index, 
                    .line = line,
                    .column = column, 
                    //.raw = word, 
                };
                strncpy(token.raw, &source[index], imin(MAX_RAW_LENGTH - 1, ws_index - index + 2));
                token.raw[imin(MAX_RAW_LENGTH - 1, ws_index - index + 2)] = '\0';
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
                //log_msg(LP_SUCCESS, "IR Lexer: LexerToken found: %s - \"%s\"", token_name[IR_LEX_STRING], word);
                IRLexerToken_t token = {
                    .type = IR_LEX_STRING, 
                    .length = ws_index - index + 1, 
                    .index = index, 
                    .line = line,
                    .column = column, 
                    //.raw = word, 
                };
                strncpy(token.raw, &source[index], imin(MAX_RAW_LENGTH - 1, ws_index - index));
                token.raw[imin(MAX_RAW_LENGTH - 1, ws_index - index)] = '\0';
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
                //log_msg(LP_SUCCESS, "IR Lexer: LexerToken found: %s - \"%s\"", token_name[IR_LEX_CHAR_LITERAL], word);
                IRLexerToken_t token = {
                    .type = IR_LEX_CHAR_LITERAL, 
                    .length = ws_index - index + 1, 
                    .index = index, 
                    .line = line,
                    .column = column, 
                    //.raw = word, 
                };
                strncpy(token.raw, &source[index], imin(MAX_RAW_LENGTH - 1, ws_index - index));
                token.raw[imin(MAX_RAW_LENGTH - 1, ws_index - index)] = '\0';
                lexer_token_list = ir_lexer_add_token(lexer_token_list, token_count, token);
                column += ws_index - index + 1;
                index = ws_index;
                found_token = 1;
            }
            continue;
        }



        // firstly, go through all the tokens and compare with those that have a static form, like most keywords
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
                //.raw = strdup(ir_lexer_token_literal[i]), 
            };
            strncpy(token.raw, ir_lexer_token_literal[i], imin(MAX_RAW_LENGTH - 1, strlen(ir_lexer_token_literal[i])));
            token.raw[imin(MAX_RAW_LENGTH - 1, strlen(ir_lexer_token_literal[i]))] = '\0';
            lexer_token_list = ir_lexer_add_token(lexer_token_list, token_count, token);
            index += strlen(ir_lexer_token_literal[i]);
            column += strlen(ir_lexer_token_literal[i]);
            found_token = 1;
            break;
        }
        if (found_token) continue;



        // secondly check if its a number literal
        // first get a copy of the next "word" until the next whitespace, we just need the index of the next whitespace
        {long ws_index = index;
        while (ws_index < source_length) {
            if (!contains(source[ws_index], "0123456789.abcdefxobABCDEFli-$")) break;
            ws_index ++;
        }
        if (ws_index != index) {
            //char word[ws_index - index + 1];
            char word[MAX_RAW_LENGTH];
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
                    //.raw = word, 
                };
                strncpy(token.raw, &source[index], imin(MAX_RAW_LENGTH - 1, ws_index - index));
                token.raw[imin(MAX_RAW_LENGTH - 1, ws_index - index)] = '\0';
                lexer_token_list = ir_lexer_add_token(lexer_token_list, token_count, token);
                column += ws_index - index;
                index = ws_index;
                found_token = 1;
                continue;
            }
        }}

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
            //log_msg(LP_SUCCESS, "IR Lexer: LexerToken found: %s - \"%s\"", token_name[IR_LEX_LABEL], word);
            IRLexerToken_t token = {
                .type = IR_LEX_LABEL, 
                .length = ws_index - index, 
                .index = index, 
                .line = line,
                .column = column, 
                //.raw = word, 
            };
            strncpy(token.raw, &source[index], imin(MAX_RAW_LENGTH - 1, ws_index - index));
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
            //log_msg(LP_SUCCESS, "IR Lexer: LexerToken found: %s - \"%s\"", token_name[IR_LEX_IDENTIFIER], word);
            IRLexerToken_t token = {
                .type = IR_LEX_IDENTIFIER, 
                .length = ws_index - index, 
                .index = index, 
                .line = line,
                .column = column, 
                //.raw = word, 
            };
            strncpy(token.raw, &source[index], imin(MAX_RAW_LENGTH - 1, ws_index - index));
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


