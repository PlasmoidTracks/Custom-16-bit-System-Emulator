#include <stdlib.h>
#include <stdio.h>
#include "Log.h"
#include "bus.h"
#include "cpu/cpu.h"
#include "ticker.h"
#include "system.h"

void hook_action_halt(System_t* system) {
    system->cpu->state = CS_HALT;
}

System_t* system_create(
    int cache_active, uint16_t cache_capacity, 
    int ticker_active, float ticker_frequency
) {
    System_t* system = calloc(1, sizeof(System_t));

    BUS_t* bus = bus_create();
    system->bus = bus;
    CPU_t* cpu = cpu_create();
    system->cpu = cpu;
    RAM_t* ram = ram_create(1 << 16);
    system->ram = ram;

    if (cache_active) {
        Cache_t* cache = cache_create(cache_capacity);
        cpu_mount_cache(cpu, cache);
    }

    bus_add_device(bus, &cpu->device);
    bus_add_device(bus, &ram->device);

    if (ticker_active) {
        Ticker_t* ticker = ticker_create(ticker_frequency);
        bus_add_device(bus, &ticker->device);
        system->ticker = ticker;
    }

    system->bus = bus;

    system->clock_order_size = 0;
    system->clock_order = malloc(sizeof(SystemClockDevice_t) * 16);
    system->clock_order[system->clock_order_size++] = SCD_CPU;
    system->clock_order[system->clock_order_size++] = SCD_BUS;
    system->clock_order[system->clock_order_size++] = SCD_RAM;
    if (ticker_active) {
        system->clock_order[system->clock_order_size++] = SCD_BUS;
        system->clock_order[system->clock_order_size++] = SCD_TICKER;
    }
    system->clock_order[system->clock_order_size++] = SCD_BUS;

    system->hook = NULL;
    system->hook_count = 0;

    return system;
}


void system_clock(System_t *system) {
    if (!system) {
        log_msg(LP_ERROR, "System: NULL pointer");
        return;
    }
    for (int i = 0; i < system->clock_order_size; i++) {
        switch (system->clock_order[i]) {
            case SCD_CPU:
                cpu_clock(system->cpu);
                break;
            case SCD_RAM:
                ram_clock(system->ram);
                break;
            case SCD_BUS:
                bus_clock(system->bus);
                break;
            case SCD_TICKER:
                ticker_clock(system->ticker);
                break;
            default:
                break;
        }
    }
}


void system_hook(System_t* system, Hook_t hook) {
    if (!system) {
        log_msg(LP_ERROR, "System: NULL pointer");
        return;
    }
    if (system->hook == NULL) {
        if (system->hook_count != 0) {
            log_msg(LP_ERROR, "System: corruption detected");
        }
        // now assuming first hook
        system->hook = malloc(sizeof(Hook_t));
        system->hook[0] = hook;
    } else {
        system->hook = realloc(system->hook, sizeof(Hook_t) * (system->hook_count + 1));
        system->hook[system->hook_count] = hook;
    }
    system->hook_count++;
    switch (hook.condition) {
        case HC_CHANGE:
            system->memory_intermediate = realloc(system->memory_intermediate, system->memory_intermediate_size + hook.target_bytes);
            system->memory_intermediate_size += hook.target_bytes;
            break;

        case HC_MATCH:
        case HC_READ_FROM:
            break;

        default:
            log_msg(LP_ERROR, "System: Unknown Hook Condition %d", hook.condition);
            break;
    }
    return;
}

void system_hook_print(System_t* system) {
    printf("Hooks: %d\n", system->hook_count);
    for (int i = 0; i < system->hook_count; i++) {
        printf("index %d: ", i);
        printf("%p, %d, %d, %p", (void*) system->hook[i].target, system->hook[i].target_bytes, system->hook[i].condition, *(void **) &system->hook[i].action);
        puts("\n");
    }
}


void system_clock_debug(System_t *system) {
    if (!system) {
        log_msg(LP_ERROR, "System: NULL pointer");
        return;
    }
    system_clock(system);
    
    // save intermediate values again
    int index = 0;
    for (int h = 0; h < system->hook_count; h++) {
        switch (system->hook[h].condition) {
            case HC_CHANGE: {
                int change = 0;
                for (int j = 0; j < system->hook[h].target_bytes; j++) {
                    if (system->memory_intermediate[index + j] != *((uint8_t*) system->hook[h].target + j)) {
                        change = 1;
                        break;
                    }
                }
                if (change) {
                    char value_before_str[256] = "0x";
                    char value_after_str[256] = "0x";
                    for (int j = system->hook[h].target_bytes - 1; j >= 0; j--) {
                        sprintf(&value_before_str[(system->hook[h].target_bytes - j) * 2], "%.2X", system->memory_intermediate[index + j]);
                        sprintf(&value_after_str[(system->hook[h].target_bytes - j) * 2], "%.2X", *((uint8_t*) system->hook[h].target + j));
                    }
                    log_msg(LP_NOTICE, "System: Hook %d (HC_CHANGE) triggered. Value changed from %s to %s", h, value_before_str, value_after_str);
                    if (system->hook[h].action) {
                        system->hook[h].action(system);
                    }
                }
                for (int j = 0; j < system->hook[h].target_bytes; j++) {
                    system->memory_intermediate[index++] = *((uint8_t*) system->hook[h].target + j);
                }
                break;
            }

            case HC_MATCH: {
                int equal = 1;
                for (int j = 0; j < system->hook[h].target_bytes; j++) {
                    if (((uint8_t*)system->hook[h].match)[j] != *((uint8_t*) system->hook[h].target + j)) {
                        equal = 0;
                        break;
                    }
                }
                if (equal) {
                    char value_str[256] = "0x";
                    for (int j = system->hook[h].target_bytes - 1; j >= 0; j--) {
                        sprintf(&value_str[(system->hook[h].target_bytes - j) * 2], "%.2X", ((uint8_t*) system->hook[h].match)[j]);
                    }
                    log_msg(LP_NOTICE, "System: Hook %d (HC_EQUAL) triggered. Value matching %s", h, value_str);
                    if (system->hook[h].action) {
                        system->hook[h].action(system);
                    }
                }
                break;
            }

            case HC_READ_FROM: {
                if (
                    system->cpu->device.device_state == DS_FETCH && 
                    &system->ram->data[system->cpu->device.address] == system->hook->target
                ) {
                    log_msg(LP_NOTICE, "System: Hook %d (HC_READ_FROM) triggered. Reading from RAM at address 0x%.4X", h, system->cpu->device.address);
                    if (system->hook[h].action) {
                        system->hook[h].action(system);
                    }
                }
                break;
            }

            case HC_ALWAYS: {
                if (system->hook[h].action) {
                    system->hook[h].action(system);
                }
                break;
            }


            default:
                log_msg(LP_ERROR, "System: Unknown Hook Condition %d", system->hook[h].condition);
                break;
        }
    }
}

