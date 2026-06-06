#include <stdio.h>
#include <string.h>

#include "compiler/ir/ir_lexer.h"
#include "utils/Log.h"

#include "compiler/ir/ir_token_name_table.h"

#include "compiler/ir/ir_semantic_analyzer.h"
#include "compiler/ir/ir_parser.h"

/*
OK, NEW IDEA: 
Lets not do a full semantic analysis step seperately. 
So instead, we can refactor this file to contain helper functions that make analysis during compile-time easier!
*/

static void _get_lines(IRParserToken_t* root, unsigned int* first_line, unsigned int* last_line) {
    if (root->child_count == 0) {
        if ((unsigned) root->token.line < *first_line) {
            (*first_line) = root->token.line;
        }
        if ((unsigned) root->token.line > *last_line) {
            (*last_line) = root->token.line;
        }
    }
    for (int i = 0; i < root->child_count; i++) {
        _get_lines(root->child[i], first_line, last_line);
    }
}

static void _get_all_tokens_of_lines(IRParserToken_t* root, IRParserToken_t** list, int capacity, int *index, unsigned first_line, unsigned int last_line) {
    if ((*index) >= capacity) {
        return;
    }
    if (root->child_count == 0) {
        if ((unsigned) root->token.line >= first_line && (unsigned) root->token.line <= last_line) {
            list[(*index)++] = root;
        }
    }
    for (int i = 0; i < root->child_count; i++) {
        _get_all_tokens_of_lines(root->child[i], list, capacity, index, first_line, last_line);
    }
}

int show_error_in_syntax(IRParserToken_t* root, IRParserToken_t* AST) {
    return show_error_in_syntax_ext(root, AST, NULL, '\n', 1, 0);
}

int show_error_in_syntax_ext(IRParserToken_t* root, IRParserToken_t* AST, int* last_line_shown, char end, int pad, int newline_on_first_line_change) {
    // first, get the line interval of the error
    unsigned int first_line = -1, last_line = 0;
    _get_lines(root, &first_line, &last_line);
    //printf("first: %u, last: %u\n", first_line, last_line);

    // then get the leaf nodes of all nodes that are at those lines
    const int CAPACTIY = 16;
    IRParserToken_t* list[CAPACTIY];
    int index = 0;
    _get_all_tokens_of_lines(AST, list, CAPACTIY, &index, first_line, last_line);
    if (index == 0) return 0;

    int current_line = 0;
    if (last_line_shown) {current_line = *last_line_shown;}

    int current_column = 0; 
    int first = 0;
    char placeholder = ' ';
    for (int i = 0; i < index; i++) {
        if (current_line != list[i]->token.line) {
            current_line = list[i]->token.line;
            current_column = 0;
            if (!first) {
                first = 1;
                if (newline_on_first_line_change) {
                    putchar('\n');
                }
            } else {
                putchar('\n');
            }
            printf("| %4d| ", current_line);
            while (++current_column < list[i]->token.column) {
                putchar(placeholder);
            }
        } else {
            while (++current_column < list[i]->token.column && pad) {
                putchar(placeholder);
            }
        }
        printf("%s", list[i]->token.raw);
        current_column += strlen(list[i]->token.raw)-1;
    }
    if (index >= CAPACTIY) {
        printf(" ...\n| %4d| ...", current_line + 1);
    }

    putchar(end);

    if (last_line_shown) {*last_line_shown = current_line;}

    return 1;
}




static void ir_semantic_analysis_check_valid_reference_expression(IRParserToken_t* token, IRParserToken_t* AST, int* valid) {
    // Checks whether the token is a value reference. For example "&1" is not allowed. 
    // token is of type IR_PAR_EXPRESSION variant 2 
    // children are [IR_LEX_AMPERSAND, IR_LEX_EXPRESSION]

    if ((IRParserTokenType_t) token->token.type != IR_PAR_EXPRESSION || token->variant != 2) {
        log_msg(LP_CRITICAL, "WRONG TOKEN TYPE (%s:%d) [%s:%d]", ir_token_name[token->token.type], token->variant, __FILE__, __LINE__);
        *valid = 0;
        return;
    }

    //log_msg(LP_DEBUG, "token->child[1]->variant: %d", token->child[1]->variant);
    
    if ((IRParserTokenType_t) token->child[1]->token.type == IR_PAR_EXPRESSION && token->child[1]->variant == 6) {
        log_msg(LP_ERROR, "malformed reference: immediate values may not be referenced");
        show_error_in_syntax(token, AST);
        *valid = 0;
    }
    
    if ((IRParserTokenType_t) token->child[1]->token.type == IR_PAR_EXPRESSION && token->child[1]->variant == 2) {
        log_msg(LP_ERROR, "malformed reference: reference values may not be referenced");
        show_error_in_syntax(token, AST);
        *valid = 0;
    }
    
    if ((IRParserTokenType_t) token->child[1]->token.type == IR_PAR_EXPRESSION && token->child[1]->variant == 10) {
        log_msg(LP_ERROR, "malformed reference: cannot reference __parg");
        show_error_in_syntax(token, AST);
        *valid = 0;
    }

    return; 
}

static void ir_semantic_analysis_check_valid_lvalue_expression(IRParserToken_t* token, IRParserToken_t* AST, int* valid) {
    // Checks whether the token is a value lvalue. For example "&a = 2;" is not allowed. 
    // token is of type IR_PAR_EXPRESSION any variant

    if ((IRParserTokenType_t) token->token.type != IR_PAR_EXPRESSION) {
        log_msg(LP_CRITICAL, "WRONG TOKEN TYPE (%s:%d) [%s:%d]", ir_token_name[token->token.type], token->variant, __FILE__, __LINE__);
        return;
    }

    //log_msg(LP_DEBUG, "IR_PAR_EXPRESSION variant: %d", token->variant);
    switch (token->variant) {
        case 2: {
            log_msg(LP_ERROR, "malformed lvalue: lvalue may not be a reference");
            show_error_in_syntax(token, AST);
            *valid = 0;
            ir_semantic_analysis_check_valid_reference_expression(token, AST, valid);
            break;
        }
        
        case 6: {
            log_msg(LP_ERROR, "malformed lvalue: lvalue may not be an immediate value");
            show_error_in_syntax(token, AST);
            *valid = 0;
            break;
        }
        
        case 10: {
            log_msg(LP_ERROR, "malformed lvalue: __parg is not mutable");
            show_error_in_syntax(token, AST);
            *valid = 0;
            break;
        }

        default:
            break;
    }
    return;
}


// root is the current token, AST is the FULL branch
static void ir_semantic_analysis_recursion(IRParserToken_t* root, IRParserToken_t* AST, int* valid) {
    switch ((IRParserTokenType_t) root->token.type) {
        case IR_PAR_STATEMENT: {
            //log_msg(LP_DEBUG, "IR_PAR_STATEMENT - variant %d", root->variant);
            switch (root->variant) {
                case 1:
                    ir_semantic_analysis_check_valid_lvalue_expression(root->child[0], AST, valid);
                    break;

                default:
                    break;
            }

            break;
        }

        case IR_PAR_EXPRESSION: {
            switch (root->variant) {
                case 2:
                    ir_semantic_analysis_check_valid_reference_expression(root, AST, valid);
                    break;

                default:
                    break;
            }
            break;
        }


        default: {
            break;
        }
    }
    for (int i = 0; i < root->child_count; i++) {
        ir_semantic_analysis_recursion(root->child[i], AST, valid);
    }
}


int ir_semantic_analysis(IRParserToken_t** AST, int root_count) {
    int valid = 1;
    for (int i = 0; i < root_count; i++) {
        ir_semantic_analysis_recursion(AST[i], AST[i], &valid);
    }
    return valid;
}
