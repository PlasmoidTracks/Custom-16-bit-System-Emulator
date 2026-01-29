
#include <string.h>

#include "utils/String.h"
#include "utils/Log.h"

#include "CLI.h"

const char* CLI_USAGE = "\
USAGE:\n\
  ./main [options] <input>\n\
\n\
FILES:\n\
  <input>                 .c | .ccan | .ir | .asm | .bin\n\
  -c=<type>               c | ccan | ir | asm | bin; force-interpret input file as <type> despite extension\n\
  -o <file>               Output file (default: file=prog.bin)\n\
  -save-temps             Keep all intermediate files\n\
\n\
ASSEMBLER:\n\
  -noerr-csb              No error when binary exceeds code memory bounds (overrides padding)\n\
  -pad-zero               Pad segment breach with zeros when binary exceeds memory bounds\n\
  -noerr-overlap          No error when binary overwrites itself (for example, on address overflow)\n\
  -overwrite-overlap      Overwrite old values with new values on overwrites\n\
\n\
DISASSEMBLER:\n\
  -d                      Enable disassembly\n\
\n\
OPTIMIZER:\n\
  -O0                     No optimization\n\
  -O1                     Basic optimization (default: O1)\n\
\n\
CANONICALIZER AND MACRO-EXPANDER:\n\
  -no-c                   Skip canonicalizer\n\
  -no-m                   Skip macro expander\n\
\n\
IR:\n\
  -pic                    compile as position independent code\n\
  -no-preamble            omit main call preamble in executable\n\
\n\
EMULATOR:\n\
  -run                    execute final binary in emulator\n\
\n\
EXAMPLES:\n\
  ./main input.ir -c=ir -run -O0 -o prog.bin -save-temps -d -pic -no-preamble -pad-zero -noerr-overlap -overwrite-overlap\n\
  ./main input.asm -run -save-temps -d\n\
  ./main input.bin -d\n\
  ./main any.file -c=bin -run\n\
";


const CompileOption_t CO_DEFAULT = {
    .input_filename = (void*) 0, 
    .binary_filename = "prog.bin", 
    .cft = CFT_BIN, 
    // Files
    .c = 0,
    .o = 0, 
    .save_temps = 0, 
    // Assembler
    .err_csb = 1, 
    .pad_zero = 1, 
    .err_overlap = 1, 
    .overwrite_overlap = 0, 
    // Disassembler
    .d = 0, 
    // Optimizer
    .O = 1, 
    // Canonicalizer and macro-expander
    .no_c = 0, 
    .no_m = 0, 
    // IR
    .pic = 0, 
    .no_preamble = 0, 
    // Emulator
    .run = 0, 
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
        if (strcmp(argv[arg_index], "-O0") == 0) {
            co.O = 0;
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
        if (strcmp(argv[arg_index], "-no-c") == 0) {
            co.no_c = 1;
            arg_index ++;
            continue;
        }
        if (strcmp(argv[arg_index], "-no-m") == 0) {
            co.no_m = 1;
            arg_index ++;
            continue;
        }
        if (strcmp(argv[arg_index], "-pic") == 0) {
            co.pic = 1;
            arg_index ++;
            continue;
        }
        if (strcmp(argv[arg_index], "-no-preamble") == 0) {
            co.no_preamble = 1;
            arg_index ++;
            continue;
        }
        if (strcmp(argv[arg_index], "-noerr-csb") == 0) {
            co.err_csb = 0;
            arg_index ++;
            continue;
        }
        if (strcmp(argv[arg_index], "-nopad-zero") == 0) {
            co.pad_zero = 0;
            arg_index ++;
            continue;
        }
        if (strcmp(argv[arg_index], "-noerr-overlap") == 0) {
            co.err_overlap = 0;
            arg_index ++;
            continue;
        }
        if (strcmp(argv[arg_index], "-overwrite-overlap") == 0) {
            co.overwrite_overlap = 1;
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

