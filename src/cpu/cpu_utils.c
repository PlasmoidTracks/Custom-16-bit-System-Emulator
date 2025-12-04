#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "utils/ExtendedTypes.h"
#include "utils/String.h"

#include "cpu/cpu.h"
#include "cpu/cpu_instructions.h"
#include "ram.h"


// Fancy function to print CPU Cache
void cpu_print_cache(CPU_t* cpu) {
    if (!cpu->cache) {
        printf("\n\033[1;34m============== NO CACHE AVAILABLE ===============\033[0m\n");
        return;
    }
    printf("\n\033[1;34m=================== CPU CACHE ===================\033[0m\n");
    printf("\033[1;36m Index | Address  | Data   | Valid | Dirty |  Uses |  Age \033[0m\n");
    printf("-------------------------------------------------\n");
    
    for (int i = 0; i < cpu->cache->capacity; i++) {
        printf("\033[1;32m   %2d  |  0x%04X  |  0x%02X  |   %d   |   %d   |  %3d  |  %3d  \033[0m\n", 
            i, 
            cpu->cache->address[i], 
            cpu->cache->data[i], 
            cpu->cache->state[i].valid, 
            cpu->cache->state[i].dirty, 
            cpu->cache->state[i].uses, 
            cpu->cache->state[i].age);
    }
    printf("\033[1;34m================================================\033[0m\n\n");
}

static char* cpu_ascii_to_string(uint16_t value) {
    char character = (char)(value & 0x00ff);
    char* output = calloc(5, 1); // enough space for longest escape + null

    switch (character) {
        case '\0':  sprintf(output, "\\0");  break;
        case '\a':  sprintf(output, "\\a");  break;
        case '\b':  sprintf(output, "\\b");  break;
        case '\t':  sprintf(output, "\\t");  break;
        case '\n':  sprintf(output, "\\n");  break;
        case '\v':  sprintf(output, "\\v");  break;
        case '\f':  sprintf(output, "\\f");  break;
        case '\r':  sprintf(output, "\\r");  break;
        case '\"':  sprintf(output, "\"");  break;
        case '\'':  sprintf(output, "\'");  break;
        case '\\':  sprintf(output, "\\\\"); break;
        default:
            if (character >= 0x20 && character <= 0x7e) {
                // printable ASCII
                sprintf(output, "%c", character);
            } else {
                // non-printable, non-standard control characters
                sprintf(output, "\\x%02X", (unsigned char)character);
            }
            break;
    }

    return output;
}


void cpu_print_register(char* name, uint16_t value) {
    char* ascii = cpu_ascii_to_string(value);
    char buf1[128];
    char buf2[128];
    format_float_to_scientific_notation(buf1, float_from_f16((float16_t) value));
    format_float_to_scientific_notation(buf2, float_from_bf16((bfloat16_t) value));
    printf(" \033[1;32m%s\033[0m  X: 0x%04X | S: %6d | F16: %-15s | BF16: %-15s | C: '%s'\n",
        name, value, (int16_t) value, buf1, buf2, ascii);
    free(ascii);
}

// Fancy function to print CPU State
void cpu_print_state(CPU_t* cpu) {
    printf("\n\033[1;35m=============== CPU STATE ===============\033[0m\n");

    // General Registers with Multiple Representations
    printf("\033[1;33m Registers\033[0m\n");

    cpu_print_register("R0", cpu->regs.r0);
    cpu_print_register("R1", cpu->regs.r1);
    cpu_print_register("R2", cpu->regs.r2);
    cpu_print_register("R3", cpu->regs.r3);
    
    // Stack and Program Counter
    printf(" \033[1;32mSP\033[0m  0x%04X  \033[1;32mPC\033[0m  0x%04X\n",
        cpu->regs.sp, cpu->regs.pc);

    // Status Register (Full Value)
    printf("\n\033[1;33m Status Register\033[0m\n");
    printf(" \033[1;32mSR\033[0m  0x%04X\n", cpu->regs.sr.value);

    // Individual Flags
    printf(" \033[1;32mZ/E\033[0m [%d]  \033[1;32mN/L\033[0m [%d]  \033[1;32mUL\033[0m  [%d]  \033[1;32mFL\033[0m  [%d]\n"
           " \033[1;32mAO\033[0m  [%d]  \033[1;32mSRC\033[0m [%d]  \033[1;32mSWC\033[0m [%d]  \033[1;32mMI\033[0m  [%d]\n",
        cpu->regs.sr.Z, cpu->regs.sr.L, cpu->regs.sr.UL, cpu->regs.sr.FL, 
        cpu->regs.sr.AO, cpu->regs.sr.SRC, cpu->regs.sr.SWC, cpu->regs.sr.MI);
    
    // Cache
    if (!cpu->cache) {
        printf("\n\033[1;33m No cache available\033[0m\n");
    } else {
        printf("\n\033[1;33m Cache\033[0m\n");
        printf(" \033[1;32mHits\033[0m [%lu]  \033[1;32mMiss\033[0m [%lu]  \033[1;32mRate\033[0m [%2.2f%%]\n", cpu->cache->hit, cpu->cache->miss, (double) cpu->cache->hit / (double) (cpu->cache->hit + cpu->cache->miss) * 100.0);
    }

    // Other
    printf("\n\033[1;33m Other\033[0m\n");
    printf(" \033[1;32mclock\033[0m    %-12ld\n", cpu->clock);
    printf(" \033[1;32minstr\033[0m    %-12ld\n", cpu->instruction);
    printf(" \033[1;32mcpi\033[0m      %2.3f\n", (double) cpu->clock / (double) cpu->instruction);
    printf(" \033[1;32mstate\033[0m    %d [%s]\n", cpu->state, cpu_state_name[cpu->state]);
    if (cpu->last_instruction < INSTRUCTION_COUNT) {
        printf(" \033[1;32mLast Instruction\033[0m %s (%d)\n", cpu_instruction_string[cpu->last_instruction], (int) cpu->last_instruction);
    } else {
        printf(" \033[1;32mLast Instruction\033[0m ??? (%d)\n", (int) cpu->last_instruction);
    }

    printf("\033[1;35m========================================\033[0m\n\n");
}

void cpu_print_state_compact(CPU_t* cpu) {
    printf("\033[1;35m[CPU STATE]\033[0m ");
    printf("\033[1;32mR0\033[0m: 0x%04X  ", cpu->regs.r0);
    printf("\033[1;32mR1\033[0m: 0x%04X  ", cpu->regs.r1);
    printf("\033[1;32mR2\033[0m: 0x%04X  ", cpu->regs.r2);
    printf("\033[1;32mR3\033[0m: 0x%04X  ", cpu->regs.r3);
    printf("\033[1;32mSP\033[0m: 0x%04X  ", cpu->regs.sp);
    printf("\033[1;32mPC\033[0m: 0x%04X\n", cpu->regs.pc);
}


void cpu_print_stack(CPU_t* cpu, RAM_t* ram, int count) {
    if (!cpu || count <= 0) return;

    printf("\n\033[1;36m=============================== CPU STACK ===============================\033[0m\n");
    printf(" \033[1;33m   Addr   |  Hex   |  Int   |     Float16     |    BFloat16     | Regs \033[0m\n");
    printf("-------------------------------------------------------------------------\n");

    uint16_t base = cpu->memory_layout.segment_stack;

    for (int i = 0; i < count; ++i) {
        uint16_t address = base - i * 2;
        uint16_t value = ram->data[address - 1] | (ram->data[address] << 8);
        char* ascii = cpu_ascii_to_string(value);

        int16_t signed_val = (int16_t)value;

        // Collect register names that match this address
        char reg_label[32] = "";
        if (address - 1 == cpu->regs.r0) strcat(reg_label, "r0/");
        if (address - 1 == cpu->regs.r1) strcat(reg_label, "r1/");
        if (address - 1 == cpu->regs.r2) strcat(reg_label, "r2/");
        if (address - 1 == cpu->regs.r3) strcat(reg_label, "r3/");
        if (address - 1 == cpu->regs.sp) strcat(reg_label, "sp/");

        // Trim trailing slash if any
        size_t len = strlen(reg_label);
        if (len > 0) reg_label[len - 1] = '\0';

        char buf1[128];
        char buf2[128];
        format_float_to_scientific_notation(buf1, float_from_f16((float16_t) value));
        format_float_to_scientific_notation(buf2, float_from_bf16((bfloat16_t) value));

        if (len > 0) {
            printf("  \033[1;32m0x%04X/%.1X | 0x%04X | %-6d | %-15s | %-15s | <-- %s \033[0m",
                address - 1, address & 0x000f, value, signed_val, buf1, buf2, reg_label);
        } else {
            printf("  0x%04X/%.1X | 0x%04X | %-6d | %-15s | %-15s |",
                address - 1, address & 0x000f, value, signed_val, buf1, buf2);
        }
        if (i < count - 1) {printf("\n");}

        free(ascii);
    }

    printf("\n\033[1;36m=========================================================================\033[0m\n");
}

void cpu_print_stack_compact(CPU_t* cpu, RAM_t* ram, int count) {
    if (!cpu || count <= 0) return;

    printf("\033[1;36m[STACK]\033[0m ");

    uint16_t base = cpu->memory_layout.segment_stack;

    for (int i = 0; i < count; ++i) {
        uint16_t address = base - i * 2;
        uint16_t value = ram->data[address] | (ram->data[address + 1] << 8);

        // Build a register label prefix
        char label[32] = "";
        if (address == cpu->regs.r0) strcat(label, "r0/");
        if (address == cpu->regs.r1) strcat(label, "r1/");
        if (address == cpu->regs.r2) strcat(label, "r2/");
        if (address == cpu->regs.r3) strcat(label, "r3/");
        if (address == cpu->regs.sp) strcat(label, "sp/");

        // Trim trailing slash
        size_t len = strlen(label);
        if (len > 0) {
            label[len - 1] = '\0';
            printf("\033[1;32m[%s:0x%04X]\033[0m ", label, value);
        } else {
            printf("0x%04X ", value);
        }
    }

    printf("\n");
}

