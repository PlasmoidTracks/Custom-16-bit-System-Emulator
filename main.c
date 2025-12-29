#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>

#include "utils/IO.h"
#include "utils/String.h"
#include "utils/Log.h"

#include "transpile/ccan/ccan_lexer.h"
#include "transpile/ccan/ccan_parser.h"
#include "asm/assembler.h"

#include "cpu/cpu_utils.h"
#include "codegen/ir_compiler.h"
#include "asm/canonicalizer.h"
#include "asm/optimizer.h"
#include "asm/macro_code_expansion.h"
#include "asm/disassembler.h"

#include "system.h"


/*
TODO: 
At most, I want to be able to compile from IR down to asm, (processing of can and exp are mandatory), activate optimizations, save the temps
generate the binary output as a file with filename of my own choosing, generate the disassembly of said binary and execute it. 

./main input.ir -run -O1 -o prog.bin -save-temps -d

*/

const char* CLI_USAGE = "\n\
USAGE:\n\
  ./main [options] <input>\n\
\n\
INPUT:\n\
  <input>                 .ir | .asm | .bin\n\
\n\
MODES (one):\n\
  -run                    Compile then execute (emulator)\n\
\n\
OPTIMIZATION:\n\
  -O0                     No optimization\n\
  -O1                     Basic optimization\n\
\n\
OUTPUT:\n\
  -o <file>               Output file (default: prog.bin)\n\
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
    unsigned int run : 1;
    unsigned int O : 1;
    unsigned int o : 1;
    unsigned int save_temps : 1;
    unsigned int d : 1;
} CompileOption_t;



#define HW_WATCH
#undef HW_WATCH





int main(int argc, char* argv[]) {

    if (argc < 2) {
        log_msg(LP_ERROR, "Not enough arguments given [%s:%d]", __FILE__, __LINE__);
        log_msg(LP_INFO, CLI_USAGE);
        return 0;
    }

    CompileOption_t co = {
        .input_filename = "test.ir", 
        .binary_filename = "prog.bin", 
        .cft = CFT_BIN, 
        .run = 0, 
        .O = 1, 
        .o = 1, 
        .save_temps = 0, 
        .d = 1, 
    };

    LOG_LEVEL = LP_MINPRIO;
    
    // Hardware step
    System_t* system = system_create(
        1, 
        64, 
        1, 
        1000.0
    );

    if (co.cft >= CFT_CCAN)
    {
        long filesize;
        long lexer_token_count;
        long parser_root_count;
        char* content = read_file(argv[1], &filesize);
        CCANLexerToken_t* lexer = ccan_lexer_parse(content, filesize, &lexer_token_count);

        log_msg(LP_INFO, "token count: %d", lexer_token_count);
        for (int i = 0; i < lexer_token_count; i++) {
            log_msg(LP_INFO, "%d :: %s", i, lexer[i].raw);
        }
        
        CCANParserToken_t** parser = ccan_parser_parse(lexer, lexer_token_count, &parser_root_count);

        log_msg(LP_ERROR, "CCAN compilation is not implemented");

        return 0;
    }


    char* filename = malloc(128);
    sprintf(filename, "%s", co.input_filename);

    // from ir to asm
    if (co.cft >= CFT_IR) {
        char* asm = ir_compile_from_filename(co.input_filename, ((IRCO_POSITION_INDEPENDENT_CODE) | (IRCO_ADD_PREAMBLE)));
        if (!asm) {
            log_msg(LP_ERROR, "IR: Compiler returned NULL [%s:%d]", __FILE__, __LINE__);
            return 1;
        }
        filename = append_filename(filename, ".asm");
        data_export(filename, asm, strlen(asm));
        free(asm);
    }

    // canonicalizes the assembly code to a standard format
    if (co.cft >= CFT_ASM) {
        char* canon_asm = canonicalizer_compile_from_file(filename);
        if (!canon_asm) {
            log_msg(LP_ERROR, "Canonicalizer: Returned NULL [%s:%d]", __FILE__, __LINE__);
            return 1;
        }
        if (co.cft >= CFT_IR && !co.save_temps) {
            remove(filename);
        }
        filename = append_filename(filename, ".can");
        data_export(filename, canon_asm, strlen(canon_asm));
        free(canon_asm);

        // optimizing asm to asm
        if (co.O) {
            char* optimized_asm = optimizer_compile_from_file(filename);
            if (!optimized_asm) {
                log_msg(LP_ERROR, "Optimizer: Returned NULL [%s:%d]", __FILE__, __LINE__);
                return 1;
            }
            if (!co.save_temps) {
                remove(filename);
            }
            filename = append_filename(filename, ".opt");
            data_export(filename, optimized_asm, strlen(optimized_asm));
            free(optimized_asm);
        }

        // expanding asm to macro code (basically emulating higher level asm instructions with more lower level instructions)
        char* expanded_asm = macro_code_expand_from_file(filename, system->cpu->feature_flag);
        if (!expanded_asm) {
            log_msg(LP_ERROR, "Macro expander: Returned NULL [%s:%d]", __FILE__, __LINE__);
            return 1;
        }
        if (!co.save_temps) {
            remove(filename);
        }
        filename = append_filename(filename, ".exp");
        data_export(filename, expanded_asm, strlen(expanded_asm));
        free(expanded_asm);
    }


    long binary_size = 0;
    uint16_t* segment = NULL;
    uint8_t* bin = NULL;
    int segment_count = 0;
    // from asm to bytecode
    if (co.cft > CFT_BIN) {
        bin = assembler_compile_from_file(filename, &binary_size, &segment, &segment_count);

        if (!co.save_temps && co.cft > CFT_BIN) {
            remove(filename);
        }
        
        if (!bin) {
            log_msg(LP_ERROR, "Assembler: Returned NULL [%s:%d]", __FILE__, __LINE__);
            return 0;
        }
        
    } else if (co.cft == CFT_BIN) {

        bin = (uint8_t*) read_file(co.binary_filename, &binary_size);

        if (!bin) {
            log_msg(LP_ERROR, "read_file for \"%s\" Returned NULL [%s:%d]", co.binary_filename, __FILE__, __LINE__);
            return 0;
        }
    }


    if (co.cft > CFT_BIN && co.o) {
        int success = data_export(co.binary_filename, bin, binary_size);
        if (!success) {
            log_msg(LP_ERROR, "data_export returned with failure");
            return 0;
        }
    }

    if (co.d) {
        disassembler_decompile_to_file(bin, "disassemble.asm", binary_size, segment, segment_count, 
            (DO_ADD_JUMP_LABEL | DO_ADD_DEST_LABEL | DO_ADD_SOURCE_LABEL | (0&DO_ADD_LABEL_TO_CODE_SEGMENT) | DO_ADD_SPECULATIVE_CODE | (0&DO_USE_FLOAT_LITERALS) | (0&DO_ALIGN_ADDRESS_JUMP) | (0&DO_ADD_RAW_BYTES)));
    }
    
    free(segment);

    
    if (co.run) {
    
        #ifdef HW_WATCH
            uint16_t match = 0x10ee;
            system_hook(
                system, 
                (Hook_t) {
                    .target = HOOK_TARGET_CPU_PC, 
                    .target_bytes = sizeof(uint16_t), 
                    .match = &match, 
                    .condition = HC_MATCH, 
                    .action = hook_action_halt
                }
            );
        #endif

        for (long i = 0; i < binary_size; i++) {
            ram_write(system->ram, i, bin[i]);
        }

        // Execution step
        for (long long int i = 0; i < 1000000000 && system->cpu->state != CS_HALT && system->cpu->state != CS_EXCEPTION; i++) {
            #ifdef HW_WATCH
                system_clock_debug(system);
            #else
                system_clock(system);
            #endif
        }

        //cpu_print_cache(cpu);
        cpu_print_state(system->cpu);
        cpu_print_stack(system->cpu, system->ram, 20);


        system_delete(&system);
    }

    free(bin);
    

    return 0;
}


