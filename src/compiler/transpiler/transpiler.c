#include <stdio.h>
#include <string.h>

#include "Log.h"
#include "utils/IO.h"
#include "utils/String.h"

#include "compiler/asm/disassembler.h"
#include "compiler/asm/assembler.h"

#include "cpu/cpu_instructions.h"
#include "cpu/cpu_addressing_modes.h"


void generate_asm_reconstruction(Instruction_t instruction, char output[128]) {
    for (int e = 0; e < instruction.expression_count; e++) {
        for (int t = 0; t < instruction.expression[e].token_count; t++) {
            int len = strlen(output);
            sprintf(output + len, "%s ", instruction.expression[e].tokens[t].raw);
        }
    }
}

int generate_code_for_single_instruction(Instruction_t instruction, char output[128], char* prefix) {
    switch (instruction.instruction) {
        case 0: {
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
    
        case CMOVNL:
        case CMOVZ: {
            char flag[8] = {0};
            char not[8] = {0};
            switch (instruction.instruction) {
                case CMOVNL:
                    sprintf(flag, "sr.L");
                    not[0] = '\0';
                    break;
                
                case CMOVZ:
                    sprintf(flag, "sr.Z");
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
                    sprintf(output, "%ssp = %s%s ? rsp : ", prefix, not, flag);
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
                
                default:
                    log_msg(LP_ERROR, "Transpiler: \"%s\" with admr \"%s\" not handled [%s:%d]", 
                        instruction_encoding[instruction.instruction].instruction_string, 
                        cpu_reduced_addressing_mode_string[instruction.admr], __FILE__, __LINE__
                    );
                    return 1;
                    break;
            }
            switch (instruction.admx) {
                case ADMX_IND16:
                    sprintf(output + strlen(output), "0x%.4X;", parse_immediate(instruction.expression[2].tokens[1].raw));
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
            break;
        }
    
        case ADD: {
            switch (instruction.admr) {
                case ADMR_R0:
                    sprintf(output, "%sr0 += ", prefix);
                    break;

                case ADMR_R1:
                    sprintf(output, "%sr1 += ", prefix);
                    break;
                    
                case ADMR_R2:
                    sprintf(output, "%sr2 += ", prefix);
                    break;

                case ADMR_R3:
                    sprintf(output, "%sr3 += ", prefix);
                    break;

                case ADMR_SP:
                    sprintf(output, "%ssp += ", prefix);
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
    
        case SUB: {
            switch (instruction.admr) {
                case ADMR_R0:
                    sprintf(output, "%sr0 -= ", prefix);
                    break;

                case ADMR_R1:
                    sprintf(output, "%sr1 -= ", prefix);
                    break;

                case ADMR_R2:
                    sprintf(output, "%sr2 -= ", prefix);
                    break;

                case ADMR_R3:
                    sprintf(output, "%sr3 -= ", prefix);
                    break;

                case ADMR_SP:
                    sprintf(output, "%ssp -= ", prefix);
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
        }

        case CMP: {
            for (int i = 0; i < 9; i++) {
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
                            
                            default:
                                log_msg(LP_ERROR, "Transpiler: \"%s\" with admr \"%s\" not handled [%s:%d]", 
                                    instruction_encoding[instruction.instruction].instruction_string, 
                                    cpu_reduced_addressing_mode_string[instruction.admr], __FILE__, __LINE__
                                );
                                return 1;
                                break;
                        }
                        sprintf(output + strlen(output), "\n");
                        break;
                    }

                    case 1: {
                        // L
                        switch (instruction.admr) {
                            case ADMR_R0:
                                sprintf(output + strlen(output), "%ssr.L = (int16_t) r0 < ", prefix);
                                break;

                            case ADMR_R1:
                                sprintf(output, "%ssr.L = (int16_t) r0 < ", prefix);
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
                            
                            default:
                                log_msg(LP_ERROR, "Transpiler: \"%s\" with admr \"%s\" not handled [%s:%d]", 
                                    instruction_encoding[instruction.instruction].instruction_string, 
                                    cpu_reduced_addressing_mode_string[instruction.admr], __FILE__, __LINE__
                                );
                                return 1;
                                break;
                        }
                        sprintf(output + strlen(output), "\n");
                        break;
                    }

                    case 2: {
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
                            
                            default:
                                log_msg(LP_ERROR, "Transpiler: \"%s\" with admr \"%s\" not handled [%s:%d]", 
                                    instruction_encoding[instruction.instruction].instruction_string, 
                                    cpu_reduced_addressing_mode_string[instruction.admr], __FILE__, __LINE__
                                );
                                return 1;
                                break;
                        }
                        break;
                    }

                    default: 
                        break;
                }
            }
            break;
        }

        case TST: {
            for (int i = 0; i < 9; i++) {
                switch (i) {
                    case 0: {
                        // Z
                        switch (instruction.admx) {
                            case ADMX_R0:
                                sprintf(output, "%ssr.Z = r0 == 0x0000;", prefix);
                                break;

                            case ADMX_R1:
                                sprintf(output, "%ssr.Z = r1 == 0x0000;", prefix);
                                break;
                            
                            default:
                                log_msg(LP_ERROR, "Transpiler: \"%s\" with admx \"%s\" not handled [%s:%d]", 
                                    instruction_encoding[instruction.instruction].instruction_string, 
                                    cpu_extended_addressing_mode_string[instruction.admr], __FILE__, __LINE__
                                );
                                return 1;
                                break;
                        }
                        sprintf(output + strlen(output), "\n");
                        break;
                    }

                    case 1: {
                        // L
                        switch (instruction.admx) {
                            case ADMX_R0:
                                sprintf(output + strlen(output), "%ssr.L = (int16_t) r0 < 0x0000;", prefix);
                                break;

                            case ADMX_R1:
                                sprintf(output + strlen(output), "%ssr.L = (int16_t) r1 < 0x0000;", prefix);
                                break;
                            
                            default:
                                log_msg(LP_ERROR, "Transpiler: \"%s\" with admx \"%s\" not handled [%s:%d]", 
                                    instruction_encoding[instruction.instruction].instruction_string, 
                                    cpu_extended_addressing_mode_string[instruction.admr], __FILE__, __LINE__
                                );
                                return 1;
                                break;
                        }
                        sprintf(output + strlen(output), "\n");
                        break;
                    }

                    case 2: {
                        // UL
                        sprintf(output + strlen(output), "%ssr.UL = 0;", prefix);
                        break;
                    }

                    default: 
                        break;
                }
            }
            break;
        }

        case JNZ: {
            switch (instruction.admx) {
                case ADMX_IMM16:
                    sprintf(output, "%sif (!sr.Z) goto _%s;", prefix, instruction.expression[1].tokens[0].raw + 1);
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
            sprintf(output, "%sexit(0);", prefix);
            break;
        }
        
        default:
            log_msg(LP_ERROR, "Transpiler: Instruction \"%s\" not handled [%s:%d]", instruction_encoding[instruction.instruction].instruction_string, __FILE__, __LINE__);
            return 1;
            break;
    }
    return 0;
}


char* transpile(uint8_t* bin, long binary_size, long* file_size) {

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

    for (int i = 0; i < instruction_count; i++) {
        //log_msg(LP_DEBUG, "%d", instruction[i].is_address);
    }

    // resolve label
    instruction = assembler_resolve_labels(instruction, instruction_count);

    char* c_code = NULL;
    long c_code_length = 0;

    c_code = append_to_output(c_code, &c_code_length, 
        "#include <stdint.h>\n"
        "#include <stdio.h>\n\n"
        "static uint8_t ram[65536] = {0};\n"
        "static uint16_t r0, r1, r2, r3, sp;\n\n"
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
        "// Todo: write the actual binary data into ram for authenticity\n\n"
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
            char tmp[128];
            sprintf(tmp, "void _%s(void);\n", instruction[i].expression[1].tokens[0].raw + 1);
            c_code = append_to_output(c_code, &c_code_length, tmp);

            call_label_collection = realloc(call_label_collection, sizeof(char*) * (call_label_count + 1));
            call_label_collection[call_label_count] = instruction[i].expression[1].tokens[0].raw;
            call_label_count += 1;
        }
    }
    c_code = append_to_output(c_code, &c_code_length, "\n");

    c_code = append_to_output(c_code, &c_code_length, "// This function contains the code that is run at the beginning of the execution\n");
    c_code = append_to_output(c_code, &c_code_length, "void preamble(void) {\n");


    // Now, generate the code until we stumble upon the first standalone label, which will mark a new function section
    int index = 0;
    while (index < instruction_count) {
        if (instruction[index].expression[0].type == EXPR_IMMEDIATE) {
            break;
        }
        // if not a label, generate the code
        log_msg(LP_DEBUG, "%s", instruction_encoding[instruction[index].instruction].instruction_string);
        char tmp[128] = {0};

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
            c_code = append_to_output(c_code, &c_code_length, "\n");
        }

        index ++;
    }
    c_code = append_to_output(c_code, &c_code_length, "}\n\n");



    // And here we loop, generating the bodies of all subsequent functions
    int function_body_started = 0;
    while (index < instruction_count) {
        char tmp[128] = {0};
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

        // first display the instruction that is being translated
        generate_asm_reconstruction(instruction[index], tmp);
        c_code = append_to_output(c_code, &c_code_length, "\t// ");
        c_code = append_to_output(c_code, &c_code_length, tmp);

        // then generate the actual C code
        int err = generate_code_for_single_instruction(instruction[index], tmp, "\t");
        if (!err) {
            c_code = append_to_output(c_code, &c_code_length, "\n");
            c_code = append_to_output(c_code, &c_code_length, tmp);
            c_code = append_to_output(c_code, &c_code_length, "\n");
        } else {
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
        "\tprintf(\"r0: %.4x\\n\", r0);\n"
        "\tprintf(\"r1: %.4x\\n\", r1);\n"
        "\tprintf(\"r2: %.4x\\n\", r2);\n"
        "\tprintf(\"r3: %.4x\\n\", r3);\n"
        "\tprintf(\"sp: %.4x\\n\", sp);\n\n"
        "\treturn 0;\n"
        "}\n"
    );

    if (file_size) {*file_size = c_code_length;}

    return c_code;
}


char* transpile_from_file(const char* const filename, long* filesize) {
    long binary_size = 0;
    uint8_t* bin = (uint8_t*)read_file(filename, &binary_size);
    char* reconstruction = transpile(bin, binary_size, filesize);
    return reconstruction;
}


