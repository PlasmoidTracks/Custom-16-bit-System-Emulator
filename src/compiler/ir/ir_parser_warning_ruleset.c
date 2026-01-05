#include "compiler/ir/ir_lexer.h"
#include "compiler/ir/ir_parser.h"
#include "compiler/ir/ir_parser_ruleset.h"
#include "compiler/ir/ir_parser_warning_ruleset.h"

#define IR_PAR_RULE_END ((IRParserTokenType_t)(-1))

/*
This file would contain expressions that are invalid, but hard to debug, like
str = "string";
is invalid, it needs to be
str = ref "string";

and other stuff, like missing semicolons

*/


IRGrammarRule_t ir_parser_warning_ruleset[256] = {
    // TODO
    // "var x;" missing the semicolon
    {
        .context = { 
            (IRParserTokenType_t) IR_PAR_TYPE_DEFINITION, 
            (IRParserTokenType_t) IR_LEX_IDENTIFIER, 
            (IRParserTokenType_t) IR_LEX_SEMICOLON, 
            IR_PAR_RULE_END 
        },
        .invert_match = {0, 0, 1},
        .context_length = 3,
        .description = "missing semicolon in variable declaration",
    }, 

    // "x = 1;" missing the semicolon
    {
        .context = { 
            (IRParserTokenType_t) IR_LEX_IDENTIFIER, 
            (IRParserTokenType_t) IR_LEX_ASSIGN, 
            (IRParserTokenType_t) IR_PAR_EXPRESSION, 
            (IRParserTokenType_t) IR_LEX_SEMICOLON, 
            IR_PAR_RULE_END 
        },
        .invert_match = {0, 0, 0, 1},
        .context_length = 4,
        .description = "missing semicolon in variable assignment",
    }, 

    // "deref x = 1;" missing the semicolon
    {
        .context = { 
            (IRParserTokenType_t) IR_LEX_DEREF,
            (IRParserTokenType_t) IR_LEX_IDENTIFIER,
            (IRParserTokenType_t) IR_LEX_ASSIGN,
            (IRParserTokenType_t) IR_PAR_EXPRESSION,
            (IRParserTokenType_t) IR_LEX_SEMICOLON,
            IR_PAR_RULE_END 
        },
        .invert_match = {0, 0, 0, 0, 1},
        .context_length = 5,
        .description = "missing semicolon in deref variable assignment",
    }, 

    // "x = .label;" missing the semicolon
    {
        .context = { 
            (IRParserTokenType_t) IR_LEX_IDENTIFIER,
            (IRParserTokenType_t) IR_LEX_ASSIGN,
            (IRParserTokenType_t) IR_LEX_LABEL,
            (IRParserTokenType_t) IR_LEX_SEMICOLON,
            IR_PAR_RULE_END 
        },
        .invert_match = {0, 0, 0, 1},
        .context_length = 4,
        .description = "missing semicolon in variable function pointer assignment",
    }, 

    // "goto .label;" missing the semicolon
    {
        .context = { 
            (IRParserTokenType_t) IR_LEX_GOTO,
            (IRParserTokenType_t) IR_LEX_LABEL,
            (IRParserTokenType_t) IR_LEX_SEMICOLON,
            IR_PAR_RULE_END 
        },
        .invert_match = {0, 0, 1},
        .context_length = 3,
        .description = "missing semicolon in goto expression",
    }, 

    // "goto .label;" missing the semicolon
    {
        .context = { 
            (IRParserTokenType_t) IR_LEX_GOTO,
            (IRParserTokenType_t) IR_PAR_EXPRESSION,
            (IRParserTokenType_t) IR_LEX_SEMICOLON,
            IR_PAR_RULE_END 
        },
        .invert_match = {0, 0, 1},
        .context_length = 3,
        .description = "missing semicolon in goto expression",
    }, 

    // "if expr .label;" missing the semicolon
    {
        .context = { 
            (IRParserTokenType_t) IR_LEX_IF,
            (IRParserTokenType_t) IR_PAR_EXPRESSION,
            (IRParserTokenType_t) IR_LEX_LABEL,
            (IRParserTokenType_t) IR_LEX_SEMICOLON,
            IR_PAR_RULE_END 
        },
        .invert_match = {0, 0, 0, 1},
        .context_length = 4,
        .description = "missing semicolon in if-conditional jump",
    }, 

    // "callpusharg;" missing the semicolon
    {
        .context = { 
            (IRParserTokenType_t) IR_LEX_CALLPUSHARG,
            (IRParserTokenType_t) IR_PAR_EXPRESSION,
            (IRParserTokenType_t) IR_LEX_SEMICOLON,
            IR_PAR_RULE_END 
        },
        .invert_match = {0, 0, 1},
        .context_length = 3,
        .description = "missing semicolon in callpusharg",
    }, 

    // "callfreearg 4;" missing the semicolon
    {
        .context = { 
            (IRParserTokenType_t) IR_LEX_CALLFREEARG,
            (IRParserTokenType_t) IR_LEX_NUMBER,
            (IRParserTokenType_t) IR_LEX_SEMICOLON,
            IR_PAR_RULE_END 
        },
        .invert_match = {0, 0, 1},
        .context_length = 3,
        .description = "missing semicolon in callfreearg",
    }, 

    // "call .label;" missing the semicolon
    {
        .context = { 
            (IRParserTokenType_t) IR_LEX_CALL,
            (IRParserTokenType_t) IR_LEX_LABEL,
            (IRParserTokenType_t) IR_LEX_SEMICOLON,
            IR_PAR_RULE_END 
        },
        .invert_match = {0, 0, 1},
        .context_length = 3,
        .description = "missing semicolon in function call",
    }, 

    // "callpusharg expr;" missing the semicolon
    {
        .context = { 
            (IRParserTokenType_t) IR_LEX_CALLPUSHARG,
            (IRParserTokenType_t) IR_PAR_EXPRESSION,
            (IRParserTokenType_t) IR_LEX_SEMICOLON,
            IR_PAR_RULE_END 
        },
        .invert_match = {0, 0, 1},
        .context_length = 3,
        .description = "missing semicolon in callpusharg",
    }, 

    // "call .label;" missing the semicolon
    {
        .context = { 
            (IRParserTokenType_t) IR_LEX_CALL,
            (IRParserTokenType_t) IR_PAR_EXPRESSION,
            (IRParserTokenType_t) IR_LEX_SEMICOLON,
            IR_PAR_RULE_END 
        },
        .invert_match = {0, 0, 1},
        .context_length = 3,
        .description = "missing semicolon in function call",
    }, 

    // End sentinel
    {
        .context = { IR_PAR_RULE_END },
        .output = IR_PAR_UNUSED,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 0,
        .priority = -1,
        .description = "end of rule list",
    }
};



