#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>

#include "utils/IO.h"
#include "utils/String.h"
#include "utils/Log.h"

#include "cpu/cpu_utils.h"

#include "compiler/ccan/ccan_lexer.h"
#include "compiler/ccan/ccan_parser.h"

#include "compiler/ir/ir_compiler.h"

#include "compiler/asm/assembler.h"
#include "compiler/asm/canonicalizer.h"
#include "compiler/asm/optimizer.h"
#include "compiler/asm/disassembler.h"

#include "modules/system.h"
#include "CLI.h"


#define HW_WATCH
//#undef HW_WATCH


int main(int argc, char* argv[]) {

    LOG_LEVEL = LP_MINPRIO;

    if (argc < 2) {
        log_msg(LP_ERROR, "Main: Not enough arguments given [%s:%d]", __FILE__, __LINE__);
        log_msg(LP_INFO, "Type \"./main help|h|?\" for CLI usage details");
        return 0;
    }

    if (
        strcmp(argv[1], "help") == 0
        || strcmp(argv[1], "h") == 0
        || strcmp(argv[1], "?") == 0
    ) {
        puts(CLI_USAGE);
        return 0;
    }

    int error;
    CompileOption_t co = cli_parse_arguments(argc, argv, &error);
    if (error) {
        log_msg(LP_ERROR, "Main: Error parsing CLI arguments [%s:%d]", __FILE__, __LINE__);
        return 0;
    }
    //log_msg(LP_DEBUG, "Compiling with option: file:%s, bin:%s, c:%d, cft:%d, run:%d, O1:%d, o:%d, save-temps:%d, d:%d", co.input_filename, co.binary_filename, co.c, co.cft, co.run, co.O, co.o, co.save_temps, co.d);

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
        (void) parser;

        log_msg(LP_ERROR, "Main: CCAN compilation is not implemented [%s:%d]", __FILE__, __LINE__);

        return 0;
    }


    char* filename = malloc(128);
    sprintf(filename, "%s", co.input_filename);
    
    // from ir to asm
    if (co.cft >= CFT_IR) {
        char* asm = ir_compile_from_filename(co.input_filename, ((IRCO_POSITION_INDEPENDENT_CODE * co.pic) | (IRCO_ADD_PREAMBLE * (1 - co.no_preamble))));
        if (!asm) {
            log_msg(LP_ERROR, "Main: IR Compiler returned NULL [%s:%d]", __FILE__, __LINE__);
            return 1;
        }
        filename = append_filename(filename, ".asm");
        data_export(filename, asm, strlen(asm));
        free(asm);
    }

    if (co.cft >= CFT_ASM) {
        if (!co.no_c) {
            // canonicalizes the assembly code to a standard format
            char* canon_asm = canonicalizer_compile_from_file(filename);
            if (!canon_asm) {
                log_msg(LP_ERROR, "Main: Canonicalizer returned NULL [%s:%d]", __FILE__, __LINE__);
                return 1;
            }
            if (co.cft >= CFT_IR && !co.save_temps) {
                remove(filename);
            }
            filename = append_filename(filename, ".can");
            data_export(filename, canon_asm, strlen(canon_asm));
            free(canon_asm);
        }

        if (co.O) {
            // optimizing asm to asm
            char* optimized_asm = optimizer_compile_from_file(filename);
            if (!optimized_asm) {
                log_msg(LP_ERROR, "Main: Optimizer returned NULL [%s:%d]", __FILE__, __LINE__);
                return 1;
            }
            if (!co.save_temps && !co.no_c) {
                remove(filename);
            }
            filename = append_filename(filename, ".opt");
            data_export(filename, optimized_asm, strlen(optimized_asm));
            free(optimized_asm);
        }
    }
    

    long binary_size = 0;
    uint16_t* segment = NULL;
    uint8_t* bin = NULL;
    int segment_count = 0;
    // from asm to bytecode
    if (co.cft > CFT_BIN) {
        bin = assembler_compile_from_file (
            filename, 
            &binary_size, 
            &segment, 
            &segment_count, 
            (
                (AO_ERROR_ON_CODE_SEGMENT_BREACH * co.err_csb) | 
                (AO_PAD_SEGMENT_BREACH_WITH_ZERO * co.pad_zero) | 
                (AO_OVERWRITE_ON_OVERLAP * co.overwrite_overlap) | 
                (AO_ERROR_ON_OVERLAP * co.err_overlap)
            )
        );

        if (!co.save_temps && co.cft > CFT_BIN && (!co.no_c || co.O)) {
            remove(filename);
        }
        if (!bin) {
            log_msg(LP_ERROR, "Main: Assembler returned NULL [%s:%d]", __FILE__, __LINE__);
            return 0;
        }
    } else if (co.cft == CFT_BIN) {
        bin = (uint8_t*) read_file(co.binary_filename, &binary_size);

        if (!bin) {
            log_msg(LP_ERROR, "Main: read_file for \"%s\" Returned NULL [%s:%d]", co.binary_filename, __FILE__, __LINE__);
            return 0;
        }
    }


    if (co.cft > CFT_BIN) {
        int success = data_export(co.binary_filename, bin, binary_size);
        if (!success) {
            log_msg(LP_ERROR, "Main: data_export returned with failure");
            return 0;
        }
    }

    if (co.d) {
        disassembler_decompile_to_file(bin, "disassemble.asm", binary_size, segment, segment_count, 
            (DO_ADD_JUMP_LABEL | DO_ADD_DEST_LABEL | DO_ADD_SOURCE_LABEL | (0&DO_ADD_LABEL_TO_CODE_SEGMENT) | (DO_ADD_SPECULATIVE_CODE) | (0&DO_USE_FLOAT_LITERALS) | (0&DO_ALIGN_ADDRESS_JUMP) | (DO_ADD_RAW_BYTES)));
    }
    
    free(segment);

    
    if (co.run) {
    
        // Hardware setup
        System_t* system = system_create(
            co.cache_size != 0, 
            co.cache_size, 
            1, 
            1000.0
        );

        if (!system) {
            log_msg(LP_ERROR, "Main: System could not be created [%s:%d]", __FILE__, __LINE__);
            return 0;
        }
    
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
        for (long long int i = 0; i < 10000000 && system->cpu->state != CS_HALT && system->cpu->state != CS_EXCEPTION; i++) {
            #ifdef HW_WATCH
                system_clock_debug(system);
            #else
                system_clock(system);
            #endif
        }

        cpu_print_state(system->cpu);
        //cpu_print_stack(system->cpu, system->ram, 20);
        //cpu_print_cache(system->cpu);

        system_delete(&system);
    }

    free(bin);


    return 0;
}


