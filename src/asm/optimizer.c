#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "ExtendedTypes.h"
#include "utils/Log.h"
#include "utils/IO.h"
#include "utils/String.h"

#include "cpu/cpu_addressing_modes.h"
#include "cpu/cpu_instructions.h"

#include "asm/optimizer.h"
#include "asm/assembler.h"


/*
CRITICAL BUG: Labels inside .data segments are not present in the output!!!
*/

int is_same_adm(CPU_REDUCED_ADDRESSING_MODE_t admr, CPU_EXTENDED_ADDRESSING_MODE_t admx) {
    int result = 0;
    result |= (admr == ADMR_R0 && admx == ADMX_R0);
    result |= (admr == ADMR_R1 && admx == ADMX_R1);
    result |= (admr == ADMR_R2 && admx == ADMX_R2);
    result |= (admr == ADMR_R3 && admx == ADMX_R3);
    result |= (admr == ADMR_SP && admx == ADMX_SP);
    result |= (admr == ADMR_IND_R0 && admx == ADMX_IND_R0);
    return result;
}

int is_same_indirect_adm(CPU_REDUCED_ADDRESSING_MODE_t admr, CPU_EXTENDED_ADDRESSING_MODE_t admx) {
    int result = 0;
    result |= (admr == ADMR_R0 && admx == ADMX_IND_R0);
    result |= (admr == ADMR_R1 && admx == ADMX_IND_R1);
    result |= (admr == ADMR_R2 && admx == ADMX_IND_R2);
    result |= (admr == ADMR_R3 && admx == ADMX_IND_R3);
    result |= (admr == ADMR_SP && admx == ADMX_IND_SP);
    return result;
}

int is_register_admx(CPU_EXTENDED_ADDRESSING_MODE_t admx) {
    int result = 0;
    result |= admx == ADMX_R0;
    result |= admx == ADMX_R1;
    result |= admx == ADMX_R2;
    result |= admx == ADMX_R3;
    result |= admx == ADMX_SP;
    result |= admx == ADMX_PC;
    return result;
}

int is_register_admr(CPU_REDUCED_ADDRESSING_MODE_t admr) {
    int result = 0;
    result |= admr == ADMR_R0;
    result |= admr == ADMR_R1;
    result |= admr == ADMR_R2;
    result |= admr == ADMR_R3;
    result |= admr == ADMR_SP;
    return result;
}

int is_register_offset_admx(CPU_EXTENDED_ADDRESSING_MODE_t admx) {
    int result = 0;
    result |= admx == ADMX_IND_R0_OFFSET16;
    result |= admx == ADMX_IND_R1_OFFSET16;
    result |= admx == ADMX_IND_R2_OFFSET16;
    result |= admx == ADMX_IND_R3_OFFSET16;
    result |= admx == ADMX_IND_SP_OFFSET16;
    result |= admx == ADMX_IND_PC_OFFSET16;
    return result;
}

int register_offset_admx_contains_admr_register(CPU_EXTENDED_ADDRESSING_MODE_t admx, CPU_REDUCED_ADDRESSING_MODE_t admr) {
    int result = 0;
    result |= (admr == ADMR_R0 && admx == ADMX_IND_R0_OFFSET16);
    result |= (admr == ADMR_R1 && admx == ADMX_IND_R1_OFFSET16);
    result |= (admr == ADMR_R2 && admx == ADMX_IND_R2_OFFSET16);
    result |= (admr == ADMR_R3 && admx == ADMX_IND_R3_OFFSET16);
    result |= (admr == ADMR_SP && admx == ADMX_IND_SP_OFFSET16);
    return result;
}

int is_arithmetic_operation(CPU_INSTRUCTION_MNEMONIC_t instr) {
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

int is_overwriting_instruction(CPU_INSTRUCTION_MNEMONIC_t instr) {
    // Meaning, the instruction does not depend on the previous value and is overwriting with new value
    int result = 0;
    result |= instr == MOV;
    result |= instr == LEA;
    return result;
}


void remove_instruction(Instruction_t* instruction, int* instruction_count, int index) {
    if (index < 0 || index >= *instruction_count) return;
    for (int i = index; i < *instruction_count - 1; i++) {
        instruction[i] = instruction[i + 1];
    }
    (*instruction_count)--;
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
                    }
                }
            }
            if (changes_applied) break;
            

            // TODO: Make it look over multiple instructions, that do not modify THAT register
            // mov rN, x
            // mov rN, y
            // =>
            // (removed)
            // mov rN, y
            if (i < instruction_count - 1) {
                if (instruction[i].instruction == MOV && instruction[i + 1].instruction == MOV) {
                    if (   cpu_reduced_addressing_mode_category[instruction[i].admr] == ADMC_REG
                        && instruction[i].admr == instruction[i + 1].admr) {
                        //log_msg(LP_DEBUG, "Optimizer: removed overshadowed mov instruction (line %d)", i);
                        remove_instruction(instruction, &instruction_count, i);
                        i --;
                        changes_applied = 1;
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
                    if (instruction[i].admx == ADMX_IMM16) {
                        if (parse_immediate(instruction[i].expression[2].tokens[0].raw) == 0) {
                            if (instruction[i].admr == instruction[i + 1].admr) {
                                //log_msg(LP_DEBUG, "Optimizer: replaced additive identity from register with direct mov operation (line %d)", i);
                                instruction[i + 1].instruction = MOV;
                                sprintf(instruction[i + 1].expression[0].tokens[0].raw, "%s", cpu_instruction_string[MOV]);
                                remove_instruction(instruction, &instruction_count, i);
                                i --;
                                changes_applied = 1;
                            }
                        }
                    }
                }
            }
            if (changes_applied) break;
            

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
                    }
                }
            }
            if (changes_applied) break;


            // mov r0, $x
            // mov [r0], ?
            // =>
            // (removed)
            // mov [$x], ?
            if (i < instruction_count - 1) {
                if (instruction[i].instruction == MOV && instruction[i + 1].instruction == MOV) {
                    if (   instruction[i].admr == ADMR_R0 
                        && instruction[i].admx == ADMX_IMM16
                        && instruction[i + 1].admr == ADMR_IND_R0
                    ) {
                        //log_msg(LP_DEBUG, "Optimizer: replaced two step immediate dereference write with one step dereference");
                        sprintf(instruction[i + 1].expression[1].tokens[1].raw, "%s", instruction[i].expression[2].tokens[0].raw);
                        instruction[i + 1].admr = ADMR_IND16; 
                        remove_instruction(instruction, &instruction_count, i);
                        changes_applied = 1;
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
                        instruction[i + 1].expression[2] = instruction[i].expression[2];
                        instruction[i + 1].admx = instruction[i].admx;
                        remove_instruction(instruction, &instruction_count, i);
                        i --;
                        changes_applied = 1;
                    }
                }
            }
            if (changes_applied) break;
            

            // mov rN, x
            // op rM, rN
            // lea rN, [?]
            // =>
            // (removed)
            // op rM, x
            // lea rN, [?]
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
                ) && instruction[i + 2].instruction == LEA) {
                    if (instruction[i].admr == instruction[i + 2].admr && is_same_adm(instruction[i].admr, instruction[i + 1].admx)) {
                        //log_msg(LP_DEBUG, "Optimizer: eliminated temporary register forwarding (line %d)", i);
                        instruction[i + 1].expression[2] = instruction[i].expression[2];
                        instruction[i + 1].admx = instruction[i].admx;
                        remove_instruction(instruction, &instruction_count, i);
                        i --;
                        changes_applied = 1;
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
                    }
                }
            }
            if (changes_applied) break;

            // mov rN, x
            // add rN, y
            // =>
            // [precompute] z :: x OP y
            // mov rN, z
            // (removed)
            if (i < instruction_count - 1) {
                if (instruction[i].instruction == MOV && is_arithmetic_operation(instruction[i + 1].instruction)) {
                    if (instruction[i].admr == instruction[i + 1].admr && instruction[i].admx == ADMX_IMM16 && instruction[i + 1].admx == ADMX_IMM16) {
                        //log_msg(LP_DEBUG, "Optimizer: combined arithmetic assign chain (line %d)", i);
                        
                        int16_t value = (int16_t) parse_immediate(instruction[i].expression[2].tokens[0].raw);
                        int16_t value2 = (int16_t) parse_immediate(instruction[i + 1].expression[2].tokens[0].raw);

                        switch(instruction[i + 1].instruction) {
                            case ADD:
                                value += value2;
                                break;
                            case SUB:
                                value -= value2;
                                break;
                            case MUL:
                                value *= value2;
                                break;
                            case DIV:
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
                                log_msg(LP_ERROR, "Optimizer, I dont even know how this would be possible, yet here we are...");
                                break;
                        }

                        sprintf(instruction[i].expression[2].tokens[0].raw, "%d", value);

                        remove_instruction(instruction, &instruction_count, i + 1);
                        changes_applied = 1;
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
                    //log_msg(LP_DEBUG, "Optimizer: replaced lea with direct move instruction (line %d)", i);
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
                    break;
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
                }
            }
            if (changes_applied) break;
            

            // sub sp, x
            // ops
            // sub sp, x
            // =>
            // [precompute] z :: x + y
            // sub sp, z (or equivalent for ADD/SUB chain)
            if (i < instruction_count - 2) {
                if ((instruction[i].instruction == SUB || instruction[i].instruction == ADD) && instruction[i].admr == ADMR_SP) {
                    int16_t sub = (int16_t) parse_immediate(instruction[i].expression[2].tokens[0].raw);
                    if (instruction[i].instruction == ADD) {sub = -sub;}
                    int index = i + 1;
                    while (index < instruction_count) {
                        if (instruction[index].instruction == SUB && instruction[index].admr == ADMR_SP) {
                            //log_msg(LP_DEBUG, "Optimizer: combined stack allocation (line %d)", i);
                            sub += (int16_t) parse_immediate(instruction[index].expression[2].tokens[0].raw);
                            remove_instruction(instruction, &instruction_count, index);
                            changes_applied = 1;
                            continue;
                        }
                        if (instruction[index].instruction == ADD && instruction[index].admr == ADMR_SP) {
                            //log_msg(LP_DEBUG, "Optimizer: combined stack allocation (line %d)", i);
                            sub -= (int16_t) parse_immediate(instruction[index].expression[2].tokens[0].raw);
                            remove_instruction(instruction, &instruction_count, index);
                            changes_applied = 1;
                            continue;
                        }
                        if (instruction[index].instruction == CALL || 
                            instruction[index].instruction == RET || 
                            instruction[index].instruction == PUSH || 
                            instruction[index].instruction == POP || 
                            instruction[index].instruction == PUSHSR || 
                            instruction[index].instruction == POPSR) {
                            break;
                        }
                        if (instruction[index].admx == ADMX_SP || 
                            instruction[index].admx == ADMX_IND_SP || 
                            instruction[index].admx == ADMX_IND_SP_OFFSET16 || 
                            instruction[index].admx == ADMX_IND16_SCALED8_SP_OFFSET) {
                            break;
                        }
                        if ((instruction[index].instruction == MOV || 
                            instruction[index].instruction == CMOVZ || 
                            instruction[index].instruction == CMOVNZ || 
                            instruction[index].instruction == CMOVL || 
                            instruction[index].instruction == CMOVNL || 
                            instruction[index].instruction == CMOVUL || 
                            instruction[index].instruction == CMOVNUL || 
                            instruction[index].instruction == CMOVFL || 
                            instruction[index].instruction == CMOVNFL || 
                            instruction[index].instruction == LEA || 
                            instruction[index].instruction == MUL || 
                            instruction[index].instruction == DIV || 
                            instruction[index].instruction == ADDF || 
                            instruction[index].instruction == SUBF || 
                            instruction[index].instruction == MULF || 
                            instruction[index].instruction == DIVF || 
                            instruction[index].instruction == BWS || 
                            instruction[index].instruction == AND || 
                            instruction[index].instruction == OR || 
                            instruction[index].instruction == XOR || 
                            instruction[index].instruction == NOT) && instruction[index].admr == ADMR_SP) {
                            break;
                        }
                        index ++;
                    }
                    sprintf(instruction[i].expression[0].tokens[0].raw, "%s", cpu_instruction_string[SUB]);
                    sprintf(instruction[i].expression[2].tokens[0].raw, "%d", sub);
                    instruction[i].instruction = SUB;
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
                                int changed_sr = 0;
                                int index = i + 1;
                                while (index < instruction_count) {
                                    if (instruction[index].instruction == JZ || 
                                        instruction[index].instruction == JNZ || 
                                        instruction[index].instruction == JL || 
                                        instruction[index].instruction == JNL ||
                                        instruction[index].instruction == JUL || 
                                        instruction[index].instruction == JNUL ||
                                        instruction[index].instruction == JFL || 
                                        instruction[index].instruction == JNFL ||
                                        instruction[index].instruction == JSO || 
                                        instruction[index].instruction == JNSO ||
                                        instruction[index].instruction == JAO || 
                                        instruction[index].instruction == JNAO ||
                                        instruction[index].instruction == CMOVZ || 
                                        instruction[index].instruction == CMOVNZ ||
                                        instruction[index].instruction == CMOVL || 
                                        instruction[index].instruction == CMOVNL ||
                                        instruction[index].instruction == CMOVUL || 
                                        instruction[index].instruction == CMOVNUL ||
                                        instruction[index].instruction == CMOVFL || 
                                        instruction[index].instruction == CMOVNFL ||
                                        instruction[index].instruction < 0 ||
                                        instruction[index].instruction >= INSTRUCTION_COUNT
                                    ) {
                                        break;
                                    }
                                    if (instruction[index].instruction == CMP || 
                                        instruction[index].instruction == TST || 
                                        instruction[index].instruction == POPSR ||
                                        instruction[index].instruction == ADD || 
                                        instruction[index].instruction == SUB || 
                                        instruction[index].instruction == MUL || 
                                        instruction[index].instruction == DIV || 
                                        instruction[index].instruction == ADDF || 
                                        instruction[index].instruction == SUBF || 
                                        instruction[index].instruction == MULF || 
                                        instruction[index].instruction == DIVF || 
                                        instruction[index].instruction == NEG || 
                                        instruction[index].instruction == INC || 
                                        instruction[index].instruction == DEC || 
                                        instruction[index].instruction == BWS || 
                                        instruction[index].instruction == AND || 
                                        instruction[index].instruction == OR || 
                                        instruction[index].instruction == XOR || 
                                        instruction[index].instruction == NOT
                                    ) {
                                        changed_sr = 1;
                                        break;
                                    }
                                    index ++;
                                }
                                if (changed_sr) {
                                    //log_msg(LP_DEBUG, "Optimizer: removed redundant and operation (line %d)", i);
                                    remove_instruction(instruction, &instruction_count, i);
                                    i --;
                                    changes_applied = 1;
                                }
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
                                int changed_sr = 0;
                                int index = i + 1;
                                while (index < instruction_count) {
                                    if (instruction[index].instruction == JZ || 
                                        instruction[index].instruction == JNZ || 
                                        instruction[index].instruction == JL || 
                                        instruction[index].instruction == JNL ||
                                        instruction[index].instruction == JUL || 
                                        instruction[index].instruction == JNUL ||
                                        instruction[index].instruction == JFL || 
                                        instruction[index].instruction == JNFL ||
                                        instruction[index].instruction == JSO || 
                                        instruction[index].instruction == JNSO ||
                                        instruction[index].instruction == JAO || 
                                        instruction[index].instruction == JNAO ||
                                        instruction[index].instruction == CMOVZ || 
                                        instruction[index].instruction == CMOVNZ ||
                                        instruction[index].instruction == CMOVL || 
                                        instruction[index].instruction == CMOVNL ||
                                        instruction[index].instruction == CMOVUL || 
                                        instruction[index].instruction == CMOVNUL ||
                                        instruction[index].instruction == CMOVFL || 
                                        instruction[index].instruction == CMOVNFL ||
                                        instruction[index].instruction < 0 ||
                                        instruction[index].instruction >= INSTRUCTION_COUNT
                                    ) {
                                        break;
                                    }
                                    if (instruction[index].instruction == CMP || 
                                        instruction[index].instruction == TST || 
                                        instruction[index].instruction == POPSR ||
                                        instruction[index].instruction == ADD || 
                                        instruction[index].instruction == SUB || 
                                        instruction[index].instruction == MUL || 
                                        instruction[index].instruction == DIV || 
                                        instruction[index].instruction == ADDF || 
                                        instruction[index].instruction == SUBF || 
                                        instruction[index].instruction == MULF || 
                                        instruction[index].instruction == DIVF || 
                                        instruction[index].instruction == NEG || 
                                        instruction[index].instruction == INC || 
                                        instruction[index].instruction == DEC || 
                                        instruction[index].instruction == BWS || 
                                        instruction[index].instruction == AND || 
                                        instruction[index].instruction == OR || 
                                        instruction[index].instruction == XOR || 
                                        instruction[index].instruction == NOT
                                    ) {
                                        changed_sr = 1;
                                        break;
                                    }
                                    index ++;
                                }
                                if (changed_sr) {
                                    //log_msg(LP_DEBUG, "Optimizer: removed redundant and operation (line %d)", i);
                                    remove_instruction(instruction, &instruction_count, i);
                                    i --;
                                    changes_applied = 1;
                                }
                            }
                        }
                    }
                }
            }
            if (changes_applied) break;
            
        }
    }

    // LOW PRIORITY OPTIMIZATIONS
    // for optimizations that would destroy canonicalization and prevent other pattern optimizations from being matched
    changes_applied = 1;
    while (changes_applied) {
        changes_applied = 0;
        // Optimize
        for (int i = 0; i < instruction_count; i++) {
            
            // mov r0, 0
            // =>
            // xor r0, r0 
            // if no status register reads follow
            // Issue: It can cause more cache misses!
            if (instruction[i].instruction == MOV) {
                if (instruction[i].admx == ADMX_IMM16) {
                    if (instruction[i].admr == ADMR_R0 || instruction[i].admr == ADMR_R1 || 
                        instruction[i].admr == ADMR_R2 || instruction[i].admr == ADMR_R3 || 
                        instruction[i].admr == ADMR_SP) {
                        if (parse_immediate(instruction[i].expression[2].tokens[0].raw) == 0 && string_is_immediate(instruction[i].expression[2].tokens[0].raw)) {
                            // now also check if any conditional instruction is present before another statur-register changing operation is done
                            int changed_sr = 0;
                            int index = i + 1;
                            while (index < instruction_count) {
                                if (instruction[index].instruction == JZ || 
                                    instruction[index].instruction == JNZ || 
                                    instruction[index].instruction == JL || 
                                    instruction[index].instruction == JNL ||
                                    instruction[index].instruction == JUL || 
                                    instruction[index].instruction == JNUL ||
                                    instruction[index].instruction == JFL || 
                                    instruction[index].instruction == JNFL ||
                                    instruction[index].instruction == JSO || 
                                    instruction[index].instruction == JNSO ||
                                    instruction[index].instruction == JAO || 
                                    instruction[index].instruction == JNAO ||
                                    instruction[index].instruction == CMOVZ || 
                                    instruction[index].instruction == CMOVNZ ||
                                    instruction[index].instruction == CMOVL || 
                                    instruction[index].instruction == CMOVNL ||
                                    instruction[index].instruction == CMOVUL || 
                                    instruction[index].instruction == CMOVNUL ||
                                    instruction[index].instruction == CMOVFL || 
                                    instruction[index].instruction == CMOVNFL ||
                                    instruction[index].instruction < 0 ||
                                    instruction[index].instruction >= INSTRUCTION_COUNT
                                ) {
                                    break;
                                }
                                if (instruction[index].instruction == CMP || 
                                    instruction[index].instruction == TST || 
                                    instruction[index].instruction == POPSR ||
                                    instruction[index].instruction == ADD || 
                                    instruction[index].instruction == SUB || 
                                    instruction[index].instruction == MUL || 
                                    instruction[index].instruction == DIV || 
                                    instruction[index].instruction == ADDF || 
                                    instruction[index].instruction == SUBF || 
                                    instruction[index].instruction == MULF || 
                                    instruction[index].instruction == DIVF || 
                                    instruction[index].instruction == NEG || 
                                    instruction[index].instruction == INC || 
                                    instruction[index].instruction == DEC || 
                                    instruction[index].instruction == BWS || 
                                    instruction[index].instruction == AND || 
                                    instruction[index].instruction == OR || 
                                    instruction[index].instruction == XOR || 
                                    instruction[index].instruction == NOT
                                ) {
                                    changed_sr = 1;
                                    break;
                                }
                                index ++;
                            }
                            if (changed_sr) {
                                //log_msg(LP_DEBUG, "Optimizer: replaced 0-mov with xor operation (rationale: reduce size of executable) (line %d)", i);
                                sprintf(instruction[i].expression[0].tokens[0].raw, "%s", cpu_instruction_string[XOR]);
                                sprintf(instruction[i].expression[2].tokens[0].raw, "%s", instruction[i].expression[1].tokens[0].raw);
                                instruction[i].instruction = XOR;
                                instruction[i].admx = ADMX_R0 + instruction[i].admr - ADMR_R0;
                                changes_applied = 1;
                            }
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

char* optimizer_compile_from_file(const char* filename) {
    // load raw text from file
    char* content = read_file(filename, NULL);
    if (!content) {
        log_msg(LP_ERROR, "read_file failed");
        return NULL;
    }
    char* machine_code = optimizer_compile(content);
    
    return machine_code;
}
