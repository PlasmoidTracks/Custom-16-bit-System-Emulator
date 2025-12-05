#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "utils/Log.h"
#include "utils/IO.h"
#include "utils/String.h"

#include "cpu/cpu.h"
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

            #ifndef DCFF_INT_ARITH_EXT
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
                            insert_instruction(instruction, push_r0, &instruction_count, i++);
                            
                            // push r1
                            Instruction_t push_r1 = {
                                .instruction = PUSH, 
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
                            insert_instruction(instruction, push_r1, &instruction_count, i++);

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
                            insert_instruction(instruction, mov_r0_admx, &instruction_count, i++);

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
                            insert_instruction(instruction, mov_r1_admr, &instruction_count, i++);

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
                                                .raw = "0x0000"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(instruction, mov_admr_0, &instruction_count, i++);

                            // push r0
                            insert_instruction(instruction, push_r0, &instruction_count, i++);

                            // push r1
                            insert_instruction(instruction, push_r1, &instruction_count, i++);

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
                            insert_instruction(instruction, abs_r0, &instruction_count, i++);

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
                            insert_instruction(instruction, abs_r1, &instruction_count, i++);

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
                            insert_instruction(instruction, cmp_r1_r0, &instruction_count, i++);

                            // jnl .L_dont_swap
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
                                        .type = EXPR_LABEL, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = ".L_dont_swap"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(instruction, jnl_dont_swap, &instruction_count, i++);

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
                            insert_instruction(instruction, pop_r0, &instruction_count, i++);

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
                            insert_instruction(instruction, pop_r1, &instruction_count, i++);

                            // jmp .L_post_pop
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
                                        .type = EXPR_LABEL, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = ".L_post_pop"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(instruction, jmp_post_pop, &instruction_count, i++);

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
                                        .type = EXPR_LABEL, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = ".L_dont_swap"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(instruction, dont_swap, &instruction_count, i++);

                            // pop r1
                            insert_instruction(instruction, pop_r1, &instruction_count, i++);

                            // pop r0
                            insert_instruction(instruction, pop_r0, &instruction_count, i++);

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
                                        .type = EXPR_LABEL, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = ".L_post_pop"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(instruction, post_pop, &instruction_count, i++);

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
                                                .raw = "0x0000"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(instruction, cmp_r0_0, &instruction_count, i++);

                            // jz .L_done
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
                                        .type = EXPR_LABEL, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = ".L_done"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(instruction, jz_done, &instruction_count, i++);

                            // jnl .L
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
                                        .type = EXPR_LABEL, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = ".L"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(instruction, jnl, &instruction_count, i++);

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
                            insert_instruction(instruction, neg_r0, &instruction_count, i++);

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
                            insert_instruction(instruction, neg_r1, &instruction_count, i++);

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
                                        .type = EXPR_LABEL, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = ".L"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(instruction, L, &instruction_count, i++);

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
                            insert_instruction(instruction, add_admr_r1, &instruction_count, i++);

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
                            insert_instruction(instruction, dec_r0, &instruction_count, i++);

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
                                        .type = EXPR_LABEL, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = ".L"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(instruction, jnz, &instruction_count, i++);

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
                                        .type = EXPR_LABEL, 
                                        .tokens = {
                                            (Token_t) {
                                                .type = TT_LABEL, 
                                                .raw = ".L_done"
                                            }
                                        }, 
                                    }
                                }
                            };
                            insert_instruction(instruction, done, &instruction_count, i++);

                            // pop r1
                            insert_instruction(instruction, pop_r1, &instruction_count, i++);

                            // pop r0
                            insert_instruction(instruction, pop_r0, &instruction_count, i++);
                    } else {
                        Expression_t admr_expr = instruction[i].expression[1];
                        Expression_t admx_expr = instruction[i].expression[2];
                        CPU_REDUCED_ADDRESSING_MODE_t admr = instruction[i].admr;
                        CPU_EXTENDED_ADDRESSING_MODE_t admx = instruction[i].admx;

                        remove_instruction(instruction, &instruction_count, i);

                        // push r2
                        Instruction_t push_r2 = {
                            .instruction = PUSH, 
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
                        insert_instruction(instruction, push_r2, &instruction_count, i++);
                        
                        // push r3
                        Instruction_t push_r3 = {
                            .instruction = PUSH, 
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
                        insert_instruction(instruction, push_r3, &instruction_count, i++);

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
                        insert_instruction(instruction, mov_r2_admx, &instruction_count, i++);

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
                        insert_instruction(instruction, mov_r3_admr, &instruction_count, i++);

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
                                            .raw = "0x0000"
                                        }
                                    }, 
                                }
                            }
                        };
                        insert_instruction(instruction, mov_admr_0, &instruction_count, i++);

                        // push r2
                        insert_instruction(instruction, push_r2, &instruction_count, i++);

                        // push r3
                        insert_instruction(instruction, push_r3, &instruction_count, i++);

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
                        insert_instruction(instruction, abs_r2, &instruction_count, i++);

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
                        insert_instruction(instruction, abs_r3, &instruction_count, i++);

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
                        insert_instruction(instruction, cmp_r3_r2, &instruction_count, i++);

                        // jnl .L_dont_swap
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
                                    .type = EXPR_LABEL, 
                                    .tokens = {
                                        (Token_t) {
                                            .type = TT_LABEL, 
                                            .raw = ".L_dont_swap"
                                        }
                                    }, 
                                }
                            }
                        };
                        insert_instruction(instruction, jnl_dont_swap, &instruction_count, i++);

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
                        insert_instruction(instruction, pop_r2, &instruction_count, i++);

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
                        insert_instruction(instruction, pop_r3, &instruction_count, i++);

                        // jmp .L_post_pop
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
                                    .type = EXPR_LABEL, 
                                    .tokens = {
                                        (Token_t) {
                                            .type = TT_LABEL, 
                                            .raw = ".L_post_pop"
                                        }
                                    }, 
                                }
                            }
                        };
                        insert_instruction(instruction, jmp_post_pop, &instruction_count, i++);

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
                                    .type = EXPR_LABEL, 
                                    .tokens = {
                                        (Token_t) {
                                            .type = TT_LABEL, 
                                            .raw = ".L_dont_swap"
                                        }
                                    }, 
                                }
                            }
                        };
                        insert_instruction(instruction, dont_swap, &instruction_count, i++);

                        // pop r3
                        insert_instruction(instruction, pop_r3, &instruction_count, i++);

                        // pop r2
                        insert_instruction(instruction, pop_r2, &instruction_count, i++);

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
                                    .type = EXPR_LABEL, 
                                    .tokens = {
                                        (Token_t) {
                                            .type = TT_LABEL, 
                                            .raw = ".L_post_pop"
                                        }
                                    }, 
                                }
                            }
                        };
                        insert_instruction(instruction, post_pop, &instruction_count, i++);

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
                                            .raw = "0x0000"
                                        }
                                    }, 
                                }
                            }
                        };
                        insert_instruction(instruction, cmp_r2_0, &instruction_count, i++);

                        // jz .L_done
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
                                    .type = EXPR_LABEL, 
                                    .tokens = {
                                        (Token_t) {
                                            .type = TT_LABEL, 
                                            .raw = ".L_done"
                                        }
                                    }, 
                                }
                            }
                        };
                        insert_instruction(instruction, jz_done, &instruction_count, i++);

                        // jnl .L
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
                                    .type = EXPR_LABEL, 
                                    .tokens = {
                                        (Token_t) {
                                            .type = TT_LABEL, 
                                            .raw = ".L"
                                        }
                                    }, 
                                }
                            }
                        };
                        insert_instruction(instruction, jnl, &instruction_count, i++);

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
                        insert_instruction(instruction, neg_r2, &instruction_count, i++);

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
                        insert_instruction(instruction, neg_r3, &instruction_count, i++);

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
                                    .type = EXPR_LABEL, 
                                    .tokens = {
                                        (Token_t) {
                                            .type = TT_LABEL, 
                                            .raw = ".L"
                                        }
                                    }, 
                                }
                            }
                        };
                        insert_instruction(instruction, L, &instruction_count, i++);

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
                        insert_instruction(instruction, add_admr_r3, &instruction_count, i++);

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
                        insert_instruction(instruction, dec_r2, &instruction_count, i++);

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
                                    .type = EXPR_LABEL, 
                                    .tokens = {
                                        (Token_t) {
                                            .type = TT_LABEL, 
                                            .raw = ".L"
                                        }
                                    }, 
                                }
                            }
                        };
                        insert_instruction(instruction, jnz, &instruction_count, i++);

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
                                    .type = EXPR_LABEL, 
                                    .tokens = {
                                        (Token_t) {
                                            .type = TT_LABEL, 
                                            .raw = ".L_done"
                                        }
                                    }, 
                                }
                            }
                        };
                        insert_instruction(instruction, done, &instruction_count, i++);

                        // pop r3
                        insert_instruction(instruction, pop_r3, &instruction_count, i++);

                        // pop r2
                        insert_instruction(instruction, pop_r2, &instruction_count, i++);
                }
                }
                if (changes_applied) break;
            #endif // DCFF_INT_ARITH_EXT

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

