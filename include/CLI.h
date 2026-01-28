#ifndef _CLI_H_
#define _CLI_H_

extern const char* CLI_USAGE;

typedef enum CompileFileType_t {
    CFT_BIN, 
    CFT_ASM, 
    CFT_IR, 
    CFT_CCAN, 
    CFT_C
    // BIN < ASM < IR < CCAN < C
} CompileFileType_t;

typedef struct CompileOption_t {
    char* input_filename;
    char* binary_filename;
    CompileFileType_t cft;
    // Files
    unsigned int c : 1;             // [c]ompile type
    unsigned int o : 1;             // [o]utput name
    unsigned int save_temps : 1;    // [save temps]
    // Assembler
    unsigned int err_csb : 1;       // [e]rror on [c]ode [s]egment [b]reach
    unsigned int pad_zero : 1;      // [pad] segment breach with [zero]s
    // Disassembler
    unsigned int d : 1;             // [d]isassemble
    // Optimizer
    unsigned int O : 1;             // [O]ptimization
    // Canonicalizer and macro-expander
    unsigned int no_c : 1;          // [no] [c]anonivalizer
    unsigned int no_m : 1;          // [no] [m]acro-expander
    // IR
    unsigned int pic : 1;           // [p]osition [i]ndependent [c]ode
    unsigned int no_preamble : 1;   // [no preamble]
    // Emulator
    unsigned int run : 1;           // [run]
} CompileOption_t;

extern const CompileOption_t CO_DEFAULT;

CompileOption_t cli_parse_arguments(int argc, char** argv, int* error);

#endif // _CLI_H_
