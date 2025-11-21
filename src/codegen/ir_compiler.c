
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "Log.h"

#include "asm/assembler.h"
#include "utils/String.h"

#include "codegen/ir_parser.h"
#include "codegen/ir_token_name_table.h"
#include "codegen/ir_lexer.h"
#include "codegen/ir_compiler.h"

IRIdentifier_t ir_identifier[IR_MAX_DEPTH][IR_MAX_IDENT];
int ir_identifier_index[IR_MAX_DEPTH] = {0};   // this gets set to 0 after each "IR_LEX_RETURN", so we can start with a new scope
int ir_identifier_scope_depth = 0;
int static_identifier_count = 0;
int data_segment_address = 0x4000;
int identifier_offset[IR_MAX_DEPTH] = {0};

int ir_identifier_get_stack_offset(const char* ident_name) {
    //log_msg(LP_DEBUG, "IR: Looking for ident \"%s\"", ident_name);
    for (int scope = 0; scope <= ir_identifier_scope_depth; scope++) {
        for (int i = 0; i < ir_identifier_index[scope]; i++) {
            if (strcmp(ir_identifier[scope][i].name, ident_name) == 0) {
                return ir_identifier[scope][i].stack_offset;
                break;
            }
        }
    }
    log_msg(LP_ERROR, "IR: Unkonwn ir_identifier \"%s\"", ident_name);
    return 0;
}

IRIdentifier_t* ir_get_identifier_from_name(const char* ident_name) {
    //log_msg(LP_DEBUG, "IR: Looking for ident \"%s\"", ident_name);
    for (int scope = 0; scope <= ir_identifier_scope_depth; scope++) {
        for (int i = 0; i < ir_identifier_index[scope]; i++) {
            if (strcmp(ir_identifier[scope][i].name, ident_name) == 0) {
                return &ir_identifier[scope][i];
                break;
            }
        }
    }
    //log_msg(LP_ERROR, "IR: Unkonwn ir_identifier \"%s\"", ident_name);
    return NULL;
}

IRTypeModifier_t vardec_get_type_modifier(IRParserToken_t* parser_token) {
    IRTypeModifier_t tm = 0;

    if (parser_token->child_count == 0) {
        // at leaf
        IRLexerTokenType_t tt = parser_token->token.type;
        switch(tt) {
            case IR_LEX_STATIC:
                tm |= IR_TM_STATIC;
                break;
            
            case IR_LEX_CONST:
                tm |= IR_TM_CONST;
                break;
            
            case IR_LEX_ANON:
                tm |= IR_TM_ANON;
                break;
            
            case IR_LEX_VAR:
                break;
            
            default:
                log_msg(LP_ERROR, "IR: Unknown type modifier \"%s\"", ir_token_name[tt]);
                break;
        }
    } else {
        for (int i = 0; i < parser_token->child_count; i++) {
            tm |= vardec_get_type_modifier(parser_token->child[i]);
        }
    }

    return tm;
}


void parser_evaluate_expression(char** output, long* length, IRParserToken_t* expr) {
    if (expr->child_count == 1) {
        IRLexerToken_t token = expr->child[0]->token;
        switch (token.type) {
            // number
            case IR_LEX_NUMBER: {
                *output = append_to_output(*output, length, "; expression -> loading number\n");
                uint16_t imm = parse_immediate(token.raw);
                char tmp[32];
                sprintf(tmp, "mov r1, %d\n", imm);
                *output = append_to_output(*output, length, tmp);
                break;
            }
            case IR_LEX_CHAR_LITERAL: {
                *output = append_to_output(*output, length, "; expression -> loading char literal\n");
                uint16_t imm = token.raw[1];
                char tmp[32];
                sprintf(tmp, "mov r1, %d\n", imm);
                *output = append_to_output(*output, length, tmp);
                break;
            }

            // identifier
            case IR_LEX_IDENTIFIER: {
                IRIdentifier_t* ident = ir_get_identifier_from_name(token.raw);
                if (!ident) {
                    log_msg(LP_ERROR, "IR: Unknown ir_identifier \"%s\"", token.raw);
                    break;
                }
                if (!(ident->type_modifier & IR_TM_STATIC)) {
                    *output = append_to_output(*output, length, "; expression -> loading ir_identifier from stack\n");
                    int stack_offset = ident->stack_offset;
                    char tmp[32];
                    sprintf(tmp, "mov r1, [r3 + %d]", stack_offset);
                    *output = append_to_output(*output, length, tmp);
                    sprintf(tmp, "\t; %s", token.raw);
                    *output = append_to_output(*output, length, tmp);
                    *output = append_to_output(*output, length, "\n");
                } else if (ident->type_modifier & IR_TM_STATIC) {
                    *output = append_to_output(*output, length, "; expression -> loading static ir_identifier from data segment\n");
                    int absolute_address = ident->absolute_address;
                    char tmp[32];
                    sprintf(tmp, "mov r1, [%d]", absolute_address);
                    *output = append_to_output(*output, length, tmp);
                    sprintf(tmp, "\t; %s", token.raw);
                    *output = append_to_output(*output, length, tmp);
                    *output = append_to_output(*output, length, "\n");
                }
                break;
            }

            // arg
            case IR_LEX_ARG: {
                // we set r1 to the pointer of arg, starting at the first (reverse order) argument, hence the add 4
                *output = append_to_output(*output, length, "; expression -> loading argument pointer\n");
                *output = append_to_output(*output, length, "mov r1, r3\nadd r1, 4\n");   
                break;
            }
            
            default:
                log_msg(LP_CRITICAL, "Unknown expression \"%s\"", ir_token_name[token.type]);
                break;
        }
    } else if (expr->child_count == 2) {
        *output = append_to_output(*output, length, "; unary operation\n");
        IRParserToken_t* token0 = expr->child[0];     // unary operator
        IRParserToken_t* op = token0->child[0];       // operand
        IRParserToken_t* token1 = expr->child[1];
        // mov token1 to r0?
        // then do the operation, which will be set in r1, r0 can be discarded
        switch(op->token.type) {
            case IR_LEX_REF: {
                *output = append_to_output(*output, length, "; unary operation -> ref\n");
                IRIdentifier_t* ident = ir_get_identifier_from_name(token1->child[0]->token.raw);
                if (ident->type_modifier & IR_TM_STATIC) {
                    char tmp[32];
                    sprintf(tmp, "lea r1, [%d]", ident->absolute_address);
                    *output = append_to_output(*output, length, tmp);
                    sprintf(tmp, "\t; %s", token1->child[0]->token.raw);
                    *output = append_to_output(*output, length, tmp);
                    *output = append_to_output(*output, length, "\n");
                } else {
                    char tmp[32];
                    sprintf(tmp, "lea r1, [r3 + %d]", ident->stack_offset);
                    *output = append_to_output(*output, length, tmp);
                    sprintf(tmp, "\t; %s", token1->child[0]->token.raw);
                    *output = append_to_output(*output, length, tmp);
                    *output = append_to_output(*output, length, "\n");
                }
                break;
            }
            
            case IR_LEX_DEREF:
                parser_evaluate_expression(output, length, token1);
                *output = append_to_output(*output, length, "; unary operation -> deref\n");
                *output = append_to_output(*output, length, "mov r0, [r1]\nmov r1, r0\n");
                break;

            case IR_LEX_CIF:
                parser_evaluate_expression(output, length, token1);
                *output = append_to_output(*output, length, "; unary operation -> cif\n");
                *output = append_to_output(*output, length, "cif r1\n");
                break;

            case IR_LEX_CFI:
                parser_evaluate_expression(output, length, token1);
                *output = append_to_output(*output, length, "; unary operation -> cfi\n");
                *output = append_to_output(*output, length, "cfi r1\n");
                break;

            case IR_LEX_CBW:
                parser_evaluate_expression(output, length, token1);
                *output = append_to_output(*output, length, "; unary operation -> cbw\n");
                *output = append_to_output(*output, length, "cbw r1\n");
                break;

            case IR_LEX_BITWISE_NOT:
                parser_evaluate_expression(output, length, token1);
                *output = append_to_output(*output, length, "; unary operation -> not\n");
                *output = append_to_output(*output, length, "not r1\n");
                break;

            default:
                log_msg(LP_CRITICAL, "Unknown unary operation \"%s\"", ir_token_name[op->token.type]);
                break;
        }

    } else if (expr->child_count == 3) {
        *output = append_to_output(*output, length, "; binary operation\n");
        // arg + expr
        IRParserToken_t* token0 = expr->child[0];
        IRParserToken_t* token1 = expr->child[1]->child[0];  // binary operator
        IRParserToken_t* token2 = expr->child[2];
        // mov token0 to r0 and token2 to r1?
        parser_evaluate_expression(output, length, token0);
        *output = append_to_output(*output, length, "mov r0, r1\n");
        parser_evaluate_expression(output, length, token2);
        // then do the operation, which will be set in r1, r0 can be discarded
        switch(token1->token.type) {
            case IR_LEX_INTEGER_PLUS:
                *output = append_to_output(*output, length, "; binary operation -> add\n");
                *output = append_to_output(*output, length, "add r0, r1\nmov r1, r0\n");
                break;

            case IR_LEX_FLOAT_PLUS:
                *output = append_to_output(*output, length, "; binary operation -> addf\n");
                *output = append_to_output(*output, length, "addf r0, r1\nmov r1, r0\n");
                break;

            case IR_LEX_INTEGER_MINUS:
                *output = append_to_output(*output, length, "; binary operation -> sub\n");
                *output = append_to_output(*output, length, "sub r0, r1\nmov r1, r0\n");
                break;

            case IR_LEX_FLOAT_MINUS:
                *output = append_to_output(*output, length, "; binary operation -> subf\n");
                *output = append_to_output(*output, length, "subf r0, r1\nmov r1, r0\n");
                break;

            case IR_LEX_INTEGER_STAR:
                *output = append_to_output(*output, length, "; binary operation -> mul\n");
                *output = append_to_output(*output, length, "mul r0, r1\nmov r1, r0\n");
                break;

            case IR_LEX_FLOAT_STAR:
                *output = append_to_output(*output, length, "; binary operation -> mulf\n");
                *output = append_to_output(*output, length, "mulf r0, r1\nmov r1, r0\n");
                break;

            case IR_LEX_INTEGER_SLASH:
                *output = append_to_output(*output, length, "; binary operation -> div\n");
                *output = append_to_output(*output, length, "div r0, r1\nmov r1, r0\n");
                break;

            case IR_LEX_FLOAT_SLASH:
                *output = append_to_output(*output, length, "; binary operation -> divf\n");
                *output = append_to_output(*output, length, "divf r0, r1\nmov r1, r0\n");
                break;

            case IR_LEX_EQUAL_EQUAL:
                *output = append_to_output(*output, length, "; binary operation -> equal\n");
                *output = append_to_output(*output, length, "cmp r0, r1\nmov r1, 0\ncmovz r1, 1\n");
                break;

            case IR_LEX_BANG_EQUAL:
                *output = append_to_output(*output, length, "; binary operation -> not equal\n");
                *output = append_to_output(*output, length, "cmp r0, r1\nmov r1, 0\ncmovnz r1, 1\n");
                break;

            case IR_LEX_INTEGER_LESS:
                *output = append_to_output(*output, length, "; binary operation -> less\n");
                *output = append_to_output(*output, length, "cmp r0, r1\nmov r1, 0\ncmovl r1, 1\ncmovz r1, 0\n");
                break;

            case IR_LEX_INTEGER_LESS_EQUAL:
                *output = append_to_output(*output, length, "; binary operation -> less equal\n");
                *output = append_to_output(*output, length, "cmp r0, r1\nmov r1, 0\ncmovl r1, 1\n");
                break;
                
            case IR_LEX_INTEGER_GREATER:
                *output = append_to_output(*output, length, "; binary operation -> greater\n");
                *output = append_to_output(*output, length, "cmp r0, r1\nmov r1, 0\ncmovnl r1, 1\ncmovz r1, 0\n");
                break;

            case IR_LEX_INTEGER_GREATER_EQUAL:
                *output = append_to_output(*output, length, "; binary operation -> greater equal\n");
                *output = append_to_output(*output, length, "cmp r0, r1\nmov r1, 0\ncmovnl r1, 1\n");
                break;

            case IR_LEX_FLOAT_LESS:
                *output = append_to_output(*output, length, "; binary operation -> float less\n");
                *output = append_to_output(*output, length, "cmp r0, r1\nmov r1, 0\ncmovfl r1, 1\ncmovz r1, 0\n");
                break;

            case IR_LEX_FLOAT_LESS_EQUAL:
                *output = append_to_output(*output, length, "; binary operation -> float less equal\n");
                *output = append_to_output(*output, length, "cmp r0, r1\nmov r1, 0\ncmovfl r1, 1\n");
                break;
                
            case IR_LEX_FLOAT_GREATER:
                *output = append_to_output(*output, length, "; binary operation -> float greater\n");
                *output = append_to_output(*output, length, "cmp r0, r1\nmov r1, 0\ncmovnfl r1, 1\ncmovz r1, 0\n");
                break;

            case IR_LEX_FLOAT_GREATER_EQUAL:
                *output = append_to_output(*output, length, "; binary operation -> float greater equal\n");
                *output = append_to_output(*output, length, "cmp r0, r1\nmov r1, 0\ncmovnfl r1, 1\n");
                break;

            case IR_LEX_UNSIGNED_INTEGER_LESS:
                *output = append_to_output(*output, length, "; binary operation -> unsigned less\n");
                *output = append_to_output(*output, length, "cmp r0, r1\nmov r1, 0\ncmovul r1, 1\ncmovz r1, 0\n");
                break;

            case IR_LEX_UNSIGNED_INTEGER_LESS_EQUAL:
                *output = append_to_output(*output, length, "; binary operation -> unsigned less equal\n");
                *output = append_to_output(*output, length, "cmp r0, r1\nmov r1, 0\ncmovul r1, 1\n");
                break;
                
            case IR_LEX_UNSIGNED_INTEGER_GREATER:
                *output = append_to_output(*output, length, "; binary operation -> unsigned greater\n");
                *output = append_to_output(*output, length, "cmp r0, r1\nmov r1, 0\ncmovnul r1, 1\ncmovz r1, 0\n");
                break;

            case IR_LEX_UNSIGNED_INTEGER_GREATER_EQUAL:
                *output = append_to_output(*output, length, "; binary operation -> unsigned greater equal\n");
                *output = append_to_output(*output, length, "cmp r0, r1\nmov r1, 0\ncmovnul r1, 1\n");
                break;

            case IR_LEX_SHIFT_RIGHT:
                *output = append_to_output(*output, length, "; binary operation -> shl\n");
                *output = append_to_output(*output, length, "bws r0, r1\nmov r1, r0\n");
                break;

            case IR_LEX_SHIFT_LEFT:
                *output = append_to_output(*output, length, "; binary operation -> shl\n");
                *output = append_to_output(*output, length, "neg r1\nbws r0, r1\nmov r1, r0\n");
                break;

            case IR_LEX_BITWISE_AND:
                *output = append_to_output(*output, length, "; binary operation -> and\n");
                *output = append_to_output(*output, length, "and r0, r1\nmov r1, r0\n");
                break;

            case IR_LEX_BITWISE_OR:
                *output = append_to_output(*output, length, "; binary operation -> or\n");
                *output = append_to_output(*output, length, "or r0, r1\nmov r1, r0\n");
                break;

            case IR_LEX_BITWISE_XOR:
                *output = append_to_output(*output, length, "; binary operation -> xor\n");
                *output = append_to_output(*output, length, "xor r0, r1\nmov r1, r0\n");
                break;

            default:
                log_msg(LP_CRITICAL, "Unknown binary operation \"%s\"", ir_token_name[token1->token.type]);
                char tmp[256];
                sprintf(tmp, "; UNKNOWN TOKEN: %s\n", ir_token_name[token1->token.type]);
                *output = append_to_output(*output, length, tmp);
                break;
        }

    }
}

char* ir_compile(IRParserToken_t** parser_token, long parser_token_count, IRCompileOption_t options) {
    char* code_output = NULL;         // final assembly string
    char* data_output = NULL;         // final data string
    long code_output_len = 0;         // current length of code output
    long data_output_len = 0;         // current length of data output

    long parser_token_index = 0;

    (void) options;

    // default header
    code_output = append_to_output(code_output, &code_output_len, "; code setup\n");
    code_output = append_to_output(code_output, &code_output_len, ".address 0\n.code\ncall .main\nhlt\n");
    {char tmp[32];
    data_output = append_to_output(data_output, &data_output_len, "; data setup\n");
    sprintf(tmp, ".address 0x%.4x\n.data\n", data_segment_address);
    data_output = append_to_output(data_output, &data_output_len, tmp);}

    while (parser_token_index < parser_token_count) {
        IRParserToken_t* token = parser_token[parser_token_index];
        switch ((int) token->token.type) {

            case IR_LEX_COMMENT:
                code_output = append_to_output(code_output, &code_output_len, ";"); 
                code_output = append_to_output_ext(code_output, &code_output_len, token->token.raw , token->token.length);
                code_output = append_to_output(code_output, &code_output_len, "\n"); 
                parser_token_index++;
                break;
            
            case IR_LEX_SEMICOLON:
                parser_token_index++;
                break;

            case IR_LEX_LABEL: 
                code_output = append_to_output_ext(code_output, &code_output_len, token->token.raw, token->token.length);
                code_output = append_to_output(code_output, &code_output_len, "\n"); 
                parser_token_index++;
                break;
            
            case IR_PAR_VARIABLE_DECLARATION: {
                code_output = append_to_output(code_output, &code_output_len, "; variable declaration\n");
                // Add ir_identifier to current frame list
                if (ir_identifier_index[ir_identifier_scope_depth] >= IR_MAX_IDENT) {
                    log_msg(LP_ERROR, "IR: Too many identifiers, max is %d", IR_MAX_IDENT);
                    return NULL;
                }

                const char* ident_name = parser_token[parser_token_index]->child[1]->token.raw;
                IRIdentifier_t* ident = ir_get_identifier_from_name(ident_name);

                IRTypeModifier_t type_mod = vardec_get_type_modifier(parser_token[parser_token_index]->child[0]);
                //log_msg(LP_NOTICE, "type modifier: %.2o", type_mod);
                if (ident && !(type_mod & IR_TM_ANON)) {
                    log_msg(LP_ERROR, "IR: Redeclaration of already existing variable \"%s\"", ident_name);
                    return NULL;
                }

                if (type_mod & IR_TM_ANON) {
                    if (type_mod & IR_TM_STATIC) {
                        log_msg(LP_ERROR, "IR: Static variables cannot be anonymous");
                        return NULL;
                    } else {
                        //log_msg(LP_DEBUG, "IR: Added anonymous ir_identifier");
                        identifier_offset[ir_identifier_scope_depth] += 1;
                        code_output = append_to_output(code_output, &code_output_len, "sub sp, 2\n");
                    }
                } else {
                    if (type_mod & IR_TM_STATIC) {
                        memcpy(ir_identifier[ir_identifier_scope_depth][ir_identifier_index[ir_identifier_scope_depth]].name, ident_name, strlen(ident_name));
                        ir_identifier[ir_identifier_scope_depth][ir_identifier_index[ir_identifier_scope_depth]].name[strlen(ident_name)] = '\0';
                        ir_identifier[ir_identifier_scope_depth][ir_identifier_index[ir_identifier_scope_depth]].type_modifier = type_mod;
                        ir_identifier[ir_identifier_scope_depth][ir_identifier_index[ir_identifier_scope_depth]].absolute_address = data_segment_address + (2 * static_identifier_count);
                        ir_identifier[ir_identifier_scope_depth][ir_identifier_index[ir_identifier_scope_depth]].initialized = 0;
                        ir_identifier[ir_identifier_scope_depth][ir_identifier_index[ir_identifier_scope_depth]].identifier_index = static_identifier_count;
                        ir_identifier[ir_identifier_scope_depth][ir_identifier_index[ir_identifier_scope_depth]].is_stack_variable = 0;
                        //log_msg(LP_DEBUG, "IR: Added static ir_identifier \"%s\"", ir_identifier[ir_identifier_scope_depth][ir_identifier_index[ir_identifier_scope_depth]].name);
                        ir_identifier_index[ir_identifier_scope_depth] ++;
                        static_identifier_count ++;
                    } else {
                        memcpy(ir_identifier[ir_identifier_scope_depth][ir_identifier_index[ir_identifier_scope_depth]].name, ident_name, strlen(ident_name));
                        ir_identifier[ir_identifier_scope_depth][ir_identifier_index[ir_identifier_scope_depth]].name[strlen(ident_name)] = '\0';
                        ir_identifier[ir_identifier_scope_depth][ir_identifier_index[ir_identifier_scope_depth]].type_modifier = type_mod;
                        ir_identifier[ir_identifier_scope_depth][ir_identifier_index[ir_identifier_scope_depth]].stack_offset = -2 * (1 + identifier_offset[ir_identifier_scope_depth]);
                        ir_identifier[ir_identifier_scope_depth][ir_identifier_index[ir_identifier_scope_depth]].initialized = 0;
                        ir_identifier[ir_identifier_scope_depth][ir_identifier_index[ir_identifier_scope_depth]].identifier_index = ir_identifier_index[ir_identifier_scope_depth];
                        ir_identifier[ir_identifier_scope_depth][ir_identifier_index[ir_identifier_scope_depth]].is_stack_variable = 1;
                        //log_msg(LP_DEBUG, "IR: Added ir_identifier \"%s\"", ir_identifier[ir_identifier_scope_depth][ir_identifier_index[ir_identifier_scope_depth]].name);
                        ir_identifier_index[ir_identifier_scope_depth] ++;
                        identifier_offset[ir_identifier_scope_depth] += 1;

                        code_output = append_to_output(code_output, &code_output_len, "sub sp, 2\n");
                    }
                }
                
                parser_token_index ++;
                break;
            }
            
            case IR_PAR_VARIABLE_ASSIGNMENT: {
                // TODO: First solve expression, then save it in r1, the find ir_identifier, set r0 to the ident and mov indirectly?
                IRIdentifier_t* ident = ir_get_identifier_from_name(parser_token[parser_token_index]->child[0]->token.raw);
                if (!ident) {
                    log_msg(LP_ERROR, "IR: Unknown ir_identifier \"%s\"", parser_token[parser_token_index]->child[0]->token.raw);
                    parser_token_index ++;
                    break;
                }
                if (!(ident->type_modifier & IR_TM_STATIC)) {
                    code_output = append_to_output(code_output, &code_output_len, "; stack variable assignment\n");
                    IRParserToken_t* expr = parser_token[parser_token_index]->child[2];
                    parser_evaluate_expression(&code_output, &code_output_len, expr);
                    int stack_offset = ident->stack_offset;
                    if (stack_offset == 0) {return NULL;}
                    char tmp[64];
                    sprintf(tmp, "lea r0, [r3 + %d]", stack_offset);
                    code_output = append_to_output(code_output, &code_output_len, tmp);
                    sprintf(tmp, "\t; %s\n", parser_token[parser_token_index]->child[0]->token.raw);
                    code_output = append_to_output(code_output, &code_output_len, tmp);
                    code_output = append_to_output(code_output, &code_output_len, "mov [r0], r1\n");
                    parser_token_index ++;
                } else if (ident->type_modifier & IR_TM_STATIC) {
                    code_output = append_to_output(code_output, &code_output_len, "; static variable assignment\n");
                    IRParserToken_t* expr = parser_token[parser_token_index]->child[2];
                    parser_evaluate_expression(&code_output, &code_output_len, expr);
                    int absolute_address = ident->absolute_address;
                    char tmp[64];
                    sprintf(tmp, "lea r0, [%d]", absolute_address);
                    code_output = append_to_output(code_output, &code_output_len, tmp);
                    sprintf(tmp, "\t; %s\n", parser_token[parser_token_index]->child[0]->token.raw);
                    code_output = append_to_output(code_output, &code_output_len, tmp);
                    code_output = append_to_output(code_output, &code_output_len, "mov [r0], r1\n");
                    parser_token_index ++;
                }
                break;
            }
            
            case IR_PAR_DEREF_VARIABLE_ASSIGNMENT: {
                code_output = append_to_output(code_output, &code_output_len, "; deref variable assignment\n");
                IRIdentifier_t* ident = ir_get_identifier_from_name(parser_token[parser_token_index]->child[1]->token.raw);
                if (!ident) {
                    log_msg(LP_ERROR, "IR: Unknown ir_identifier \"%s\"", parser_token[parser_token_index]->child[1]->token.raw);
                    parser_token_index ++;
                    break;
                }
                if (!(ident->type_modifier & IR_TM_STATIC)) {
                    // TODO: First solve expression, then save it in r1, the find ir_identifier, set r0 to the ident and mov indirectly?
                    IRParserToken_t* expr = parser_token[parser_token_index]->child[3];
                    parser_evaluate_expression(&code_output, &code_output_len, expr);
                    int stack_offset = ident->stack_offset;
                    if (stack_offset == 0) {return NULL;}
                    char tmp[64];
                    sprintf(tmp, "lea r0, [r3 + %d]", stack_offset);
                    code_output = append_to_output(code_output, &code_output_len, tmp);
                    sprintf(tmp, "\t; %s\n", parser_token[parser_token_index]->child[1]->token.raw);
                    code_output = append_to_output(code_output, &code_output_len, tmp);
                    code_output = append_to_output(code_output, &code_output_len, "mov r0, [r0]\nmov [r0], r1\n");
                    parser_token_index ++;
                } else if (ident->type_modifier & IR_TM_STATIC) {
                    IRParserToken_t* expr = parser_token[parser_token_index]->child[3];
                    parser_evaluate_expression(&code_output, &code_output_len, expr);
                    int absolute_address = ident->absolute_address;
                    char tmp[64];
                    sprintf(tmp, "lea r0, [%d]", absolute_address);
                    code_output = append_to_output(code_output, &code_output_len, tmp);
                    sprintf(tmp, "\t; %s\n", parser_token[parser_token_index]->child[1]->token.raw);
                    code_output = append_to_output(code_output, &code_output_len, tmp);
                    code_output = append_to_output(code_output, &code_output_len, "mov r0, [r0]\nmov [r0], r1\n");
                    parser_token_index ++;
                }
                break;
            }
            
            case IR_PAR_VARIABLE_FUNCTION_POINTER_ASSIGNMENT: {
                code_output = append_to_output(code_output, &code_output_len, "; variable function pointer assignment\n");
                IRIdentifier_t* ident = ir_get_identifier_from_name(parser_token[parser_token_index]->child[0]->token.raw);
                if (!ident) {
                    log_msg(LP_ERROR, "IR: Unknown ir_identifier \"%s\"", parser_token[parser_token_index]->child[0]->token.raw);
                    parser_token_index ++;
                    break;
                }
                if (!(ident->type_modifier & IR_TM_STATIC)) {
                    // TODO: First solve expression, then save it in r1, the find ir_identifier, set r0 to the ident and mov indirectly?
                    int stack_offset = ident->stack_offset;
                    if (stack_offset == 0) {return NULL;}
                    char tmp[64];
                    sprintf(tmp, "lea r0, [r3 + %d]\n", stack_offset);
                    code_output = append_to_output(code_output, &code_output_len, tmp);
                    sprintf(tmp, "mov [r0], %s\n", parser_token[parser_token_index]->child[2]->token.raw);
                    code_output = append_to_output(code_output, &code_output_len, tmp);
                    parser_token_index ++;
                } else if (ident->type_modifier & IR_TM_STATIC) {
                    // TODO: First solve expression, then save it in r1, the find ir_identifier, set r0 to the ident and mov indirectly?
                    int absolute_address = ident->absolute_address;
                    char tmp[64];
                    sprintf(tmp, "lea r0, [%d]\n", absolute_address);
                    code_output = append_to_output(code_output, &code_output_len, tmp);
                    sprintf(tmp, "mov [r0], %s\n", parser_token[parser_token_index]->child[2]->token.raw);
                    code_output = append_to_output(code_output, &code_output_len, tmp);
                    parser_token_index ++;
                }
                break;
            }
            
            case IR_PAR_VARIABLE_REF_STRING_ASSIGNMENT: {
                code_output = append_to_output(code_output, &code_output_len, "; ref string assignment\n");
                IRIdentifier_t* ident = ir_get_identifier_from_name(parser_token[parser_token_index]->child[0]->token.raw);

                if (!(ident->type_modifier & IR_TM_STATIC)) {
                    
                    // set the variable of the string to the pointer (one next in stack), because strings are always pointers, especially in data segment
                    log_msg(LP_ERROR, "IR: String allocation of \"%s\" is not permitted in stack as of now", ident->name);
                    parser_token_index ++;
                    break;

                    const char* str = parser_token[parser_token_index]->child[3]->token.raw + 1;
                    int len = parser_token[parser_token_index]->child[3]->token.length - 2; // -2 to compensate for the '"'
                    int nulls = 2 - ((len + 1) % 2);
                    //int total_bytes = (len + 2) / 2;

                    char tmp[320];
                    //ident->stack_offset = ident->stack_offset - (len / 2) * 2 - (len % 2) * 2;
                    sprintf(tmp, "sub sp, %d\nlea r0, [r3 + %d]\t; ptr of %s\n", (len / 2) * 2 - (len % 2) * 2, ident->stack_offset, ident->name);
                    code_output = append_to_output(code_output, &code_output_len, tmp);
                    identifier_offset[ir_identifier_scope_depth] ++;
                    sprintf(tmp, "lea r1, [r3 + %d]\t; content of string %s\n", ident->stack_offset - (len / 2) * 2 - (len % 2) * 2, ident->name);
                    code_output = append_to_output(code_output, &code_output_len, tmp);
                    code_output = append_to_output(code_output, &code_output_len, "mov [r0], r1\n");

                    sprintf(tmp, "lea r0, [r3 + %d]\n", ident->stack_offset - (len / 2) * 2 - (len % 2) * 2);
                    code_output = append_to_output(code_output, &code_output_len, tmp);

                    if (nulls == 2) {
                        // we know its always ending in an even byte from nulls == 2
                        identifier_offset[ir_identifier_scope_depth] ++;
                        for (int i = 0; i < len - 1; i+=2) {
                            char tmp2[32];
                            sprintf(tmp2, "mov [r0], $%.4x\t; '%c%c'\n", str[i] | (str[i+1] << 8), str[i], str[i+1]);
                            code_output = append_to_output(code_output, &code_output_len, tmp2);
                            code_output = append_to_output(code_output, &code_output_len, "add r0, 2\n");
                            identifier_offset[ir_identifier_scope_depth] ++;
                        }
                        code_output = append_to_output(code_output, &code_output_len, "mov [r0], $0000\t; '\\0'\n");
                    } else {
                        // here it ends in odd bytes
                        identifier_offset[ir_identifier_scope_depth] ++;
                        int i = 0;
                        for (; i < len - 2; i+=2) {
                            char tmp2[32];
                            sprintf(tmp2, "mov [r0], $%.4x\t; '%c%c'\n", str[i] | (str[i+1] << 8), str[i], str[i+1]);
                            code_output = append_to_output(code_output, &code_output_len, tmp2);
                            code_output = append_to_output(code_output, &code_output_len, "add r0, 2\n");
                            identifier_offset[ir_identifier_scope_depth] ++;
                        }
                        sprintf(tmp, "mov [r0], $00%.2x\t; '%c\\0'\n", str[i], str[i]);
                        code_output = append_to_output(code_output, &code_output_len, tmp);
                    }
                    identifier_offset[ir_identifier_scope_depth] --;

                    parser_token_index ++;
                    break;
                } else if (ident->type_modifier & IR_TM_STATIC) {
                    if (ident->initialized) {
                        log_msg(LP_ERROR, "IR: Already initialized const variable \"%s\" cannot be reassigned", parser_token[parser_token_index]->child[0]->token.raw);
                        parser_token_index ++;
                        break;
                    }
                    int len = parser_token[parser_token_index]->child[3]->token.length - 2;
                    char tmp[280];
                    sprintf(tmp, ".text %s\t; %s\n", parser_token[parser_token_index]->child[3]->token.raw, ident->name);
                    data_output = append_to_output(data_output, &data_output_len, tmp);
                    static_identifier_count += len / 2 - 1;
                    parser_token_index ++;
                    break;
                }
                parser_token_index ++;
                break;
            }

            case IR_LEX_SCOPEBEGIN:
                code_output = append_to_output(code_output, &code_output_len, "; scope begin\n");
                // Function requires pushing of the old frame pointer in case it arrived from another function call
                // and then r3 is set to the current stack-pointer to act as the frame-pointer
                code_output = append_to_output(code_output, &code_output_len, "push r3\n");
                code_output = append_to_output(code_output, &code_output_len, "mov r3, sp\n");
                parser_token_index++;
                ir_identifier_scope_depth ++;
                break;

            case IR_LEX_SCOPEEND:
                code_output = append_to_output(code_output, &code_output_len, "; scope end\n");
                parser_token_index++;
                ir_identifier_index[ir_identifier_scope_depth] = 0;
                identifier_offset[ir_identifier_scope_depth] = 0;
                ir_identifier_scope_depth --;
                break;

            case IR_LEX_RETURN:
                code_output = append_to_output(code_output, &code_output_len, "; function return\n");
                // Restoring the stack pointer with frame pointer and restoring old frame pointer
                code_output = append_to_output(code_output, &code_output_len, "mov sp, r3\npop r3\nret\n");
                parser_token_index++;
                break;
            
            case IR_PAR_CALLPUSHARG: {
                code_output = append_to_output(code_output, &code_output_len, "; callpusharg\n");
                IRParserToken_t* expr = parser_token[parser_token_index]->child[1];
                code_output = append_to_output(code_output, &code_output_len, "sub sp, 2\n");
                parser_evaluate_expression(&code_output, &code_output_len, expr);
                // expr is now in r1
                code_output = append_to_output(code_output, &code_output_len, "lea r0, [sp]\nmov [r0], r1\n");
                parser_token_index++;
                break;
            }
            
            case IR_PAR_CALL_LABEL: {
                code_output = append_to_output(code_output, &code_output_len, "; call\n");
                char tmp[256];
                sprintf(tmp, "call %s\n", token->child[1]->token.raw);
                code_output = append_to_output(code_output, &code_output_len, tmp);
                parser_token_index++;
                break;
            }
            
            case IR_PAR_CALL_EXPRESSION: {
                code_output = append_to_output(code_output, &code_output_len, "; call\n");
                IRParserToken_t* expr = parser_token[parser_token_index]->child[1];
                parser_evaluate_expression(&code_output, &code_output_len, expr);
                code_output = append_to_output(code_output, &code_output_len, "call r1\n");
                parser_token_index++;
                break;
            }
            
            case IR_PAR_CALLFREEARG: {
                code_output = append_to_output(code_output, &code_output_len, "; callfreearg\n");
                char tmp[256];
                sprintf(tmp, "add sp, %s\n", token->child[1]->token.raw);
                code_output = append_to_output(code_output, &code_output_len, tmp);
                parser_token_index++;
                break;
            }
            
            case IR_PAR_GOTO: {
                code_output = append_to_output(code_output, &code_output_len, "; goto\n");
                IRParserToken_t* expr = parser_token[parser_token_index]->child[1];
                if (expr->child_count == 0) {
                    IRLexerToken_t lexer_token = expr->token;
                    switch (lexer_token.type) {
                        case IR_LEX_LABEL: {
                            code_output = append_to_output(code_output, &code_output_len, "; expression -> loading label\n");
                            char tmp[32];
                            sprintf(tmp, "mov r1, %s\n", lexer_token.raw);
                            code_output = append_to_output(code_output, &code_output_len, tmp);
                            break;
                        }
                        
                        default:
                            log_msg(LP_CRITICAL, "Unknown expression \"%s\"", ir_token_name[lexer_token.type]);
                            char tmp[256];
                            sprintf(tmp, "; UNKNOWN TOKEN: %s\n", ir_token_name[lexer_token.type]);
                            code_output = append_to_output(code_output, &code_output_len, tmp);
                            break;
                    }
                }
                parser_evaluate_expression(&code_output, &code_output_len, expr);
                char tmp[256];
                sprintf(tmp, "jmp r1\n");
                code_output = append_to_output(code_output, &code_output_len, tmp);
                parser_token_index++;
                break;
            }
            
            case IR_PAR_IF: {
                code_output = append_to_output(code_output, &code_output_len, "; if condition\n");
                // TODO: First solve expression, then save it in r1, the find ir_identifier, set r0 to the ident and mov indirectly?
                IRParserToken_t* expr = parser_token[parser_token_index]->child[1];
                parser_evaluate_expression(&code_output, &code_output_len, expr);
                char tmp[64];
                sprintf(tmp, "tst r1\njnz %s\n", parser_token[parser_token_index]->child[2]->token.raw);
                code_output = append_to_output(code_output, &code_output_len, tmp);
                parser_token_index ++;
                break;
            }
            
            case IR_PAR_ADDRESS: {
                code_output = append_to_output(code_output, &code_output_len, "; .address\n");
                char tmp[256];
                sprintf(tmp, ".address %s\n", parser_token[parser_token_index]->child[1]->token.raw);
                code_output = append_to_output(code_output, &code_output_len, tmp);
                parser_token_index++;
                break;
            }

            case IR_LEX_IRQBEGIN:
                code_output = append_to_output(code_output, &code_output_len, "; irq begin\n");
                // Function requires pushing of the old frame pointer in case it arrived from another function call
                // and then r3 is set to the current stack-pointer to act as the frame-pointer
                code_output = append_to_output(code_output, &code_output_len, "semi\n");
                code_output = append_to_output(code_output, &code_output_len, "push r3\n");
                code_output = append_to_output(code_output, &code_output_len, "mov r3, sp\n");
                code_output = append_to_output(code_output, &code_output_len, "push r2\n");
                code_output = append_to_output(code_output, &code_output_len, "push r1\n");
                code_output = append_to_output(code_output, &code_output_len, "push r0\n");
                code_output = append_to_output(code_output, &code_output_len, "pushsr\n");
                parser_token_index++;
                ir_identifier_scope_depth ++;
                break;

            case IR_LEX_IRQEND:
                code_output = append_to_output(code_output, &code_output_len, "; irq end\n");
                code_output = append_to_output(code_output, &code_output_len, "popsr\n");
                code_output = append_to_output(code_output, &code_output_len, "pop r0\n");
                code_output = append_to_output(code_output, &code_output_len, "pop r1\n");
                code_output = append_to_output(code_output, &code_output_len, "pop r2\n");
                code_output = append_to_output(code_output, &code_output_len, "clmi\n");
                parser_token_index++;
                ir_identifier_index[ir_identifier_scope_depth] = 0;
                identifier_offset[ir_identifier_scope_depth] = 0;
                ir_identifier_scope_depth --;
                break;
            
            case IR_PAR_INLINE_ASM: {
                code_output = append_to_output(code_output, &code_output_len, "; inline assembly\n");
                // TODO: First solve expression, then save it in r1, the find ir_identifier, set r0 to the ident and mov indirectly?
                const char* asm = parser_token[parser_token_index]->child[1]->token.raw;
                char inline_asm[strlen(asm) + 16];
                strncpy(inline_asm, asm + 1, strlen(asm) - 2);
                inline_asm[strlen(asm) - 2] = '\0';
                char** splits = split(inline_asm, " ,\t", "");
                int index = 0;
                int found_ident = 0;
                while (splits[index]) {
                    IRIdentifier_t* ident = ir_get_identifier_from_name(splits[index]);
                    if (ident) {
                        found_ident = 1;
                        if (ident->is_stack_variable) {
                            splits[index] = realloc(splits[index], 32);
                            sprintf(splits[index], "[$%.4X + r3]", ((int16_t) ident->stack_offset) & 0xffff);
                        } else {
                            sprintf(splits[index], "[$%.4X]", ((int16_t) ident->absolute_address) & 0xffff);
                        }
                        break;
                    }
                    index ++;
                }
                // merge splits into one
                int offset = 0;
                index = 0;
                while (splits[index]) {
                    char tmp[32];
                    if (splits[index + 1] == NULL || index == 2) {
                        sprintf(tmp, "%s", splits[index]);
                    } else if (index == 0) {
                        sprintf(tmp, "%s ", splits[index]);
                    } else if (index == 1) {
                        sprintf(tmp, "%s, ", splits[index]);
                    }
                    sprintf(&inline_asm[offset], "%s", tmp);
                    offset += strlen(tmp);
                    index++;
                }

                if (!found_ident) {
                    int token_count = 0;
                    Token_t* tokens = assembler_parse_words(splits, index, &token_count);
                    Expression_t* expr = assembler_parse_token(tokens, token_count, NULL);
                    if (!expr || expr->type == EXPR_INSTRUCTION) {
                        log_msg(LP_ERROR, "IR: Symbolic address in inline-assembly either doesn't exist, or is out of scope");
                        log_msg(LP_INFO, "IR: Symbolic operand in question from this inline-assembly line: %s", asm);
                        return NULL;
                    }
                }
                code_output = append_to_output(code_output, &code_output_len, inline_asm);
                code_output = append_to_output(code_output, &code_output_len, "\n");
                parser_token_index ++;
                break;
            }

            default: {
                char tmp[256];
                sprintf(tmp, "; UNKNOWN TOKEN: %s\n", ir_token_name[token->token.type]);
                code_output = append_to_output(code_output, &code_output_len, tmp);
                log_msg(LP_ERROR, "IR: Unknown token: %s - \"%s\"", ir_token_name[token->token.type], token->token.raw);
                parser_token_index++;
                break;
            }
        }
    }

    return code_output;
}

