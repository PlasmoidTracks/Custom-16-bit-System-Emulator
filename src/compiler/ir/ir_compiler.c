
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
#include "compiler/ir/ir_semantic_analyzer.h"


typedef enum {
    STACK       = 0001, 
    STATIC      = 0002, 
    VOLATILE    = 0004, 
    REGISTER    = 0010, 
    MMIO        = 0020, 
    TEMP        = 0040, 
    AT          = 0100, 
    PADALIGN    = 0200, 
} VariableModifier_t;

typedef enum {
    PERILOGUE   = 0001, 
    ATOMIC      = 0002, 
    REENTRANT   = 0004, 
    INTERRUPT   = 0010, 
    ADDRESS     = 0020, 
    LOCAL       = 0040, 
    ALIGN       = 0100, 
} FunctionModifier_t;


typedef struct Variable_t {
    IRParserToken_t* token; // hold what? Parent Statement?
    VariableModifier_t modifier;
    int at;
    int padalign;
    char* name;
    int size;
    int scope_index;    // -1 is global
    union {
        int offset;     // for stack variables
        int address;    // for static variables
    };
} Variable_t;

typedef struct Function_t {
    IRParserToken_t* token; // hold what? Parent Statement?
    FunctionModifier_t modifier;
    int interrupt;
    int align;
    char* name;
    // epilogue size (how many bytes in the epilogue ends and function body starts)
    // memory layout...?
} Function_t;


// int* at and int* padalign MUST to be initialized as -1
static VariableModifier_t ir_prepass_variable_declaration_get_type_definition(IRParserToken_t* type_definition, int* at, int* padalign) {
    if ((IRParserTokenType_t) type_definition->token.type != IR_PAR_TYPE_DEFINITION) {
        return 0;
    }
    VariableModifier_t modifier = 0;
    for (int i = 0; i < type_definition->child_count; i++) {
        if ((IRParserTokenType_t) type_definition->child[i]->token.type == IR_PAR_TYPE_DEFINITION) {
            modifier |= ir_prepass_variable_declaration_get_type_definition(type_definition->child[i], at, padalign);
        } else {
            switch (type_definition->child[i]->token.type) {
                case IR_LEX_STACK:
                    modifier |= STACK;
                    break;

                case IR_LEX_STATIC:
                    modifier |= STATIC;
                    break;

                case IR_LEX_VOLATILE:
                    modifier |= VOLATILE;
                    break;

                case IR_LEX_REGISTER:
                    modifier |= REGISTER;
                    break;

                case IR_LEX_MMIO:
                    modifier |= MMIO;
                    break;

                case IR_LEX_TEMP:
                    modifier |= TEMP;
                    break;
                
                case IR_LEX_AT:
                    modifier |= AT;
                    if (*at != -1) {
                        log_msg(LP_WARNING, "Multiple apprearenced of 'at' modifier will use last set value [%s:%d]", __FILE__, __LINE__);
                    }
                    *at = parse_immediate(type_definition->child[2]->token.raw);
                    break;

                default:
                    //log_msg(LP_ERROR, "Unknown variable type modifier \"%s\" [%s:%d]", ir_token_name[type_definition->child[i]->token.type], __FILE__, __LINE__);
                    break;
            }
        }
    }

    return modifier;
}




// int* interrupt and int* align MUST to be initialized as -1
static FunctionModifier_t ir_prepass_function_declaration_get_type_definition(IRParserToken_t* type_definition, int* interrupt, int* align) {
    if ((IRParserTokenType_t) type_definition->token.type != IR_PAR_FUNCTION_MODIFIER) {
        return 0;
    }
    FunctionModifier_t modifier = 0;
    for (int i = 0; i < type_definition->child_count; i++) {
        if ((IRParserTokenType_t) type_definition->child[i]->token.type == IR_PAR_FUNCTION_MODIFIER) {
            modifier |= ir_prepass_function_declaration_get_type_definition(type_definition->child[i], interrupt, align);
        } else {
            switch (type_definition->child[i]->token.type) {
                case IR_LEX_PERILOGUE:
                    modifier |= PERILOGUE;
                    break;

                case IR_LEX_ATOMIC:
                    modifier |= ATOMIC;
                    break;

                case IR_LEX_REENTRANT:
                    modifier |= REENTRANT;
                    break;

                case IR_LEX_INTERRUPT:
                    modifier |= INTERRUPT;
                    if (*interrupt != -1) {
                        log_msg(LP_WARNING, "Multiple apprearenced of 'interrupt' modifier will use last set value [%s:%d]", __FILE__, __LINE__);
                    }
                    *interrupt = parse_immediate(type_definition->child[2]->token.raw);
                    break;

                case IR_LEX_LOCAL:
                    modifier |= LOCAL;
                    break;

                default:
                    //log_msg(LP_ERROR, "Unknown variable type modifier \"%s\" [%s:%d]", ir_token_name[type_definition->child[i]->token.type], __FILE__, __LINE__);
                    break;
            }
        }
    }

    return modifier;
}


static void list_append(void** list, int* list_length, size_t size, void* data) {
    if (!list_length) {
        log_msg(LP_ERROR, "NULL pointer [%s:%d]", __FILE__, __LINE__);
        return;
    }
    *list = realloc(*list, size * ((*list_length) + 1));
    memcpy((char*)(*list) + (size * (*list_length)), data, size);
    (*list_length) ++;
}


static void _ir_prepass_variable(IRParserToken_t* AST, Variable_t** variable_list_ptr, int* variable_list_length_ptr, int inside_function_definition, int* scope_index_ptr) {
    //log_msg(LP_DEBUG, "%s: %d", ir_token_name[AST->token.type], inside_function_definition);
    if ((IRParserTokenType_t) AST->token.type == IR_PAR_FUNCTION_DEFINITION) {
        (*scope_index_ptr) ++;
    }

    if ((IRParserTokenType_t) AST->token.type == IR_PAR_VARIABLE_DECLARATION) {
        int at = -1;
        int padalign = -1;
        VariableModifier_t modifier = ir_prepass_variable_declaration_get_type_definition(AST->child[0], &at, &padalign);
        char* name = AST->child[4]->token.raw;
        int size = 0;
        switch (AST->child[2]->token.type) {
            case IR_LEX_NUMBER: {
                size = parse_immediate(AST->child[2]->token.raw);
                break;
            }

            default:
                log_msg(LP_ERROR, "Unexpected token as variable size \"%s\" [%s:%d]", ir_token_name[AST->child[2]->child[0]->token.type], __FILE__, __LINE__);
                break;
        }

        int scope_index = inside_function_definition ? (*scope_index_ptr) : -1;
        //log_msg(LP_DEBUG, "tm:%.4x, n:%s, s:%d, si:%d, v:%d, at:%.4x", modifier, name, size, scope_index, vla, at);

        Variable_t variable = {
            .token = AST, 
            .at = at, 
            .modifier = modifier, 
            .name = name, 
            .size = size, 
            .scope_index = scope_index, 
        };

        list_append((void*) variable_list_ptr, variable_list_length_ptr, sizeof(Variable_t), &variable);

        return;
    }

    for (int i = 0; i < AST->child_count; i++) {
        _ir_prepass_variable(AST->child[i], variable_list_ptr, variable_list_length_ptr, inside_function_definition || (IRParserTokenType_t) AST->token.type == IR_PAR_FUNCTION_DEFINITION, scope_index_ptr);
    }
}


static void _ir_prepass_function(IRParserToken_t* AST, Function_t** function_list_ptr, int* function_list_length_ptr, FunctionModifier_t modifier, int interrupt, int align) {

    if ((IRParserTokenType_t) AST->token.type == IR_PAR_FUNCTION_DEFINITION) {
        if (AST->variant == 2) {
            FunctionModifier_t fetched_modifier = ir_prepass_function_declaration_get_type_definition(AST->child[0], &interrupt, &align);
            _ir_prepass_function(AST->child[1], function_list_ptr, function_list_length_ptr, fetched_modifier, interrupt, align);
        } else {
            char* name = AST->child[0]->token.raw;

            Function_t function = {
                .token = AST, 
                .modifier = modifier, 
                .name = name, 
                .interrupt = interrupt, 
                .align = align, 
            };

            list_append((void*) function_list_ptr, function_list_length_ptr, sizeof(Function_t), &function);
        }

        return;
    }

    for (int i = 0; i < AST->child_count; i++) {
        _ir_prepass_function(AST->child[i], function_list_ptr, function_list_length_ptr, 0, -1, -1);
    }
}


void ir_prepass_variable(IRParserToken_t* AST, Variable_t** variable_list_ptr, int* variable_list_length_ptr, int* scope_index_ptr) {
    _ir_prepass_variable(AST, variable_list_ptr, variable_list_length_ptr, 0, scope_index_ptr);
}

void ir_prepass_function(IRParserToken_t* AST, Function_t** function_list_ptr, int* function_list_length_ptr) {
    _ir_prepass_function(AST, function_list_ptr, function_list_length_ptr, 0, -1, -1);
}



char* ir_codegen(IRParserToken_t** roots, int parser_root_count) {
    (void) roots;
    (void) parser_root_count;
    return NULL;
}

char* ir_compile(char* source, long source_length, const char* const source_identifier, IRCompileOption_t options) {
    (void) options;

    long parser_root_count;
    IRParserToken_t** parse = ir_parser_parse(source, source_length, &parser_root_count);
    if (!parse) {
        log_msg(LP_ERROR, "parser returned NULL [%s:%d]", __FILE__, __LINE__);
        return NULL;
    }

    // Check whether the AST roots consists of only Statements and Function definitions
    int invalid = 0;
    int last_line_shown = -1;
    for (long i = 0; i < parser_root_count; i++) {
        if ((IRParserTokenType_t) parse[i]->token.type != IR_PAR_STATEMENT && 
            (IRParserTokenType_t) parse[i]->token.type != IR_PAR_FUNCTION_DEFINITION) {
                if (!invalid) {
                    log_msg_inline(LP_ERROR, "\"%s\" - Invalid syntax [%s:%d]", 
                        source_identifier, 
                        __FILE__, 
                        __LINE__
                    );
                }
                show_error_in_syntax_ext(parse[i], parse[i], &last_line_shown, 1, i > 0);
                invalid = 1;
        }
    }
    if (invalid) {
        putchar('\n');
        return NULL;
    }

    // First comes semantic analysis, operations beyond functions is not allowed for instance. 
    // TODO - delete, honestly, just do it while codegen for the untracked cases
    if (!ir_semantic_analysis(parse, parser_root_count)) {
        log_msg(LP_ERROR, "\"%s\" - Semantic analysis returned invalid semantics [%s:%d]", source_identifier, __FILE__, __LINE__);
        return NULL;
    }
    
    // Prepass, collecting all variables and their scope (either global, or inside one of the function scopes (scopebegin - scopeend))
    Variable_t* variable_list = NULL;
    int variable_list_lenght = 0;
    int scope_index = -1;
    for (int i = 0; i < parser_root_count; i++) {
        ir_prepass_variable(parse[i], &variable_list, &variable_list_lenght, &scope_index);
    }
    int error = 0;
    for (int i = 0; i < variable_list_lenght; i++) {
        // Check for name clashing:
        for (int j = i + 1; j < variable_list_lenght; j++) {
            if ((variable_list[i].scope_index != variable_list[j].scope_index) && variable_list[i].scope_index != -1 && variable_list[j].scope_index != -1) continue;
            if (strcmp(variable_list[i].name, variable_list[j].name) == 0) {
                log_msg(LP_ERROR, "\"%s\" - Clashing variable name \"%s\" [%s:%d]", 
                    source_identifier, variable_list[i].name, __FILE__, __LINE__
                );
                for (int k = 0; k < parser_root_count; k++) {
                    if (show_error_in_syntax(variable_list[i].token, parse[k])) {
                        printf("| ... |\n");
                    }
                    show_error_in_syntax(variable_list[j].token, parse[k]);
                }
                error = 1;
            }
        }

        // check for valid configuration
        if (variable_list[i].size <= 0) {
            log_msg(LP_ERROR, "\"%s\" - Invalid variable size: size may not be zero or negative, got %d [%s:%d]", source_identifier, variable_list[i].size, __FILE__, __LINE__);
            for (int k = 0; k < parser_root_count; k++) {
                show_error_in_syntax(variable_list[i].token, parse[k]);
            }
            error = 1;
        }
        if (variable_list[i].size > 0x0400) {
            log_msg(LP_ERROR, "\"%s\" - Extremely large variable \"%s\" with size %d [%s:%d]", source_identifier, variable_list[i].name, variable_list[i].size, __FILE__, __LINE__);
            for (int k = 0; k < parser_root_count; k++) {
                show_error_in_syntax(variable_list[i].token, parse[k]);
            }
            error = 1;
        }
        int modifier_value = (variable_list[i].modifier & (STACK | STATIC | REGISTER | TEMP));
        if ((modifier_value & (modifier_value - 1))) {
            log_msg(LP_ERROR, "\"%s\" - Variable \"%s\" may only use at most one (1) of the following modifiers at once: <stack | static | register | temp> [%s:%d]", source_identifier, variable_list[i].name, __FILE__, __LINE__);
            for (int k = 0; k < parser_root_count; k++) {
                show_error_in_syntax(variable_list[i].token, parse[k]);
            }
            error = 1;
        }
        if (!modifier_value) {
            log_msg(LP_ERROR, "\"%s\" - Variable \"%s\" must declare storage type by using one (1) of the following modifiers: <stack | static | register | temp> [%s:%d]", source_identifier, variable_list[i].name, __FILE__, __LINE__);
            for (int k = 0; k < parser_root_count; k++) {
                show_error_in_syntax(variable_list[i].token, parse[k]);
            }
            error = 1;
        }
        if ((variable_list[i].modifier & (REGISTER | TEMP)) && variable_list[i].size != 2) {
            log_msg(LP_ERROR, "\"%s\" - Register and temp Variables may only be of size 2, got %d [%s:%d]", source_identifier, variable_list[i].size, __FILE__, __LINE__);
            for (int k = 0; k < parser_root_count; k++) {
                show_error_in_syntax(variable_list[i].token, parse[k]);
            }
            error = 1;
        }
        if ((variable_list[i].modifier & AT) && !(variable_list[i].modifier & STATIC)) {
            log_msg(LP_ERROR, "\"%s\" - Only static variables may use the 'at' modifier [%s:%d]", source_identifier, __FILE__, __LINE__);
            for (int k = 0; k < parser_root_count; k++) {
                show_error_in_syntax(variable_list[i].token, parse[k]);
            }
            error = 1;
        }
        if ((variable_list[i].modifier & AT) && (variable_list[i].modifier & PADALIGN)) {
            log_msg(LP_ERROR, "\"%s\" - 'padalign' may not be used together with 'at' modifier [%s:%d]", source_identifier, __FILE__, __LINE__);
            for (int k = 0; k < parser_root_count; k++) {
                show_error_in_syntax(variable_list[i].token, parse[k]);
            }
            error = 1;
        }
        if ((variable_list[i].modifier & STACK) && variable_list[i].scope_index == -1) {
            log_msg(LP_ERROR, "\"%s\" - 'stack' variable may not be used at global scope [%s:%d]", source_identifier, __FILE__, __LINE__);
            for (int k = 0; k < parser_root_count; k++) {
                show_error_in_syntax(variable_list[i].token, parse[k]);
            }
            error = 1;
        }
    }


    // Prepass, collecting all functions and their modifiers
    Function_t* function_list = NULL;
    int function_list_lenght = 0;
    for (int i = 0; i < parser_root_count; i++) {
        ir_prepass_function(parse[i], &function_list, &function_list_lenght);
    }

    for (int i = 0; i < function_list_lenght; i++) {
        // Check for name clashing:
        for (int j = i + 1; j < function_list_lenght; j++) {
            if (strcmp(function_list[i].name, function_list[j].name) == 0) {
                log_msg(LP_ERROR, "\"%s\" - Clashing function name \"%s\" [%s:%d]", 
                    source_identifier, function_list[i].name, __FILE__, __LINE__
                );
                for (int k = 0; k < parser_root_count; k++) {
                    if (show_error_in_syntax(function_list[i].token, parse[k])) {
                        printf("| ... |\n");
                    }
                    show_error_in_syntax(function_list[j].token, parse[k]);
                }
                error = 1;
            }

            if (function_list[i].interrupt != -1 && function_list[i].interrupt == function_list[j].interrupt) {
                log_msg(LP_ERROR, "\"%s\" - Two functions sharing the same interrupt vector table enty at 0x%.4x [%s:%d]", 
                    source_identifier, function_list[i].interrupt, __FILE__, __LINE__
                );
                for (int k = 0; k < parser_root_count; k++) {
                    if (show_error_in_syntax(function_list[i].token, parse[k])) {
                        printf("| ... |\n");
                    }
                    show_error_in_syntax(function_list[j].token, parse[k]);
                }
                error = 1;
            }
        }

        if (function_list[i].modifier & INTERRUPT) {
            if (!(function_list[i].modifier & ATOMIC)) {
                log_msg(LP_WARNING, "\"%s\" - Interrupt function should also be atomic [%s:%d]", source_identifier, __FILE__, __LINE__);
                for (int k = 0; k < parser_root_count; k++) {
                    show_error_in_syntax(function_list[i].token, parse[k]);
                }
            }
        }
    }


    if (error) return NULL;


    // So all offsets are stored, VLAs are tracked, type modifiers set, statics delegated and indexed, etc. required files, 
    
    char* assembly = malloc(128);
    strcpy(assembly, "\nhlt\n");
    return assembly;
    
    // codegen
    char* binary = ir_codegen(parse, parser_root_count);
    if (!binary) {
        log_msg(LP_ERROR, "\"%s\" - binary is NULL [%s:%d]", source_identifier, __FILE__, __LINE__);
        return NULL;
    }
    
    return NULL;
}

char* ir_compile_from_filename(const char* const filename, IRCompileOption_t options) {
    long content_size;
    char* content = read_file(filename, &content_size);
    if (!content) {
        log_msg(LP_ERROR, "IR: read_file failed [%s:%d]", __FILE__, __LINE__);
        return NULL;
    }
    char* asm = ir_compile(content, content_size, filename, options);
    if (!asm) {
        log_msg(LP_ERROR, "IR: Compiler returned NULL [%s:%d]", __FILE__, __LINE__);
        return NULL;
    }
    return asm;
}
