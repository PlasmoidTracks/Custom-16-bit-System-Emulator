#include "transpile/ira_lexer.h"
#include "transpile/ira_parser.h"
#include "transpile/ira_parser_ruleset.h"

#define IRA_PAR_RULE_END ((IRAParserTokenType_t)(-1))

IRAGrammarRule_t ira_parser_ruleset[256] = {
    // Expressions
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_IDENTIFIER,
            (IRAParserTokenType_t) IRA_LEX_ASSIGN,     // NOT "index1 = " to "expression = "
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_EXPRESSION,
        .context_rule = {IRA_CR_REPLACE, IRA_CR_KEEP},
        .invert_match = {0, 1},
        .context_length = 2,
        .priority = 0,         // DO AT VERY LAST!
        .description = "identifier -> expression",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_NUMBER,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_EXPRESSION,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100, 
        .description = "number -> expression",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_CHAR_LITERAL,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_EXPRESSION,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100, 
        .description = "number -> expression",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_ARG,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_EXPRESSION,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100, 
        .description = "arg -> expression",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_PAR_EXPRESSION,
            (IRAParserTokenType_t) IRA_PAR_BINARY_OPERATOR,
            (IRAParserTokenType_t) IRA_PAR_EXPRESSION,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_EXPRESSION,
        .context_rule = {IRA_CR_DISCARD, IRA_CR_REPLACE, IRA_CR_DISCARD},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 100, 
        .description = "expr binop expr -> expression",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_PAR_UNARY_OPERATOR,
            (IRAParserTokenType_t) IRA_PAR_EXPRESSION,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_EXPRESSION,
        .context_rule = {IRA_CR_DISCARD, IRA_CR_REPLACE},
        .invert_match = {0, 0},
        .context_length = 2,
        .priority = 100, 
        .description = "unaryop expr -> expression",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_PAR_TYPE_CAST,
            (IRAParserTokenType_t) IRA_PAR_EXPRESSION,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_EXPRESSION,
        .context_rule = {IRA_CR_DISCARD, IRA_CR_REPLACE},
        .invert_match = {0, 0},
        .context_length = 2,
        .priority = 100, 
        .description = "type cast -> expression",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_LEFT_PARENTH,
            (IRAParserTokenType_t) IRA_PAR_TYPE,
            (IRAParserTokenType_t) IRA_LEX_RIGHT_PARENTH,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_TYPE_CAST,
        .context_rule = {IRA_CR_DISCARD, IRA_CR_REPLACE, IRA_CR_DISCARD},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 100, 
        .description = "type cast -> expression",
    },

    // Type definitions
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_INT16,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_TYPE,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100, 
        .description = "int16 -> type definition",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_UINT16,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_TYPE,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100, 
        .description = "uint16 -> type definition",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_INT8,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_TYPE,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100, 
        .description = "int8 -> type definition",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_UINT8,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_TYPE,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100, 
        .description = "uint8 -> type definition",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_INT32,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_TYPE,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100, 
        .description = "int32 -> type definition",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_UINT32,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_TYPE,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100, 
        .description = "uint32 -> type definition",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_FLOAT16,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_TYPE,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100, 
        .description = "float16 -> type definition",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_STATIC,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_TYPE_MODIFIER,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100, 
        .description = "static -> type definition",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_CONST,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_TYPE_MODIFIER,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100, 
        .description = "const -> type definition",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_ANON,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_TYPE_MODIFIER,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100, 
        .description = "anon -> type definition",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_PAR_TYPE_MODIFIER,
            (IRAParserTokenType_t) IRA_PAR_TYPE_MODIFIER,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_TYPE_MODIFIER,
        .context_rule = {IRA_CR_REPLACE, IRA_CR_DISCARD},
        .invert_match = {0, 0},
        .context_length = 2,
        .priority = 1000, 
        .description = "type_def type_def -> type definition",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_PAR_TYPE_MODIFIER,
            (IRAParserTokenType_t) IRA_PAR_TYPE,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_TYPE,
        .context_rule = {IRA_CR_REPLACE, IRA_CR_DISCARD},
        .invert_match = {0, 0},
        .context_length = 2,
        .priority = 1000, 
        .description = "type_def type_def -> type definition",
    },
    
    // Binary Operators
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_PLUS,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_BINARY_OPERATOR,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "+ -> binary_operator",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_MINUS,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_BINARY_OPERATOR,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "- -> binary_operator",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_STAR,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_BINARY_OPERATOR,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "* -> binary_operator",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_SLASH,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_BINARY_OPERATOR,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "/ -> binary_operator",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_GREATER,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_BINARY_OPERATOR,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "i> -> binary_operator",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_GREATER_EQUAL,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_BINARY_OPERATOR,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "i>= -> binary_operator",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_LESS,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_BINARY_OPERATOR,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "i< -> binary_operator",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_LESS_EQUAL,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_BINARY_OPERATOR,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "i<= -> binary_operator",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_EQUAL_EQUAL,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_BINARY_OPERATOR,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "== -> binary_operator",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_BANG_EQUAL,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_BINARY_OPERATOR,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "!= -> binary_operator",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_BITWISE_AND,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_BINARY_OPERATOR,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "& -> binary_operator",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_BITWISE_OR,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_BINARY_OPERATOR,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "| -> binary_operator",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_BITWISE_XOR,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_BINARY_OPERATOR,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "^ -> binary_operator",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_SHIFT_LEFT,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_BINARY_OPERATOR,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "<< -> binary_operator",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_SHIFT_RIGHT,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_BINARY_OPERATOR,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = ">> -> binary_operator",
    },

    // Unary operators
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_REF,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_UNARY_OPERATOR,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "ref -> unary_operator",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_DEREF,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_UNARY_OPERATOR,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 0,
        .description = "deref -> unary_operator",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_BITWISE_NOT,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_UNARY_OPERATOR,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "~ -> unary_operator",
    },
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_BANG,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_UNARY_OPERATOR,
        .context_rule = {IRA_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 100,
        .description = "! -> unary_operator",
    },

    // Variable declaration
    {
        .context = {
            (IRAParserTokenType_t) IRA_PAR_TYPE,
            (IRAParserTokenType_t) IRA_LEX_IDENTIFIER,
            (IRAParserTokenType_t) IRA_LEX_SEMICOLON,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_VARIABLE_DECLARATION,
        .context_rule = {IRA_CR_REPLACE, IRA_CR_DISCARD, IRA_CR_DISCARD},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 50,
        .description = "var identifier -> variable_declaration",
    },

    // Variable assignment
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_IDENTIFIER,
            (IRAParserTokenType_t) IRA_LEX_ASSIGN,
            (IRAParserTokenType_t) IRA_PAR_EXPRESSION,
            (IRAParserTokenType_t) IRA_LEX_SEMICOLON,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_VARIABLE_ASSIGNMENT,
        .context_rule = {IRA_CR_REPLACE, IRA_CR_DISCARD, IRA_CR_DISCARD, IRA_CR_DISCARD},
        .invert_match = {0, 0, 0, 0},
        .context_length = 4,
        .priority = 20,
        .description = "identifier = expression; -> variable_assignment",
    },

    // Variable deref assignment
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_DEREF,
            (IRAParserTokenType_t) IRA_LEX_IDENTIFIER,
            (IRAParserTokenType_t) IRA_LEX_ASSIGN,
            (IRAParserTokenType_t) IRA_PAR_EXPRESSION,
            (IRAParserTokenType_t) IRA_LEX_SEMICOLON,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_DEREF_VARIABLE_ASSIGNMENT,
        .context_rule = {IRA_CR_REPLACE, IRA_CR_DISCARD, IRA_CR_DISCARD, IRA_CR_DISCARD, IRA_CR_DISCARD},
        .invert_match = {0, 0, 0, 0, 0},
        .context_length = 5,
        .priority = 2000,
        .description = "deref identifier = expression; -> deref_variable_assignment",
    },

    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_DEREF,
            (IRAParserTokenType_t) IRA_LEX_IDENTIFIER,
            (IRAParserTokenType_t) IRA_LEX_ASSIGN,
            (IRAParserTokenType_t) IRA_LEX_IDENTIFIER,
            (IRAParserTokenType_t) IRA_LEX_SEMICOLON,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_EXPRESSION,
        .context_rule = {IRA_CR_KEEP, IRA_CR_KEEP, IRA_CR_KEEP, IRA_CR_REPLACE, IRA_CR_KEEP},
        .invert_match = {0, 0, 0, 0, 0},
        .context_length = 5,
        .priority = 2000,
        .description = "deref identifier = ident; -> deref identifier = expression;",
    },

    // Variable function pointer assignment
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_IDENTIFIER,
            (IRAParserTokenType_t) IRA_LEX_ASSIGN,
            (IRAParserTokenType_t) IRA_LEX_LABEL,
            (IRAParserTokenType_t) IRA_LEX_SEMICOLON,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_VARIABLE_FUNCTION_POINTER_ASSIGNMENT,
        .context_rule = {IRA_CR_REPLACE, IRA_CR_DISCARD, IRA_CR_DISCARD, IRA_CR_DISCARD},
        .invert_match = {0, 0, 0, 0},
        .context_length = 4,
        .priority = 20,
        .description = "identifier = .label; -> variable_function_pointer_assignment",
    },

    // Variable string ref assignment
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_IDENTIFIER,
            (IRAParserTokenType_t) IRA_LEX_ASSIGN,
            (IRAParserTokenType_t) IRA_LEX_REF,
            (IRAParserTokenType_t) IRA_LEX_STRING,
            (IRAParserTokenType_t) IRA_LEX_SEMICOLON,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_VARIABLE_REF_STRING_ASSIGNMENT,
        .context_rule = {IRA_CR_REPLACE, IRA_CR_DISCARD, IRA_CR_DISCARD, IRA_CR_DISCARD, IRA_CR_DISCARD},
        .invert_match = {0, 0, 0, 0, 0},
        .context_length = 5,
        .priority = 2000,
        .description = "identifier = ref string; -> variable_ref_string_assignment",
    },

    // Goto
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_GOTO,
            (IRAParserTokenType_t) IRA_LEX_LABEL,
            (IRAParserTokenType_t) IRA_LEX_SEMICOLON,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_GOTO,
        .context_rule = {IRA_CR_REPLACE, IRA_CR_DISCARD, IRA_CR_DISCARD},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 20,
        .description = "goto .label; -> goto",
    },

    // Goto
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_GOTO,
            (IRAParserTokenType_t) IRA_PAR_EXPRESSION,
            (IRAParserTokenType_t) IRA_LEX_SEMICOLON,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_GOTO,
        .context_rule = {IRA_CR_REPLACE, IRA_CR_DISCARD, IRA_CR_DISCARD},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 20,
        .description = "goto expr; -> goto",
    },

    // If
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_IF,
            (IRAParserTokenType_t) IRA_PAR_EXPRESSION,
            (IRAParserTokenType_t) IRA_LEX_LABEL,
            (IRAParserTokenType_t) IRA_LEX_SEMICOLON,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_IF,
        .context_rule = {IRA_CR_REPLACE, IRA_CR_DISCARD, IRA_CR_DISCARD, IRA_CR_DISCARD},
        .invert_match = {0, 0, 0, 0},
        .context_length = 4,
        .priority = 200,
        .description = "if expr .label; -> if",
    },

    // Callpusharg
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_CALLPUSHARG,
            (IRAParserTokenType_t) IRA_PAR_EXPRESSION,
            (IRAParserTokenType_t) IRA_LEX_SEMICOLON,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_CALLPUSHARG,
        .context_rule = {IRA_CR_REPLACE, IRA_CR_DISCARD, IRA_CR_DISCARD},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 20,
        .description = "callpusharg identifier; -> callpusharg",
    },

    // Callfreearg
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_CALLFREEARG,
            (IRAParserTokenType_t) IRA_LEX_NUMBER,
            (IRAParserTokenType_t) IRA_LEX_SEMICOLON,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_CALLFREEARG,
        .context_rule = {IRA_CR_REPLACE, IRA_CR_DISCARD, IRA_CR_DISCARD},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 200,
        .description = "callfreearg number; -> callfreearg",
    },

    // Call .label
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_CALL,
            (IRAParserTokenType_t) IRA_LEX_LABEL,
            (IRAParserTokenType_t) IRA_LEX_SEMICOLON,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_CALL_LABEL,
        .context_rule = {IRA_CR_REPLACE, IRA_CR_DISCARD, IRA_CR_DISCARD},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 20,
        .description = "call .label; -> call",
    },

    // Call expression
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_CALL,
            (IRAParserTokenType_t) IRA_PAR_EXPRESSION,
            (IRAParserTokenType_t) IRA_LEX_SEMICOLON,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_CALL_EXPRESSION,
        .context_rule = {IRA_CR_REPLACE, IRA_CR_DISCARD, IRA_CR_DISCARD},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 20,
        .description = "call expression; -> call",
    },

    // .address
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_ADDRESS,
            (IRAParserTokenType_t) IRA_LEX_NUMBER,
            (IRAParserTokenType_t) IRA_LEX_SEMICOLON,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_ADDRESS,
        .context_rule = {IRA_CR_REPLACE, IRA_CR_DISCARD, IRA_CR_DISCARD},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 1000,
        .description = ".address number; -> address",
    },

    // Inline assembly
    {
        .context = {
            (IRAParserTokenType_t) IRA_LEX_ASM,
            (IRAParserTokenType_t) IRA_LEX_STRING,
            (IRAParserTokenType_t) IRA_LEX_SEMICOLON,
            IRA_PAR_RULE_END
        },
        .output = IRA_PAR_INLINE_ASM,
        .context_rule = {IRA_CR_REPLACE, IRA_CR_DISCARD, IRA_CR_DISCARD},
        .invert_match = {0, 0, 0},
        .context_length = 3,
        .priority = 200,
        .description = "asm \"asm\"; -> inline assembly",
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



