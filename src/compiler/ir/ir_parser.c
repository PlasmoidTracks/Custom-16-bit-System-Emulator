#include <stdlib.h>
#include <stdio.h>

#include "compiler/ir/ir_parser_warning_ruleset.h"
#include "utils/Log.h"

#include "compiler/ir/ir_lexer.h"
#include "compiler/ir/ir_parser.h"
#include "compiler/ir/ir_parser_ruleset.h"

#define IR_PARSER_DEBUG
#undef IR_PARSER_DEBUG

#ifdef IR_PARSER_DEBUG
    #include "compiler/ir/ir_token_name_table.h"
    static void ir_recursion(IRParserToken_t* parser_token, int depth) {
        //if (depth > 0) return;
        for (int i = 0; i < depth; i++) {
            printf("    ");
        }
        printf("%s, \"%s\"\n", ir_token_name[parser_token->token.type], parser_token->token.raw);
        for (int i = 0; i < parser_token->child_count; i++) {
            ir_recursion(parser_token->child[i], depth + 1);
        }
    }
#endif // IR_PARSER_DEBUG

static void ir_recursion_print(IRParserToken_t* parser_token, int depth) {
    //if (depth > 0) return;
    if (parser_token->child_count == 0) {
        printf("%s ", parser_token->token.raw);
    }
    for (int i = 0; i < parser_token->child_count; i++) {
        ir_recursion_print(parser_token->child[i], depth + 1);
    }
}

void ir_print_token(IRParserToken_t* parser_token) {
    if (parser_token->child_count == 0) {
        //printf("%s ", ir_token_name[parser_token->token.type]);
        printf("%s ", parser_token->token.raw);
        return;
    }
    for (int i = 0; i < parser_token->child_count; i++) {
        ir_print_token(parser_token->child[i]);
    }
}


IRParserToken_t** ir_parser_parse(char* source, long source_length, long* parser_root_count) {
    // 1) Convert all lexer_token -> parser_token array
    long lexer_token_count;
    IRLexerToken_t* lexer_token = ir_lexer_parse(source, source_length, &lexer_token_count);
    if (!lexer_token) {
        log_msg(LP_WARNING, "IR: Lexer returned NULL [%s:%d]", __FILE__, __LINE__);
        //return NULL;
    }
    
    IRParserToken_t** parser_token = malloc(sizeof(IRParserToken_t*) * lexer_token_count);
    for (int i = 0; i < lexer_token_count; i++) {
        parser_token[i] = malloc(sizeof(IRParserToken_t));
        *parser_token[i] = (IRParserToken_t){
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
        int best_rule_index = -1;        // index in parser_ruleset
        int best_rule_startpos = -1;     // where it matches in parser_token
        int best_rule_priority = -1;

        // 3) Search all token positions
        for (int start = 0; start < lexer_token_count; start++) {
            // 4) For each position, try every rule
            for (int rule = 0; rule < 256; rule++) {
                if (ir_parser_ruleset[rule].context[0] == IR_PAR_RULE_END)
                    break; // No more rules

                int context_len = ir_parser_ruleset[rule].context_length;
                if (start + context_len > lexer_token_count) 
                    continue; // Not enough tokens for this rule

                // Check match (including invert_match)
                int match = 1;
                for (int c = 0; c < context_len; c++) {
                    IRParserTokenType_t expected = ir_parser_ruleset[rule].context[c];
                    IRParserTokenType_t actual   = (IRParserTokenType_t) parser_token[start + c]->token.type;
                    int invert = ir_parser_ruleset[rule].invert_match[c];

                    if ((actual == expected && invert) || (actual != expected && !invert)) {
                        match = 0;
                        break;
                    }
                }

                if (!match) 
                    continue;

                // If we get here, we have a match.
                int this_priority = ir_parser_ruleset[rule].priority;
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
            #ifdef IR_PARSER_DEBUG
            log_msg(LP_SUCCESS, "Applying highest-priority rule: %s  (priority=%d, start=%d)",
                ir_parser_ruleset[best_rule_index].description,
                best_rule_priority,
                best_rule_startpos
            );
            #endif

            // Create the new parser token node
            IRParserToken_t* new_token = malloc(sizeof(IRParserToken_t));
            *new_token = (IRParserToken_t){
                .token = {
                    .type      = (IRLexerTokenType_t) ir_parser_ruleset[best_rule_index].output,
                    .raw       = ir_parser_ruleset[best_rule_index].description,
                    .line      = -1,
                    .column    = -1,
                    .index     = -1,
                    .length    = -1,
                },
                .parent     = NULL,
                .child_count = 0,
            };

            // Fill the children according to IR_CR_REPLACE / IR_CR_DISCARD / IR_CR_KEEP
            int context_len = ir_parser_ruleset[best_rule_index].context_length;
            for (int c = 0; c < context_len; c++) {
                IRContextRule_t mode = ir_parser_ruleset[best_rule_index].context_rule[c];
                // Only attach them if it's REPLACE or DISCARD 
                // (That's how you've been building partial subtrees.)
                if (mode == IR_CR_REPLACE || mode == IR_CR_DISCARD) {
                    new_token->child[new_token->child_count++] = parser_token[best_rule_startpos + c];
                }
                // IR_CR_KEEP is reinserted as a top-level token later
            }

            // Rebuild the token list
            IRParserToken_t** new_token_list = malloc(sizeof(IRParserToken_t*) * lexer_token_count);
            int new_count = 0;

            // Copy everything up to startpos
            for (int i = 0; i < best_rule_startpos; i++) {
                new_token_list[new_count++] = parser_token[i];
            }

            // Insert: for each token in the matched range
            for (int c = 0; c < context_len; c++) {
                IRContextRule_t mode = ir_parser_ruleset[best_rule_index].context_rule[c];
                if (mode == IR_CR_REPLACE) {
                    // Insert the new token node just once
                    new_token_list[new_count++] = new_token;
                } else if (mode == IR_CR_KEEP) {
                    // Keep the original token
                    new_token_list[new_count++] = parser_token[best_rule_startpos + c];
                }
                // IR_CR_DISCARD => skip
            }

            // Copy everything after the matched range
            for (int i = best_rule_startpos + context_len; i < lexer_token_count; i++) {
                new_token_list[new_count++] = parser_token[i];
            }

            free(parser_token);
            parser_token       = new_token_list;
            lexer_token_count  = new_count;

        } else {

            // No rule found => no more matches
            // Check for matching warning ruleset
            for (int start = 0; start < lexer_token_count; start++) {
                // 4) For each position, try every rule
                for (int rule = 0; rule < 256; rule++) {
                    if (ir_parser_warning_ruleset[rule].context[0] == IR_PAR_RULE_END)
                        break; // No more rules

                    int context_len = ir_parser_warning_ruleset[rule].context_length;
                    if (start + context_len > lexer_token_count) 
                        continue; // Not enough tokens for this rule

                    // Check match (including invert_match)
                    int match = 1;
                    for (int c = 0; c < context_len; c++) {
                        IRParserTokenType_t expected = ir_parser_warning_ruleset[rule].context[c];
                        IRParserTokenType_t actual   = (IRParserTokenType_t) parser_token[start + c]->token.type;
                        int invert = ir_parser_warning_ruleset[rule].invert_match[c];

                        if ((actual == expected && invert) || (actual != expected && !invert)) {
                            match = 0;
                            break;
                        }
                    }

                    if (!match) 
                        continue;

                    // If we get here, we have a match.
                    log_msg(LP_ERROR, "IR Parser: Matching warning at line %d:%d [%s:%d]", parser_token[start]->token.line, parser_token[start]->token.column, __FILE__, __LINE__);
                    log_msg(LP_INFO, ir_parser_warning_ruleset[rule].description);
                    for (int i = start; i < ((start + context_len >= lexer_token_count) ? (lexer_token_count - 1) : (start + context_len - 1)); i++) {
                        if (i == start) {
                            printf("%-4d : \"", parser_token[i]->token.line);
                        }
                        ir_recursion_print(parser_token[i], 0);
                    }
                    printf("\"\n");
                    #ifdef IR_PARSER_DEBUG
                        for (int i = 0; i < lexer_token_count; i++) {
                            log_msg(LP_INFO, "AST root %d", i);
                            ir_recursion(parser_token[i], 0);
                        }
                    #endif //IR_PARSER_DEBUG
                    return NULL;
                }
            }
            break;
        }
    }

    if (parser_root_count) {*parser_root_count = lexer_token_count;}

    // 6) Done reducing. Let's display the final AST forest.
    #ifdef IR_PARSER_DEBUG
        for (int i = 0; i < lexer_token_count; i++) {
            log_msg(LP_INFO, "AST root %d", i);
            ir_recursion(parser_token[i], 0);
        }
    #endif //IR_PARSER_DEBUG

    return parser_token;
}


