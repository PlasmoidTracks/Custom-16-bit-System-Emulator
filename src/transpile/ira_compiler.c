
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "Log.h"

#include "utils/String.h"

#include "transpile/ira_parser.h"
#include "transpile/ira_token_name_table.h"
#include "transpile/ira_lexer.h"
#include "transpile/ira_compiler.h"

IRAIdentifier_t ira_identifier[IR_MAX_DEPTH][IR_MAX_IDENT];
int ira_identifier_index[IR_MAX_DEPTH] = {0};   // this gets set to 0 after each "IRA_LEX_RETURN", so we can start with a new scope
int ira_identifier_scope_depth = 0;
int ira_static_ira_identifier_count = 0;
int ira_data_segment_address = 0x4000;
int ira_identifier_offset[IR_MAX_DEPTH] = {0};

void append_raw_recursive_ext(char** output, long* length, IRAParserToken_t* token, int depth) {
    if (token->child_count >= 1) {
        for (int i = 0; i < token->child_count; i++) {
            append_raw_recursive_ext(output, length, token->child[i], depth+1);
        }
    } else {
        *output = append_to_output(*output, length, token->token.raw);
        *output = append_to_output(*output, length, " ");
    }
    if (depth == 0) {
        *output = append_to_output(*output, length, "\n");
    }
}

void append_raw_recursive(char** output, long* length, IRAParserToken_t* token) {
    append_raw_recursive_ext(output, length, token, 0);
}

IRAIdentifier_t* ira_get_identifier_from_name(const char* ident_name) {
    //log_msg(LP_DEBUG, "IRA: Looking for ident \"%s\"", ident_name);
    for (int scope = 0; scope <= ira_identifier_scope_depth; scope++) {
        for (int i = 0; i < ira_identifier_index[scope]; i++) {
            if (strcmp(ira_identifier[scope][i].name, ident_name) == 0) {
                return &ira_identifier[scope][i];
                break;
            }
        }
    }
    //log_msg(LP_ERROR, "IRA: Unkonwn ira_identifier \"%s\"", ident_name);
    return NULL;
}

IRATypeModifier_t ira_vardec_get_type_modifier(IRAParserToken_t* parser_token) {
    IRATypeModifier_t mod = 0;
    
    if (parser_token->token.type == IRA_PAR_TYPE || parser_token->token.type == IRA_PAR_TYPE_MODIFIER) {
        for (int i = 0; i < parser_token->child_count; i++) {
            mod |= ira_vardec_get_type_modifier(parser_token->child[i]);
        }
    }

    switch (parser_token->token.type) {
        case IRA_LEX_STATIC:
            mod |= IRA_TM_STATIC;
            break;
        case IRA_LEX_ANON:
            mod |= IRA_TM_ANON;
            break;
        case IRA_LEX_CONST:
            mod |= IRA_TM_CONST;
            break;
        default:
            break;
    }

    return mod;
}

IRAType_t ira_vardec_get_type(IRAParserToken_t* parser_token) {
    if (parser_token->child_count >= 1) {
        if (parser_token->token.type == (IRALexerTokenType_t) IRA_PAR_TYPE) {
            return ira_vardec_get_type(parser_token->child[parser_token->child_count - 1]);
        }
    }
    switch (parser_token->token.type) {
        case IRA_LEX_INT16:
            return IRA_T_INT16;
        
        case IRA_LEX_UINT16:
            return IRA_T_UINT16;

        case IRA_LEX_INT8:
            return IRA_T_INT8;
        
        case IRA_LEX_UINT8:
            return IRA_T_UINT8;

        case IRA_LEX_INT32:
            return IRA_T_INT32;
        
        case IRA_LEX_UINT32:
            return IRA_T_UINT32;
        
        case IRA_LEX_FLOAT16:
            return IRA_T_FLOAT16;
        
        default:
            return 0;
    }

    return 0;
}



char* ira_compile(IRAParserToken_t** parser_token, long parser_token_count, IRACompileOption_t options) {
    char* code_output = NULL;         // final assembly string
    long code_output_len = 0;         // current length of code output

    long parser_token_index = 0;

    while (parser_token_index < parser_token_count) {
        IRAParserToken_t* token = parser_token[parser_token_index];
        switch ((int) token->token.type) {

            case IRA_PAR_VARIABLE_DECLARATION: {
                IRAIdentifier_t* ident = &ira_identifier[ira_identifier_scope_depth][ira_identifier_index[ira_identifier_scope_depth]];
                sprintf(ident->name, "%s", token->child[1]->token.raw);
                ident->type = ira_vardec_get_type(token->child[0]);
                ira_identifier_index[ira_identifier_scope_depth] ++;
                parser_token_index++;
                break;
            }

            case IRA_LEX_SCOPEBEGIN: {
                ira_identifier_scope_depth ++;
                append_raw_recursive(&code_output, &code_output_len, token);
                parser_token_index++;
                break;
            }

            case IRA_LEX_SCOPEEND: {
                ira_identifier_index[ira_identifier_scope_depth] = 0;
                ira_identifier_scope_depth --;
                append_raw_recursive(&code_output, &code_output_len, token);
                parser_token_index++;
                break;
            }
            
            case IRA_PAR_VARIABLE_ASSIGNMENT: {
                append_raw_recursive(&code_output, &code_output_len, token);
                // TODO:
                // get type of left hand variable
                // then get all types of right hand
                // we can ignore literals or give a warning for wrong formats (like int instead of float)
                // handle type cast operation
                // check if right side is same type as left side
                IRAIdentifier_t* ident = ira_get_identifier_from_name(token->child[0]->token.raw);
                IRAType_t left_ident_type = ident->type;
                log_msg(LP_INFO, "name %s - type %d - mod %d", ident->name, ident->type, ident->type_modifier);

                IRAParserToken_t* expr = token->child[2];
                log_msg(LP_INFO, "%s", expr->token.raw);
                

                parser_token_index++;
                break;
            }

            default: {
                append_raw_recursive(&code_output, &code_output_len, token);
                parser_token_index++;
                break;
            }
        }
    }

    for (int d = 0; d < 3; d++) {
        for (int i = 0; i < ira_identifier_index[d]; i++) {
            log_msg(LP_INFO, "depth %d - ident %d - name %s - type %d - mod %d", 
                d, i, ira_identifier[d][i].name, ira_identifier[d][i].type, ira_identifier[d][i].type_modifier);
        }
    }
    
    return code_output;
}

