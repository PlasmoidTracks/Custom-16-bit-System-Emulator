
#include <string.h>

#include "utils/String.h"
#include "utils/Log.h"

#include "CLI.h"

const char* CLI_USAGE = "\
USAGE:\n\
  ./main [options] <input>\n\
\n\
INPUT:\n\
  <input>                 .c | .ccan | .ir | .asm | .bin\n\
\n\
TYPE: \n\
  -c=<type>               c | ccan | ir | asm | bin; force-interpret input file as <type> despite extension\n\
\n\
MODES (one):\n\
  -run                    execute final binary in emulator\n\
\n\
OPTIMIZATION:\n\
  -O0                     No optimization\n\
  -O1                     Basic optimization (default: O1)\n\
\n\
OUTPUT:\n\
  -o <file>               Output file (default: file=prog.bin)\n\
\n\
INTERMEDIATES:\n\
  -save-temps             Keep all intermediate files\n\
\n\
DEBUG / INSPECTION:\n\
  -d                      Enable disassembly\n\
\n\
EXAMPLES:\n\
  ./main input.ir -run -O1 -o prog.bin -save-temps -d\n\
  ./main input.asm -run -O1 -s -save-temps -d\n\
  ./main input.bin -d\n\
";

const CompileOption_t CO_DEFAULT = {
    .input_filename = (void*) 0, 
    .binary_filename = "prog.bin", 
    .cft = CFT_BIN, 
    .run = 0, 
    .O = 1, 
    .o = 0, 
    .save_temps = 0, 
    .d = 0, 
};

CompileOption_t cli_parse_arguments(int argc, char** argv, int* error) {
    if (error) {*error = 0;}
    CompileOption_t co = CO_DEFAULT;

    int arg_index = 2;
    while (arg_index < argc) {
        if (strcmp(argv[arg_index], "-run") == 0) {
            co.run = 1;
            arg_index ++;
            continue;
        }
        if (strcmp(argv[arg_index], "-O1") == 0) {
            co.O = 1;
            arg_index ++;
            continue;
        }
        if (strcmp(argv[arg_index], "-o") == 0) {
            co.o = 1;
            arg_index ++;
            if (argc <= arg_index) {
                log_msg(LP_ERROR, "Compiler option '-o' has to be followed by another argument [%s:%d]", __FILE__, __LINE__);
                if (error) {*error = 1;}
                return co;
            }
            co.binary_filename = argv[arg_index];
            arg_index ++;
            continue;
        }
        if (strcmp(argv[arg_index], "-save-temps") == 0) {
            co.save_temps = 1;
            arg_index ++;
            continue;
        }
        if (strcmp(argv[arg_index], "-d") == 0) {
            co.d = 1;
            arg_index ++;
            continue;
        }
        char tmp[32];
        strcpy(tmp, argv[arg_index]);
        tmp[3] = '\0';
        if (strcmp(tmp, "-c=") == 0) {
            co.c = 1;
            char* type = &argv[arg_index][3];
            if (strcmp(type, "bin") == 0) {
                co.cft = CFT_BIN;
                co.binary_filename = argv[1];
            } else if (strcmp(type, "asm") == 0) {
                co.cft = CFT_ASM;
            } else if (strcmp(type, "ir") == 0) {
                co.cft = CFT_IR;
            } else if (strcmp(type, "ccan") == 0) {
                co.cft = CFT_CCAN;
            } else if (strcmp(type, "c") == 0) {
                co.cft = CFT_C;
            } else {
                log_msg(LP_ERROR, "Unkown file format \"%s\" [%s:%d]", type, __FILE__, __LINE__);
                if (error) {*error = 1;}
                return co;
            }
            arg_index ++;
            continue;
        }
        log_msg(LP_WARNING, "Unknown argument \"%s\" [%s:%d]", argv[arg_index], __FILE__, __LINE__);
        arg_index ++;
    }
    
    co.input_filename = argv[1];

    if (!co.c) {
        char** splits = split(argv[1], ".", "");
        int index = 0;
        while (splits[++index]);
        char* extension = splits[index-1];
        if (strcmp(extension, "bin") == 0) {
            co.cft = CFT_BIN;
        } else if (strcmp(extension, "asm") == 0) {
            co.cft = CFT_ASM;
        } else if (strcmp(extension, "ir") == 0) {
            co.cft = CFT_IR;
        } else if (strcmp(extension, "ccan") == 0) {
            co.cft = CFT_CCAN;
        } else if (strcmp(extension, "c") == 0) {
            co.cft = CFT_C;
        } else {
            log_msg(LP_ERROR, "Unknown file format \"%s\" [%s:%d]", extension, __FILE__, __LINE__);
            if (error) {*error = 1;}
        }
    }

    return co;
}

