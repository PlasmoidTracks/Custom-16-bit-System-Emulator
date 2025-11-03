#ifndef _IRA_PARSER_RULESET_H_
#define _IRA_PARSER_RULESET_H_

#include "transpile/ira_parser.h"

#define IRA_PAR_RULE_END ((IRAParserTokenType_t)(-1))

typedef enum {
    IRA_CR_DISCARD,     // will discard the context token
    IRA_CR_REPLACE,     // will replace this token with the output
    IRA_CR_KEEP,        // will keep this token around for context
} IRAContextRule_t;

typedef int IRARulePriority_t;

typedef struct IRAGrammarRule_t {
    IRAParserTokenType_t context[8];  // NULL terminated context
    IRAParserTokenType_t output;
    IRAContextRule_t context_rule[8];              // set index to 1 if the n-th token should be kept untouched
    int invert_match[8];
    IRARulePriority_t priority;                   // for precedence
    int context_length;
    const char* description;
} IRAGrammarRule_t;

extern IRAGrammarRule_t ira_parser_ruleset[256];

#endif
