#ifndef _RAM_H_
#define _RAM_H_

#include <stdint.h>

#include "device.h"

#define __RAM_DEBUG
#undef __RAM_DEBUG


#ifdef __RAM_DEBUG
    typedef struct {
        uint32_t* reads;
        uint32_t* writes;
    } __RAM_DEBUG_t;
#endif //__RAM_DEBUG

typedef struct RAM_t {
    Device_t device;

    uint64_t clock;
    uint8_t* data;

    #ifdef __RAM_DEBUG
        __RAM_DEBUG_t debug;
    #endif

    uint64_t reads;
    uint64_t writes;
    
    uint32_t capacity;
} RAM_t;


extern RAM_t* ram_create(uint32_t capacity);

extern void ram_delete(RAM_t** ram);

extern uint8_t ram_read(RAM_t* ram, uint16_t address);

extern void ram_write(RAM_t* ram, uint16_t address, uint8_t data);

extern void ram_clock(RAM_t* ram);


#endif

