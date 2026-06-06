#include <stdio.h>

#include "utils/IO.h"

#include "compiler/asm/disassembler.h"
#include "compiler/asm/assembler.h"

#include "cpu/cpu_instructions.h"


char* transpile(uint8_t* bin, long binary_size) {

    char* content = disassembler_decompile(bin, binary_size, NULL, 0, DO_ADD_JUMP_LABEL);

    // split text into lines
    char** line = assembler_split_to_separate_lines(content);

    // remove comments from lines
    assembler_remove_comments(&line);

    // split into separate words
    int word_count;
    char** word = assembler_split_to_words(line, &word_count);

    // tokenize
    int token_count = 0;
    Token_t* token = assembler_parse_words(word, word_count, &token_count);

    // build expression array
    int expression_count = 0;
    Expression_t* expression = assembler_parse_token(token, token_count, &expression_count);

    // ToDo, resolve all include and incbin directives, then restart. No, lol

    // build instruction array
    int instruction_count = 0;
    Instruction_t* instruction = assembler_parse_expression(expression, expression_count, &instruction_count, NULL, NULL, AO_ERROR_ON_OVERLAP | AO_ERROR_ON_CODE_SEGMENT_BREACH);

    // resolve label
    instruction = assembler_resolve_labels(instruction, instruction_count);

    for (int i = 0; i < instruction_count; i++) {
        printf("%s\n", instruction_encoding[instruction[i].instruction].instruction_string);
    }


    char* c_code = NULL;
    


    return NULL;
}


char* transpile_from_file(const char* const filename) {
    long binary_size = 0;
    uint8_t* bin = (uint8_t*)read_file(filename, &binary_size);
    char* reconstruction = transpile(bin, binary_size);
    return reconstruction;
}


