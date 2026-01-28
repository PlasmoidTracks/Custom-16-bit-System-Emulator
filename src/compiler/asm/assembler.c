#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "utils/Log.h"
#include "utils/IO.h"
#include "utils/String.h"

#include "cpu/cpu_addressing_modes.h"
#include "cpu/cpu_instructions.h"

#include "globals/memory_layout.h"

#include "compiler/asm/assembler.h"

Label_t jump_label[MAX_LABELS];
Label_t const_label[MAX_LABELS];

int jump_label_index = 0;
int const_label_index = 0;


const char* token_type_string[] = {
    [TT_INSTRUCTION]                = "instruction",
    [TT_REGISTER]                   = "register",
    [TT_IMMEDIATE]                  = "immediate",
    [TT_PLUS]                       = "plus",
    [TT_STAR]                       = "star",
    [TT_INDIRECT_IMMEDIATE]         = "indirect_immediate",
    [TT_INDIRECT_REGISTER]          = "indirect_register",
    [TT_INDIRECT_REGISTER_OFFSET]   = "indirect_register_offset",
    [TT_INDIRECT_SCALE_OFFSET]      = "indirect_scale_offset",
    [TT_BRACKET_OPEN]               = "bracket_open",
    [TT_BRACKET_CLOSE]              = "bracket_close", 
    [TT_LABEL]                      = "label",
    [TT_ADDRESS]                    = "address",
    [TT_SEGMENT_CODE]               = "segment_code",
    [TT_SEGMENT_DATA]               = "segment_data",
    [TT_RESERVE]                    = "reserve",
    [TT_INCLUDE]                    = "include",
    [TT_INCBIN]                     = "incbin",
    [TT_TEXT]                       = "text",
    [TT_STRING]                     = "string",
    [TT_NOCACHE]                    = "nocache"
};

const char* expression_type_string[] = {
    [EXPR_NONE]                     = "none",
    [EXPR_INSTRUCTION]              = "instruction",
    [EXPR_REGISTER]                 = "register",
    [EXPR_IMMEDIATE]                = "immediate",
    [EXPR_INDIRECT_IMMEDIATE]       = "indirect_immediate",
    [EXPR_INDIRECT_REGISTER]        = "indirect_register",
    [EXPR_INDIRECT_REGISTER_OFFSET] = "indirect_register_offset",
    [EXPR_INDIRECT_SCALE_OFFSET]    = "indirect_scale_offset", 
    [EXPR_SEGMENT_DATA]             = "segment_data", 
    [EXPR_SEGMENT_CODE]             = "segment_code", 
    [EXPR_RESERVE]                  = "reserve", 
    [EXPR_INCBIN]                   = "incbin", 
    [EXPR_ADDRESS]                  = "address", 
    [EXPR_TEXT_DEFINITION]          = "text definition", 
};



char** assembler_split_to_separate_lines(const char text[]) {
    char** lines = split(text, "\n", "");
    free((void*) text);
    return lines;
}

void assembler_remove_comments(char **text[]) { // argument is pointer to text
    int index = 0;
    while ((*text)[index]) {
        unsigned long length = strlen((*text)[index]);
        unsigned long character_index = 0;
        while (character_index < length) {
            if ((*text)[index][character_index] == ';') break;
            character_index ++;
        }
        // here if either ';' found, or end of line
        // now delete all following characters by nulling them
        while (character_index < length) {
            (*text)[index][character_index] = '\0';
            character_index ++;
        }
        index ++;
    }
}

char** assembler_split_to_words(char* lines[], int* token_count) {
    int token_string_index_allocated = 8;
    char** token_string = calloc(token_string_index_allocated, sizeof(char*));
    int count = 0;
    int index = 0;
    while (lines[index]) {
        char** line_token_string = split(lines[index], "\t, []%", "[]%");
        free(lines[index]);
        int index2 = 0;
        while (line_token_string[index2]) {
            if (count >= token_string_index_allocated) {
                token_string_index_allocated *= 2;
                token_string = realloc(token_string, sizeof(char*) * token_string_index_allocated);
                if (!token_string) {
                    fprintf(stderr, "Memory allocation failed\n");
                    exit(1);
                }
            }
            token_string[count] = line_token_string[index2];
            (count)++;
            index2++;
        }
        free(line_token_string);
        index++;
    }
    *token_count = count;

    if (count >= token_string_index_allocated) {
        token_string_index_allocated *= 2;
        token_string = realloc(token_string, sizeof(char*) * token_string_index_allocated);
        if (!token_string) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }
    }
    token_string[count] = NULL;

    free(lines);

    return token_string;
}



Token_t* assembler_parse_words(char** word, int word_count, int* token_count) {
    Token_t* token = calloc(word_count, sizeof(Token_t));
    for (int i = 0; i < word_count; i++) {
        token[i].raw = word[i];
        token[i].type = -1;
        if (word[i][0] == '.') {
            if (strcmp(&word[i][1], "address") == 0) {
                token[i].type = TT_ADDRESS;
            } else if (strcmp(&word[i][1], "code") == 0) {
                token[i].type = TT_SEGMENT_CODE;
            } else if (strcmp(&word[i][1], "data") == 0) {
                token[i].type = TT_SEGMENT_DATA;
            } else if (strcmp(&word[i][1], "include") == 0) {
                token[i].type = TT_INCLUDE;
            } else if (strcmp(&word[i][1], "incbin") == 0) {
                token[i].type = TT_INCBIN;
            } else if (strcmp(&word[i][1], "text") == 0) {
                token[i].type = TT_TEXT;
            } else if (strcmp(&word[i][1], "reserve") == 0) {
                token[i].type = TT_RESERVE;
            } else {
                token[i].type = TT_LABEL;
            }
        } else if (word[i][0] == '[') {
            token[i].type = TT_BRACKET_OPEN;
        } else if (word[i][0] == ']') {
            token[i].type = TT_BRACKET_CLOSE;
        } else if (string_is_asm_immediate(word[i]) || word[i][0] == '_') {
            token[i].type = TT_IMMEDIATE;
        } else if (word[i][0] == '+') {
            token[i].type = TT_PLUS;
        } else if (word[i][0] == '*') {
            token[i].type = TT_STAR;
        } else if (word[i][0] == 'r' && contains(word[i][1], "0123")) {
            token[i].type = TT_REGISTER;
        } else if (word[i][0] == 's' && word[i][1] == 'p') {
            token[i].type = TT_REGISTER;
        } else if (word[i][0] == 'f' && word[i][1] == 'p') {
            token[i].type = TT_REGISTER;
        } else if (word[i][0] == 'p' && word[i][1] == 'c') {
            token[i].type = TT_REGISTER;
        } else if (string_is_string(word[i])) {
            token[i].type = TT_STRING;
        } else if (word[i][0] == '%') {
            token[i].type = TT_NOCACHE;
        } else {
            token[i].type = TT_INSTRUCTION; // TODO: check if its really part of the mnemonic vocab
        }
        //free(word[i]);
    }
    //free(word);

    if (token_count) {
        *token_count = word_count;
    }

    return token;
}

Expression_t* assembler_parse_token(Token_t* tokens, int token_count, int* expression_count) {
    Expression_t* expression = calloc(token_count, sizeof(Expression_t));
    
    int token_index = 0;
    int expression_index = 0;
    while (token_index < token_count) {
        // .label + $ffff
        if (token_index + 2 < token_count &&
            tokens[token_index + 0].type == TT_LABEL &&
            tokens[token_index + 1].type == TT_PLUS &&
            tokens[token_index + 2].type == TT_IMMEDIATE) {

                expression[expression_index].type = EXPR_IMMEDIATE;
                for (int j = 0; j < 3; j++) {
                    expression[expression_index].tokens[j] = tokens[token_index + j];
                }
                expression[expression_index].token_count = 3;
                token_index += 3;
                expression_index ++;
        }

        // $ffff
        if (token_index + 0 < token_count &&
            (tokens[token_index + 0].type == TT_IMMEDIATE || tokens[token_index + 0].type == TT_LABEL)) {

                expression[expression_index].type = EXPR_IMMEDIATE;
                expression[expression_index].tokens[0] = tokens[token_index];
                expression[expression_index].token_count = 1;
                token_index += 1;
                expression_index ++;
        }

        // reg
        else if (token_index + 0 < token_count &&
            tokens[token_index + 0].type == TT_REGISTER) {

                expression[expression_index].type = EXPR_REGISTER;
                expression[expression_index].tokens[0] = tokens[token_index];
                expression[expression_index].token_count = 1;
                token_index += 1;
                expression_index ++;
        }

        // [ $ffff ]
        else if (token_index + 2 < token_count &&
            tokens[token_index + 0].type == TT_BRACKET_OPEN &&
            (tokens[token_index + 1].type == TT_IMMEDIATE || tokens[token_index + 1].type == TT_LABEL) &&
            tokens[token_index + 2].type == TT_BRACKET_CLOSE) {

                expression[expression_index].type = EXPR_INDIRECT_IMMEDIATE;
                for (int j = 0; j < 3; j++) {
                    expression[expression_index].tokens[j] = tokens[token_index + j];
                }
                expression[expression_index].token_count = 3;
                expression_index ++;
                token_index += 3;
        }

        // [ .label + $ffff ]
        else if (token_index + 4 < token_count &&
            tokens[token_index + 0].type == TT_BRACKET_OPEN &&
            tokens[token_index + 1].type == TT_LABEL &&
            tokens[token_index + 2].type == TT_PLUS &&
            tokens[token_index + 3].type == TT_IMMEDIATE &&
            tokens[token_index + 4].type == TT_BRACKET_CLOSE) {

                expression[expression_index].type = EXPR_INDIRECT_IMMEDIATE;
                for (int j = 0; j < 5; j++) {
                    expression[expression_index].tokens[j] = tokens[token_index + j];
                }
                expression[expression_index].token_count = 5;
                expression_index ++;
                token_index += 5;
        }

        // [ reg ]
        else if (token_index + 2 < token_count &&
            tokens[token_index + 0].type == TT_BRACKET_OPEN &&
            tokens[token_index + 1].type == TT_REGISTER &&
            tokens[token_index + 2].type == TT_BRACKET_CLOSE) {

                expression[expression_index].type = EXPR_INDIRECT_REGISTER;
                for (int j = 0; j < 3; j++) {
                    expression[expression_index].tokens[j] = tokens[token_index + j];
                }
                expression[expression_index].token_count = 3;
                expression_index ++;
                token_index += 3;
        }

        // [ $xxxx + reg ]
        else if (token_index + 4 < token_count &&
            tokens[token_index + 0].type == TT_BRACKET_OPEN &&
            (tokens[token_index + 1].type == TT_IMMEDIATE || tokens[token_index + 1].type == TT_LABEL) &&
            tokens[token_index + 2].type == TT_PLUS &&
            tokens[token_index + 3].type == TT_REGISTER &&
            tokens[token_index + 4].type == TT_BRACKET_CLOSE) {

                expression[expression_index].type = EXPR_INDIRECT_REGISTER_OFFSET;
                for (int j = 0; j < 5; j++) {
                    expression[expression_index].tokens[j] = tokens[token_index + j];
                }
                expression[expression_index].token_count = 5;
                expression_index ++;
                token_index += 5;
        }

        // [ reg + $xxxx ]
        else if (token_index + 4 < token_count &&
            tokens[token_index + 0].type == TT_BRACKET_OPEN &&
            tokens[token_index + 1].type == TT_REGISTER &&
            tokens[token_index + 2].type == TT_PLUS &&
            (tokens[token_index + 3].type == TT_IMMEDIATE || tokens[token_index + 3].type == TT_LABEL) &&
            tokens[token_index + 4].type == TT_BRACKET_CLOSE) {

                expression[expression_index].type = EXPR_INDIRECT_REGISTER_OFFSET;

                expression[expression_index].tokens[0] = tokens[token_index + 0];
                expression[expression_index].tokens[1] = tokens[token_index + 3];
                expression[expression_index].tokens[2] = tokens[token_index + 2];
                expression[expression_index].tokens[3] = tokens[token_index + 1];
                expression[expression_index].tokens[4] = tokens[token_index + 4];

                expression[expression_index].token_count = 5;
                expression_index ++;
                token_index += 5;
        }

        // [ reg - $xxxx ]
        else if (token_index + 4 < token_count &&
            tokens[token_index + 0].type == TT_BRACKET_OPEN &&
            tokens[token_index + 1].type == TT_REGISTER &&
            tokens[token_index + 2].type == TT_MINUS &&
            (tokens[token_index + 3].type == TT_IMMEDIATE || tokens[token_index + 3].type == TT_LABEL) &&
            tokens[token_index + 4].type == TT_BRACKET_CLOSE) {

                expression[expression_index].type = EXPR_INDIRECT_REGISTER_OFFSET;

                expression[expression_index].tokens[0] = tokens[token_index + 0];
                expression[expression_index].tokens[1] = tokens[token_index + 3];
                expression[expression_index].tokens[2] = tokens[token_index + 2];
                expression[expression_index].tokens[3] = tokens[token_index + 1];
                expression[expression_index].tokens[4] = tokens[token_index + 4];

                expression[expression_index].token_count = 5;
                expression_index ++;
                token_index += 5;
        }

        // [ $xxxx + $xx * reg ]
        else if (token_index + 6 < token_count &&
            tokens[token_index + 0].type == TT_BRACKET_OPEN &&
            (tokens[token_index + 1].type == TT_IMMEDIATE || tokens[token_index + 1].type == TT_LABEL) &&
            tokens[token_index + 2].type == TT_PLUS &&
            (tokens[token_index + 3].type == TT_IMMEDIATE || tokens[token_index + 3].type == TT_LABEL) &&
            tokens[token_index + 4].type == TT_STAR &&
            tokens[token_index + 5].type == TT_REGISTER &&
            tokens[token_index + 6].type == TT_BRACKET_CLOSE) {

                expression[expression_index].type = EXPR_INDIRECT_SCALE_OFFSET;
                for (int j = 0; j < 7; j++) {
                    expression[expression_index].tokens[j] = tokens[token_index + j];
                }
                expression[expression_index].token_count = 7;
                expression_index ++;
                token_index += 7;
        }

        // [ $xxxx + reg * $xx ]
        else if (token_index + 6 < token_count &&
            tokens[token_index + 0].type == TT_BRACKET_OPEN &&
            (tokens[token_index + 1].type == TT_IMMEDIATE || tokens[token_index + 1].type == TT_LABEL) &&
            tokens[token_index + 2].type == TT_PLUS &&
            tokens[token_index + 3].type == TT_REGISTER &&
            tokens[token_index + 4].type == TT_STAR &&
            (tokens[token_index + 5].type == TT_IMMEDIATE || tokens[token_index + 3].type == TT_LABEL) &&
            tokens[token_index + 6].type == TT_BRACKET_CLOSE) {

                expression[expression_index].type = EXPR_INDIRECT_SCALE_OFFSET;
                
                expression[expression_index].tokens[0] = tokens[token_index + 0];
                expression[expression_index].tokens[1] = tokens[token_index + 1];
                expression[expression_index].tokens[2] = tokens[token_index + 2];
                expression[expression_index].tokens[3] = tokens[token_index + 5];
                expression[expression_index].tokens[4] = tokens[token_index + 4];
                expression[expression_index].tokens[5] = tokens[token_index + 3];
                expression[expression_index].tokens[6] = tokens[token_index + 6];

                expression[expression_index].token_count = 7;
                expression_index ++;
                token_index += 7;
        }

        // [ $xx * reg + $xxxx ]
        else if (token_index + 6 < token_count &&
            tokens[token_index + 0].type == TT_BRACKET_OPEN &&
            (tokens[token_index + 1].type == TT_IMMEDIATE || tokens[token_index + 1].type == TT_LABEL) &&
            tokens[token_index + 2].type == TT_STAR &&
            tokens[token_index + 3].type == TT_REGISTER &&
            tokens[token_index + 4].type == TT_PLUS &&
            (tokens[token_index + 5].type == TT_IMMEDIATE || tokens[token_index + 3].type == TT_LABEL) &&
            tokens[token_index + 6].type == TT_BRACKET_CLOSE) {

                expression[expression_index].type = EXPR_INDIRECT_SCALE_OFFSET;
                
                expression[expression_index].tokens[0] = tokens[token_index + 0];
                expression[expression_index].tokens[1] = tokens[token_index + 5];
                expression[expression_index].tokens[2] = tokens[token_index + 4];
                expression[expression_index].tokens[3] = tokens[token_index + 1];
                expression[expression_index].tokens[4] = tokens[token_index + 2];
                expression[expression_index].tokens[5] = tokens[token_index + 3];
                expression[expression_index].tokens[6] = tokens[token_index + 6];

                expression[expression_index].token_count = 7;
                expression_index ++;
                token_index += 7;
        }

        // [ reg * $xx + $xxxx ]
        else if (token_index + 6 < token_count &&
            tokens[token_index + 0].type == TT_BRACKET_OPEN &&
            tokens[token_index + 1].type == TT_REGISTER &&
            tokens[token_index + 2].type == TT_STAR &&
            (tokens[token_index + 3].type == TT_IMMEDIATE || tokens[token_index + 1].type == TT_LABEL) &&
            tokens[token_index + 4].type == TT_PLUS &&
            (tokens[token_index + 5].type == TT_IMMEDIATE || tokens[token_index + 3].type == TT_LABEL) &&
            tokens[token_index + 6].type == TT_BRACKET_CLOSE) {

                expression[expression_index].type = EXPR_INDIRECT_SCALE_OFFSET;
                
                expression[expression_index].tokens[0] = tokens[token_index + 0];
                expression[expression_index].tokens[1] = tokens[token_index + 5];
                expression[expression_index].tokens[2] = tokens[token_index + 4];
                expression[expression_index].tokens[3] = tokens[token_index + 3];
                expression[expression_index].tokens[4] = tokens[token_index + 2];
                expression[expression_index].tokens[5] = tokens[token_index + 1];
                expression[expression_index].tokens[6] = tokens[token_index + 6];

                expression[expression_index].token_count = 7;
                expression_index ++;
                token_index += 7;
        }

        // no-cache instruction
        else if (token_index + 1 < token_count &&
            tokens[token_index + 0].type == TT_NOCACHE &&
            tokens[token_index + 1].type == TT_INSTRUCTION) {

                expression[expression_index].type = EXPR_INSTRUCTION;
                // here no-cache modifier is token AFTER instruction
                expression[expression_index].tokens[0] = tokens[token_index + 1];
                expression[expression_index].tokens[1] = tokens[token_index + 0];
                expression[expression_index].token_count = 2;
                token_index += 2;
                expression_index ++;
        }

        // instruction
        else if (token_index + 0 < token_count &&
            tokens[token_index + 0].type == TT_INSTRUCTION) {

                expression[expression_index].type = EXPR_INSTRUCTION;
                expression[expression_index].tokens[0] = tokens[token_index];
                expression[expression_index].token_count = 1;
                token_index += 1;
                expression_index ++;
        }

        // .address $ffff
        else if (token_index + 1 < token_count &&
            tokens[token_index + 0].type == TT_ADDRESS &&
            tokens[token_index + 1].type == TT_IMMEDIATE) {

                expression[expression_index].type = EXPR_ADDRESS;
                expression[expression_index].tokens[0] = tokens[token_index + 0];
                expression[expression_index].tokens[1] = tokens[token_index + 1];
                expression[expression_index].token_count = 2;
                token_index += 2;
                expression_index ++;
        }

        // .data
        else if (token_index + 0 < token_count &&
            tokens[token_index + 0].type == TT_SEGMENT_DATA) {

                expression[expression_index].type = EXPR_SEGMENT_DATA;
                expression[expression_index].tokens[0] = tokens[token_index];
                expression[expression_index].token_count = 1;
                token_index += 1;
                expression_index ++;
        }

        // .code
        else if (token_index + 0 < token_count &&
            tokens[token_index + 0].type == TT_SEGMENT_CODE) {

                expression[expression_index].type = EXPR_SEGMENT_CODE;
                expression[expression_index].tokens[0] = tokens[token_index];
                expression[expression_index].token_count = 1;
                token_index += 1;
                expression_index ++;
        }

        // .reserve $ffff
        else if (token_index + 1 < token_count &&
            tokens[token_index + 0].type == TT_RESERVE &&
            tokens[token_index + 1].type == TT_IMMEDIATE) {

                expression[expression_index].type = EXPR_RESERVE;
                expression[expression_index].tokens[0] = tokens[token_index + 0];
                expression[expression_index].tokens[1] = tokens[token_index + 1];
                expression[expression_index].token_count = 2;
                token_index += 2;
                expression_index ++;
        }

        // .incbin "..."
        else if (token_index + 0 < token_count &&
            tokens[token_index + 0].type == TT_INCBIN &&
            tokens[token_index + 1].type == TT_STRING) {

                expression[expression_index].type = EXPR_INCBIN;
                expression[expression_index].tokens[0] = tokens[token_index + 0];
                expression[expression_index].tokens[1] = tokens[token_index + 1];
                expression[expression_index].token_count = 2;
                token_index += 2;
                expression_index ++;
        }

        // .text "..."
        else if (token_index + 1 < token_count &&
            tokens[token_index + 0].type == TT_TEXT &&
            tokens[token_index + 1].type == TT_STRING) {

                expression[expression_index].type = EXPR_TEXT_DEFINITION;
                expression[expression_index].tokens[0] = tokens[token_index + 0];
                expression[expression_index].tokens[1] = tokens[token_index + 1];
                expression[expression_index].token_count = 1;
                token_index += 2;
                expression_index ++;
        }

        // failure
        else {
            log_msg(LP_ERROR, "ASSEMBLER: Parsing tokens: Faulty expression! [%s:%d]", __FILE__, __LINE__);
            log_msg(LP_INFO, "ASSEMBLER: Parsing tokens: Previous Expression: \"%s\"", expression_type_string[expression[expression_index - 1].type]);
            log_msg(LP_INFO, "ASSEMBLER: Parsing tokens: Here is a breakdown of all the tokens:");
            for (int i = ((token_index - 8 > 0) ? (token_index - 8) : 0); i < ((token_index + 8 < token_count) ? (token_index + 8) : token_count); i++) {
                log_msg(LP_INFO, "ASSEMBLER: Parsing tokens: Token %d :: [%s] \"%s\"", i + 1, token_type_string[tokens[i].type], tokens[i].raw);
                if (i == token_index) {
                    log_msg(LP_NOTICE, "ASSEMBLER: Parsing tokens: ^^^ The sequence of tokens, starting with the upper one, is unknown ^^^");
                }
            }
            //exit(1);
            return NULL;
            token_index ++;
        }
    }
    
    if (expression_count) {
        *expression_count = expression_index;
    }

    return expression;
}


Instruction_t* assembler_parse_expression(Expression_t* expression, int expression_count, int* instruction_count, uint16_t** segment, int* segment_count, AssembleOption_t options) {
    int allocated_instructions = 16;
    Instruction_t* instruction = calloc(allocated_instructions, sizeof(Instruction_t));
    instruction->instruction = -1;

    int expression_index = 0;
    int instruction_index = 0;
    int byte_index = 0;
    //int is_label = 0;

    typedef enum {CODE, DATA} Mode_t;

    Mode_t mode = CODE;
    int byte_alignment = 2;
    int current_byte_align = 0; // switches between 0 and 1, with alignment 1, to show which byte to write to, since I can only do 16-bit else

    int exceeding_boundary_error = 0;

    while (expression_index < expression_count) {

        if (byte_index > SEGMENT_CODE_END) {
            if (!exceeding_boundary_error) {
                LOG_PRIORITY lp = (options & AO_ERROR_ON_CODE_SEGMENT_BREACH) ? LP_ERROR : LP_WARNING;
                log_msg(lp, "Parsing expressions: Binary exceeds memory boundary of 0x0000 - 0x%.4X [%s:%d]", SEGMENT_CODE_END, __FILE__, __LINE__);
                if (options & AO_ERROR_ON_CODE_SEGMENT_BREACH) {
                    // abort
                    return NULL;
                }
                if (options & AO_PAD_SEGMENT_BREACH_WITH_ZERO) {
                    log_msg(LP_INFO, "Regarding the warning above: The code segment will be padded with zeros instead, cutting part of the code off");
                } else {
                    log_msg(LP_WARNING, "Regarding the warning above: The data will spill into the segments outside the designated code segment");
                }
            }
            exceeding_boundary_error = 1;
            if (options & AO_PAD_SEGMENT_BREACH_WITH_ZERO) {
                // simply stop, the rest will be zeros instead
                break;
            }
            // else, do nothing. 
        }

        // check for const 
        if (instruction_index >= allocated_instructions) {
            allocated_instructions *= 2;
            instruction = realloc(instruction, sizeof(Instruction_t) * allocated_instructions);
            if (!instruction) exit(1);
        }

        if (mode == CODE) {
            instruction[instruction_index].expression[0] = expression[expression_index];
            instruction[instruction_index].expression_count = 1;
            instruction[instruction_index].is_address = 0;
            instruction[instruction_index].is_raw_data = 0;
            instruction[instruction_index].no_cache = 0;

            // verify its an instruction
            if (expression[expression_index].type != EXPR_INSTRUCTION) {
                // checking if its maybe a label instead
                if (expression[expression_index].type == EXPR_IMMEDIATE && expression[expression_index].tokens[0].type == TT_LABEL) {
                    
                    strcpy(jump_label[jump_label_index].name, expression[expression_index].tokens[0].raw);
                    int address = 0;

                    int instruction_index_since_last_address_change = instruction_index;
                    while (instruction_index_since_last_address_change > 0 && !instruction[instruction_index_since_last_address_change].is_address) {
                        instruction_index_since_last_address_change --;
                    }
                    if (instruction[instruction_index_since_last_address_change].is_address) {
                        address = instruction[instruction_index_since_last_address_change].address - 2;
                        if (segment && segment_count) {
                            // Allocate or reallocate the segment buffer
                            uint16_t* new_segment = realloc(*segment, sizeof(uint16_t) * (*segment_count + 1));
                            if (!new_segment) {
                                fprintf(stderr, "Memory allocation for segment failed\n");
                                exit(1);
                            }
                            *segment = new_segment;
                            (*segment)[*segment_count] = instruction[instruction_index_since_last_address_change].address;
                            (*segment_count)++;
                        }
                    }

                    for (int i = instruction_index_since_last_address_change; i < instruction_index; i++) {
                        if (instruction[i].expression[0].type == EXPR_SEGMENT_CODE || 
                            instruction[i].expression[0].type == EXPR_SEGMENT_DATA ||
                            instruction[i].expression[0].tokens[0].type == TT_LABEL) {
                            continue;
                        } 
                        address += instruction[i].argument_bytes + 2;
                    }
                    jump_label[jump_label_index].value = address;
                    //log_msg(LP_INFO, "Parsing expressions: Added label \"%s\" with current value %d", jump_label[jump_label_index].name, jump_label[jump_label_index].value);
                    jump_label_index ++;
                    if (jump_label_index >= MAX_LABELS) {
                        log_msg(LP_ERROR, "Parsing expressions: Label count is over the maximum limit (%d) [%s:%d]", MAX_LABELS, __FILE__, __LINE__);
                        exit(1);
                    }
                    instruction_index ++;
                    expression_index ++;
                    continue;
                } else if (expression[expression_index].type == EXPR_ADDRESS) {
                    instruction[instruction_index].is_address = 1;

                    char* string_value = expression[expression_index].tokens[1].raw;
                    //printf("EXPR_ADDRESS: %s\n", string_value);
                    int value = parse_immediate(string_value); //(int) strtol(&string_value[1], NULL, 16);
                    instruction[instruction_index].address = value;

                    byte_index = value;

                    //log_msg(LP_INFO, "Parsing expressions: Added address jump to %.4x", value);
                    
                    instruction_index ++;

                    while (instruction_index >= allocated_instructions) {
                        allocated_instructions *= 2;
                        instruction = realloc(instruction, sizeof(Instruction_t) * allocated_instructions);
                        //log_msg(LP_INFO, "Reallocated instruction array to %d", allocated_instructions);
                    }
                    
                    expression_index ++;
                    continue;
                } else if (expression[expression_index].type == EXPR_RESERVE) {
                    instruction[instruction_index].is_address = 1;

                    char* string_value = expression[expression_index].tokens[1].raw;
                    //printf("EXPR_RESERVE: %s\n", string_value);
                    int value = parse_immediate(string_value); //(int) strtol(&string_value[1], NULL, 16);
                    instruction[instruction_index].address = byte_index + value;

                    byte_index += value;

                    //log_msg(LP_INFO, "Parsing expressions: Added address jump to %.4x", value);
                    
                    instruction_index ++;

                    while (instruction_index >= allocated_instructions) {
                        allocated_instructions *= 2;
                        instruction = realloc(instruction, sizeof(Instruction_t) * allocated_instructions);
                        //log_msg(LP_INFO, "Reallocated instruction array to %d", allocated_instructions);
                    }
                    
                    expression_index ++;
                    continue;
                } else if (expression[expression_index].type == EXPR_SEGMENT_DATA) {
                    instruction_index ++;
                    mode = DATA;
                    expression_index ++;
                    continue;
                } else if (expression[expression_index].type == EXPR_SEGMENT_CODE) {
                    instruction_index ++;
                    expression_index ++;
                    continue;
                } else {
                    log_msg(LP_ERROR, "Parsing expressions: Expected an INSTRUCTION or a LABEL, got \"%s\" instead [%s:%d]", expression_type_string[expression[expression_index].type], __FILE__, __LINE__);
                    return NULL;
                }
            }

            // 1. identify instruction
            int found = 0;
            for (int i = 0; i < INSTRUCTION_COUNT; i++) {
                //printf("%s vs %s : %d\n", expression[expression_index].tokens[0].raw, cpu_instruction_string[i], strcmp(expression[expression_index].tokens[0].raw, cpu_instruction_string[i]));
                if (strcmp(expression[expression_index].tokens[0].raw, cpu_instruction_string[i]) == 0) {
                    instruction[instruction_index].instruction = i;
                    found = 1;
                    break;
                }
            }

            if (!found) {
                log_msg(LP_ERROR, "Parsing expressions: Unknown instruction \"%s\" [%s:%d]", expression[expression_index].tokens[0].raw, __FILE__, __LINE__);
                return NULL;
            }

            // Do a check for no-cache modifier
            if (expression[expression_index].token_count > 1 && expression[expression_index].tokens[1].type == TT_NOCACHE) {
                instruction[instruction_index].no_cache = 1;
            } 

            // 2. get instruction argument count
            int argument_count = cpu_instruction_argument_count[instruction[instruction_index].instruction];
            //log_msg(LP_INFO, "Parsing expressions: instruction \"%s\" takes %d arguments", cpu_instruction_string[instruction[instruction_index].instruction], argument_count);
            expression_index ++;

            // 3. identify adm's
            instruction[instruction_index].admx = ADMX_NONE;
            instruction[instruction_index].admr = ADMR_NONE;
            int admr_expression_index = expression_index;
            if (argument_count == 2 || cpu_instruction_single_operand_writeback[instruction[instruction_index].instruction]) {
                
                instruction[instruction_index].expression[instruction[instruction_index].expression_count] = expression[expression_index];
                instruction[instruction_index].expression_count ++;

                CPU_REDUCED_ADDRESSING_MODE_t admr = -1;
                switch (expression[expression_index].type) {
                    case EXPR_INDIRECT_REGISTER:
                        {
                            char* register_string = expression[expression_index].tokens[1].raw;
                            if (register_string[0] == 'r') {
                                if (register_string[1] == '0') {
                                    admr = ADMR_IND_R0;
                                }
                            }
                        }
                        break;
                    case EXPR_INDIRECT_IMMEDIATE:
                        admr = ADMR_IND16;
                        break;
                    
                    case EXPR_IMMEDIATE: // cant be immediate, so it must have been the placeholder '_'
                        log_msg(LP_WARNING, "Assembler: Implicitly treating garbage arguments as raw data (Immediate cannot be writeback address in single operand admr) [%s:%d]", __FILE__, __LINE__);
                        admr = ADMR_NONE;
                        break;

                    case EXPR_REGISTER:
                    {
                        char* register_string = expression[expression_index].tokens[0].raw;
                        if (register_string[0] == 'r') {
                            if (register_string[1] == '0') {
                                admr = ADMR_R0;
                            } else if (register_string[1] == '1') {
                                admr = ADMR_R1;
                            } else if (register_string[1] == '2') {
                                admr = ADMR_R2;
                            } else if (register_string[1] == '3') {
                                admr = ADMR_R3;
                            }
                        } else if (register_string[0] == 's' && register_string[1] == 'p') {
                            admr = ADMR_SP;
                        }
                        break;
                    }

                    default:
                        log_msg(LP_ERROR, "Assembler: Parsing expressions: Unknown admr \"%s\" [%s:%d]", expression_type_string[expression[expression_index].type], __FILE__, __LINE__);
                        /*int index = 0;
                        while (expression[expression_index].tokens[index].raw) {
                            log_msg(LP_INFO, "expression token %d: \"%s\"", index, expression[expression_index].tokens[index].raw);
                            index ++;
                        }*/
                        break;
                }
                if ((int) admr == -1) {
                    log_msg(LP_ERROR, "Assembler: Parsing expressions: admr is invalid! \"%s\" [%s:%d]", expression[expression_index].tokens[0].raw, __FILE__, __LINE__);
                    log_msg_inline(LP_INFO, "line: ");
                    for (int t = 0; t < expression[expression_index - 1].token_count; t++) {
                        printf("%s ", expression[expression_index - 1].tokens[t].raw);
                    }
                    for (int t = 0; t < expression[expression_index].token_count; t++) {
                        printf("%s ", expression[expression_index].tokens[t].raw);
                    }
                    printf("...\n");
                    return NULL;
                    //exit(1);
                }
                instruction[instruction_index].admr = admr;
                expression_index ++;
            }

            int admx_expression_index = expression_index;
            if (argument_count >= 1 && !cpu_instruction_single_operand_writeback[instruction[instruction_index].instruction]) {

                instruction[instruction_index].expression[instruction[instruction_index].expression_count] = expression[expression_index];
                instruction[instruction_index].expression_count ++;

                CPU_EXTENDED_ADDRESSING_MODE_t admx = -1;
                switch (expression[expression_index].type) {
                    case EXPR_INDIRECT_REGISTER:
                        {
                            char* register_string = expression[expression_index].tokens[1].raw;
                            if (register_string[0] == 'r') {
                                if (register_string[1] == '0') {
                                    admx = ADMX_IND_R0;
                                } else if (register_string[1] == '1') {
                                    admx = ADMX_IND_R1;
                                } else if (register_string[1] == '2') {
                                    admx = ADMX_IND_R2;
                                } else if (register_string[1] == '3') {
                                    admx = ADMX_IND_R3;
                                }
                            } else if (register_string[0] == 's' && register_string[1] == 'p') {
                                admx = ADMX_IND_SP;
                            } else if (register_string[0] == 'p' && register_string[1] == 'c') {
                                admx = ADMX_IND_PC;
                            }
                        }
                        break;
                    
                    case EXPR_INDIRECT_REGISTER_OFFSET:
                        {
                            char* register_string = expression[expression_index].tokens[3].raw;
                            if (register_string[0] == 'r') {
                                if (register_string[1] == '0') {
                                    admx = ADMX_IND_R0_OFFSET16;
                                } else if (register_string[1] == '1') {
                                    admx = ADMX_IND_R1_OFFSET16;
                                } else if (register_string[1] == '2') {
                                    admx = ADMX_IND_R2_OFFSET16;
                                } else if (register_string[1] == '3') {
                                    admx = ADMX_IND_R3_OFFSET16;
                                }
                            } else if (register_string[0] == 's' && register_string[1] == 'p') {
                                admx = ADMX_IND_SP_OFFSET16;
                            } else if (register_string[0] == 'p' && register_string[1] == 'c') {
                                admx = ADMX_IND_PC_OFFSET16;
                            }
                        }
                        break;
                    
                    case EXPR_INDIRECT_SCALE_OFFSET:
                        {
                            char* register_string = expression[expression_index].tokens[5].raw;
                            if (register_string[0] == 'r') {
                                if (register_string[1] == '0') {
                                    admx = ADMX_IND16_SCALED8_R0_OFFSET;
                                } else if (register_string[1] == '1') {
                                    admx = ADMX_IND16_SCALED8_R1_OFFSET;
                                } else if (register_string[1] == '2') {
                                    admx = ADMX_IND16_SCALED8_R2_OFFSET;
                                } else if (register_string[1] == '3') {
                                    admx = ADMX_IND16_SCALED8_R3_OFFSET;
                                }
                            } else if (register_string[0] == 's' && register_string[1] == 'p') {
                                admx = ADMX_IND16_SCALED8_SP_OFFSET;
                            } else if (register_string[0] == 'p' && register_string[1] == 'c') {
                                admx = ADMX_IND16_SCALED8_PC_OFFSET;
                            }
                        }
                        break;

                    case EXPR_IMMEDIATE:
                        {
                            if (expression[expression_index].tokens[0].raw[0] == '_') {
                                log_msg(LP_WARNING, "Assembler: Implicitly treating garbage arguments as raw data ('_' Token found) [%s:%d]", __FILE__, __LINE__);
                                admx = ADMX_NONE;
                            } else {
                                admx = ADMX_IMM16;
                            }
                        }
                        break;
                    
                    case EXPR_REGISTER:
                        {
                            char* register_string = expression[expression_index].tokens[0].raw;
                            if (register_string[0] == 'r') {
                                if (register_string[1] == '0') {
                                    admx = ADMX_R0;
                                } else if (register_string[1] == '1') {
                                    admx = ADMX_R1;
                                } else if (register_string[1] == '2') {
                                    admx = ADMX_R2;
                                } else if (register_string[1] == '3') {
                                    admx = ADMX_R3;
                                }
                            } else if (register_string[0] == 's' && register_string[1] == 'p') {
                                admx = ADMX_SP;
                            } else if (register_string[0] == 'p' && register_string[1] == 'c') {
                                admx = ADMX_PC;
                            }
                        }
                        break;
                    
                    case EXPR_INDIRECT_IMMEDIATE:
                        admx = ADMX_IND16;
                        break;

                    default:
                        log_msg(LP_ERROR, "Parsing expressions: Unknown admx \"%s\" [%s:%d]", expression_type_string[expression[expression_index].type], __FILE__, __LINE__);
                        log_msg(LP_INFO, "Parsing expressions: Here is a breakdown of all the expressions:");
                        for (int i = ((expression_index - 8 > 0) ? (expression_index - 8) : 0); i < ((expression_index + 8 < expression_count) ? (expression_index + 8) : expression_count); i++) {
                            log_msg_inline(LP_INFO, "Parsing expressions: Expression %d :: \"%s\", {", i + 1, expression_type_string[expression[i].type]);
                            for (int j = 0; j < expression[i].token_count; j++) {
                                printf("%s", expression[i].tokens[j].raw);
                                if (j < expression[i].token_count - 1) {printf(", ");}
                            } printf("}\n");
                            if (i == expression_index) {
                                log_msg(LP_NOTICE, "Parsing expressions: ^^^ The sequence of tokens, starting with the upper one, is unknown ^^^");
                            }
                        }
                        break;
                }
                if ((int) admx == -1) {
                    log_msg(LP_ERROR, "Parsing expressions: admx is invalid! [%s:%d]", __FILE__, __LINE__);
                    exit(1);
                }
                instruction[instruction_index].admx = admx;
                expression_index ++;
            }

            //printf("admr: %s, admx: %s\n", cpu_reduced_addressing_mode_string[instruction[instruction_index].admr], cpu_extended_addressing_mode_string[instruction[instruction_index].admx]);
            
            // 4. extract arguments
            // first arguments from admx, then admr!
            int argument_bytes_used = 0;
            switch (instruction[instruction_index].admx) {
                case ADMX_R0:
                case ADMX_R1:
                case ADMX_R2:
                case ADMX_R3:
                case ADMX_SP:
                case ADMX_PC:
                case ADMX_IND_R0:
                case ADMX_IND_R1:
                case ADMX_IND_R2:
                case ADMX_IND_R3:
                case ADMX_IND_SP:
                case ADMX_IND_PC:
                case ADMX_NONE:
                    break;

                case ADMX_IND_R0_OFFSET16:
                case ADMX_IND_R1_OFFSET16:
                case ADMX_IND_R2_OFFSET16:
                case ADMX_IND_R3_OFFSET16:
                case ADMX_IND_SP_OFFSET16:
                case ADMX_IND_PC_OFFSET16:
                    {
                        char* string_base = expression[admx_expression_index].tokens[1].raw;
                        int value_base = parse_immediate(string_base); //(int) strtol(&string_base[1], NULL, 16);
                        instruction[instruction_index].arguments[0] = value_base & 0xff;
                        instruction[instruction_index].arguments[1] = (value_base >> 8) & 0xff;
                        argument_bytes_used = 2;
                    }
                    break;

                case ADMX_IND16_SCALED8_R0_OFFSET:
                case ADMX_IND16_SCALED8_R1_OFFSET:
                case ADMX_IND16_SCALED8_R2_OFFSET:
                case ADMX_IND16_SCALED8_R3_OFFSET:
                case ADMX_IND16_SCALED8_SP_OFFSET:
                case ADMX_IND16_SCALED8_PC_OFFSET:
                    {
                        char* string_base = expression[admx_expression_index].tokens[1].raw;
                        char* string_scale = expression[admx_expression_index].tokens[3].raw;
                        int value_base = parse_immediate(string_base); //(int) strtol(&string_base[1], NULL, 16);
                        int value_scale = parse_immediate(string_scale); //(int) strtol(&string_scale[1], NULL, 16);
                        if (value_scale > 0xff) {
                            log_msg(LP_WARNING, "Parsing expressions: Scaling in indirect scaled register addressing mode will be truncated! [0x%.4x -> 0x%.2x] [%s:%d]", value_scale, value_scale & 0x00ff, __FILE__, __LINE__);
                        } 
                        instruction[instruction_index].arguments[0] = value_base & 0xff;
                        instruction[instruction_index].arguments[1] = (value_base >> 8) & 0xff;
                        instruction[instruction_index].arguments[2] = value_scale;
                        argument_bytes_used = 3;
                    }
                    break;

                case ADMX_IMM16:
                    {
                        if (expression[admx_expression_index].token_count > 1) {
                            char* string_value = expression[admx_expression_index].tokens[2].raw;
                            //printf("%s\n", string_value);
                            int value = parse_immediate(string_value); //(int) strtol(&string_value[1], NULL, 16);
                            instruction[instruction_index].arguments[0] = value & 0xff;
                            instruction[instruction_index].arguments[1] = (value >> 8) & 0xff;
                            argument_bytes_used = 2;
                        } else {
                            char* string_value = expression[admx_expression_index].tokens[0].raw;
                            //printf("%s\n", string_value);
                            int value = parse_immediate(string_value); //(int) strtol(&string_value[1], NULL, 16);
                            instruction[instruction_index].arguments[0] = value & 0xff;
                            instruction[instruction_index].arguments[1] = (value >> 8) & 0xff;
                            argument_bytes_used = 2;
                        }
                    }
                    break;

                case ADMX_IND16:
                    {
                        
                        if (expression[admx_expression_index].token_count > 3) {
                            char* string_value = expression[admx_expression_index].tokens[3].raw;
                            int value = parse_immediate(string_value); //(int) strtol(&string_value[1], NULL, 16);
                            instruction[instruction_index].arguments[0] = value & 0xff;
                            instruction[instruction_index].arguments[1] = (value >> 8) & 0xff;
                            argument_bytes_used = 2;
                        } else {
                            char* string_value = expression[admx_expression_index].tokens[1].raw;
                            int value = parse_immediate(string_value); //(int) strtol(&string_value[1], NULL, 16);
                            instruction[instruction_index].arguments[0] = value & 0xff;
                            instruction[instruction_index].arguments[1] = (value >> 8) & 0xff;
                            argument_bytes_used = 2;
                        }
                    }
                    break;

                default:
                    log_msg(LP_ERROR, "Parsing expressions: Unhandled admx: \"%s\" [%s:%d]", cpu_extended_addressing_mode_string[instruction[instruction_index].admx], __FILE__, __LINE__);
                    break;
            }

            switch (instruction[instruction_index].admr) {
                case ADMR_R0:
                case ADMR_R1:
                case ADMR_R2:
                case ADMR_R3:
                case ADMR_SP:
                case ADMR_IND_R0:
                case ADMR_NONE:
                    break;
                
                case ADMR_IND16:
                    {
                        if (expression[admr_expression_index].token_count > 3) {
                            char* string_value = expression[admr_expression_index].tokens[3].raw;
                            int value = parse_immediate(string_value); //(int) strtol(&string_value[1], NULL, 16);
                            instruction[instruction_index].arguments[argument_bytes_used + 0] = value & 0xff;
                            instruction[instruction_index].arguments[argument_bytes_used + 1] = (value >> 8) & 0xff;
                            argument_bytes_used += 2;
                        } else {
                            char* string_value = expression[admr_expression_index].tokens[1].raw;
                            int value = parse_immediate(string_value); //(int) strtol(&string_value[1], NULL, 16);
                            instruction[instruction_index].arguments[argument_bytes_used + 0] = value & 0xff;
                            instruction[instruction_index].arguments[argument_bytes_used + 1] = (value >> 8) & 0xff;
                            argument_bytes_used += 2;
                        }
                    }
                    break;

                default:
                    log_msg(LP_ERROR, "Parsing expressions: Unhandled admr: \"%d\" [%s:%d]", instruction[instruction_index].admr, __FILE__, __LINE__);
                    break;
            }
            instruction[instruction_index].argument_bytes = argument_bytes_used;
            instruction[instruction_index].address = byte_index;

            byte_index += 2 + argument_bytes_used;

            instruction_index ++;

            while (instruction_index >= allocated_instructions) {
                allocated_instructions *= 2;
                instruction = realloc(instruction, sizeof(Instruction_t) * allocated_instructions);
                //log_msg(LP_INFO, "Reallocated instruction array to %d", allocated_instructions);
            }

        } else if (mode == DATA) {  // |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

            instruction[instruction_index].expression[0] = expression[expression_index];
            instruction[instruction_index].expression_count = 1;
            instruction[instruction_index].is_address = 0;
            instruction[instruction_index].is_raw_data = 1;
            instruction[instruction_index].byte_aligned = 1;

            if (expression[expression_index].tokens[0].type == TT_LABEL) {

                strcpy(jump_label[jump_label_index].name, expression[expression_index].tokens[0].raw);
                int address = 0;

                int instruction_index_since_last_address_change = instruction_index;
                while (instruction_index_since_last_address_change > 0 && !instruction[instruction_index_since_last_address_change].is_address) {
                    instruction_index_since_last_address_change --;
                }
                if (instruction[instruction_index_since_last_address_change].is_address) {
                    address = instruction[instruction_index_since_last_address_change].address - 2;
                    if (segment && segment_count) {
                        // Allocate or reallocate the segment buffer
                        uint16_t* new_segment = realloc(*segment, sizeof(uint16_t) * (*segment_count + 1));
                        if (!new_segment) {
                            fprintf(stderr, "Memory allocation for segment failed\n");
                            exit(1);
                        }
                        *segment = new_segment;
                        (*segment)[*segment_count] = instruction[instruction_index_since_last_address_change].address;
                        (*segment_count)++;
                    }
                }

                for (int i = instruction_index_since_last_address_change; i < instruction_index; i++) {
                    if (instruction[i].expression[0].type == EXPR_SEGMENT_CODE || 
                        instruction[i].expression[0].type == EXPR_SEGMENT_DATA ||
                        instruction[i].expression[0].tokens[0].type == TT_LABEL) {
                        continue;
                    } 
                    address += instruction[i].argument_bytes + 2;
                    byte_index = address;
                }
                jump_label[jump_label_index].value = address;
                if (current_byte_align == 1) {
                    jump_label[jump_label_index].value += 1; // accomodate for shift by 1 byte
                }
                //log_msg(LP_INFO, "Parsing expressions: Added label \"%s\" with current value %d", jump_label[jump_label_index].name, jump_label[jump_label_index].value);
                jump_label_index ++;
                if (jump_label_index >= MAX_LABELS) {
                    log_msg(LP_ERROR, "Parsing expressions: Label count is over the maximum limit (%d) [%s:%d]", MAX_LABELS, __FILE__, __LINE__);
                    exit(1);
                }
                
                expression_index ++;
                instruction_index ++;
                continue;
            } else if (expression[expression_index].type == EXPR_SEGMENT_CODE) {
                instruction_index ++;
                mode = CODE;
                /*if (current_byte_align) {
                    instruction_index += 1;
                }*/
                expression_index += 1;
                continue;
            } else if (expression[expression_index].type == EXPR_IMMEDIATE) {
                if (expression[expression_index].token_count > 1) {
                    log_msg(LP_ERROR, "Parsing expressions: label offset immediate values are not allowed inside data segment [%s:%d]", __FILE__, __LINE__);
                    return NULL;
                }
                char* string_value = expression[expression_index].tokens[0].raw;
                int value = parse_immediate(string_value); //(int) strtol(&string_value[1], NULL, 16);
                if (byte_alignment == 1) {
                    if (value > 0x00ff) {
                        log_msg(LP_WARNING, "Parsing expressions: Due to the alignment, values above 0xff will be truncated! This also applies to floating point values! [%s:%d]", __FILE__, __LINE__);
                    }
                    if (current_byte_align == 0) {
                        instruction[instruction_index].raw_data = value & 0x00ff;
                        current_byte_align = 1;
                        instruction[instruction_index].byte_aligned = 0;
                        expression_index ++;
                        continue;
                    } else {
                        instruction[instruction_index].raw_data = ((value & 0x00ff) << 8) | (instruction[instruction_index].raw_data & 0x00ff);
                        current_byte_align = 0;
                        instruction[instruction_index].byte_aligned = 1;
                    }
                } else if (byte_alignment == 2) {
                    instruction[instruction_index].raw_data = value;
                } else {
                    log_msg(LP_ERROR, "Parsing expressions: No alignment specified in data segment! [%s:%d]", __FILE__, __LINE__);
                    exit(1);
                }
                byte_index += 2;
            } else if (expression[expression_index].type == EXPR_SEGMENT_DATA) {
                instruction_index ++;
                expression_index ++;
                continue;
            } else if (expression[expression_index].type == EXPR_ADDRESS) {
                instruction[instruction_index].is_address = 1;

                char* string_value = expression[expression_index].tokens[1].raw;
                //printf("%s\n", string_value);
                int value = parse_immediate(string_value); //(int) strtol(&string_value[1], NULL, 16);
                instruction[instruction_index].address = value;
                byte_index = value;

                //log_msg(LP_INFO, "Parsing expressions: Added address jump to %.4x", value);
                
                instruction_index ++;

                while (instruction_index >= allocated_instructions) {
                    allocated_instructions *= 2;
                    instruction = realloc(instruction, sizeof(Instruction_t) * allocated_instructions);
                    //log_msg(LP_INFO, "Reallocated instruction array to %d", allocated_instructions);
                }
                
                expression_index ++;
                continue;
            } else if (expression[expression_index].type == EXPR_RESERVE) {
                instruction[instruction_index].is_address = 1;

                char* string_value = expression[expression_index].tokens[1].raw;
                //printf("%s\n", string_value);
                int value = parse_immediate(string_value); //(int) strtol(&string_value[1], NULL, 16);
                instruction[instruction_index].address = byte_index + value;
                byte_index += value;

                //log_msg(LP_INFO, "Parsing expressions: Added address jump to %.4x", value);
                
                instruction_index ++;

                while (instruction_index >= allocated_instructions) {
                    allocated_instructions *= 2;
                    instruction = realloc(instruction, sizeof(Instruction_t) * allocated_instructions);
                    //log_msg(LP_INFO, "Reallocated instruction array to %d", allocated_instructions);
                }
                
                expression_index ++;
                continue;
            } else if (expression[expression_index].type == EXPR_TEXT_DEFINITION) {
                char* string_value = expression[expression_index].tokens[1].raw;
                string_value[strlen(string_value) - 1] = '\0';  // this makes the output implicitly null terminated
                for (size_t i = 1; i < strlen(string_value) + 1; i++) {
                    printf("%d: %.2x\n", instruction_index, (uint8_t) string_value[i]);
                    uint8_t value = 0x00;
                    if (i < strlen(string_value)) {
                        value = (uint8_t) string_value[i];
                    }
                    if (current_byte_align == 0) {
                        instruction[instruction_index].raw_data = value;
                        current_byte_align = 1;

                        instruction[instruction_index].expression[0] = expression[expression_index];
                        instruction[instruction_index].expression_count = 1;
                        instruction[instruction_index].is_address = 0;
                        instruction[instruction_index].is_raw_data = 1;
                        instruction[instruction_index].byte_aligned = 0;
                    } else {
                        instruction[instruction_index].raw_data = (value << 8) | (instruction[instruction_index].raw_data & 0x00ff);
                        current_byte_align = 0;
                        instruction[instruction_index].byte_aligned = 1;
                        instruction_index ++;

                        while (instruction_index >= allocated_instructions) {
                            allocated_instructions *= 2;
                            instruction = realloc(instruction, sizeof(Instruction_t) * allocated_instructions);
                            //log_msg(LP_INFO, "Reallocated instruction array to %d", allocated_instructions);
                        }
                        
                        instruction[instruction_index].expression[0] = expression[expression_index];
                        instruction[instruction_index].expression_count = 1;
                        instruction[instruction_index].is_address = 0;
                        instruction[instruction_index].is_raw_data = 1;
                        instruction[instruction_index].byte_aligned = 1;
                    }
                    byte_index ++;
                }
                current_byte_align = 0;
                expression_index ++;
                continue;
                //exit(1);
            } else if (expression[expression_index].type == EXPR_INCBIN) {
                instruction[instruction_index].expression[0] = expression[expression_index];
                instruction[instruction_index].expression_count = 1;
                instruction[instruction_index].address = byte_index;
                //char* filename = calloc(strlen(instruction[instruction_index].expression[0].tokens[1].raw) - 1, 1);
                //memcpy(filename, instruction[instruction_index].expression[0].tokens[1].raw + 1, strlen(instruction[instruction_index].expression[0].tokens[1].raw + 1));
                //filename[strlen(filename) - 1] = '\0';
                //byte_index += file_size(filename);
                //free(filename);
            } else if (expression[expression_index].type == EXPR_NONE) {

            } else {
                log_msg(LP_WARNING, "Parsing expression: Unknown expression type while in data segment \"%s\" [%s:%d]", expression_type_string[expression[expression_index].type], __FILE__, __LINE__);
                log_msg(LP_INFO, "Parsing expression: Will treat code as code from now on, but this could be instable");
                mode = CODE;
                continue;
                //exit(1);
            }

            expression_index ++;
            instruction_index ++;

            while (instruction_index >= allocated_instructions) {
                allocated_instructions *= 2;
                instruction = realloc(instruction, sizeof(Instruction_t) * allocated_instructions);
                //log_msg(LP_INFO, "Reallocated instruction array to %d", allocated_instructions);
            }
        }
    }

    if (current_byte_align) { // here if the last assembly line is a 1-aligned byte data that is not aligned to 2
        //instruction_index ++;
    }

    *instruction_count = instruction_index;

    return instruction;
}

Instruction_t* assembler_resolve_labels(Instruction_t* instruction, int instruction_count) {
    if (! instruction) {
        return NULL;
    }
    for (int i = 0; i < instruction_count; i++) {
        int expression_count = instruction[i].expression_count;
        int argument_byte_index = 0;
        for (int exp = expression_count - 1; exp >= 0; exp --) {
            switch (instruction[i].expression[exp].type) {
                case EXPR_IMMEDIATE:
                    if ((int) instruction[i].instruction != -1 && !cpu_instruction_is_relative_jump[instruction[i].instruction]) {
                        // here the first two bytes are guaranteed to be the label destination
                        if (instruction[i].expression[exp].tokens[0].type == TT_LABEL) {
                            // find corresponding label
                            int corresponding_label_found = -1;
                            for (int j = 0; j < jump_label_index; j++) {
                                if (strcmp(instruction[i].expression[exp].tokens[0].raw, jump_label[j].name) == 0) {
                                    if (corresponding_label_found == -1) {
                                        corresponding_label_found = j;
                                    } else {
                                        log_msg(LP_ERROR, "Label \"%s\" is not unique [%s:%d]", instruction[i].expression[exp].tokens[0].raw, __FILE__, __LINE__);
                                        exit(1);
                                        break;
                                    }
                                }
                            }
                            if (corresponding_label_found == -1) {
                                log_msg(LP_ERROR, "Solving label: Unable to find corresponding label: \"%s\" [%s:%d]", instruction[i].expression[exp].tokens[0].raw, __FILE__, __LINE__);
                                exit(1);
                                break;
                            }
                            //log_msg(LP_INFO, "Solving label: Resolving \"%s\" to value 0x%.4x", jump_label[corresponding_label_found].name, jump_label[corresponding_label_found].value);
                            if (instruction[i].expression[exp].token_count > 1) {
                                // here if expression has form [.label + imm]
                                instruction[i].arguments[argument_byte_index] += jump_label[corresponding_label_found].value & 0x00ff;
                                instruction[i].arguments[argument_byte_index + 1] += ((jump_label[corresponding_label_found].value & 0xff00) >> 8) + (instruction[i].arguments[argument_byte_index] >> 8);
                                instruction[i].arguments[argument_byte_index] &= 0x00ff;
                                argument_byte_index += 2;
                            } else {
                                instruction[i].arguments[argument_byte_index++] = jump_label[corresponding_label_found].value & 0x00ff;
                                instruction[i].arguments[argument_byte_index++] = (jump_label[corresponding_label_found].value & 0xff00) >> 8;
                            }
                        } else {
                            argument_byte_index += 2;
                        }
                        break;
                    } else {
                        // Immediate values are the only ones where we can directly calculate the relative equivalent address, otherwise it wouldnt work (i.e. for indirect accesses)
                        if (instruction[i].expression[exp].tokens[0].type == TT_LABEL) {
                            // find corresponding label
                            int corresponding_label_found = -1;
                            for (int j = 0; j < jump_label_index; j++) {
                                if (strcmp(instruction[i].expression[exp].tokens[0].raw, jump_label[j].name) == 0) {
                                    if (corresponding_label_found == -1) {
                                        corresponding_label_found = j;
                                    } else {
                                        log_msg(LP_ERROR, "Label \"%s\" is not unique [%s:%d]", instruction[i].expression[exp].tokens[0].raw, __FILE__, __LINE__);
                                        exit(1);
                                        break;
                                    }
                                }
                            }
                            if (corresponding_label_found == -1) {
                                log_msg(LP_ERROR, "Solving label: Unable to find corresponding label: \"%s\" [%s:%d]", instruction[i].expression[exp].tokens[0].raw, __FILE__, __LINE__);
                                exit(1);
                                break;
                            }
                            //log_msg(LP_INFO, "Solving label: Resolving \"%s\" to value 0x%.4x", jump_label[corresponding_label_found].name, (jump_label[corresponding_label_found].value - instruction[i].address - 4));
                            if (instruction[i].expression[exp].token_count > 1) {
                                // here if expression has form [.label + imm]
                                instruction[i].arguments[argument_byte_index] += (jump_label[corresponding_label_found].value - instruction[i].address) & 0x00ff;
                                instruction[i].arguments[argument_byte_index + 1] += (((jump_label[corresponding_label_found].value - instruction[i].address) & 0xff00) >> 8) + (instruction[i].arguments[argument_byte_index] >> 8);
                                instruction[i].arguments[argument_byte_index] &= 0x00ff;
                                argument_byte_index += 2;
                            } else {
                                instruction[i].arguments[argument_byte_index++] = (jump_label[corresponding_label_found].value - instruction[i].address - 4) & 0x00ff;
                                instruction[i].arguments[argument_byte_index++] = ((jump_label[corresponding_label_found].value - instruction[i].address - 4) & 0xff00) >> 8;
                            }
                            
                        } else {
                            argument_byte_index += 2;
                        }
                        break;
                    }
                
                case EXPR_INDIRECT_IMMEDIATE:
                    // here the second and third bytes are guaranteed to be the label destination
                    if (instruction[i].expression[exp].tokens[1].type == TT_LABEL) {
                        // find corresponding label
                        int corresponding_label_found = -1;
                        for (int j = 0; j < jump_label_index; j++) {
                            if (strcmp(instruction[i].expression[exp].tokens[1].raw, jump_label[j].name) == 0) {
                                corresponding_label_found = j;
                                break;
                            }
                        }
                        if (corresponding_label_found == -1) {
                            log_msg(LP_ERROR, "Solving label: Unable to find corresponding label: \"%s\" [%s:%d]", instruction[i].expression[exp].tokens[1].raw, __FILE__, __LINE__);
                            exit(1);
                            break;
                        }
                        //log_msg(LP_INFO, "Solving label: Resolving \"%s\" to value 0x%.4x", jump_label[corresponding_label_found].name, jump_label[corresponding_label_found].value);
                        if (instruction[i].expression[exp].token_count > 3) {
                            // here if expression has form [.label + imm]
                            instruction[i].arguments[argument_byte_index] += jump_label[corresponding_label_found].value & 0x00ff;
                            instruction[i].arguments[argument_byte_index + 1] += ((jump_label[corresponding_label_found].value & 0xff00) >> 8) + (instruction[i].arguments[argument_byte_index] >> 8);
                            instruction[i].arguments[argument_byte_index] &= 0x00ff;
                            argument_byte_index += 2;
                        } else {
                            instruction[i].arguments[argument_byte_index++] = jump_label[corresponding_label_found].value & 0x00ff;
                            instruction[i].arguments[argument_byte_index++] = (jump_label[corresponding_label_found].value & 0xff00) >> 8;
                        }
                    } else {
                        argument_byte_index += 2;
                    }
                    break;
                
                case EXPR_INDIRECT_REGISTER_OFFSET:
                    // here the first two arguments are guaranteed to be the label destination
                    if (instruction[i].expression[exp].tokens[1].type == TT_LABEL) {
                        // find corresponding label
                        int corresponding_label_found = -1;
                        for (int j = 0; j < jump_label_index; j++) {
                            if (strcmp(instruction[i].expression[exp].tokens[1].raw, jump_label[j].name) == 0) {
                                corresponding_label_found = j;
                                break;
                            }
                        }
                        if (corresponding_label_found == -1) {
                            log_msg(LP_ERROR, "Solving label: Unable to find corresponding label: \"%s\" [%s:%d]", instruction[i].expression[exp].tokens[1].raw, __FILE__, __LINE__);
                            exit(1);
                            break;
                        }
                        //log_msg(LP_INFO, "Solving label: Resolving \"%s\" to value 0x%.4x", jump_label[corresponding_label_found].name, jump_label[corresponding_label_found].value);
                        instruction[i].arguments[argument_byte_index++] = jump_label[corresponding_label_found].value & 0x00ff;
                        instruction[i].arguments[argument_byte_index++] = (jump_label[corresponding_label_found].value & 0xff00) >> 8;
                    } else {
                        argument_byte_index += 2;
                    }
                    break;
                
                case EXPR_INDIRECT_SCALE_OFFSET:
                    // here the first two arguments are guaranteed to be the label destination
                    if (instruction[i].expression[exp].tokens[1].type == TT_LABEL) {
                        // find corresponding label
                        int corresponding_label_found = -1;
                        for (int j = 0; j < jump_label_index; j++) {
                            if (strcmp(instruction[i].expression[exp].tokens[1].raw, jump_label[j].name) == 0) {
                                corresponding_label_found = j;
                                break;
                            }
                        }
                        if (corresponding_label_found == -1) {
                            log_msg(LP_ERROR, "Solving label: Unable to find corresponding label: \"%s\" [%s:%d]", instruction[i].expression[exp].tokens[1].raw, __FILE__, __LINE__);
                            exit(1);
                            break;
                        }
                        //log_msg(LP_INFO, "Solving label: Resolving \"%s\" to value 0x%.4x", jump_label[corresponding_label_found].name, jump_label[corresponding_label_found].value);
                        instruction[i].arguments[argument_byte_index++] = jump_label[corresponding_label_found].value & 0x00ff;
                        instruction[i].arguments[argument_byte_index++] = (jump_label[corresponding_label_found].value & 0xff00) >> 8;
                    } else {
                        argument_byte_index += 2;
                    }
                    if (instruction[i].expression[exp].tokens[3].type == TT_LABEL) {
                        // find corresponding label
                        int corresponding_label_found = -1;
                        for (int j = 0; j < jump_label_index; j++) {
                            if (strcmp(instruction[i].expression[exp].tokens[3].raw, jump_label[j].name) == 0) {
                                corresponding_label_found = j;
                                break;
                            }
                        }
                        if (corresponding_label_found == -1) {
                            log_msg(LP_ERROR, "Solving label: Unable to find corresponding label: \"%s\" [%s:%d]", instruction[i].expression[exp].tokens[3].raw, __FILE__, __LINE__);
                            exit(1);
                            break;
                        }
                        //log_msg(LP_INFO, "Solving label: Resolving \"%s\" to value 0x%.4x", jump_label[corresponding_label_found].name, jump_label[corresponding_label_found].value);
                        instruction[i].arguments[argument_byte_index++] = jump_label[corresponding_label_found].value & 0x00ff;
                        if (jump_label[corresponding_label_found].value > 0x00ff) {
                            log_msg(LP_WARNING, "Solving label: While resolving label, value for \"%s\" will be cutoff to 8 bits [%s:%d]", jump_label[corresponding_label_found].name, __FILE__, __LINE__);
                        }
                    } else {
                        argument_byte_index ++;
                    }
                    break;

                case EXPR_NONE:
                case EXPR_REGISTER:
                case EXPR_INSTRUCTION:
                case EXPR_INDIRECT_REGISTER:
                case EXPR_ADDRESS:
                case EXPR_SEGMENT_CODE:
                case EXPR_SEGMENT_DATA:
                case EXPR_RESERVE:
                case EXPR_INCBIN:
                case EXPR_TEXT_DEFINITION:
                // etc.
                    break;

                default:
                    log_msg(LP_WARNING, "Solving label: Non implemented label resolve for expression type %d at instruction index %d, expression index %d [%s:%d]", instruction[i].expression[exp].type, i, exp, __FILE__, __LINE__);
                    //exit(1);
                    break;
            }
        }
    }
    return instruction;
}

static void safe_write(uint8_t* bin, int index, uint8_t value, AssembleOption_t options) {
    if ((options & AO_PAD_SEGMENT_BREACH_WITH_ZERO) && index > SEGMENT_CODE_END) {
        return;
    }
    bin[index] = value;
}

uint8_t* assembler_parse_instruction(Instruction_t* instruction, int instruction_count, long* binary_size, AssembleOption_t options) {
    if (!instruction) {
        return NULL;
    }
    
    uint8_t* bin = calloc(0xffff, sizeof(uint8_t));
    uint8_t* written = calloc(0xffff, sizeof(uint8_t));
    int index = 0;
    int instruction_index = 0;

    //printf("instruction_count: %d\n", instruction_count);
    while (instruction_index < instruction_count) {
        if (instruction[instruction_index].is_address) {
            //log_msg(LP_INFO, "Parsing instruction: Jumping in address from 0x%.4x to 0x%.4x", index, instruction[instruction_index].address);
            index = instruction[instruction_index].address;
            instruction_index ++;
            if (index > *binary_size) {
                *binary_size = index;
            }
            continue;
        }
        if (instruction[instruction_index].expression[0].type == EXPR_SEGMENT_CODE) {
            instruction_index ++;
            continue;
        } 
        if (instruction[instruction_index].expression[0].type == EXPR_SEGMENT_DATA) {
            instruction_index ++;
            continue;
        } 
        if (instruction[instruction_index].expression[0].tokens[0].type == TT_LABEL) {
            instruction_index ++;
            continue;
        }
        if (instruction[instruction_index].expression[0].type == EXPR_INCBIN) {
            char* filename = calloc(strlen(instruction[instruction_index].expression[0].tokens[1].raw) - 1, 1);
            memcpy(filename, instruction[instruction_index].expression[0].tokens[1].raw + 1, strlen(instruction[instruction_index].expression[0].tokens[1].raw + 1));
            filename[strlen(filename) - 1] = '\0';
            //log_msg(LP_INFO, "filename: %s", filename);
            long filesize;
            uint8_t* content = (uint8_t*) read_file(filename, &filesize);
            for (long i = 0; i < filesize; i++) {
                if (written[index]) {
                    log_msg(LP_ERROR, "Parsing instruction: The inserted binary data is overlapping with existing code, likely due to missplaced \".address\" operations [%s:%d]", __FILE__, __LINE__);
                    log_msg(LP_INFO, "Parsing instruciton: Overlap occured at address 0x%.4x", index);
                    return NULL;
                }
                written[index] = 1;
                safe_write(bin, index++, content[i], options);
                //bin[index++] = content[i];
                if (index > *binary_size) {
                    *binary_size = index;
                }
            }
            instruction_index ++;
            continue;
        }
        // this here has to be checked last, else weird offsets occure
        if (instruction[instruction_index].is_raw_data) {
            //printf("is raw data! %d\n", instruction_index);
            if (instruction[instruction_index].byte_aligned) {
                //printf("aligned!\n");
                written[index] = 1;
                //bin[index++] = instruction[instruction_index].raw_data & 0x00ff;
                safe_write(bin, index++, instruction[instruction_index].raw_data & 0x00ff, options);
                written[index] = 1;
                //bin[index++] = (instruction[instruction_index].raw_data & 0xff00) >> 8;
                safe_write(bin, index++, instruction[instruction_index].raw_data & 0x00ff, options);
                instruction_index ++;
                if (index > *binary_size) {
                    *binary_size = index;
                }
            } else {
                //printf("unaligned!\n");
                written[index] = 1;
                //bin[index++] = instruction[instruction_index].raw_data & 0x00ff;
                safe_write(bin, index++, instruction[instruction_index].raw_data & 0x00ff, options);
                instruction_index ++;
                if (index > *binary_size) {
                    *binary_size = index;
                }
            }
            continue;
        }
        if (written[index]) {
            log_msg(LP_ERROR, "Parsing instruction: The machine code produced is overlapping with existing code, likely due to missplaced \".address\" operations [%s:%d]", __FILE__, __LINE__);
            log_msg(LP_INFO, "Parsing instruciton: Overlap occured at address 0x%.4x", index);
            return NULL;
        }
        written[index] = 1;
        //bin[index++] = instruction[instruction_index].instruction | ((instruction[instruction_index].no_cache != 0) << 7);
        safe_write(bin, index++, instruction[instruction_index].instruction | ((instruction[instruction_index].no_cache != 0) << 7), options);
        written[index] = 1;
        //bin[index++] = instruction[instruction_index].admr | (instruction[instruction_index].admx << 3);
        safe_write(bin, index++, instruction[instruction_index].admr | (instruction[instruction_index].admx << 3), options);
        for (int i = 0; i < instruction[instruction_index].argument_bytes; i++) {
            written[index] = 1;
            //bin[index++] = (uint8_t) instruction[instruction_index].arguments[i];
            safe_write(bin, index++, (uint8_t) instruction[instruction_index].arguments[i], options);
        }
        instruction_index ++;
        if (index > *binary_size) {
            *binary_size = index;
        }
    }

    free(written);

    //*binary_size = 0xffff; //index;
    //bin = realloc(bin, (size_t) index);
    return bin;
}


uint8_t* assembler_compile(char* content, long* binary_size, uint16_t** segment, int* segment_count, AssembleOption_t options) {
    // char* content - holds the raw assembly in text form
    // long* binary_size - deref will be overwritten with the final binary size in bytes
    // uint16_t** segment - will be allocated to hold the segment offsets
    // int* segment_count - deref will be overwritten with the final segment count
    // usually each variable should be stack allocated and a reference should be passed. 
    if (!binary_size) {
        log_msg(LP_ERROR, "binary_size is a required argument for assembler_compile [%s:%d]", __FILE__, __LINE__);
        return NULL;
    }
    *binary_size = 0;
    if (segment_count && segment) {
        *segment_count = 0;
        *segment = malloc(sizeof(uint16_t));
    }

    jump_label_index = 0;
    const_label_index = 0;

    // split text into lines
    char** line = assembler_split_to_separate_lines(content);
    //int index = 0;
    //while (line[index]) {
        //printf("%s\n", line[index++]);
    //}

    // remove comments from lines
    assembler_remove_comments(&line);
    //index = 0;
    //while (line[index]) {
        //printf("%s\n", line[index++]);
    //}

    // split into separate words
    int word_count;
    char** word = assembler_split_to_words(line, &word_count);

    // tokenize
    int token_count = 0;
    Token_t* token = assembler_parse_words(word, word_count, &token_count);
    for (int i = 0; i < token_count; i++) {
        //printf("Token %d: [%s], \"%s\"\n", i + 1, (int) token[i].type == -1 ? "?" : token_type_string[token[i].type], token[i].raw);
    }

    // build expression array
    int expression_count = 0;
    Expression_t* expression = assembler_parse_token(token, token_count, &expression_count);
    for (int i = 0; i < expression_count; i++) {
        //printf("Expression %d: \"%s\"\n", i + 1, (int) expression[i].type == -1 ? "?" : expression_type_string[expression[i].type]);
    }

    // ToDo, resolve all include and incbin directives, then restart

    // build instruction array
    int instruction_count = 0;
    Instruction_t* instruction = assembler_parse_expression(expression, expression_count, &instruction_count, segment, segment_count, options);
    for (int i = 0; i < instruction_count; i++) {
        /*if ((int) instruction[i].instruction >= 0) {
            printf("Instruction %d: \"%s\" [pos: %d] - ", i + 1, cpu_instruction_string[instruction[i].instruction], instruction[i].address);
            for (int e = 0; e < instruction[i].expression_count; e++) {
                for (int t = 0; t < instruction[i].expression[e].token_count; t++) {
                    printf("%s ", instruction[i].expression[e].tokens[t].raw);
                }
            }
            printf("\n");
        } else {
            printf("Instruction %d: \"Not an instr.\"\n", i + 1);
        }*/
    }

    // resolve label
    instruction = assembler_resolve_labels(instruction, instruction_count);

    // to machine code
    uint8_t* machine_code = assembler_parse_instruction(instruction, instruction_count, binary_size, options);


    // free

    int windex = 0;
    while (word[windex]) {
        free(word[windex++]);
    }
    free(word);
    free(token);
    free(expression);
    free(instruction);


    return machine_code;
}

uint8_t* assembler_compile_from_file(const char* filename, long* binary_size, uint16_t** segment, int* segment_count, AssembleOption_t options) {
    // load raw text from file
    char* content = read_file(filename, binary_size);
    if (!content) {
        log_msg(LP_ERROR, "read_file failed [%s:%d]", __FILE__, __LINE__);
        return NULL;
    }
    uint8_t* machine_code = assembler_compile(content, binary_size, segment, segment_count, options);
    
    return machine_code;
}
