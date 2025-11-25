#include "transpile/ccan/ccan_lexer.h"
#include "transpile/ccan/ccan_parser.h"
#include "transpile/ccan/ccan_parser_ruleset.h"

#define CCAN_PAR_RULE_END ((CCANParserTokenType_t)(-1))

CCANGrammarRule_t ccan_parser_ruleset[256] = {
    // Expressions
{
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION,
            (CCANParserTokenType_t) CCAN_PAR_BINARY_OPERATOR,
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION,
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_EXPRESSION,
        .context_rule = {CCAN_CR_DISCARD, CCAN_CR_REPLACE, CCAN_CR_DISCARD},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 100, 
        .description = "expr binop expr -> expr",
    },
    {
            .context = {
                (CCANParserTokenType_t) CCAN_PAR_UNARY_OPERATOR,
                (CCANParserTokenType_t) CCAN_PAR_EXPRESSION,
                CCAN_PAR_RULE_END
            },
            .output = CCAN_PAR_EXPRESSION,
            .context_rule = {CCAN_CR_REPLACE, CCAN_CR_DISCARD},
            .invert_match = {0, 0},
            .context_length = 2,
            .priority = 100, 
            .description = "unaryop expr -> expr",
        },
        {
            .context = {
                (CCANParserTokenType_t) CCAN_LEX_ELSE,
                (CCANParserTokenType_t) CCAN_LEX_IF,
                (CCANParserTokenType_t) CCAN_LEX_LEFT_PARENTHESES,
                (CCANParserTokenType_t) CCAN_PAR_EXPRESSION,
                (CCANParserTokenType_t) CCAN_LEX_RIGHT_PARENTHESES,
                CCAN_PAR_RULE_END
            },
            .output = CCAN_PAR_ELSE_IF_CLAUSE,
            .context_rule = {CCAN_CR_REPLACE, CCAN_CR_DISCARD, CCAN_CR_DISCARD, CCAN_CR_DISCARD, CCAN_CR_DISCARD},
            .invert_match = {0, 0, 0, 0, 0},
            .context_length = 5,
            .priority = 110, 
            .description = "else if ( expr ) -> elseifclause",
        },
        {
            .context = {
                (CCANParserTokenType_t) CCAN_LEX_IF,
                (CCANParserTokenType_t) CCAN_LEX_LEFT_PARENTHESES,
                (CCANParserTokenType_t) CCAN_PAR_EXPRESSION,
                (CCANParserTokenType_t) CCAN_LEX_RIGHT_PARENTHESES,
                CCAN_PAR_RULE_END
            },
            .output = CCAN_PAR_IF_CLAUSE,
            .context_rule = {CCAN_CR_REPLACE, CCAN_CR_DISCARD, CCAN_CR_DISCARD, CCAN_CR_DISCARD},
            .invert_match = {0, 0, 0, 0},
            .context_length = 4,
            .priority = 100, 
            .description = "if ( expr ) -> ifclause",
        },
        {
            .context = {
                (CCANParserTokenType_t) CCAN_LEX_ELSE,
                (CCANParserTokenType_t) CCAN_LEX_IF,
                CCAN_PAR_RULE_END
            },
            .output = CCAN_PAR_ELSE_CLAUSE,
            .context_rule = {CCAN_CR_REPLACE, CCAN_CR_KEEP},
            .invert_match = {0, 1},
            .context_length = 2,
            .priority = 100, 
            .description = "else -> elseclause",
        },
        {
            .context = {
                (CCANParserTokenType_t) CCAN_PAR_IF_CLAUSE,
                (CCANParserTokenType_t) CCAN_PAR_SCOPE_BODY,
                CCAN_PAR_RULE_END
            },
            .output = CCAN_PAR_IF_BRANCH,
            .context_rule = {CCAN_CR_REPLACE, CCAN_CR_DISCARD},
            .invert_match = {0, 0},
            .context_length = 2,
            .priority = 100, 
            .description = "ifclause scopebdy -> ifbranch",
        },
        {
            .context = {
                (CCANParserTokenType_t) CCAN_PAR_ELSE_IF_CLAUSE,
                (CCANParserTokenType_t) CCAN_PAR_SCOPE_BODY,
                CCAN_PAR_RULE_END
            },
            .output = CCAN_PAR_ELSE_IF_BRANCH,
            .context_rule = {CCAN_CR_REPLACE, CCAN_CR_DISCARD},
            .invert_match = {0, 0},
            .context_length = 2,
            .priority = 100, 
            .description = "elseifclause scopebdy -> elseifbranch",
        },
        {
            .context = {
                (CCANParserTokenType_t) CCAN_PAR_ELSE_CLAUSE,
                (CCANParserTokenType_t) CCAN_PAR_SCOPE_BODY,
                CCAN_PAR_RULE_END
            },
            .output = CCAN_PAR_ELSE_BRANCH,
            .context_rule = {CCAN_CR_REPLACE, CCAN_CR_DISCARD},
            .invert_match = {0, 0},
            .context_length = 2,
            .priority = 100, 
            .description = "elseclause scopebdy -> elsebranch",
        },
        {
            .context = {
                (CCANParserTokenType_t) CCAN_PAR_ELSE_IF_BRANCH,
                (CCANParserTokenType_t) CCAN_PAR_ELSE_IF_BRANCH,
                CCAN_PAR_RULE_END
            },
            .output = CCAN_PAR_ELSE_IF_BRANCH,
            .context_rule = {CCAN_CR_REPLACE, CCAN_CR_DISCARD},
            .invert_match = {0, 0},
            .context_length = 2,
            .priority = 100, 
            .description = "elseifbranch elseifbranch -> elseifbranch",
        },
        {
            .context = {
                (CCANParserTokenType_t) CCAN_PAR_IF_BRANCH,
                (CCANParserTokenType_t) CCAN_PAR_ELSE_IF_BRANCH,
                (CCANParserTokenType_t) CCAN_PAR_ELSE_BRANCH,
                CCAN_PAR_RULE_END
            },
            .output = CCAN_PAR_IF_CONTROL_FLOW,
            .context_rule = {CCAN_CR_REPLACE, CCAN_CR_DISCARD, CCAN_CR_DISCARD},
            .invert_match = {0, 0, 0},
            .context_length = 3,
            .priority = 100, 
            .description = "ifbranch elseifbranch elsebranch -> ifctlflow",
        },
        {
            .context = {
                (CCANParserTokenType_t) CCAN_PAR_IF_BRANCH,
                (CCANParserTokenType_t) CCAN_PAR_ELSE_BRANCH,
                CCAN_PAR_RULE_END
            },
            .output = CCAN_PAR_IF_CONTROL_FLOW,
            .context_rule = {CCAN_CR_REPLACE, CCAN_CR_DISCARD},
            .invert_match = {0, 0},
            .context_length = 2,
            .priority = 100, 
            .description = "ifbranch elsebranch -> ifctlflow",
        },
        {
            .context = {
                (CCANParserTokenType_t) CCAN_PAR_IF_BRANCH,
                CCAN_PAR_RULE_END
            },
            .output = CCAN_PAR_IF_CONTROL_FLOW,
            .context_rule = {CCAN_CR_REPLACE},
            .invert_match = {0},
            .context_length = 1,
            .priority = 1, 
            .description = "ifbranch -> ifctlflow",
        },
        {
            .context = {
                (CCANParserTokenType_t) CCAN_PAR_IF_CONTROL_FLOW,
                CCAN_PAR_RULE_END
            },
            .output = CCAN_PAR_STATEMENT,
            .context_rule = {CCAN_CR_REPLACE},
            .invert_match = {0},
            .context_length = 1,
            .priority = 100, 
            .description = "ifctlflow -> statement",
        },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_LEX_LEFT_PARENTHESES,
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION,
            (CCANParserTokenType_t) CCAN_LEX_RIGHT_PARENTHESES,
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_EXPRESSION,
        .context_rule = {CCAN_CR_DISCARD, CCAN_CR_REPLACE, CCAN_CR_DISCARD},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 10, 
        .description = "( expr ) -> expr",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_LEX_INT,
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_DATATYPE,
        .context_rule = {CCAN_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100, 
        .description = "int -> datatype",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_LEX_FLOAT,
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_DATATYPE,
        .context_rule = {CCAN_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100, 
        .description = "int -> datatype",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_DATATYPE,
            (CCANParserTokenType_t) CCAN_LEX_IDENTIFIER,
            (CCANParserTokenType_t) CCAN_LEX_LEFT_PARENTHESES,
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_FUNCTION_SIGNATURE,
        .context_rule = {CCAN_CR_REPLACE, CCAN_CR_DISCARD, CCAN_CR_KEEP},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 1000, 
        .description = "datatype ident ( -> funcsig (",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_DATATYPE,
            (CCANParserTokenType_t) CCAN_LEX_IDENTIFIER,
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_VARIABLE_SIGNATURE,
        .context_rule = {CCAN_CR_REPLACE, CCAN_CR_DISCARD},
        .invert_match = {0, 0},
        .context_length = 2,
        .priority = 100, 
        .description = "datatype ident -> varsig",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_VARIABLE_SIGNATURE,
            (CCANParserTokenType_t) CCAN_LEX_ASSIGN,
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION,
            (CCANParserTokenType_t) CCAN_LEX_SEMICOLON,
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_ASSIGNED_VARIABLE_DECLARATION,
        .context_rule = {CCAN_CR_REPLACE, CCAN_CR_DISCARD, CCAN_CR_DISCARD, CCAN_CR_DISCARD},
        .invert_match = {0, 0, 0, 0},
        .context_length = 4,
        .priority = 100, 
        .description = "varsig = expr; -> varassigndecl",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_VARIABLE_SIGNATURE,
            (CCANParserTokenType_t) CCAN_LEX_SEMICOLON,
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_UNDEFINED_VARIABLE_DECLARATION,
        .context_rule = {CCAN_CR_REPLACE, CCAN_CR_DISCARD},
        .invert_match = {0, 0},
        .context_length = 2,
        .priority = 100, 
        .description = "varsig; -> vardecl",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_UNDEFINED_VARIABLE_DECLARATION,
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_VARIABLE_DECLARATION,
        .context_rule = {CCAN_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100, 
        .description = "varassigndecl -> varassigndecl",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_ASSIGNED_VARIABLE_DECLARATION,
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_VARIABLE_DECLARATION,
        .context_rule = {CCAN_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100, 
        .description = "vardecl -> varassigndecl",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION, 
            (CCANParserTokenType_t) CCAN_LEX_PLUS,
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION, 
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_BINARY_OPERATOR,
        .context_rule = {CCAN_CR_KEEP, CCAN_CR_REPLACE, CCAN_CR_KEEP},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 10, 
        .description = "expr + expr -> expr binop expr",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION, 
            (CCANParserTokenType_t) CCAN_LEX_MINUS,
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION, 
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_BINARY_OPERATOR,
        .context_rule = {CCAN_CR_KEEP, CCAN_CR_REPLACE, CCAN_CR_KEEP},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 10, 
        .description = "expr - expr -> expr binop expr",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION, 
            (CCANParserTokenType_t) CCAN_LEX_STAR,
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION, 
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_BINARY_OPERATOR,
        .context_rule = {CCAN_CR_KEEP, CCAN_CR_REPLACE, CCAN_CR_KEEP},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 10, 
        .description = "expr * expr -> expr binop expr",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION, 
            (CCANParserTokenType_t) CCAN_LEX_PLUS,
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION, 
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_BINARY_OPERATOR,
        .context_rule = {CCAN_CR_KEEP, CCAN_CR_REPLACE, CCAN_CR_KEEP},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 10, 
        .description = "expr / expr -> expr binop expr",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION, 
            (CCANParserTokenType_t) CCAN_LEX_EQUAL_EQUAL,
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION, 
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_BINARY_OPERATOR,
        .context_rule = {CCAN_CR_KEEP, CCAN_CR_REPLACE, CCAN_CR_KEEP},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 10, 
        .description = "expr == expr -> expr binop expr",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION, 
            (CCANParserTokenType_t) CCAN_LEX_BANG_EQUAL,
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION, 
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_BINARY_OPERATOR,
        .context_rule = {CCAN_CR_KEEP, CCAN_CR_REPLACE, CCAN_CR_KEEP},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 10, 
        .description = "expr != expr -> expr binop expr",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION, 
            (CCANParserTokenType_t) CCAN_LEX_GREATER,
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION, 
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_BINARY_OPERATOR,
        .context_rule = {CCAN_CR_KEEP, CCAN_CR_REPLACE, CCAN_CR_KEEP},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 10, 
        .description = "expr > expr -> expr binop expr",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION, 
            (CCANParserTokenType_t) CCAN_LEX_GREATER_EQUAL,
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION, 
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_BINARY_OPERATOR,
        .context_rule = {CCAN_CR_KEEP, CCAN_CR_REPLACE, CCAN_CR_KEEP},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 10, 
        .description = "expr >= expr -> expr binop expr",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION, 
            (CCANParserTokenType_t) CCAN_LEX_LESS,
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION, 
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_BINARY_OPERATOR,
        .context_rule = {CCAN_CR_KEEP, CCAN_CR_REPLACE, CCAN_CR_KEEP},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 10, 
        .description = "expr < expr -> expr binop expr",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION, 
            (CCANParserTokenType_t) CCAN_LEX_LESS_EQUAL,
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION, 
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_BINARY_OPERATOR,
        .context_rule = {CCAN_CR_KEEP, CCAN_CR_REPLACE, CCAN_CR_KEEP},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 10, 
        .description = "expr <= expr -> expr binop expr",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION, 
            (CCANParserTokenType_t) CCAN_LEX_LOGICAL_AND,
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION, 
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_BINARY_OPERATOR,
        .context_rule = {CCAN_CR_KEEP, CCAN_CR_REPLACE, CCAN_CR_KEEP},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 10, 
        .description = "expr && expr -> expr binop expr",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION, 
            (CCANParserTokenType_t) CCAN_LEX_LOGICAL_OR,
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION, 
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_BINARY_OPERATOR,
        .context_rule = {CCAN_CR_KEEP, CCAN_CR_REPLACE, CCAN_CR_KEEP},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 10, 
        .description = "expr || expr -> expr binop expr",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION, 
            (CCANParserTokenType_t) CCAN_LEX_PLUS,
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION, 
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_UNARY_OPERATOR,
        .context_rule = {CCAN_CR_KEEP, CCAN_CR_REPLACE, CCAN_CR_KEEP},
        .invert_match = {1, 0, 0},
        .context_length = 3,
        .priority = 100, 
        .description = "+ expr -> unaryop expr",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION, 
            (CCANParserTokenType_t) CCAN_LEX_MINUS,
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION, 
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_UNARY_OPERATOR,
        .context_rule = {CCAN_CR_KEEP, CCAN_CR_REPLACE, CCAN_CR_KEEP},
        .invert_match = {1, 0, 0},
        .context_length = 3,
        .priority = 100, 
        .description = "- expr -> unaryop expr",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION, 
            (CCANParserTokenType_t) CCAN_LEX_BANG,
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION, 
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_UNARY_OPERATOR,
        .context_rule = {CCAN_CR_KEEP, CCAN_CR_REPLACE, CCAN_CR_KEEP},
        .invert_match = {1, 0, 0},
        .context_length = 3,
        .priority = 100, 
        .description = "! expr -> unaryop expr",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION,
            (CCANParserTokenType_t) CCAN_PAR_BINARY_OPERATOR,
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION,
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_EXPRESSION,
        .context_rule = {CCAN_CR_REPLACE, CCAN_CR_DISCARD, CCAN_CR_DISCARD},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 1000, 
        .description = "expr binop expr -> expr",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_LEX_NUMBER,
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_EXPRESSION,
        .context_rule = {CCAN_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100, 
        .description = "number -> expr",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_LEX_IDENTIFIER,
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_EXPRESSION,
        .context_rule = {CCAN_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 10, 
        .description = "ident -> expr",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_FUNCTION_SIGNATURE,
            (CCANParserTokenType_t) CCAN_LEX_LEFT_PARENTHESES, 
            (CCANParserTokenType_t) CCAN_PAR_VARIABLE_SIGNATURE, 
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_FUNCTION_PARAMETER,
        .context_rule = {CCAN_CR_KEEP, CCAN_CR_KEEP, CCAN_CR_REPLACE},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 100, 
        .description = "funcsig ( varsig -> funcsig ( funcpar",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_FUNCTION_PARAMETER,
            (CCANParserTokenType_t) CCAN_LEX_COMMA, 
            (CCANParserTokenType_t) CCAN_PAR_VARIABLE_SIGNATURE, 
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_FUNCTION_PARAMETER,
        .context_rule = {CCAN_CR_KEEP, CCAN_CR_DISCARD, CCAN_CR_REPLACE},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 100, 
        .description = "funcpar, varsig -> funcpar funcpar",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_FUNCTION_PARAMETER,
            (CCANParserTokenType_t) CCAN_PAR_FUNCTION_PARAMETER, 
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_FUNCTION_PARAMETER,
        .context_rule = {CCAN_CR_REPLACE, CCAN_CR_DISCARD},
        .invert_match = {0, 0},
        .context_length = 2,
        .priority = 100, 
        .description = "funcpar funcpar -> funcpar",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_FUNCTION_SIGNATURE,
            (CCANParserTokenType_t) CCAN_LEX_LEFT_PARENTHESES, 
            (CCANParserTokenType_t) CCAN_PAR_FUNCTION_PARAMETER, 
            (CCANParserTokenType_t) CCAN_LEX_RIGHT_PARENTHESES, 
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_FUNCTION_DECLARATION,
        .context_rule = {CCAN_CR_REPLACE, CCAN_CR_DISCARD, CCAN_CR_DISCARD, CCAN_CR_DISCARD},
        .invert_match = {0, 0, 0, 0},
        .context_length = 4,
        .priority = 100, 
        .description = "funcsig ( funcpar ) -> funcdecl",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_FUNCTION_SIGNATURE,
            (CCANParserTokenType_t) CCAN_LEX_LEFT_PARENTHESES, 
            (CCANParserTokenType_t) CCAN_LEX_RIGHT_PARENTHESES, 
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_FUNCTION_DECLARATION,
        .context_rule = {CCAN_CR_REPLACE, CCAN_CR_DISCARD, CCAN_CR_DISCARD},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 100, 
        .description = "funcsig ( ) -> funcdecl",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_LEX_RETURN,
            (CCANParserTokenType_t) CCAN_LEX_SEMICOLON,
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_RETURN_STATEMENT,
        .context_rule = {CCAN_CR_REPLACE, CCAN_CR_DISCARD},
        .invert_match = {0, 0},
        .context_length = 2,
        .priority = 100, 
        .description = "return ; -> returnstmnt",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_LEX_RETURN,
            (CCANParserTokenType_t) CCAN_PAR_EXPRESSION,
            (CCANParserTokenType_t) CCAN_LEX_SEMICOLON,
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_RETURN_STATEMENT,
        .context_rule = {CCAN_CR_REPLACE, CCAN_CR_DISCARD, CCAN_CR_DISCARD},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 100, 
        .description = "return expr ; -> returnstmnt",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_RETURN_STATEMENT,
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_STATEMENT,
        .context_rule = {CCAN_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100, 
        .description = "returnstmnt -> statement",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_VARIABLE_DECLARATION,
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_STATEMENT,
        .context_rule = {CCAN_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100, 
        .description = "vardecl -> statement",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_STATEMENT,
            (CCANParserTokenType_t) CCAN_PAR_STATEMENT,
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_STATEMENT,
        .context_rule = {CCAN_CR_REPLACE, CCAN_CR_DISCARD},
        .invert_match = {0, 0},
        .context_length = 2,
        .priority = 100, 
        .description = "statement statement -> statement",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_LEX_LEFT_BRACE,
            (CCANParserTokenType_t) CCAN_PAR_STATEMENT,
            (CCANParserTokenType_t) CCAN_LEX_RIGHT_BRACE,
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_SCOPE_BODY,
        .context_rule = {CCAN_CR_REPLACE, CCAN_CR_DISCARD, CCAN_CR_DISCARD},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 50, 
        .description = "{ statement } -> scopebdy",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_FUNCTION_DECLARATION,
            (CCANParserTokenType_t) CCAN_PAR_SCOPE_BODY,
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_FUNCTION_DEFINITION,
        .context_rule = {CCAN_CR_REPLACE, CCAN_CR_DISCARD},
        .invert_match = {0, 0},
        .context_length = 2,
        .priority = 100, 
        .description = "funcdecl scopebdy -> funcdef",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_PAR_SCOPE_BODY,
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_STATEMENT,
        .context_rule = {CCAN_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 10, 
        .description = "scopebdy -> statement",
    },
    {
        .context = {
            (CCANParserTokenType_t) CCAN_LEX_SEMICOLON, 
            CCAN_PAR_RULE_END
        },
        .output = CCAN_PAR_STATEMENT,
        .context_rule = {CCAN_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1, 
        .description = "; -> statement",
    },

    // End sentinel
    {
        .context = { CCAN_PAR_RULE_END },
        .output = CCAN_PAR_UNUSED,
        .context_rule = {CCAN_CR_REPLACE},
        .invert_match = {0},
        .context_length = 0,
        .priority = -1,
        .description = "end of rule list",
    }
};

