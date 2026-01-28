#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include "cpu/cpu.h"
#include "modules/bus.h"
#include "modules/memory_bank.h"
#include "modules/ram.h"
#include "modules/terminal.h"
#include "modules/ticker.h"

extern int VERBOSE;

typedef enum {
    SCD_CPU, 
    SCD_RAM, 
    SCD_BUS, 
    SCD_TICKER, 
    SCD_TERMINAL, 
    SCD_MEMORY_BANK, 
} SystemClockDevice_t;

typedef enum {
    HC_CHANGE,          // triggers when the target value changes
    HC_MATCH,           // triggers when the target value matches match value
    HC_READ_FROM,       // triggers when the target was read from in ram (currently only RAM can trigger)
    HC_ALWAYS,          // triggers every clock
} HookCondition_t;

typedef struct Hook_t {
    void* target;
    int target_bytes;       // 2 for 16-bit values, 1 for 8-bit, etc.
    void* match;
    HookCondition_t condition;
    void (*action)(void*);
} Hook_t;

typedef struct System_t {
    BUS_t* bus;
    CPU_t* cpu;
    RAM_t* ram;
    Ticker_t* ticker;
    Terminal_t* terminal;
    MemoryBank_t* memory_bank;
    int clock_order_size;
    SystemClockDevice_t* clock_order;
    Hook_t* hook;
    int hook_count;
    uint8_t* memory_intermediate;       // this array stores the value watch values
    int memory_intermediate_size;
} System_t;


extern void hook_action_halt(void* system);
#define hook_action_pass NULL

#define HOOK_TARGET_CPU_R0 ((void*) &system->cpu->regs.r0)
#define HOOK_TARGET_CPU_R1 ((void*) &system->cpu->regs.r1)
#define HOOK_TARGET_CPU_R2 ((void*) &system->cpu->regs.r2)
#define HOOK_TARGET_CPU_R3 ((void*) &system->cpu->regs.r3)
#define HOOK_TARGET_CPU_SP ((void*) &system->cpu->regs.sp)
#define HOOK_TARGET_CPU_PC ((void*) &system->cpu->regs.pc)
#define HOOK_TARGET_CPU_INSTRUCTION ((void*) &system->cpu->instruction)
#define HOOK_TARGET_CPU_CLOCK ((void*) &system->cpu->clock)
#define HOOK_TARGET_RAM(address) ((void*) &system->ram->data[address])



extern System_t* system_create(int cache_active, uint16_t cache_capacity, int ticker_active, float ticker_frequency);

extern void system_delete(System_t** system);

extern void system_clock(System_t* system);

// This function adds a hardware watch that allows for thorough debugging
// These hooks include a watch-target, a trigger condition and an action-on-trigger
extern void system_hook(System_t* system, Hook_t hook);

extern void system_hook_print(System_t* system);

extern void system_clock_debug(System_t *system);

#endif
