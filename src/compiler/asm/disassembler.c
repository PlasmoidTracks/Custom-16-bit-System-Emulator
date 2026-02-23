#include <ctype.h>
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
#include "compiler/asm/disassembler.h"


char disassembler_4bits_to_hex(uint8_t value) {
    if (value < 0x0a) return '0' + value;
    return 'A' + (value - 0x0a);
}

uint16_t disassembler_read_u16(uint8_t* data) {
    return (data[1] << 8) | data[0];
}



char* disassembler_decompile_single_instruction(uint8_t* binary, int* binary_index, int* valid_instruction, CPU_REDUCED_ADDRESSING_MODE_t* extern_admr, CPU_EXTENDED_ADDRESSING_MODE_t* extern_admx, int* instruction_bytes, DisassembleOption_t options) {
    int instruction = binary[(*binary_index) ++];
    if (instruction_bytes) *instruction_bytes = 0;

    int ext_count = 0;
    while ((instruction & 0x7f) == EXT) {
        instruction = binary[(*binary_index) ++];
        ext_count ++;
        if (instruction_bytes) *instruction_bytes += 1; // given that EXT is a one-byte-instruction
    }

    uint8_t addressing_mode = 0;
    int one_byte_instruction = 1;
    if (valid_instruction) *valid_instruction = 1;

    int no_cache = 0;

    no_cache = (instruction & 0x80) != 0;
    instruction &= 0x7f;

    instruction += ext_count * 0x80;

    if (cpu_instruction_argument_count[instruction] > 0) {
        addressing_mode = binary[(*binary_index) ++];
        one_byte_instruction = 0;
    }

    if (instruction >= INSTRUCTION_COUNT) {
        //log_msg(LP_ERROR, "Disassembler: Unknown instruction [%.2x] [%s:%d]", instruction, __FILE__, __LINE__);
        if (valid_instruction) *valid_instruction = 0;
        return strdup("");  // makes it heap allocated
    }
    if (instruction == NOP) {
        if (valid_instruction) *valid_instruction = 0;
        if (instruction_bytes) *instruction_bytes += 2 - one_byte_instruction;
        if (no_cache) {return strdup("%nop");}
        return strdup("nop");  // makes it heap allocated
    }

    uint8_t admr = addressing_mode & 0x07;
    uint8_t admx = (addressing_mode & 0xf8) >> 3;

    if (extern_admr) {
        *extern_admr = admr;
    }
    if (extern_admx) {
        *extern_admx = admx;
    }

    //printf("admr: %s, admx: %s\n", cpu_reduced_addressing_mode_string[admr], cpu_extended_addressing_mode_string[admx]);

    int argument_count = cpu_instruction_argument_count[instruction];

    int data_size = 0;

    if (argument_count == 2) {
        //log_msg(LP_INFO, "decompile: instruction \"%s %s, %s\"", cpu_instruction_string[instruction], cpu_reduced_addressing_mode_string[admr], cpu_extended_addressing_mode_string[admx]);
        data_size += cpu_extended_addressing_mode_bytes[admx];
        data_size += cpu_reduced_addressing_mode_bytes[admr];
    } else if (argument_count == 0) {
        //log_msg(LP_INFO, "decompile: instruction \"%s\"", cpu_instruction_string[instruction]);
        //log_msg(LP_INFO, "decompile: admr, amdx: \"%s\", \"%s\"", cpu_reduced_addressing_mode_string[admr], cpu_extended_addressing_mode_string[admx]);
        if (admr != 0 || admx != 0) {
            //log_msg(LP_DEBUG, "Instructions with no argument, but set adm is not realistic. Assuming it is raw data instead");
            if (valid_instruction) *valid_instruction = 0;
            return "";
        }
    } else if (argument_count == 1) {
        if (cpu_instruction_single_operand_writeback[instruction]) {
            if (admx != 0) {
                //log_msg(LP_DEBUG, "Instructions with one writeback argument (admr), but set admx is not realistic. Assuming it is raw data instead");
                if (valid_instruction) *valid_instruction = 0;
                //return "";
            }
            //log_msg(LP_INFO, "decompile: instruction \"%s, %s\"", cpu_instruction_string[instruction], cpu_reduced_addressing_mode_string[admr]);
            data_size += cpu_reduced_addressing_mode_bytes[admr];
        } else {
            if (admr != 0) {
                //log_msg(LP_DEBUG, "Instructions with one argument (admx), but set admr is not realistic. Assuming it is raw data instead");
                if (valid_instruction) *valid_instruction = 0;
                //return "";
            }
            //log_msg(LP_INFO, "decompile: instruction \"%s, %s\"", cpu_instruction_string[instruction], cpu_extended_addressing_mode_string[admx]);
            data_size += cpu_extended_addressing_mode_bytes[admx];
        }
    }

    uint8_t data[data_size];

    for (int i = 0; i < data_size; i++) {
        data[i] = binary[*binary_index + i];
        //printf("data[%d]: %.2x\n", i+1, data[i]);
    }

    (*binary_index) += data_size;

    if (instruction_bytes) {
        *instruction_bytes += data_size + 2 - one_byte_instruction;
    }

    // should be plenty of space
    char* instruction_string = calloc(256, sizeof(char)); 

    char* instruction_string_pointer = instruction_string;
    if (no_cache) {
        instruction_string[0] = '%';
        instruction_string_pointer = &instruction_string[1];
    }

    int instruction_string_index = 0;
    if (cpu_instruction_string[instruction]) {
        strcpy(instruction_string_pointer, cpu_instruction_string[instruction]);
        instruction_string_index = strlen(cpu_instruction_string[instruction]);
    }

    if (argument_count == 0) {
        //printf("instruction_string: %s\n", instruction_string);
        char* instruction_string_newline = calloc(256, sizeof(char)); 
        sprintf(instruction_string_newline, "%s", instruction_string);
        free(instruction_string);

        return instruction_string_newline;
    }
    
    instruction_string_pointer[instruction_string_index++] = ' ';

    char* reg = NULL;

    

    if (argument_count == 2 || (argument_count == 1 && cpu_instruction_single_operand_writeback[instruction])) {
        // next up: admr
        switch (admr) {
            case ADMR_R0: reg = "r0"; goto admr_write_register;
            case ADMR_R1: reg = "r1"; goto admr_write_register;
            case ADMR_R2: reg = "r2"; goto admr_write_register;
            case ADMR_R3: reg = "r3"; goto admr_write_register;
            case ADMR_SP: reg = "sp"; goto admr_write_register;
            admr_write_register:
                instruction_string_pointer[instruction_string_index++] = reg[0];
                instruction_string_pointer[instruction_string_index++] = reg[1];
                break;


            case ADMR_IND16:
                {
                    int argument_byte_offset = 0;
                    if (argument_count != 1) {
                        switch (admx) {
                            case ADMX_IMM16: case ADMX_IND16:
                            case ADMX_IND_R0_OFFSET16: case ADMX_IND_R1_OFFSET16:
                            case ADMX_IND_R2_OFFSET16: case ADMX_IND_R3_OFFSET16:
                            case ADMX_IND_PC_OFFSET16:
                                argument_byte_offset = 2;
                                break;
                            case ADMX_IND16_SCALED8_R0_OFFSET: case ADMX_IND16_SCALED8_R1_OFFSET:
                            case ADMX_IND16_SCALED8_R2_OFFSET: case ADMX_IND16_SCALED8_R3_OFFSET:
                            case ADMX_IND16_SCALED8_PC_OFFSET:
                                argument_byte_offset = 3;
                                break;
                            default:
                                break;
                        }
                    }
                    char str[16];
                    uint16_t value = disassembler_read_u16(data + argument_byte_offset);
                    sprintf(str, "[$%04X]", value);
                    strcpy(&instruction_string_pointer[instruction_string_index], str);
                    instruction_string_index += strlen(str);
                }
                break;
            
            case ADMR_IND_R0:
                {
                    char str[5];
                    str[0] = '[';
                    strcpy(str + 1, cpu_reduced_addressing_mode_string[admr - ADMR_IND_R0 + 1]);
                    str[1] = tolower(str[1]);
                    str[2] = tolower(str[2]);
                    str[3] = ']';
                    str[4] = '\0';
                    strcpy(&instruction_string_pointer[instruction_string_index], str);
                    instruction_string_index += 4;
                }
                break;
            
            case ADMR_NONE:
                strcpy(&instruction_string_pointer[instruction_string_index], "_");
                instruction_string_index += 1;
                if (valid_instruction) *valid_instruction = 0;
                break;

            default:
                //log_msg(LP_ERROR, "Unsupported admr \"%s\" [%s:%d]", cpu_reduced_addressing_mode_string[admr], __FILE__, __LINE__);
                if (valid_instruction) *valid_instruction = 0;
                break;
        }
    } 
    if (argument_count == 2 || (argument_count == 1 && !cpu_instruction_single_operand_writeback[instruction])) {
        if (argument_count == 2) {
            instruction_string_pointer[instruction_string_index++] = ',';
            instruction_string_pointer[instruction_string_index++] = ' ';
        }
        // next up: admx
        switch (admx) {
            case ADMX_R0: reg = "r0"; goto admx_write_register;
            case ADMX_R1: reg = "r1"; goto admx_write_register;
            case ADMX_R2: reg = "r2"; goto admx_write_register;
            case ADMX_R3: reg = "r3"; goto admx_write_register;
            case ADMX_SP: reg = "sp"; goto admx_write_register;
            case ADMX_PC: reg = "pc"; goto admx_write_register;
            admx_write_register:
                instruction_string_pointer[instruction_string_index++] = reg[0];
                instruction_string_pointer[instruction_string_index++] = reg[1];
                break;
            
            
            case ADMX_IMM16:
                {
                    if (options & DO_USE_FLOAT_LITERALS) {
                        float16_t value = data[0] | (data[1] << 8);
                        float v = float_from_f16(value);
                        if (v != 0.0 
                            && v >= 0.01 
                            && v <= 65535.0 
                            && (v - (float)((int) v) == 0 
                                || (value & 0x00ff) == 0 // here, checking for the mantissa, if its more than two digits, fallback to hex
                                || v == float_from_f16(f16_from_float(3.14159265358979323)) 
                                || v == float_from_f16(f16_from_float(2.71828182845904523)))
                            ) 
                        {
                            char buf[64];
                            sprintf(buf, "f%f", float_from_f16(value));
                            sprintf(&instruction_string_pointer[instruction_string_index], "%s", buf);
                            instruction_string_index += strlen(buf);
                        } else {
                            sprintf(&instruction_string_pointer[instruction_string_index], "$%.4X", data[0] | (data[1] << 8));
                            instruction_string_index += 5;
                        }
                    } else {
                        sprintf(&instruction_string_pointer[instruction_string_index], "$%.4X", data[0] | (data[1] << 8));
                        instruction_string_index += 5;
                    }
                }
                break;

            case ADMX_IND16:
                {
                    sprintf(&instruction_string_pointer[instruction_string_index], "[$%.4X]", data[0] | (data[1] << 8));
                    instruction_string_index += 7;
                }
                break;
            
            case ADMX_IND_R0: reg = "r0"; goto admx_write_indirect_reg;
            case ADMX_IND_R1: reg = "r1"; goto admx_write_indirect_reg;
            case ADMX_IND_R2: reg = "r2"; goto admx_write_indirect_reg;
            case ADMX_IND_R3: reg = "r3"; goto admx_write_indirect_reg;
            case ADMX_IND_SP: reg = "sp"; goto admx_write_indirect_reg;
            case ADMX_IND_PC: reg = "pc"; goto admx_write_indirect_reg;
            admx_write_indirect_reg:
                sprintf(&instruction_string_pointer[instruction_string_index], "[%c%c]", reg[0], reg[1]);
                instruction_string_index += 4;
                break;
            
            case ADMX_IND_R0_OFFSET16: reg = "r0"; goto admx_write_offset16;
            case ADMX_IND_R1_OFFSET16: reg = "r1"; goto admx_write_offset16;
            case ADMX_IND_R2_OFFSET16: reg = "r2"; goto admx_write_offset16;
            case ADMX_IND_R3_OFFSET16: reg = "r3"; goto admx_write_offset16;
            case ADMX_IND_SP_OFFSET16: reg = "sp"; goto admx_write_offset16;
            case ADMX_IND_PC_OFFSET16: reg = "pc"; goto admx_write_offset16;
            admx_write_offset16: {
                uint16_t offset = disassembler_read_u16(data);
                char str[32];
                sprintf(str, "[$%.4X + %s]", offset, reg);
                strcpy(&instruction_string_pointer[instruction_string_index], str);
                instruction_string_index += strlen(str);
                break;
            }
            
            case ADMX_IND16_SCALED8_R0_OFFSET: reg = "r0"; goto admx_write_scaled;
            case ADMX_IND16_SCALED8_R1_OFFSET: reg = "r1"; goto admx_write_scaled;
            case ADMX_IND16_SCALED8_R2_OFFSET: reg = "r2"; goto admx_write_scaled;
            case ADMX_IND16_SCALED8_R3_OFFSET: reg = "r3"; goto admx_write_scaled;
            case ADMX_IND16_SCALED8_SP_OFFSET: reg = "sp"; goto admx_write_scaled;
            case ADMX_IND16_SCALED8_PC_OFFSET: reg = "pc"; goto admx_write_scaled;
            admx_write_scaled: {
                uint16_t base = disassembler_read_u16(data);
                uint8_t scale = data[2];
                char str[32];
                sprintf(str, "[$%04X + $%02X * %s]", base, scale, reg);
                strcpy(&instruction_string_pointer[instruction_string_index], str);
                instruction_string_index += strlen(str);
                break;
            }
            
            case ADMX_NONE:
                strcpy(&instruction_string_pointer[instruction_string_index], "_");
                instruction_string_index += 1;
                if (valid_instruction) *valid_instruction = 0;
                break;
            
            default:
            //log_msg(LP_ERROR, "Unsupported admx \"%s\" [%s:%d]", cpu_extended_addressing_mode_string[admx], __FILE__, __LINE__);
            if (valid_instruction) *valid_instruction = 0;
                break;
        }
    }

    char* instruction_string_newline = calloc(256, sizeof(char)); 
    sprintf(instruction_string_newline, "%s", instruction_string);
    free(instruction_string);

    return instruction_string_newline;
}

Disassembly_t disassembler_naive_decompile(uint8_t* machine_code, long binary_size, int* assembly_lines, uint16_t* segment, int segment_count, DisassembleOption_t options) {

    unsigned long allocated_space = binary_size;
    Disassembly_t disassembly;
    disassembly.code = calloc(allocated_space, sizeof(char[256]));
    disassembly.binary_index = calloc(allocated_space, sizeof(int));
    disassembly.admr = calloc(allocated_space, sizeof(CPU_REDUCED_ADDRESSING_MODE_t));
    disassembly.admx = calloc(allocated_space, sizeof(CPU_EXTENDED_ADDRESSING_MODE_t));
    disassembly.is_data = calloc(allocated_space, sizeof(int));
    disassembly.lines = allocated_space;
    for (unsigned long i = 0; i < allocated_space; i++) {
        //disassembly.code[i] = malloc(sizeof(char) * 256);
    }
    unsigned long assembly_index = 0;

    int binary_index = 0;
    int set_data_segment = 0;
    int nop_detected = 0;

    while (binary_index < binary_size) {

        int previous_binary_index = binary_index;
        int valid_instruction = 0;
        int instruction_bytes = 0;
        int tmp_binary_index = binary_index + 1;
        int one_byte_ahead_valid_instruction = 0;
        char* instruction = disassembler_decompile_single_instruction(machine_code, &binary_index, &valid_instruction, &disassembly.admr[assembly_index], &disassembly.admx[assembly_index], &instruction_bytes, options);
        // probe one byte offset for instruction encoding, used to determine in .data segment if last entry should be byte or word. 
        char* one_byte_ahead_instruction = disassembler_decompile_single_instruction(machine_code, &tmp_binary_index, &one_byte_ahead_valid_instruction, NULL, NULL, NULL, options);
        free(one_byte_ahead_instruction);
        
        disassembly.is_data[assembly_index] = 0;

        // I dont know if this is even good to do like this
        // TODO: Instead, just write ".data" and let it be data until we encounter valid instructions again. 
        if (!valid_instruction) { // this includes "nop"
            //log_msg(LP_WARNING, "Disassembler: Encountered an invalid instruction. Realigning to the next byte and retrying [%s] [%s:%d]", instruction, __FILE__, __LINE__);

            while (assembly_index + 3 >= allocated_space) {
                allocated_space *= 2;
                //log_msg(LP_DEBUG, "Disassembler: Increased allocated space");
                disassembly.code = realloc(disassembly.code, allocated_space * sizeof(char[256]));
                disassembly.binary_index = realloc(disassembly.binary_index, allocated_space * sizeof(int));
                disassembly.lines = allocated_space;
                disassembly.is_data = realloc(disassembly.is_data, allocated_space * sizeof(int));
                disassembly.admr = realloc(disassembly.admr, allocated_space * sizeof(CPU_REDUCED_ADDRESSING_MODE_t));
                disassembly.admx = realloc(disassembly.admx, allocated_space * sizeof(CPU_EXTENDED_ADDRESSING_MODE_t));
                for (unsigned long i = assembly_index; i < allocated_space; i++) {
                    //disassembly.code[i] = malloc(sizeof(char) * 256);
                }
            }
            
            //if (strcmp(instruction, "nop") == 0) {
            //printf("got byte 0x%.2x\n", machine_code[previous_binary_index]);
            int condition = machine_code[previous_binary_index] == 0x00;
            if (options & DO_ALIGN_ADDRESS_JUMP) {
                condition &= machine_code[previous_binary_index + 1] == 0x00;
            }
            if (condition) {
                nop_detected = 1;
                binary_index = previous_binary_index + 1;
                disassembly.is_data[assembly_index] = 1;
                if (options & DO_ALIGN_ADDRESS_JUMP) {
                    binary_index ++;
                }
                continue;
            } else {

                if (nop_detected /*&& !(options & DO_USE_NOP_AS_PAD)*/) {
                    disassembly.binary_index[assembly_index] = previous_binary_index;
                    disassembly.is_data[assembly_index] = 1;
                    sprintf(disassembly.code[assembly_index++], ".address $%.4X\n", previous_binary_index);
                    nop_detected = 0;
                }
                // I could show the code length in bytes by "binary_index - previous_binary_index"
    
                if (!set_data_segment) {
                    disassembly.binary_index[assembly_index] = previous_binary_index;
                    disassembly.is_data[assembly_index] = 1;
                    sprintf(disassembly.code[assembly_index++], ".data\n");
                    //sprintf(disassembly.code[assembly_index++], ".align 2\n");
                    set_data_segment = 1;
                }

                if (instruction[0] != '\0') {
                    // Calculate padding so the second ';' starts at column 34
                    int len = snprintf(NULL, 0, "$%.2X                      ; %s\n", machine_code[previous_binary_index], instruction);
                    int padding = 53 - len;
                    if (padding < 0) padding = 1; // Ensure at least one space before second semicolon
                    disassembly.binary_index[assembly_index] = previous_binary_index;
                    disassembly.is_data[assembly_index] = 1;
                    if (options & DO_USE_FLOAT_LITERALS) {
                        float16_t value = machine_code[previous_binary_index] | (machine_code[previous_binary_index + 1] << 8);
                        float v = float_from_f16(value);
                        if (v != 0.0 && v >= 0.01 && v <= 65535.0 && (
                            v - (float)((int) v) == 0 || 
                            (value & 0x00ff) == 0 ||    // here, checking for the mantissa, if its more than two digits, fallback to hex
                            v == float_from_f16(f16_from_float(3.14159265358979323)) ||
                            v == float_from_f16(f16_from_float(2.71828182845904523)))) {
                            sprintf(disassembly.code[assembly_index++], "f%f\n", float_from_f16(value));
                            binary_index = previous_binary_index + 2;
                        } else {
                            if (options & DO_ADD_SPECULATIVE_CODE) {
                                if (instruction_bytes == 1) {
                                    sprintf(disassembly.code[assembly_index++], ".db $%.2X                        ; %s%*s; %d byte instruction\n",
                                        machine_code[previous_binary_index], instruction, padding, "", binary_index - previous_binary_index
                                    );
                                    binary_index = previous_binary_index + 1;
                                } else {
                                    sprintf(disassembly.code[assembly_index++], ".dw $%.2X%.2X                      ; %s%*s; %d byte instruction\n",
                                        machine_code[previous_binary_index + 1], machine_code[previous_binary_index], instruction, padding, "", binary_index - previous_binary_index
                                    );
                                    binary_index = previous_binary_index + 2;
                                }
                            } else {
                                if (one_byte_ahead_valid_instruction == 1) {
                                    sprintf(disassembly.code[assembly_index++], ".db $%.2X\n",
                                        machine_code[previous_binary_index]
                                    );
                                    binary_index = previous_binary_index + 1;
                                } else {
                                    sprintf(disassembly.code[assembly_index++], ".dw $%.2X%.2X\n",
                                        machine_code[previous_binary_index + 1], machine_code[previous_binary_index]
                                    );
                                    binary_index = previous_binary_index + 2;
                                }
                            }
                        }
                    } else {
                        if (options & DO_ADD_SPECULATIVE_CODE) {
                            if (instruction_bytes == 1) {
                                sprintf(disassembly.code[assembly_index++], ".db $%.2X                        ; %s%*s; %d byte instruction\n",
                                    machine_code[previous_binary_index], instruction, padding, "", binary_index - previous_binary_index
                                );
                                binary_index = previous_binary_index + 1;
                            } else {
                                sprintf(disassembly.code[assembly_index++], ".dw $%.2X%.2X                      ; %s%*s; %d byte instruction\n",
                                    machine_code[previous_binary_index + 1], machine_code[previous_binary_index], instruction, padding, "", binary_index - previous_binary_index
                                );
                                binary_index = previous_binary_index + 2;
                            }
                        } else {
                            if (one_byte_ahead_valid_instruction == 1) {
                                sprintf(disassembly.code[assembly_index++], ".db $%.2X\n",
                                    machine_code[previous_binary_index]
                                );
                                binary_index = previous_binary_index + 1;
                            } else {
                                sprintf(disassembly.code[assembly_index++], ".dw $%.2X%.2X\n",
                                    machine_code[previous_binary_index + 1], machine_code[previous_binary_index]
                                );
                                binary_index = previous_binary_index + 2;
                            }
                        }
                    }
                } else {
                    if (options & DO_USE_FLOAT_LITERALS) {
                        float16_t value = machine_code[previous_binary_index] | (machine_code[previous_binary_index + 1] << 8);
                        float v = float_from_f16(value);
                        if (v != 0.0 && v >= 0.01 && v <= 65535.0 && (
                            v - (float)((int) v) == 0 || 
                            (value & 0x00ff) == 0 ||    // here, checking for the mantissa, if its more than two digits, fallback to hex
                            v == float_from_f16(f16_from_float(3.14159265358979323)) ||
                            v == float_from_f16(f16_from_float(2.71828182845904523)))) {
                            sprintf(disassembly.code[assembly_index++], "f%f\n", float_from_f16(value));
                        } else {
                            //sprintf(disassembly.code[assembly_index++], ".dw $%.2X%.2X\n", machine_code[previous_binary_index + 1], machine_code[previous_binary_index]);
                            if (one_byte_ahead_valid_instruction == 1) {
                                sprintf(disassembly.code[assembly_index++], ".db $%.2X\n",
                                    machine_code[previous_binary_index]
                                );
                                binary_index = previous_binary_index + 1;
                            } else {
                                sprintf(disassembly.code[assembly_index++], ".dw $%.2X%.2X\n",
                                    machine_code[previous_binary_index + 1], machine_code[previous_binary_index]
                                );
                                binary_index = previous_binary_index + 2;
                            }
                        }
                        //binary_index = previous_binary_index + 2;
                    } else {
                        disassembly.binary_index[assembly_index] = previous_binary_index;
                        disassembly.is_data[assembly_index] = 1;
                        //sprintf(disassembly.code[assembly_index++], ".dw $%.2X%.2X\n", machine_code[previous_binary_index + 1], machine_code[previous_binary_index]);
                        if (one_byte_ahead_valid_instruction == 1) {
                            sprintf(disassembly.code[assembly_index++], ".db $%.2X\n",
                                machine_code[previous_binary_index]
                            );
                            binary_index = previous_binary_index + 1;
                        } else {
                            sprintf(disassembly.code[assembly_index++], ".dw $%.2X%.2X\n",
                                machine_code[previous_binary_index + 1], machine_code[previous_binary_index]
                            );
                            binary_index = previous_binary_index + 2;
                        }
                        //binary_index = previous_binary_index + 2;
                    }
                }

                
            }
            continue;
        } else {
            if (nop_detected /*&& !(options & DO_USE_NOP_AS_PAD)*/) {
                disassembly.binary_index[assembly_index] = previous_binary_index;
                disassembly.is_data[assembly_index] = 0;
                sprintf(disassembly.code[assembly_index++], ".address $%.4X\n", previous_binary_index);
                nop_detected = 0;
            }
            if (set_data_segment) {
                // since data segment was set, we need to reset to code segment
                disassembly.binary_index[assembly_index] = previous_binary_index;
                disassembly.is_data[assembly_index] = 0;
                sprintf(disassembly.code[assembly_index++], ".code\n");
                set_data_segment = 0;
            }
        }

        // check for crossing segments and realign. We do this before writing the current decoded instruction, because
        // this very instruction could be from an overlap, say reading from 0x00fe over to 0x0102, 
        // and if the segment is at 0x0100, then this instruction is garbage
        int realign = 0;
        if (segment) {
            for (int i = 0; i < segment_count; i++) {
                if (previous_binary_index < segment[i] && binary_index > segment[i]) {
                    //log_msg(LP_WARNING, "Disassembler: Detected segment overlap. realigning from 0x%.4x to 0x%.4x [%s:%d]", binary_index, segment[i], __FILE__, __LINE__);
                    //log_msg(LP_INFO, "Disassembler: This could mean that the previous instructions were wrongfully decompiled");
                    disassembly.is_data[assembly_index] = 1;
                    binary_index = segment[i];
                    realign = 1;
                    break;
                }
            }
        }
        if (realign) continue;


        if (instruction == NULL) {
            //log_msg(LP_ERROR, "Disassembler: Decompilation failed [%s:%d]", __FILE__, __LINE__);
            return (Disassembly_t) {0};
        }

        while (assembly_index >= allocated_space) {
            allocated_space *= 2;
            //log_msg(LP_DEBUG, "Disassembler: Increased allocated space");
            disassembly.code = realloc(disassembly.code, allocated_space * sizeof(char[256]));
            disassembly.binary_index = realloc(disassembly.binary_index, allocated_space * sizeof(int));
            disassembly.lines = allocated_space;
            disassembly.is_data = realloc(disassembly.is_data, allocated_space * sizeof(int));
            disassembly.admr = realloc(disassembly.admr, allocated_space * sizeof(CPU_REDUCED_ADDRESSING_MODE_t));
            disassembly.admx = realloc(disassembly.admx, allocated_space * sizeof(CPU_EXTENDED_ADDRESSING_MODE_t));
            for (unsigned long i = assembly_index; i < allocated_space; i++) {
                //disassembly.code[i] = malloc(sizeof(char) * 256);
            }
        }

        //strcpy(disassembly.code[assembly_index], instruction);
        if (options & DO_ADD_RAW_BYTES) {
            char tmp[256];
            sprintf(tmp, "; [0x%.4x] : ", previous_binary_index);
            for (int i = 0; i < instruction_bytes; i++) {
                sprintf(&tmp[strlen(tmp)], "0x%.2x ",
                    machine_code[previous_binary_index + i]
                );
            }
            sprintf(&tmp[strlen(tmp)], "\n");
            sprintf(disassembly.code[assembly_index], "%s", tmp);
            assembly_index++;
        }
        sprintf(disassembly.code[assembly_index], "%s\n", instruction);
        free(instruction);
        disassembly.binary_index[assembly_index] = previous_binary_index;
        assembly_index ++;
    }

    *assembly_lines = assembly_index;

    return disassembly;
}



char* disassembler_decompile(uint8_t* machine_code, long binary_size, uint16_t* segment, int segment_count, DisassembleOption_t options) {

    // first pass, naive disassembly
    int assembly_lines;
    Disassembly_t assembly_code = disassembler_naive_decompile(machine_code, binary_size, &assembly_lines, segment, segment_count, options);
    assembly_code.is_label = calloc(assembly_lines, sizeof(int)); // zero init

    for (int i = 0; i < assembly_lines; i++) {
        //printf("%s", assembly_code.code[i]);
    }

    // add labels and make jumps labeled
    if (options & DO_ADD_JUMP_LABEL) {
        int jump_label_index = 0;
        for (int i = 0; i < assembly_lines; i++) {
            //printf("%.4x - %s", assembly_code.binary_index[i], assembly_code.code[i]);
            // check if its a jump instruction
            char** words = split(assembly_code.code[i], "\t, []", "");
            // Now for every instruction, check if its a jump through a dedicated table plus has an argument. Then compare strings
            int found_jump_instruction = 0;
            int is_relative_jump = 0;
            for (int instruction_index = 0; instruction_index < INSTRUCTION_COUNT; instruction_index++) {
                if (cpu_instruction_is_jump[instruction_index] == 1 && cpu_instruction_argument_count[instruction_index] == 1) {
                    if (strcmp(words[0], cpu_instruction_string[instruction_index]) == 0) {
                        //log_msg(LP_CRITICAL, "jump op: \"%s\"", cpu_instruction_string[instruction_index]);
                        found_jump_instruction = 1;
                        is_relative_jump = cpu_instruction_is_relative_jump[instruction_index];

                        // FOR SAFETY, AS ITS NOT FULLY IMPLEMETNED YET: 
                        if (cpu_instruction_is_relative_jump[instruction_index]) {found_jump_instruction = 0;}

                        break;
                    }
                }
            }
            if (!found_jump_instruction) {
                //log_msg(LP_INFO, "was not a jump instruction: %s", words[0]);
                int windex = 0;
                while (words[windex]) {
                    free(words[windex++]);
                }
                free(words);
                continue;
            }
            // Here when jump instruction was found

            if (words[1][0] == 'r') {
                if (words[1][1] == '0') {
                    continue;
                } else if (words[1][1] == '1') {
                    continue;
                } else if (words[1][1] == '2') {
                    continue;
                } else if (words[1][1] == '3') {
                    continue;
                }
            } else if (words[1][0] == 's' && words[1][1] == 'p') {
                continue;
            }

            int destination = parse_immediate(words[1]);
            //log_msg(LP_CRITICAL, "jump dest: %.4x", destination);

            // check if a preexisting label exists that points to the destination
            int found_match = 0;
            for (int j = assembly_lines - 1; j >= 0; j--) {
                if (!is_relative_jump) {
                    if (assembly_code.is_label[j] && assembly_code.binary_index[j] == destination) {
                        //log_msg(LP_INFO, "Found a matching label for jump to \"%s\"", assembly_code.code[j]);
                        sprintf(assembly_code.code[i], "%s %s", words[0], assembly_code.code[j]);
                        found_match = 1;
                        break;
                    }
                } else {
                    if (assembly_code.is_label[j] && assembly_code.binary_index[i] + (int16_t) destination == assembly_code.binary_index[j]) {
                        //log_msg(LP_INFO, "Found a matching label for relative jump to \"%s\"", assembly_code.code[j]);
                        sprintf(assembly_code.code[i], "%s %s", words[0], assembly_code.code[j]);
                        found_match = 1;
                        break;
                    }
                }
            }
            if (found_match) {
                int windex = 0;
                while (words[windex]) {
                    free(words[windex++]);
                }
                free(words);
                continue;
            }

            // look for instruction or data that starts at that destination. Choose last byte, so its after all control instrucitons like .code etc.
            for (int j = assembly_lines - 1; j >= 0; j--) {
                int condition = 0;
                if (is_relative_jump) {
                    condition = assembly_code.binary_index[i] + (int16_t) destination == assembly_code.binary_index[j];
                } else {
                    condition = assembly_code.binary_index[j] == destination;
                }
                if (condition) {
                    //log_msg(LP_CRITICAL, "Found target: [%.4x - %s]", assembly_code.binary_index[j], assembly_code.code[j]);
                    // first, increase capacity
                    for (long x = assembly_lines * 2 ; x < assembly_code.lines; x++) {
                        //free(assembly_code.code[x]);
                    }
                    assembly_code.lines = assembly_lines * 2;
                    assembly_code.code = realloc(assembly_code.code, assembly_lines * 2 * sizeof(char[256]));
                    assembly_code.binary_index = realloc(assembly_code.binary_index, assembly_lines * 2 * sizeof(int));
                    assembly_code.admr = realloc(assembly_code.admr, assembly_lines * 2 * sizeof(CPU_REDUCED_ADDRESSING_MODE_t));
                    assembly_code.admx = realloc(assembly_code.admx, assembly_lines * 2 * sizeof(CPU_EXTENDED_ADDRESSING_MODE_t));
                    assembly_code.is_label = realloc(assembly_code.is_label, assembly_lines * 2 * sizeof(int));
                    assembly_code.is_data = realloc(assembly_code.is_data, assembly_lines * 2 * sizeof(int));

                    for (int k = assembly_lines; k < assembly_lines * 2; k++) {
                        assembly_code.is_label[k] = 0;
                        //printf("BEFORE: %s", assembly_code.code[k]);
                    }
                    // first move every other instruction over by one
                    for (int k = assembly_lines; k > j; k--) {
                        sprintf(assembly_code.code[k], "%s", assembly_code.code[k - 1]);
                        assembly_code.binary_index[k] = assembly_code.binary_index[k - 1];
                        assembly_code.admx[k] = assembly_code.admx[k - 1];
                        assembly_code.admr[k] = assembly_code.admr[k - 1];
                    }

                    assembly_lines ++;
                    if (j < i) {
                        i += 1;
                    }

                    // Then inser the label
                    sprintf(assembly_code.code[j], ".jump_label%d\n", jump_label_index);
                    assembly_code.is_label[j] = 1;
                    
                    // and rewrite the instruciton
                    sprintf(assembly_code.code[i], "%s %s%d\n", words[0], ".jump_label", jump_label_index);

                    jump_label_index ++;

                    for (int k = 0; k < assembly_lines; k++) {
                        //printf("AFTER: %s", assembly_code.code[k]);
                    }

                    break;
                }
            }

            int windex = 0;
            while (words[windex]) {
                free(words[windex++]);
            }
            free(words);
        }
    }

    // add labels and make indirect immediate accesses labeled
    
    int dest_label_index = 0;
    int source_label_index = 0;
    for (int i = 0; i < assembly_lines; i++) {
        //printf("%d : %.4x [%d, %d] - %s", i, assembly_code.binary_index[i], assembly_code.admr[i], assembly_code.admx[i], assembly_code.code[i]);
        // check if its a jump instruction
        char** words = split(assembly_code.code[i], "\t, []", "");
        if (!words || !words[0]) {
            break;
        }
        // Now for every instruction, check if its a jump through a dedicated table plus has an argument. Then compare strings
        if (strcmp(words[0], cpu_instruction_string[MOV]) != 0) {
            //log_msg(LP_INFO, "was not mov instruction: %s", words[0]);
            int windex = 0;
            while (words[windex]) {
                free(words[windex++]);
            }
            free(words);
            continue;
        }

        int word_index = 0;
        while (words[word_index]) {
            //printf("word %d: %s\n", word_index, words[word_index]);
            word_index ++;
        }

        // Here when mov instruction was found
        // and we have the addressing mode
        int skip_admx = 0;
        int source = 0;
        char prefix_admx[256];
        char postfix_admx[256];
        switch (assembly_code.admx[i]) {
            case ADMX_IND16:
                if (cpu_reduced_addressing_mode_category[assembly_code.admr[i]] == ADMC_IND) {
                    sprintf(prefix_admx, "%s [%s], [", words[0], words[1]);
                } else {
                    sprintf(prefix_admx, "%s %s, [", words[0], words[1]);
                }
                sprintf(postfix_admx, "]");
                source = parse_immediate(words[2]);
                break;
            
            case ADMX_IND_R0_OFFSET16:
            case ADMX_IND_R1_OFFSET16:
            case ADMX_IND_R2_OFFSET16:
            case ADMX_IND_R3_OFFSET16:
            case ADMX_IND_SP_OFFSET16:
            case ADMX_IND_PC_OFFSET16:
                if (cpu_reduced_addressing_mode_category[assembly_code.admr[i]] == ADMC_IND) {
                    sprintf(prefix_admx, "%s [%s], [", words[0], words[1]);
                } else {
                    sprintf(prefix_admx, "%s %s, [", words[0], words[1]);
                }
                sprintf(postfix_admx, " + %s]", words[4]);
                source = parse_immediate(words[2]);
                break;
            
            case ADMX_IND16_SCALED8_R0_OFFSET:
            case ADMX_IND16_SCALED8_R1_OFFSET:
            case ADMX_IND16_SCALED8_R2_OFFSET:
            case ADMX_IND16_SCALED8_R3_OFFSET:
            case ADMX_IND16_SCALED8_SP_OFFSET:
            case ADMX_IND16_SCALED8_PC_OFFSET:
                if (cpu_reduced_addressing_mode_category[assembly_code.admr[i]] == ADMC_IND) {
                    sprintf(prefix_admx, "%s [%s], [", words[0], words[1]);
                } else {
                    sprintf(prefix_admx, "%s %s, [", words[0], words[1]);
                }
                sprintf(postfix_admx, " + %s * %s]", words[4], words[6]);
                source = parse_immediate(words[2]);
                break;

            default:
                skip_admx = 1;
                sprintf(postfix_admx, "%s", words[2]);
                //log_msg(LP_INFO, "Disassembler: Skipping admx");
                break;
        }
        if (!skip_admx && (options & DO_ADD_SOURCE_LABEL)) {

            //log_msg(LP_INFO, "mov src: %.4x", source);

            // check if a preexisting label exists that points to the destination
            int found_match = 0;
            for (int j = assembly_lines - 1; j >= 0; j--) {
                //printf("CHECKING OUT: %.4x %d %s", assembly_code.binary_index[j], assembly_code.is_label[j], assembly_code.code[j]);
                if (assembly_code.is_label[j] && assembly_code.binary_index[j] == source) {
                    //log_msg(LP_EMERGENCY, "Found a matching label \"%s\" sitting at %.4x", assembly_code.code[j], assembly_code.binary_index[j]);
                    char code[256];
                    strcpy(code, assembly_code.code[j]);
                    code[strlen(code) - 1] = '\0'; // turn \n into \0
                    sprintf(assembly_code.code[i], "%s%s%s\n", prefix_admx, code, postfix_admx);
                    found_match = 1;
                    break;
                }
            }
            if (found_match) {
                int windex = 0;
                while (words[windex]) {
                    free(words[windex++]);
                }
                free(words);
                continue;
            }

            // look for instruction or data that starts at that destination. Choose last byte, so its after all control instrucitons like .code etc.
            for (int j = assembly_lines - 1; j >= 0; j--) {
                if (assembly_code.binary_index[j] == source) {
                    //log_msg(LP_INFO, "Found target: [%.4x - %s]", assembly_code.binary_index[j], assembly_code.code[j]);
                    // Check if its within a data segment
                    if (!assembly_code.is_data[j] && !(options & DO_ADD_LABEL_TO_CODE_SEGMENT)) {
                        break;
                    }
                    //if (assembly_code.code[j])
                    // insert label and replace mov source immediate with label
                    // first, increase capacity
                    for (long x = assembly_lines * 2 ; x < assembly_code.lines; x++) {
                        //free(assembly_code.code[x]);
                    }
                    assembly_code.code = realloc(assembly_code.code, assembly_lines * 2 * sizeof(char[256]));
                    assembly_code.binary_index = realloc(assembly_code.binary_index, assembly_lines * 2 * sizeof(int));
                    assembly_code.admr = realloc(assembly_code.admr, assembly_lines * 2 * sizeof(CPU_REDUCED_ADDRESSING_MODE_t));
                    assembly_code.admx = realloc(assembly_code.admx, assembly_lines * 2 * sizeof(CPU_EXTENDED_ADDRESSING_MODE_t));
                    assembly_code.is_label = realloc(assembly_code.is_label, assembly_lines * 2 * sizeof(int));
                    assembly_code.is_data = realloc(assembly_code.is_data, assembly_lines * 2 * sizeof(int));
                    
                    for (int k = 0; k < assembly_lines; k++) {
                        //printf("BEFORE: %s", assembly_code.code[k]);
                        //if (k == i) {printf(".^^^.\n");}
                    }

                    // first move every other instruction over by one
                    for (int k = assembly_lines; k > j; k--) {
                        sprintf(assembly_code.code[k], "%s", assembly_code.code[k - 1]);
                        assembly_code.binary_index[k] = assembly_code.binary_index[k - 1];
                        assembly_code.admx[k] = assembly_code.admx[k - 1];
                        assembly_code.admr[k] = assembly_code.admr[k - 1];
                    }

                    for (int k = 0; k < assembly_lines; k++) {
                        //printf("%s", assembly_code.code[k]);
                    }

                    assembly_lines ++;
                    if (j <= i) {
                        //log_msg(LP_INFO, "Label was added to PRIOR code, thus moving ahead by one");
                        i += 1;
                    } else {
                        //log_msg(LP_INFO, "Label was added to code AHEAD");
                    }

                    // Then inser the label
                    sprintf(assembly_code.code[j], ".source_label%d\n", source_label_index);
                    assembly_code.is_label[j] = 1;

                    for (int k = 0; k < assembly_lines; k++) {
                        //printf("INTERMEDIATE: %s", assembly_code.code[k]);
                        //if (k == i) {printf(".^^^.\n");}
                    }
                    
                    // and rewrite the instruciton
                    sprintf(assembly_code.code[i], "%s%s%d%s\n", prefix_admx, ".source_label", source_label_index, postfix_admx);

                    source_label_index ++;

                    for (int k = 0; k < assembly_lines; k++) {
                        //printf("AFTER: %s", assembly_code.code[k]);
                        //if (k == i) {printf(".^^^.\n");}
                    }

                    break;
                }
            }
        }
        
        
        int skip_admr = 0;
        source = 0;
        char prefix_admr[256];
        char postfix_admr[256];
        free(words); // TODO: free each element too

        if (assembly_code.admr[i] != ADMR_IND16) {
            if (!skip_admx) {
                //i += 2;
            }
            //log_msg(LP_INFO, "Skipping admr");
            continue;
        }

        words = split(assembly_code.code[i], "\t, []\n", "");
        word_index = 0;
        while (words[word_index]) {
            //printf("%d: %s\n", word_index, words[word_index]);
            word_index ++;
        }

        switch (assembly_code.admx[i]) {

            case ADMX_IMM16:
                sprintf(prefix_admr, "%s [", words[0]);
                sprintf(postfix_admr, "], %s", words[2]);
                source = parse_immediate(words[1]);
                break;

            case ADMX_R0:
            case ADMX_R1:
            case ADMX_R2:
            case ADMX_R3:
            case ADMX_SP:
            case ADMX_PC:
                sprintf(prefix_admr, "%s [", words[0]);
                sprintf(postfix_admr, "], %s", words[2]);
                source = parse_immediate(words[1]);
                break;

            case ADMX_IND16:
                sprintf(prefix_admr, "%s [", words[0]);
                sprintf(postfix_admr, "], [%s]", words[2]);
                source = parse_immediate(words[1]);
                break;

            case ADMX_IND_R0:
            case ADMX_IND_R1:
            case ADMX_IND_R2:
            case ADMX_IND_R3:
            case ADMX_IND_SP:
            case ADMX_IND_PC:
                sprintf(prefix_admr, "%s [", words[0]);
                sprintf(postfix_admr, "], [%s]", words[2]);
                source = parse_immediate(words[1]);
                break;
            
            case ADMX_IND_R0_OFFSET16:
            case ADMX_IND_R1_OFFSET16:
            case ADMX_IND_R2_OFFSET16:
            case ADMX_IND_R3_OFFSET16:
            case ADMX_IND_SP_OFFSET16:
            case ADMX_IND_PC_OFFSET16:
            sprintf(prefix_admr, "%s [", words[0]);
            sprintf(postfix_admr, "], [%s + %s]", words[2], words[4]);
                source = parse_immediate(words[1]);
                break;
            
            case ADMX_IND16_SCALED8_R0_OFFSET:
            case ADMX_IND16_SCALED8_R1_OFFSET:
            case ADMX_IND16_SCALED8_R2_OFFSET:
            case ADMX_IND16_SCALED8_R3_OFFSET:
            case ADMX_IND16_SCALED8_SP_OFFSET:
            case ADMX_IND16_SCALED8_PC_OFFSET:
                sprintf(prefix_admr, "%s [", words[0]);
                sprintf(postfix_admr, "], [%s + %s * %s]", words[2], words[4], words[6]);
                source = parse_immediate(words[1]);
                break;

            default:
                skip_admr = 1;
                sprintf(postfix_admr, "%s", words[1]);
                source = parse_immediate(words[1]);
                log_msg(LP_INFO, "Disassembler: Skipping admr");
                break;
        }

        if (!skip_admr && (options & DO_ADD_DEST_LABEL)) {
            source = parse_immediate(words[1]);

            //log_msg(LP_CRITICAL, "mov dest: %.4x", source);

            // look for instruction or data that starts at that destination. Choose last byte, so its after all control instrucitons like .code etc.
            for (int j = assembly_lines - 1; j >= 0; j--) {
                if (assembly_code.binary_index[j] == source) {
                    //log_msg(LP_INFO, "Found target: [%.4x - %s]", assembly_code.binary_index[j], assembly_code.code[j]);
                    // Check if its within a data segment
                    if (!assembly_code.is_data[j] && !(options & DO_ADD_LABEL_TO_CODE_SEGMENT)) {
                        break;
                    }
                    //if (assembly_code.code[j])
                    // insert label and replace mov source immediate with label
                    // first, increase capacity
                    for (long x = assembly_lines * 2 ; x < assembly_code.lines; x++) {
                        //free(assembly_code.code[x]);
                    }
                    assembly_code.code = realloc(assembly_code.code, assembly_lines * 2 * sizeof(char[256]));
                    assembly_code.binary_index = realloc(assembly_code.binary_index, assembly_lines * 2 * sizeof(int));
                    assembly_code.admr = realloc(assembly_code.admr, assembly_lines * 2 * sizeof(CPU_REDUCED_ADDRESSING_MODE_t));
                    assembly_code.admx = realloc(assembly_code.admx, assembly_lines * 2 * sizeof(CPU_EXTENDED_ADDRESSING_MODE_t));
                    assembly_code.is_label = realloc(assembly_code.is_label, assembly_lines * 2 * sizeof(int));
                    assembly_code.is_data = realloc(assembly_code.is_data, assembly_lines * 2 * sizeof(int));
                    
                    for (int k = 0; k < assembly_lines; k++) {
                        //printf("BEFORE: %s", assembly_code.code[k]);
                        //if (k == i) {printf(".^^^.\n");}
                    }

                    // first move every other instruction over by one
                    for (int k = assembly_lines; k > j; k--) {
                        sprintf(assembly_code.code[k], "%s", assembly_code.code[k - 1]);
                        assembly_code.binary_index[k] = assembly_code.binary_index[k - 1];
                        assembly_code.admx[k] = assembly_code.admx[k - 1];
                        assembly_code.admr[k] = assembly_code.admr[k - 1];
                    }

                    for (int k = 0; k < assembly_lines; k++) {
                        //printf("%s", assembly_code.code[k]);
                    }

                    assembly_lines ++;
                    if (j <= i) {
                        //log_msg(LP_INFO, "Label was added to PRIOR code, thus moving ahead by one");
                        i += 1;
                    } else {
                        //log_msg(LP_INFO, "Label was added to code AHEAD");
                    }

                    // Then inser the label
                    sprintf(assembly_code.code[j], ".dest_label%d\n", dest_label_index);
                    assembly_code.is_label[j] = 1;

                    for (int k = 0; k < assembly_lines; k++) {
                        //printf("INTERMEDIATE: %s", assembly_code.code[k]);
                        //if (k == i) {printf(".^^^.\n");}
                    }
                    
                    // and rewrite the instruciton
                    sprintf(assembly_code.code[i], "%s%s%d%s\n", prefix_admr, ".dest_label", dest_label_index, postfix_admr);

                    dest_label_index ++;

                    for (int k = 0; k < assembly_lines; k++) {
                        //printf("AFTER: %s", assembly_code.code[k]);
                        //if (k == i) {printf(".^^^.\n");}
                    }

                    break;
                }
            }
        }
        
        
        int windex = 0;
        while (words[windex]) {
            free(words[windex++]);
        }
        free(words);
    }
    
    for (int i = 0; i < assembly_lines; i++) {
        //printf("%.4x : %s ,,,, %d\n", assembly_code.binary_index[i], assembly_code.code[i], assembly_code.is_data[i]);
    }


    // reduce overall size
    unsigned long total_size = 0;
    for (int i = 0; i < assembly_lines; i++) {
        total_size += strlen(assembly_code.code[i]);
    }

    char* assembly_code_reduced = calloc(sizeof(char), total_size + 1);
    int offset = 0;
    for (int i = 0; i < assembly_lines; i++) {
        strcpy(assembly_code_reduced + offset, assembly_code.code[i]);
        offset += strlen(assembly_code.code[i]);
    }

    for (long i = 0; i < assembly_lines; i++) {
        //free(assembly_code.code[i]);
    }

    free(assembly_code.code);
    free(assembly_code.admr);
    free(assembly_code.admx);
    //free(assembly_code.binary_index);
    free(assembly_code.is_label);

    return assembly_code_reduced;
}

void disassembler_decompile_to_file(uint8_t* machine_code, const char filename[], long binary_size, uint16_t* segment, int segment_count, DisassembleOption_t options) {

    char* assembly = disassembler_decompile(machine_code, binary_size, segment, segment_count, options);
    data_export(filename, assembly, strlen(assembly));
    free(assembly);

}

