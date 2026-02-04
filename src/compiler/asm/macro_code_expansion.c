#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>

#include "utils/Log.h"
#include "utils/IO.h"
#include "utils/String.h"

#include "cpu/cpu_features.h"
#include "cpu/cpu_addressing_modes.h"
#include "cpu/cpu_instructions.h"

#include "compiler/asm/assembler.h"

#include "compiler/asm/macro_code_expansion.h"


#define DUMP_INTERMEDIATE
#undef DUMP_INTERMEDIATE


static CPU_EXTENDED_ADDRESSING_MODE_t get_equal_admx_from_admr(CPU_REDUCED_ADDRESSING_MODE_t admr) {
    switch(admr) {
        case ADMR_IND16:
            return ADMX_IND16;
        case ADMR_R0:
        case ADMR_R1:
        case ADMR_R2:
        case ADMR_R3:
        case ADMR_SP:
            return ADMX_R0 + admr - ADMR_R0;
        case ADMR_IND_R0:
            return ADMX_IND_R0;
        default:
            return ADMX_NONE;
    }
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

char* macro_code_expand(char* content, CpuFeatureFlag_t feature_flag) {

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
    int label_uid = 0;
    const char* label_prefix = ".L_macro_expansion_uid_";

    #ifdef DUMP_INTERMEDIATE
        int interation = 0;
    #endif

    while (changes_applied) {
        changes_applied = 0;
        /*for (int j = 0; j < instruction_count; j++) {
            if (instruction[j].instruction < 0 || instruction[j].instruction >= INSTRUCTION_COUNT) continue;
            printf("Instruction %d: \"%s\" %s, %s\n", j + 1, cpu_instruction_string[instruction[j].instruction], cpu_reduced_addressing_mode_string[instruction[j].admr], cpu_extended_addressing_mode_string[instruction[j].admx]);
        }*/


        #ifdef DUMP_INTERMEDIATE
            //temporary reconstruction
            char* output = NULL;         // final assembly string
            long output_len = 0;         // current length of code output
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
            char filename[64];
            sprintf(filename, "reco_%d.asm", interation++);
            data_export(filename, output, output_len);
        #endif


        // Expand
        for (int i = 0; i < instruction_count; i++) {

            if (!(feature_flag & CFF_INT_ARITH_EXT)) {
                // replace mul instruction with simulated mul using repeated addition
                if (instruction[i].instruction == MUL) {
                    if (instruction[i].admr != ADMR_R0 && 
                        instruction[i].admr != ADMR_R1 && 
                        instruction[i].admr != ADMR_IND_R0) {
                            Expression_t admr_expr = instruction[i].expression[1];
                            Expression_t admx_expr = instruction[i].expression[2];
                            CPU_REDUCED_ADDRESSING_MODE_t admr = instruction[i].admr;
                            CPU_EXTENDED_ADDRESSING_MODE_t admx = instruction[i].admx;

                            remove_instruction(instruction, &instruction_count, i);

                            // push r0
                            Instruction_t push_r0 = {
                                .instruction = PUSH, 
                                .admr = ADMR_NONE, 
                                .admx = ADMX_R0,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "push"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, push_r0, &instruction_count, i++);
                            
                            // push r1
                            Instruction_t push_r1 = {
                                .instruction = PUSH, 
                                .admr = ADMR_NONE, 
                                .admx = ADMX_R1,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "push"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r1"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, push_r1, &instruction_count, i++);

                            // mov r0, admx
                            Instruction_t mov_r0_admx = {
                                .instruction = MOV, 
                                .admr = ADMR_R0, 
                                .admx = admx,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "mov"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                    admx_expr
                                }
                            };
                            insert_instruction(&instruction, mov_r0_admx, &instruction_count, i++);

                            // mov r1, admr
                            Instruction_t mov_r1_admr = {
                                .instruction = MOV, 
                                .admr = ADMR_R0, 
                                .admx = admx,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "mov"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r1"
                                            }
                                        }, 
                                    }, 
                                    admr_expr
                                }
                            };
                            insert_instruction(&instruction, mov_r1_admr, &instruction_count, i++);

                            // mov admr, 0
                            Instruction_t mov_admr_0 = {
                                .instruction = MOV, 
                                .admr = ADMR_R0, 
                                .admx = admx,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "mov"
                                            }
                                        }, 
                                    }, 
                                    admr_expr, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_IMMEDIATE, 
                                                .raw = "$0000"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, mov_admr_0, &instruction_count, i++);

                            // push r0
                            insert_instruction(&instruction, push_r0, &instruction_count, i++);

                            // push r1
                            insert_instruction(&instruction, push_r1, &instruction_count, i++);

                            // abs r0
                            Instruction_t abs_r0 = {
                                .instruction = ABS, 
                                .admr = ADMR_R0, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "abs"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, abs_r0, &instruction_count, i++);

                            // abs r1
                            Instruction_t abs_r1 = {
                                .instruction = ABS, 
                                .admr = ADMR_R1, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "abs"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r1"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, abs_r1, &instruction_count, i++);

                            // cmp r1, r0
                            Instruction_t cmp_r1_r0 = {
                                .instruction = CMP, 
                                .admr = ADMR_R1, 
                                .admx = ADMX_R0,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "cmp"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r1"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, cmp_r1_r0, &instruction_count, i++);

                            // jnl .L_dont_swap
                            char* label_L_dont_swap = malloc(64);
                            sprintf(label_L_dont_swap, "%s%d", label_prefix, label_uid);
                            Instruction_t jnl_dont_swap = {
                                .instruction = JNL, 
                                .admr = ADMR_NONE, 
                                .admx = ADMX_IMM16,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "jnl"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = label_L_dont_swap
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, jnl_dont_swap, &instruction_count, i++);

                            // pop r0
                            Instruction_t pop_r0 = {
                                .instruction = POP, 
                                .admr = ADMR_R0, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "pop"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, pop_r0, &instruction_count, i++);

                            // pop r1
                            Instruction_t pop_r1 = {
                                .instruction = POP, 
                                .admr = ADMR_R1, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "pop"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r1"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, pop_r1, &instruction_count, i++);

                            // jmp .L_post_pop
                            char* label_L_post_pop = malloc(64);
                            sprintf(label_L_post_pop, "%s%d", label_prefix, label_uid + 1);
                            Instruction_t jmp_post_pop = {
                                .instruction = JMP, 
                                .admr = ADMR_NONE, 
                                .admx = ADMX_IMM16,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "jmp"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = label_L_post_pop
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, jmp_post_pop, &instruction_count, i++);

                            // .L_dont_swap
                            Instruction_t dont_swap = {
                                .admr = ADMR_NONE, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 1, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = label_L_dont_swap
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, dont_swap, &instruction_count, i++);

                            // pop r1
                            insert_instruction(&instruction, pop_r1, &instruction_count, i++);

                            // pop r0
                            insert_instruction(&instruction, pop_r0, &instruction_count, i++);

                            // .L_post_pop
                            Instruction_t post_pop = {
                                .admr = ADMR_NONE, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 1, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = label_L_post_pop
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, post_pop, &instruction_count, i++);

                            // cmp r0, 0
                            Instruction_t cmp_r0_0 = {
                                .instruction = CMP, 
                                .admr = ADMR_R0, 
                                .admx = ADMX_IMM16,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "cmp"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_IMMEDIATE, 
                                                .raw = "$0000"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, cmp_r0_0, &instruction_count, i++);

                            // jz .L_done
                            char* label_L_done = malloc(64);
                            sprintf(label_L_done, "%s%d", label_prefix, label_uid + 2);
                            Instruction_t jz_done = {
                                .instruction = JZ, 
                                .admr = ADMR_NONE, 
                                .admx = ADMX_IMM16,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "jz"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = label_L_done
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, jz_done, &instruction_count, i++);

                            // jnl .L
                            char* label_L = malloc(64);
                            sprintf(label_L, "%s%d", label_prefix, label_uid + 3);
                            Instruction_t jnl = {
                                .instruction = JNL, 
                                .admr = ADMR_NONE, 
                                .admx = ADMX_IMM16,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "jnl"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = label_L
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, jnl, &instruction_count, i++);

                            // neg r0
                            Instruction_t neg_r0 = {
                                .instruction = NEG, 
                                .admr = ADMR_R0, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "neg"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, neg_r0, &instruction_count, i++);

                            // neg r1
                            Instruction_t neg_r1 = {
                                .instruction = NEG, 
                                .admr = ADMR_R1, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "neg"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r1"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, neg_r1, &instruction_count, i++);

                            // .L
                            Instruction_t L = {
                                .admr = ADMR_NONE, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 1, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = label_L
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, L, &instruction_count, i++);

                            // add admr, r1
                            Instruction_t add_admr_r1 = {
                                .instruction = ADD, 
                                .admr = admr, 
                                .admx = ADMX_R1,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "add"
                                            }
                                        }, 
                                    }, 
                                    admr_expr, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r1"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, add_admr_r1, &instruction_count, i++);

                            // dec r0
                            Instruction_t dec_r0 = {
                                .instruction = DEC, 
                                .admr = ADMR_R0, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "dec"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, dec_r0, &instruction_count, i++);

                            // jnz .L
                            Instruction_t jnz = {
                                .instruction = JNZ, 
                                .admr = ADMR_NONE, 
                                .admx = ADMX_IMM16,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "jnz"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = label_L
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, jnz, &instruction_count, i++);

                            // .L_done
                            Instruction_t done = {
                                .admr = ADMR_NONE, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 1, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = label_L_done
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, done, &instruction_count, i++);

                            // pop r1
                            insert_instruction(&instruction, pop_r1, &instruction_count, i++);

                            // pop r0
                            insert_instruction(&instruction, pop_r0, &instruction_count, i++);
                    
                            // tst admr
                            Instruction_t tst_admr = {
                                .instruction = TST, 
                                .admr = admr, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "tst"
                                            }
                                        }, 
                                    }, 
                                    admr_expr
                                }
                            };
                            insert_instruction(&instruction, tst_admr, &instruction_count, i++);

                            label_uid += 4;
                            changes_applied = 1;
                        } else {
                            Expression_t admr_expr = instruction[i].expression[1];
                            Expression_t admx_expr = instruction[i].expression[2];
                            CPU_REDUCED_ADDRESSING_MODE_t admr = instruction[i].admr;
                            CPU_EXTENDED_ADDRESSING_MODE_t admx = instruction[i].admx;

                            remove_instruction(instruction, &instruction_count, i);

                            // push r2
                            Instruction_t push_r2 = {
                                .instruction = PUSH, 
                                .admr = ADMR_NONE, 
                                .admx = ADMX_R0,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "push"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r2"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, push_r2, &instruction_count, i++);
                            
                            // push r3
                            Instruction_t push_r3 = {
                                .instruction = PUSH, 
                                .admr = ADMR_NONE, 
                                .admx = ADMX_R1,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "push"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r3"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, push_r3, &instruction_count, i++);

                            // mov r2, admx
                            Instruction_t mov_r2_admx = {
                                .instruction = MOV, 
                                .admr = ADMR_R0, 
                                .admx = admx,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "mov"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r2"
                                            }
                                        }, 
                                    }, 
                                    admx_expr
                                }
                            };
                            insert_instruction(&instruction, mov_r2_admx, &instruction_count, i++);

                            // mov r3, admr
                            Instruction_t mov_r3_admr = {
                                .instruction = MOV, 
                                .admr = ADMR_R0, 
                                .admx = admx,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "mov"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r3"
                                            }
                                        }, 
                                    }, 
                                    admr_expr
                                }
                            };
                            insert_instruction(&instruction, mov_r3_admr, &instruction_count, i++);

                            // mov admr, 0
                            Instruction_t mov_admr_0 = {
                                .instruction = MOV, 
                                .admr = ADMR_R0, 
                                .admx = admx,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "mov"
                                            }
                                        }, 
                                    }, 
                                    admr_expr, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_IMMEDIATE, 
                                                .raw = "$0000"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, mov_admr_0, &instruction_count, i++);

                            // push r2
                            insert_instruction(&instruction, push_r2, &instruction_count, i++);

                            // push r3
                            insert_instruction(&instruction, push_r3, &instruction_count, i++);

                            // abs r2
                            Instruction_t abs_r2 = {
                                .instruction = ABS, 
                                .admr = ADMR_R0, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "abs"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r2"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, abs_r2, &instruction_count, i++);

                            // abs r3
                            Instruction_t abs_r3 = {
                                .instruction = ABS, 
                                .admr = ADMR_R1, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "abs"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r3"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, abs_r3, &instruction_count, i++);

                            // cmp r3, r2
                            Instruction_t cmp_r3_r2 = {
                                .instruction = CMP, 
                                .admr = ADMR_R1, 
                                .admx = ADMX_R0,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "cmp"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r3"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r2"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, cmp_r3_r2, &instruction_count, i++);

                            // jnl .L_dont_swap
                            char* label_L_dont_swap = malloc(64);
                            sprintf(label_L_dont_swap, "%s%d", label_prefix, label_uid);
                            Instruction_t jnl_dont_swap = {
                                .instruction = JNL, 
                                .admr = ADMR_NONE, 
                                .admx = ADMX_IMM16,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "jnl"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = label_L_dont_swap
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, jnl_dont_swap, &instruction_count, i++);

                            // pop r2
                            Instruction_t pop_r2 = {
                                .instruction = POP, 
                                .admr = ADMR_R0, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "pop"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r2"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, pop_r2, &instruction_count, i++);

                            // pop r3
                            Instruction_t pop_r3 = {
                                .instruction = POP, 
                                .admr = ADMR_R1, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "pop"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r3"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, pop_r3, &instruction_count, i++);

                            // jmp .L_post_pop
                            char* label_L_post_pop = malloc(64);
                            sprintf(label_L_post_pop, "%s%d", label_prefix, label_uid + 1);
                            Instruction_t jmp_post_pop = {
                                .instruction = JMP, 
                                .admr = ADMR_NONE, 
                                .admx = ADMX_IMM16,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "jmp"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = label_L_post_pop
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, jmp_post_pop, &instruction_count, i++);

                            // .L_dont_swap
                            Instruction_t dont_swap = {
                                .admr = ADMR_NONE, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 1, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = label_L_dont_swap
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, dont_swap, &instruction_count, i++);

                            // pop r3
                            insert_instruction(&instruction, pop_r3, &instruction_count, i++);

                            // pop r2
                            insert_instruction(&instruction, pop_r2, &instruction_count, i++);

                            // .L_post_pop
                            Instruction_t post_pop = {
                                .admr = ADMR_NONE, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 1, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = label_L_post_pop
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, post_pop, &instruction_count, i++);

                            // cmp r2, 0
                            Instruction_t cmp_r2_0 = {
                                .instruction = CMP, 
                                .admr = ADMR_R0, 
                                .admx = ADMX_IMM16,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "cmp"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r2"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_IMMEDIATE, 
                                                .raw = "$0000"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, cmp_r2_0, &instruction_count, i++);

                            // jz .L_done
                            char* label_L_done = malloc(64);
                            sprintf(label_L_done, "%s%d", label_prefix, label_uid + 2);
                            Instruction_t jz_done = {
                                .instruction = JZ, 
                                .admr = ADMR_NONE, 
                                .admx = ADMX_IMM16,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "jz"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = label_L_done
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, jz_done, &instruction_count, i++);

                            // jnl .L
                            char* label_L = malloc(64);
                            sprintf(label_L, "%s%d", label_prefix, label_uid + 3);
                            Instruction_t jnl = {
                                .instruction = JNL, 
                                .admr = ADMR_NONE, 
                                .admx = ADMX_IMM16,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "jnl"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = label_L
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, jnl, &instruction_count, i++);

                            // neg r2
                            Instruction_t neg_r2 = {
                                .instruction = NEG, 
                                .admr = ADMR_R0, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "neg"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r2"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, neg_r2, &instruction_count, i++);

                            // neg r3
                            Instruction_t neg_r3 = {
                                .instruction = NEG, 
                                .admr = ADMR_R1, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "neg"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r3"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, neg_r3, &instruction_count, i++);

                            // .L
                            Instruction_t L = {
                                .admr = ADMR_NONE, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 1, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = label_L
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, L, &instruction_count, i++);

                            // add admr, r3
                            Instruction_t add_admr_r3 = {
                                .instruction = ADD, 
                                .admr = admr, 
                                .admx = ADMX_R1,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "add"
                                            }
                                        }, 
                                    }, 
                                    admr_expr, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r3"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, add_admr_r3, &instruction_count, i++);

                            // dec r2
                            Instruction_t dec_r2 = {
                                .instruction = DEC, 
                                .admr = ADMR_R0, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "dec"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r2"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, dec_r2, &instruction_count, i++);

                            // jnz .L
                            Instruction_t jnz = {
                                .instruction = JNZ, 
                                .admr = ADMR_NONE, 
                                .admx = ADMX_IMM16,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "jnz"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = label_L
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, jnz, &instruction_count, i++);

                            // .L_done
                            Instruction_t done = {
                                .admr = ADMR_NONE, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 1, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = label_L_done
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, done, &instruction_count, i++);

                            // pop r3
                            insert_instruction(&instruction, pop_r3, &instruction_count, i++);

                            // pop r2
                            insert_instruction(&instruction, pop_r2, &instruction_count, i++);
                    
                            // tst admr
                            Instruction_t tst_admr = {
                                .instruction = TST, 
                                .admr = admr, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "tst"
                                            }
                                        }, 
                                    }, 
                                    admr_expr
                                }
                            };
                            insert_instruction(&instruction, tst_admr, &instruction_count, i++);

                            label_uid += 4;
                            changes_applied = 1;
                    }
                }
                if (changes_applied) break;
            } // CFF_INT_ARITH_EXT


            if (!(feature_flag & CFF_INT_ARITH)) {
                if (instruction[i].instruction == ADD) {
                    Expression_t admr_expr = instruction[i].expression[1];
                    Expression_t admx_expr = instruction[i].expression[2];
                    CPU_REDUCED_ADDRESSING_MODE_t admr = instruction[i].admr;
                    CPU_EXTENDED_ADDRESSING_MODE_t admx = instruction[i].admx;
                    if (instruction[i].admr != ADMR_R0 && 
                        instruction[i].admr != ADMR_IND_R0 &&
                        instruction[i].admr != ADMR_SP
                    ) {
                        if (!is_register_admx(instruction[i].admx) &&
                            instruction[i].admx != ADMX_IND_R0 &&
                            instruction[i].admx != ADMX_IND_R0_OFFSET16 && 
                            instruction[i].admx != ADMX_IND16_SCALED8_R0_OFFSET &&
                            instruction[i].admx != ADMX_IMM16
                        ) {
                            remove_instruction(instruction, &instruction_count, i);

                            // push r0
                            Instruction_t push_r0 = {
                                .instruction = PUSH, 
                                .admr = ADMR_NONE, 
                                .admx = ADMX_R0,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "push"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, push_r0, &instruction_count, i++);

                            // push admx
                            Instruction_t push_admx = {
                                .instruction = PUSH, 
                                .admr = ADMR_NONE, 
                                .admx = admx,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "push"
                                            }
                                        }, 
                                    }, 
                                    admx_expr
                                }
                            };
                            insert_instruction(&instruction, push_admx, &instruction_count, i++);

                            // push $8000
                            Instruction_t push_0x8000 = {
                                .instruction = PUSH, 
                                .admr = ADMR_NONE, 
                                .admx = admx,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "push"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_IMMEDIATE, 
                                                .raw = "$8000"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, push_0x8000, &instruction_count, i++);

                            // .L
                            char* label_L = malloc(64);
                            sprintf(label_L, "%s%d", label_prefix, label_uid);
                            Instruction_t L = {
                                .instruction = NOP, 
                                .admr = ADMR_NONE, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 1, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = label_L
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, L, &instruction_count, i++);

                            // mov r0, admr
                            Instruction_t mov_r0_admr = {
                                .instruction = MOV, 
                                .admr = ADMR_R0, 
                                .admx = get_equal_admx_from_admr(admr),
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "mov"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                    admr_expr,
                                }
                            };
                            insert_instruction(&instruction, mov_r0_admr, &instruction_count, i++);

                            // xor r0, admx
                            Instruction_t xor_r0_admx = {
                                .instruction = XOR, 
                                .admr = ADMR_R0, 
                                .admx = admx,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "xor"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                    admx_expr,
                                }
                            };
                            insert_instruction(&instruction, xor_r0_admx, &instruction_count, i++);

                            // push r0
                            insert_instruction(&instruction, push_r0, &instruction_count, i++);

                            // lea r0, admx
                            Instruction_t lea_r0_admx = {
                                .instruction = LEA, 
                                .admr = ADMR_R0, 
                                .admx = admx,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "lea"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                    admx_expr,
                                }
                            };
                            insert_instruction(&instruction, lea_r0_admx, &instruction_count, i++);

                            // and [r0], admr
                            Instruction_t and_indr0_admr = {
                                .instruction = AND, 
                                .admr = ADMR_IND_R0, 
                                .admx = get_equal_admx_from_admr(admr),
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "and"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 3, 
                                        .type = EXPR_INDIRECT_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_BRACKET_OPEN, 
                                                .raw = "["
                                            }, 
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }, 
                                            (Token_t) {
                                                .type = TT_BRACKET_CLOSE, 
                                                .raw = "]"
                                            }
                                        }, 
                                    }, 
                                    admr_expr,
                                }
                            };
                            insert_instruction(&instruction, and_indr0_admr, &instruction_count, i++);

                            // bws [r0], admr
                            Instruction_t bws_indr0_0xffff = {
                                .instruction = BWS, 
                                .admr = ADMR_IND_R0, 
                                .admx = ADMX_IMM16,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "bws"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 3, 
                                        .type = EXPR_INDIRECT_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_BRACKET_OPEN, 
                                                .raw = "["
                                            }, 
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }, 
                                            (Token_t) {
                                                .type = TT_BRACKET_CLOSE, 
                                                .raw = "]"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_IMMEDIATE, 
                                                .raw = "$FFFF"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, bws_indr0_0xffff, &instruction_count, i++);

                            // pop r0
                            Instruction_t pop_r0 = {
                                .instruction = POP, 
                                .admr = ADMR_R0, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "pop"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, pop_r0, &instruction_count, i++);

                            // mov admr, r0
                            Instruction_t mov_admr_r0 = {
                                .instruction = MOV, 
                                .admr = admr, 
                                .admx = ADMX_R0,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "mov"
                                            }
                                        }, 
                                    }, 
                                    admr_expr,
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, mov_admr_r0, &instruction_count, i++);

                            // pop r0
                            insert_instruction(&instruction, pop_r0, &instruction_count, i++);

                            // bws r0, 1
                            Instruction_t bws_r0_1 = {
                                .instruction = BWS, 
                                .admr = ADMR_R0, 
                                .admx = ADMX_IMM16,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "bws"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_IMMEDIATE, 
                                                .raw = "$0001"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, bws_r0_1, &instruction_count, i++);

                            // tst r0
                            Instruction_t tst_r0 = {
                                .instruction = TST, 
                                .admr = ADMR_R0, 
                                .admx = ADMX_IMM16,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "tst"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, tst_r0, &instruction_count, i++);

                            // push r0
                            insert_instruction(&instruction, push_r0, &instruction_count, i++);

                            // jnz .L
                            Instruction_t jnz_L = {
                                .instruction = JNZ, 
                                .admr = ADMR_NONE, 
                                .admx = ADMX_IMM16,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "jnz"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = label_L
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, jnz_L, &instruction_count, i++);

                            // pop r0
                            insert_instruction(&instruction, pop_r0, &instruction_count, i++);

                            // lea r0, admx
                            insert_instruction(&instruction, lea_r0_admx, &instruction_count, i++);

                            // pop [r0]
                            Instruction_t pop_indr0 = {
                                .instruction = POP, 
                                .admr = ADMR_IND_R0, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "pop"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 3, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_BRACKET_OPEN, 
                                                .raw = "["
                                            }, 
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }, 
                                            (Token_t) {
                                                .type = TT_BRACKET_CLOSE, 
                                                .raw = "]"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, pop_indr0, &instruction_count, i++);

                            // pop r0
                            insert_instruction(&instruction, pop_r0, &instruction_count, i++);
                            
                            // tst admr
                            Instruction_t tst_admr = {
                                .instruction = TST, 
                                .admr = admr, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "tst"
                                            }
                                        }, 
                                    }, 
                                    admr_expr
                                }
                            };
                            insert_instruction(&instruction, tst_admr, &instruction_count, i++);

                            label_uid += 1;
                            changes_applied = 1;

                        } else if (is_register_admx(instruction[i].admx)) {

                            remove_instruction(instruction, &instruction_count, i);

                            // push r0
                            Instruction_t push_r0 = {
                                .instruction = PUSH, 
                                .admr = ADMR_NONE, 
                                .admx = ADMX_R0,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "push"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, push_r0, &instruction_count, i++);

                            // push admx
                            Instruction_t push_admx = {
                                .instruction = PUSH, 
                                .admr = ADMR_NONE, 
                                .admx = admx,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "push"
                                            }
                                        }, 
                                    }, 
                                    admx_expr
                                }
                            };
                            insert_instruction(&instruction, push_admx, &instruction_count, i++);

                            // push $8000
                            Instruction_t push_0x8000 = {
                                .instruction = PUSH, 
                                .admr = ADMR_NONE, 
                                .admx = admx,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "push"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_IMMEDIATE, 
                                                .raw = "$8000"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, push_0x8000, &instruction_count, i++);

                            // .L
                            char* label_L = malloc(64);
                            sprintf(label_L, "%s%d", label_prefix, label_uid);
                            Instruction_t L = {
                                .instruction = NOP, 
                                .admr = ADMR_NONE, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 1, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = label_L
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, L, &instruction_count, i++);

                            // mov r0, admr
                            Instruction_t mov_r0_admr = {
                                .instruction = MOV, 
                                .admr = ADMR_R0, 
                                .admx = get_equal_admx_from_admr(admr),
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "mov"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                    admr_expr,
                                }
                            };
                            insert_instruction(&instruction, mov_r0_admr, &instruction_count, i++);

                            // xor r0, admx
                            Instruction_t xor_r0_admx = {
                                .instruction = XOR, 
                                .admr = ADMR_R0, 
                                .admx = admx,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "xor"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                    admx_expr,
                                }
                            };
                            insert_instruction(&instruction, xor_r0_admx, &instruction_count, i++);

                            // push r0
                            insert_instruction(&instruction, push_r0, &instruction_count, i++);

                            // and admx, admr
                            Instruction_t and_indr0_admr = {
                                .instruction = AND, 
                                .admr = ADMR_IND_R0, 
                                .admx = get_equal_admx_from_admr(admr),
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "and"
                                            }
                                        }, 
                                    }, 
                                    admx_expr, 
                                    admr_expr,
                                }
                            };
                            insert_instruction(&instruction, and_indr0_admr, &instruction_count, i++);

                            // bws admx, $ffff
                            Instruction_t bws_indr0_0xffff = {
                                .instruction = BWS, 
                                .admr = ADMR_IND_R0, 
                                .admx = ADMX_IMM16,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "bws"
                                            }
                                        }, 
                                    }, 
                                    admx_expr, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_IMMEDIATE, 
                                                .raw = "$FFFF"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, bws_indr0_0xffff, &instruction_count, i++);

                            // pop r0
                            Instruction_t pop_r0 = {
                                .instruction = POP, 
                                .admr = ADMR_R0, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "pop"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, pop_r0, &instruction_count, i++);

                            // mov admr, r0
                            Instruction_t mov_admr_r0 = {
                                .instruction = MOV, 
                                .admr = admr, 
                                .admx = ADMX_R0,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "mov"
                                            }
                                        }, 
                                    }, 
                                    admr_expr,
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, mov_admr_r0, &instruction_count, i++);

                            // pop r0
                            insert_instruction(&instruction, pop_r0, &instruction_count, i++);

                            // bws r0, 1
                            Instruction_t bws_r0_1 = {
                                .instruction = BWS, 
                                .admr = ADMR_R0, 
                                .admx = ADMX_IMM16,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "bws"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_IMMEDIATE, 
                                                .raw = "$0001"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, bws_r0_1, &instruction_count, i++);

                            // tst r0
                            Instruction_t tst_r0 = {
                                .instruction = TST, 
                                .admr = ADMR_R0, 
                                .admx = ADMX_IMM16,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "tst"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, tst_r0, &instruction_count, i++);

                            // push r0
                            insert_instruction(&instruction, push_r0, &instruction_count, i++);

                            // jnz .L
                            Instruction_t jnz_L = {
                                .instruction = JNZ, 
                                .admr = ADMR_NONE, 
                                .admx = ADMX_IMM16,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "jnz"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = label_L
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, jnz_L, &instruction_count, i++);

                            // pop r0
                            insert_instruction(&instruction, pop_r0, &instruction_count, i++);

                            // pop admx
                            Instruction_t pop_indr0 = {
                                .instruction = POP, 
                                .admr = ADMR_IND_R0, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "pop"
                                            }
                                        }, 
                                    }, 
                                    admx_expr
                                }
                            };
                            insert_instruction(&instruction, pop_indr0, &instruction_count, i++);

                            // pop r0
                            insert_instruction(&instruction, pop_r0, &instruction_count, i++);

                            // tst admr
                            Instruction_t tst_admr = {
                                .instruction = TST, 
                                .admr = admr, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "tst"
                                            }
                                        }, 
                                    }, 
                                    admr_expr
                                }
                            };
                            insert_instruction(&instruction, tst_admr, &instruction_count, i++);

                            label_uid += 1;
                            changes_applied = 1;

                        } else if (instruction[i].admx == ADMX_IMM16) {

                            remove_instruction(instruction, &instruction_count, i);

                            // push r0
                            Instruction_t push_r0 = {
                                .instruction = PUSH, 
                                .admr = ADMR_NONE, 
                                .admx = ADMX_R0,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "push"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, push_r0, &instruction_count, i++);

                            // push admx
                            Instruction_t push_admx = {
                                .instruction = PUSH, 
                                .admr = ADMR_NONE, 
                                .admx = admx,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "push"
                                            }
                                        }, 
                                    }, 
                                    admx_expr
                                }
                            };
                            insert_instruction(&instruction, push_admx, &instruction_count, i++);

                            // push $8000
                            Instruction_t push_0x8000 = {
                                .instruction = PUSH, 
                                .admr = ADMR_NONE, 
                                .admx = admx,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "push"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_IMMEDIATE, 
                                                .raw = "$8000"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, push_0x8000, &instruction_count, i++);

                            // .L
                            char* label_L = malloc(64);
                            sprintf(label_L, "%s%d", label_prefix, label_uid);
                            Instruction_t L = {
                                .instruction = NOP, 
                                .admr = ADMR_NONE, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 1, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = label_L
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, L, &instruction_count, i++);

                            // mov r0, [$0002 + sp]
                            Instruction_t mov_r0_admr = {
                                .instruction = MOV, 
                                .admr = ADMR_R0, 
                                .admx = get_equal_admx_from_admr(admr),
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "mov"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 5, 
                                        .type = EXPR_INDIRECT_REGISTER_OFFSET, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_BRACKET_OPEN, 
                                                .raw = "["
                                            }, 
                                            (Token_t) {
                                                .type = TT_IMMEDIATE, 
                                                .raw = "$0002"
                                            }, 
                                            (Token_t) {
                                                .type = TT_PLUS, 
                                                .raw = "+"
                                            }, 
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "sp"
                                            }, 
                                            (Token_t) {
                                                .type = TT_BRACKET_CLOSE, 
                                                .raw = "]"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, mov_r0_admr, &instruction_count, i++);

                            // xor r0, admr
                            Instruction_t xor_r0_admr = {
                                .instruction = XOR, 
                                .admr = ADMR_R0, 
                                .admx = admx,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "xor"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                    admr_expr,
                                }
                            };
                            insert_instruction(&instruction, xor_r0_admr, &instruction_count, i++);

                            // push r0
                            insert_instruction(&instruction, push_r0, &instruction_count, i++);

                            // lea r0, [$0004 + sp]
                            Instruction_t lea_r0_admx = {
                                .instruction = LEA, 
                                .admr = ADMR_R0, 
                                .admx = admx,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "lea"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 5, 
                                        .type = EXPR_INDIRECT_REGISTER_OFFSET, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_BRACKET_OPEN, 
                                                .raw = "["
                                            }, 
                                            (Token_t) {
                                                .type = TT_IMMEDIATE, 
                                                .raw = "$0004"
                                            }, 
                                            (Token_t) {
                                                .type = TT_PLUS, 
                                                .raw = "+"
                                            }, 
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "sp"
                                            }, 
                                            (Token_t) {
                                                .type = TT_BRACKET_CLOSE, 
                                                .raw = "]"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, lea_r0_admx, &instruction_count, i++);

                            // and [r0], admr
                            Instruction_t and_indr0_admr = {
                                .instruction = AND, 
                                .admr = ADMR_IND_R0, 
                                .admx = get_equal_admx_from_admr(admr),
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "and"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 3, 
                                        .type = EXPR_INDIRECT_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_BRACKET_OPEN, 
                                                .raw = "["
                                            }, 
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }, 
                                            (Token_t) {
                                                .type = TT_BRACKET_CLOSE, 
                                                .raw = "]"
                                            }
                                        }, 
                                    }, 
                                    admr_expr,
                                }
                            };
                            insert_instruction(&instruction, and_indr0_admr, &instruction_count, i++);

                            // bws [r0], admr
                            Instruction_t bws_indr0_0xffff = {
                                .instruction = BWS, 
                                .admr = ADMR_IND_R0, 
                                .admx = ADMX_IMM16,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "bws"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 3, 
                                        .type = EXPR_INDIRECT_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_BRACKET_OPEN, 
                                                .raw = "["
                                            }, 
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }, 
                                            (Token_t) {
                                                .type = TT_BRACKET_CLOSE, 
                                                .raw = "]"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_IMMEDIATE, 
                                                .raw = "$FFFF"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, bws_indr0_0xffff, &instruction_count, i++);

                            // pop r0
                            Instruction_t pop_r0 = {
                                .instruction = POP, 
                                .admr = ADMR_R0, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "pop"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, pop_r0, &instruction_count, i++);

                            // mov admr, r0
                            Instruction_t mov_admr_r0 = {
                                .instruction = MOV, 
                                .admr = admr, 
                                .admx = ADMX_R0,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "mov"
                                            }
                                        }, 
                                    }, 
                                    admr_expr,
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, mov_admr_r0, &instruction_count, i++);

                            // pop r0
                            insert_instruction(&instruction, pop_r0, &instruction_count, i++);

                            // bws r0, 1
                            Instruction_t bws_r0_1 = {
                                .instruction = BWS, 
                                .admr = ADMR_R0, 
                                .admx = ADMX_IMM16,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "bws"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_IMMEDIATE, 
                                                .raw = "$0001"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, bws_r0_1, &instruction_count, i++);

                            // tst r0
                            Instruction_t tst_r0 = {
                                .instruction = TST, 
                                .admr = ADMR_R0, 
                                .admx = ADMX_IMM16,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "tst"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, tst_r0, &instruction_count, i++);

                            // push r0
                            insert_instruction(&instruction, push_r0, &instruction_count, i++);

                            // jnz .L
                            Instruction_t jnz_L = {
                                .instruction = JNZ, 
                                .admr = ADMR_NONE, 
                                .admx = ADMX_IMM16,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "jnz"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = label_L
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, jnz_L, &instruction_count, i++);

                            // pop r0
                            insert_instruction(&instruction, pop_r0, &instruction_count, i++);

                            // pop r0
                            insert_instruction(&instruction, pop_r0, &instruction_count, i++);

                            // pop r0
                            insert_instruction(&instruction, pop_r0, &instruction_count, i++);

                            // tst admr
                            Instruction_t tst_admr = {
                                .instruction = TST, 
                                .admr = admr, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "tst"
                                            }
                                        }, 
                                    }, 
                                    admr_expr
                                }
                            };
                            insert_instruction(&instruction, tst_admr, &instruction_count, i++);

                            label_uid += 1;
                            changes_applied = 1;

                        } else {
                            // TODO: find replacement for remaining admx's
                            log_msg(LP_ERROR, "unbable to expand ADD operation with addressing modes %s, %s [%s:%d]", 
                                cpu_reduced_addressing_mode_string[instruction[i].admr], 
                                cpu_extended_addressing_mode_string[instruction[i].admx],
                                 __FILE__, __LINE__
                            );
                        }
                    } else if (instruction[i].admr == ADMR_R0) {
                        if (instruction[i].admx != ADMX_R1) {
                            // The general idea is that we simple move the value to a different compatible register

                            remove_instruction(instruction, &instruction_count, i);
                            
                            // push r1
                            Instruction_t push_r1 = {
                                .instruction = PUSH, 
                                .admr = ADMR_NONE, 
                                .admx = ADMX_R1,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "push"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r1"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, push_r1, &instruction_count, i++);
                            
                            // mov r1, r0
                            Instruction_t mov_r1_r0 = {
                                .instruction = MOV, 
                                .admr = ADMR_R1, 
                                .admx = ADMX_R0,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "mov"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r1"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, mov_r1_r0, &instruction_count, i++);

                            // add r1, admx
                            Instruction_t add_r1_admx = {
                                .instruction = ADD, 
                                .admr = ADMR_R1, 
                                .admx = admx,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "add"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r1"
                                            }
                                        }, 
                                    }, 
                                    admx_expr
                                }
                            };
                            insert_instruction(&instruction, add_r1_admx, &instruction_count, i++);

                            // mov r0, r1
                            Instruction_t mov_r0_r1 = {
                                .instruction = MOV, 
                                .admr = ADMR_R0, 
                                .admx = ADMX_R1,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "mov"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r1"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, mov_r0_r1, &instruction_count, i++);

                            // pop r1
                            Instruction_t pop_r1 = {
                                .instruction = POP, 
                                .admr = ADMR_NONE, 
                                .admx = ADMX_R1,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "pop"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r1"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, pop_r1, &instruction_count, i++);

                            changes_applied = 1;
                        } else {
                            // The general idea is that we simple move the value to a different compatible register

                            remove_instruction(instruction, &instruction_count, i);

                            // push r2
                            Instruction_t push_r2 = {
                                .instruction = PUSH, 
                                .admr = ADMR_NONE, 
                                .admx = ADMX_R2,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "push"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r2"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, push_r2, &instruction_count, i++);
                            
                            // mov r2, r0
                            Instruction_t mov_r2_r0 = {
                                .instruction = MOV, 
                                .admr = ADMR_R2, 
                                .admx = ADMX_R0,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "mov"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r2"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, mov_r2_r0, &instruction_count, i++);

                            // add r2, admx
                            Instruction_t add_r2_admx = {
                                .instruction = ADD, 
                                .admr = ADMR_R2, 
                                .admx = admx,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "add"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r2"
                                            }
                                        }, 
                                    }, 
                                    admx_expr
                                }
                            };
                            insert_instruction(&instruction, add_r2_admx, &instruction_count, i++);

                            // mov r0, r2
                            Instruction_t mov_r0_r2 = {
                                .instruction = MOV, 
                                .admr = ADMR_R0, 
                                .admx = ADMX_R2,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "mov"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r2"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, mov_r0_r2, &instruction_count, i++);

                            // pop r2
                            Instruction_t pop_r2 = {
                                .instruction = POP, 
                                .admr = ADMR_NONE, 
                                .admx = ADMX_R2,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "pop"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r2"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, pop_r2, &instruction_count, i++);

                            changes_applied = 1;
                        }
                        
                    } else if (instruction[i].admr == ADMR_SP) {
                        if (instruction[i].admx == ADMX_IMM16) {

                            remove_instruction(instruction, &instruction_count, i);

                            // push r1
                            Instruction_t push_r1 = {
                                .instruction = PUSH, 
                                .admr = ADMR_NONE, 
                                .admx = ADMX_R1,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "push"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r1"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, push_r1, &instruction_count, i++);

                            // mov r1, admr
                            Instruction_t mov_r1_admr = {
                                .instruction = MOV, 
                                .admr = ADMR_R1, 
                                .admx = get_equal_admx_from_admr(admr),
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "mov"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r1"
                                            }
                                        }, 
                                    }, 
                                    admr_expr
                                }
                            };
                            insert_instruction(&instruction, mov_r1_admr, &instruction_count, i++);

                            // push r0
                            Instruction_t push_r0 = {
                                .instruction = PUSH, 
                                .admr = ADMR_NONE, 
                                .admx = ADMX_R0,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "push"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(&instruction, push_r0, &instruction_count, i++);

                            // push admx
                            Instruction_t push_admx = {
                                .instruction = PUSH, 
                                .admr = ADMR_NONE, 
                                .admx = admx,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "push"
                                            }
                                        }, 
                                    }, 
                                    admx_expr
                                }
                            };
                            insert_instruction(&instruction, push_admx, &instruction_count, i++);

                            // push $8000
                            Instruction_t push_0x8000 = {
                                .instruction = PUSH, 
                                .admr = ADMR_NONE, 
                                .admx = admx,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "push"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_IMMEDIATE, 
                                                .raw = "$8000"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, push_0x8000, &instruction_count, i++);

                            // .L
                            char* label_L = malloc(64);
                            sprintf(label_L, "%s%d", label_prefix, label_uid);
                            Instruction_t L = {
                                .instruction = NOP, 
                                .admr = ADMR_NONE, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 1, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = label_L
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, L, &instruction_count, i++);

                            // mov r0, [$0002 + sp]
                            Instruction_t mov_r0_admr = {
                                .instruction = MOV, 
                                .admr = ADMR_R0, 
                                .admx = get_equal_admx_from_admr(admr),
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "mov"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 5, 
                                        .type = EXPR_INDIRECT_REGISTER_OFFSET, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_BRACKET_OPEN, 
                                                .raw = "["
                                            }, 
                                            (Token_t) {
                                                .type = TT_IMMEDIATE, 
                                                .raw = "$0002"
                                            }, 
                                            (Token_t) {
                                                .type = TT_PLUS, 
                                                .raw = "+"
                                            }, 
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "sp"
                                            }, 
                                            (Token_t) {
                                                .type = TT_BRACKET_CLOSE, 
                                                .raw = "]"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, mov_r0_admr, &instruction_count, i++);

                            // xor r0, r1
                            Instruction_t xor_r0_r1 = {
                                .instruction = XOR, 
                                .admr = ADMR_R1, 
                                .admx = admx,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "xor"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r1"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, xor_r0_r1, &instruction_count, i++);

                            // push r0
                            insert_instruction(&instruction, push_r0, &instruction_count, i++);

                            // lea r0, [$0004 + sp]
                            Instruction_t lea_r0_admx = {
                                .instruction = LEA, 
                                .admr = ADMR_R0, 
                                .admx = admx,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "lea"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 5, 
                                        .type = EXPR_INDIRECT_REGISTER_OFFSET, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_BRACKET_OPEN, 
                                                .raw = "["
                                            }, 
                                            (Token_t) {
                                                .type = TT_IMMEDIATE, 
                                                .raw = "$0004"
                                            }, 
                                            (Token_t) {
                                                .type = TT_PLUS, 
                                                .raw = "+"
                                            }, 
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "sp"
                                            }, 
                                            (Token_t) {
                                                .type = TT_BRACKET_CLOSE, 
                                                .raw = "]"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, lea_r0_admx, &instruction_count, i++);

                            // and [r0], r1
                            Instruction_t and_indr0_admr = {
                                .instruction = AND, 
                                .admr = ADMR_IND_R0, 
                                .admx = ADMX_R1,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "and"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 3, 
                                        .type = EXPR_INDIRECT_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_BRACKET_OPEN, 
                                                .raw = "["
                                            }, 
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }, 
                                            (Token_t) {
                                                .type = TT_BRACKET_CLOSE, 
                                                .raw = "]"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r1"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, and_indr0_admr, &instruction_count, i++);

                            // bws [r0], imm16
                            Instruction_t bws_indr0_0xffff = {
                                .instruction = BWS, 
                                .admr = ADMR_IND_R0, 
                                .admx = ADMX_IMM16,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "bws"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 3, 
                                        .type = EXPR_INDIRECT_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_BRACKET_OPEN, 
                                                .raw = "["
                                            }, 
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }, 
                                            (Token_t) {
                                                .type = TT_BRACKET_CLOSE, 
                                                .raw = "]"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_IMMEDIATE, 
                                                .raw = "$FFFF"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, bws_indr0_0xffff, &instruction_count, i++);

                            // pop r0
                            Instruction_t pop_r0 = {
                                .instruction = POP, 
                                .admr = ADMR_R0, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "pop"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, pop_r0, &instruction_count, i++);

                            // mov r1, r0
                            Instruction_t mov_r1_r0 = {
                                .instruction = MOV, 
                                .admr = ADMR_R1, 
                                .admx = ADMX_R0,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "mov"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r1"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, mov_r1_r0, &instruction_count, i++);

                            // pop r0
                            insert_instruction(&instruction, pop_r0, &instruction_count, i++);

                            // bws r0, 1
                            Instruction_t bws_r0_1 = {
                                .instruction = BWS, 
                                .admr = ADMR_R0, 
                                .admx = ADMX_IMM16,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "bws"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_IMMEDIATE, 
                                                .raw = "$0001"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, bws_r0_1, &instruction_count, i++);

                            // tst r0
                            Instruction_t tst_r0 = {
                                .instruction = TST, 
                                .admr = ADMR_R0, 
                                .admx = ADMX_IMM16,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "tst"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r0"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, tst_r0, &instruction_count, i++);

                            // push r0
                            insert_instruction(&instruction, push_r0, &instruction_count, i++);

                            // jnz .L
                            Instruction_t jnz_L = {
                                .instruction = JNZ, 
                                .admr = ADMR_NONE, 
                                .admx = ADMX_IMM16,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "jnz"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_IMMEDIATE, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = label_L
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, jnz_L, &instruction_count, i++);

                            // pop r0
                            insert_instruction(&instruction, pop_r0, &instruction_count, i++);

                            // pop r0
                            insert_instruction(&instruction, pop_r0, &instruction_count, i++);

                            // pop r0
                            insert_instruction(&instruction, pop_r0, &instruction_count, i++);

                            // mov admr, r1
                            Instruction_t mov_admr_r1 = {
                                .instruction = MOV, 
                                .admr = admr, 
                                .admx = ADMX_R1,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 3, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "mov"
                                            }
                                        }, 
                                    }, 
                                    admr_expr, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r1"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, mov_admr_r1, &instruction_count, i++);

                            // pop r1
                            Instruction_t pop_r1 = {
                                .instruction = POP, 
                                .admr = ADMR_R1, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "pop"
                                            }
                                        }, 
                                    }, 
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_REGISTER, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_REGISTER, 
                                                .raw = "r1"
                                            }
                                        }, 
                                    }, 
                                }
                            };
                            insert_instruction(&instruction, pop_r1, &instruction_count, i++);

                            // tst admr
                            Instruction_t tst_admr = {
                                .instruction = TST, 
                                .admr = admr, 
                                .admx = ADMX_NONE,
                                .argument_bytes = 0, 
                                .is_address = 0, 
                                .is_raw_data = 0, 
                                .expression_count = 2, 
                                .expression = {
                                    (Expression_t) {
                                        .token_count = 1, 
                                        .type = EXPR_INSTRUCTION, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_INSTRUCTION, 
                                                .raw = "tst"
                                            }
                                        }, 
                                    }, 
                                    admr_expr
                                }
                            };
                            insert_instruction(&instruction, tst_admr, &instruction_count, i++);

                            label_uid += 1;
                            changes_applied = 1;

                        } else {
                            // TODO: find replacement for remaining admr's
                            log_msg(LP_ERROR, "unbable to expand ADD operation with addressing modes %s, %s [%s:%d]", 
                                cpu_reduced_addressing_mode_string[instruction[i].admr], 
                                cpu_extended_addressing_mode_string[instruction[i].admx]
                                , __FILE__, __LINE__
                            );
                        }
                        
                    } else {
                        // TODO: find replacement for remaining admr's
                        log_msg(LP_ERROR, "unbable to expand ADD operation with addressing modes %s, %s [%s:%d]", 
                            cpu_reduced_addressing_mode_string[instruction[i].admr], 
                            cpu_extended_addressing_mode_string[instruction[i].admx],
                             __FILE__, __LINE__
                        );
                    }
                }
                if (changes_applied) break;

                if (instruction[i].instruction == ABS) {
                    Expression_t admr_expr = instruction[i].expression[1];
                    CPU_REDUCED_ADDRESSING_MODE_t admr = instruction[i].admr;

                    remove_instruction(instruction, &instruction_count, i);

                    // tst admr
                    Instruction_t tst_admr = {
                        .instruction = TST, 
                        .admr = admr, 
                        .admx = ADMX_NONE,
                        .argument_bytes = 0, 
                        .is_address = 0, 
                        .is_raw_data = 0, 
                        .expression_count = 2, 
                        .expression = {
                            (Expression_t) {
                                .token_count = 1, 
                                .type = EXPR_INSTRUCTION, 
                                .tokens = {
                                    (Token_t) {
                                        .type = TT_INSTRUCTION, 
                                        .raw = "tst"
                                    }
                                }, 
                            }, 
                            admr_expr
                        }
                    };
                    insert_instruction(&instruction, tst_admr, &instruction_count, i++);

                    // jnz .L
                    char* label_L = malloc(64);
                    sprintf(label_L, "%s%d", label_prefix, label_uid);
                    Instruction_t jnz_L = {
                        .instruction = JNZ, 
                        .admr = ADMR_NONE, 
                        .admx = ADMX_IMM16,
                        .argument_bytes = 0, 
                        .is_address = 0, 
                        .is_raw_data = 0, 
                        .expression_count = 2, 
                        .expression = {
                            (Expression_t) {
                                .token_count = 1, 
                                .type = EXPR_INSTRUCTION, 
                                .tokens = {
                                    (Token_t) {
                                        .type = TT_INSTRUCTION, 
                                        .raw = "jnz"
                                    }
                                }, 
                            }, 
                            (Expression_t) {
                                .token_count = 1, 
                                .type = EXPR_IMMEDIATE, 
                                .tokens = {
                                    (Token_t) {
                                        .type = TT_LABEL, 
                                        .raw = label_L
                                    }
                                }, 
                            }, 
                        }
                    };
                    insert_instruction(&instruction, jnz_L, &instruction_count, i++);

                    // not admr
                    Instruction_t not_admr = {
                        .instruction = NOT, 
                        .admr = admr, 
                        .admx = ADMX_NONE,
                        .argument_bytes = 0, 
                        .is_address = 0, 
                        .is_raw_data = 0, 
                        .expression_count = 2, 
                        .expression = {
                            (Expression_t) {
                                .token_count = 1, 
                                .type = EXPR_INSTRUCTION, 
                                .tokens = {
                                    (Token_t) {
                                        .type = TT_INSTRUCTION, 
                                        .raw = "not"
                                    }
                                }, 
                            }, 
                            admr_expr
                        }
                    };
                    insert_instruction(&instruction, not_admr, &instruction_count, i++);

                    // add admr, $0001
                    Instruction_t add_admr_0x0001 = {
                        .instruction = ADD, 
                        .admr = admr, 
                        .admx = ADMX_IMM16,
                        .argument_bytes = 0, 
                        .is_address = 0, 
                        .is_raw_data = 0, 
                        .expression_count = 3, 
                        .expression = {
                            (Expression_t) {
                                .token_count = 1, 
                                .type = EXPR_INSTRUCTION, 
                                .tokens = {
                                    (Token_t) {
                                        .type = TT_INSTRUCTION, 
                                        .raw = "add"
                                    }
                                }, 
                            }, 
                            admr_expr, 
                            (Expression_t) {
                                .token_count = 1, 
                                .type = EXPR_IMMEDIATE, 
                                .tokens = {
                                    (Token_t) {
                                        .type = TT_IMMEDIATE, 
                                        .raw = "$0001"
                                    }
                                }, 
                            }, 
                        }
                    };
                    insert_instruction(&instruction, add_admr_0x0001, &instruction_count, i++);

                    // .L
                    Instruction_t L = {
                        .instruction = NOP, 
                        .admr = ADMR_NONE, 
                        .admx = ADMX_NONE,
                        .argument_bytes = 0, 
                        .is_address = 0, 
                        .is_raw_data = 0, 
                        .expression_count = 1, 
                        .expression = {
                            (Expression_t) {
                                .token_count = 1, 
                                .type = EXPR_IMMEDIATE, 
                                .tokens = {
                                    (Token_t) {
                                        .type = TT_LABEL, 
                                        .raw = label_L
                                    }
                                }, 
                            }, 
                        }
                    };
                    insert_instruction(&instruction, L, &instruction_count, i++);

                    // tst admr
                    insert_instruction(&instruction, tst_admr, &instruction_count, i++);

                    label_uid += 1;
                    changes_applied = 1;
                }
                if (changes_applied) break;

                if (instruction[i].instruction == DEC) {
                    Expression_t admr_expr = instruction[i].expression[1];
                    CPU_REDUCED_ADDRESSING_MODE_t admr = instruction[i].admr;

                    remove_instruction(instruction, &instruction_count, i);

                    // add admr, $FFFF
                    Instruction_t tst_admr = {
                        .instruction = ADD, 
                        .admr = admr, 
                        .admx = ADMX_IMM16,
                        .argument_bytes = 0, 
                        .is_address = 0, 
                        .is_raw_data = 0, 
                        .expression_count = 3, 
                        .expression = {
                            (Expression_t) {
                                .token_count = 1, 
                                .type = EXPR_INSTRUCTION, 
                                .tokens = {
                                    (Token_t) {
                                        .type = TT_INSTRUCTION, 
                                        .raw = "tst"
                                    }
                                }, 
                            }, 
                            admr_expr, 
                            (Expression_t) {
                                .token_count = 1, 
                                .type = EXPR_IMMEDIATE, 
                                .tokens = {
                                    (Token_t) {
                                        .type = TT_IMMEDIATE, 
                                        .raw = "$FFFF"
                                    }
                                }, 
                            }, 
                        }
                    };
                    insert_instruction(&instruction, tst_admr, &instruction_count, i++);

                    changes_applied = 1;
                }
                if (changes_applied) break;

                if (instruction[i].instruction == INC) {
                    Expression_t admr_expr = instruction[i].expression[1];
                    CPU_REDUCED_ADDRESSING_MODE_t admr = instruction[i].admr;

                    remove_instruction(instruction, &instruction_count, i);

                    // add admr, $0001
                    Instruction_t tst_admr = {
                        .instruction = ADD, 
                        .admr = admr, 
                        .admx = ADMX_IMM16,
                        .argument_bytes = 0, 
                        .is_address = 0, 
                        .is_raw_data = 0, 
                        .expression_count = 3, 
                        .expression = {
                            (Expression_t) {
                                .token_count = 1, 
                                .type = EXPR_INSTRUCTION, 
                                .tokens = {
                                    (Token_t) {
                                        .type = TT_INSTRUCTION, 
                                        .raw = "tst"
                                    }
                                }, 
                            }, 
                            admr_expr, 
                            (Expression_t) {
                                .token_count = 1, 
                                .type = EXPR_IMMEDIATE, 
                                .tokens = {
                                    (Token_t) {
                                        .type = TT_IMMEDIATE, 
                                        .raw = "$0001"
                                    }
                                }, 
                            }, 
                        }
                    };
                    insert_instruction(&instruction, tst_admr, &instruction_count, i++);
                    
                    changes_applied = 1;
                }
                if (changes_applied) break;

                if (instruction[i].instruction == SUB) {
                    Expression_t admr_expr = instruction[i].expression[1];
                    Expression_t admx_expr = instruction[i].expression[2];
                    CPU_REDUCED_ADDRESSING_MODE_t admr = instruction[i].admr;
                    CPU_EXTENDED_ADDRESSING_MODE_t admx = instruction[i].admx;

                    /*
                    sub admr, admx
                    =>
                    neg admr
                    add admr, admx
                    neg admr
                    tst admr
                    */

                    remove_instruction(instruction, &instruction_count, i);

                    // neg admr
                    Instruction_t neg_admr = {
                        .instruction = NEG, 
                        .admr = admr, 
                        .admx = ADMX_NONE,
                        .argument_bytes = 0, 
                        .is_address = 0, 
                        .is_raw_data = 0, 
                        .expression_count = 2, 
                        .expression = {
                            (Expression_t) {
                                .token_count = 1, 
                                .type = EXPR_INSTRUCTION, 
                                .tokens = {
                                    (Token_t) {
                                        .type = TT_INSTRUCTION, 
                                        .raw = "neg"
                                    }
                                }, 
                            }, 
                            admr_expr
                        }
                    };
                    insert_instruction(&instruction, neg_admr, &instruction_count, i++);

                    // add admr, admx
                    Instruction_t add_admx_admr = {
                        .instruction = ADD, 
                        .admr = admr, 
                        .admx = admx,
                        .argument_bytes = 0, 
                        .is_address = 0, 
                        .is_raw_data = 0, 
                        .expression_count = 3, 
                        .expression = {
                            (Expression_t) {
                                .token_count = 1, 
                                .type = EXPR_INSTRUCTION, 
                                .tokens = {
                                    (Token_t) {
                                        .type = TT_INSTRUCTION, 
                                        .raw = "add"
                                    }
                                }, 
                            }, 
                            admr_expr, 
                            admx_expr
                        }
                    };
                    insert_instruction(&instruction, add_admx_admr, &instruction_count, i++);

                    // neg admr
                    insert_instruction(&instruction, neg_admr, &instruction_count, i++);

                    // tst admr
                    Instruction_t tst_admr = {
                        .instruction = TST, 
                        .admr = admr, 
                        .admx = ADMX_NONE,
                        .argument_bytes = 0, 
                        .is_address = 0, 
                        .is_raw_data = 0, 
                        .expression_count = 2, 
                        .expression = {
                            (Expression_t) {
                                .token_count = 1, 
                                .type = EXPR_INSTRUCTION, 
                                .tokens = {
                                    (Token_t) {
                                        .type = TT_INSTRUCTION, 
                                        .raw = "tst"
                                    }
                                }, 
                            }, 
                            admr_expr
                        }
                    };
                    insert_instruction(&instruction, tst_admr, &instruction_count, i++);

                    
                    changes_applied = 1;
                }
                if (changes_applied) break;

                if (instruction[i].instruction == NEG) {
                    Expression_t admr_expr = instruction[i].expression[1];
                    CPU_REDUCED_ADDRESSING_MODE_t admr = instruction[i].admr;

                    /*
                    neg admr
                    =>
                    not admr
                    add admr, $0001
                    */

                    remove_instruction(instruction, &instruction_count, i);

                    // not admr
                    Instruction_t not_admr = {
                        .instruction = NOT, 
                        .admr = admr, 
                        .admx = ADMX_NONE,
                        .argument_bytes = 0, 
                        .is_address = 0, 
                        .is_raw_data = 0, 
                        .expression_count = 2, 
                        .expression = {
                            (Expression_t) {
                                .token_count = 1, 
                                .type = EXPR_INSTRUCTION, 
                                .tokens = {
                                    (Token_t) {
                                        .type = TT_INSTRUCTION, 
                                        .raw = "not"
                                    }
                                }, 
                            }, 
                            admr_expr
                        }
                    };
                    insert_instruction(&instruction, not_admr, &instruction_count, i++);

                    // add admr, $0001
                    Instruction_t add_admr_0x0001 = {
                        .instruction = ADD, 
                        .admr = admr, 
                        .admx = ADMX_IMM16,
                        .argument_bytes = 0, 
                        .is_address = 0, 
                        .is_raw_data = 0, 
                        .expression_count = 3, 
                        .expression = {
                            (Expression_t) {
                                .token_count = 1, 
                                .type = EXPR_INSTRUCTION, 
                                .tokens = {
                                    (Token_t) {
                                        .type = TT_INSTRUCTION, 
                                        .raw = "add"
                                    }
                                }, 
                            }, 
                            admr_expr, 
                            (Expression_t) {
                                .token_count = 1, 
                                .type = EXPR_IMMEDIATE, 
                                .tokens = {
                                    (Token_t) {
                                        .type = TT_IMMEDIATE, 
                                        .raw = "$0001"
                                    }
                                }, 
                            }, 
                        }
                    };
                    insert_instruction(&instruction, add_admr_0x0001, &instruction_count, i++);

                    changes_applied = 1;
                }
                if (changes_applied) break;
            } // CFF_INT_ARITH

        }// reconstruct asm file
    }

    // reconstruct asm file
    char* output = NULL;         // final assembly string
    long output_len = 0;         // current length of code output
    for (int i = 0; i < instruction_count; i++) {
        Instruction_t instr = instruction[i];
        int ec = instr.expression_count;
        if (instr.is_address) {
            if (instr.expression[0].type == EXPR_ADDRESS) {
                char tmp[32];
                sprintf(tmp, ".address $%.4x", instr.address);
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


char* macro_code_expand_from_file(char* filename, CpuFeatureFlag_t feature_flag) {
    char* content = read_file(filename, NULL);
    if (!content) {
        log_msg(LP_ERROR, "read_file failed [%s:%d]", __FILE__, __LINE__);
        return NULL;
    }
    char* new_code = macro_code_expand(content, feature_flag);

    return new_code;
}

