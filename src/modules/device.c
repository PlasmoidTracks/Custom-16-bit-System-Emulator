#include "utils/Random.h"

#include "modules/device.h"

Device_t device_create(DEVICE_TYPE_t type) {
    return (Device_t) {
        .device_id = (DeviceID_t) rand64(),
        .device_type = type,
        .device_state = DS_IDLE,
        .processed = 0,
        .address = 0,
        .data = 0,
        .device_target_id = 0, 
        .listening_region = NULL, 
        .listening_region_count = 0, 
    };
}

ListeningRegion_t listening_region_create(uint16_t address_listener_low, uint16_t address_listener_high, int readable, int writable) {
    return (ListeningRegion_t) {
        .address_listener_high = address_listener_high, 
        .address_listener_low = address_listener_low, 
        .readable = readable, 
        .writable = writable, 
    };
}

void device_add_listening_region(Device_t* device, ListeningRegion_t listening_region) {
    device->listening_region = realloc(device->listening_region, sizeof(ListeningRegion_t) * (device->listening_region_count + 1));
    device->listening_region[device->listening_region_count++] = listening_region;
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

