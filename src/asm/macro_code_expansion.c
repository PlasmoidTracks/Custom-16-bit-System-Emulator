#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "utils/Log.h"
#include "utils/IO.h"
#include "utils/String.h"

#include "cpu/cpu_addressing_modes.h"
#include "cpu/cpu_instructions.h"

#include "asm/assembler.h"

#include "asm/macro_code_expansion.h"


static int is_same_adm(CPU_REDUCED_ADDRESSING_MODE_t admr, CPU_EXTENDED_ADDRESSING_MODE_t admx) {
    int result = 0;
    result |= (admr == ADMR_R0 && admx == ADMX_R0);
    result |= (admr == ADMR_R1 && admx == ADMX_R1);
    result |= (admr == ADMR_R2 && admx == ADMX_R2);
    result |= (admr == ADMR_R3 && admx == ADMX_R3);
    result |= (admr == ADMR_SP && admx == ADMX_SP);
    result |= (admr == ADMR_IND_R0 && admx == ADMX_IND_R0);
    return result;
}

static int is_same_indirect_adm(CPU_REDUCED_ADDRESSING_MODE_t admr, CPU_EXTENDED_ADDRESSING_MODE_t admx) {
    int result = 0;
    result |= (admr == ADMR_R0 && admx == ADMX_IND_R0);
    result |= (admr == ADMR_R1 && admx == ADMX_IND_R1);
    result |= (admr == ADMR_R2 && admx == ADMX_IND_R2);
    result |= (admr == ADMR_R3 && admx == ADMX_IND_R3);
    result |= (admr == ADMR_SP && admx == ADMX_IND_SP);
    return result;
}

static int is_register_admx(CPU_EXTENDED_ADDRESSING_MODE_t admx) {
    int result = 0;
    result |= admx == ADMX_R0;
    result |= admx == ADMX_R1;
    result |= admx == ADMX_R2;
    result |= admx == ADMX_R3;
    result |= admx == ADMX_SP;
    result |= admx == ADMX_PC;
    return result;
}

static int is_register_admr(CPU_REDUCED_ADDRESSING_MODE_t admr) {
    int result = 0;
    result |= admr == ADMR_R0;
    result |= admr == ADMR_R1;
    result |= admr == ADMR_R2;
    result |= admr == ADMR_R3;
    result |= admr == ADMR_SP;
    return result;
}

static int is_register_offset_admx(CPU_EXTENDED_ADDRESSING_MODE_t admx) {
    int result = 0;
    result |= admx == ADMX_IND_R0_OFFSET16;
    result |= admx == ADMX_IND_R1_OFFSET16;
    result |= admx == ADMX_IND_R2_OFFSET16;
    result |= admx == ADMX_IND_R3_OFFSET16;
    result |= admx == ADMX_IND_SP_OFFSET16;
    result |= admx == ADMX_IND_PC_OFFSET16;
    return result;
}

static int register_offset_admx_contains_admr_register(CPU_EXTENDED_ADDRESSING_MODE_t admx, CPU_REDUCED_ADDRESSING_MODE_t admr) {
    int result = 0;
    result |= (admr == ADMR_R0 && admx == ADMX_IND_R0_OFFSET16);
    result |= (admr == ADMR_R1 && admx == ADMX_IND_R1_OFFSET16);
    result |= (admr == ADMR_R2 && admx == ADMX_IND_R2_OFFSET16);
    result |= (admr == ADMR_R3 && admx == ADMX_IND_R3_OFFSET16);
    result |= (admr == ADMR_SP && admx == ADMX_IND_SP_OFFSET16);
    return result;
}

static int is_arithmetic_operation(CPU_INSTRUCTION_MNEMONIC_t instr) {
    int result = 0;
    result |= instr == ADD;
    result |= instr == SUB;
    result |= instr == MUL;
    result |= instr == DIV;
    result |= instr == ADDF;
    result |= instr == SUBF;
    result |= instr == MULF;
    result |= instr == DIVF;
    result |= instr == BWS;
    result |= instr == AND;
    result |= instr == OR;
    result |= instr == XOR;
    return result;
}

static int is_overwriting_instruction(CPU_INSTRUCTION_MNEMONIC_t instr) {
    // Meaning, the instruction does not depend on the previous value and is overwriting with new value
    int result = 0;
    result |= instr == MOV;
    result |= instr == LEA;
    return result;
}


static void remove_instruction(Instruction_t* instruction, int* instruction_count, int index) {
    if (index < 0 || index >= *instruction_count) return;
    for (int i = index; i < *instruction_count - 1; i++) {
        instruction[i] = instruction[i + 1];
    }
    (*instruction_count)--;
}

static void insert_instruction(Instruction_t* instruction, Instruction_t new_instruction, int* instruction_count, int index) {
    instruction = realloc(instruction, sizeof(Instruction_t) * (*instruction_count + 1));
    if (index < 0 || index >= *instruction_count) return;
    for (int i = *instruction_count; i > index; i--) {
        instruction[i] = instruction[i - 1];
    }
    instruction[index] = new_instruction;
    (*instruction_count)++;
}

char* macro_code_expand(char* content) {
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
    
    int changes_applied = 1;
    while (changes_applied) {
        changes_applied = 0;
        /*for (int j = 0; j < instruction_count; j++) {
            if (instruction[j].instruction < 0 || instruction[j].instruction >= INSTRUCTION_COUNT) continue;
            printf("Instruction %d: \"%s\" %s, %s\n", j + 1, cpu_instruction_string[instruction[j].instruction], cpu_reduced_addressing_mode_string[instruction[j].admr], cpu_extended_addressing_mode_string[instruction[j].admx]);
        }*/

        // Optimize
        for (int i = 0; i < instruction_count; i++) {

            // mov r0, r0
            // =>
            // (removed)
            if (instruction[i].instruction == MOV) {
                if (is_same_adm(instruction[i].admr, instruction[i].admx)) {
                    //log_msg(LP_DEBUG, "Optimizer: removed self assigning mov instruction (line %d)", i);
                    remove_instruction(instruction, &instruction_count, i);
                    i --;
                    changes_applied = 1;
                }
            }
            if (changes_applied) break;

        }
    }

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
            } else if (instr.expression[0].tokens[0].type == TT_LABEL) {
                //log_msg(LP_INFO, "Label found");
                output = append_to_output(output, &output_len, instr.expression[0].tokens[0].raw);
                output = append_to_output(output, &output_len, " ");
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


char* macro_code_expand_from_file(char* filename) {
    char* content = read_file(filename, NULL);
    if (!content) {
        log_msg(LP_ERROR, "read_file failed");
        return NULL;
    }
    char* new_code = macro_code_expand(content);

    return new_code;
}

