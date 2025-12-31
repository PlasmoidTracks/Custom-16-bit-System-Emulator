#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "globals/memory_layout.h"

#include "modules/device.h"
#include "modules/ram.h"
#include "modules/memory_bank.h"

MemoryBank_t* memory_bank_create(void) {
    MemoryBank_t* memory_bank = malloc(sizeof(MemoryBank_t));
    memory_bank->device = device_create(DT_TERMINAL, 0, 1, SEGMENT_MMIO + 2, SEGMENT_MMIO + 2);
    memory_bank->device.device_state = DS_IDLE;

    memory_bank->ram = ram_create(0xffff);

    return memory_bank;
}

void memory_bank_delete(MemoryBank_t** memory_bank) {
    if (!memory_bank) {return;}
    free(*memory_bank);
    *memory_bank = NULL;
}

void memory_bank_clock(MemoryBank_t* memory_bank) {
    // check device for commands
    if (memory_bank->device.processed == 1) {
        //log_msg(LP_INFO, "RAM %d: Waiting for confirmation, nothing to do", ram->clock);
        memory_bank->clock ++;
        return;
    }
    //log_msg(LP_INFO, "RAM %d: state %d", ram->clock, ram->device.device_state);
    if (memory_bank->device.device_state == DS_FETCH) {
        //log_msg(LP_INFO, "RAM %d: recieved fetch request", ram->clock);
        uint16_t address = (uint16_t) memory_bank->device.address;
        uint64_t data = 0;
        for (size_t i = 0; i < sizeof(data); i++) {
            data |= ((uint64_t) ram_read(memory_bank->ram, address + i) << (8 * i));
        }
        memory_bank->device.data = data;
        memory_bank->device.processed = 1;
        //log_msg(LP_INFO, "RAM %d: fetch [%.8x] = %.8x", ram->clock, ram->device.address, ram->device.data);
    }
    if (memory_bank->device.device_state == DS_STORE) {
        //log_msg(LP_INFO, "RAM %d: recieved store request", ram->clock);
        uint16_t address = (uint16_t) memory_bank->device.address;
        uint8_t data = memory_bank->device.data;
        ram_write(memory_bank->ram, address, data);
        memory_bank->device.processed = 1;
        //log_msg(LP_INFO, "RAM %d: written %.8x at [%.8x]", ram->clock, ram->device.data, ram->device.address);
    }

    memory_bank->clock ++;
}
