#ifndef _BUS_H_
#define _BUS_H_

#include <stdint.h>

#include "device.h"


typedef struct BUS_t {
    uint64_t clock;
    
    int device_count;           // the number of devices connected
    int attended_device_index;  // the currently attented device id
    Device_t* device[16];       // the devices connected to the bus
} BUS_t;

extern BUS_t* bus_create(void);

extern void bus_delete(BUS_t** bus);

extern void bus_add_device(BUS_t* bus, Device_t* device);

extern void bus_clock(BUS_t* bus);


#endif

