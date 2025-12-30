#ifndef _CCAN_PARSER_RULESET_H_
#define _CCAN_PARSER_RULESET_H_

#include "compiler/ccan/ccan_parser.h"

#define CCAN_PAR_RULE_END ((CCANParserTokenType_t)(-1))

typedef enum {
    CCAN_CR_DISCARD,     // will discard the context token
    CCAN_CR_REPLACE,     // will replace this token with the output
    CCAN_CR_KEEP,        // will keep this token around for context
} CCANContextRule_t;

typedef int CCANRulePriority_t;

typedef struct CCANGrammarRule_t {
    CCANParserTokenType_t context[8];  // NULL terminated context
    CCANParserTokenType_t output;
    CCANContextRule_t context_rule[8];              // set index to 1 if the n-th token should be kept untouched
    int invert_match[8];
    CCANRulePriority_t priority;                   // for precedence
    int context_length;
    const char* description;
} CCANGrammarRule_t;

extern CCANGrammarRule_t ccan_parser_ruleset[256];

#endif
