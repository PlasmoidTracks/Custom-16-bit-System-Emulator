#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>

#include "IO.h"
#include "Log.h"

#include "String.h"
#include "bus.h"
#include "cache.h"
#include "cpu/cpu.h"
#include "cpu/cpu_utils.h"
#include "ram.h"
#include "ticker.h"

#include "asm/assembler.h"
#include "asm/disassembler.h"
#include "asm/optimizer.h"
#include "asm/macro_code_expansion.h"

#include "codegen/ir_lexer.h"
#include "codegen/ir_parser.h"
#include "codegen/ir_compiler.h"

#include "transpile/ira_lexer.h"
#include "transpile/ira_parser.h"
#include "transpile/ira_compiler.h"




#define COMPILE_IRA
#undef COMPILE_IRA

#define COMPILE_IR
#undef COMPILE_IR


int main(int argc, char* argv[]) {

    if (argc < 2) {
        log_msg(LP_ERROR, "Usage: ./main [filename.ir]");
        return 0;
    }

    LOG_LEVEL = LP_MINPRIO;
    
    // Hardware step
    BUS_t* bus = bus_create();
    CPU_t* cpu = cpu_create();
    RAM_t* ram = ram_create(1 << 16);
    Cache_t* cache = cache_create(64);
    cpu_mount_cache(cpu, cache);                      // cache is only faster when ram is slow. thus high bus/ram speed is better than cache
    Ticker_t* ticker = ticker_create(1000.0);

    /*GenericClocker_t gc = {
        .clock = (void(*)(void*)) cpu_clock, 
        .device = (void*) cpu, 
    };*/

    bus_add_device(bus, &cpu->device);
    bus_add_device(bus, &ram->device);
    bus_add_device(bus, &ticker->device);
    




    // Compiling step
    // from ira to ir
    long content_size;
    long lexer_token_count = 0;
    char* content;
    #ifdef COMPILE_IRA
    content = read_file(argv[1], &content_size);

    IRALexerToken_t* ira_lexer_token = ira_lexer_parse(content, content_size, &lexer_token_count);
    if (!ira_lexer_token) {
        log_msg(LP_ERROR, "lexer returned NULL");
        return 1;
    }

    long ira_parser_root_count = 0;
    IRAParserToken_t** ira_parser_token = ira_parser_parse(ira_lexer_token, lexer_token_count, &ira_parser_root_count);

    char* ir = ira_compile(ira_parser_token, ira_parser_root_count, 0xffffffff);
    if (!ir) {
        log_msg(LP_ERROR, "ira compiler returned NULL");
        return 1;
    }
    #endif

    char** words = split(argv[1], ".", "");
    char filename[128];
    sprintf(filename, "%s.ir", words[0]);
    #ifdef COMPILE_IRA
    data_export(filename, ir, strlen(ir));
    #endif


    // from ir to asm
    #ifdef COMPILE_IR
    content = read_file(filename, &content_size);
    IRLexerToken_t* lexer_token = ir_lexer_parse(content, content_size, &lexer_token_count);
    if (!lexer_token) {
        log_msg(LP_ERROR, "lexer returned NULL");
        return 1;
    }

    long parser_root_count = 0;
    IRParserToken_t** parser_token = ir_parser_parse(lexer_token, lexer_token_count, &parser_root_count);

    char* asm = ir_compile(parser_token, parser_root_count, 0xffffffff);
    if (!asm) {
        log_msg(LP_ERROR, "ir compiler returned NULL");
        return 1;
    }
    #endif

    words = split(argv[1], ".", "");
    sprintf(filename, "%s.asm", words[0]);
    #ifdef COMPILE_IR
    data_export(filename, asm, strlen(asm));
    #endif

    // expanding asm to macro code (basically emulating higher level asm instructions with more lower level instructions)
    char* expanded_asm = macro_code_expand_from_file(filename);
    sprintf(filename, "%s.exp.asm", words[0]);
    data_export(filename, expanded_asm, strlen(expanded_asm));

    // optimizing asm to asm
    char* optimized_asm = optimizer_compile_from_file(filename);
    if (!optimized_asm) {
        log_msg(LP_ERROR, "optimizer returned NULL");
        return 1;
    }
    sprintf(filename, "%s.opt.exp.asm", words[0]);
    data_export(filename, optimized_asm, strlen(optimized_asm));


    // from asm to bytecode
    long binary_size = 0;
    uint16_t* segment = NULL;
    int segment_count = 0;
    uint8_t* bin = assembler_compile_from_file(filename, &binary_size, &segment, &segment_count);
    
    if (!bin) {
        log_msg(LP_ERROR, "Compiler returned NULL");
        return 0;
    }

    for (long i = 0; i < binary_size; i++) {
        ram_write(ram, i, bin[i]);
    }

    disassembler_decompile_to_file(ram->data, "disassemble.asm", binary_size, segment, segment_count, 
        (DO_ADD_JUMP_LABEL | DO_ADD_DEST_LABEL | DO_ADD_SOURCE_LABEL | (0&DO_ADD_LABEL_TO_CODE_SEGMENT) | DO_ADD_SPECULATIVE_CODE | (0&DO_USE_FLOAT_LITERALS) | DO_ALIGN_ADDRESS_JUMP));
    free(segment);


    

    // Execution step
    uint16_t min_sp = cpu->regs.sp;
    for (int i = 0; i < 100000000 && cpu->state != CS_HALT && cpu->state != CS_EXCEPTION; i++) {
        //int test = 0;
        //if (cpu->state == CS_EXECUTE) {test = 1;}
        //cpu_clock(cpu);
        cpu_clock(cpu);
        bus_clock(bus);
        ram_clock(ram);
        bus_clock(bus);
        //ticker_clock(ticker);
        //if (test) {cpu_print_state_compact(cpu);
        //cpu_print_stack_compact(cpu, ram, 12);
        //printf("\n");}
        if (cpu->regs.sp < min_sp) {min_sp = cpu->regs.sp;}
    }
    printf("%.4x\n", min_sp);

    //cpu_print_cache(cpu);
    cpu_print_state(cpu);
    cpu_print_stack(cpu, ram, 20);

    ram_delete(ram);
    cpu_delete(cpu);
    bus_delete(bus);

    return 0;
}

