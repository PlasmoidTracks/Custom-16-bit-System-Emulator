#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "utils/ExtendedTypes.h"
#include "utils/Log.h"
#include "utils/IO.h"
#include "utils/String.h"

#include "cpu/cpu_addressing_modes.h"
#include "cpu/cpu_instructions.h"

#include "compiler/asm/optimizer.h"
#include "compiler/asm/assembler.h"


#define OPT_DEBUG
#undef OPT_DEBUG

/*
CRITICAL BUG: Labels inside .data segments are not present in the output!!!
*/

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
    result |= instr == INC;
    result |= instr == DEC;
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

static int is_move_operator(CPU_INSTRUCTION_MNEMONIC_t instr) {
    int result = 0;
    result |= instr == MOV;
    result |= instr == CMOVZ;
    result |= instr == CMOVNZ;
    result |= instr == CMOVFZ;
    result |= instr == CMOVNFZ;
    result |= instr == CMOVL;
    result |= instr == CMOVNL;
    result |= instr == CMOVUL;
    result |= instr == CMOVNUL;
    result |= instr == CMOVFL;
    result |= instr == CMOVNFL;
    result |= instr == CMOVBL;
    result |= instr == CMOVNBL;
    result |= instr == CMOVAO;
    result |= instr == CMOVNAO;
    result |= instr == CMOVMI;
    result |= instr == CMOVNMI;
    return result;
}


static void remove_instruction(Instruction_t* instruction, int* instruction_count, int index) {
    if (index < 0 || index >= *instruction_count) return;
    for (int i = index; i < *instruction_count - 1; i++) {
        instruction[i] = instruction[i + 1];
    }
    (*instruction_count)--;
}

static void insert_instruction(Instruction_t** instruction, Instruction_t new_instruction, int* instruction_count, int index) {
    *instruction = realloc(*instruction, sizeof(Instruction_t) * (*instruction_count + 2));
    if (index < 0 || index >= *instruction_count) return;
    for (int i = *instruction_count; i > index; i--) {
        (*instruction)[i] = (*instruction)[i - 1];
    }
    (*instruction)[index] = new_instruction;
    (*instruction_count)++;
}

char* optimizer_compile(char* content) {
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
    Instruction_t* instruction = assembler_parse_expression(expression, expression_count, &instruction_count, NULL, 0, 0);
    
    int changes_applied = 1;
    //int changes_total = 0;
    while (changes_applied) {
        changes_applied = 0;
        //changes_total ++;
        //if (changes_total == 0) {break;}
        //log_msg(LP_DEBUG, "changes_total: %d", changes_total);
        /*for (int j = 0; j < instruction_count; j++) {
            if (instruction[j].instruction < 0 || instruction[j].instruction >= INSTRUCTION_COUNT) continue;
            //printf("Instruction %d: \"%s\" %s, %s\n", j + 1, cpu_instruction_string[instruction[j].instruction], cpu_reduced_addressing_mode_string[instruction[j].admr], cpu_extended_addressing_mode_string[instruction[j].admx]);
        }*/
        /*for (int j = 0; j < instruction_count; j++) {
            for (int e = 0; e < instruction[j].expression_count; e++) {
                for (int t = 0; t < instruction[j].expression[e].token_count; t++) {
                    printf("%s ", instruction[j].expression[e].tokens[t].raw);
                }
            }
            printf("\n");
        }*/

        // Optimize
        for (int i = 0; i < instruction_count; i++) {


            // ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
            // :::::::::::::::::::::::::::::::::::::::::::::: O2 optimizations ::::::::::::::::::::::::::::::::::::::::::::::
            // ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
            // O1 optimizations come after O2 optimizations


            // TODO: Make it look over multiple instructions, that do not modify THAT register
            // TODO: check for nc
            // mov/lea rN, x
            // mov/lea rN, y
            // =>
            // (removed)
            // mov/lea rN, y
            if (i < instruction_count - 1) {
                if (instruction[i].instruction == MOV || instruction[i].instruction == LEA) {
                    if (is_register_admr(instruction[i].admr)) {
                        CPU_REDUCED_ADDRESSING_MODE_t admr = instruction[i].admr;
                        int j = i + 1;
                        while (j < instruction_count - 1) {
                            if (instruction[j].instruction == CALL || 
                                instruction[j].instruction == RET || 
                                (!(instruction[j].instruction == MOV || instruction[j].instruction == LEA) && instruction[j].admr == admr) ||
                                cpu_instruction_is_jump[instruction[j].instruction] ||
                                (admr == ADMR_R0 && instruction[j].admr == ADMR_IND_R0) ||
                                is_same_adm(admr, instruction[j].admx) ||
                                (instruction[j].admx == ADMX_IMM16 && instruction[j].expression[2].tokens[0].type == TT_LABEL) ||
                                (instruction[j].admr == admr && (is_same_indirect_adm(admr, instruction[j].admx) || register_offset_admx_contains_admr_register(instruction[j].admx, admr))) ||
                                instruction[j].instruction == HLT ||
                                (is_arithmetic_operation(instruction[j].instruction) && instruction[j].admr == admr) ||
                                register_offset_admx_contains_admr_register(instruction[j].admx, admr)
                            ) {
                                    break;
                            }
                            if (instruction[j].instruction == MOV || instruction[j].instruction == LEA) {
                                if (instruction[j].admr == admr) {
                                    remove_instruction(instruction, &instruction_count, i);
                                    i --;
                                    changes_applied = 1;
                                    #ifdef OPT_DEBUG
                                        log_msg(LP_DEBUG, "Optimizer: applied {mov rN, x; ...(%d); mov rN, y} => {-; ...; mov rN, y} [%s:%d]", j - i, __FILE__, __LINE__);
                                    #endif
                                    break;
                                }
                            }
                            if (instruction[j].instruction == POP) {
                                if (instruction[j].admr == admr) {
                                    remove_instruction(instruction, &instruction_count, i);
                                    i --;
                                    changes_applied = 1;
                                    #ifdef OPT_DEBUG
                                        log_msg(LP_DEBUG, "Optimizer: applied {mov rN, x; ...(%d); pop rN} => {-; ...; pop rN} [%s:%d]", j - i, __FILE__, __LINE__);
                                    #endif
                                    break;
                                }
                            }
                            j++;
                        }
                    }
                }
            }
            if (changes_applied) break;
            

            // mov r0, $xyz
            // mov [r0], ?
            // =>
            // mov r0, $xyz
            // mov [$xyz], ?
            if (i < instruction_count - 2) {
                if (instruction[i].instruction == MOV && instruction[i + 1].instruction == MOV) {
                    if (instruction[i].admr == ADMR_R0 && instruction[i + 1].admr == ADMR_IND_R0 && instruction[i].admx == ADMX_IMM16) {
                        //if (instruction[i].expression[2].tokens[0].type != TT_LABEL) {
                            instruction[i + 1].admr = ADMR_IND16;
                            sprintf(instruction[i + 1].expression[1].tokens[1].raw, "%s", instruction[i].expression[2].tokens[0].raw);
                            changes_applied = 1;
                            #ifdef OPT_DEBUG
                                log_msg(LP_DEBUG, "Optimizer: applied {mov r0, $xyz; mov [r0], ?} => {mov r0, $xyz; mov [$xyz], ?} [%s:%d]", __FILE__, __LINE__);
                            #endif
                        //}
                    }
                }
            }
            if (changes_applied) break;
            

            // mov rN, $0
            // add rN, rM
            // =>
            // (removed)
            // mov rN, rM
            if (i < instruction_count - 1) {
                if (instruction[i].instruction == MOV && instruction[i + 1].instruction == ADD) {
                    if (instruction[i].admx == ADMX_IMM16 && instruction[i].expression[2].tokens[0].type != TT_LABEL) {
                        if (parse_immediate(instruction[i].expression[2].tokens[0].raw) == 0) {
                            if (instruction[i].admr == instruction[i + 1].admr) {
                                instruction[i + 1].instruction = MOV;
                                sprintf(instruction[i + 1].expression[0].tokens[0].raw, "%s", cpu_instruction_string[MOV]);
                                remove_instruction(instruction, &instruction_count, i);
                                i --;
                                changes_applied = 1;
                                #ifdef OPT_DEBUG
                                    log_msg(LP_DEBUG, "Optimizer: applied {mov rN, 0; add rN, rM} => {-; mov rN, rM} [%s:%d]", __FILE__, __LINE__);
                                #endif // OPT_DEBUG
                            }
                        }
                    }
                }
            }
            if (changes_applied) break;
            

            // mov rN, rM
            // mov rM, rN
            // =>
            // mov rN, rM
            // (removed)
            if (i < instruction_count - 1) {
                if (instruction[i].instruction == MOV && instruction[i + 1].instruction == MOV) {
                    if (is_same_adm(instruction[i].admr, instruction[i + 1].admx) && is_same_adm(instruction[i + 1].admr, instruction[i].admx)) {
                        remove_instruction(instruction, &instruction_count, i + 1);

                        #ifdef OPT_DEBUG
                            log_msg(LP_DEBUG, "Optimizer: applied {mov rN, rM; mov rM, rN} => {mov rN, rM; -} [%s:%d]", __FILE__, __LINE__);
                        #endif // OPT_DEBUG
                        changes_applied = 1;
                    }
                }
            }
            if (changes_applied) break;


            // mov rN, $xyz
            // mov rN, [rN]
            // ->
            // mov rN, [$xyz]
            
            

            // mov r0, 0
            // add ?, r0
            // =>
            // mov r0, 0
            // (removed)
            if (i < instruction_count - 1) {
                if (instruction[i].instruction == MOV && instruction[i + 1].instruction == ADD) {
                    if (instruction[i].admx == ADMX_IMM16) {
                        if (parse_immediate(instruction[i].expression[2].tokens[0].raw) == 0) {
                            if (is_same_adm(instruction[i].admr, instruction[i + 1].admx)) {
                                //log_msg(LP_DEBUG, "Optimizer: removed additive identity from register operation (line %d)", i);
                                remove_instruction(instruction, &instruction_count, i + 1);
                                changes_applied = 1;
                                #ifdef OPT_DEBUG
                                    log_msg(LP_DEBUG, "Optimizer: applied {mov rN, 0; add x, r0} => {mov rN, 0; -} [%s:%d]", __FILE__, __LINE__);
                                #endif // OPT_DEBUG
                            }
                        }
                    }
                }
            }
            if (changes_applied) break;
            

            // lea rN, [???]
            // mov rN, [rN]
            // =>
            // mov rN, [???]
            // (removed)
            if (i < instruction_count - 1) {
                if (instruction[i].instruction == LEA && instruction[i + 1].instruction == MOV) {
                    if (instruction[i].admr == instruction[i + 1].admr && is_same_indirect_adm(instruction[i + 1].admr, instruction[i + 1].admx)) {
                        //log_msg(LP_DEBUG, "Optimizer: replaced two step dereference with one step dereference (line %d)", i);
                        sprintf(instruction[i].expression[0].tokens[0].raw, "%s", cpu_instruction_string[MOV]);
                        instruction[i].instruction = MOV;
                        remove_instruction(instruction, &instruction_count, i + 1);
                        changes_applied = 1;
                        #ifdef OPT_DEBUG
                            log_msg(LP_DEBUG, "Optimizer: applied {lea rN, [x]; mov rN, [rN]} => {mov rN, [x]; -} [%s:%d]", __FILE__, __LINE__);
                        #endif // OPT_DEBUG
                    }
                }
            }
            if (changes_applied) break;
            

            // mov rN, x
            // op rM, rN
            // mov rN, y
            // =>
            // (removed)
            // op rM, x
            // mov rN, y
            if (i < instruction_count - 2) {
                if (instruction[i].instruction == MOV && (
                    instruction[i + 1].instruction == MOV ||
                    instruction[i + 1].instruction == ADD ||
                    instruction[i + 1].instruction == SUB ||
                    instruction[i + 1].instruction == MUL ||
                    instruction[i + 1].instruction == DIV ||
                    instruction[i + 1].instruction == ADDF ||
                    instruction[i + 1].instruction == SUBF ||
                    instruction[i + 1].instruction == MULF ||
                    instruction[i + 1].instruction == DIVF ||
                    instruction[i + 1].instruction == CMP ||
                    instruction[i + 1].instruction == BWS ||
                    instruction[i + 1].instruction == AND ||
                    instruction[i + 1].instruction == OR ||
                    instruction[i + 1].instruction == XOR
                ) && instruction[i + 2].instruction == MOV) {
                    if (instruction[i].admr == instruction[i + 2].admr && is_same_adm(instruction[i].admr, instruction[i + 1].admx)) {
                        //log_msg(LP_DEBUG, "Optimizer: eliminated temporary register forwarding (line %d)", i);
                        instruction[i + 1].expression[2].token_count = instruction[i].expression[2].token_count;
                        instruction[i + 1].expression[2].type = instruction[i].expression[2].type;
                        for (int j = 0; j < instruction[i].expression[2].token_count; j++) {
                            instruction[i + 1].expression[2].tokens[j].type = instruction[i].expression[2].tokens[j].type;
                            instruction[i + 1].expression[2].tokens[j].raw = realloc(instruction[i + 1].expression[2].tokens[j].raw, strlen(instruction[i].expression[2].tokens[j].raw) + 1);
                            memcpy(instruction[i + 1].expression[2].tokens[j].raw, instruction[i].expression[2].tokens[j].raw, strlen(instruction[i].expression[2].tokens[j].raw) + 1);
                        }
                        instruction[i + 1].admx = instruction[i].admx;
                        remove_instruction(instruction, &instruction_count, i);
                        i --;
                        changes_applied = 1;
                        #ifdef OPT_DEBUG
                            log_msg(LP_DEBUG, "Optimizer: applied {mov rN, x; op rM, rN; mov rN, y} => {-; op rM, x; mov rN, y} [%s:%d]", __FILE__, __LINE__);
                        #endif // OPT_DEBUG
                    }
                }
            }
            if (changes_applied) break;


            // mov rN, rM
            // add rN, x
            // =>
            // lea rN, [x + rM]
            // (removed)
            if (i < instruction_count - 1) {
                if (instruction[i].instruction == MOV && (
                    instruction[i + 1].instruction == ADD ||
                    instruction[i + 1].instruction == SUB)) {
                    if (instruction[i].admr == instruction[i + 1].admr && is_register_admx(instruction[i].admx) && instruction[i + 1].admx == ADMX_IMM16) {
                        //log_msg(LP_DEBUG, "Optimizer: combined arithmetic addressing with register offset lea (line %d)", i);
                        for (int j = 1; j < 5; j++) {
                            instruction[i].expression[2].tokens[j].raw = malloc(32);
                        }
                        sprintf(instruction[i].expression[0].tokens[0].raw, "%s", cpu_instruction_string[LEA]);                             // replace "MOV" with "LEA"
                        sprintf(instruction[i].expression[2].tokens[3].raw, "%s", instruction[i].expression[2].tokens[0].raw);              // add "rM" in "[rM + x]"
                        sprintf(instruction[i].expression[2].tokens[0].raw, "[");                                                           // add "[" in "[rM + x]"
                        sprintf(instruction[i].expression[2].tokens[2].raw, "+");                                                           // add "+" in "[rM + x]"
                        if (instruction[i + 1].instruction == ADD) {
                            uint16_t imm = parse_immediate(instruction[i + 1].expression[2].tokens[0].raw);
                            char tmp[16];
                            sprintf(tmp, "$%.4X", imm);
                            sprintf(instruction[i].expression[2].tokens[1].raw, "%s", tmp);
                        } else if (instruction[i + 1].instruction == SUB) {
                            uint16_t imm = -(int16_t)parse_immediate(instruction[i + 1].expression[2].tokens[0].raw);
                            char tmp[16];
                            sprintf(tmp, "$%.4X", imm);
                            sprintf(instruction[i].expression[2].tokens[1].raw, "%s", tmp);
                        }
                        sprintf(instruction[i].expression[2].tokens[4].raw, "]");
                        instruction[i].expression[2].token_count = 5;
                        //log_msg(LP_DEBUG, "%s -> %s", cpu_extended_addressing_mode_string[instruction[i].admx], cpu_extended_addressing_mode_string[ADMX_IND_R0_OFFSET16 + instruction[i].admx - ADMX_R0]);
                        instruction[i].instruction = LEA;
                        instruction[i].admx = ADMX_IND_R0_OFFSET16 + instruction[i].admx - ADMX_R0;
                        remove_instruction(instruction, &instruction_count, i + 1);
                        changes_applied = 1;
                        #ifdef OPT_DEBUG
                            log_msg(LP_DEBUG, "Optimizer: applied {mov rN, rM; add rN, x} => {lea rN, [x + rM]; -} [%s:%d]", __FILE__, __LINE__);
                        #endif // OPT_DEBUG
                    }
                }
            }
            if (changes_applied) break;


            // mov rN, .L
            // add rN, $xyz
            // ->
            // lea rN, [.L + $xyz]
            // (removed)
            if (i < instruction_count - 2) {
                if (instruction[i].instruction == MOV && instruction[i + 1].instruction == ADD) {
                    if (is_register_admr(instruction[i].admr) && instruction[i].admr == instruction[i + 1].admr && instruction[i].admx == ADMX_IMM16 && instruction[i + 1].admx == ADMX_IMM16) {
                        if (
                            instruction[i].expression[2].type == EXPR_IMMEDIATE &&
                            instruction[i + 1].expression[2].type == EXPR_IMMEDIATE &&
                            instruction[i].expression[2].tokens[0].type == TT_LABEL &&
                            parse_immediate(instruction[i + 1].expression[2].tokens[0].raw) != 0
                        ) {
                            instruction[i].instruction = LEA;
                            sprintf(instruction[i].expression[0].tokens[0].raw, "%s", cpu_instruction_string[LEA]);
                            instruction[i].admx = ADMX_IND16;
                            
                            instruction[i].expression[2].tokens[1].raw = malloc(16);
                            sprintf(instruction[i].expression[2].tokens[1].raw, "%s", instruction[i].expression[2].tokens[0].raw);
                            sprintf(instruction[i].expression[2].tokens[0].raw, "[");
                            instruction[i].expression[2].tokens[2].raw = malloc(16);
                            sprintf(instruction[i].expression[2].tokens[2].raw, "+");
                            instruction[i].expression[2].tokens[3].raw = malloc(16);
                            sprintf(instruction[i].expression[2].tokens[3].raw, "%s", instruction[i + 1].expression[2].tokens[0].raw);
                            instruction[i].expression[2].tokens[4].raw = malloc(16);
                            sprintf(instruction[i].expression[2].tokens[4].raw, "]");
                            instruction[i].expression[2].token_count = 5;

                            remove_instruction(instruction, &instruction_count, i + 1);

                            changes_applied = 1;
                            #ifdef OPT_DEBUG
                                log_msg(LP_DEBUG, "Optimizer: applied {mov rN, .L; add rN, $x} => {lea rN, [.L + $x]; -} [%s:%d]", __FILE__, __LINE__);
                            #endif // OPT_DEBUG
                        }
                    }
                }
            }
            if (changes_applied) break;

            

            // mov rN, x
            // add rN, y
            // =>
            // [precompute] z :: x OP y
            // mov rN, z
            // (removed) or "SEAO"
            if (i < instruction_count - 1) {
                if (instruction[i].instruction == MOV && is_arithmetic_operation(instruction[i + 1].instruction)) {
                    if (
                        instruction[i].admr == instruction[i + 1].admr && instruction[i].admx == ADMX_IMM16 && instruction[i + 1].admx == ADMX_IMM16
                        && instruction[i].expression[2].tokens[0].type != TT_LABEL
                    ) {
                        //log_msg(LP_DEBUG, "Optimizer: combined arithmetic assign chain (line %d)", i);
                        
                        int16_t value = (int16_t) parse_immediate(instruction[i].expression[2].tokens[0].raw);
                        int16_t value2 = (int16_t) parse_immediate(instruction[i + 1].expression[2].tokens[0].raw);
                        int set_AO = 0;

                        switch(instruction[i + 1].instruction) {
                            case ADD:
                                set_AO = (uint32_t) ((uint16_t) value) + (uint32_t) ((uint16_t) value2) > 0xffff;
                                value += value2;
                                break;
                            case SUB:
                                set_AO = (int32_t) value - (int32_t) value2 < -(0x8000);
                                value -= value2;
                                break;
                            case MUL:
                                set_AO = (uint32_t) ((uint16_t) value) * (uint32_t) ((uint16_t) value2) > 0xffff;
                                value *= value2;
                                break;
                            case DIV:
                                set_AO = value2 == 0;
                                value /= value2;
                                break;
                            case ADDF:
                                value = f16_add(value, value2);
                                break;
                            case SUBF:
                                value = f16_sub(value, value2);
                                break;
                            case MULF:
                                value = f16_mult(value, value2);
                                break;
                            case DIVF:
                                value = f16_div(value, value2);
                                break;
                            case AND:
                                value &= value2;
                                break;
                            case OR:
                                value |= value2;
                                break;
                            case XOR:
                                value ^= value2;
                                break;
                            default:
                                log_msg(LP_ERROR, "Optimizer, I dont even know how this would be possible, yet here we are... [%s:%d]", __FILE__, __LINE__);
                                break;
                        }

                        sprintf(instruction[i].expression[2].tokens[0].raw, "$%.4X", (uint16_t) value);
                        
                        Instruction_t new_instruction = {
                            .instruction = SEAO, 
                            .address = instruction[i].address, 
                            .admr = ADMR_NONE, 
                            .admx = ADMX_NONE, 
                            .argument_bytes = 0, 
                            .expression_count = 1, 
                            .is_address = 0, 
                            .is_raw_data = 0, 
                            .expression = {
                                (Expression_t) {
                                    .token_count = 1, 
                                    .type = EXPR_INSTRUCTION, 
                                    .tokens = {
                                        (Token_t) {
                                            .type = TT_INSTRUCTION, 
                                            .raw = "seao"
                                        }
                                    }
                                }
                            }
                        };
                        remove_instruction(instruction, &instruction_count, i + 1);
                        if (set_AO) {
                            insert_instruction(&instruction, new_instruction, &instruction_count, i + 1);
                        }
                        changes_applied = 1;
                        #ifdef OPT_DEBUG
                            log_msg(LP_DEBUG, "Optimizer: applied {mov rN, x; add rN, y} => {mov rN, z; -} with z precomputed from x and y [%s:%d]", __FILE__, __LINE__);
                        #endif // OPT_DEBUG
                    }
                }
            }
            if (changes_applied) break;


            // mov rN, ?
            // call ?
            // ->
            // (remove)
            // call/ret ?
            if (i < instruction_count - 2) {
                if (instruction[i].instruction == MOV && is_register_admr(instruction[i].admr) && instruction[i].admr != ADMR_SP) {
                    if (instruction[i + 1].instruction == CALL || instruction[i + 1].instruction == RET) {
                        remove_instruction(instruction, &instruction_count, i);
                        changes_applied = 1;
                        #ifdef OPT_DEBUG
                            log_msg(LP_DEBUG, "Optimizer: applied {mov rN, ?; call/ret} => {-; call/ret} [%s:%d]", __FILE__, __LINE__);
                        #endif // OPT_DEBUG
                    }
                }
            }


            // mov rN, $xyz
            // push [rN]
            // ->
            // (remove)
            // push [$xyz]
            if (i < instruction_count - 2) {
                if (instruction[i].instruction == MOV && is_register_admr(instruction[i].admr)) {
                    if (instruction[i + 1].instruction == PUSH) {
                        //log_msg(LP_CRITICAL, "%s ?= %s (%s)", cpu_reduced_addressing_mode_string[instruction[i].admr], cpu_extended_addressing_mode_string[instruction[i + 1].admx], instruction[i + 1].expression[1].tokens[0].raw);
                        if (instruction[i + 1].instruction == PUSH && is_same_indirect_adm(instruction[i].admr, instruction[i + 1].admx)) {
                            sprintf(instruction[i + 1].expression[1].tokens[1].raw, "%s", instruction[i].expression[2].tokens[0].raw);
                            instruction[i + 1].admx = ADMX_IND_R0 + (instruction[i].admr - ADMR_R0);

                            remove_instruction(instruction, &instruction_count, i);
                            changes_applied = 1;
                            #ifdef OPT_DEBUG
                                log_msg(LP_DEBUG, "Optimizer: applied {mov rN, ?; call/ret} => {-; call/ret} [%s:%d]", __FILE__, __LINE__);
                            #endif // OPT_DEBUG
                        }
                    }
                }
            }


            // sub sp, $x
            // mov r0, sp
            // mov [r0], ?
            // =>
            // sub sp, $(x - 2)
            // (remove)
            // push ?
            if (i < instruction_count - 3) {
                if (instruction[i].instruction == SUB && instruction[i].admr == ADMR_SP && instruction[i].admx == ADMX_IMM16) {
                    int j = i + 1;
                    while (j < instruction_count - 2) {
                        if (instruction[j].instruction == CALL || instruction[j].instruction == RET || cpu_instruction_is_jump[instruction[j].instruction]) {
                            break;
                        }
                        if (instruction[j].admr == ADMR_SP) {
                            break;
                        }
                        if (instruction[j].instruction == MOV && instruction[j].admr == ADMR_R0 && instruction[j].admx == ADMX_SP) {
                            if (instruction[j + 1].instruction == MOV && instruction[j + 1].admr == ADMR_IND_R0) {

                                instruction[j + 1].instruction = PUSH;
                                sprintf(instruction[j + 1].expression[0].tokens[0].raw, "%s", cpu_instruction_string[PUSH]);
                                for (int t = 0; t < instruction[j + 1].expression[2].token_count; t++) {
                                    if (!instruction[j + 1].expression[1].tokens[t].raw) {instruction[j + 1].expression[1].tokens[t].raw = malloc(16);}
                                    sprintf(instruction[j + 1].expression[1].tokens[t].raw, "%s", instruction[j + 1].expression[2].tokens[t].raw);
                                    instruction[j + 1].expression[1].tokens[t].type = instruction[j + 1].expression[2].tokens[t].type;
                                }
                                instruction[j + 1].expression[1].token_count = instruction[j + 1].expression[2].token_count;
                                instruction[j + 1].expression_count = 2;

                                uint16_t imm16 = parse_immediate(instruction[i].expression[2].tokens[0].raw);
                                sprintf(instruction[i].expression[2].tokens[0].raw, "$%.4X", imm16 - 2);

                                remove_instruction(instruction, &instruction_count, j);
                                
                                changes_applied = 1;
                                #ifdef OPT_DEBUG
                                    log_msg(LP_DEBUG, "Optimizer: applied {sub sp, $x; mov r0, sp; mov [r0], ?} => {sub sp, $(x - 2); -; push ?} [%s:%d]", __FILE__, __LINE__);
                                #endif
                                break;
                            }
                        }
                        j ++;
                    }
                }
            }
            if (changes_applied) break;


            // mov rN, [rN]
            // instruction that doesnt use or modify rN
            // mov [r0], rN
            // ->
            // instruction that doesnt use or modify rN
            // mov [r0], [rN]
            if (i < instruction_count - 2) {
                if (instruction[i].instruction == MOV && instruction[i + 2].instruction == MOV) {
                    if (   cpu_reduced_addressing_mode_category[instruction[i].admr] == ADMC_REG
                        && instruction[i].admr != ADMR_R0
                        && instruction[i].admr != ADMR_IND_R0
                        && is_same_indirect_adm(instruction[i].admr, instruction[i].admx)
                        && is_same_adm(instruction[i].admr, instruction[i + 2].admx)            // rN == rN
                        && !is_same_adm(instruction[i + 2].admr, instruction[i + 2].admx)
                        && !is_same_indirect_adm(instruction[i + 2].admr, instruction[i + 2].admx)
                        && (
                               instruction[i + 1].expression_count == 1
                            || (
                                   instruction[i + 1].expression_count == 2
                                && !(instruction[i + 1].admr == instruction[i].admr)
                            )
                            || (
                                instruction[i + 1].expression_count == 3
                             && !(instruction[i + 1].admr == instruction[i].admr)
                             && !is_same_adm(instruction[i].admr, instruction[i + 1].admx)
                             && !is_same_indirect_adm(instruction[i].admr, instruction[i + 1].admx)
                         )
                        )
                    ) {
                        //log_msg(LP_DEBUG, "Optimizer: reduced two step deref to one step (line %d)", i);
                        char tmp[32];
                        for (int t = 0; t < 3; t++) {
                            if (!instruction[i + 2].expression[2].tokens[t].raw) {
                                instruction[i + 2].expression[2].tokens[t].raw = malloc(16);
                            }
                        }
                        sprintf(tmp, "%s", instruction[i + 2].expression[2].tokens[0].raw);
                        sprintf(instruction[i + 2].expression[2].tokens[0].raw, "[");
                        sprintf(instruction[i + 2].expression[2].tokens[1].raw, "%s", tmp);
                        sprintf(instruction[i + 2].expression[2].tokens[2].raw, "]");
                        instruction[i + 2].expression[2].token_count = 3;
                        instruction[i + 2].admx = ADMX_IND_R0 + instruction[i + 2].admx - ADMX_R0;

                        insert_instruction(&instruction, instruction[i], &instruction_count, i + 3);
                        remove_instruction(instruction, &instruction_count, i);
                        changes_applied = 1;
                        #ifdef OPT_DEBUG
                            log_msg(LP_DEBUG, "Optimizer: applied {mov rN, [rN]; op; mov [r0], rN} => {-; op; mov [r0], [rN]} [%s:%d]", __FILE__, __LINE__);
                        #endif // OPT_DEBUG
                    }
                }
            }
            if (changes_applied) break;


            // lea rN, [$x + rO]
            // lea rM, [$y + rN]
            // mov rN, rM
            // ->
            // lea rN, [$z + rO] where $z = (uint16) ($x + $y)
            // lea rM, [$y + rN] unchanged
            // (removed)
            if (i < instruction_count - 3) {
                if (instruction[i].instruction == LEA               // lea
                    && is_register_admr(instruction[i].admr)        // rN
                    && is_register_offset_admx(instruction[i].admx) // [$x + rO]
                    && register_offset_admx_contains_admr_register(instruction[i + 1].admx, instruction[i].admr)    // lea -rN-, [...] == lea rM, [$y + -rN-]
                    && instruction[i].admr == instruction[i + 2].admr   // lea -rN-, [...] == mov -rN-, rM
                    && is_same_adm(instruction[i + 1].admr, instruction[i + 2].admx)    // lea -rM-, [...] == mov rN, -rM-
                    && instruction[i + 1].admr == instruction[i + 3].admr
                    && is_overwriting_instruction(instruction[i + 3].instruction)
                ) {
                    //log_msg(LP_DEBUG, "Optimizer: reduced lea accumulation (line %d)", i);
                    uint16_t x = parse_immediate(instruction[i].expression[2].tokens[1].raw);
                    uint16_t y = parse_immediate(instruction[i + 1].expression[2].tokens[1].raw);
                    sprintf(instruction[i].expression[2].tokens[1].raw, "$%.4X", x + y);            // replacing "$x" with new precomputed z
                    remove_instruction(instruction, &instruction_count, i + 2);      // removing "mov rN, rM"
                    changes_applied = 1;
                    #ifdef OPT_DEBUG
                        log_msg(LP_DEBUG, "Optimizer: applied {lea rN, [x + rO]; lea rM, [y + rN]; mov rN, rM} => {lea rN, [z + rO]; lea rM, [$y + rN], -} where z is precomputed from x and y [%s:%d]", __FILE__, __LINE__);
                    #endif // OPT_DEBUG
                }
            }
            if (changes_applied) break;


            // TODO: Make it look over multiple instructions, that do not modify THAT register
            // TODO: check for nc
            // mov rN, x
            // mov rM, rN
            // =>
            // (removed)
            // mov rM, x
            // given that rN is overwritten soon / not used anymore until next reset
            if (i < instruction_count - 1) {
                if (instruction[i].instruction == MOV/* || instruction[i].instruction == LEA*/) {
                    if (is_register_admr(instruction[i].admr)) {
                        CPU_REDUCED_ADDRESSING_MODE_t admr = instruction[i].admr;
                        int j = i + 1;
                        //printf("origin: admr: %s, admx: %s\n", cpu_reduced_addressing_mode_string[instruction[i].admr], cpu_extended_addressing_mode_string[instruction[i].admx]);
                        while (j < instruction_count - 1) {
                            /*printf("%d: ", j - i);
                            for (int e = 0; e < instruction[j].expression_count; e++) {
                                for (int t = 0; t < instruction[j].expression[e].token_count; t++) {
                                    printf("%s ", instruction[j].expression[e].tokens[t].raw);
                                }
                            }
                            printf("admr: %s, admx: %s", cpu_reduced_addressing_mode_string[instruction[j].admr], cpu_extended_addressing_mode_string[instruction[j].admx]);
                            printf("\n");*/
                            CPU_EXTENDED_ADDRESSING_MODE_t admx_j = instruction[j].admx;

                            if (instruction[j].instruction == CALL || 
                                instruction[j].instruction == RET || 
                                ((instruction[j].instruction == MOV || instruction[j].instruction == LEA) && instruction[j].admr == admr) ||
                                cpu_instruction_is_jump[instruction[j].instruction] ||
                                (instruction[j].admx == ADMX_IMM16 && instruction[j].expression[2].tokens[0].type == TT_LABEL) ||
                                (instruction[j].admr == admr && (is_same_indirect_adm(admr, instruction[j].admx) || register_offset_admx_contains_admr_register(instruction[j].admx, admr))) ||
                                instruction[j].instruction == HLT ||
                                (is_arithmetic_operation(instruction[j].instruction) && instruction[j].admr == admr) ||
                                register_offset_admx_contains_admr_register(instruction[j].admx, admr) || 
                                ((instruction[j].instruction == MOV || instruction[j].instruction == LEA) && is_same_adm(instruction[j].admr, instruction[i].admx)) ||
                                (is_move_operator(instruction[j].instruction) && instruction[j].admr == admr)
                            ) {
                                    break;
                            }
                            
                            if (instruction[j].instruction == MOV || instruction[j].instruction == LEA) {
                                if (is_same_adm(admr, admx_j)) {

                                    // now need to validate that rN is unused until reset
                                    int k = j + 1;
                                    int valid = 1;
                                    while (k < instruction_count - i) {
                                        if (
                                            is_same_adm(admr, instruction[k].admx) ||
                                            is_same_indirect_adm(admr, instruction[k].admx) ||
                                            register_offset_admx_contains_admr_register(instruction[k].admx, admr) ||
                                            instruction[k].instruction == CALL ||
                                            instruction[k].instruction == RET ||
                                            cpu_instruction_is_jump[instruction[k].instruction]
                                        ) {
                                            valid = 0;
                                            break;
                                        }
                                        if (instruction[k].instruction == MOV || instruction[k].instruction == LEA) {
                                            if (instruction[k].admr == admr) {
                                                valid = 1;
                                                break;
                                            }
                                        }
                                        k++;
                                    }

                                    if (valid) {
                                        instruction[j].admx = instruction[i].admx;
                                        for (int t = 0; t < instruction[i].expression[2].token_count; t++) {
                                            if (!instruction[j].expression[2].tokens[t].raw) {
                                                instruction[j].expression[2].tokens[t].raw = malloc(16);
                                            }
                                            sprintf(instruction[j].expression[2].tokens[t].raw, "%s", instruction[i].expression[2].tokens[t].raw);
                                        }
                                        instruction[j].expression[2].token_count = instruction[i].expression[2].token_count;
                                        remove_instruction(instruction, &instruction_count, i);
                                        i --;
                                        changes_applied = 1;
                                        #ifdef OPT_DEBUG
                                            log_msg(LP_DEBUG, "Optimizer: applied {mov rN, x; ...(%d); mov rM, rN} => {-; ...; mov rM, x} [%s:%d]", j - i, __FILE__, __LINE__);
                                        #endif
                                        break;
                                    }
                                }
                            }

                            j++;
                        }
                    }
                }
            }
            if (changes_applied) break;


            // mov rN, ?
            // op rN, ?     (repeating operations on rN only)
            // mov rM, rN
            // ->
            // mov rM, ?
            // op rM, ?     (repeating operations on rM only)
            // mov rN, rM   (just to restore rN, in case its used afterwards)
            // (removed)

            // TODO


            // ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
            // :::::::::::::::::::::::::::::::::::::::::::::: O1 optimizations ::::::::::::::::::::::::::::::::::::::::::::::
            // ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

            // mov r0, r0
            // =>
            // (removed)
            if (instruction[i].instruction == MOV) {
                if (is_same_adm(instruction[i].admr, instruction[i].admx)) {
                    //log_msg(LP_DEBUG, "Optimizer: removed self assigning mov instruction (line %d)", i);
                    remove_instruction(instruction, &instruction_count, i);
                    i --;
                    changes_applied = 1;
                    #ifdef OPT_DEBUG
                        log_msg(LP_DEBUG, "Optimizer: applied {mov rN, rN} => {-} [%s:%d]", __FILE__, __LINE__);
                    #endif // OPT_DEBUG
                }
            }
            if (changes_applied) break;

            // add rN, 0 / addf rN, f0 / sub rN, 0 / subf rN, f0
            // =>
            // (removed)
            if (instruction[i].instruction == ADD || instruction[i].instruction == SUB ||
                instruction[i].instruction == ADDF || instruction[i].instruction == SUBF) {
                if (instruction[i].admx == ADMX_IMM16) {
                    if (parse_immediate(instruction[i].expression[2].tokens[0].raw) == 0) {
                        //log_msg(LP_DEBUG, "Optimizer: removed additive identity operation (line %d)", i);
                        remove_instruction(instruction, &instruction_count, i);
                        i --;
                        changes_applied = 1;
                        #ifdef OPT_DEBUG
                            log_msg(LP_DEBUG, "Optimizer: applied {add rN, 0} => {-} [%s:%d]", __FILE__, __LINE__);
                        #endif // OPT_DEBUG
                    }
                }
            }
            if (changes_applied) break;
            

            // mul rN, 1 / div rN, 1
            // =>
            // (removed)
            if (instruction[i].instruction == MUL || instruction[i].instruction == DIV) {
                if (instruction[i].admx == ADMX_IMM16) {
                    if (parse_immediate(instruction[i].expression[2].tokens[0].raw) == 1) {
                        //log_msg(LP_DEBUG, "Optimizer: removed multiplicative identity operation (line %d)", i);
                        remove_instruction(instruction, &instruction_count, i);
                        i --;
                        changes_applied = 1;
                        #ifdef OPT_DEBUG
                            log_msg(LP_DEBUG, "Optimizer: applied {mul rN, 1} => {-} [%s:%d]", __FILE__, __LINE__);
                        #endif // OPT_DEBUG
                    }
                }
            }
            if (changes_applied) break;
            

            // mulf r0, f1 / divf r0, f1
            // =>
            // (removed)
            if (instruction[i].instruction == MULF || instruction[i].instruction == DIVF) {
                if (instruction[i].admx == ADMX_IMM16) {
                    if (float_from_f16(parse_immediate(instruction[i].expression[2].tokens[0].raw)) == 1.0) {
                        //log_msg(LP_DEBUG, "Optimizer: removed float multiplicative identity operation (line %d)", i);
                        remove_instruction(instruction, &instruction_count, i);
                        i --;
                        changes_applied = 1;
                        #ifdef OPT_DEBUG
                            log_msg(LP_DEBUG, "Optimizer: applied {mulf rN, 1} => {-} [%s:%d]", __FILE__, __LINE__);
                        #endif // OPT_DEBUG
                    }
                }
            }
            if (changes_applied) break;
            

            // TODO: Make it look over multiple instructions, that do not modify THAT register
            // TODO: check for nc
            // mov/lea rN, x
            // mov/lea rN, y
            // =>
            // (removed)
            // mov/lea rN, y
            if (i < instruction_count - 1) {
                if (instruction[i].instruction == MOV && instruction[i + 1].instruction == MOV) {
                    if (   cpu_reduced_addressing_mode_category[instruction[i].admr] == ADMC_REG
                        && instruction[i].admr == instruction[i + 1].admr
                        && !is_same_indirect_adm(instruction[i + 1].admr, instruction[i + 1].admx)
                    ) {
                        //log_msg(LP_DEBUG, "Optimizer: removed overshadowed mov instruction (line %d)", i);
                        remove_instruction(instruction, &instruction_count, i);
                        i --;
                        changes_applied = 1;
                        #ifdef OPT_DEBUG
                            log_msg(LP_DEBUG, "Optimizer: applied {mov rN, x; mov rN, y} => {mov rN, y} [%s:%d]", __FILE__, __LINE__);
                        #endif // OPT_DEBUG
                    }
                }
                if (instruction[i].instruction == LEA && instruction[i + 1].instruction == LEA) {
                    if (   cpu_reduced_addressing_mode_category[instruction[i].admr] == ADMC_REG
                        && instruction[i].admr == instruction[i + 1].admr) {
                        //log_msg(LP_DEBUG, "Optimizer: removed overshadowed lea instruction (line %d)", i);
                        remove_instruction(instruction, &instruction_count, i);
                        i --;
                        changes_applied = 1;
                        #ifdef OPT_DEBUG
                            log_msg(LP_DEBUG, "Optimizer: applied {lea rN, x; lea rN, y} => {-; lea rN, y} [%s:%d]", __FILE__, __LINE__);
                        #endif // OPT_DEBUG
                    }
                }
            }
            if (changes_applied) break;


            // lea rN [ imm16 ] // or indirect register
            // =>
            // mov rN, imm16
            if (i < instruction_count - 1) {
                if (   instruction[i].instruction == LEA 
                    && cpu_extended_addressing_mode_category[instruction[i].admx] == ADMC_IND 
                    && instruction[i].admx < ADMX_IND_R0_OFFSET16  // ADMX_IND16, ADMX_IND_R0, ADMX_IND_R1, ADMX_IND_R2, ADMX_IND_R3, ADMX_IND_SP, ADMX_IND_PC, 
                ) {
                    if (instruction[i].expression[2].token_count == 3) {    // if more than 3, then its something like "[.L + $xyz]"
                        //log_msg(LP_DEBUG, "Optimizer: replaced lea with direct move instruction (line %d)", i);
                        instruction[i].expression[0].tokens[0].raw = realloc(instruction[i].expression[0].tokens[0].raw, strlen(cpu_instruction_string[MOV]) + 1);
                        sprintf(instruction[i].expression[0].tokens[0].raw, "%s", cpu_instruction_string[MOV]);
                        instruction[i].instruction = MOV;
                        if (instruction[i].admx == ADMX_IND16) {
                            instruction[i].admx = ADMX_IMM16;
                        } else {
                            instruction[i].admx = ADMX_R0 + instruction[i].admx - ADMX_IND_R0;
                        }
                        instruction[i].expression[2].tokens[0] = instruction[i].expression[2].tokens[1];
                        instruction[i].expression[2].token_count = 1;
                        changes_applied = 1;
                        #ifdef OPT_DEBUG
                            log_msg(LP_DEBUG, "Optimizer: applied {lea rN, [x]} => {mov rN, x} [%s:%d]", __FILE__, __LINE__);
                        #endif // OPT_DEBUG
                    }
                }
            }
            if (changes_applied) break;


            // push rN
            // pop rN
            // =>
            // (removed)
            if (i < instruction_count - 1) {
                if (instruction[i].instruction == PUSH && instruction[i + 1].instruction == POP) {
                    if (is_same_adm(instruction[i + 1].admr, instruction[i].admx)) {
                        //log_msg(LP_DEBUG, "Optimizer: removed redundant stack operations (line %d)", i);
                        remove_instruction(instruction, &instruction_count, i + 1);
                        remove_instruction(instruction, &instruction_count, i);
                        i -= 1;
                        changes_applied = 1;
                        #ifdef OPT_DEBUG
                            log_msg(LP_DEBUG, "Optimizer: applied {push rN, pop rN} => {-; -} [%s:%d]", __FILE__, __LINE__);
                        #endif // OPT_DEBUG
                    }
                }
            }
            if (changes_applied) break;


            // pushsr
            // popsr
            // =>
            // (removed)
            if (i < instruction_count - 1) {
                if (instruction[i].instruction == PUSHSR && instruction[i + 1].instruction == POPSR) {
                    //log_msg(LP_DEBUG, "Optimizer: removed redundant stack operation (line %d)", i);
                    remove_instruction(instruction, &instruction_count, i + 1);
                    remove_instruction(instruction, &instruction_count, i);
                    i -= 1;
                    changes_applied = 1;
                    #ifdef OPT_DEBUG
                        log_msg(LP_DEBUG, "Optimizer: applied {pushsr; popsr} => {-; -} [%s:%d]", __FILE__, __LINE__);
                    #endif // OPT_DEBUG
                }
            }
            if (changes_applied) break;

            
            // and ?, $ffff
            // =>
            // (removed, if no status register reads follow)
            if (i < instruction_count) {
                if (instruction[i].instruction == AND) {
                    if (instruction[i].admx == ADMX_IMM16) {
                        if (instruction[i].admr == ADMR_R0 || instruction[i].admr == ADMR_R1 || 
                            instruction[i].admr == ADMR_R2 || instruction[i].admr == ADMR_R3 || 
                            instruction[i].admr == ADMR_SP) {
                            if (parse_immediate(instruction[i].expression[2].tokens[0].raw) == 0xffff) {
                                // now also check if any conditional instruction is present before another statur-register changing operation is done
                                //log_msg(LP_DEBUG, "Optimizer: removed redundant and operation (line %d)", i);
                                remove_instruction(instruction, &instruction_count, i);
                                i --;
                                changes_applied = 1;
                                #ifdef OPT_DEBUG
                                    log_msg(LP_DEBUG, "Optimizer: applied {and x, $ffff} => {-} [%s:%d]", __FILE__, __LINE__);
                                #endif // OPT_DEBUG
                            }
                        }
                    }
                }
            }
            if (changes_applied) break;

            
            // or ?, 0
            // =>
            // (removed, if no status register reads follow)
            if (i < instruction_count) {
                if (instruction[i].instruction == OR) {
                    if (instruction[i].admx == ADMX_IMM16) {
                        if (instruction[i].admr == ADMR_R0 || instruction[i].admr == ADMR_R1 || 
                            instruction[i].admr == ADMR_R2 || instruction[i].admr == ADMR_R3 || 
                            instruction[i].admr == ADMR_SP) {
                            if (parse_immediate(instruction[i].expression[2].tokens[0].raw) == 0 && string_is_immediate(instruction[i].expression[2].tokens[0].raw)) {
                                // now also check if any conditional instruction is present before another statur-register changing operation is done
                            
                                //log_msg(LP_DEBUG, "Optimizer: removed redundant and operation (line %d)", i);
                                remove_instruction(instruction, &instruction_count, i);
                                i --;
                                changes_applied = 1;
                                #ifdef OPT_DEBUG
                                    log_msg(LP_DEBUG, "Optimizer: applied {or x, 0} => {-} [%s:%d]", __FILE__, __LINE__);
                                #endif // OPT_DEBUG
                            }
                        }
                    }
                }
            }
            if (changes_applied) break;
            
            // mov rN, ?
            // tst rN
            // ->
            // mov rN, ?    (to restore rN)
            // test ?

            
        }
    }

    // LOW PRIORITY OPTIMIZATIONS
    // for optimizations that would destroy canonicalization and prevent other pattern optimizations from being matched
    changes_applied = 1;
    while (changes_applied) {
        changes_applied = 0;
        // Optimize
        for (int i = 0; i < instruction_count; i++) {

            // ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
            // :::::::::::::::::::::::::::::::::::::::::::::: O2 optimizations ::::::::::::::::::::::::::::::::::::::::::::::
            // ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

            // lea rN, ?
            // ...
            // mov ??, rN
            // =>
            // lea rN, ?
            // ...
            // lea ??, ?
            /*if (i < instruction_count) {
                if (instruction[i].instruction == LEA && is_register_admr(instruction[i].admr)) {
                    int j = i + 1;
                    while (j < instruction_count - 1) {
                        if (instruction[j].instruction == CALL || instruction[j].instruction == RET || cpu_instruction_is_jump[instruction[j].instruction]) {
                            break;
                        }
                        if (instruction[j].admr == instruction[i].admr) {
                            break;
                        }
                        if (instruction[j].instruction == MOV && is_same_adm(instruction[i].admr, instruction[j].admx)) {

                            instruction[j].instruction = LEA;
                            sprintf(instruction[j].expression[0].tokens[0].raw, "%s", cpu_instruction_string[LEA]);
                            for (int t = 0; t < instruction[i].expression[2].token_count; t++) {
                                if (!instruction[j].expression[2].tokens[t].raw) {instruction[j].expression[2].tokens[t].raw = malloc(16);}
                                sprintf(instruction[j].expression[2].tokens[t].raw, "%s", instruction[i].expression[2].tokens[t].raw);
                                instruction[j].expression[2].tokens[t].type = instruction[i].expression[2].tokens[t].type;
                            }
                            instruction[j].expression[2].token_count = instruction[i].expression[2].token_count;
                            
                            changes_applied = 1;
                            #ifdef OPT_DEBUG
                                log_msg(LP_DEBUG, "Optimizer: applied {lea rN, ?; ...; mov ??, rN} => {lea rN, ?; ...; lea ??, ?} [%s:%d]", __FILE__, __LINE__);
                            #endif
                            break;
                        }
                        j ++;
                    }
                }
            }
            if (changes_applied) break;*/


            // mov rN, x
            // mov rM, rN
            // =>
            // (removed)
            // mov rM, x
            // given that rN is overwritten soon / not used anymore until next reset
            /*if (i < instruction_count - 1) {
                if (instruction[i].instruction == MOV || instruction[i].instruction == LEA) {
                    if (is_register_admr(instruction[i].admr)) {
                        CPU_REDUCED_ADDRESSING_MODE_t admr = instruction[i].admr;
                        int j = i + 1;
                        //printf("origin: admr: %s, admx: %s\n", cpu_reduced_addressing_mode_string[instruction[i].admr], cpu_extended_addressing_mode_string[instruction[i].admx]);
                        while (j < instruction_count - 1) {
                            CPU_EXTENDED_ADDRESSING_MODE_t admx_j = instruction[j].admx;

                            if (((instruction[j].instruction == MOV || instruction[j].instruction == LEA) && (instruction[j].admr == admr || (admr == ADMR_R0 && instruction[j].admr == ADMR_IND_R0))) ||
                                cpu_instruction_is_jump[instruction[j].instruction] ||
                                (instruction[j].admr == admr && (is_same_indirect_adm(admr, admx_j) || register_offset_admx_contains_admr_register(admx_j, admr))) ||
                                (is_arithmetic_operation(instruction[j].instruction) && instruction[j].admr == admr) ||
                                register_offset_admx_contains_admr_register(admx_j, admr) || 
                                ((instruction[j].instruction == MOV || instruction[j].instruction == LEA) && is_same_adm(instruction[j].admr, instruction[i].admx)) ||
                                (is_move_operator(instruction[j].instruction) && instruction[j].admr == admr)
                            ) {
                                    break;
                            }
                            
                            if (((instruction[j].instruction == MOV || instruction[j].instruction == LEA) && is_same_adm(admr, admx_j)) || 
                                instruction[j].instruction == HLT ||
                                instruction[j].instruction == CALL
                            ) {
                                if (1) {

                                    // now need to validate that rN is unused until reset
                                    int k = j + 1;
                                    int valid = 1;
                                    while (k < instruction_count - i) {
                                        if (
                                            is_same_adm(admr, instruction[k].admx) ||
                                            is_same_indirect_adm(admr, instruction[k].admx) ||
                                            register_offset_admx_contains_admr_register(instruction[k].admx, admr) ||
                                            instruction[k].instruction == CALL ||
                                            instruction[k].instruction == RET ||
                                            cpu_instruction_is_jump[instruction[k].instruction]
                                        ) {
                                            valid = 0;
                                            break;
                                        }
                                        if (instruction[k].instruction == MOV || instruction[k].instruction == LEA) {
                                            if (instruction[k].admr == admr) {
                                                valid = 1;
                                                break;
                                            }
                                        }
                                        k++;
                                    }

                                    if (valid) {
                                        instruction[j].admx = instruction[i].admx;
                                        for (int t = 0; t < instruction[i].expression[2].token_count; t++) {
                                            if (!instruction[j].expression[2].tokens[t].raw) {
                                                instruction[j].expression[2].tokens[t].raw = malloc(16);
                                            }
                                            sprintf(instruction[j].expression[2].tokens[t].raw, "%s", instruction[i].expression[2].tokens[t].raw);
                                        }
                                        instruction[j].expression[2].token_count = instruction[i].expression[2].token_count;
                                        remove_instruction(instruction, &instruction_count, i);
                                        i --;
                                        changes_applied = 1;
                                        #ifdef OPT_DEBUG
                                            log_msg(LP_DEBUG, "Optimizer: applied {mov rN, x; ...(%d); mov rM, rN} => {-; ...; mov rM, x} [%s:%d]", j - i, __FILE__, __LINE__);
                                        #endif
                                        break;
                                    }
                                }
                            }

                            j++;
                        }
                    }
                }
            }
            if (changes_applied) break;*/


            // ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
            // :::::::::::::::::::::::::::::::::::::::::::::: O1 optimizations ::::::::::::::::::::::::::::::::::::::::::::::
            // ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
            
            // mov r0, 0
            // =>
            // xor r0, r0 
            // if no status register reads follow
            // Issue: It can cause more cache misses!
            if (instruction[i].instruction == MOV) {
                if (instruction[i].admx == ADMX_IMM16) {
                    if (is_register_admr(instruction[i].admr)) {
                        if (parse_immediate(instruction[i].expression[2].tokens[0].raw) == 0 && string_is_immediate(instruction[i].expression[2].tokens[0].raw)) {
                            // now also check if any conditional instruction is present before another statur-register changing operation is done
                            //log_msg(LP_DEBUG, "Optimizer: replaced 0-mov with xor operation (rationale: reduce size of executable) (line %d)", i);
                            sprintf(instruction[i].expression[0].tokens[0].raw, "%s", cpu_instruction_string[XOR]);
                            sprintf(instruction[i].expression[2].tokens[0].raw, "%s", instruction[i].expression[1].tokens[0].raw);
                            instruction[i].instruction = XOR;
                            instruction[i].admx = ADMX_R0 + instruction[i].admr - ADMR_R0;
                            changes_applied = 1;
                            #ifdef OPT_DEBUG
                                log_msg(LP_DEBUG, "Optimizer: applied {mov rN, 0} => {xor rN, rN} [%s:%d]", __FILE__, __LINE__);
                            #endif // OPT_DEBUG
                        }
                    }
                }
            }
            if (changes_applied) break;
        }
    }

    // reconstruct asm file
    for (int i = 0; i < instruction_count; i++) {
        Instruction_t instr = instruction[i];
        int ec = instr.expression_count;
        if (instr.is_address) {
            if (instr.expression[0].type == EXPR_ADDRESS) {
                char tmp[32];
                sprintf(tmp, ".address $%.4X", instr.address);
                output = append_to_output(output, &output_len, tmp);
            } else if (instr.expression[0].type == EXPR_RESERVE) {
                char tmp[32];
                sprintf(tmp, ".reserve %s", instr.expression[0].tokens[1].raw);
                output = append_to_output(output, &output_len, tmp);
            }
        } else if (instr.expression[0].type == EXPR_SEGMENT_CODE) {
            output = append_to_output(output, &output_len, ".code");
        } else if (instr.expression[0].type == EXPR_SEGMENT_DATA) {
            output = append_to_output(output, &output_len, ".data");
        } else if (instr.expression[0].type == EXPR_INCBIN) {
            output = append_to_output(output, &output_len, ".incbin ");
            output = append_to_output(output, &output_len, instr.expression[0].tokens[1].raw);
        } else if (instr.expression[0].tokens[0].type == TT_LABEL) {
            //log_msg(LP_INFO, "Label found");
            output = append_to_output(output, &output_len, instr.expression[0].tokens[0].raw);
            output = append_to_output(output, &output_len, " ");
        } else if (instr.expression[0].tokens[0].type == TT_DW) {
            //log_msg(LP_INFO, "Label found");
            output = append_to_output(output, &output_len, instr.expression[0].tokens[0].raw);
            output = append_to_output(output, &output_len, " ");
            output = append_to_output(output, &output_len, instr.expression[0].tokens[1].raw);
        } else if (instr.expression[0].tokens[0].type == TT_DB) {
            //log_msg(LP_INFO, "Label found");
            output = append_to_output(output, &output_len, instr.expression[0].tokens[0].raw);
            output = append_to_output(output, &output_len, " ");
            output = append_to_output(output, &output_len, instr.expression[0].tokens[1].raw);
        } else {
            for (int j = 0; j < ec; j++) {
                if (instr.expression[j].type == EXPR_INSTRUCTION && instr.expression[j].token_count > 1 && instr.expression[j].tokens[1].type == TT_NOCACHE) {
                    output = append_to_output(output, &output_len, "%");
                    output = append_to_output(output, &output_len, instr.expression[j].tokens[0].raw);
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

    if (output == NULL) {
        output = malloc(sizeof("; empty\0"));
        memcpy(output, "; empty\0", sizeof("; empty\0"));
    }

    return output;
}

char* optimizer_compile_from_file(const char* filename) {
    // load raw text from file
    char* content = read_file(filename, NULL);
    if (!content) {
        log_msg(LP_ERROR, "read_file failed [%s:%d]", __FILE__, __LINE__);
        return NULL;
    }
    char* machine_code = optimizer_compile(content);
    
    return machine_code;
}
