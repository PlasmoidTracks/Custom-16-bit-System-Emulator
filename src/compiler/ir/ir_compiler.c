
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "utils/Log.h"
#include "utils/String.h"
#include "utils/IO.h"

#include "compiler/asm/assembler.h"

#include "compiler/ir/ir_parser.h"
#include "compiler/ir/ir_token_name_table.h"
#include "compiler/ir/ir_lexer.h"
#include "compiler/ir/ir_compiler.h"

#define IR_COMPILER_DEBUG
#undef IR_COMPILER_DEBUG

IRIdentifier_t ir_identifier[IR_MAX_DEPTH][IR_MAX_IDENT];
int ir_identifier_index[IR_MAX_DEPTH] = {0};   // this gets set to 0 after each "IR_LEX_RETURN", so we can start with a new scope
int ir_identifier_scope_depth = 0;
int static_identifier_count = 0;
int identifier_offset[IR_MAX_DEPTH] = {0};

int is_in_function_body = 0;

#ifdef IR_COMPILER_DEBUG
    static void ir_recursion(IRParserToken_t* parser_token, int depth) {
        //if (depth > 0) return;
        for (int i = 0; i < depth; i++) {
            printf("    ");
        }
        printf("%s, \"%s\"\n", ir_token_name[parser_token->token.type], parser_token->token.raw);
        for (int i = 0; i < parser_token->child_count; i++) {
            ir_recursion(parser_token->child[i], depth + 1);
        }
    }
#endif //IR_COMPILER_DEBUG

int ir_identifier_get_stack_offset(const char* ident_name) {
    //log_msg(LP_DEBUG, "IR Compiler: Looking for ident \"%s\"", ident_name);
    for (int scope = 0; scope <= ir_identifier_scope_depth; scope++) {
        for (int i = 0; i < ir_identifier_index[scope]; i++) {
            if (strcmp(ir_identifier[scope][i].name, ident_name) == 0) {
                return ir_identifier[scope][i].stack_offset;
                break;
            }
        }
    }
    log_msg(LP_ERROR, "IR Compiler: Unkonwn ir_identifier \"%s\" [%s:%d]", ident_name, __FILE__, __LINE__);
    return 0;
}

IRIdentifier_t* ir_get_identifier_from_name(const char* ident_name) {
    //log_msg(LP_DEBUG, "IR Compiler: Looking for ident \"%s\"", ident_name);
    for (int scope = 0; scope <= ir_identifier_scope_depth; scope++) {
        for (int i = 0; i < ir_identifier_index[scope]; i++) {
            if (strcmp(ir_identifier[scope][i].name, ident_name) == 0) {
                return &ir_identifier[scope][i];
                break;
            }
        }
    }
    //log_msg(LP_ERROR, "IR Compiler: Unkonwn ir_identifier \"%s\" [%s:%d]", ident_name, __FILE__, __LINE__);
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
                log_msg(LP_ERROR, "IR Compiler: Unknown type modifier \"%s\" [%s:%d]", ir_token_name[tt], __FILE__, __LINE__);
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
    // Solves the expression and stores the intermediate value in r0
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
                    log_msg(LP_ERROR, "IR Compiler: Unknown ir_identifier \"%s\" [%s:%d]", token.raw, __FILE__, __LINE__);
                    break;
                }
                if (!(ident->type_modifier & IR_TM_STATIC)) {
                    *output = append_to_output(*output, length, "; expression -> loading ir_identifier from stack\n");
                    int stack_offset = ident->stack_offset;
                    char tmp[32];
                    sprintf(tmp, "mov r1, [r3 + %d]", stack_offset);
                    *output = append_to_output(*output, length, tmp);
                    sprintf(tmp, "\t; %s\n", token.raw);
                    *output = append_to_output(*output, length, tmp);
                } else if (ident->type_modifier & IR_TM_STATIC) {
                    *output = append_to_output(*output, length, "; expression -> loading static ir_identifier from data segment\n");
                    char tmp[64];
                    sprintf(tmp, "mov r1, .__static_data\nadd r1, %d\nmov r1, [r1]", ident->static_offset);
                    *output = append_to_output(*output, length, tmp);
                    sprintf(tmp, "\t; %s\n", token.raw);
                    *output = append_to_output(*output, length, tmp);
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
                if (!ident) {
                    log_msg(LP_ERROR, "IR Compiler: Unknown ir_identifier \"%s\" [%s:%d]", token1->child[0]->token.raw, __FILE__, __LINE__);
                    break;
                }
                if (ident->type_modifier & IR_TM_STATIC) {
                    char tmp[64];
                    sprintf(tmp, "mov r1, .__static_data\nadd r1, %d\n", ident->static_offset);
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

            case IR_LEX_CIB:
                parser_evaluate_expression(output, length, token1);
                *output = append_to_output(*output, length, "; unary operation -> cib\n");
                *output = append_to_output(*output, length, "cib r1\n");
                break;

            case IR_LEX_CFI:
                parser_evaluate_expression(output, length, token1);
                *output = append_to_output(*output, length, "; unary operation -> cfi\n");
                *output = append_to_output(*output, length, "cfi r1\n");
                break;

            case IR_LEX_CFB:
                parser_evaluate_expression(output, length, token1);
                *output = append_to_output(*output, length, "; unary operation -> cfb\n");
                *output = append_to_output(*output, length, "cfb r1\n");
                break;

            case IR_LEX_CBI:
                parser_evaluate_expression(output, length, token1);
                *output = append_to_output(*output, length, "; unary operation -> cbi\n");
                *output = append_to_output(*output, length, "cbi r1\n");
                break;

            case IR_LEX_CBF:
                parser_evaluate_expression(output, length, token1);
                *output = append_to_output(*output, length, "; unary operation -> cbf\n");
                *output = append_to_output(*output, length, "cbf r1\n");
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

            case IR_LEX_BFLOAT_PLUS:
                *output = append_to_output(*output, length, "; binary operation -> addbf\n");
                *output = append_to_output(*output, length, "addbf r0, r1\nmov r1, r0\n");
                break;

            case IR_LEX_INTEGER_MINUS:
                *output = append_to_output(*output, length, "; binary operation -> sub\n");
                *output = append_to_output(*output, length, "sub r0, r1\nmov r1, r0\n");
                break;

            case IR_LEX_FLOAT_MINUS:
                *output = append_to_output(*output, length, "; binary operation -> subf\n");
                *output = append_to_output(*output, length, "subf r0, r1\nmov r1, r0\n");
                break;

            case IR_LEX_BFLOAT_MINUS:
                *output = append_to_output(*output, length, "; binary operation -> subbf\n");
                *output = append_to_output(*output, length, "subbf r0, r1\nmov r1, r0\n");
                break;

            case IR_LEX_INTEGER_STAR:
                *output = append_to_output(*output, length, "; binary operation -> mul\n");
                *output = append_to_output(*output, length, "mul r0, r1\nmov r1, r0\n");
                break;

            case IR_LEX_FLOAT_STAR:
                *output = append_to_output(*output, length, "; binary operation -> mulf\n");
                *output = append_to_output(*output, length, "mulf r0, r1\nmov r1, r0\n");
                break;

            case IR_LEX_BFLOAT_STAR:
                *output = append_to_output(*output, length, "; binary operation -> mulbf\n");
                *output = append_to_output(*output, length, "mulbf r0, r1\nmov r1, r0\n");
                break;

            case IR_LEX_INTEGER_SLASH:
                *output = append_to_output(*output, length, "; binary operation -> div\n");
                *output = append_to_output(*output, length, "div r0, r1\nmov r1, r0\n");
                break;

            case IR_LEX_FLOAT_SLASH:
                *output = append_to_output(*output, length, "; binary operation -> divf\n");
                *output = append_to_output(*output, length, "divf r0, r1\nmov r1, r0\n");
                break;

            case IR_LEX_BFLOAT_SLASH:
                *output = append_to_output(*output, length, "; binary operation -> divbf\n");
                *output = append_to_output(*output, length, "divbf r0, r1\nmov r1, r0\n");
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
                *output = append_to_output(*output, length, "cmp r0, r1\nmov r1, 0\ncmovl r1, 1\ncmovz r1, 1\n");
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
                *output = append_to_output(*output, length, "cmp r0, r1\nmov r1, 0\ncmovfl r1, 1\ncmovfz r1, 0\n");
                break;

            case IR_LEX_FLOAT_LESS_EQUAL:
                *output = append_to_output(*output, length, "; binary operation -> float less equal\n");
                *output = append_to_output(*output, length, "cmp r0, r1\nmov r1, 0\ncmovfl r1, 1\ncmovfz r1, 1\n");
                break;
                
            case IR_LEX_FLOAT_GREATER:
                *output = append_to_output(*output, length, "; binary operation -> float greater\n");
                *output = append_to_output(*output, length, "cmp r0, r1\nmov r1, 0\ncmovnfl r1, 1\ncmovfz r1, 0\n");
                break;

            case IR_LEX_FLOAT_GREATER_EQUAL:
                *output = append_to_output(*output, length, "; binary operation -> float greater equal\n");
                *output = append_to_output(*output, length, "cmp r0, r1\nmov r1, 0\ncmovnfl r1, 1\n");
                break;

            case IR_LEX_BFLOAT_LESS:
                *output = append_to_output(*output, length, "; binary operation -> bfloat less\n");
                *output = append_to_output(*output, length, "cmp r0, r1\nmov r1, 0\ncmovbl r1, 1\ncmovfz r1, 0\n");
                break;

            case IR_LEX_BFLOAT_LESS_EQUAL:
                *output = append_to_output(*output, length, "; binary operation -> bfloat less equal\n");
                *output = append_to_output(*output, length, "cmp r0, r1\nmov r1, 0\ncmovbl r1, 1\ncmovfz r1, 1\n");
                break;
                
            case IR_LEX_BFLOAT_GREATER:
                *output = append_to_output(*output, length, "; binary operation -> bfloat greater\n");
                *output = append_to_output(*output, length, "cmp r0, r1\nmov r1, 0\ncmovnbl r1, 1\ncmovfz r1, 0\n");
                break;

            case IR_LEX_BFLOAT_GREATER_EQUAL:
                *output = append_to_output(*output, length, "; binary operation -> bfloat greater equal\n");
                *output = append_to_output(*output, length, "cmp r0, r1\nmov r1, 0\ncmovnbl r1, 1\n");
                break;

            case IR_LEX_UNSIGNED_INTEGER_LESS:
                *output = append_to_output(*output, length, "; binary operation -> unsigned less\n");
                *output = append_to_output(*output, length, "cmp r0, r1\nmov r1, 0\ncmovul r1, 1\ncmovz r1, 0\n");
                break;

            case IR_LEX_UNSIGNED_INTEGER_LESS_EQUAL:
                *output = append_to_output(*output, length, "; binary operation -> unsigned less equal\n");
                *output = append_to_output(*output, length, "cmp r0, r1\nmov r1, 0\ncmovul r1, 1\ncmovz r1, 1\n");
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

char* ir_compile(char* source, long source_length, IRCompileOption_t options) {
    
    long parser_token_count;
    IRParserToken_t** parser_token = ir_parser_parse(source, source_length, &parser_token_count);
    if (!parser_token) {
        log_msg(LP_ERROR, "IR: Parser returned NULL [%s:%d]", __FILE__, __LINE__);
        return NULL;
    }

    //ToDo, do pre-pass, where we store how much stack space 'scopebegin' should allocate through static memory use analysis, so, just counting how many variable declarations are done.
    long parser_token_index = 0;
    int scopebegin_count = 0;
    int scopebegin_offset[256] = {0}; // ToDo, dont hardcode how often 'scopebegin's there can be. 
    while (parser_token_index < parser_token_count) {
        IRParserToken_t* token = parser_token[parser_token_index];
        switch ((int) token->token.type) {

            case IR_LEX_SCOPEBEGIN:
                scopebegin_count ++;
                scopebegin_offset[scopebegin_count] = 0;
                break;
            
            case IR_LEX_IRQBEGIN:
                scopebegin_count ++;
                scopebegin_offset[scopebegin_count] = 0;
                break;
        
            case IR_PAR_VARIABLE_DECLARATION: {
                IRTypeModifier_t type_mod = vardec_get_type_modifier(parser_token[parser_token_index]->child[0]);
                if (type_mod & IR_TM_STATIC) {break;}
                scopebegin_offset[scopebegin_count] += 2;
                break;
            }

            default:
                break;
        }
        parser_token_index++;
    }

    char* data_output = NULL;         // final data string
    long data_output_len = 0;         // current length of data output
    {
        char tmp[32];
        data_output = append_to_output(data_output, &data_output_len, "; data setup\n");
        sprintf(tmp, ".data\n.__static_data\n");
        data_output = append_to_output(data_output, &data_output_len, tmp);
    }

    //ToDo, another pre-pass, where we check for static variables and build the static initializer section
    parser_token_index = 0;
    while (parser_token_index < parser_token_count) {
        IRParserToken_t* token = parser_token[parser_token_index];
        switch ((int) token->token.type) {
        
            case IR_PAR_VARIABLE_DECLARATION: {
                //log_msg(LP_DEBUG, "IR Compiler: Prepass 2 a: Found variable declaration outside body of %s", token->child[1]->token.raw);
                IRTypeModifier_t type_mod = vardec_get_type_modifier(parser_token[parser_token_index]->child[0]);
                if (!(type_mod & IR_TM_STATIC)) {break;}
                // Now check if at any point that variable gets an assignment outside of a function body
                int in_function_body = 0;
                int parser_token_index_2 = 0;
                int found_match = 0;
                while (parser_token_index_2 < parser_token_count) {
                    IRParserToken_t* token_2 = parser_token[parser_token_index_2];
                    switch ((int) token_2->token.type) {

                        case IR_LEX_SCOPEBEGIN:
                        case IR_LEX_IRQBEGIN:
                            in_function_body = 1;
                            break;
                        
                        case IR_LEX_SCOPEEND:
                        case IR_LEX_IRQEND:
                            in_function_body = 0;
                            break;
                        
                        case IR_PAR_VARIABLE_ASSIGNMENT:
                            if (in_function_body) {break;}
                            //log_msg(LP_DEBUG, "IR Compiler: Prepass 2 b: Found variable assignment outside body of %s", token_2->child[0]->token.raw);
                            // check if the variable assignment outside a function body is the same as the found static one... 
                            if (strcmp(token_2->child[0]->token.raw, token->child[1]->token.raw) == 0) {
                                found_match = 1;
                            }
                            if (found_match) {
                                // Here append the value to the data segment
                                //log_msg(LP_DEBUG, "IR Compiler: Prepass 2 b1: Found matching static variable assignment for %s", token->child[1]->token.raw);
                                data_output = append_to_output(data_output, &data_output_len, "; static variable assignment outside function body\n");
                                IRParserToken_t* expr = token_2->child[2];
                                if (expr->child[0]->token.type != IR_LEX_NUMBER) {
                                    log_msg(LP_ERROR, "IR Compiler: static variables outside of function bodies cannot be assigned anything other than numeric values. Instead got %s [%s:%d]", ir_token_name[expr->child[0]->token.type], __FILE__, __LINE__);
                                    #ifdef IR_COMPILER_DEBUG
                                        ir_recursion(expr, 0);
                                    #endif
                                    return NULL;
                                }
                                char tmp[64];
                                sprintf(tmp, "%s\n", expr->child[0]->token.raw);
                                data_output = append_to_output(data_output, &data_output_len, tmp);
                            }
                            break;
                    }
                    if (found_match) {break;}
                    parser_token_index_2++;
                }
                if (!found_match) {
                    // Here append reserve to the data segment
                    //log_msg(LP_DEBUG, "IR Compiler: Prepass 2 b2: Found static variable %s to be uninitialized", token->child[1]->token.raw);
                    data_output = append_to_output(data_output, &data_output_len, "; uninitialized static variable placeholder\n");
                    data_output = append_to_output(data_output, &data_output_len, ".reserve $0002\n");
                }
                break;
            }

            default:
                break;
        }
        parser_token_index++;
    }

    int scopebegin_index = 1;

    char* code_output = NULL;         // final assembly string
    long code_output_len = 0;         // current length of assembly output

    parser_token_index = 0;

    (void) options;

    // default header
    if (options & IRCO_ADD_PREAMBLE) {
        code_output = append_to_output(code_output, &code_output_len, "; code setup\n");
        code_output = append_to_output(code_output, &code_output_len, ".address 0\n.code\ncall .main\nhlt\n");
    }

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
                // Add ir_identifier to current frame list
                if (ir_identifier_index[ir_identifier_scope_depth] >= IR_MAX_IDENT) {
                    log_msg(LP_ERROR, "IR Compiler: Too many identifiers, max is %d [%s:%d]", IR_MAX_IDENT, __FILE__, __LINE__);
                    return NULL;
                }

                const char* ident_name = parser_token[parser_token_index]->child[1]->token.raw;
                IRIdentifier_t* ident = ir_get_identifier_from_name(ident_name);

                IRTypeModifier_t type_mod = vardec_get_type_modifier(parser_token[parser_token_index]->child[0]);
                //log_msg(LP_NOTICE, "type modifier: %.2o", type_mod);
                if (ident && !(type_mod & IR_TM_ANON)) {
                    log_msg(LP_ERROR, "IR Compiler: Redeclaration of already existing variable \"%s\" [%s:%d]", ident_name, __FILE__, __LINE__);
                    return NULL;
                }

                if (type_mod & IR_TM_ANON) {
                    if (type_mod & IR_TM_STATIC) {
                        log_msg(LP_ERROR, "IR Compiler: Static variables cannot be anonymous [%s:%d]", __FILE__, __LINE__);
                        return NULL;
                    }
                    identifier_offset[ir_identifier_scope_depth] += 1;
                } else {
                    if (type_mod & IR_TM_STATIC) {
                        memcpy(ir_identifier[ir_identifier_scope_depth][ir_identifier_index[ir_identifier_scope_depth]].name, ident_name, strlen(ident_name));
                        ir_identifier[ir_identifier_scope_depth][ir_identifier_index[ir_identifier_scope_depth]].name[strlen(ident_name)] = '\0';
                        ir_identifier[ir_identifier_scope_depth][ir_identifier_index[ir_identifier_scope_depth]].type_modifier = type_mod;
                        ir_identifier[ir_identifier_scope_depth][ir_identifier_index[ir_identifier_scope_depth]].static_offset = (2 * static_identifier_count);
                        ir_identifier[ir_identifier_scope_depth][ir_identifier_index[ir_identifier_scope_depth]].initialized = 0;
                        ir_identifier[ir_identifier_scope_depth][ir_identifier_index[ir_identifier_scope_depth]].identifier_index = static_identifier_count;
                        ir_identifier[ir_identifier_scope_depth][ir_identifier_index[ir_identifier_scope_depth]].is_stack_variable = 0;
                        //log_msg(LP_DEBUG, "IR Compiler: Added static ir_identifier \"%s\"", ir_identifier[ir_identifier_scope_depth][ir_identifier_index[ir_identifier_scope_depth]].name);
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
                        //log_msg(LP_DEBUG, "IR Compiler: Added ir_identifier \"%s\"", ir_identifier[ir_identifier_scope_depth][ir_identifier_index[ir_identifier_scope_depth]].name);
                        ir_identifier_index[ir_identifier_scope_depth] ++;
                        identifier_offset[ir_identifier_scope_depth] += 1;
                    }
                }
                
                parser_token_index ++;
                break;
            }
            
            case IR_PAR_VARIABLE_ASSIGNMENT: {
                // TODO: First solve expression, then save it in r1, the find ir_identifier, set r0 to the ident and mov indirectly?
                IRIdentifier_t* ident = ir_get_identifier_from_name(parser_token[parser_token_index]->child[0]->token.raw);
                if (!ident) {
                    log_msg(LP_ERROR, "IR Compiler: Unknown ir_identifier \"%s\" [%s:%d]", parser_token[parser_token_index]->child[0]->token.raw, __FILE__, __LINE__);
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
                    if (is_in_function_body) {
                        code_output = append_to_output(code_output, &code_output_len, "; static variable assignment inside function body\n");
                        IRParserToken_t* expr = parser_token[parser_token_index]->child[2];
                        parser_evaluate_expression(&code_output, &code_output_len, expr);
                        char tmp[64];
                        sprintf(tmp, "mov r0, .__static_data\nadd r0, %d", ident->static_offset);
                        code_output = append_to_output(code_output, &code_output_len, tmp);
                        sprintf(tmp, "\t; %s\n", parser_token[parser_token_index]->child[0]->token.raw);
                        code_output = append_to_output(code_output, &code_output_len, tmp);
                        code_output = append_to_output(code_output, &code_output_len, "mov [r0], r1\n");
                    } // other path was already handled in second prepass
                    parser_token_index ++;
                }
                break;
            }
            
            case IR_PAR_DEREF_VARIABLE_ASSIGNMENT: {
                code_output = append_to_output(code_output, &code_output_len, "; deref variable assignment\n");
                IRIdentifier_t* ident = ir_get_identifier_from_name(parser_token[parser_token_index]->child[1]->token.raw);
                if (!ident) {
                    log_msg(LP_ERROR, "IR Compiler: Unknown ir_identifier \"%s\" [%s:%d]", parser_token[parser_token_index]->child[1]->token.raw, __FILE__, __LINE__);
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
                    if (is_in_function_body) {
                        IRParserToken_t* expr = parser_token[parser_token_index]->child[3];
                        parser_evaluate_expression(&code_output, &code_output_len, expr);
                        char tmp[64];
                        sprintf(tmp, "mov r0, .__static_data\nadd r0, %d\n", ident->static_offset);
                        code_output = append_to_output(code_output, &code_output_len, tmp);
                        sprintf(tmp, "\t; %s\n", parser_token[parser_token_index]->child[1]->token.raw);
                        code_output = append_to_output(code_output, &code_output_len, tmp);
                        code_output = append_to_output(code_output, &code_output_len, "mov r0, [r0]\nmov [r0], r1\n");
                        parser_token_index ++;
                    } else {
                        log_msg(LP_ERROR, "IR Compiler: Deref variable assignment of static variable may not occure outside of function bodies");
                        return NULL;
                    }
                }
                break;
            }
            
            case IR_PAR_VARIABLE_FUNCTION_POINTER_ASSIGNMENT: {
                code_output = append_to_output(code_output, &code_output_len, "; variable function pointer assignment\n");
                IRIdentifier_t* ident = ir_get_identifier_from_name(parser_token[parser_token_index]->child[0]->token.raw);
                if (!ident) {
                    log_msg(LP_ERROR, "IR Compiler: Unknown ir_identifier \"%s\" [%s:%d]", parser_token[parser_token_index]->child[0]->token.raw, __FILE__, __LINE__);
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
                    if (is_in_function_body) {
                        char tmp[64];
                        sprintf(tmp, "mov r0, .__static_data\nadd r0, %d\n", ident->static_offset);
                        code_output = append_to_output(code_output, &code_output_len, tmp);
                        sprintf(tmp, "mov [r0], %s\n", parser_token[parser_token_index]->child[2]->token.raw);
                        code_output = append_to_output(code_output, &code_output_len, tmp);
                        parser_token_index ++;
                    } else {
                        log_msg(LP_ERROR, "IR Compiler: static variables outside of function bodies cannot be assigned anything other than numeric values. Instead got label \"%s\" [%s:%d]", ident->name, __FILE__, __LINE__);
                        return NULL;
                    }
                }
                break;
            }

            case IR_LEX_SCOPEBEGIN: {
                code_output = append_to_output(code_output, &code_output_len, "; scope begin\n");
                // Function requires pushing of the old frame pointer in case it arrived from another function call
                // and then r3 is set to the current stack-pointer to act as the frame-pointer
                code_output = append_to_output(code_output, &code_output_len, "push r3\n");
                code_output = append_to_output(code_output, &code_output_len, "mov r3, sp\n");
                char tmp[64];
                sprintf(tmp, "sub sp, %d\n", scopebegin_offset[scopebegin_index++]);
                code_output = append_to_output(code_output, &code_output_len, tmp);
                parser_token_index++;
                ir_identifier_scope_depth ++;
                is_in_function_body = 1;
                break;
            }

            case IR_LEX_SCOPEEND:
                code_output = append_to_output(code_output, &code_output_len, "; scope end\n");
                parser_token_index++;
                ir_identifier_index[ir_identifier_scope_depth] = 0;
                identifier_offset[ir_identifier_scope_depth] = 0;
                ir_identifier_scope_depth --;
                is_in_function_body = 0;
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
                if (options & IRCO_POSITION_INDEPENDENT_CODE) {
                    sprintf(tmp, "rcall %s\n", token->child[1]->token.raw);
                } else {
                    sprintf(tmp, "call %s\n", token->child[1]->token.raw);
                }
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
                code_output = append_to_output(code_output, &code_output_len, "; expression -> loading label\n");
                char tmp[256];
                if (options & IRCO_POSITION_INDEPENDENT_CODE) {
                    sprintf(tmp, "rjmp %s\n", expr->token.raw);
                } else {
                    sprintf(tmp, "jmp %s\n", expr->token.raw);
                }
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
                sprintf(tmp, "tst r1\nrjnz %s\n", parser_token[parser_token_index]->child[2]->token.raw);
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

            case IR_LEX_IRQBEGIN: {
                code_output = append_to_output(code_output, &code_output_len, "; irq begin\n");
                // Function requires pushing of the old frame pointer in case it arrived from another function call
                // and then r3 is set to the current stack-pointer to act as the frame-pointer
                code_output = append_to_output(code_output, &code_output_len, "semi\n");
                code_output = append_to_output(code_output, &code_output_len, "push r3\n");
                code_output = append_to_output(code_output, &code_output_len, "mov r3, sp\n");
                char tmp[64];
                sprintf(tmp, "sub sp, %d\n", scopebegin_offset[scopebegin_index++]);
                code_output = append_to_output(code_output, &code_output_len, tmp);
                code_output = append_to_output(code_output, &code_output_len, "push r2\n");
                code_output = append_to_output(code_output, &code_output_len, "push r1\n");
                code_output = append_to_output(code_output, &code_output_len, "push r0\n");
                code_output = append_to_output(code_output, &code_output_len, "pushsr\n");
                parser_token_index++;
                ir_identifier_scope_depth ++;
                is_in_function_body = 1;
                break;
            }

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
                is_in_function_body = 0;
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
                            splits[index] = realloc(splits[index], 32);
                            sprintf(splits[index], "[.__static_data + $%.4X]", ((int16_t) ident->stack_offset) & 0xffff);
                            //log_msg(LP_ERROR, "IR Compiler: resolving of static variable from symbolic expression within inline assembly is not possible [%s:%d]", __FILE__, __LINE__);
                            //return NULL;
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
                        // check if valid instruction
                        int is_valid_instruction = 0;
                        for (int i = 0; i < INSTRUCTION_COUNT; i++) {
                            //printf("%s vs %s : %d\n", expression[expression_index].tokens[0].raw, cpu_instruction_string[i], strcmp(expression[expression_index].tokens[0].raw, cpu_instruction_string[i]));
                            if (strcmp(expr->tokens[0].raw, cpu_instruction_string[i]) == 0) {
                                is_valid_instruction = 1;
                                break;
                            }
                        }
                        if (!is_valid_instruction) {
                            log_msg(LP_ERROR, "IR Compiler: Symbolic address in inline-assembly either doesn't exist, or is out of scope [%s:%d]", __FILE__, __LINE__);
                            log_msg(LP_INFO, "IR Compiler: Symbolic operand in question from this inline-assembly line: %s", asm);
                            return NULL;
                        }
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
                log_msg(LP_ERROR, "IR Compiler: Unknown token: %s - \"%s\" [%s:%d]", ir_token_name[token->token.type], token->token.raw, __FILE__, __LINE__);
                #ifdef IR_COMPILER_DEBUG
                    for (int i = parser_token_index; i < ((parser_token_index + 8) >= parser_token_count ? parser_token_count : parser_token_index + 8); i++) {
                        ir_recursion(parser_token[i], 0);
                    }
                #endif //IR_COMPILER_DEBUG
                parser_token_index++;
                break;
            }
        }
    }
    
    code_output = append_to_output(code_output, &code_output_len, data_output);
    //code_output = append_to_output(code_output, &code_output_len, ".__heap_data\n");

    return code_output;
}

char* ir_compile_from_filename(const char* const filename, IRCompileOption_t options) {
    long content_size;
    char* content = read_file(filename, &content_size);
    if (!content) {
        log_msg(LP_ERROR, "IR: read_file failed [%s:%d]", __FILE__, __LINE__);
        return NULL;
    }
    char* asm = ir_compile(content, content_size, options);
    if (!asm) {
        log_msg(LP_ERROR, "IR: Compiler returned NULL [%s:%d]", __FILE__, __LINE__);
        return NULL;
    }
    return asm;
}
