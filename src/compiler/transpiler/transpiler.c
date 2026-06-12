#include <stdio.h>
#include <string.h>

#include "utils/Log.h"
#include "utils/IO.h"
#include "utils/String.h"

#include "compiler/asm/disassembler.h"
#include "compiler/asm/assembler.h"

#include "cpu/cpu_instructions.h"
#include "cpu/cpu_addressing_modes.h"


void generate_asm_reconstruction(Instruction_t instruction, char output[256]) {
    for (int e = 0; e < instruction.expression_count; e++) {
        for (int t = 0; t < instruction.expression[e].token_count; t++) {
            int len = strlen(output);
            sprintf(output + len, "%s ", instruction.expression[e].tokens[t].raw);
        }
    }
}

int generate_code_for_single_instruction(Instruction_t instruction, char output[512], char* prefix) {
    switch (instruction.instruction) {
        case 0: {
            if (instruction.expression[0].type == EXPR_ADDRESS || 
                instruction.expression[0].type == EXPR_DATA || 
                instruction.expression[0].type == EXPR_DEREF_LABEL || 
                instruction.expression[0].type == EXPR_PAD || 
                instruction.expression[0].type == EXPR_RESERVE || 
                instruction.expression[0].type == EXPR_STORE_ADDRESS || 
                instruction.expression[0].type == EXPR_RESTORE_ADDRESS || 
                instruction.expression[0].type == EXPR_SEGMENT_CODE || 
                instruction.expression[0].type == EXPR_SEGMENT_DATA
            ) {
                sprintf(output, "%s// No operation", prefix);
                break;
            }
            if (instruction.expression[0].type == EXPR_IMMEDIATE) {
                // Here if it was a simple label
                sprintf(output, "_%s:", instruction.expression[0].tokens[0].raw + 1);
            }
            break;
        }

        case CALL: {
            sprintf(output, "%spush16(0xFFFF); // stub push of pc\n%s_%s();", prefix, prefix, instruction.expression[1].tokens[0].raw + 1);
            break;
        }

        case RET: {
            sprintf(output, "%s(void) pop16();\n%sreturn;", prefix, prefix);
            break;
        }

        case PUSH: {
            switch (instruction.admx) {
                case ADMX_IMM16:
                    sprintf(output, "%spush16(0x%.4X);", prefix, parse_immediate(instruction.expression[1].tokens[0].raw));
                    break;

                case ADMX_R0:
                    sprintf(output, "%spush16(r0);", prefix);
                    break;

                case ADMX_R1:
                    sprintf(output, "%spush16(r1);", prefix);
                    break;

                case ADMX_R2:
                    sprintf(output, "%spush16(r2);", prefix);
                    break;

                case ADMX_R3:
                    sprintf(output, "%spush16(r3);", prefix);
                    break;

                case ADMX_SP:
                    sprintf(output, "%spush16(sp);", prefix);
                    break;

                case ADMX_IND16:
                    sprintf(output, "%spush16(read16(0x%.4X));", prefix, parse_immediate(instruction.expression[1].tokens[1].raw));
                    break;

                case ADMX_IND_R3_OFFSET16:
                    sprintf(output, "%spush16(read16(0x%.4X + r3));", prefix, parse_immediate(instruction.expression[1].tokens[1].raw));
                    break;

                default:
                    log_msg(LP_ERROR, "Transpiler: \"%s\" with admx \"%s\" not handled [%s:%d]", instruction_encoding[instruction.instruction].instruction_string, cpu_extended_addressing_mode_string[instruction.admx], __FILE__, __LINE__);
                    return 1;
                    break;
            }
            break;
        }

        case POP: {
            switch (instruction.admr) {
                case ADMR_R0:
                    sprintf(output, "%sr0 = pop16();", prefix);
                    break;
                case ADMR_R1:
                    sprintf(output, "%sr1 = pop16();", prefix);
                    break;
                case ADMR_R2:
                    sprintf(output, "%sr2 = pop16();", prefix);
                    break;
                case ADMR_R3:
                    sprintf(output, "%sr3 = pop16();", prefix);
                    break;
                case ADMR_SP:
                    sprintf(output, "%ssp = pop16();", prefix);
                    break;

                default:
                    log_msg(LP_ERROR, "Transpiler: \"%s\" with admr \"%s\" not handled [%s:%d]", instruction_encoding[instruction.instruction].instruction_string, cpu_reduced_addressing_mode_string[instruction.admr], __FILE__, __LINE__);
                    return 1;
                    break;
            }
            break;
        }
    
        case MOV: {
            switch (instruction.admr) {
                case ADMR_R0:
                    sprintf(output, "%sr0 = ", prefix);
                    break;

                case ADMR_R1:
                    sprintf(output, "%sr1 = ", prefix);
                    break;

                case ADMR_R2:
                    sprintf(output, "%sr2 = ", prefix);
                    break;

                case ADMR_R3:
                    sprintf(output, "%sr3 = ", prefix);
                    break;

                case ADMR_SP:
                    sprintf(output, "%ssp = ", prefix);
                    break;

                case ADMR_IND_R0:
                    sprintf(output, "%swrite16(r0, ", prefix);
                    break;

                case ADMR_IND16:
                    sprintf(output, "%swrite16(0x%.4X, ", prefix, parse_immediate(instruction.expression[1].tokens[1].raw));
                    break;
                
                default:
                    log_msg(LP_ERROR, "Transpiler: \"%s\" with admr \"%s\" not handled [%s:%d]", 
                        instruction_encoding[instruction.instruction].instruction_string, 
                        cpu_reduced_addressing_mode_string[instruction.admr], __FILE__, __LINE__
                    );
                    return 1;
                    break;
            }
            if (cpu_reduced_addressing_mode_category[instruction.admr] == ADMC_REG) {
                switch (instruction.admx) {
                    case ADMX_IMM16:
                        sprintf(output + strlen(output), "0x%.4X;", parse_immediate(instruction.expression[2].tokens[0].raw));
                        break;

                    case ADMX_R0:
                        sprintf(output + strlen(output), "r0;");
                        break;
    
                    case ADMX_R1:
                        sprintf(output + strlen(output), "r1;");
                        break;
    
                    case ADMX_R2:
                        sprintf(output + strlen(output), "r2;");
                        break;
    
                    case ADMX_R3:
                        sprintf(output + strlen(output), "r3;");
                        break;

                    case ADMX_SP:
                        sprintf(output + strlen(output), "sp;");
                        break;

                    case ADMX_IND16:
                        sprintf(output + strlen(output), "read16(0x%.4X);", parse_immediate(instruction.expression[2].tokens[1].raw));
                        break;
                
                    case ADMX_IND_R0:
                        sprintf(output + strlen(output), "read16(r0);");
                        break;
                
                    case ADMX_IND_R1:
                        sprintf(output + strlen(output), "read16(r1);");
                        break;
                
                    case ADMX_IND_R2:
                        sprintf(output + strlen(output), "read16(r2);");
                        break;
                
                    case ADMX_IND_R3:
                        sprintf(output + strlen(output), "read16(r3);");
                        break;
                
                    case ADMX_IND_SP:
                        sprintf(output + strlen(output), "read16(sp);");
                        break;
                
                    case ADMX_IND_R3_OFFSET16:
                        sprintf(output + strlen(output), "read16(0x%.4X + r3);", parse_immediate(instruction.expression[2].tokens[1].raw));
                        break;
                    
                    default:
                        log_msg(LP_ERROR, "Transpiler: \"%s\" with admx \"%s\" not handled [%s:%d]", 
                            instruction_encoding[instruction.instruction].instruction_string, 
                            cpu_extended_addressing_mode_string[instruction.admx], __FILE__, __LINE__
                        );
                        return 1;
                        break;
                }
            } else if (cpu_reduced_addressing_mode_category[instruction.admr] == ADMC_IND) {
                switch (instruction.admx) {
                    case ADMX_IMM16:
                        sprintf(output + strlen(output), "0x%.4X);", parse_immediate(instruction.expression[2].tokens[0].raw));
                        break;

                    case ADMX_R0:
                        sprintf(output + strlen(output), "r0);");
                        break;
    
                    case ADMX_R1:
                        sprintf(output + strlen(output), "r1);");
                        break;
    
                    case ADMX_R2:
                        sprintf(output + strlen(output), "r2);");
                        break;
    
                    case ADMX_R3:
                        sprintf(output + strlen(output), "r3);");
                        break;

                    case ADMX_SP:
                        sprintf(output + strlen(output), "sp);");
                        break;

                    case ADMX_IND16:
                        sprintf(output + strlen(output), "read16(0x%.4X));", parse_immediate(instruction.expression[2].tokens[1].raw));
                        break;
                
                    case ADMX_IND_R0:
                        sprintf(output + strlen(output), "read16(r0));");
                        break;
                
                    case ADMX_IND_R1:
                        sprintf(output + strlen(output), "read16(r1));");
                        break;
                
                    case ADMX_IND_R2:
                        sprintf(output + strlen(output), "read16(r2));");
                        break;
                
                    case ADMX_IND_R3:
                        sprintf(output + strlen(output), "read16(r3));");
                        break;
                
                    case ADMX_IND_SP:
                        sprintf(output + strlen(output), "read16(sp));");
                        break;
                
                    case ADMX_IND_R0_OFFSET16:
                        sprintf(output + strlen(output), "read16(0x%.4X + r0));", parse_immediate(instruction.expression[2].tokens[1].raw));
                        break;
                
                    case ADMX_IND_R1_OFFSET16:
                        sprintf(output + strlen(output), "read16(0x%.4X + r1));", parse_immediate(instruction.expression[2].tokens[1].raw));
                        break;
                
                    case ADMX_IND_R2_OFFSET16:
                        sprintf(output + strlen(output), "read16(0x%.4X + r2));", parse_immediate(instruction.expression[2].tokens[1].raw));
                        break;
                
                    case ADMX_IND_R3_OFFSET16:
                        sprintf(output + strlen(output), "read16(0x%.4X + r3));", parse_immediate(instruction.expression[2].tokens[1].raw));
                        break;
                    
                    default:
                        log_msg(LP_ERROR, "Transpiler: \"%s\" with admx \"%s\" not handled [%s:%d]", 
                            instruction_encoding[instruction.instruction].instruction_string, 
                            cpu_extended_addressing_mode_string[instruction.admx], __FILE__, __LINE__
                        );
                        return 1;
                        break;
                }
            }
            break;
        }
    
        case CMOVZ: 
        case CMOVFZ: 
        case CMOVL: 
        case CMOVUL: 
        case CMOVFL: 
        case CMOVDL: 
        case CMOVLL: 
        case CMOVAO: 
        case CMOVMI: 
        case CMOVNZ: 
        case CMOVNFZ: 
        case CMOVNL: 
        case CMOVNUL: 
        case CMOVNFL: 
        case CMOVNDL: 
        case CMOVNLL: 
        case CMOVNAO: 
        case CMOVNMI:  {
            char flag[8] = {0};
            char not[8] = {0};
            switch (instruction.instruction) {
                case CMOVNZ:
                    sprintf(flag, "sr.Z");
                    break;
                
                case CMOVNFZ:
                    sprintf(flag, "sr.FZ");
                    break;

                case CMOVNL:
                    sprintf(flag, "sr.L");
                    break;

                case CMOVNUL:
                    sprintf(flag, "sr.UL");
                    break;

                case CMOVNFL:
                    sprintf(flag, "sr.FL");
                    break;

                case CMOVNDL:
                    sprintf(flag, "sr.DL");
                    break;

                case CMOVNLL:
                    sprintf(flag, "sr.LL");
                    break;

                case CMOVNAO:
                    sprintf(flag, "sr.AO");
                    break;

                case CMOVNMI:
                    sprintf(flag, "sr.MI");
                    break;

            
                case CMOVZ:
                    sprintf(flag, "sr.Z");
                    sprintf(not, "!");
                    break;
                
                case CMOVFZ:
                    sprintf(flag, "sr.FZ");
                    sprintf(not, "!");
                    break;

                case CMOVL:
                    sprintf(flag, "sr.L");
                    sprintf(not, "!");
                    break;

                case CMOVUL:
                    sprintf(flag, "sr.UL");
                    sprintf(not, "!");
                    break;

                case CMOVFL:
                    sprintf(flag, "sr.FL");
                    sprintf(not, "!");
                    break;

                case CMOVDL:
                    sprintf(flag, "sr.DL");
                    sprintf(not, "!");
                    break;

                case CMOVLL:
                    sprintf(flag, "sr.LL");
                    sprintf(not, "!");
                    break;

                case CMOVAO:
                    sprintf(flag, "sr.AO");
                    sprintf(not, "!");
                    break;

                case CMOVMI:
                    sprintf(flag, "sr.MI");
                    sprintf(not, "!");
                    break;
                
                default:
                    log_msg(LP_ERROR, "Transpiler: \"%s\" not handled [%s:%d]", 
                        instruction_encoding[instruction.instruction].instruction_string, 
                        cpu_reduced_addressing_mode_string[instruction.admr], __FILE__, __LINE__
                    );
                    return 1;
            }
            switch (instruction.admr) {
                case ADMR_R0:
                    sprintf(output, "%sr0 = %s%s ? r0 : ", prefix, not, flag);
                    break;

                case ADMR_R1:
                    sprintf(output, "%sr1 = %s%s ? r1 : ", prefix, not, flag);
                    break;

                case ADMR_R2:
                    sprintf(output, "%sr2 = %s%s ? r2 : ", prefix, not, flag);
                    break;

                case ADMR_R3:
                    sprintf(output, "%sr3 = %s%s ? r3 : ", prefix, not, flag);
                    break;

                case ADMR_SP:
                    sprintf(output, "%ssp = %s%s ? sp : ", prefix, not, flag);
                    break;

                case ADMR_IND_R0:
                    sprintf(output, "%swrite16(r0, %s%s ? read16(r0) : ", prefix, not, flag);
                    break;

                case ADMR_IND16:
                    sprintf(output, "%swrite16(0x%.4X, %s%s ? read16(0x%.4X) : ", prefix, 
                        parse_immediate(instruction.expression[1].tokens[1].raw), 
                        not, flag, 
                        parse_immediate(instruction.expression[1].tokens[1].raw)
                    );
                    break;
                
                default:
                    log_msg(LP_ERROR, "Transpiler: \"%s\" with admr \"%s\" not handled [%s:%d]", 
                        instruction_encoding[instruction.instruction].instruction_string, 
                        cpu_reduced_addressing_mode_string[instruction.admr], __FILE__, __LINE__
                    );
                    return 1;
                    break;
            }
            if (cpu_reduced_addressing_mode_category[instruction.admr] == ADMC_REG) {
                switch (instruction.admx) {
                    case ADMX_IMM16:
                        sprintf(output + strlen(output), "0x%.4X;", parse_immediate(instruction.expression[2].tokens[0].raw));
                        break;

                    case ADMX_R0:
                        sprintf(output + strlen(output), "r0;");
                        break;
    
                    case ADMX_R1:
                        sprintf(output + strlen(output), "r1;");
                        break;
    
                    case ADMX_R2:
                        sprintf(output + strlen(output), "r2;");
                        break;
    
                    case ADMX_R3:
                        sprintf(output + strlen(output), "r3;");
                        break;

                    case ADMX_SP:
                        sprintf(output + strlen(output), "sp;");
                        break;

                    case ADMX_IND16:
                        sprintf(output + strlen(output), "read16(0x%.4X);", parse_immediate(instruction.expression[2].tokens[1].raw));
                        break;
                
                    case ADMX_IND_R0:
                        sprintf(output + strlen(output), "read16(r0);");
                        break;
                
                    case ADMX_IND_R1:
                        sprintf(output + strlen(output), "read16(r1);");
                        break;
                
                    case ADMX_IND_R2:
                        sprintf(output + strlen(output), "read16(r2);");
                        break;
                
                    case ADMX_IND_R3:
                        sprintf(output + strlen(output), "read16(r3);");
                        break;
                
                    case ADMX_IND_SP:
                        sprintf(output + strlen(output), "read16(sp);");
                        break;
                
                    case ADMX_IND_R3_OFFSET16:
                        sprintf(output + strlen(output), "read16(0x%.4X + r3);", parse_immediate(instruction.expression[2].tokens[1].raw));
                        break;
                    
                    default:
                        log_msg(LP_ERROR, "Transpiler: \"%s\" with admx \"%s\" not handled [%s:%d]", 
                            instruction_encoding[instruction.instruction].instruction_string, 
                            cpu_extended_addressing_mode_string[instruction.admx], __FILE__, __LINE__
                        );
                        return 1;
                        break;
                }
            } else if (cpu_reduced_addressing_mode_category[instruction.admr] == ADMC_IND) {
                switch (instruction.admx) {
                    case ADMX_IMM16:
                        sprintf(output + strlen(output), "0x%.4X);", parse_immediate(instruction.expression[2].tokens[0].raw));
                        break;

                    case ADMX_R0:
                        sprintf(output + strlen(output), "r0);");
                        break;
    
                    case ADMX_R1:
                        sprintf(output + strlen(output), "r1);");
                        break;
    
                    case ADMX_R2:
                        sprintf(output + strlen(output), "r2);");
                        break;
    
                    case ADMX_R3:
                        sprintf(output + strlen(output), "r3);");
                        break;

                    case ADMX_SP:
                        sprintf(output + strlen(output), "sp);");
                        break;
                
                    case ADMX_IND_R0:
                        sprintf(output + strlen(output), "read16(r0));");
                        break;
                
                    case ADMX_IND_R1:
                        sprintf(output + strlen(output), "read16(r1));");
                        break;
                
                    case ADMX_IND_R2:
                        sprintf(output + strlen(output), "read16(r2));");
                        break;
                
                    case ADMX_IND_R3:
                        sprintf(output + strlen(output), "read16(r3));");
                        break;
                
                    case ADMX_IND_SP:
                        sprintf(output + strlen(output), "read16(sp));");
                        break;
                
                    case ADMX_IND_R0_OFFSET16:
                        sprintf(output + strlen(output), "read16(0x%.4X + r0));", parse_immediate(instruction.expression[2].tokens[1].raw));
                        break;
                
                    case ADMX_IND_R1_OFFSET16:
                        sprintf(output + strlen(output), "read16(0x%.4X + r1));", parse_immediate(instruction.expression[2].tokens[1].raw));
                        break;
                
                    case ADMX_IND_R2_OFFSET16:
                        sprintf(output + strlen(output), "read16(0x%.4X + r2));", parse_immediate(instruction.expression[2].tokens[1].raw));
                        break;
                
                    case ADMX_IND_R3_OFFSET16:
                        sprintf(output + strlen(output), "read16(0x%.4X + r3));", parse_immediate(instruction.expression[2].tokens[1].raw));
                        break;
                    
                    default:
                        log_msg(LP_ERROR, "Transpiler: \"%s\" with admx \"%s\" not handled [%s:%d]", 
                            instruction_encoding[instruction.instruction].instruction_string, 
                            cpu_extended_addressing_mode_string[instruction.admx], __FILE__, __LINE__
                        );
                        return 1;
                        break;
                }
            }
            break;
        }
    
        case LEA: {
            switch (instruction.admr) {
                case ADMR_R0:
                    sprintf(output, "%sr0 = ", prefix);
                    break;

                case ADMR_R1:
                    sprintf(output, "%sr1 = ", prefix);
                    break;

                case ADMR_R2:
                    sprintf(output, "%sr2 = ", prefix);
                    break;

                case ADMR_R3:
                    sprintf(output, "%sr3 = ", prefix);
                    break;

                case ADMR_IND_R0:
                    sprintf(output, "%swrite16(r0, ", prefix);
                    break;
                
                default:
                    log_msg(LP_ERROR, "Transpiler: \"%s\" with admr \"%s\" not handled [%s:%d]", 
                        instruction_encoding[instruction.instruction].instruction_string, 
                        cpu_reduced_addressing_mode_string[instruction.admr], __FILE__, __LINE__
                    );
                    return 1;
                    break;
            }
            if (cpu_reduced_addressing_mode_category[instruction.admr] == ADMC_REG) {
                switch (instruction.admx) {
                    case ADMX_IND16:
                        sprintf(output + strlen(output), "0x%.4X;", parse_immediate(instruction.expression[2].tokens[1].raw));
                        break;

                    case ADMX_IND_SP:
                        sprintf(output + strlen(output), "read16(sp);");
                        break;
                    
                    case ADMX_IND_R3_OFFSET16:
                        sprintf(output + strlen(output), "0x%.4X + r3;", parse_immediate(instruction.expression[2].tokens[1].raw));
                        break;
                    
                    default:
                        log_msg(LP_ERROR, "Transpiler: \"%s\" with admx \"%s\" not handled [%s:%d]", 
                            instruction_encoding[instruction.instruction].instruction_string, 
                            cpu_extended_addressing_mode_string[instruction.admx], __FILE__, __LINE__
                        );
                        return 1;
                        break;
                }
            }
            if (cpu_reduced_addressing_mode_category[instruction.admr] == ADMC_IND) {
                switch (instruction.admx) {
                    case ADMX_IND16:
                        sprintf(output + strlen(output), "0x%.4X);", parse_immediate(instruction.expression[2].tokens[1].raw));
                        break;
                    
                    case ADMX_IND_R3_OFFSET16:
                        sprintf(output + strlen(output), "0x%.4X + r3);", parse_immediate(instruction.expression[2].tokens[1].raw));
                        break;
                    
                    default:
                        log_msg(LP_ERROR, "Transpiler: \"%s\" with admx \"%s\" not handled [%s:%d]", 
                            instruction_encoding[instruction.instruction].instruction_string, 
                            cpu_extended_addressing_mode_string[instruction.admx], __FILE__, __LINE__
                        );
                        return 1;
                        break;
                }
            }
            break;
        }
    
        case ADD: {
            switch (instruction.admr) {
                case ADMR_R0:
                    sprintf(output, "%soverflow_check = r0;\n", prefix);
                    sprintf(output + strlen(output), "%sr0 += ", prefix);
                    break;

                case ADMR_R1:
                    sprintf(output, "%soverflow_check = r1;\n", prefix);
                    sprintf(output + strlen(output), "%sr1 += ", prefix);
                    break;
                    
                case ADMR_R2:
                    sprintf(output, "%soverflow_check = r2;\n", prefix);
                    sprintf(output + strlen(output), "%sr2 += ", prefix);
                    break;

                case ADMR_R3:
                    sprintf(output, "%soverflow_check = r3;\n", prefix);
                    sprintf(output + strlen(output), "%sr3 += ", prefix);
                    break;

                case ADMR_SP:
                    sprintf(output, "%soverflow_check = sp;\n", prefix);
                    sprintf(output + strlen(output), "%ssp += ", prefix);
                    break;
                
                default:
                    log_msg(LP_ERROR, "Transpiler: \"%s\" with admr \"%s\" not handled [%s:%d]", 
                        instruction_encoding[instruction.instruction].instruction_string, 
                        cpu_reduced_addressing_mode_string[instruction.admr], __FILE__, __LINE__
                    );
                    return 1;
                    break;
            }
            switch (instruction.admx) {
                case ADMX_IMM16:
                    sprintf(output + strlen(output), "0x%.4X;\n", parse_immediate(instruction.expression[2].tokens[0].raw));
                    sprintf(output + strlen(output), "%ssr.AO = (overflow_check + 0x%.4X) > 0xFFFF;", prefix, parse_immediate(instruction.expression[2].tokens[0].raw));
                    break;

                case ADMX_R0:
                    sprintf(output + strlen(output), "r0;");
                    sprintf(output + strlen(output), "%ssr.AO = (overflow_check + r0) > 0xFFFF;", prefix);
                    break;

                case ADMX_R1:
                    sprintf(output + strlen(output), "r1;");
                    sprintf(output + strlen(output), "%ssr.AO = (overflow_check + r1) > 0xFFFF;", prefix);
                    break;

                case ADMX_R2:
                    sprintf(output + strlen(output), "r2;");
                    sprintf(output + strlen(output), "%ssr.AO = (overflow_check + r2) > 0xFFFF;", prefix);
                    break;

                case ADMX_R3:
                    sprintf(output + strlen(output), "r3;");
                    sprintf(output + strlen(output), "%ssr.AO = (overflow_check + r3) > 0xFFFF;", prefix);
                    break;

                case ADMX_SP:
                    sprintf(output + strlen(output), "sp;");
                    sprintf(output + strlen(output), "%ssr.AO = (overflow_check + sp) > 0xFFFF;", prefix);
                    break;

                case ADMX_IND_R3_OFFSET16:
                    sprintf(output + strlen(output), "read16(0x%.4X + r3);", parse_immediate(instruction.expression[2].tokens[1].raw));
                    sprintf(output + strlen(output), "%ssr.AO = (overflow_check + read16(0x%.4X + r3)) > 0xFFFF;", prefix, parse_immediate(instruction.expression[2].tokens[1].raw));
                    break;
                
                default:
                    log_msg(LP_ERROR, "Transpiler: \"%s\" with admx \"%s\" not handled [%s:%d]", 
                        instruction_encoding[instruction.instruction].instruction_string, 
                        cpu_extended_addressing_mode_string[instruction.admx], __FILE__, __LINE__
                    );
                    return 1;
                    break;
            }
            break;
        }
    
        case SUB: {
            switch (instruction.admr) {
                case ADMR_R0:
                    sprintf(output, "%soverflow_check = r0;\n", prefix);
                    sprintf(output + strlen(output), "%sr0 -= ", prefix);
                    break;

                case ADMR_R1:
                    sprintf(output, "%soverflow_check = r1;\n", prefix);
                    sprintf(output + strlen(output), "%sr1 -= ", prefix);
                    break;
                    
                case ADMR_R2:
                    sprintf(output, "%soverflow_check = r2;\n", prefix);
                    sprintf(output + strlen(output), "%sr2 -= ", prefix);
                    break;

                case ADMR_R3:
                    sprintf(output, "%soverflow_check = r3;\n", prefix);
                    sprintf(output + strlen(output), "%sr3 -= ", prefix);
                    break;

                case ADMR_SP:
                    sprintf(output, "%soverflow_check = sp;\n", prefix);
                    sprintf(output + strlen(output), "%ssp -= ", prefix);
                    break;
                
                default:
                    log_msg(LP_ERROR, "Transpiler: \"%s\" with admr \"%s\" not handled [%s:%d]", 
                        instruction_encoding[instruction.instruction].instruction_string, 
                        cpu_reduced_addressing_mode_string[instruction.admr], __FILE__, __LINE__
                    );
                    return 1;
                    break;
            }
            switch (instruction.admx) {
                case ADMX_IMM16:
                    sprintf(output + strlen(output), "0x%.4X;\n", parse_immediate(instruction.expression[2].tokens[0].raw));
                    sprintf(output + strlen(output), "%ssr.AO = (overflow_check - 0x%.4X) < 0;", prefix, parse_immediate(instruction.expression[2].tokens[0].raw));
                    break;

                case ADMX_R0:
                    sprintf(output + strlen(output), "r0;");
                    sprintf(output + strlen(output), "%ssr.AO = (overflow_check - r0) < 0;", prefix);
                    break;

                case ADMX_R1:
                    sprintf(output + strlen(output), "r1;");
                    sprintf(output + strlen(output), "%ssr.AO = (overflow_check - r1) < 0;", prefix);
                    break;

                case ADMX_R2:
                    sprintf(output + strlen(output), "r2;");
                    sprintf(output + strlen(output), "%ssr.AO = (overflow_check - r2) < 0;", prefix);
                    break;

                case ADMX_R3:
                    sprintf(output + strlen(output), "r3;");
                    sprintf(output + strlen(output), "%ssr.AO = (overflow_check - r3) < 0;", prefix);
                    break;

                case ADMX_SP:
                    sprintf(output + strlen(output), "sp;");
                    sprintf(output + strlen(output), "%ssr.AO = (overflow_check - sp) < 0;", prefix);
                    break;

                case ADMX_IND_R3_OFFSET16:
                    sprintf(output + strlen(output), "read16(0x%.4X + r3);", parse_immediate(instruction.expression[2].tokens[1].raw));
                    sprintf(output + strlen(output), "%ssr.AO = (overflow_check - read16(0x%.4X + r3)) < 0;", prefix, parse_immediate(instruction.expression[2].tokens[1].raw));
                    break;
                
                default:
                    log_msg(LP_ERROR, "Transpiler: \"%s\" with admx \"%s\" not handled [%s:%d]", 
                        instruction_encoding[instruction.instruction].instruction_string, 
                        cpu_extended_addressing_mode_string[instruction.admx], __FILE__, __LINE__
                    );
                    return 1;
                    break;
            }
            break;
        }
    
        case MUL: {
            switch (instruction.admr) {
                case ADMR_R0:
                    sprintf(output, "%sr0 *= ", prefix);
                    break;

                case ADMR_R1:
                    sprintf(output, "%sr1 *= ", prefix);
                    break;
                    
                case ADMR_R2:
                    sprintf(output, "%sr2 *= ", prefix);
                    break;

                case ADMR_R3:
                    sprintf(output, "%sr3 *= ", prefix);
                    break;

                case ADMR_SP:
                    sprintf(output, "%ssp *= ", prefix);
                    break;
                
                default:
                    log_msg(LP_ERROR, "Transpiler: \"%s\" with admr \"%s\" not handled [%s:%d]", 
                        instruction_encoding[instruction.instruction].instruction_string, 
                        cpu_reduced_addressing_mode_string[instruction.admr], __FILE__, __LINE__
                    );
                    return 1;
                    break;
            }
            switch (instruction.admx) {
                case ADMX_IMM16:
                    sprintf(output + strlen(output), "0x%.4X;", parse_immediate(instruction.expression[2].tokens[0].raw));
                    break;

                case ADMX_R0:
                    sprintf(output + strlen(output), "r0;");
                    break;

                case ADMX_R1:
                    sprintf(output + strlen(output), "r1;");
                    break;

                case ADMX_R2:
                    sprintf(output + strlen(output), "r2;");
                    break;

                case ADMX_R3:
                    sprintf(output + strlen(output), "r3;");
                    break;

                case ADMX_SP:
                    sprintf(output + strlen(output), "sp;");
                    break;

                case ADMX_IND_R3_OFFSET16:
                    sprintf(output + strlen(output), "read16(0x%.4X + r3);", parse_immediate(instruction.expression[2].tokens[1].raw));
                    break;
                
                default:
                    log_msg(LP_ERROR, "Transpiler: \"%s\" with admx \"%s\" not handled [%s:%d]", 
                        instruction_encoding[instruction.instruction].instruction_string, 
                        cpu_extended_addressing_mode_string[instruction.admx], __FILE__, __LINE__
                    );
                    return 1;
                    break;
            }
            break;
        }
    
        case INC: {
            switch (instruction.admr) {
                case ADMR_R0:
                    sprintf(output, "%soverflow_check = r0;\n", prefix);
                    sprintf(output + strlen(output), "%sr0 ++;\n", prefix);
                    break;

                case ADMR_R1:
                    sprintf(output, "%soverflow_check = r1;\n", prefix);
                    sprintf(output + strlen(output), "%sr1 ++;\n", prefix);
                    break;
                    
                case ADMR_R2:
                    sprintf(output, "%soverflow_check = r2;\n", prefix);
                    sprintf(output + strlen(output), "%sr2 ++;\n", prefix);
                    break;

                case ADMR_R3:
                    sprintf(output, "%soverflow_check = r3;\n", prefix);
                    sprintf(output + strlen(output), "%sr3 ++;\n", prefix);
                    break;

                case ADMR_SP:
                    sprintf(output, "%soverflow_check = sp;\n", prefix);
                    sprintf(output + strlen(output), "%ssp ++;\n", prefix);
                    break;

                case ADMR_IND_R0:
                    sprintf(output, "%soverflow_check = read16(r0);\n", prefix);
                    sprintf(output + strlen(output), "%swrite16(r0, read16(r0) + 1);\n", prefix);
                    break;

                case ADMR_IND16:
                    sprintf(output, "%soverflow_check = read16(0x%.4x);\n", prefix, 
                        parse_immediate(instruction.expression[1].tokens[1].raw)
                    );
                    sprintf(output + strlen(output), "%swrite16(0x%.4x, read16(0x%.4x) + 1);\n", prefix, 
                        parse_immediate(instruction.expression[1].tokens[1].raw), 
                        parse_immediate(instruction.expression[1].tokens[1].raw)
                    );
                    break;
                
                default:
                    log_msg(LP_ERROR, "Transpiler: \"%s\" with admr \"%s\" not handled [%s:%d]", 
                        instruction_encoding[instruction.instruction].instruction_string, 
                        cpu_reduced_addressing_mode_string[instruction.admr], __FILE__, __LINE__
                    );
                    return 1;
                    break;
            }
            sprintf(output + strlen(output), "%ssr.AO = (overflow_check + 1) > 0xFFFF;", prefix);
            break;
        }
    
        case DEC: {
            switch (instruction.admr) {
                case ADMR_R0:
                    sprintf(output, "%soverflow_check = r0;\n", prefix);
                    sprintf(output + strlen(output), "%sr0 --;\n", prefix);
                    break;

                case ADMR_R1:
                    sprintf(output, "%soverflow_check = r1;\n", prefix);
                    sprintf(output + strlen(output), "%sr1 --;\n", prefix);
                    break;
                    
                case ADMR_R2:
                    sprintf(output, "%soverflow_check = r2;\n", prefix);
                    sprintf(output + strlen(output), "%sr2 --;\n", prefix);
                    break;

                case ADMR_R3:
                    sprintf(output, "%soverflow_check = r3;\n", prefix);
                    sprintf(output + strlen(output), "%sr3 --;\n", prefix);
                    break;

                case ADMR_SP:
                    sprintf(output, "%soverflow_check = sp;\n", prefix);
                    sprintf(output + strlen(output), "%ssp --;\n", prefix);
                    break;

                case ADMR_IND_R0:
                    sprintf(output, "%soverflow_check = read16(r0);\n", prefix);
                    sprintf(output + strlen(output), "%swrite16(r0, read16(r0) - 1);\n", prefix);
                    break;

                case ADMR_IND16:
                    sprintf(output, "%soverflow_check = read16(0x%.4x);\n", prefix, 
                        parse_immediate(instruction.expression[1].tokens[1].raw)
                    );
                    sprintf(output + strlen(output), "%swrite16(0x%.4x, read16(0x%.4x) - 1);\n", prefix, 
                        parse_immediate(instruction.expression[1].tokens[1].raw), 
                        parse_immediate(instruction.expression[1].tokens[1].raw)
                    );
                    break;
                
                default:
                    log_msg(LP_ERROR, "Transpiler: \"%s\" with admr \"%s\" not handled [%s:%d]", 
                        instruction_encoding[instruction.instruction].instruction_string, 
                        cpu_reduced_addressing_mode_string[instruction.admr], __FILE__, __LINE__
                    );
                    return 1;
                    break;
            }
            sprintf(output + strlen(output), "%ssr.AO = (overflow_check - 1) < 0;", prefix);
            break;
        }
    
        case ADDF: 
        case SUBF: 
        case MULF: 
        case DIVF: 
        case ADDD: 
        case SUBD: 
        case MULD: 
        case DIVD: {
            char operation[32];
            switch (instruction.instruction) {
                case ADDF:
                    sprintf(operation, "f16_add");
                    break;

                case MULF:
                    sprintf(operation, "f16_mult");
                    break;

                case SUBF:
                    sprintf(operation, "f16_sub");
                    break;

                case DIVF:
                    sprintf(operation, "f16_div");
                    break;

                case ADDD:
                    sprintf(operation, "bf16_add");
                    break;

                case MULD:
                    sprintf(operation, "bf16_mult");
                    break;

                case SUBD:
                    sprintf(operation, "bf16_sub");
                    break;

                case DIVD:
                    sprintf(operation, "bf16_div");
                    break;
                
                default:
                    log_msg(LP_ERROR, "Transpiler: Unknown operation: %s [%s:%d]", instruction_encoding[instruction.instruction].instruction_string, __FILE__, __LINE__);
                    break;
            }
            switch (instruction.admr) {
                case ADMR_R0:
                    sprintf(output, "%sr0 = %s(r0, ", prefix, operation);
                    break;

                case ADMR_R1:
                    sprintf(output, "%sr1 = %s(r1, ", prefix, operation);
                    break;
                    
                case ADMR_R2:
                    sprintf(output, "%sr2 = %s(r2, ", prefix, operation);
                    break;

                case ADMR_R3:
                    sprintf(output, "%sr3 = %s(r3, ", prefix, operation);
                    break;

                case ADMR_SP:
                    sprintf(output, "%ssp = %s(sp, ", prefix, operation);
                    break;
                
                default:
                    log_msg(LP_ERROR, "Transpiler: \"%s\" with admr \"%s\" not handled [%s:%d]", 
                        instruction_encoding[instruction.instruction].instruction_string, 
                        cpu_reduced_addressing_mode_string[instruction.admr], __FILE__, __LINE__
                    );
                    return 1;
                    break;
            }
            switch (instruction.admx) {
                case ADMX_IMM16:
                    sprintf(output + strlen(output), "0x%.4X);", parse_immediate(instruction.expression[2].tokens[0].raw));
                    break;

                case ADMX_R0:
                    sprintf(output + strlen(output), "r0);");
                    break;

                case ADMX_R1:
                    sprintf(output + strlen(output), "r1);");
                    break;

                case ADMX_R2:
                    sprintf(output + strlen(output), "r2);");
                    break;

                case ADMX_R3:
                    sprintf(output + strlen(output), "r3);");
                    break;

                case ADMX_SP:
                    sprintf(output + strlen(output), "sp);");
                    break;

                case ADMX_IND_R3_OFFSET16:
                    sprintf(output + strlen(output), "read16(0x%.4X + r3));", parse_immediate(instruction.expression[2].tokens[1].raw));
                    break;
                
                default:
                    log_msg(LP_ERROR, "Transpiler: \"%s\" with admx \"%s\" not handled [%s:%d]", 
                        instruction_encoding[instruction.instruction].instruction_string, 
                        cpu_extended_addressing_mode_string[instruction.admx], __FILE__, __LINE__
                    );
                    return 1;
                    break;
            }
            break;
        }
    
        case CIF: 
        case CID: 
        case CFI: 
        case CFD: 
        case CDI: 
        case CDF: {
            char operation[64];
            switch (instruction.instruction) {
                case CIF:
                    sprintf(operation, "f16_from_float((float) ((int16_t) ");
                    break;

                case CID:
                    sprintf(operation, "bf16_from_float((float) ((int16_t) ");
                    break;

                case CFI:
                    sprintf(operation, "(int16_t) float_from_f16(");
                    break;

                case CFD:
                    sprintf(operation, "bf16_from_float(float_from_f16(");
                    break;

                case CDI:
                    sprintf(operation, "(int16_t) float_from_bf16(");
                    break;

                case CDF:
                    sprintf(operation, "f16_from_float(float_from_bf16(");
                    break;
                
                default:
                    log_msg(LP_ERROR, "Transpiler: Unknown operation: %s [%s:%d]", instruction_encoding[instruction.instruction].instruction_string, __FILE__, __LINE__);
                    break;
            }
            switch (instruction.admr) {
                case ADMR_R0:
                    sprintf(output, "%sr0 = %sr0", prefix, operation);
                    break;

                case ADMR_R1:
                    sprintf(output, "%sr1 = %sr1", prefix, operation);
                    break;
                    
                case ADMR_R2:
                    sprintf(output, "%sr2 = %sr2", prefix, operation);
                    break;

                case ADMR_R3:
                    sprintf(output, "%sr3 = %sr3", prefix, operation);
                    break;

                case ADMR_SP:
                    sprintf(output, "%ssp = %ssp", prefix, operation);
                    break;
                
                default:
                    log_msg(LP_ERROR, "Transpiler: \"%s\" with admr \"%s\" not handled [%s:%d]", 
                        instruction_encoding[instruction.instruction].instruction_string, 
                        cpu_reduced_addressing_mode_string[instruction.admr], __FILE__, __LINE__
                    );
                    return 1;
                    break;
            }
            switch (instruction.instruction) {
                case CIF:
                case CID:
                case CFD:
                case CDF:
                    sprintf(output + strlen(output), "));");
                    break;
                
                default:
                    sprintf(output + strlen(output), ");");
                    break;
            }
            break;
        }

        case CMP: {
            for (int i = 0; i < 7; i++) {
                switch (i) {
                    case 0: {
                        // Z
                        switch (instruction.admr) {
                            case ADMR_R0:
                                sprintf(output, "%ssr.Z = r0 == ", prefix);
                                break;

                            case ADMR_R1:
                                sprintf(output, "%ssr.Z = r1 == ", prefix);
                                break;
                            
                            default:
                                log_msg(LP_ERROR, "Transpiler: \"%s\" with admr \"%s\" not handled [%s:%d]", 
                                    instruction_encoding[instruction.instruction].instruction_string, 
                                    cpu_reduced_addressing_mode_string[instruction.admr], __FILE__, __LINE__
                                );
                                return 1;
                                break;
                        }
                        switch (instruction.admx) {
                            case ADMX_IMM16:
                                sprintf(output + strlen(output), "0x%.4X;", parse_immediate(instruction.expression[2].tokens[0].raw));
                                break;

                            case ADMX_R0:
                                sprintf(output + strlen(output), "r0;");
                                break;

                            case ADMX_R1:
                                sprintf(output + strlen(output), "r1;");
                                break;

                            case ADMX_IND_R3_OFFSET16:
                                sprintf(output + strlen(output), "read16(0x%.4x + r3);", parse_immediate(instruction.expression[2].tokens[1].raw));
                                break;
                            
                            default:
                                log_msg(LP_ERROR, "Transpiler: \"%s\" with admx \"%s\" not handled [%s:%d]", 
                                    instruction_encoding[instruction.instruction].instruction_string, 
                                    cpu_extended_addressing_mode_string[instruction.admx], __FILE__, __LINE__
                                );
                                return 1;
                                break;
                        }
                        sprintf(output + strlen(output), "\n");
                        break;
                    }

                    case 1: {
                        // FZ
                        switch (instruction.admr) {
                            case ADMR_R0:
                                sprintf(output + strlen(output), "%ssr.FZ = (r0 == ", prefix);
                                break;

                            case ADMR_R1:
                                sprintf(output + strlen(output), "%ssr.FZ = (r1 == ", prefix);
                                break;
                            
                            default:
                                log_msg(LP_ERROR, "Transpiler: \"%s\" with admr \"%s\" not handled [%s:%d]", 
                                    instruction_encoding[instruction.instruction].instruction_string, 
                                    cpu_reduced_addressing_mode_string[instruction.admr], __FILE__, __LINE__
                                );
                                return 1;
                                break;
                        }
                        switch (instruction.admx) {
                            case ADMX_IMM16:
                                sprintf(output + strlen(output), "0x%.4X)", parse_immediate(instruction.expression[2].tokens[0].raw));
                                break;

                            case ADMX_R0:
                                sprintf(output + strlen(output), "r0)");
                                break;

                            case ADMX_R1:
                                sprintf(output + strlen(output), "r1)");
                                break;

                            case ADMX_IND_R3_OFFSET16:
                                sprintf(output + strlen(output), "read16(0x%.4x + r3))", parse_immediate(instruction.expression[2].tokens[1].raw));
                                break;
                            
                            default:
                                log_msg(LP_ERROR, "Transpiler: \"%s\" with admx \"%s\" not handled [%s:%d]", 
                                    instruction_encoding[instruction.instruction].instruction_string, 
                                    cpu_extended_addressing_mode_string[instruction.admx], __FILE__, __LINE__
                                );
                                return 1;
                                break;
                        }
                        switch (instruction.admr) {
                            case ADMR_R0:
                                sprintf(output + strlen(output), " || ((r0 & 0x7FFF) == 0 &&");
                                break;

                            case ADMR_R1:
                                sprintf(output + strlen(output), " || ((r1 & 0x7FFF) == 0 &&");
                                break;
                            
                            default:
                                log_msg(LP_ERROR, "Transpiler: \"%s\" with admr \"%s\" not handled [%s:%d]", 
                                    instruction_encoding[instruction.instruction].instruction_string, 
                                    cpu_reduced_addressing_mode_string[instruction.admr], __FILE__, __LINE__
                                );
                                return 1;
                                break;
                        }
                        switch (instruction.admx) {
                            case ADMX_IMM16:
                                sprintf(output + strlen(output), " (0x%.4X & 0x7FFF) == 0);", parse_immediate(instruction.expression[2].tokens[0].raw));
                                break;

                            case ADMX_R0:
                                sprintf(output + strlen(output), " (r0 & 0x7FFF) == 0);");
                                break;

                            case ADMX_R1:
                                sprintf(output + strlen(output), " (r1 & 0x7FFF) == 0);");
                                break;

                            case ADMX_IND_R3_OFFSET16:
                                sprintf(output + strlen(output), " (read16(0x%.4x + r3) & 0x7FFF) == 0);", parse_immediate(instruction.expression[2].tokens[1].raw));
                                break;
                            
                            default:
                                log_msg(LP_ERROR, "Transpiler: \"%s\" with admx \"%s\" not handled [%s:%d]", 
                                    instruction_encoding[instruction.instruction].instruction_string, 
                                    cpu_extended_addressing_mode_string[instruction.admx], __FILE__, __LINE__
                                );
                                return 1;
                                break;
                        }
                        sprintf(output + strlen(output), "\n");
                        break;
                    }

                    case 2: {
                        // L
                        switch (instruction.admr) {
                            case ADMR_R0:
                                sprintf(output + strlen(output), "%ssr.L = (int16_t) r0 < ", prefix);
                                break;

                            case ADMR_R1:
                                sprintf(output + strlen(output), "%ssr.L = (int16_t) r0 < ", prefix);
                                break;
                            
                            default:
                                log_msg(LP_ERROR, "Transpiler: \"%s\" with admr \"%s\" not handled [%s:%d]", 
                                    instruction_encoding[instruction.instruction].instruction_string, 
                                    cpu_reduced_addressing_mode_string[instruction.admr], __FILE__, __LINE__
                                );
                                return 1;
                                break;
                        }
                        switch (instruction.admx) {
                            case ADMX_IMM16:
                                sprintf(output + strlen(output), "(int16_t) 0x%.4X;", parse_immediate(instruction.expression[2].tokens[0].raw));
                                break;

                            case ADMX_R0:
                                sprintf(output + strlen(output), "(int16_t) r0;");
                                break;

                            case ADMX_R1:
                                sprintf(output + strlen(output), "(int16_t) r1;");
                                break;

                            case ADMX_IND_R3_OFFSET16:
                                sprintf(output + strlen(output), "(int16_t) read16(0x%.4x + r3);", parse_immediate(instruction.expression[2].tokens[1].raw));
                                break;
                            
                            default:
                                log_msg(LP_ERROR, "Transpiler: \"%s\" with admx \"%s\" not handled [%s:%d]", 
                                    instruction_encoding[instruction.instruction].instruction_string, 
                                    cpu_extended_addressing_mode_string[instruction.admx], __FILE__, __LINE__
                                );
                                return 1;
                                break;
                        }
                        sprintf(output + strlen(output), "\n");
                        break;
                    }

                    case 3: {
                        // UL
                        switch (instruction.admr) {
                            case ADMR_R0:
                                sprintf(output + strlen(output), "%ssr.UL = r0 < ", prefix);
                                break;

                            case ADMR_R1:
                                sprintf(output + strlen(output), "%ssr.UL = r1 < ", prefix);
                                break;
                            
                            default:
                                log_msg(LP_ERROR, "Transpiler: \"%s\" with admr \"%s\" not handled [%s:%d]", 
                                    instruction_encoding[instruction.instruction].instruction_string, 
                                    cpu_reduced_addressing_mode_string[instruction.admr], __FILE__, __LINE__
                                );
                                return 1;
                                break;
                        }
                        switch (instruction.admx) {
                            case ADMX_IMM16:
                                sprintf(output + strlen(output), "0x%.4X;", parse_immediate(instruction.expression[2].tokens[0].raw));
                                break;

                            case ADMX_R0:
                                sprintf(output + strlen(output), "r0;");
                                break;

                            case ADMX_R1:
                                sprintf(output + strlen(output), "r1;");
                                break;

                            case ADMX_IND_R3_OFFSET16:
                                sprintf(output + strlen(output), "read16(0x%.4x + r3);", parse_immediate(instruction.expression[2].tokens[1].raw));
                                break;
                            
                            default:
                                log_msg(LP_ERROR, "Transpiler: \"%s\" with admx \"%s\" not handled [%s:%d]", 
                                    instruction_encoding[instruction.instruction].instruction_string, 
                                    cpu_extended_addressing_mode_string[instruction.admx], __FILE__, __LINE__
                                );
                                return 1;
                                break;
                        }
                        sprintf(output + strlen(output), "\n");
                        break;
                    }

                    case 4: {
                        // FL
                        switch (instruction.admr) {
                            case ADMR_R0:
                                sprintf(output + strlen(output), "%ssr.FL = float_from_f16(r0) < ", prefix);
                                break;

                            case ADMR_R1:
                                sprintf(output + strlen(output), "%ssr.FL = float_from_f16(r1) < ", prefix);
                                break;
                            
                            default:
                                log_msg(LP_ERROR, "Transpiler: \"%s\" with admr \"%s\" not handled [%s:%d]", 
                                    instruction_encoding[instruction.instruction].instruction_string, 
                                    cpu_reduced_addressing_mode_string[instruction.admr], __FILE__, __LINE__
                                );
                                return 1;
                                break;
                        }
                        switch (instruction.admx) {
                            case ADMX_IMM16:
                                sprintf(output + strlen(output), "float_from_f16(0x%.4X);", parse_immediate(instruction.expression[2].tokens[0].raw));
                                break;

                            case ADMX_R0:
                                sprintf(output + strlen(output), "r0;");
                                break;

                            case ADMX_R1:
                                sprintf(output + strlen(output), "r1;");
                                break;

                            case ADMX_IND_R3_OFFSET16:
                                sprintf(output + strlen(output), "float_from_f16(read16(0x%.4x + r3));", parse_immediate(instruction.expression[2].tokens[1].raw));
                                break;
                            
                            default:
                                log_msg(LP_ERROR, "Transpiler: \"%s\" with admx \"%s\" not handled [%s:%d]", 
                                    instruction_encoding[instruction.instruction].instruction_string, 
                                    cpu_extended_addressing_mode_string[instruction.admx], __FILE__, __LINE__
                                );
                                return 1;
                                break;
                        }
                        sprintf(output + strlen(output), "\n");
                        break;
                    }

                    case 5: {
                        // DL
                        switch (instruction.admr) {
                            case ADMR_R0:
                                sprintf(output + strlen(output), "%ssr.FL = float_from_bf16(r0) < ", prefix);
                                break;

                            case ADMR_R1:
                                sprintf(output + strlen(output), "%ssr.FL = float_from_bf16(r1) < ", prefix);
                                break;
                            
                            default:
                                log_msg(LP_ERROR, "Transpiler: \"%s\" with admr \"%s\" not handled [%s:%d]", 
                                    instruction_encoding[instruction.instruction].instruction_string, 
                                    cpu_reduced_addressing_mode_string[instruction.admr], __FILE__, __LINE__
                                );
                                return 1;
                                break;
                        }
                        switch (instruction.admx) {
                            case ADMX_IMM16:
                                sprintf(output + strlen(output), "float_from_bf16(0x%.4X);", parse_immediate(instruction.expression[2].tokens[0].raw));
                                break;

                            case ADMX_R0:
                                sprintf(output + strlen(output), "r0;");
                                break;

                            case ADMX_R1:
                                sprintf(output + strlen(output), "r1;");
                                break;

                            case ADMX_IND_R3_OFFSET16:
                                sprintf(output + strlen(output), "float_from_bf16(read16(0x%.4x + r3));", parse_immediate(instruction.expression[2].tokens[1].raw));
                                break;
                            
                            default:
                                log_msg(LP_ERROR, "Transpiler: \"%s\" with admx \"%s\" not handled [%s:%d]", 
                                    instruction_encoding[instruction.instruction].instruction_string, 
                                    cpu_extended_addressing_mode_string[instruction.admx], __FILE__, __LINE__
                                );
                                return 1;
                                break;
                        }
                        sprintf(output + strlen(output), "\n");
                        break;
                    }

                    case 6: {
                        // LL
                        switch (instruction.admr) {
                            case ADMR_R0:
                                sprintf(output + strlen(output), "%ssr.FL = long_long_from_fi16(r0) < ", prefix);
                                break;

                            case ADMR_R1:
                                sprintf(output + strlen(output), "%ssr.FL = long_long_from_fi16(r1) < ", prefix);
                                break;
                            
                            default:
                                log_msg(LP_ERROR, "Transpiler: \"%s\" with admr \"%s\" not handled [%s:%d]", 
                                    instruction_encoding[instruction.instruction].instruction_string, 
                                    cpu_reduced_addressing_mode_string[instruction.admr], __FILE__, __LINE__
                                );
                                return 1;
                                break;
                        }
                        switch (instruction.admx) {
                            case ADMX_IMM16:
                                sprintf(output + strlen(output), "long_long_from_fi16(0x%.4X);", parse_immediate(instruction.expression[2].tokens[0].raw));
                                break;

                            case ADMX_R0:
                                sprintf(output + strlen(output), "r0;");
                                break;

                            case ADMX_R1:
                                sprintf(output + strlen(output), "r1;");
                                break;

                            case ADMX_IND_R3_OFFSET16:
                                sprintf(output + strlen(output), "long_long_from_fi16(read16(0x%.4x + r3));", parse_immediate(instruction.expression[2].tokens[1].raw));
                                break;
                            
                            default:
                                log_msg(LP_ERROR, "Transpiler: \"%s\" with admx \"%s\" not handled [%s:%d]", 
                                    instruction_encoding[instruction.instruction].instruction_string, 
                                    cpu_extended_addressing_mode_string[instruction.admx], __FILE__, __LINE__
                                );
                                return 1;
                                break;
                        }
                        break;
                    }

                    default: 
                        log_msg(LP_WARNING, "Transpiler: Missing further flags! [%s:%d]", __FILE__, __LINE__);
                        break;
                }
            }
            break;
        }

        case TST: {
            for (int i = 0; i < 3; i++) {
                switch (i) {
                    case 0: {
                        // Z
                        switch (instruction.admx) {
                            case ADMX_IMM16:
                                sprintf(output, "%ssr.Z = 0x%.4X == 0x0000;", prefix, 
                                    parse_immediate(instruction.expression[1].tokens[0].raw)
                                );
                                break;

                            case ADMX_R0:
                                sprintf(output, "%ssr.Z = r0 == 0x0000;", prefix);
                                break;

                            case ADMX_R1:
                                sprintf(output, "%ssr.Z = r1 == 0x0000;", prefix);
                                break;

                            case ADMX_IND_R3_OFFSET16:
                                sprintf(output, "%ssr.Z = read16(0x%.4X + r3) == 0x0000;", prefix, 
                                    parse_immediate(instruction.expression[1].tokens[1].raw)
                                );
                                break;
                            
                            default:
                                log_msg(LP_ERROR, "Transpiler: \"%s\" with admx \"%s\" not handled [%s:%d]", 
                                    instruction_encoding[instruction.instruction].instruction_string, 
                                    cpu_extended_addressing_mode_string[instruction.admx], __FILE__, __LINE__
                                );
                                return 1;
                                break;
                        }
                        sprintf(output + strlen(output), "\n");
                        break;
                    }

                    case 1: {
                        // FZ
                        switch (instruction.admx) {
                            case ADMX_IMM16:
                                sprintf(output, "%ssr.Z = (0x%.4X & 0x7FFF) == 0x0000;", prefix, 
                                    parse_immediate(instruction.expression[1].tokens[0].raw)
                                );
                                break;

                            case ADMX_R0:
                                sprintf(output, "%ssr.Z = (r0 & 0x7FFF) == 0x0000;", prefix);
                                break;

                            case ADMX_R1:
                                sprintf(output, "%ssr.Z = (r1 & 0x7FFF) == 0x0000;", prefix);
                                break;

                            case ADMX_IND_R3_OFFSET16:
                                sprintf(output, "%ssr.Z = (read16(0x%.4X + r3) & 0x7FFF) == 0x0000;", prefix, 
                                    parse_immediate(instruction.expression[1].tokens[1].raw)
                                );
                                break;
                            
                            default:
                                log_msg(LP_ERROR, "Transpiler: \"%s\" with admx \"%s\" not handled [%s:%d]", 
                                    instruction_encoding[instruction.instruction].instruction_string, 
                                    cpu_extended_addressing_mode_string[instruction.admx], __FILE__, __LINE__
                                );
                                return 1;
                                break;
                        }
                        sprintf(output + strlen(output), "\n");
                        break;
                    }

                    case 2: {
                        // L
                        switch (instruction.admx) {
                            case ADMX_IMM16:
                                sprintf(output, "%ssr.Z = (int16_t) 0x%.4X < 0x0000;", prefix, 
                                    parse_immediate(instruction.expression[1].tokens[0].raw)
                                );
                                break;

                            case ADMX_R0:
                                sprintf(output + strlen(output), "%ssr.L = (int16_t) r0 < 0x0000;", prefix);
                                break;

                            case ADMX_R1:
                                sprintf(output + strlen(output), "%ssr.L = (int16_t) r1 < 0x0000;", prefix);
                                break;

                            case ADMX_IND_R3_OFFSET16:
                                sprintf(output + strlen(output), "%ssr.L = (int16_t) read16(0x%.4X + r3) < 0x0000;", prefix, 
                                    parse_immediate(instruction.expression[1].tokens[1].raw)
                                );
                                break;
                            
                            default:
                                log_msg(LP_ERROR, "Transpiler: \"%s\" with admx \"%s\" not handled [%s:%d]", 
                                    instruction_encoding[instruction.instruction].instruction_string, 
                                    cpu_extended_addressing_mode_string[instruction.admx], __FILE__, __LINE__
                                );
                                return 1;
                                break;
                        }
                        break;
                    }

                    default: 
                        log_msg(LP_WARNING, "Transpiler: Missing further flags! [%s:%d]", __FILE__, __LINE__);
                        break;
                }
            }
            break;
        }

        case JMP: {
            switch (instruction.admx) {
                case ADMX_IMM16:
                    if (instruction.expression[1].tokens[0].type != TT_LABEL) {
                        log_msg(LP_ERROR, "Transpiler: Jump instruction is only valid when using a label. Got immediate instead");
                    }
                    sprintf(output, "%sgoto _%s;", prefix, instruction.expression[1].tokens[0].raw + 1);
                    break;

                default:
                    log_msg(LP_ERROR, "Transpiler: \"%s\" with admx \"%s\" not handled [%s:%d]", 
                        instruction_encoding[instruction.instruction].instruction_string, 
                        cpu_extended_addressing_mode_string[instruction.admx], __FILE__, __LINE__
                    );
                    return 1;
                    break;
                }
            break;
        }

        case JZ: 
        case JNZ: 
        case JFZ: 
        case JNFZ: 
        case JL: 
        case JNL: 
        case JUL: 
        case JNUL: 
        case JFL: 
        case JNFL: 
        case JDL: 
        case JNDL: 
        case JLL: 
        case JNLL: 
        case JAO: 
        case JNAO: {
            char flag[8];
            switch(instruction.instruction) {
                case JZ: 
                    sprintf(flag, "sr.Z");
                    break;
                case JNZ: 
                    sprintf(flag, "!sr.Z");
                    break;
                case JFZ: 
                    sprintf(flag, "sr.FZ");
                    break;
                case JNFZ: 
                    sprintf(flag, "!sr.FZ");
                    break;
                case JL: 
                    sprintf(flag, "sr.L");
                    break;
                case JNL: 
                    sprintf(flag, "!sr.L");
                    break;
                case JUL: 
                    sprintf(flag, "sr.UL");
                    break;
                case JNUL: 
                    sprintf(flag, "!sr.UL");
                    break;
                case JFL: 
                    sprintf(flag, "sr.FL");
                    break;
                case JNFL: 
                    sprintf(flag, "!sr.FL");
                    break;
                case JDL: 
                    sprintf(flag, "sr.DL");
                    break;
                case JNDL: 
                    sprintf(flag, "!sr.DL");
                    break;
                case JLL: 
                    sprintf(flag, "sr.LL");
                    break;
                case JNLL: 
                    sprintf(flag, "!sr.LL");
                    break;
                case JAO: 
                    sprintf(flag, "sr.AO");
                    break;
                case JNAO:
                    sprintf(flag, "!sr.AO");
                    break;
                default:
                    log_msg(LP_ERROR, "Transpiler: Unknown flag [%s:%d]", __FILE__, __LINE__);
                    break;
            }
            switch (instruction.admx) {
                case ADMX_IMM16:
                    sprintf(output, "%sif (%s) goto _%s;", prefix, flag, instruction.expression[1].tokens[0].raw + 1);
                    break;

                default:
                    log_msg(LP_ERROR, "Transpiler: \"%s\" with admx \"%s\" not handled [%s:%d]", 
                        instruction_encoding[instruction.instruction].instruction_string, 
                        cpu_extended_addressing_mode_string[instruction.admx], __FILE__, __LINE__
                    );
                    return 1;
                    break;
                }
            break;
        }

        case XOR:
            switch (instruction.admr) {
                case ADMR_R0:
                    sprintf(output, "%sr0 ^= ", prefix);
                    break;

                case ADMR_R1:
                    sprintf(output, "%sr1 ^= ", prefix);
                    break;
                    
                case ADMR_R2:
                    sprintf(output, "%sr2 ^= ", prefix);
                    break;

                case ADMR_R3:
                    sprintf(output, "%sr3 ^= ", prefix);
                    break;

                case ADMR_SP:
                    sprintf(output, "%ssp ^= ", prefix);
                    break;
                
                default:
                    log_msg(LP_ERROR, "Transpiler: \"%s\" with admr \"%s\" not handled [%s:%d]", 
                        instruction_encoding[instruction.instruction].instruction_string, 
                        cpu_reduced_addressing_mode_string[instruction.admr], __FILE__, __LINE__
                    );
                    return 1;
                    break;
            }
            switch (instruction.admx) {
                case ADMX_IMM16:
                    sprintf(output + strlen(output), "0x%.4X;", parse_immediate(instruction.expression[2].tokens[0].raw));
                    break;

                case ADMX_R0:
                    sprintf(output + strlen(output), "r0;");
                    break;

                case ADMX_R1:
                    sprintf(output + strlen(output), "r1;");
                    break;

                case ADMX_R2:
                    sprintf(output + strlen(output), "r2;");
                    break;

                case ADMX_R3:
                    sprintf(output + strlen(output), "r3;");
                    break;

                case ADMX_SP:
                    sprintf(output + strlen(output), "sp;");
                    break;
                
                default:
                    log_msg(LP_ERROR, "Transpiler: \"%s\" with admr \"%s\" not handled [%s:%d]", 
                        instruction_encoding[instruction.instruction].instruction_string, 
                        cpu_reduced_addressing_mode_string[instruction.admr], __FILE__, __LINE__
                    );
                    return 1;
                    break;
            }
            break;
    
        case SEMI: {
            sprintf(output, "%ssr.MI = 0x0001;", prefix);
            break;
        }
    
        case CLMI: {
            sprintf(output, "%ssr.MI = 0x0000;", prefix);
            break;
        }
    
        case PUSHSR: {
            sprintf(output, "%spush16(sr.value);", prefix);
            break;
        }
    
        case POPSR: {
            sprintf(output, "%ssr.value = pop16();", prefix);
            break;
        }

        case HLT: {
            sprintf(output, "%sprint_registers();\n%sexit(0);", prefix, prefix);
            break;
        }
        
        default:
            log_msg(LP_ERROR, "Transpiler: Instruction \"%s\" not handled [%s:%d]", instruction_encoding[instruction.instruction].instruction_string, __FILE__, __LINE__);
            return 1;
            break;
    }
    return 0;
}


char* transpile(char* asm, long* file_size) {

    /*
    Transpile options: 

    no_helper -> removes read16 and write16 (THIS IS A NIGHTMARE IF WE ADD MMAPs ETC.!)
    
    */

    long binary_size = 0;
    uint16_t* segment = NULL;
    int seegment_count = 0;
    uint8_t* bin = assembler_compile(asm, &binary_size, &segment, &seegment_count, 0);
    if (!bin) {
        log_msg(LP_ERROR, "Transpiler: Assembler returned NULL [%s:%d]", __FILE__, __LINE__);
        return NULL;
    }

    char* content = disassembler_decompile(bin, binary_size, segment, seegment_count, DO_ADD_JUMP_LABEL | DO_ADD_DEST_LABEL);

    data_export("temp.dmp", content, strlen(content));


    assembler_reset();

    // split text into lines
    char** line = assembler_split_to_separate_lines(content);

    // remove comments from lines
    //assembler_remove_comments(&line);

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
    Instruction_t* instruction = assembler_parse_expression(expression, expression_count, &instruction_count, &segment, &seegment_count, 0);

    free(segment);

    for (int i = 0; i < instruction_count; i++) {
        //log_msg(LP_DEBUG, "%d: %d", i, instruction[i].is_address);
    }

    // resolve label
    instruction = assembler_resolve_labels(instruction, instruction_count);

    if (!instruction) {
        log_msg(LP_ERROR, "Transpiler: Assembler returned NULL [%s:%d]", __FILE__, __LINE__);
    }

    char* c_code = NULL;
    long c_code_length = 0;

    c_code = append_to_output(c_code, &c_code_length, 
        "#include <stdint.h>\n"
        "#include <stdio.h>\n"
        "#include <stdlib.h>\n\n"
        "static uint8_t ram[65536] = {"
    );

    /*char tmp[128];
    for (long i = 0; i < binary_size - 1; i++) {
        sprintf(tmp, "0x%.2x, ", bin[i]);
        c_code = append_to_output(c_code, &c_code_length, tmp);
        if (!((i+1) % 0x40)) {
            c_code = append_to_output(c_code, &c_code_length, "\n\t");
        }
    }
    sprintf(tmp, "0x%.2x\n};\n", bin[binary_size - 1]);
    c_code = append_to_output(c_code, &c_code_length, tmp);*/

    char tmp[512];
    long idx = 0;
    int latch = 0;
    for (long i = 0; i < binary_size; i++) {
        if (!((idx) % 0x10) && latch == 0) {
            c_code = append_to_output(c_code, &c_code_length, "\n\t");
            latch = 1;
        }
        if (bin[i]) {
            sprintf(tmp, "[0x%.4lX] = 0x%.2x, ", i, bin[i]);
            c_code = append_to_output(c_code, &c_code_length, tmp);
            idx ++;
            latch = 0;
        }
    }
    sprintf(tmp, "\n};\n\n");
    c_code = append_to_output(c_code, &c_code_length, tmp);

    c_code = append_to_output(c_code, &c_code_length, 
        "static uint16_t r0, r1, r2, r3, sp;\n\n"
        "static int32_t overflow_check;\n\n"
        "static struct {\n"
        "\tunion {\n"
        "\t\tuint16_t value;\n"
        "\t\tstruct {\n"
        "\t\t\tuint16_t Z : 1;\n"
        "\t\t\tuint16_t FZ : 1;\n"
        "\t\t\tuint16_t L : 1;\n"
        "\t\t\tuint16_t UL : 1;\n"
        "\t\t\tuint16_t FL : 1;\n"
        "\t\t\tuint16_t DL : 1;\n"
        "\t\t\tuint16_t LL : 1;\n"
        "\t\t\tuint16_t AO : 1;\n"
        "\t\t\tuint16_t MI : 1;\n"
        "\t\t};\n"
        "\t};\n"
        "} sr = {0};\n\n"
        "void write16(uint16_t address, uint16_t data) {\n"
        "\tram[address] = data & 0x00ff;\n"
        "\tram[address + 1] = data >> 8;\n"
        "}\n"
        "uint16_t read16(uint16_t address) {\n"
        "\treturn ram[address] | (ram[address + 1] << 8);\n"
        "}\n\n"
        "void push16(uint16_t data) {\n"
        "\tsp -= 0x0002;\n"
        "\twrite16(sp, data);\n"
        "}\n"
        "uint16_t pop16(void) {\n"
        "\tuint16_t data = read16(sp);\n"
        "\tsp += 0x0002;\n"
        "\treturn data;\n"
        "}\n\n"
    );

    // TODO: Filter by which function is actually needed by the set of instructions
    c_code = append_to_output(c_code, &c_code_length, "typedef uint16_t float16_t;\n");
    c_code = append_to_output(c_code, &c_code_length, "typedef enum {\n\tF16_ZERO,\n\tF16_SUBNORMAL,\n\tF16_NORMAL,\n\tF16_INF,\n\tF16_NAN\n} f16_class;\n\n");
    char* extended_types = "float16_t f16_from_float(float f32) { \n\tvoid* tmp = &f32;\n\tuint32_t bits = *(uint32_t*)tmp;\n\tuint32_t sign = bits >> 31;\n\tint exp32 = (bits >> 23) & 0xFF;\n\tuint32_t mant32 = bits & 0x7FFFFF;\n\n\tuint16_t exp16, mant16;\n\n\tif (exp32 == 0xFF) {\n\t\texp16 = 0x1F;\n\t\tmant16 = mant32 ? 0x200 : 0;\n\t} else if (exp32 == 0) {\n\t\texp16 = 0;\n\t\tmant16 = 0;\n\t} else {\n\t\tint new_exp = exp32 - 127 + 15;\n\t\tif (new_exp >= 0x1F) {\n\t\t\texp16 = 0x1F;\n\t\t\tmant16 = 0;\n\t\t} else if (new_exp <= 0) {\n\t\t\tuint32_t m = mant32 | 0x800000;\n\t\t\tint shift = 1 - new_exp;\n\t\t\tif (shift > 24) {\n\t\t\t\texp16 = 0;\n\t\t\t\tmant16 = 0;\n\t\t\t} else {\n\t\t\t\tm += 1u << (shift - 1);\n\t\t\t\tmant16 = m >> (shift + 13);\n\t\t\t\texp16 = 0;\n\t\t\t}\n\t\t} else {\n\t\t\tmant32 += 1u << 12;\n\t\t\tmant16 = mant32 >> 13;\n\t\t\texp16 = new_exp;\n\t\t\tif (mant16 == 0x400) {\n\t\t\t\tmant16 = 0;\n\t\t\t\texp16++;\n\t\t\t}\n\t\t}\n\t}\n\n\treturn (sign << 15) | (exp16 << 10) | (mant16 & 0x3FF);\n}\n\n";
    c_code = append_to_output(c_code, &c_code_length, extended_types);
    extended_types = "float float_from_f16(float16_t f16) {\n\tuint32_t sign = (f16 >> 15) & 1;\n\tuint32_t exp = (f16 >> 10) & 0x1F;\n\tuint32_t mant = f16 & 0x3FF;\n\n\tuint32_t exp32, mant32;\n\n\tif (exp == 0) {\n\t\tif (mant == 0) {\n\t\t\texp32 = 0;\n\t\t\tmant32 = 0;\n\t\t} else {\n\t\t\tint shift = 0;\n\t\t\twhile ((mant & 0x400) == 0) {\n\t\t\t\tmant <<= 1;\n\t\t\t\tshift++;\n\t\t\t}\n\t\t\tmant &= 0x3FF;\n\t\t\texp32 = 127 - 14 - shift;\n\t\t\tmant32 = mant << 13;\n\t\t}\n\t} else if (exp == 0x1F) {\n\t\texp32 = 0xFF;\n\t\tmant32 = mant << 13;\n\t} else {\n\t\texp32 = exp - 15 + 127;\n\t\tmant32 = mant << 13;\n\t}\n\n\tuint32_t bits = (sign << 31) | (exp32 << 23) | mant32;\n\tvoid* tmp = &bits;\n\treturn *(float*)tmp;\n}\n\n";
    c_code = append_to_output(c_code, &c_code_length, extended_types);
    extended_types = "static f16_class f16_unpack(float16_t f, int *sign, int *exp, uint32_t *mant) {\n\t*sign = (f >> 15) & 1;\n\tuint16_t e = (f >> 10) & 0x1F;\n\tuint32_t m = f & 0x3FF;\n\n\tif (e == 0) {\n\t\tif (m == 0) {\n\t\t\t*exp = 0;\n\t\t\t*mant = 0;\n\t\t\treturn F16_ZERO;\n\t\t}\n\t\t*exp = -14;\n\t\t*mant = m;\n\t\treturn F16_SUBNORMAL;\n\t}\n\n\tif (e == 0x1F) {\n\t\t*exp = 0;\n\t\t*mant = m;\n\t\treturn m ? F16_NAN : F16_INF;\n\t}\n\n\t*exp = e - 15;\n\t*mant = m | 0x400;\n\treturn F16_NORMAL;\n}\n\n";
    c_code = append_to_output(c_code, &c_code_length, extended_types);
    extended_types = "static float16_t f16_pack(int sign, int exp, uint32_t mant) {\n\tif (mant == 0)\n\t\treturn sign << 15;\n\n\twhile (mant >= (1u << 11)) {\n\t\tmant >>= 1;\n\t\texp++;\n\t}\n\twhile (mant < (1u << 10)) {\n\t\tmant <<= 1;\n\t\texp--;\n\t}\n\n\tif (exp > 15)\n\t\treturn (sign << 15) | (0x1F << 10);\n\n\tif (exp < -14) {\n\t\tint shift = (-14) - exp;\n\t\tmant = (shift >= 32) ? 0 : (mant + (1u << (shift - 1))) >> shift;\n\t\treturn (sign << 15) | (mant & 0x3FF);\n\t}\n\n\treturn (sign << 15) | ((exp + 15) << 10) | (mant & 0x3FF);\n}\n\n";
    c_code = append_to_output(c_code, &c_code_length, extended_types);
    extended_types = "float16_t f16_add(float16_t a, float16_t b) {\n\tint sa, sb, ea, eb;\n\tuint32_t ma, mb;\n\tf16_class ca = f16_unpack(a, &sa, &ea, &ma);\n\tf16_class cb = f16_unpack(b, &sb, &eb, &mb);\n\n\tif (ca == F16_NAN || cb == F16_NAN) return 0x7E00;\n\n\tif (ca == F16_INF || cb == F16_INF) {\n\t\tif (ca == cb && sa != sb) return 0x7E00;\n\t\tint s = (ca == F16_INF) ? sa : sb;\n\t\treturn (s << 15) | (0x1F << 10);\n\t}\n\n\tif (ca == F16_ZERO) return b;\n\tif (cb == F16_ZERO) return a;\n\n\tif (ea > eb) mb >>= (ea - eb);\n\telse if (eb > ea) ma >>= (eb - ea);\n\n\tint exp = ea > eb ? ea : eb;\n\tuint32_t mant;\n\tint sign;\n\n\tif (sa == sb) {\n\t\tmant = ma + mb;\n\t\tsign = sa;\n\t} else {\n\t\tif (ma >= mb) {\n\t\t\tmant = ma - mb;\n\t\t\tsign = sa;\n\t\t} else {\n\t\t\tmant = mb - ma;\n\t\t\tsign = sb;\n\t\t}\n\t}\n\n\treturn f16_pack(sign, exp, mant);\n}\n\n";
    c_code = append_to_output(c_code, &c_code_length, extended_types);
    extended_types = "float16_t f16_sub(float16_t a, float16_t b) {\n\treturn f16_add(a, b ^ 0x8000);\n}\n\n";
    c_code = append_to_output(c_code, &c_code_length, extended_types);
    extended_types = "float16_t f16_mult(float16_t a, float16_t b) {\n\tint sa, sb, ea, eb;\n\tuint32_t ma, mb;\n\tf16_class ca = f16_unpack(a, &sa, &ea, &ma);\n\tf16_class cb = f16_unpack(b, &sb, &eb, &mb);\n\tint sign = sa ^ sb;\n\n\tif (ca == F16_NAN || cb == F16_NAN) return 0x7E00;\n\tif ((ca == F16_INF && cb == F16_ZERO) ||\n\t\t(cb == F16_INF && ca == F16_ZERO)) return 0x7E00;\n\tif (ca == F16_INF || cb == F16_INF)\n\t\treturn (sign << 15) | (0x1F << 10);\n\tif (ca == F16_ZERO || cb == F16_ZERO)\n\t\treturn sign << 15;\n\n\tuint64_t prod = (uint64_t)ma * mb;\n\tint exp = ea + eb;\n\n\tif (prod & (1ULL << 21)) {\n\t\tprod >>= 11;\n\t\texp++;\n\t} else {\n\t\tprod >>= 10;\n\t}\n\n\treturn f16_pack(sign, exp, (uint32_t)prod);\n}\n\n";
    c_code = append_to_output(c_code, &c_code_length, extended_types);
    extended_types = "float16_t f16_div(float16_t a, float16_t b) {\n\tint sa, sb, ea, eb;\n\tuint32_t ma, mb;\n\tf16_class ca = f16_unpack(a, &sa, &ea, &ma);\n\tf16_class cb = f16_unpack(b, &sb, &eb, &mb);\n\tint sign = sa ^ sb;\n\n\tif (ca == F16_NAN || cb == F16_NAN) return 0x7E00;\n\tif (ca == F16_INF && cb == F16_INF) return 0x7E00;\n\tif (cb == F16_INF) return sign << 15;\n\tif (ca == F16_INF) return (sign << 15) | (0x1F << 10);\n\tif (cb == F16_ZERO) return (sign << 15) | (0x1F << 10);\n\tif (ca == F16_ZERO) return sign << 15;\n\n\tint shift = 0;\n\twhile (mb < (1u << 10)) {\n\t\tmb <<= 1;\n\t\tshift++;\n\t}\n\n\tuint64_t dividend = (uint64_t)ma << (10 + shift);\n\tuint32_t mant = dividend / mb;\n\tint exp = ea - eb - shift;\n\n\treturn f16_pack(sign, exp, mant);\n}\n\n";
    c_code = append_to_output(c_code, &c_code_length, extended_types);
    extended_types = "float16_t f16_neg(float16_t x) { return x ^ 0x8000; }\nfloat16_t f16_abs(float16_t x) { return x & 0x7FFF; }\n\n";
    c_code = append_to_output(c_code, &c_code_length, extended_types);

    c_code = append_to_output(c_code, &c_code_length, "typedef uint16_t bfloat16_t;\n");
    c_code = append_to_output(c_code, &c_code_length, "typedef enum {\n\tBF16_ZERO,\n\tBF16_SUBNORMAL,\n\tBF16_NORMAL,\n\tBF16_INF,\n\tBF16_NAN\n} bf16_class;\n\n");
    extended_types = "bfloat16_t bf16_from_float(float f32) {\n\tvoid* tmp = &f32;\n\tuint32_t bits = *(uint32_t*)tmp;\n\tuint32_t lsb = (bits >> 16) & 1;\n\tuint32_t rounding_bias = 0x7FFF + lsb;\n\tbits += rounding_bias;\n\treturn (bfloat16_t)(bits >> 16);\n}\n\n";
    c_code = append_to_output(c_code, &c_code_length, extended_types);
    extended_types = "float float_from_bf16(bfloat16_t bf16) {\n\tuint32_t bits = ((uint32_t)bf16) << 16;\n\tvoid* tmp = &bits;\n\treturn *(float*)tmp;\n}\n\n";
    c_code = append_to_output(c_code, &c_code_length, extended_types);
    extended_types = "static bf16_class bf16_unpack(\n\tbfloat16_t f,\n\tint *sign,\n\tint *exp,\n\tuint32_t *mant\n) {\n\t*sign = (f >> 15) & 1;\n\tuint16_t e = (f >> 7) & 0xFF;\n\tuint32_t m = f & 0x7F;\n\n\tif (e == 0) {\n\t\tif (m == 0) {\n\t\t\t*exp = 0;\n\t\t\t*mant = 0;\n\t\t\treturn BF16_ZERO;\n\t\t}\n\t\t*exp = 1 - 127;\n\t\t*mant = m;\n\t\treturn BF16_SUBNORMAL;\n\t}\n\n\tif (e == 0xFF) {\n\t\t*exp = 0;\n\t\t*mant = m;\n\t\treturn m ? BF16_NAN : BF16_INF;\n\t}\n\n\t*exp = e - 127;\n\t*mant = m | 0x80;\n\treturn BF16_NORMAL;\n}\n\n";
    c_code = append_to_output(c_code, &c_code_length, extended_types);
    extended_types = "static bfloat16_t bf16_pack(int sign, int exp, uint32_t mant) {\n\tif (mant == 0)\n\t\treturn sign << 15;\n\n\twhile (mant >= (1u << 8)) {\n\t\tmant >>= 1;\n\t\texp++;\n\t}\n\twhile (mant < (1u << 7)) {\n\t\tmant <<= 1;\n\t\texp--;\n\t}\n\n\tif (exp >= 128)\n\t\treturn (sign << 15) | (0xFF << 7);\n\n\tif (exp <= -127) {\n\t\tint shift = (-127) - exp + 1;\n\t\tif (shift >= 32)\n\t\t\tmant = 0;\n\t\telse\n\t\t\tmant >>= shift;\n\t\treturn (sign << 15) | (mant & 0x7F);\n\t}\n\n\treturn (sign << 15) | ((exp + 127) << 7) | (mant & 0x7F);\n}\n\n";
    c_code = append_to_output(c_code, &c_code_length, extended_types);
    extended_types = "bfloat16_t bf16_add(bfloat16_t a, bfloat16_t b) {\n\tint sa, sb, ea, eb;\n\tuint32_t ma, mb;\n\tbf16_class ca = bf16_unpack(a, &sa, &ea, &ma);\n\tbf16_class cb = bf16_unpack(b, &sb, &eb, &mb);\n\n\tif (ca == BF16_NAN || cb == BF16_NAN)\n\t\treturn 0x7FC0;\n\n\tif (ca == BF16_INF || cb == BF16_INF) {\n\t\tif (ca == cb && sa != sb)\n\t\t\treturn 0x7FC0;\n\t\tint s = (ca == BF16_INF) ? sa : sb;\n\t\treturn (s << 15) | (0xFF << 7);\n\t}\n\n\tif (ca == BF16_ZERO) return b;\n\tif (cb == BF16_ZERO) return a;\n\n\tif (ea > eb) mb >>= (ea - eb);\n\telse if (eb > ea) ma >>= (eb - ea);\n\n\tint exp = (ea > eb) ? ea : eb;\n\n\tuint32_t mant;\n\tint sign;\n\n\tif (sa == sb) {\n\t\tmant = ma + mb;\n\t\tsign = sa;\n\t} else {\n\t\tif (ma >= mb) {\n\t\t\tmant = ma - mb;\n\t\t\tsign = sa;\n\t\t} else {\n\t\t\tmant = mb - ma;\n\t\t\tsign = sb;\n\t\t}\n\t}\n\n\treturn bf16_pack(sign, exp, mant);\n}\n\n";
    c_code = append_to_output(c_code, &c_code_length, extended_types);
    extended_types = "bfloat16_t bf16_sub(bfloat16_t a, bfloat16_t b) {\n\treturn bf16_add(a, b ^ 0x8000);\n}\n\n";
    c_code = append_to_output(c_code, &c_code_length, extended_types);
    extended_types = "bfloat16_t bf16_mult(bfloat16_t a, bfloat16_t b) {\n\tint sa, sb, ea, eb;\n\tuint32_t ma, mb;\n\tbf16_class ca = bf16_unpack(a, &sa, &ea, &ma);\n\tbf16_class cb = bf16_unpack(b, &sb, &eb, &mb);\n\n\tint sign = sa ^ sb;\n\n\tif (ca == BF16_NAN || cb == BF16_NAN)\n\t\treturn 0x7FC0;\n\n\tif ((ca == BF16_INF && cb == BF16_ZERO) ||\n\t\t(cb == BF16_INF && ca == BF16_ZERO))\n\t\treturn 0x7FC0;\n\n\tif (ca == BF16_INF || cb == BF16_INF)\n\t\treturn (sign << 15) | (0xFF << 7);\n\n\tif (ca == BF16_ZERO || cb == BF16_ZERO)\n\t\treturn sign << 15;\n\n\tuint64_t prod = (uint64_t)ma * mb;\n\tint exp = ea + eb;\n\n\tif (prod & (1ULL << 15)) {\n\t\tprod >>= 8;\n\t\texp++;\n\t} else {\n\t\tprod >>= 7;\n\t}\n\n\treturn bf16_pack(sign, exp, (uint32_t)prod);\n}\n\n";
    c_code = append_to_output(c_code, &c_code_length, extended_types);
    extended_types = "bfloat16_t bf16_div(bfloat16_t a, bfloat16_t b) {\n\tint sa, sb, ea, eb;\n\tuint32_t ma, mb;\n\tbf16_class ca = bf16_unpack(a, &sa, &ea, &ma);\n\tbf16_class cb = bf16_unpack(b, &sb, &eb, &mb);\n\n\tint sign = sa ^ sb;\n\n\tif (ca == BF16_NAN || cb == BF16_NAN)\n\t\treturn 0x7FC0;\n\n\tif (ca == BF16_INF && cb == BF16_INF)\n\t\treturn 0x7FC0;\n\n\tif (cb == BF16_INF)\n\t\treturn sign << 15;\n\n\tif (ca == BF16_INF)\n\t\treturn (sign << 15) | (0xFF << 7);\n\n\tif (cb == BF16_ZERO)\n\t\treturn (sign << 15) | (0xFF << 7);\n\n\tif (ca == BF16_ZERO)\n\t\treturn sign << 15;\n\n\tint shift = 0;\n\twhile (mb < (1u << 7)) {\n\t\tmb <<= 1;\n\t\tshift++;\n\t}\n\n\tuint64_t dividend = (uint64_t)ma << (7 + shift);\n\tuint32_t mant = dividend / mb;\n\tint exp = ea - eb - shift;\n\n\treturn bf16_pack(sign, exp, mant);\n}\n\n";
    c_code = append_to_output(c_code, &c_code_length, extended_types);
    extended_types = "bfloat16_t bf16_neg(bfloat16_t x) { return x ^ 0x8000; }\nbfloat16_t bf16_abs(bfloat16_t x) { return x & 0x7FFF; }\n\n";
    c_code = append_to_output(c_code, &c_code_length, extended_types);

    c_code = append_to_output(c_code, &c_code_length, "typedef uint16_t fint16_t;\n");
    extended_types = "long long int long_long_from_fi16(fint16_t fi16) {\n\tconst long long int mbits = 10;\n\tlong long int sign = fi16 & 0x8000;\n\tlong long int exponent = (fi16 & 0x7fff) >> mbits;\n\tlong long int mantissa = fi16 & ((1 << mbits) - 1);\n\tif (exponent == 0) {\n\t\treturn sign ? -mantissa : mantissa;\n\t}\n\treturn ((mantissa + (1 << mbits)) << (exponent - 1)) * (sign ? -1 : 1);\n}\n\n";
    c_code = append_to_output(c_code, &c_code_length, extended_types);
    extended_types = "int int_from_fi16(fint16_t fi16) {\n\tconst int mbits = 10;\n\tint sign = fi16 & 0x8000;\n\tint exponent = (fi16 & 0x7fff) >> mbits;\n\tint mantissa = fi16 & ((1 << mbits) - 1);\n\tif (exponent == 0) {\n\t\treturn sign ? -mantissa : mantissa;\n\t}\n\treturn ((mantissa + (1 << mbits)) << (exponent - 1)) * (sign ? -1 : 1);\n}\n\n";
    c_code = append_to_output(c_code, &c_code_length, extended_types);
    extended_types = "fint16_t fi16_from_int(int i32) {\n\tconst int mbits = 10;\n\tfint16_t fi16;\n\t\n\tint sign = (i32 < 0) ? 1 : 0;\n\tint abs_i32 = (i32 < 0) ? -i32 : i32;\n\t\n\tif (abs_i32 == 0) {\n\t\treturn 0x0000;\n\t}\n\t\n\tint msb_pos = 0;\n\tunsigned int temp = abs_i32;\n\twhile (temp >>= 1) {\n\t\tmsb_pos++;\n\t}\n\t\n\tint exponent, mantissa;\n\t\n\tif (msb_pos < mbits) {\n\t\texponent = 0;\n\t\tmantissa = abs_i32;\n\t} else {\n\t\texponent = msb_pos - 9;\n\t\tint shift = exponent - 1;\n\t\tmantissa = (abs_i32 >> shift) - (1 << mbits);\n\t}\n\t\n\tfi16 = (sign << 15) | (exponent << mbits) | mantissa;\n\t\n\tif (mantissa < ((1 << mbits) - 1) || exponent < 31) {\n\t\tfint16_t fi16_plus_1 = fi16 + 1;\n\t\t\n\t\tint reconstructed = int_from_fi16(fi16);\n\t\tint reconstructed_plus_1 = int_from_fi16(fi16_plus_1);\n\t\t\n\t\tint dist_current = abs_i32 - reconstructed;\n\t\tif (dist_current < 0) dist_current = -dist_current;\n\t\t\n\t\tint dist_next = abs_i32 - reconstructed_plus_1;\n\t\tif (dist_next < 0) dist_next = -dist_next;\n\t\t\n\t\tif (dist_next < dist_current) {\n\t\t\tfi16 = fi16_plus_1;\n\t\t}\n\t}\n\t\n\treturn fi16;\n}\n\n";
    c_code = append_to_output(c_code, &c_code_length, extended_types);
    extended_types = "fint16_t fi16_from_long_long(long long int i64) {\n\tconst int mbits = 10;\n\tfint16_t fi16;\n\t\n\tint sign = (i64 < 0) ? 1 : 0;\n\tlong long int abs_i64 = (i64 < 0) ? -i64 : i64;\n\t\n\tif (abs_i64 == 0) {\n\t\treturn 0x0000;\n\t}\n\t\n\tint msb_pos = 0;\n\tlong long int temp = abs_i64;\n\twhile (temp >>= 1) {\n\t\tmsb_pos++;\n\t}\n\t\n\tint exponent, mantissa;\n\t\n\tif (msb_pos < mbits) {\n\t\texponent = 0;\n\t\tmantissa = abs_i64;\n\t} else {\n\t\texponent = msb_pos - 9;\n\t\tint shift = exponent - 1;\n\t\tmantissa = (abs_i64 >> shift) - (1 << mbits);\n\t}\n\t\n\tfi16 = (sign << 15) | (exponent << mbits) | mantissa;\n\t\n\tif (mantissa < ((1 << mbits) - 1) || exponent < 31) {\n\t\tfint16_t fi16_plus_1 = fi16 + 1;\n\t\t\n\t\tlong long int reconstructed = long_long_from_fi16(fi16);\n\t\tlong long int reconstructed_plus_1 = long_long_from_fi16(fi16_plus_1);\n\t\t\n\t\tlong long int dist_current = abs_i64 - reconstructed;\n\t\tif (dist_current < 0) dist_current = -dist_current;\n\t\t\n\t\tlong long int dist_next = abs_i64 - reconstructed_plus_1;\n\t\tif (dist_next < 0) dist_next = -dist_next;\n\t\t\n\t\tif (dist_next < dist_current) {\n\t\t\tfi16 = fi16_plus_1;\n\t\t}\n\t}\n\t\n\treturn fi16;\n}\n\n";
    c_code = append_to_output(c_code, &c_code_length, extended_types);
    extended_types = "fint16_t fi16_add(fint16_t fi16_a, fint16_t fi16_b) {\n\tconst int mbits = 10;\n\t\n\t// Extract fields from both operands\n\tuint16_t sign_a = (fi16_a >> 15) & 1;\n\tuint16_t exp_a = (fi16_a >> mbits) & 0x1F;\n\tuint16_t mant_a = fi16_a & 0x3FF;\n\t\n\tuint16_t sign_b = (fi16_b >> 15) & 1;\n\tuint16_t exp_b = (fi16_b >> mbits) & 0x1F;\n\tuint16_t mant_b = fi16_b & 0x3FF;\n\t\n\t// Convert to actual mantissa values (add implicit 1 for normalized)\n\tuint32_t actual_mant_a = (exp_a == 0) ? mant_a : (mant_a + (1 << mbits));\n\tuint32_t actual_mant_b = (exp_b == 0) ? mant_b : (mant_b + (1 << mbits));\n\t\n\t// Handle special case: both are subnormal\n\tif (exp_a == 0 && exp_b == 0) {\n\t\tif (sign_a == sign_b) {\n\t\t\tint result_val = actual_mant_a + actual_mant_b;\n\t\t\treturn fi16_from_int(sign_a ? -result_val : result_val);\n\t\t} else {\n\t\t\tint result_val, result_sign;\n\t\t\tif (actual_mant_a >= actual_mant_b) {\n\t\t\t\tresult_val = actual_mant_a - actual_mant_b;\n\t\t\t\tresult_sign = sign_a;\n\t\t\t} else {\n\t\t\t\tresult_val = actual_mant_b - actual_mant_a;\n\t\t\t\tresult_sign = sign_b;\n\t\t\t}\n\t\t\treturn fi16_from_int(result_sign ? -result_val : result_val);\n\t\t}\n\t}\n\t\n\t// Align exponents\n\tuint16_t exp_result;\n\tif (exp_a > exp_b) {\n\t\tuint16_t shift = (exp_a > 0 ? exp_a - 1 : 0) - (exp_b > 0 ? exp_b - 1 : 0);\n\t\tactual_mant_b >>= shift;\n\t\texp_result = exp_a;\n\t} else if (exp_b > exp_a) {\n\t\tuint16_t shift = (exp_b > 0 ? exp_b - 1 : 0) - (exp_a > 0 ? exp_a - 1 : 0);\n\t\tactual_mant_a >>= shift;\n\t\texp_result = exp_b;\n\t} else {\n\t\texp_result = exp_a;\n\t}\n\t\n\t// Perform addition/subtraction based on signs\n\tuint32_t result_mant;\n\tuint16_t result_sign;\n\t\n\tif (sign_a == sign_b) {\n\t\tresult_mant = actual_mant_a + actual_mant_b;\n\t\tresult_sign = sign_a;\n\t} else {\n\t\tif (actual_mant_a >= actual_mant_b) {\n\t\t\tresult_mant = actual_mant_a - actual_mant_b;\n\t\t\tresult_sign = sign_a;\n\t\t} else {\n\t\t\tresult_mant = actual_mant_b - actual_mant_a;\n\t\t\tresult_sign = sign_b;\n\t\t}\n\t}\n\t\n\t// Convert back to actual value\n\tint result_val;\n\tif (exp_result == 0) {\n\t\tresult_val = result_mant;\n\t} else {\n\t\tresult_val = result_mant << (exp_result - 1);\n\t}\n\t\n\treturn fi16_from_int(result_sign ? -result_val : result_val);\n}\n\n";
    c_code = append_to_output(c_code, &c_code_length, extended_types);
    extended_types = "fint16_t fi16_sub(fint16_t fi16_a, fint16_t fi16_b) {\n\t// Negate fi16_b by flipping its sign bit\n\tfint16_t fi16_b_negated = fi16_b ^ 0x8000;\n\treturn fi16_add(fi16_a, fi16_b_negated);\n}\n\n";
    c_code = append_to_output(c_code, &c_code_length, extended_types);
    extended_types = "fint16_t fi16_mult(fint16_t fi16_a, fint16_t fi16_b) {\n\tconst int mbits = 10;\n\t\n\t// Handle zero\n\tif (fi16_a == 0 || fi16_b == 0) {\n\t\treturn 0;\n\t}\n\t\n\t// Extract fields\n\tuint16_t sign_a = (fi16_a >> 15) & 1;\n\tuint16_t exp_a = (fi16_a >> mbits) & 0x1F;\n\tuint16_t mant_a = fi16_a & 0x3FF;\n\t\n\tuint16_t sign_b = (fi16_b >> 15) & 1;\n\tuint16_t exp_b = (fi16_b >> mbits) & 0x1F;\n\tuint16_t mant_b = fi16_b & 0x3FF;\n\t\n\t// Result sign (XOR of input signs)\n\tuint16_t result_sign = sign_a ^ sign_b;\n\t\n\t// Get actual mantissa values and shifts\n\tuint32_t actual_mant_a, shift_a;\n\tif (exp_a == 0) {\n\t\tactual_mant_a = mant_a;\n\t\tshift_a = 0;\n\t} else {\n\t\tactual_mant_a = mant_a + (1 << mbits);\n\t\tshift_a = exp_a - 1;\n\t}\n\t\n\tuint32_t actual_mant_b, shift_b;\n\tif (exp_b == 0) {\n\t\tactual_mant_b = mant_b;\n\t\tshift_b = 0;\n\t} else {\n\t\tactual_mant_b = mant_b + (1 << mbits);\n\t\tshift_b = exp_b - 1;\n\t}\n\t\n\t// Multiply mantissas (32-bit intermediate to avoid overflow)\n\tuint32_t result_mant = actual_mant_a * actual_mant_b;\n\t\n\t// Combine shifts\n\tuint32_t total_shift = shift_a + shift_b;\n\t\n\t// Compute result value (may need long long for large shifts)\n\tlong long int result_val = ((long long int) result_mant) << total_shift;\n\t\n\treturn fi16_from_long_long(result_sign ? -result_val : result_val);\n}\n\n";
    c_code = append_to_output(c_code, &c_code_length, extended_types);
    extended_types = "fint16_t fi16_div(fint16_t fi16_a, fint16_t fi16_b) {\n\tconst int mbits = 10;\n\t\n\t// Handle division by zero - return max value with appropriate sign\n\tif (fi16_b == 0) {\n\t\treturn (fi16_a & 0x8000) ? 0xFFFF : 0x7FFF;\n\t}\n\t\n\t// Handle zero dividend\n\tif (fi16_a == 0) {\n\t\treturn 0;\n\t}\n\t\n\t// Extract fields\n\tuint16_t sign_a = (fi16_a >> 15) & 1;\n\tuint16_t exp_a = (fi16_a >> mbits) & 0x1F;\n\tuint16_t mant_a = fi16_a & 0x3FF;\n\t\n\tuint16_t sign_b = (fi16_b >> 15) & 1;\n\tuint16_t exp_b = (fi16_b >> mbits) & 0x1F;\n\tuint16_t mant_b = fi16_b & 0x3FF;\n\t\n\t// Result sign (XOR of input signs)\n\tuint16_t result_sign = sign_a ^ sign_b;\n\t\n\t// Get actual values\n\tint val_a, val_b;\n\tif (exp_a == 0) {\n\t\tval_a = mant_a;\n\t} else {\n\t\tval_a = (mant_a + (1 << mbits)) << (exp_a - 1);\n\t}\n\t\n\tif (exp_b == 0) {\n\t\tval_b = mant_b;\n\t} else {\n\t\tval_b = (mant_b + (1 << mbits)) << (exp_b - 1);\n\t}\n\t\n\t// Perform division\n\tif (val_b == 0) {\n\t\treturn result_sign ? 0xFFFF : 0x7FFF;\n\t}\n\t\n\tint result_val = val_a / val_b;\n\t\n\treturn fi16_from_int(result_sign ? -result_val : result_val);\n}\n\n";
    c_code = append_to_output(c_code, &c_code_length, extended_types);
    extended_types = "fint16_t fi16_neg(fint16_t x) { return x ^ 0x8000; }\nfint16_t fi16_abs(fint16_t x) { return x & 0x7FFF; }\n\n";
    c_code = append_to_output(c_code, &c_code_length, extended_types);

    // One last helper function to print out the registers
    c_code = append_to_output(c_code, &c_code_length, 
        "void print_registers(void) {\n"
        "\tprintf(\"    HEX      FLOAT     DOUBLE     LONG\\n\");"
        "\tprintf(\"r0: %.4x   %f    %f    %lld\\n\", r0, float_from_f16(r0), float_from_bf16(r0), long_long_from_fi16(r0));\n"
        "\tprintf(\"r1: %.4x   %f    %f    %lld\\n\", r1, float_from_f16(r1), float_from_bf16(r1), long_long_from_fi16(r1));\n"
        "\tprintf(\"r2: %.4x   %f    %f    %lld\\n\", r2, float_from_f16(r2), float_from_bf16(r2), long_long_from_fi16(r2));\n"
        "\tprintf(\"r3: %.4x   %f    %f    %lld\\n\", r3, float_from_f16(r3), float_from_bf16(r3), long_long_from_fi16(r3));\n"
        "\tprintf(\"sp: %.4x\\n\", sp);\n\n"
        "}\n"
    );


    // first prepass where we collect all labels that are used to call to, to define them beforehand. 
    int call_label_count = 0;
    char** call_label_collection = NULL;
    for (int i = 0; i < instruction_count; i++) {
        if (instruction[i].instruction == CALL) {
            // check if we already covered this label
            int unique = 1;
            for (int l = 0; l < call_label_count; l++) {
                if (strcmp(instruction[i].expression[1].tokens[0].raw, call_label_collection[l]) == 0) {
                    unique = 0;
                    break;
                }
            }
            if (!unique) continue;

            // if not covered yet, add function definition, 
            // but without the '.' prefix and instead a '_' prefix, 
            // just to keep them all unique to already existing functions
            sprintf(tmp, "void _%s(void);\n", instruction[i].expression[1].tokens[0].raw + 1);
            c_code = append_to_output(c_code, &c_code_length, tmp);

            call_label_collection = realloc(call_label_collection, sizeof(char*) * (call_label_count + 1));
            call_label_collection[call_label_count] = instruction[i].expression[1].tokens[0].raw;
            call_label_count += 1;
        }
    }
    c_code = append_to_output(c_code, &c_code_length, "\n");

    c_code = append_to_output(c_code, &c_code_length, "// This function contains the code that is run at the beginning of the execution\n");
    c_code = append_to_output(c_code, &c_code_length, "void preamble(void) {");


    // Now, generate the code until we stumble upon the first standalone label, which will mark a new function section
    int index = 0;
    while (index < instruction_count) {
        if (instruction[index].expression[0].type == EXPR_IMMEDIATE) {
            break;
        }
        // if not a label, generate the code
        memset(tmp, 0, sizeof(tmp));

        // first display the instruction that is being translated
        generate_asm_reconstruction(instruction[index], tmp);
        c_code = append_to_output(c_code, &c_code_length, "\t// ");
        c_code = append_to_output(c_code, &c_code_length, tmp);

        // then generate the actual C code
        // Important: During preamble, the HLT instruction will be interpreted as "return" to the main function, everywhere else, it will yield "exit(0)"
        if (instruction[index].instruction == HLT) {
            c_code = append_to_output(c_code, &c_code_length, "\n\treturn;\n");
            index ++;
            continue;
        }
        
        int err = generate_code_for_single_instruction(instruction[index], tmp, "\t");
        if (!err) {
            c_code = append_to_output(c_code, &c_code_length, "\n");
            c_code = append_to_output(c_code, &c_code_length, tmp);
            c_code = append_to_output(c_code, &c_code_length, "\n");
        } else {
            c_code = append_to_output(c_code, &c_code_length, "// UNKNOWN\n");
        }

        index ++;
    }

    // Exception is where no call is ever executed, then dont prematurely end the function body:
    if (call_label_count > 0) {
        c_code = append_to_output(c_code, &c_code_length, "}\n\n");
    }



    // And here we loop, generating the bodies of all subsequent functions
    int function_body_started = 0;
    while (index < instruction_count) {
        memset(tmp, 0, sizeof(tmp));
        if (instruction[index].expression[0].type == EXPR_IMMEDIATE) {
            // now be careful, ONLY start a new function, if there was a call to that lable at any point. 
            // Ignore if it's a pure jump label
            int unique = 1;
            for (int l = 0; l < call_label_count; l++) {
                if (strcmp(instruction[index].expression[0].tokens[0].raw, call_label_collection[l]) == 0) {
                    unique = 0;
                    break;
                }
            }
            if (!unique) {
                // OK, here is the exception: If a label was followed by another label, they should point to the same function. 
                // But in code the label is just left empty: 
                if (function_body_started == 1) {
                    // meaning a label was followed by a label again. Then call the other label in this function as a workaround. 
                    sprintf(tmp, "\t_%s();\n", instruction[index].expression[0].tokens[0].raw + 1);
                    c_code = append_to_output(c_code, &c_code_length, tmp);
                }

                // ending old function body if a body was started before
                if (function_body_started) {
                    c_code = append_to_output(c_code, &c_code_length, "}\n\n");
                }

                // generating new function body
                sprintf(tmp, "void _%s(void) {\n", instruction[index].expression[0].tokens[0].raw + 1);
                c_code = append_to_output(c_code, &c_code_length, tmp);
                function_body_started = 1;
                index ++;
                continue;
            }
        }
        // setting it to something else to denote that the body actually has content. 
        function_body_started = 2;

        // first display the instruction that is being translated
        generate_asm_reconstruction(instruction[index], tmp);
        c_code = append_to_output(c_code, &c_code_length, "\t// ");
        c_code = append_to_output(c_code, &c_code_length, tmp);

        // then generate the actual C code
        int err = generate_code_for_single_instruction(instruction[index], tmp, "\t");
        c_code = append_to_output(c_code, &c_code_length, "\n");
        if (!err) {
            c_code = append_to_output(c_code, &c_code_length, tmp);
            c_code = append_to_output(c_code, &c_code_length, "\n");
        } else {
            c_code = append_to_output(c_code, &c_code_length, "\t// UNKNOWN");
            c_code = append_to_output(c_code, &c_code_length, "\n");
        }

        index ++;
    }
    c_code = append_to_output(c_code, &c_code_length, "}\n\n");

    // lastly we generate the entry/setup point
    c_code = append_to_output(c_code, &c_code_length, 
        "int main(void) {\n"
        "\tr0 = 0x0000;\n"
        "\tr1 = 0x0000;\n"
        "\tr2 = 0x0000;\n"
        "\tr3 = 0x0000;\n"
        "\tsp = 0x7F00;\n\n"
        "\tpreamble();\n\n"
        "\tprint_registers();\n"
        "\treturn 0;\n"
        "}\n"
    );

    if (file_size) {*file_size = c_code_length;}

    return c_code;
}


char* transpile_from_file(const char* const filename, long* file_size) {
    char* asm = read_file(filename, NULL);
    char* reconstruction = transpile(asm, file_size);
    return reconstruction;
}


