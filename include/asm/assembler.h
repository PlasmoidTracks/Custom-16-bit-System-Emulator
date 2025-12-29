#ifndef _ASSEMBLER_H_
#define _ASSEMBLER_H_

#include <stdint.h>

#include "cpu/cpu_instructions.h"
#include "cpu/cpu_addressing_modes.h"

#define MAX_LABELS 256

typedef enum TokenType_t {
    TT_INSTRUCTION, 
    TT_REGISTER, 
    TT_IMMEDIATE, 
    TT_PLUS, 
    TT_MINUS, 
    TT_STAR, 
    TT_INDIRECT_IMMEDIATE, 
    TT_INDIRECT_REGISTER, 
    TT_INDIRECT_REGISTER_OFFSET, 
    TT_INDIRECT_SCALE_OFFSET, 
    TT_BRACKET_OPEN, 
    TT_BRACKET_CLOSE, 
    TT_LABEL, 
    TT_ADDRESS, 
    TT_SEGMENT_CODE, 
    TT_SEGMENT_DATA, 
    TT_RESERVE, 
    TT_INCLUDE, 
    TT_INCBIN, 
    TT_TEXT, 
    TT_STRING, 
} TokenType_t;

typedef enum {
    EXPR_NONE,
    EXPR_INSTRUCTION, 
    EXPR_REGISTER,
    EXPR_IMMEDIATE,
    EXPR_INDIRECT_IMMEDIATE,
    EXPR_INDIRECT_REGISTER,
    EXPR_INDIRECT_REGISTER_OFFSET,
    EXPR_INDIRECT_SCALE_OFFSET, 
    EXPR_SEGMENT_DATA, 
    EXPR_SEGMENT_CODE, 
    EXPR_RESERVE, 
    EXPR_INCBIN, 
    EXPR_ADDRESS, 
    EXPR_TEXT_DEFINITION, 
} ExpressionType_t;


typedef struct Token_t {
    TokenType_t type;
    char* raw;
} Token_t;

typedef struct Expression_t {
    ExpressionType_t type;
    Token_t tokens[8];  // holds up to 8 tokens, e.g., for complex expressions
    int token_count;
} Expression_t;

typedef struct Instruction_t {
    Expression_t expression[3];
    int expression_count;
    CPU_INSTRUCTION_MNEMONIC_t instruction;
    CPU_EXTENDED_ADDRESSING_MODE_t admx;
    CPU_REDUCED_ADDRESSING_MODE_t admr;
    uint8_t arguments[6];
    int argument_bytes;

    int is_raw_data;
    uint16_t raw_data;

    int byte_aligned;

    int is_address;
    int address;
} Instruction_t;

typedef struct Label_t {
    char name[256];
    int value;
} Label_t;

extern Label_t jump_label[MAX_LABELS];
extern Label_t const_label[MAX_LABELS];

extern const char* token_type_string[];
extern const char* expression_type_string[];

extern int jump_label_index;
extern int const_label_index;

extern char** assembler_split_to_separate_lines(const char text[]);

extern void assembler_remove_comments(char **text[]);

extern char** assembler_split_to_words(char* lines[], int* token_count);

extern Token_t* assembler_parse_words(char** word, int word_count, int* token_count);

extern Expression_t* assembler_parse_token(Token_t* tokens, int token_count, int* expression_count);

extern Instruction_t* assembler_parse_expression(Expression_t* expression, int expression_count, int* instruction_count, uint16_t** segment, int* segment_count);

extern uint8_t* assembler_compile(char* content, long* binary_size, uint16_t** segment, int* segment_count);

extern uint8_t* assembler_compile_from_file(const char* filename, long* binary_size, uint16_t** segment, int* segment_count);

#endif
