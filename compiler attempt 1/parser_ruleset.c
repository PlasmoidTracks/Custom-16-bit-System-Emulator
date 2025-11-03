#include "compiler/lexer.h"
#include "compiler/parser.h"
#include "compiler/parser_ruleset.h"

#define PAR_RULE_END ((ParserTokenType_t)(-1))

GrammarRule_t parser_ruleset[256] = {
    // Literals and identifiers
    {
        .context = { (ParserTokenType_t) LEX_NUMBER, PAR_RULE_END },
        .output = PAR_EXPRESSION,
        .context_rule = {CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 2,
        .description = "number → expression",
    },
    {
        .context = { (ParserTokenType_t) LEX_IDENTIFIER, PAR_RULE_END },
        .output = PAR_EXPRESSION,
        .context_rule = {CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 0,
        .description = "identifier → expression",
    },

    // Grouping
    {
        .context = {
            (ParserTokenType_t) LEX_LEFT_PAREN,
            PAR_EXPRESSION,
            (ParserTokenType_t) LEX_RIGHT_PAREN,
            PAR_RULE_END
        },
        .output = PAR_EXPRESSION,
        .context_rule = {CR_DISCARD, CR_REPLACE, CR_DISCARD},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 2,
        .description = "( expression ) → expression",
    },

    // Unary expressions
    {
        .context = { PAR_UNARY_OPERATOR, PAR_EXPRESSION, PAR_RULE_END },
        .output = PAR_EXPRESSION,
        .context_rule = {CR_REPLACE, CR_DISCARD},
        .invert_match = {0, 0},
        .context_length = 2,
        .priority = 3,
        .description = "unary_operator expression → expression",
    },
    {
        .context = { (ParserTokenType_t) LEX_BANG, PAR_RULE_END },
        .output = PAR_UNARY_OPERATOR,
        .context_rule = {CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 3,
        .description = "'!' → unary_operator",
    },

    // Binary expressions
    {
        .context = { PAR_EXPRESSION, PAR_BINARY_OPERATOR, PAR_EXPRESSION, PAR_RULE_END },
        .output = PAR_EXPRESSION,
        .context_rule = {CR_REPLACE, CR_DISCARD, CR_DISCARD},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 4,  // placeholder, gets refined per operator
        .description = "expression binary_operator expression → expression",
    },

    // Binary operators (priorities mapped by actual operator)
    {
        .context = { (ParserTokenType_t) LEX_LOGICAL_OR, PAR_RULE_END },
        .output = PAR_BINARY_OPERATOR,
        .context_rule = {CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 8,
        .description = "'||' → binary_operator",
    },
    {
        .context = { (ParserTokenType_t) LEX_LOGICAL_AND, PAR_RULE_END },
        .output = PAR_BINARY_OPERATOR,
        .context_rule = {CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 8,
        .description = "'&&' → binary_operator",
    },
    {
        .context = { (ParserTokenType_t) LEX_EQUAL_EQUAL, PAR_RULE_END },
        .output = PAR_BINARY_OPERATOR,
        .context_rule = {CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 7,
        .description = "'==' → binary_operator",
    },
    {
        .context = { (ParserTokenType_t) LEX_BANG_EQUAL, PAR_RULE_END },
        .output = PAR_BINARY_OPERATOR,
        .context_rule = {CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 7,
        .description = "'!=' → binary_operator",
    },
    {
        .context = { (ParserTokenType_t) LEX_LESS, PAR_RULE_END },
        .output = PAR_BINARY_OPERATOR,
        .context_rule = {CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 6,
        .description = "'<' → binary_operator",
    },
    {
        .context = { (ParserTokenType_t) LEX_GREATER, PAR_RULE_END },
        .output = PAR_BINARY_OPERATOR,
        .context_rule = {CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 6,
        .description = "'>' → binary_operator",
    },
    {
        .context = { (ParserTokenType_t) LEX_LESS_EQUAL, PAR_RULE_END },
        .output = PAR_BINARY_OPERATOR,
        .context_rule = {CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 6,
        .description = "'<=' → binary_operator",
    },
    {
        .context = { (ParserTokenType_t) LEX_GREATER_EQUAL, PAR_RULE_END },
        .output = PAR_BINARY_OPERATOR,
        .context_rule = {CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 6,
        .description = "'>=' → binary_operator",
    },
    {
        .context = { (ParserTokenType_t) LEX_PLUS, PAR_RULE_END },
        .output = PAR_BINARY_OPERATOR,
        .context_rule = {CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1,
        .description = "'+' → binary_operator",
    },
    {
        .context = { PAR_EXPRESSION, (ParserTokenType_t) LEX_MINUS, PAR_EXPRESSION, PAR_RULE_END },
        .output = PAR_BINARY_OPERATOR,
        .context_rule = {CR_KEEP, CR_REPLACE, CR_KEEP},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 4,
        .description = "'-' → binary_operator",
    },
    {
        .context = { PAR_EXPRESSION, (ParserTokenType_t) LEX_MINUS, PAR_EXPRESSION, PAR_RULE_END },
        .output = PAR_UNARY_OPERATOR,
        .context_rule = {CR_KEEP, CR_REPLACE, CR_KEEP},
        .invert_match = {1, 0, 0},
        .context_length = 3,
        .priority = 3,
        .description = "'-' → unary_operator [full context]",
    },
    {
        .context = { (ParserTokenType_t) LEX_MINUS, PAR_EXPRESSION, PAR_RULE_END },
        .output = PAR_UNARY_OPERATOR,
        .context_rule = {CR_REPLACE, CR_KEEP},
        .invert_match = {0, 0},
        .context_length = 2,
        .priority = 1,
        .description = "'-' → unary_operator [no prior context]",
    },
    {
        .context = { (ParserTokenType_t) LEX_SLASH, PAR_RULE_END },
        .output = PAR_BINARY_OPERATOR,
        .context_rule = {CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 5,
        .description = "'/' → binary_operator",
    },
    {
        .context = { PAR_EXPRESSION, (ParserTokenType_t) LEX_STAR, PAR_EXPRESSION, PAR_RULE_END },
        .output = PAR_BINARY_OPERATOR,
        .context_rule = {CR_KEEP, CR_REPLACE, CR_KEEP},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 5,
        .description = "'*' → binary_operator",
    },
    {
        .context = { (ParserTokenType_t) LEX_STAR, (ParserTokenType_t) LEX_IDENTIFIER, PAR_RULE_END },
        .output = PAR_DEREFERENCE,
        .context_rule = {CR_REPLACE, CR_KEEP},
        .invert_match = {0, 0},
        .context_length = 2,
        .priority = 3,
        .description = "'*' → dereference",
    },

    // Type declarations
    {
        .context = { (ParserTokenType_t) LEX_UNSIGNED_INTEGER_16, PAR_RULE_END },
        .output = PAR_TYPE_DECLARATION,
        .context_rule = {CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 11,
        .description = "'u16' → type declaration",
    },
    {
        .context = { (ParserTokenType_t) LEX_SIGNED_INTEGER_16, PAR_RULE_END },
        .output = PAR_TYPE_DECLARATION,
        .context_rule = {CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 11,
        .description = "'s16' → type declaration",
    },
    {
        .context = { (ParserTokenType_t) LEX_FLOAT_16, PAR_RULE_END },
        .output = PAR_TYPE_DECLARATION,
        .context_rule = {CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 11,
        .description = "'f16' → type declaration",
    },
    {
        .context = { PAR_TYPE_DECLARATION, (ParserTokenType_t) LEX_IDENTIFIER, (ParserTokenType_t) LEX_ASSIGN, PAR_EXPRESSION, (ParserTokenType_t) LEX_SEMICOLON, PAR_RULE_END },
        .output = PAR_VARIABLE_DECLARATION,
        .context_rule = {CR_REPLACE, CR_DISCARD, CR_DISCARD, CR_DISCARD, CR_DISCARD},
        .invert_match = {0, 0, 0, 0, 0},
        .context_length = 5,
        .priority = 11,
        .description = "'TYPE ident = expression' → variable declaration",
    },

    // End sentinel
    {
        .context = { PAR_RULE_END },
        .output = PAR_UNUSED,
        .context_rule = {CR_REPLACE},
        .invert_match = {0},
        .context_length = 0,
        .priority = -1,
        .description = "end of rule list",
    }
};



