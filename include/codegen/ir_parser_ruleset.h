#ifndef _IR_PARSER_RULESET_H_
#define _IR_PARSER_RULESET_H_

#include "codegen/ir_parser.h"

#define IR_PAR_RULE_END ((IRParserTokenType_t)(-1))

typedef enum {
    IR_CR_DISCARD,     // will discard the context token
    IR_CR_REPLACE,     // will replace this token with the output
    IR_CR_KEEP,        // will keep this token around for context
} IRContextRule_t;

typedef int IRRulePriority_t;

typedef struct IRGrammarRule_t {
    IRParserTokenType_t context[8];  // NULL terminated context
    IRParserTokenType_t output;
    IRContextRule_t context_rule[8];              // set index to 1 if the n-th token should be kept untouched
    int invert_match[8];
    IRRulePriority_t priority;                   // for precedence
    int context_length;
    const char* description;
} IRGrammarRule_t;

extern IRGrammarRule_t parser_ruleset[256];

#endif
