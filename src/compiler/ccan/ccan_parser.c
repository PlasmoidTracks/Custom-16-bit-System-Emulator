#include <stdlib.h>
#include <stdio.h>

#include "utils/Log.h"

#include "compiler/ccan/ccan_lexer.h"
#include "compiler/ccan/ccan_parser.h"
#include "compiler/ccan/ccan_parser_ruleset.h"
#include "compiler/ccan/ccan_parser_warning_ruleset.h"
#include "compiler/ccan/ccan_token_name_table.h"

void ccan_recursion(CCANParserToken_t* parser_token, int depth) {
    //if (depth > 0) return;
    for (int i = 0; i < depth; i++) {
        printf("    ");
    }
    printf("%s, \"%s\"\n", ccan_token_name[parser_token->token.type], parser_token->token.raw);
    for (int i = 0; i < parser_token->child_count; i++) {
        ccan_recursion(parser_token->child[i], depth + 1);
    }
}

void ccan_print_token(CCANParserToken_t* parser_token) {
    if (parser_token->child_count == 0) {
        //printf("%s ", ccan_token_name[parser_token->token.type]);
        printf("%s ", parser_token->token.raw);
        return;
    }
    for (int i = 0; i < parser_token->child_count; i++) {
        ccan_print_token(parser_token->child[i]);
    }
}


CCANParserToken_t** ccan_parser_parse(CCANLexerToken_t* lexer_token, long lexer_token_count, long* parser_root_count) {
    // 1) Convert all lexer_token -> parser_token array
    CCANParserToken_t** parser_token = malloc(sizeof(CCANParserToken_t*) * lexer_token_count);
    for (int i = 0; i < lexer_token_count; i++) {
        parser_token[i] = malloc(sizeof(CCANParserToken_t));
        *parser_token[i] = (CCANParserToken_t){
            .token = lexer_token[i],
            .child_count = 0,
            .parent = NULL,
        };
    }


    // 2) Loop until no rule can be applied
    int something_changed = 1;
    while (something_changed) {

        something_changed = 0;

        // We'll search for the single best match across the entire token list
        int best_rule_index = -1;        // index in ccan_parser_ruleset
        int best_rule_startpos = -1;     // where it matches in parser_token
        int best_rule_priority = -1;

        // 3) Search all token positions
        for (int start = 0; start < lexer_token_count; start++) {
            // 4) For each position, try every rule
            for (int rule = 0; rule < 256; rule++) {
                if (ccan_parser_ruleset[rule].context[0] == CCAN_PAR_RULE_END)
                    break; // No more rules

                int context_len = ccan_parser_ruleset[rule].context_length;
                if (start + context_len > lexer_token_count) 
                    continue; // Not enough tokens for this rule

                // Check match (including invert_match)
                int match = 1;
                for (int c = 0; c < context_len; c++) {
                    CCANParserTokenType_t expected = ccan_parser_ruleset[rule].context[c];
                    CCANParserTokenType_t actual   = (CCANParserTokenType_t) parser_token[start + c]->token.type;
                    int invert = ccan_parser_ruleset[rule].invert_match[c];

                    if ((actual == expected && invert) || (actual != expected && !invert)) {
                        match = 0;
                        break;
                    }
                }

                if (!match) 
                    continue;

                // If we get here, we have a match.
                int this_priority = ccan_parser_ruleset[rule].priority;
                if (this_priority > best_rule_priority) {
                    best_rule_priority = this_priority;
                    best_rule_index    = rule;
                    best_rule_startpos = start;
                }
            }
        }

        // 5) If we found a rule to apply, apply it. Otherwise, we are done.
        if (best_rule_index >= 0) {
            something_changed = 1;
            log_msg(LP_SUCCESS, "Applying highest-priority rule: %s  (priority=%d, start=%d)",
                    ccan_parser_ruleset[best_rule_index].description,
                    best_rule_priority,
                    best_rule_startpos);

            // Create the new parser token node
            CCANParserToken_t* new_token = malloc(sizeof(CCANParserToken_t));
            *new_token = (CCANParserToken_t){
                .token = {
                    .type      = (CCANLexerTokenType_t) ccan_parser_ruleset[best_rule_index].output,
                    .raw       = ccan_parser_ruleset[best_rule_index].description,
                    .line      = -1,
                    .column    = -1,
                    .index     = -1,
                    .length    = -1,
                },
                .parent     = NULL,
                .child_count = 0,
            };

            // Fill the children according to CCAN_CR_REPLACE / CCAN_CR_DISCARD / CCAN_CR_KEEP
            int context_len = ccan_parser_ruleset[best_rule_index].context_length;
            for (int c = 0; c < context_len; c++) {
                CCANContextRule_t mode = ccan_parser_ruleset[best_rule_index].context_rule[c];
                // Only attach them if it's REPLACE or DISCARD 
                // (That's how you've been building partial subtrees.)
                if (mode == CCAN_CR_REPLACE || mode == CCAN_CR_DISCARD) {
                    new_token->child[new_token->child_count++] = parser_token[best_rule_startpos + c];
                }
                // CCAN_CR_KEEP is reinserted as a top-level token later
            }

            // Rebuild the token list
            CCANParserToken_t** new_token_list = malloc(sizeof(CCANParserToken_t*) * lexer_token_count);
            int new_count = 0;

            // Copy everything up to startpos
            for (int i = 0; i < best_rule_startpos; i++) {
                new_token_list[new_count++] = parser_token[i];
            }

            // Insert: for each token in the matched range
            for (int c = 0; c < context_len; c++) {
                CCANContextRule_t mode = ccan_parser_ruleset[best_rule_index].context_rule[c];
                if (mode == CCAN_CR_REPLACE) {
                    // Insert the new token node just once
                    new_token_list[new_count++] = new_token;
                } else if (mode == CCAN_CR_KEEP) {
                    // Keep the original token
                    new_token_list[new_count++] = parser_token[best_rule_startpos + c];
                }
                // CCAN_CR_DISCARD => skip
            }

            // Copy everything after the matched range
            for (int i = best_rule_startpos + context_len; i < lexer_token_count; i++) {
                new_token_list[new_count++] = parser_token[i];
            }

            free(parser_token);
            parser_token       = new_token_list;
            lexer_token_count  = new_count;
        }
        else {
            // No rule found => no more matches
            break;
        }
    }

    if (parser_root_count) {*parser_root_count = lexer_token_count;}

    // 6) Done reducing. Let's display the final AST forest.
    for (int i = 0; i < lexer_token_count; i++) {
        //log_msg(LP_INFO, "AST root %d", i);
        ccan_recursion(parser_token[i], 0);
    }

    return parser_token;
}


