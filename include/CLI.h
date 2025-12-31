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
    unsigned int c : 1;
    unsigned int run : 1;
    unsigned int O : 1;
    unsigned int o : 1;
    unsigned int save_temps : 1;
    unsigned int d : 1;
    unsigned int no_c : 1;
    unsigned int no_m : 1;
} CompileOption_t;

extern const CompileOption_t CO_DEFAULT;

CompileOption_t cli_parse_arguments(int argc, char** argv, int* error);

#endif // _CLI_H_
