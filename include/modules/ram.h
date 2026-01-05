#ifndef _RAM_H_
#define _RAM_H_

#include <stdint.h>

#include "device.h"

typedef struct RAM_t {
    uint64_t clock;
    
    uint32_t capacity;
    uint8_t *data;

    Device_t device;

    uint64_t reads;
    uint64_t writes;
} RAM_t;


extern RAM_t* ram_create(uint32_t capacity);

extern void ram_delete(RAM_t** ram);

extern uint8_t ram_read(RAM_t* ram, uint16_t address);

extern void ram_write(RAM_t* ram, uint16_t address, uint8_t data);

extern void ram_clock(RAM_t* ram);


#endif

