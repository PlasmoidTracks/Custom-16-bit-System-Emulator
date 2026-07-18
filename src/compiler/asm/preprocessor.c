

#include "utils/Log.h"
#include "utils/IO.h"
#include "utils/String.h"
#include <stdlib.h>
#include <string.h>
#include <utils/String.h>

#include "compiler/asm/preprocessor.h"

DefinitionMap_t* map = NULL;
int map_capacity = 0;
int map_length = 0;

char** assembly_preprocessor_split_to_tokens(const char text[]) {
    char** lines = split(text, " \n,[]+", "\n[]+");
    free((void*) text);
    return lines;
}

char* assembly_preprocessor_compile(const char* content) {
    char* output = malloc(128);         // final assembly string
    long output_len = 0;         // current length of code output

    // split text into lines
    char** token = assembly_preprocessor_split_to_tokens(content);
    long token_count = 0;

    while (token[++token_count]);

    // collect definition maps
    for (int i = 0; i < token_count; i++) {
        if (strcmp(token[i], "#define") == 0 && i < token_count - 2) {
            // add replacement to map
            if (map_length <= map_capacity) {
                map = realloc(map, sizeof(DefinitionMap_t) * (map_capacity * 2 + 1));
                map_capacity = map_capacity * 2 + 1;
            }
            strncpy(map[map_length].match, token[i + 1], ASM_PREPROCESSOR_CAPACITY);
            strncpy(map[map_length].replacement, token[i + 2], ASM_PREPROCESSOR_CAPACITY);
            map_length ++;

            // delete definition tokens from list
            free(token[i]);
            free(token[i + 1]);
            free(token[i + 2]);
            for (int j = i; j < token_count - 3; j++) {
                token[j] = token[j + 3];
            }
            token_count -= 3;
            i --;
        }
    }

    // replace with definition map
    for (int i = 0; i < token_count; i++) {
        int replacement = 1;
        while (replacement) {
            replacement = 0;
            for (int l = 0; l < map_length; l++) {
                if (strcmp(token[i], map[l].match) == 0) {
                    token[i] = realloc(token[i], strlen(map[l].replacement) + 1);
                    strcpy(token[i], map[l].replacement);
                    replacement = 1;
                }
            }
        }
    }

    // reconstruct
    for (int i = 0; i < token_count; i++) {
        output = append_to_output(output, &output_len, token[i]);
        if (strcmp(token[i], "\n") != 0) {
            output = append_to_output(output, &output_len, " ");
        }
    }

    return output;
}

char* assembly_preprocessor_compile_from_file(const char* filename) {
    // load raw text from file
    char* content = read_file(filename, NULL);
    if (!content) {
        log_msg(LP_ERROR, "Preprocessor: read_file failed [%s:%d]", __FILE__, __LINE__);
        return NULL;
    }
    char* asm = assembly_preprocessor_compile(content);
    
    return asm;
}
