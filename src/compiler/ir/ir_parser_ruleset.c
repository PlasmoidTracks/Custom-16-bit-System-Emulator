#include "compiler/ir/ir_lexer.h"
#include "compiler/ir/ir_parser.h"
#include "compiler/ir/ir_parser_ruleset.h"

#define IR_PAR_RULE_END ((IRParserTokenType_t)(-1))

IRGrammarRule_t parser_ruleset[256] = {
    // Expressions
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_IDENTIFIER,
            (IRParserTokenType_t) IR_LEX_ASSIGN,     // NOT "index1 = " to "expression = "
            IR_PAR_RULE_END
        },
        .output = IR_PAR_EXPRESSION,
        .context_rule = {IR_CR_REPLACE, IR_CR_KEEP},
        .invert_match = {0, 1},
        .context_length = 2,
        .priority = 0,         // DO AT VERY LAST!
        .description = "identifier -> expression",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_NUMBER,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_EXPRESSION,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100, 
        .description = "number -> expression",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_CHAR_LITERAL,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_EXPRESSION,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100, 
        .description = "char literal -> expression",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_ARG,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_EXPRESSION,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100, 
        .description = "arg -> expression",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_PAR_EXPRESSION,
            (IRParserTokenType_t) IR_PAR_BINARY_OPERATOR,
            (IRParserTokenType_t) IR_PAR_EXPRESSION,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_EXPRESSION,
        .context_rule = {IR_CR_DISCARD, IR_CR_REPLACE, IR_CR_DISCARD},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 100, 
        .description = "expr binop expr -> expression",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_PAR_UNARY_OPERATOR,
            (IRParserTokenType_t) IR_PAR_EXPRESSION,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_EXPRESSION,
        .context_rule = {IR_CR_DISCARD, IR_CR_REPLACE},
        .invert_match = {0, 0},
        .context_length = 2,
        .priority = 100, 
        .description = "unaryop expr -> expression",
    },

    // Type definitions
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_VAR,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_TYPE_DEFINITION,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100, 
        .description = "var -> type definition",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_STATIC,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_TYPE_DEFINITION,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100, 
        .description = "static -> type definition",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_CONST,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_TYPE_DEFINITION,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100, 
        .description = "const -> type definition",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_ANON,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_TYPE_DEFINITION,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100, 
        .description = "anon -> type definition",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_VOLATILE,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_TYPE_DEFINITION,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100, 
        .description = "volatile -> type definition",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_PAR_TYPE_DEFINITION,
            (IRParserTokenType_t) IR_PAR_TYPE_DEFINITION,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_TYPE_DEFINITION,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD},
        .invert_match = {0, 0},
        .context_length = 2,
        .priority = 1000, 
        .description = "type_def type_def -> type definition",
    },
    
    // Binary Operators
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_INTEGER_PLUS,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "i+ -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_FLOAT_PLUS,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "f+ -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_BFLOAT_PLUS,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "b+ -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_INTEGER_MINUS,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "i- -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_FLOAT_MINUS,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "f- -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_BFLOAT_MINUS,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "b- -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_INTEGER_STAR,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "i* -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_FLOAT_STAR,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "f* -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_BFLOAT_STAR,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "b* -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_INTEGER_SLASH,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "i/ -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_FLOAT_SLASH,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "f/ -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_BFLOAT_SLASH,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "b/ -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_UNSIGNED_INTEGER_GREATER,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "u> -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_INTEGER_GREATER,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "i> -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_FLOAT_GREATER,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "f> -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_BFLOAT_GREATER,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "b> -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_UNSIGNED_INTEGER_GREATER_EQUAL,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "u>= -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_INTEGER_GREATER_EQUAL,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "i>= -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_FLOAT_GREATER_EQUAL,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "f>= -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_BFLOAT_GREATER_EQUAL,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "b>= -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_UNSIGNED_INTEGER_LESS,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "u< -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_INTEGER_LESS,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "i< -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_FLOAT_LESS,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "f< -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_BFLOAT_LESS,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "b< -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_UNSIGNED_INTEGER_LESS_EQUAL,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "u<= -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_INTEGER_LESS_EQUAL,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "i<= -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_FLOAT_LESS_EQUAL,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "f<= -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_BFLOAT_LESS_EQUAL,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "b<= -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_EQUAL_EQUAL,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "== -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_BANG_EQUAL,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "!= -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_BITWISE_AND,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "& -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_BITWISE_OR,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "| -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_BITWISE_XOR,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "^ -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_SHIFT_LEFT,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "<< -> binary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_SHIFT_RIGHT,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_BINARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = ">> -> binary_operator",
    },

    // Unary operators
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_CFI,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_UNARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "cfi -> unary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_CFB,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_UNARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "cfb -> unary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_CIF,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_UNARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "cif -> unary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_CIB,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_UNARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "cib -> unary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_CBI,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_UNARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "cbi -> unary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_CBF,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_UNARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "cbf -> unary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_CBW,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_UNARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "cbw -> unary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_REF,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_UNARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "ref -> unary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_DEREF,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_UNARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 0,
        .description = "deref -> unary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_BITWISE_NOT,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_UNARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "~ -> unary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_BANG,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_UNARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "! -> unary_operator",
    },

    // Variable declaration
    {
        .context = {
            (IRParserTokenType_t) IR_PAR_TYPE_DEFINITION,
            (IRParserTokenType_t) IR_LEX_IDENTIFIER,
            (IRParserTokenType_t) IR_LEX_SEMICOLON,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_VARIABLE_DECLARATION,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD, IR_CR_DISCARD},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 50,
        .description = "var identifier -> variable_declaration",
    },

    // Variable assignment
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_IDENTIFIER,
            (IRParserTokenType_t) IR_LEX_ASSIGN,
            (IRParserTokenType_t) IR_PAR_EXPRESSION,
            (IRParserTokenType_t) IR_LEX_SEMICOLON,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_VARIABLE_ASSIGNMENT,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD, IR_CR_DISCARD, IR_CR_DISCARD},
        .invert_match = {0, 0, 0, 0},
        .context_length = 4,
        .priority = 20,
        .description = "identifier = expression; -> variable_assignment",
    },

    // Variable deref assignment
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_DEREF,
            (IRParserTokenType_t) IR_LEX_IDENTIFIER,
            (IRParserTokenType_t) IR_LEX_ASSIGN,
            (IRParserTokenType_t) IR_PAR_EXPRESSION,
            (IRParserTokenType_t) IR_LEX_SEMICOLON,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_DEREF_VARIABLE_ASSIGNMENT,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD, IR_CR_DISCARD, IR_CR_DISCARD, IR_CR_DISCARD},
        .invert_match = {0, 0, 0, 0, 0},
        .context_length = 5,
        .priority = 2000,
        .description = "deref identifier = expression; -> deref_variable_assignment",
    },

    {
        .context = {
            (IRParserTokenType_t) IR_LEX_DEREF,
            (IRParserTokenType_t) IR_LEX_IDENTIFIER,
            (IRParserTokenType_t) IR_LEX_ASSIGN,
            (IRParserTokenType_t) IR_LEX_IDENTIFIER,
            (IRParserTokenType_t) IR_LEX_SEMICOLON,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_EXPRESSION,
        .context_rule = {IR_CR_KEEP, IR_CR_KEEP, IR_CR_KEEP, IR_CR_REPLACE, IR_CR_KEEP},
        .invert_match = {0, 0, 0, 0, 0},
        .context_length = 5,
        .priority = 2000,
        .description = "deref identifier = ident; -> deref identifier = expression;",
    },

    // Variable function pointer assignment
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_IDENTIFIER,
            (IRParserTokenType_t) IR_LEX_ASSIGN,
            (IRParserTokenType_t) IR_LEX_LABEL,
            (IRParserTokenType_t) IR_LEX_SEMICOLON,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_VARIABLE_FUNCTION_POINTER_ASSIGNMENT,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD, IR_CR_DISCARD, IR_CR_DISCARD},
        .invert_match = {0, 0, 0, 0},
        .context_length = 4,
        .priority = 20,
        .description = "identifier = .label; -> variable_function_pointer_assignment",
    },

    // Goto
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_GOTO,
            (IRParserTokenType_t) IR_LEX_LABEL,
            (IRParserTokenType_t) IR_LEX_SEMICOLON,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_GOTO,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD, IR_CR_DISCARD},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 20,
        .description = "goto .label; -> goto",
    },

    // Goto
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_GOTO,
            (IRParserTokenType_t) IR_PAR_EXPRESSION,
            (IRParserTokenType_t) IR_LEX_SEMICOLON,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_GOTO,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD, IR_CR_DISCARD},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 20,
        .description = "goto expr; -> goto",
    },

    // If
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_IF,
            (IRParserTokenType_t) IR_PAR_EXPRESSION,
            (IRParserTokenType_t) IR_LEX_LABEL,
            (IRParserTokenType_t) IR_LEX_SEMICOLON,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_IF,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD, IR_CR_DISCARD, IR_CR_DISCARD},
        .invert_match = {0, 0, 0, 0},
        .context_length = 4,
        .priority = 200,
        .description = "if expr .label; -> if",
    },

    // Callpusharg
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_CALLPUSHARG,
            (IRParserTokenType_t) IR_PAR_EXPRESSION,
            (IRParserTokenType_t) IR_LEX_SEMICOLON,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_CALLPUSHARG,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD, IR_CR_DISCARD},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 20,
        .description = "callpusharg identifier; -> callpusharg",
    },

    // Callfreearg
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_CALLFREEARG,
            (IRParserTokenType_t) IR_LEX_NUMBER,
            (IRParserTokenType_t) IR_LEX_SEMICOLON,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_CALLFREEARG,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD, IR_CR_DISCARD},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 200,
        .description = "callfreearg number; -> callfreearg",
    },

    // Call .label
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_CALL,
            (IRParserTokenType_t) IR_LEX_LABEL,
            (IRParserTokenType_t) IR_LEX_SEMICOLON,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_CALL_LABEL,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD, IR_CR_DISCARD},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 20,
        .description = "call .label; -> call",
    },

    // Call expression
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_CALL,
            (IRParserTokenType_t) IR_PAR_EXPRESSION,
            (IRParserTokenType_t) IR_LEX_SEMICOLON,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_CALL_EXPRESSION,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD, IR_CR_DISCARD},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 20,
        .description = "call expression; -> call",
    },

    // .address
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_ADDRESS,
            (IRParserTokenType_t) IR_LEX_NUMBER,
            (IRParserTokenType_t) IR_LEX_SEMICOLON,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_ADDRESS,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD, IR_CR_DISCARD},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 1000,
        .description = ".address number; -> address",
    },

    // Inline assembly
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_ASM,
            (IRParserTokenType_t) IR_LEX_STRING,
            (IRParserTokenType_t) IR_LEX_SEMICOLON,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_INLINE_ASM,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD, IR_CR_DISCARD},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 200,
        .description = "asm \"asm\"; -> inline assembly",
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



