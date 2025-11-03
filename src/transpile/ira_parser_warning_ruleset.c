#include "transpile/ira_lexer.h"
#include "transpile/ira_parser.h"
#include "transpile/ira_parser_ruleset.h"
#include "transpile/ira_parser_warning_ruleset.h"

#define IRA_PAR_RULE_END ((IRAParserTokenType_t)(-1))

/*
This file would contain expressions that are invalid, but hard to debug, like
str = "string";
is invalid, it needs to be
str = ref "string";

and other stuff, like missing semicolons

*/


IRAGrammarRule_t ira_parser_warning_ruleset[256] = {
    // TODO
    // "var x;" missing the semicolon
    {
        .context = { 
            (IRAParserTokenType_t) IRA_PAR_TYPE, 
            (IRAParserTokenType_t) IRA_LEX_IDENTIFIER, 
            (IRAParserTokenType_t) IRA_LEX_SEMICOLON, 
            IRA_PAR_RULE_END 
        },
        .invert_match = {0, 0, 1},
        .context_length = 3,
        .description = "missing semicolon in variable declaration",
    }, 

    // "x = 1;" missing the semicolon
    {
        .context = { 
            (IRAParserTokenType_t) IRA_LEX_IDENTIFIER, 
            (IRAParserTokenType_t) IRA_LEX_ASSIGN, 
            (IRAParserTokenType_t) IRA_PAR_EXPRESSION, 
            (IRAParserTokenType_t) IRA_LEX_SEMICOLON, 
            IRA_PAR_RULE_END 
        },
        .invert_match = {0, 0, 0, 1},
        .context_length = 4,
        .description = "missing semicolon in variable assignment",
    }, 

    // "deref x = 1;" missing the semicolon
    {
        .context = { 
            (IRAParserTokenType_t) IRA_LEX_DEREF,
            (IRAParserTokenType_t) IRA_LEX_IDENTIFIER,
            (IRAParserTokenType_t) IRA_LEX_ASSIGN,
            (IRAParserTokenType_t) IRA_PAR_EXPRESSION,
            (IRAParserTokenType_t) IRA_LEX_SEMICOLON,
            IRA_PAR_RULE_END 
        },
        .invert_match = {0, 0, 0, 0, 1},
        .context_length = 5,
        .description = "missing semicolon in deref variable assignment",
    }, 

    // "x = .label;" missing the semicolon
    {
        .context = { 
            (IRAParserTokenType_t) IRA_LEX_IDENTIFIER,
            (IRAParserTokenType_t) IRA_LEX_ASSIGN,
            (IRAParserTokenType_t) IRA_LEX_LABEL,
            (IRAParserTokenType_t) IRA_LEX_SEMICOLON,
            IRA_PAR_RULE_END 
        },
        .invert_match = {0, 0, 0, 1},
        .context_length = 4,
        .description = "missing semicolon in variable function pointer assignment",
    }, 

    // "goto .label;" missing the semicolon
    {
        .context = { 
            (IRAParserTokenType_t) IRA_LEX_GOTO,
            (IRAParserTokenType_t) IRA_LEX_LABEL,
            (IRAParserTokenType_t) IRA_LEX_SEMICOLON,
            IRA_PAR_RULE_END 
        },
        .invert_match = {0, 0, 1},
        .context_length = 3,
        .description = "missing semicolon in goto expression",
    }, 

    // "goto .label;" missing the semicolon
    {
        .context = { 
            (IRAParserTokenType_t) IRA_LEX_GOTO,
            (IRAParserTokenType_t) IRA_PAR_EXPRESSION,
            (IRAParserTokenType_t) IRA_LEX_SEMICOLON,
            IRA_PAR_RULE_END 
        },
        .invert_match = {0, 0, 1},
        .context_length = 3,
        .description = "missing semicolon in goto expression",
    }, 

    // "if expr .label;" missing the semicolon
    {
        .context = { 
            (IRAParserTokenType_t) IRA_LEX_IF,
            (IRAParserTokenType_t) IRA_PAR_EXPRESSION,
            (IRAParserTokenType_t) IRA_LEX_LABEL,
            (IRAParserTokenType_t) IRA_LEX_SEMICOLON,
            IRA_PAR_RULE_END 
        },
        .invert_match = {0, 0, 0, 1},
        .context_length = 4,
        .description = "missing semicolon in if-conditional jump",
    }, 

    // "callpusharg;" missing the semicolon
    {
        .context = { 
            (IRAParserTokenType_t) IRA_LEX_CALLPUSHARG,
            (IRAParserTokenType_t) IRA_PAR_EXPRESSION,
            (IRAParserTokenType_t) IRA_LEX_SEMICOLON,
            IRA_PAR_RULE_END 
        },
        .invert_match = {0, 0, 1},
        .context_length = 3,
        .description = "missing semicolon in callpusharg",
    }, 

    // "callfreearg 4;" missing the semicolon
    {
        .context = { 
            (IRAParserTokenType_t) IRA_LEX_CALLFREEARG,
            (IRAParserTokenType_t) IRA_LEX_NUMBER,
            (IRAParserTokenType_t) IRA_LEX_SEMICOLON,
            IRA_PAR_RULE_END 
        },
        .invert_match = {0, 0, 1},
        .context_length = 3,
        .description = "missing semicolon in callfreearg",
    }, 

    // "call .label;" missing the semicolon
    {
        .context = { 
            (IRAParserTokenType_t) IRA_LEX_CALL,
            (IRAParserTokenType_t) IRA_LEX_LABEL,
            (IRAParserTokenType_t) IRA_LEX_SEMICOLON,
            IRA_PAR_RULE_END 
        },
        .invert_match = {0, 0, 1},
        .context_length = 3,
        .description = "missing semicolon in function call",
    }, 

    // "call .label;" missing the semicolon
    {
        .context = { 
            (IRAParserTokenType_t) IRA_LEX_CALL,
            (IRAParserTokenType_t) IRA_PAR_EXPRESSION,
            (IRAParserTokenType_t) IRA_LEX_SEMICOLON,
            IRA_PAR_RULE_END 
        },
        .invert_match = {0, 0, 1},
        .context_length = 3,
        .description = "missing semicolon in function call",
    }, 

    // End sentinel
    {
        .context = { IRA_PAR_RULE_END },
        .output = IRA_PAR_UNUSED,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 0,
        .priority = -1,
        .description = "end of rule list",
    }
};



