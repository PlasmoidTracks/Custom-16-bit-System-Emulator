#ifndef _CPU_H_
#define _CPU_H_

#include <stdint.h>

#include "modules/device.h"
#include "modules/ram.h"
#include "modules/cache.h"

#include "cpu/cpu_instructions.h"

/*
Instructions are encoded as follows: 
first 6 bits are the instruction [i]
if the instruction has arguments, then the next 10 bits encode the two addressing modes [adm], 5 bits each [w/x]
xxxxxwww iiiiiiii
The next bytes are the arguments for the first adm (0, 1, 2 or 3 bytes, depending on the adm)
After those, the next bytes are the arguments for the second adm (0, 1, 2 or 3 bytes again)

what about cache modifiers?!


6 bits instruction
2 bits UNUSED / For future expansion
3 bits reduced adm for first argument / destination argument
5 bits extended adm for second argument / source argument

instructions with two arguments use the reduced adm for the dest and the extended adm for the dest
instructions with one argument that only reads use the extended adm, ones that write use reduced adm


Interrupts do NOT push the registers, you gotta do it in the IRQ vector yourself bucko. 
I aint implementing the push-low push-high CS's for that and also implement iret, hellnah


push :: sp--; src -> [sp]           works ONLY with admx argument
pop  :: [sp] -> dest; sp++          works ONLY with admr argument

*/



typedef enum CpuState_t {
    CS_FETCH_INSTRUCTION,                       // fetching the next instruction [reapeat until read successful]. This is also the one where we check for interrupts
    CS_FETCH_ADDRESSING_MODES,       // decode the instruction to get the argument count of the instruction
    CS_FETCH_ARGUMENT_BYTES, 
    CS_COMPUTE_ADDRESS,    // computes the absolute address from the addressing mode and arguments
    CS_FETCH_SOURCE,                // fetches data from the indirect source addressing
    CS_FETCH_SOURCE_HIGH, 
    CS_FETCH_DESTINATION,           // fetches data from the indirect destination addressing
    CS_FETCH_DESTINATION_HIGH, 
    CS_EXECUTE,                     // processes instruction and saves intermediate result for possible writeback
    CS_WRITEBACK_LOW,                   // writes result back to memory
    CS_WRITEBACK_HIGH,                   // writes result back to memory
    CS_PUSH_LOW,                    // push and pop are unique, thats why
    CS_PUSH_HIGH, 
    CS_POP_LOW, 
    CS_POP_HIGH, 
    CS_POPSR_LOW, 
    CS_POPSR_HIGH, 
    CS_RET_LOW, 
    CS_RET_HIGH, 

    CS_INTERRUPT_PUSH_PC_HIGH, 
    CS_INTERRUPT_PUSH_PC_LOW, 
    CS_INTERRUPT_FETCH_IRQ_VECTOR_LOW, 
    CS_INTERRUPT_FETCH_IRQ_VECTOR_HIGH, 
    CS_INTERRUPT_JUMP, 

    CS_HALT,                    // halts all execution indefinitely

    CS_EXCEPTION, 
    CS_COUNT, 
} CpuState_t;

extern const char* cpu_state_name[CS_COUNT];

typedef struct CpuIntermediate_t {
    uint8_t instruction;
    union {
        uint8_t value;
        struct {
            uint8_t addressing_mode_reduced : 3;
            uint8_t addressing_mode_extended : 5;
        };
    } addressing_mode;
    uint16_t previous_pc;
    uint16_t previous_sp;
    int argument_count;
    int8_t argument_bytes_to_load;
    int argument_data_raw_index;
    int8_t argument_data_raw[5];    // 5 bytes since thats the max amount of memory an isntruction can require
    uint16_t argument_address_reduced;  // the address the reduced adm is pointing to
    uint16_t argument_address_extended; // the address the extended adm is pointing to
    uint16_t data_address_reduced;  // the address the reduced adm is pointing to
    uint16_t data_address_extended; // the address the extended adm is pointing to
    int argument_count_for_admr;
    uint16_t result;
    uint16_t irq_id;
} CpuIntermediate_t;


/*
At the current time, dirty cache is equivalent to invalid cache. 
This mean no RAM writebacks are done on dirty cache lines, though this behaviour might change in the future
*/

typedef struct CpuMemoryLayout_t {
    uint16_t segment_data;      // random memory access begin
    uint16_t segment_code;      // machine code begin
    uint16_t segment_stack;     // stack
    uint16_t segment_mmio;      // other devices
    uint16_t segment_irq_table; // lookup table for interrupt subroutines
} CpuMemoryLayout_t;

typedef struct CPU_t {
    uint64_t clock;             // keeps track of the number of cycles
    uint64_t instruction;       // keeps track of the number of executed instructions
    CPU_INSTRUCTION_MNEMONIC_t last_instruction; // the last executed/pending instruction of the cpu

    uint16_t feature_flag;

    Cache_t* cache;

    union {
        struct {
            uint16_t r0, r1, r2, r3, pc, sp;
            union {  // Union to allow `sr` to be accessed as both uint16_t and bitfield
                uint16_t value;  // Full status register
                struct {
                    uint16_t Z : 1;                             // [Z]ero result / [E]qual
                    uint16_t FZ: 1;                             // [F]loat [Z]ero result / [F]loat [E]qual (includes 0.0 == -0.0)
                    uint16_t L : 1;                             // [N]egative / signed [L]ess
                    uint16_t UL : 1;                            // [U]nsigned [L]ess
                    uint16_t FL : 1;                            // [F]loat [L]ess
                    uint16_t BL : 1;                            // [B]float [L]ess
                    uint16_t AO : 1;                            // [A]rithmetic [O]verflow
                    uint16_t SRC : 1;                           // [S]kip [R]eading [C]ache     (when set, skip looking through cache first)
                    uint16_t SWC : 1;                           // [S]kip [W]riting [C]ache     (when set, skip updating cache)
                    uint16_t MI : 1;                            // [M]ask [I]nterrupts          (when set, hardware interrupts from the bus will be ignored)
                    uint16_t MNI : 1;                           // [M]ask [N]onmaskable [I]nterrupts    (internal flag ONLY, no user control, this is for cpu cycle management)
                    uint16_t reserved : 5;                      // Ensures correct size
                };
            } sr;
        } regs;
    };


    CpuState_t state;
    CpuIntermediate_t intermediate;
    
    Device_t device;
} CPU_t;


extern CPU_t* cpu_create(void);

extern uint16_t cpu_generate_feature_flag(void);

extern void cpu_delete(CPU_t** cpu);

extern void cpu_mount_cache(CPU_t* cpu, Cache_t* cache);

extern void cpu_print_cache(CPU_t* cpu);

extern void cpu_print_state(CPU_t* cpu);

extern void cpu_print_state_compact(CPU_t* cpu);

extern void cpu_print_stack(CPU_t* cpu, RAM_t* ram, int count);

extern void cpu_print_stack_compact(CPU_t* cpu, RAM_t* ram, int count);

extern int cpu_read_memory(CPU_t* cpu, uint16_t address, uint8_t *data);

extern int cpu_write_memory(CPU_t* cpu, uint16_t address, uint8_t data);

extern void cpu_clock(CPU_t* cpu);


#endif
