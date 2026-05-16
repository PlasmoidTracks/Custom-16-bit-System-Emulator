
#include <stdlib.h>
#include <string.h>

#include "utils/IO.h"

#include "utils/Log.h"

#include "compiler/ir/ir_token_name_table.h"
#include "compiler/ir/ir_lexer.h"
#include "compiler/ir/ir_parser.h"
#include "compiler/ir/ir_compiler.h"

#include "compiler/linker/linker.h"


void recurse_search_for_required_files(
    IRParserToken_t* parse, 
    char*** dependency_set, 
    int* dependency_file_count, 
    int* current_search_item_index
) {
    // check for IR_PAR_REQUIRE
    if ((IRParserTokenType_t) parse->token.type == IR_PAR_REQUIRE) {
        (*dependency_set) = realloc(*dependency_set, sizeof(char*) * ((*dependency_file_count) + 2));
        (*dependency_set)[(*dependency_file_count)] = malloc(strlen(parse->child[1]->token.raw) + 1);
        strcpy((*dependency_set)[(*dependency_file_count)], &parse->child[1]->token.raw[1]);
        (*dependency_set)[(*dependency_file_count)][strlen(parse->child[1]->token.raw) - 2] = '\0';
        // check for duplicates
        for (int i = 0; i < *dependency_file_count; i++) {
            if (strcmp((*dependency_set)[(*dependency_file_count)], (*dependency_set)[i]) == 0) {
                // reverte
                free((*dependency_set)[(*dependency_file_count)]);
                (*dependency_set)[(*dependency_file_count)] = NULL;
                return;
            }
        }
        (*dependency_file_count) += 1;
        (*dependency_set)[(*dependency_file_count)] = NULL;
        return;
    }
    // then recurse
    for (int i = 0; i < parse->child_count; i++) {
        recurse_search_for_required_files(parse->child[i], dependency_set, dependency_file_count, current_search_item_index);
    }
}


char** linker_build_dependency_set(char** source_files_ir) {

    int dependency_file_count = 0;
    char **current_file = &source_files_ir[0];

    while (*current_file) {
        dependency_file_count++; 
        current_file++;
    }
    log_msg(LP_DEBUG, "%d file(s) provided!", dependency_file_count);

    // initial set, which will also be used as the stack for the breadth first search approach
    char** dependency_set = malloc(sizeof(char*) * (dependency_file_count + 1));
    for (int i = 0; i < dependency_file_count; i++) {
        dependency_set[i] = malloc(strlen(source_files_ir[i] + 1));
        strcpy(dependency_set[i], source_files_ir[i]);
    }
    dependency_set[dependency_file_count] = NULL;

    int current_search_item_index = 0;
    
    while (current_search_item_index < dependency_file_count) {
        char* current_search_item = dependency_set[current_search_item_index];
        log_msg(LP_DEBUG, "current_search_item: \"%s\"", current_search_item);
        // TODO: parse file and append imports to dependency_set
        
        long content_size;
        char* content = read_file(dependency_set[current_search_item_index], &content_size);
        if (!content) {
            log_msg(LP_ERROR, "IR: read_file failed [%s:%d]", __FILE__, __LINE__);
            return NULL;
        }

        long parser_root_count;
        IRParserToken_t** parse = ir_parser_parse(content, content_size, &parser_root_count);
        if (!parse) {
            log_msg(LP_ERROR, "parser returned NULL [%s:%d]", __FILE__, __LINE__);
            return NULL;
        }

        for (int i = 0; i < parser_root_count; i++) {
            recurse_search_for_required_files(parse[i], &dependency_set, &dependency_file_count, &current_search_item_index);
        }

        current_search_item_index ++;
    }

    return dependency_set;
}

