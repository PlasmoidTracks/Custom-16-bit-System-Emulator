#ifndef _CPU_UTILS_H_
#define _CPU_UTILS_H_

#include <stdint.h>

#include "cpu.h"
#include "ram.h"


extern void cpu_print_cache(CPU_t* cpu);

extern void cpu_print_register(char* name, uint16_t value);

// Fancy function to print CPU State
extern void cpu_print_state(CPU_t* cpu);

extern void cpu_print_state_compact(CPU_t* cpu);

extern void cpu_print_stack(CPU_t* cpu, RAM_t* ram, int count);

extern void cpu_print_stack_compact(CPU_t* cpu, RAM_t* ram, int count);


#endif

