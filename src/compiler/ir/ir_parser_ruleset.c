#include "compiler/ir/ir_lexer.h"
#include "compiler/ir/ir_parser.h"
#include "compiler/ir/ir_parser_ruleset.h"

#define IR_PAR_RULE_END ((IRParserTokenType_t)(-1))

IRGrammarRule_t ir_parser_ruleset[256] = {

    {
        .context = {
            (IRParserTokenType_t) IR_LEX_STACK, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_TYPE_DEFINITION,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "stack -> type_definition",
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
        .priority = 1000, 
        .description = "static -> type_definition",
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
        .priority = 1000, 
        .description = "volatile -> type_definition",
    },

    {
        .context = {
            (IRParserTokenType_t) IR_LEX_REGISTER, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_TYPE_DEFINITION,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "register -> type_definition",
    },

    // TYPE_DEFINITION TYPE_DEFINITION -> TYPE_DEFINITION

    {
        .context = {
            (IRParserTokenType_t) IR_PAR_TYPE_DEFINITION, 
            (IRParserTokenType_t) IR_PAR_TYPE_DEFINITION, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_TYPE_DEFINITION,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD},
        .invert_match = {0},
        .context_length = 2,
        .priority = 1000, 
        .description = "type_definition type_definition -> type_definition",
    },

    
    // TYPE_DEFINITION { EXPRESSION } IDENTIFIER ;  -> VARIABLE_DECLARATION
    {
        .context = {
            (IRParserTokenType_t) IR_PAR_TYPE_DEFINITION, 
            (IRParserTokenType_t) IR_LEX_LEFT_CURLY_BRACKET, 
            (IRParserTokenType_t) IR_PAR_EXPRESSION, 
            (IRParserTokenType_t) IR_LEX_RIGHT_CURLY_BRACKET, 
            (IRParserTokenType_t) IR_LEX_IDENTIFIER, 
            (IRParserTokenType_t) IR_LEX_SEMICOLON, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_VARIABLE_DECLARATION,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD, IR_CR_DISCARD, IR_CR_DISCARD, IR_CR_DISCARD, IR_CR_DISCARD},
        .invert_match = {0},
        .context_length = 6,
        .priority = 1000, 
        .description = "type_definition { number } identifier ; -> variable_declatation",
    },

    // = -> assign
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_ASSIGN_16_BIT, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_ASSIGN,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 10000, 
        .description = "= -> assign",
    },

    // 8= -> assign
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_ASSIGN_8_BIT, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_ASSIGN,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 10000, 
        .description = "8= -> assign",
    },

    // * EXPRESSION -> DEREF_EXPRESSION
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_STAR, 
            (IRParserTokenType_t) IR_PAR_EXPRESSION,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_EXPRESSION,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD},
        .invert_match = {0},
        .context_length = 2,
        .priority = 1000, 
        .description = "* expression -> expression",
        .variant = 1, 
    },

    // & EXPRESSION -> EXPRESSION
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_AMPERSAND, 
            (IRParserTokenType_t) IR_PAR_EXPRESSION,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_EXPRESSION,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD},
        .invert_match = {0},
        .context_length = 2,
        .priority = 1000, 
        .description = "& expression -> expression",
        .variant = 2, 
    },


    
    // { IDENTIFIER }  -> { EXPRESSION }
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_LEFT_CURLY_BRACKET, 
            (IRParserTokenType_t) IR_LEX_IDENTIFIER, 
            (IRParserTokenType_t) IR_LEX_RIGHT_CURLY_BRACKET, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_EXPRESSION,
        .context_rule = {IR_CR_KEEP, IR_CR_REPLACE, IR_CR_KEEP},
        .invert_match = {0},
        .context_length = 3,
        .priority = 1000, 
        .description = "{ identifier } -> { expression }",
        .variant = 3, 
    },
    // { NUMBER }  -> { EXPRESSION }
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_LEFT_CURLY_BRACKET, 
            (IRParserTokenType_t) IR_LEX_NUMBER, 
            (IRParserTokenType_t) IR_LEX_RIGHT_CURLY_BRACKET, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_EXPRESSION,
        .context_rule = {IR_CR_KEEP, IR_CR_REPLACE, IR_CR_KEEP},
        .invert_match = {0},
        .context_length = 3,
        .priority = 1000, 
        .description = "{ number } -> { expression }",
        .variant = 4, 
    },

    // IDENTIFIER -> EXPRESSION
    {
        .context = { 
            (IRParserTokenType_t) IR_LEX_IDENTIFIER, 
            IR_PAR_RULE_END 
        },
        .output = IR_PAR_EXPRESSION,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 10,
        .description = "identifier -> expression",
        .variant = 5, 
    }, 

    // NUMBER -> EXPRESSION
    {
        .context = { 
            (IRParserTokenType_t) IR_LEX_NUMBER, 
            IR_PAR_RULE_END 
        },
        .output = IR_PAR_EXPRESSION,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1,
        .description = "number -> expression",
        .variant = 6, 
    }, 

    // operator LABEL -> operator EXPRESSION
    {
        .context = { 
            (IRParserTokenType_t) IR_PAR_OPERATOR, 
            (IRParserTokenType_t) IR_LEX_LABEL, 
            IR_PAR_RULE_END 
        },
        .output = IR_PAR_EXPRESSION,
        .context_rule = {IR_CR_KEEP, IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 2,
        .priority = 1,
        .description = "operator label -> operator expression",
        .variant = 7,  
    }, 

    // operator LABEL -> operator EXPRESSION
    {
        .context = { 
            (IRParserTokenType_t) IR_LEX_STAR,  
            (IRParserTokenType_t) IR_LEX_LABEL, 
            IR_PAR_RULE_END 
        },
        .output = IR_PAR_EXPRESSION,
        .context_rule = {IR_CR_KEEP, IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 2,
        .priority = 1,
        .description = "* label -> * expression",
        .variant = 8, 
    }, 


    // L_VALUE operator R_VALUE ; -> STATEMENT
    {
        .context = {
            (IRParserTokenType_t) IR_PAR_EXPRESSION, 
            (IRParserTokenType_t) IR_PAR_OPERATOR,
            (IRParserTokenType_t) IR_PAR_EXPRESSION, 
            (IRParserTokenType_t) IR_LEX_SEMICOLON, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_STATEMENT,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD, IR_CR_DISCARD, IR_CR_DISCARD},
        .invert_match = {0},
        .context_length = 4,
        .priority = 1000, 
        .description = "expression operator expression -> statement",
        .variant = 1, 
    },

    // unary_operator expression ; -> STATEMENT
    {
        .context = {
            (IRParserTokenType_t) IR_PAR_UNARY_OPERATOR,
            (IRParserTokenType_t) IR_PAR_EXPRESSION, 
            (IRParserTokenType_t) IR_LEX_SEMICOLON, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_STATEMENT,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD, IR_CR_DISCARD},
        .invert_match = {0},
        .context_length = 3,
        .priority = 1000, 
        .description = "unary_operator expression -> statement",
        .variant = 2, 
    },

    // scopebegin ; -> SCOPEBEGIN
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_SCOPEBEGIN, 
            (IRParserTokenType_t) IR_LEX_SEMICOLON, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_SCOPEBEGIN,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD},
        .invert_match = {0},
        .context_length = 2,
        .priority = 10, 
        .description = "scopebegin ; -> scopebegin",
    },

    // scopebegin ; -> SCOPEBEGIN
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_SCOPEEND, 
            (IRParserTokenType_t) IR_LEX_SEMICOLON, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_SCOPEEND,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD},
        .invert_match = {0},
        .context_length = 2,
        .priority = 10, 
        .description = "scopeend ; -> scopeend",
    },

    // return ; -> RETURN
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_RETURN, 
            (IRParserTokenType_t) IR_LEX_SEMICOLON, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_RETURN,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD},
        .invert_match = {0},
        .context_length = 2,
        .priority = 1000, 
        .description = "return ; -> return",
    },

    // variable_declaration -> statement
    {
        .context = {
            (IRParserTokenType_t) IR_PAR_VARIABLE_DECLARATION, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_STATEMENT,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "variable_declaration -> statement",
        .variant = 3, 
    },

    // STATEMENT STATEMENT -> STATEMENT
    {
        .context = {
            (IRParserTokenType_t) IR_PAR_STATEMENT, 
            (IRParserTokenType_t) IR_PAR_STATEMENT,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_STATEMENT,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD},
        .invert_match = {0},
        .context_length = 2,
        .priority = 10, 
        .description = "statement statement -> statement",
        .variant = 4, 
    },

    // label SCOPEBEGIN STATEMENT SCOPEEND return -> FUNCTION_DEFINITION
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_LABEL, 
            (IRParserTokenType_t) IR_PAR_SCOPEBEGIN, 
            (IRParserTokenType_t) IR_PAR_STATEMENT,
            (IRParserTokenType_t) IR_PAR_SCOPEEND, 
            (IRParserTokenType_t) IR_PAR_RETURN,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_FUNCTION_DEFINITION,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD, IR_CR_DISCARD, IR_CR_DISCARD, IR_CR_DISCARD},
        .invert_match = {0},
        .context_length = 5,
        .priority = 10, 
        .description = "label scopebegin statement scopeend return -> function_definition",
    },

    // label SCOPEBEGIN SCOPEEND return -> FUNCTION_DEFINITION
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_LABEL, 
            (IRParserTokenType_t) IR_PAR_SCOPEBEGIN, 
            (IRParserTokenType_t) IR_PAR_SCOPEEND, 
            (IRParserTokenType_t) IR_PAR_RETURN,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_FUNCTION_DEFINITION,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD, IR_CR_DISCARD, IR_CR_DISCARD},
        .invert_match = {0},
        .context_length = 4,
        .priority = 10, 
        .description = "label scopebegin scopeend return -> function_definition",
    },



    // LEFT_SIDE_EXPRESSION = RIGHT_SIDE_EXPRESSION -> STATEMENT

    // operators like "i+="

    // i+=, l+=, f+=, d+=, i-=, ... -> OPERATOR
    {
        .context = {
            (IRParserTokenType_t) IR_PAR_ASSIGN, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "assign -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_INTEGER_PLUS_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "i+= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_UNSIGNED_INTEGER_PLUS_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "u+= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_SATURATED_INTEGER_PLUS_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "si+= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_SATURATED_UNSIGNED_INTEGER_PLUS_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "su+= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_FLOAT_PLUS_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "f+= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_DOUBLE_PLUS_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "d+= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_LONG_PLUS_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "l+= -> operator",
    },

    {
        .context = {
            (IRParserTokenType_t) IR_LEX_INTEGER_MINUS_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "i-= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_UNSIGNED_INTEGER_MINUS_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "u-= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_SATURATED_INTEGER_MINUS_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "si-= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_SATURATED_UNSIGNED_INTEGER_MINUS_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "su-= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_FLOAT_MINUS_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "f-= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_DOUBLE_MINUS_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "d-= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_LONG_MINUS_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "l-= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_INTEGER_STAR_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "i*= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_UNSIGNED_INTEGER_STAR_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "u*= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_SATURATED_INTEGER_STAR_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "si*= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_SATURATED_UNSIGNED_INTEGER_STAR_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "su*= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_FLOAT_STAR_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "f*= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_DOUBLE_STAR_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "d*= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_LONG_STAR_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "l*= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_INTEGER_SLASH_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "i/= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_UNSIGNED_INTEGER_SLASH_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "u/= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_FLOAT_SLASH_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "f/= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_DOUBLE_SLASH_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "d/= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_LONG_SLASH_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "l/= -> operator",
    },

    {
        .context = {
            (IRParserTokenType_t) IR_LEX_INTEGER_EQUAL_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "i== -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_FLOAT_EQUAL_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "f== -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_DOUBLE_EQUAL_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "d== -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_LONG_EQUAL_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "l== -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_INTEGER_BANG_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "i!= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_FLOAT_BANG_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "f!= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_DOUBLE_BANG_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "d!= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_LONG_BANG_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "l!= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_INTEGER_GREATER_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "i>= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_FLOAT_GREATER_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "f>= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_DOUBLE_GREATER_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "d>= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_LONG_GREATER_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "l>= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_INTEGER_LESS_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "i<= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_FLOAT_LESS_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "f<= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_DOUBLE_LESS_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "d<= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_LONG_LESS_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "l<= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_INTEGER_GREATER, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "i> -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_FLOAT_GREATER, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "f> -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_DOUBLE_GREATER, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "d> -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_LONG_GREATER, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "l> -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_INTEGER_LESS, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "i< -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_FLOAT_LESS, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "f< -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_DOUBLE_LESS, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "d< -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_LONG_LESS, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "l< -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_SHIFT_LEFT_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "<<= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_SHIFT_RIGHT_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = ">>= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_BITWISE_AND_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "&= -> operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_BITWISE_OR_EQUAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "|= -> operator",
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
        .priority = 1000, 
        .description = "cif -> unary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_CID, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_UNARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "cid -> unary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_CIL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_UNARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "cil -> unary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_CFI, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_UNARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "cfi -> unary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_CFD, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_UNARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "cfd -> unary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_CFL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_UNARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "cfl -> unary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_CDI, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_UNARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "cdi -> unary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_CDF, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_UNARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "cdf -> unary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_CDL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_UNARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "cdl -> unary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_CLI, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_UNARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "cli -> unary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_CLF, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_UNARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "clf -> unary_operator",
    },
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_CLD, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_UNARY_OPERATOR,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "cld -> unary_operator",
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
        .priority = 1000, 
        .description = "cbi -> unary_operator",
    },

    
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_REQUIRE, 
            (IRParserTokenType_t) IR_LEX_STRING, 
            (IRParserTokenType_t) IR_LEX_SEMICOLON, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_REQUIRE,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD, IR_CR_DISCARD},
        .invert_match = {0},
        .context_length = 3,
        .priority = 1000, 
        .description = "require string ; -> require",
    },

    
    {
        .context = {
            (IRParserTokenType_t) IR_PAR_REQUIRE, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_STATEMENT,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "require -> statement",
        .variant = 5, 
    },
    
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_PUSHARG, 
            (IRParserTokenType_t) IR_LEX_LEFT_CURLY_BRACKET, 
            (IRParserTokenType_t) IR_PAR_EXPRESSION, 
            (IRParserTokenType_t) IR_LEX_RIGHT_CURLY_BRACKET, 
            (IRParserTokenType_t) IR_PAR_EXPRESSION, 
            (IRParserTokenType_t) IR_LEX_SEMICOLON, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_PUSHARG,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD, IR_CR_DISCARD, IR_CR_DISCARD, IR_CR_DISCARD, IR_CR_DISCARD},
        .invert_match = {0},
        .context_length = 6,
        .priority = 1000, 
        .description = "callpusharg { expr } expr ; -> callpusharg",
    },
    
    {
        .context = {
            (IRParserTokenType_t) IR_PAR_PUSHARG, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_STATEMENT,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "callpusharg -> statement",
        .variant = 6, 
    },
    
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_FREEARG, 
            (IRParserTokenType_t) IR_LEX_LEFT_CURLY_BRACKET, 
            (IRParserTokenType_t) IR_PAR_EXPRESSION, 
            (IRParserTokenType_t) IR_LEX_RIGHT_CURLY_BRACKET, 
            (IRParserTokenType_t) IR_LEX_SEMICOLON, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_FREEARG,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD, IR_CR_DISCARD, IR_CR_DISCARD, IR_CR_DISCARD},
        .invert_match = {0},
        .context_length = 5,
        .priority = 1000, 
        .description = "callfreearg { expr } ; -> callfreearg",
    },
    
    {
        .context = {
            (IRParserTokenType_t) IR_PAR_FREEARG, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_STATEMENT,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "callfreearg -> statement",
        .variant = 7, 
    },
    
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_CALL, 
            (IRParserTokenType_t) IR_LEX_LABEL, 
            (IRParserTokenType_t) IR_LEX_SEMICOLON, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_EXPRESSION,
        .context_rule = {IR_CR_KEEP, IR_CR_REPLACE, IR_CR_KEEP},
        .invert_match = {0},
        .context_length = 3,
        .priority = 1000, 
        .description = "call label ; -> call expression ;",
        .variant = 9, 
    },
    
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_CALL, 
            (IRParserTokenType_t) IR_PAR_EXPRESSION, 
            (IRParserTokenType_t) IR_LEX_SEMICOLON, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_STATEMENT,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 3,
        .priority = 1000, 
        .description = "call expression ; -> call",
        .variant = 8, 
    },
    


    // perilogue -> function_modifier
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_PERILOGUE, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_FUNCTION_MODIFIER,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "perilogue -> function_modifier",
    },
    // atomic -> function_modifier
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_ATOMIC, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_FUNCTION_MODIFIER,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "atomic -> function_modifier",
    },
    // reentrant -> function_modifier
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_REENTRANT, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_FUNCTION_MODIFIER,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "reentrant -> function_modifier",
    },
    // interrupt { number } -> function_modifier
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_INTERRUPT, 
            (IRParserTokenType_t) IR_LEX_LEFT_CURLY_BRACKET, 
            (IRParserTokenType_t) IR_LEX_NUMBER, 
            (IRParserTokenType_t) IR_LEX_RIGHT_CURLY_BRACKET, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_FUNCTION_MODIFIER,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD, IR_CR_DISCARD, IR_CR_DISCARD},
        .invert_match = {0},
        .context_length = 4,
        .priority = 1000, 
        .description = "interrupt { number } -> function_modifier",
    },
    // local -> function_modifier
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_LOCAL, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_FUNCTION_MODIFIER,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "local -> function_modifier",
    },
    // align { number } -> function_modifier
    {
        .context = {
            (IRParserTokenType_t) IR_LEX_ALIGN, 
            (IRParserTokenType_t) IR_LEX_LEFT_CURLY_BRACKET, 
            (IRParserTokenType_t) IR_LEX_NUMBER, 
            (IRParserTokenType_t) IR_LEX_RIGHT_CURLY_BRACKET, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_FUNCTION_MODIFIER,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD, IR_CR_DISCARD, IR_CR_DISCARD},
        .invert_match = {0},
        .context_length = 4,
        .priority = 1000, 
        .description = "align { number } -> function_modifier",
    },


    // function_modifier function_modifier -> function_modifier
    {
        .context = {
            (IRParserTokenType_t) IR_PAR_FUNCTION_MODIFIER, 
            (IRParserTokenType_t) IR_PAR_FUNCTION_MODIFIER, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_FUNCTION_MODIFIER,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD},
        .invert_match = {0},
        .context_length = 2,
        .priority = 1000, 
        .description = "function_modifier function_modifier -> function_modifier",
    },

    {
        .context = {
            (IRParserTokenType_t) IR_PAR_FUNCTION_MODIFIER, 
            (IRParserTokenType_t) IR_PAR_FUNCTION_DEFINITION, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_FUNCTION_DEFINITION,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD},
        .invert_match = {0},
        .context_length = 2,
        .priority = 10, 
        .description = "function_modifier function_definition -> function_definition",
    },

    {
        .context = {
            (IRParserTokenType_t) IR_LEX_PARG, 
            IR_PAR_RULE_END
        },
        .output = IR_PAR_EXPRESSION,
        .context_rule = {IR_CR_REPLACE},
        .invert_match = {0},
        .context_length = 1,
        .priority = 1000, 
        .description = "__parg -> expression",
        .variant = 10, 
    },

    // function_definition function_definition -> function_definition
    /*
    {
        .context = {
            (IRParserTokenType_t) IR_PAR_FUNCTION_DEFINITION, 
            (IRParserTokenType_t) IR_PAR_FUNCTION_DEFINITION,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_FUNCTION_DEFINITION,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD},
        .invert_match = {0},
        .context_length = 2,
        .priority = 10, 
        .description = "function_definition function_definition -> function_definition",
    },

    // statement function_definition -> root
    {
        .context = {
            (IRParserTokenType_t) IR_PAR_STATEMENT, 
            (IRParserTokenType_t) IR_PAR_FUNCTION_DEFINITION,
            IR_PAR_RULE_END
        },
        .output = IR_PAR_ROOT,
        .context_rule = {IR_CR_REPLACE, IR_CR_DISCARD},
        .invert_match = {0},
        .context_length = 2,
        .priority = 0, 
        .description = "statement function_definition -> root",
    },
    */


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



