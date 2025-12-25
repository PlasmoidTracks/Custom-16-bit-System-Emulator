#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "utils/Log.h"
#include "utils/IO.h"
#include "utils/String.h"

#include "cpu/cpu_addressing_modes.h"

#include "asm/assembler.h"


char* canonicalizer_compile(char* content) {
    char* output = NULL;         // final assembly string
    long output_len = 0;         // current length of code output

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

    // build instruction array
    int instruction_count = 0;
    Instruction_t* instruction = assembler_parse_expression(expression, expression_count, &instruction_count, NULL, 0);
    

    // reconstruct asm file
    for (int i = 0; i < instruction_count; i++) {
        Instruction_t instr = instruction[i];
        int ec = instr.expression_count;
        for (int j = 0; j < ec; j++) {
            if (instr.is_address) {
                char tmp[32];
                sprintf(tmp, ".address $%.4x", instr.address);
                output = append_to_output(output, &output_len, tmp);
            } else if (instr.expression[0].type == EXPR_SEGMENT_CODE) {
                output = append_to_output(output, &output_len, ".code");
            } else if (instr.expression[0].type == EXPR_SEGMENT_DATA) {
                output = append_to_output(output, &output_len, ".data");
            } else if (instr.expression[0].type == EXPR_INCLUDE) {
                output = append_to_output(output, &output_len, ".include ");
                output = append_to_output(output, &output_len, instr.expression[0].tokens[1].raw);
            } else if (instr.expression[0].type == EXPR_INCBIN) {
                output = append_to_output(output, &output_len, ".incbin ");
                output = append_to_output(output, &output_len, instr.expression[0].tokens[1].raw);
            } else if (instr.expression[0].tokens[0].type == TT_LABEL) {
                //log_msg(LP_INFO, "Label found");
                output = append_to_output(output, &output_len, instr.expression[0].tokens[0].raw);
                output = append_to_output(output, &output_len, " ");
            } else {
                // here if instruction
                if (  (instr.admx == ADMX_IND_R0_OFFSET16 
                    || instr.admx == ADMX_IND_R1_OFFSET16 
                    || instr.admx == ADMX_IND_R2_OFFSET16 
                    || instr.admx == ADMX_IND_R3_OFFSET16 
                    || instr.admx == ADMX_IND_SP_OFFSET16 
                    || instr.admx == ADMX_IND_PC_OFFSET16)
                    && j == 2
                ) {
                    // [ $xxxx + reg ]
                    if (instr.expression[j].token_count > 4 &&
                        instr.expression[j].tokens[0].type == TT_BRACKET_OPEN &&
                        (instr.expression[j].tokens[1].type == TT_IMMEDIATE || instr.expression[j].tokens[1].type == TT_LABEL) &&
                        instr.expression[j].tokens[2].type == TT_PLUS &&
                        instr.expression[j].tokens[3].type == TT_REGISTER &&
                        instr.expression[j].tokens[4].type == TT_BRACKET_CLOSE
                    ) {
                        output = append_to_output(output, &output_len, "[");
                        uint16_t imm = parse_immediate(instr.expression[j].tokens[1].raw);
                        char tmp[16];
                        sprintf(tmp, "$%.4X", imm);
                        output = append_to_output(output, &output_len, tmp);
                        output = append_to_output(output, &output_len, " + ");
                        output = append_to_output(output, &output_len, instr.expression[j].tokens[3].raw);
                        output = append_to_output(output, &output_len, "]");
                    }

                    // [ reg + $xxxx ]
                    else if (instr.expression[j].token_count > 4 &&
                        instr.expression[j].tokens[0].type == TT_BRACKET_OPEN &&
                        instr.expression[j].tokens[3].type == TT_REGISTER &&
                        instr.expression[j].tokens[2].type == TT_PLUS &&
                        (instr.expression[j].tokens[1].type == TT_IMMEDIATE || instr.expression[j].tokens[1].type == TT_LABEL) &&
                        instr.expression[j].tokens[4].type == TT_BRACKET_CLOSE
                    ) {
                        output = append_to_output(output, &output_len, "[");
                        uint16_t imm = parse_immediate(instr.expression[j].tokens[3].raw);
                        char tmp[16];
                        sprintf(tmp, "$%.4X", imm);
                        output = append_to_output(output, &output_len, tmp);
                        output = append_to_output(output, &output_len, " + ");
                        output = append_to_output(output, &output_len, instr.expression[j].tokens[1].raw);
                        output = append_to_output(output, &output_len, "]");
                    }

                    else {
                        output = append_to_output(output, &output_len, "; UNKNOWN\n");
                        log_msg(LP_ERROR, "Canonicalizer: Unknown configuration for indirect register with 16-bit offset [%s:%d]", __FILE__, __LINE__);
                    }
                } else if ((instr.admx == ADMX_IND16_SCALED8_R0_OFFSET
                    || instr.admx == ADMX_IND16_SCALED8_R1_OFFSET
                    || instr.admx == ADMX_IND16_SCALED8_R2_OFFSET
                    || instr.admx == ADMX_IND16_SCALED8_R3_OFFSET
                    || instr.admx == ADMX_IND16_SCALED8_SP_OFFSET
                    || instr.admx == ADMX_IND16_SCALED8_PC_OFFSET)
                    && j == 2
                ) {
                    // [ $xxxx + $xx * reg ]
                    if (instr.expression[j].token_count > 4 &&
                        instr.expression[j].tokens[0].type == TT_BRACKET_OPEN &&
                        (instr.expression[j].tokens[1].type == TT_IMMEDIATE || instr.expression[j].tokens[1].type == TT_LABEL) &&
                        instr.expression[j].tokens[2].type == TT_PLUS &&
                        (instr.expression[j].tokens[3].type == TT_IMMEDIATE || instr.expression[j].tokens[3].type == TT_LABEL) &&
                        instr.expression[j].tokens[4].type == TT_STAR &&
                        instr.expression[j].tokens[5].type == TT_REGISTER &&
                        instr.expression[j].tokens[6].type == TT_BRACKET_CLOSE
                    ) {
                        output = append_to_output(output, &output_len, "[");
                        uint16_t imm = parse_immediate(instr.expression[j].tokens[1].raw);
                        char tmp[16];
                        sprintf(tmp, "$%.4X", imm);
                        output = append_to_output(output, &output_len, tmp);
                        output = append_to_output(output, &output_len, " + ");
                        imm = parse_immediate(instr.expression[j].tokens[3].raw);
                        sprintf(tmp, "$%.2X", imm);
                        output = append_to_output(output, &output_len, tmp);
                        output = append_to_output(output, &output_len, " * ");
                        output = append_to_output(output, &output_len, instr.expression[j].tokens[5].raw);
                        output = append_to_output(output, &output_len, "]");
                    }
                    // [ $xxxx + reg * $xx ]
                    else if (instr.expression[j].token_count > 4 &&
                        instr.expression[j].tokens[0].type == TT_BRACKET_OPEN &&
                        (instr.expression[j].tokens[1].type == TT_IMMEDIATE || instr.expression[j].tokens[1].type == TT_LABEL) &&
                        instr.expression[j].tokens[2].type == TT_PLUS &&
                        instr.expression[j].tokens[5].type == TT_REGISTER &&
                        instr.expression[j].tokens[4].type == TT_STAR &&
                        (instr.expression[j].tokens[3].type == TT_IMMEDIATE || instr.expression[j].tokens[3].type == TT_LABEL) &&
                        instr.expression[j].tokens[6].type == TT_BRACKET_CLOSE
                    ) {
                        output = append_to_output(output, &output_len, "[");
                        uint16_t imm = parse_immediate(instr.expression[j].tokens[1].raw);
                        char tmp[16];
                        sprintf(tmp, "$%.4X", imm);
                        output = append_to_output(output, &output_len, tmp);
                        output = append_to_output(output, &output_len, " + ");
                        imm = parse_immediate(instr.expression[j].tokens[5].raw);
                        sprintf(tmp, "$%.2X", imm);
                        output = append_to_output(output, &output_len, tmp);
                        output = append_to_output(output, &output_len, " * ");
                        output = append_to_output(output, &output_len, instr.expression[j].tokens[3].raw);
                        output = append_to_output(output, &output_len, "]");
                    }
                    // [ $xx * reg + $xxxx ]
                    else if (instr.expression[j].token_count > 4 &&
                        instr.expression[j].tokens[0].type == TT_BRACKET_OPEN &&
                        (instr.expression[j].tokens[1].type == TT_IMMEDIATE || instr.expression[j].tokens[1].type == TT_LABEL) &&
                        instr.expression[j].tokens[2].type == TT_STAR &&
                        instr.expression[j].tokens[5].type == TT_REGISTER &&
                        instr.expression[j].tokens[4].type == TT_PLUS &&
                        (instr.expression[j].tokens[3].type == TT_IMMEDIATE || instr.expression[j].tokens[3].type == TT_LABEL) &&
                        instr.expression[j].tokens[6].type == TT_BRACKET_CLOSE
                    ) {
                        output = append_to_output(output, &output_len, "[");
                        uint16_t imm = parse_immediate(instr.expression[j].tokens[5].raw);
                        char tmp[16];
                        sprintf(tmp, "$%.4X", imm);
                        output = append_to_output(output, &output_len, tmp);
                        output = append_to_output(output, &output_len, " + ");
                        imm = parse_immediate(instr.expression[j].tokens[1].raw);
                        sprintf(tmp, "$%.2X", imm);
                        output = append_to_output(output, &output_len, tmp);
                        output = append_to_output(output, &output_len, " * ");
                        output = append_to_output(output, &output_len, instr.expression[j].tokens[3].raw);
                        output = append_to_output(output, &output_len, "]");
                    }
                    // [ reg * $xx + $xxxx ]
                    else if (instr.expression[j].token_count > 4 &&
                        instr.expression[j].tokens[0].type == TT_BRACKET_OPEN &&
                        instr.expression[j].tokens[5].type == TT_REGISTER &&
                        instr.expression[j].tokens[2].type == TT_STAR &&
                        (instr.expression[j].tokens[3].type == TT_IMMEDIATE || instr.expression[j].tokens[3].type == TT_LABEL) &&
                        instr.expression[j].tokens[4].type == TT_PLUS &&
                        (instr.expression[j].tokens[1].type == TT_IMMEDIATE || instr.expression[j].tokens[1].type == TT_LABEL) &&
                        instr.expression[j].tokens[6].type == TT_BRACKET_CLOSE
                    ) {
                        output = append_to_output(output, &output_len, "[");
                        uint16_t imm = parse_immediate(instr.expression[j].tokens[5].raw);
                        char tmp[16];
                        sprintf(tmp, "$%.4X", imm);
                        output = append_to_output(output, &output_len, tmp);
                        output = append_to_output(output, &output_len, " + ");
                        imm = parse_immediate(instr.expression[j].tokens[3].raw);
                        sprintf(tmp, "$%.2X", imm);
                        output = append_to_output(output, &output_len, tmp);
                        output = append_to_output(output, &output_len, " * ");
                        output = append_to_output(output, &output_len, instr.expression[j].tokens[1].raw);
                        output = append_to_output(output, &output_len, "]");
                    }

                    else {
                        output = append_to_output(output, &output_len, "; UNKNOWN\n");
                        log_msg(LP_ERROR, "Canonicalizer: Unknown configuration for scaled indirect register with offset [%s:%d]", __FILE__, __LINE__);
                    }
                } else if (  (
                    instr.admx == ADMX_IND16
                    || instr.admx == ADMX_IMM16)
                    && j == 2
                ) {
                    // $xxxx
                    if (
                        instr.expression[j].tokens[0].type == TT_IMMEDIATE
                    ) {
                        uint16_t imm = parse_immediate(instr.expression[j].tokens[0].raw);
                        char tmp[16];
                        sprintf(tmp, "$%.4X", imm);
                        output = append_to_output(output, &output_len, tmp);
                    } else if (
                        instr.expression[j].tokens[0].type == TT_LABEL
                    ) {
                        output = append_to_output(output, &output_len, instr.expression[j].tokens[0].raw);
                    }
                    // [ $xxxx ]
                    else if (instr.expression[j].token_count > 2 &&
                        instr.expression[j].tokens[0].type == TT_BRACKET_OPEN &&
                        instr.expression[j].tokens[1].type == TT_IMMEDIATE &&
                        instr.expression[j].tokens[2].type == TT_BRACKET_CLOSE
                    ) {
                            output = append_to_output(output, &output_len, "[");
                            uint16_t imm = parse_immediate(instr.expression[j].tokens[1].raw);
                            char tmp[16];
                            sprintf(tmp, "$%.4X", imm);
                            output = append_to_output(output, &output_len, tmp);
                            output = append_to_output(output, &output_len, "]");
                    } else if (instr.expression[j].token_count > 2 &&
                        instr.expression[j].tokens[0].type == TT_BRACKET_OPEN &&
                        instr.expression[j].tokens[1].type == TT_LABEL &&
                        instr.expression[j].tokens[2].type == TT_BRACKET_CLOSE
                    ) {
                            output = append_to_output(output, &output_len, "[");
                            output = append_to_output(output, &output_len, instr.expression[j].tokens[1].raw);
                            output = append_to_output(output, &output_len, "]");
                    }
                } else {
                    int tc = instr.expression[j].token_count;
                    for (int k = 0; k < tc; k++) {
                        output = append_to_output(output, &output_len, instr.expression[j].tokens[k].raw);
                        if (k > 0 && k < tc - 2) {
                            output = append_to_output(output, &output_len, " ");
                        }
                    }
                    if (j == 1 && ec > 2) {
                        output = append_to_output(output, &output_len, ",");
                    }
                    if (j < ec-1) {
                        output = append_to_output(output, &output_len, " ");
                    }
                }
            }
        }
        output = append_to_output(output, &output_len, "\n");
    }

    // free
    int windex = 0;
    while (word[windex]) {
        free(word[windex++]);
    }
    free(word);
    free(token);
    free(expression);
    free(instruction);

    return output;
}

char* canonicalizer_compile_from_file(const char* filename) {
    // load raw text from file
    char* content = read_file(filename, NULL);
    if (!content) {
        log_msg(LP_ERROR, "Canonicalizer: read_file failed [%s:%d]", __FILE__, __LINE__);
        return NULL;
    }
    char* machine_code = canonicalizer_compile(content);
    
    return machine_code;
}
