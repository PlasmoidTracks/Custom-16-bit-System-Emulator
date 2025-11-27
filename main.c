#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>

#include "IO.h"
#include "Log.h"

#include "String.h"
#include "bus.h"
#include "cpu/cpu.h"
#include "cpu/cpu_utils.h"
#include "include/utils/Log.h"
#include "ram.h"
#include "system.h"

#include "asm/assembler.h"
#include "asm/disassembler.h"
#include "asm/optimizer.h"
#include "asm/canonicalizer.h"
#include "asm/macro_code_expansion.h"

#include "codegen/ir_lexer.h"
#include "codegen/ir_parser.h"
#include "codegen/ir_compiler.h"

#include "transpile/ccan/ccan_lexer.h"
#include "transpile/ccan/ccan_parser.h"
#include "transpile/ira_lexer.h"
#include "transpile/ira_parser.h"
#include "transpile/ira_compiler.h"



#define CCAN
//#undef CCAN

#define COMPILE_IR
//#undef COMPILE_IR

#define CANONICALIZE
//#undef CANONICALIZE

#define MACRO_EXPAND
//#undef MACRO_EXPAND

#define OPTIMIZE
//#undef OPTIMIZE

#define DISASSEMBLE
//#undef DISASSEMBLE

#define BINARY_DUMP
//#undef BINARY_DUMP

#define HW_WATCH
#undef HW_WATCH

int main(int argc, char* argv[]) {

    if (argc < 2) {
        log_msg(LP_ERROR, "Not enough arguments given");
        log_msg(LP_INFO, "Usage: ./main [filename.ir]");
        return 0;
    }

    LOG_LEVEL = LP_MINPRIO;
    
    // Hardware step
    System_t* system = system_create(
        1, 
        64, 
        1, 
        1000.0
    );

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

    long content_size;
    long lexer_token_count = 0;
    long parser_root_count = 0;
    char* content;

    #ifdef CCAN
        {
            long filesize;
            char* content = read_file(argv[1], &filesize);
            CCANLexerToken_t* lexer = ccan_lexer_parse(content, filesize, &lexer_token_count);

            log_msg(LP_INFO, "token count: %d", lexer_token_count);
            for (int i = 0; i < lexer_token_count; i++) {
                log_msg(LP_INFO, "%d :: %s", i, lexer[i].raw);
            }
            
            CCANParserToken_t** parser = ccan_parser_parse(lexer, lexer_token_count, &parser_root_count);

            return 0;
        }
    #endif



    char** words = split(argv[1], ".", "");
    char* filename = malloc(128);
    sprintf(filename, "%s", words[0]);

    // Compiling step
    // from ira to ir

    sprintf(filename, "%s", argv[1]);

    // from ir to asm
    #ifdef COMPILE_IR
        content = read_file(filename, &content_size);
        if (!content) {
            log_msg(LP_ERROR, "IR: read_file failed");
            return 1;
        }
        IRLexerToken_t* lexer_token = ir_lexer_parse(content, content_size, &lexer_token_count);
        if (!lexer_token) {
            log_msg(LP_ERROR, "IR: Lexer returned NULL");
            return 1;
        }
        IRParserToken_t** parser_token = ir_parser_parse(lexer_token, lexer_token_count, &parser_root_count);
        if (!lexer_token) {
            log_msg(LP_ERROR, "IR: Parser returned NULL");
            return 1;
        }
        char* asm = ir_compile(parser_token, parser_root_count, 0xffffffff);
        if (!asm) {
            log_msg(LP_ERROR, "IR: Compiler returned NULL");
            return 1;
        }
        filename = append_filename(filename, ".asm");
        data_export(filename, asm, strlen(asm));
    #endif

    // canonicalizes the assembly code to a standard format
    #ifdef CANONICALIZE
        char* canon_asm = canonicalizer_compile_from_file(filename);
        if (!canon_asm) {
            log_msg(LP_ERROR, "Canonicalizer: Returned NULL");
            return 1;
        }
        filename = append_filename(filename, ".can");
        data_export(filename, canon_asm, strlen(canon_asm));
    #endif

    // expanding asm to macro code (basically emulating higher level asm instructions with more lower level instructions)
    #ifdef MACRO_EXPAND
        char* expanded_asm = macro_code_expand_from_file(filename);
        if (!expanded_asm) {
            log_msg(LP_ERROR, "Macro expander: Returned NULL");
            return 1;
        }
        filename = append_filename(filename, ".exp");
        data_export(filename, expanded_asm, strlen(expanded_asm));
    #endif

    // optimizing asm to asm
    #ifdef OPTIMIZE
        char* optimized_asm = optimizer_compile_from_file(filename);
        if (!optimized_asm) {
            log_msg(LP_ERROR, "Optimizer: Returned NULL");
            return 1;
        }
        filename = append_filename(filename, ".opt");
        data_export(filename, optimized_asm, strlen(optimized_asm));
    #endif


    // from asm to bytecode
    long binary_size = 0;
    uint16_t* segment = NULL;
    int segment_count = 0;
    uint8_t* bin = assembler_compile_from_file(filename, &binary_size, &segment, &segment_count);
    
    if (!bin) {
        log_msg(LP_ERROR, "Assembler: Returned NULL");
        return 0;
    }

    #ifdef BINARY_DUMP
        data_export("dump.bin", bin, binary_size);
    #endif

    for (long i = 0; i < binary_size; i++) {
        ram_write(system->ram, i, bin[i]);
    }

    #ifdef DISASSEMBLE
        disassembler_decompile_to_file(system->ram->data, "disassemble.asm", binary_size, segment, segment_count, 
            (DO_ADD_JUMP_LABEL | DO_ADD_DEST_LABEL | DO_ADD_SOURCE_LABEL | (0&DO_ADD_LABEL_TO_CODE_SEGMENT) | DO_ADD_SPECULATIVE_CODE | (0&DO_USE_FLOAT_LITERALS) | (0&DO_ALIGN_ADDRESS_JUMP) | (0&DO_ADD_RAW_BYTES)));
    #endif
    
    free(segment);

    // Execution step
    //uint16_t min_sp = cpu->regs.sp;
    for (int i = 0; i < 100000000 && system->cpu->state != CS_HALT && system->cpu->state != CS_EXCEPTION; i++) {
        #ifdef HW_WATCH
            system_clock_debug(system);
        #else
            system_clock(system);
        #endif
    }

    //cpu_print_cache(cpu);
    cpu_print_state(system->cpu);
    cpu_print_stack(system->cpu, system->ram, 20);

    ram_delete(system->ram);
    cpu_delete(system->cpu);
    bus_delete(system->bus);

    return 0;
}


