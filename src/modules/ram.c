#include <stdlib.h>

#include "modules/device.h"
#include "modules/ram.h"


RAM_t* ram_create(uint32_t capacity) {
    RAM_t* ram = malloc(sizeof(RAM_t));
    ram->device = device_create(DT_RAM);
    device_add_listening_region(
        &ram->device, 
        listening_region_create(0x0000, 0x7fff, 1, 1)
    );
    device_add_listening_region(
        &ram->device, 
        listening_region_create(0xa000, 0xefff, 1, 1)
    );
    ram->capacity = capacity;
    //log_msg(LP_INFO, "setting ram cap to %.4x and is now %.4x\n", capacity, ram->capacity);
    ram->data = malloc(sizeof(uint8_t) * capacity);
    for (uint32_t i = 0; i < capacity; i++) {
        ram->data[i] = 0x00; //rand8();
    }
    ram->clock = 0ULL;

    ram->reads = 0;
    ram->writes = 0;

    return ram;
}

void ram_delete(RAM_t** ram) {
    if (!ram) {return;}
    free((*ram)->data);
    free(*ram);
    *ram = NULL;
}

uint8_t ram_read(RAM_t* ram, uint16_t address) {
    if (ram->capacity == 0) {
        //log_msg(LP_CRITICAL, "RAM %d: capacity is zero!", ram->clock);
        return 0x00;
    }
    ram->reads += 1ULL;
    return ram->data[address % ram->capacity];
}

void ram_write(RAM_t* ram, uint16_t address, uint8_t data) {
    if (ram->capacity == 0) {
        //log_msg(LP_CRITICAL, "RAM %d: capacity is zero!", ram->clock);
        return;
    }
    ram->writes += 1ULL;
    ram->data[address % ram->capacity] = data;
}

void ram_clock(RAM_t* ram) {
    // check device for commands
    if (ram->device.processed == 1) {
        //log_msg(LP_INFO, "RAM %d: Waiting for confirmation, nothing to do", ram->clock);
        ram->clock ++;
        return;
    }
    //log_msg(LP_INFO, "RAM %d: state %d", ram->clock, ram->device.device_state);
    if (ram->device.device_state == DS_FETCH) {
        //log_msg(LP_INFO, "RAM %d: recieved fetch request", ram->clock);
        uint16_t address = (uint16_t) ram->device.address;
        uint64_t data = 0;
        for (size_t i = 0; i < sizeof(data); i++) {
            data |= ((uint64_t) ram_read(ram, address + i) << (8 * i));
        }
        ram->device.data = data;
        ram->device.processed = 1;
        //log_msg(LP_INFO, "RAM %d: fetch [%.8x] = %.8x", ram->clock, ram->device.address, ram->device.data);
    }
    if (ram->device.device_state == DS_STORE) {
        //log_msg(LP_INFO, "RAM %d: recieved store request", ram->clock);
        uint16_t address = (uint16_t) ram->device.address;
        uint8_t data = ram->device.data;
        ram_write(ram, address, data);
        ram->device.processed = 1;
        //log_msg(LP_INFO, "RAM %d: written %.8x at [%.8x]", ram->clock, ram->device.data, ram->device.address);
    }

    ram->clock ++;

    return;
}

