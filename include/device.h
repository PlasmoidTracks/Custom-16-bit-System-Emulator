#ifndef _DEVICE_H_
#define _DEVICE_H_

#include <stdint.h>

typedef uint32_t DeviceID_t;

typedef enum {
    DT_CPU,             // Central Processing Unit
    DT_RAM,             // Random Access Memory
    DT_CLOCK, 
    DT_STORAGE,         // Storage, like hard drives
    DT_DISPLAY,         // Visual display
    DT_KEYBOARD         // User input device
} DEVICE_TYPE_t;

typedef enum {
    DS_IDLE,            // Device is doing nothing
    DS_OCCUPY,          // Device does not want to be disturbed, like when executing other tasks
    DS_FETCH,           // Device is AWAITING data [CPU]
    DS_STORE,           // Device is currently WRITING to the BUS to be saved
    DS_REPLY,           // Device wants to reply
    DS_INTERRUPT,       // Device is interrupting
} DEVICE_STATE_t;

/*
cpu is requesting data      [FETCH]
ram recieves read request   [FETCH]
ram recieves write request  [STORE] 
ram is providing data       [REPLY]
*/

typedef struct Device_t {
    DeviceID_t device_id;                   // self identifier
    DeviceID_t device_target_id;            // identifier of the target
    DEVICE_TYPE_t device_type;              // What device type the holder is, i.e. CPU
    DEVICE_STATE_t device_state;            // The current state of the device (idle, busy, read, write, fetch, dispatch)
    int processed;                          // Flag to indicated that the current request has been processed
    uint64_t address;                       // the request body, like address
    uint64_t data;                          // the response to the request
} Device_t;

Device_t device_create(DEVICE_TYPE_t type);

// sets the device back to idle and set processed to 0
void device_reset(Device_t* device);

// returns 1 if the current request has been processed, else 0
int device_check_response(Device_t* device);

// returns the data of the device
uint64_t device_get_data(Device_t* device);

int device_make_request(Device_t* device, uint64_t address, uint64_t data);


#endif

