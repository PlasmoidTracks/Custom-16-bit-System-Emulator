#ifndef _DISASSEMBLER_H_
#define _DISASSEMBLER_H_

#include "cpu/cpu_addressing_modes.h"

#include <stdint.h>

typedef enum {
    DO_ADD_JUMP_LABEL = 0001,               // this option adds .jump_label to every jump instruction                   (default: 1)
    DO_ADD_DEST_LABEL = 0002,               // this option adds .dest_label to every indirect immediate destination     (default: 1)
    DO_ADD_SOURCE_LABEL = 0004,             // this option adds .dest_label to every indirect immediate source          (default: 1)
    DO_ADD_LABEL_TO_CODE_SEGMENT = 0010,    // this option adds labels even when the target is within a .code segment   (default: 0)
    DO_ADD_SPECULATIVE_CODE = 0020,         // this option adds speculative code in comments in data segments whenever the data aligns with real code, but contain an invalid addressing mode
    DO_USE_FLOAT_LITERALS = 0040,           // this option replaces immediate values with floats where probable
    DO_ALIGN_ADDRESS_JUMP = 0100,           // this option forces the disassembler to align to 2 bytes, especially when jumping using .address
    DO_ADD_RAW_BYTES = 0200,                // this option adds the raw bytes for each instruction as a comment
} DisassembleOption_t;

typedef struct Disassembly_t {
    char (*code) [256];
    int* binary_index;
    CPU_EXTENDED_ADDRESSING_MODE_t* admx;
    CPU_REDUCED_ADDRESSING_MODE_t* admr;
    int* is_label;
    int lines;
    int* is_data;
} Disassembly_t;

extern char* disassembler_decompile_single_instruction(uint8_t* binary, int* binary_index, int* valid_instruction, CPU_REDUCED_ADDRESSING_MODE_t* extern_admr, CPU_EXTENDED_ADDRESSING_MODE_t* extern_admx, int* instruction_bytes, DisassembleOption_t options);

extern char* disassembler_decompile(uint8_t* machine_code, long binary_size, uint16_t* segment, int segment_count, DisassembleOption_t options);

extern void disassembler_decompile_to_file(uint8_t* machine_code, const char filename[], long binary_size, uint16_t* segment, int segment_count, DisassembleOption_t options);

#endif
