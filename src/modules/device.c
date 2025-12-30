#include "utils/Random.h"

#include "modules/device.h"

Device_t device_create(DEVICE_TYPE_t type, int readable, int writable, uint16_t address_listener_low, uint16_t address_listener_high) {
    return (Device_t) {
        .device_id = (DeviceID_t) rand64(),
        .device_type = type,
        .device_state = DS_IDLE,
        .processed = 0,
        .address = 0,
        .data = 0,
        .device_target_id = 0, 
        .readable = readable, 
        .writable = writable, 
        .address_listener_low = address_listener_low, 
        .address_listener_high = address_listener_high, 
    };
}

// sets the device back to idle and set processed to 0
void device_reset(Device_t* device) {
    device->device_state = DS_IDLE;
    device->processed = 0;
    device->address = 0xffffffffffffffffULL;
    device->data = 0xffffffffffffffffULL;
}

// returns 1 if the current request has been processed, else 0
int device_check_response(Device_t* device) {
    return device->processed;
}

// returns the data of the device
uint64_t device_get_data(Device_t* device) {
    return device->data;
}

