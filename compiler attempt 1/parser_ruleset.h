#ifndef _PARSER_RULESET_H_
#define _PARSER_RULESET_H_

#include "parser.h"

#define PAR_RULE_END ((ParserTokenType_t)(-1))

typedef enum {
    CR_DISCARD,     // will discard the context token
    CR_REPLACE,     // will replace this token with the output
    CR_KEEP,        // will keep this token around for context
} ContextRule_t;

/*typedef enum {
    PRIO_LITERAL         = 1,  // Numbers, identifiers
    PRIO_GROUPING        = 2,  // ( expr )
    PRIO_UNARY           = 3,  // -x, !x
    PRIO_BINARY_ADD_SUB  = 4,  // +, -
    PRIO_BINARY_MUL_DIV  = 5,  // *, /
    PRIO_BINARY_COMPARE  = 6,  // <, >
    PRIO_BINARY_EQUALITY = 7,  // ==, !=
    PRIO_LOGICAL         = 8,  // &&, ||
    PRIO_ASSIGNMENT      = 9,  // =
    PRIO_STATEMENT       = 10, // return, break, etc.
    PRIO_DECLARATION     = 11, // var, const, typedef
    PRIO_FUNC_DEF        = 12, // fn () {}
} RulePriority_t;*/

typedef int RulePriority_t;

typedef struct GrammarRule_t {
    ParserTokenType_t context[8];  // NULL terminated context
    ParserTokenType_t output;
    ContextRule_t context_rule[8];              // set index to 1 if the n-th token should be kept untouched
    int invert_match[8];
    RulePriority_t priority;                   // for precedence
    int context_length;
    const char* description;
} GrammarRule_t;

extern GrammarRule_t parser_ruleset[256];

#endif
