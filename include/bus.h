#ifndef _BUS_H_
#define _BUS_H_

#include "device.h"
#include <stdint.h>



typedef struct BUS_t {
    uint64_t clock;
    
    int device_count;           // the number of devices connected
    int attended_device_index;  // the currently attented device id
    Device_t* device[16];       // the devices connected to the bus
} BUS_t;

BUS_t* bus_create(void);

void bus_delete(BUS_t* bus);

void bus_add_device(BUS_t* bus, Device_t* device);

void bus_clock(BUS_t* bus);


#endif

