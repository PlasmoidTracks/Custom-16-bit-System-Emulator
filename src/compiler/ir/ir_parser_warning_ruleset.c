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



