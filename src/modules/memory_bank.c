#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "globals/memory_layout.h"

#include "modules/device.h"
#include "modules/ram.h"
#include "modules/memory_bank.h"

const uint16_t MMIO_REGISTER_ADDRESS = SEGMENT_MMIO + 4;
const uint16_t MMIO_BANK_WIDTH = 0x2000;

MemoryBank_t* memory_bank_create(void) {
    MemoryBank_t* memory_bank = malloc(sizeof(MemoryBank_t));
    memory_bank->device = device_create(DT_MEMORY_BANK);
    device_add_listening_region(
        &memory_bank->device, 
        listening_region_create(MMIO_REGISTER_ADDRESS, MMIO_REGISTER_ADDRESS, 0, 1)
    );
    device_add_listening_region(
        &memory_bank->device, 
        listening_region_create(0x8000, 0x8000 + MMIO_BANK_WIDTH - 1, 1, 1)
    );
    memory_bank->device.device_state = DS_IDLE;
    memory_bank->bank_index = 0;
    memory_bank->clock = 0ULL;

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
        uint16_t address = (uint16_t) memory_bank->device.address;
        uint16_t virtual_address = (MMIO_BANK_WIDTH * memory_bank->bank_index) + (address % MMIO_BANK_WIDTH);
        /*log_msg(LP_INFO, "MEMORY BANK %d: recieved fetch request at $%.4x, mapped to $%.4x", 
            memory_bank->clock, 
            address, 
            virtual_address
        );*/
        uint64_t data = 0;
        for (size_t i = 0; i < sizeof(data); i++) {
            data |= ((uint64_t) ram_read(memory_bank->ram, virtual_address + i) << (8 * i));
        }
        memory_bank->device.data = data;
        memory_bank->device.processed = 1;
        //log_msg(LP_INFO, "RAM %d: fetch [%.8x] = %.8x", ram->clock, ram->device.address, ram->device.data);
    }
    if (memory_bank->device.device_state == DS_STORE) {
        //log_msg(LP_INFO, "RAM %d: recieved store request", ram->clock);
        uint16_t address = (uint16_t) memory_bank->device.address;
        uint16_t virtual_address = (MMIO_BANK_WIDTH * memory_bank->bank_index) + (address % MMIO_BANK_WIDTH);
        uint8_t data = memory_bank->device.data;
        if (address == MMIO_REGISTER_ADDRESS) {
            memory_bank->bank_index = data & 0x07;  // mod 8
            memory_bank->clock ++;
            memory_bank->device.processed = 1;
            //log_msg(LP_INFO, "MEMORY BANK %d: setbank_index to %d (raw %.4x)", memory_bank->clock, memory_bank->bank_index, data);
            return;
        }
        /*log_msg(LP_INFO, "MEMORY BANK %d: recieved store request at $%.4x, mapped to $%.4x", 
            memory_bank->clock, 
            address, 
            virtual_address
        );*/
        ram_write(memory_bank->ram, virtual_address, data);
        memory_bank->device.processed = 1;
        //log_msg(LP_INFO, "RAM %d: written %.8x at [%.8x]", ram->clock, ram->device.data, ram->device.address);
    }

    memory_bank->clock ++;
}
