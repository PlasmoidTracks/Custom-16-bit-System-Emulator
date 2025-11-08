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
#include "include/utils/Log.h"
#include "ram.h"
#include "ticker.h"

#include "asm/assembler.h"
#include "asm/disassembler.h"
#include "asm/optimizer.h"
#include "asm/canonicalizer.h"
#include "asm/macro_code_expansion.h"

#include "codegen/ir_lexer.h"
#include "codegen/ir_parser.h"
#include "codegen/ir_compiler.h"

#include "transpile/ira_lexer.h"
#include "transpile/ira_parser.h"
#include "transpile/ira_compiler.h"


// This script aims at checking that every possible decombile options combination, when compiled, still results in the exact same binary as the raw program. 


int main(int argc, char* argv[]) {

    if (argc < 2) {
        log_msg(LP_ERROR, "Usage: ./main [filename.ir]");
        return 0;
    }

    long binary_size = 0;
    uint16_t* segment = NULL;
    int segment_count = 0;
    uint8_t* bin = assembler_compile_from_file(argv[1], &binary_size, &segment, &segment_count);
    
    if (!bin) {
        log_msg(LP_ERROR, "Compiler returned NULL");
        return 0;
    }


    for (int option = 0; option < (DO_ALIGN_ADDRESS_JUMP << 1); option ++) {
        long new_binary_size = 0;
        uint16_t* new_segment = NULL;
        int new_segment_count = 0;
        // decompile
        char* decomp = disassembler_decompile(bin, binary_size, segment, segment_count, option);
        // recompile
        uint8_t* new_bin = assembler_compile(decomp, &new_binary_size, &new_segment, &new_segment_count);
        // validate
        int err = 0;
        if (binary_size != new_binary_size) {
            log_msg(LP_ERROR, "[option: %.2x] binary_size do not match (%ld vs %ld)", option, binary_size, new_binary_size);
            err = 1;
        }
        for (int i = 0; i < (binary_size < new_binary_size ? binary_size : new_binary_size); i++) {
            if (bin[i] != new_bin[i]) {
                log_msg(LP_ERROR, "[option: %.2x] binaries do not match", option);
                err = 1;
                break;
            }
        }
        if (!err) {
            log_msg(LP_SUCCESS, "[option: %.2x] match", option);
        }
        free(new_bin);
        free(new_segment);
    }


    
    free(segment);    

    return 0;
}
