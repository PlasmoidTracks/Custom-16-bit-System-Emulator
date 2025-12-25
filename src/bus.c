#include <stdlib.h>
#include <stdio.h>

#include "Log.h"

#include "device.h"
#include "bus.h"




BUS_t* bus_create(void) {
    BUS_t* bus = malloc(sizeof(BUS_t));
    for (int i = 0; i < 16; i++) {
        bus->device[i] = 0;
    }
    bus->device_count = 0;
    bus->attended_device_index = 0;
    bus->clock = 0ULL;
    return bus;
}

void bus_delete(BUS_t** bus) {
    if (!bus) {return;}
    free(*bus);
    *bus = NULL;
}

void bus_add_device(BUS_t* bus, Device_t* device) {
    int index = bus->device_count;
    bus->device[index] = device;
    bus->device_count++;
}

Device_t* bus_find_device_by_type(BUS_t* bus, DEVICE_TYPE_t device_type) {
    for (int i = 0; i < bus->device_count; i++) {
        if (bus->device[i]->device_type == device_type) {
            return bus->device[i];
        }
    }
    return NULL;
}

Device_t* bus_find_readable_device_by_mmio_address(BUS_t* bus, uint16_t address) {
    Device_t* device = NULL;
    for (int i = 0; i < bus->device_count; i++) {
        if (!bus->device[i]->readable) continue;
        if (address >= bus->device[i]->address_listener_low && address <= bus->device[i]->address_listener_high) {
            if (device) {
                log_msg(LP_ERROR, "BUS: Device memory read listeners are not unique (%d)", device->device_type);
            }
            device = bus->device[i];
        }
    }
    if (device) {
        //log_msg(LP_DEBUG, "BUS: Found readable mmio device %d", device->device_type);
    }
    return device;
}

Device_t* bus_find_writable_device_by_mmio_address(BUS_t* bus, uint16_t address) {
    Device_t* device = NULL;
    for (int i = 0; i < bus->device_count; i++) {
        if (!bus->device[i]->writable) continue;
        if (address >= bus->device[i]->address_listener_low && address <= bus->device[i]->address_listener_high) {
            if (device) {
                log_msg(LP_ERROR, "BUS: Device memory write listeners are not unique (%d)", device->device_type);
            }
            device = bus->device[i];
        }
    }
    if (device) {
        //log_msg(LP_DEBUG, "BUS: Found writable mmio device %d", device->device_type);
    }
    return device;
}

Device_t* bus_find_device_by_id(BUS_t* bus, DeviceID_t id) {
    for (int i = 0; i < bus->device_count; i++) {
        if (bus->device[i]->device_id == id) {
            return bus->device[i];
        }
    }
    return NULL;
}


void bus_clock(BUS_t* bus) {
    Device_t* device = bus->device[bus->attended_device_index];
    DEVICE_TYPE_t device_type = device->device_type;
    DEVICE_STATE_t device_state = device->device_state;
    switch (device_type) {
        case DT_CPU:
            //log_msg(LP_DEBUG, "BUS %d: Attending to a CPU", bus->clock);
            switch (device_state) {
                case DS_IDLE:
                    //log_msg(LP_DEBUG, "BUS %d: The CPU is idle", bus->clock);
                    break;
                
                case DS_FETCH: {
                    //log_msg(LP_DEBUG, "BUS %d: The CPU is fetching data", bus->clock);
                    if (device->processed == 1) {
                        //log_msg(LP_DEBUG, "BUS %d: The CPU has a fulfilled request pending", bus->clock);
                        break;
                    }
                    // Check the reading address, if its below 0xF000, then its addressing ram
                    Device_t* device_mmio = bus_find_readable_device_by_mmio_address(bus, device->address);
                    if (!device_mmio) {
                        device->processed = 1;
                        device->device_state = DS_IDLE;
                        //log_msg(LP_DEBUG, "BUS %d: No MMIO device attached to the BUS, that is responding on reads from address $%.4x [%s:%d]", bus->clock, device->address, __FILE__, __LINE__);
                        // what do now?
                        break;
                    }
                    //log_msg(LP_DEBUG, "BUS %d: Found RAM device to fetch data from. Making request", bus->clock);
                    // lets just straight up overwrite it and see what happens
                    if (device_mmio->device_state == DS_IDLE) {
                        device_mmio->address = device->address;
                        device_mmio->device_target_id = device->device_id;
                        device_mmio->device_state = DS_FETCH;
                        device_mmio->processed = 0;
                    } else {
                        //log_msg(LP_DEBUG, "BUS %d: RAM device is Idle, need to wait", bus->clock);
                    }
                    break;
                }
                
                case DS_STORE: {
                    //log_msg(LP_DEBUG, "BUS %d: The CPU is storing data", bus->clock);
                    if (device->processed == 1) {
                        //log_msg(LP_SUCCESS, "BUS %d: The CPU has a fulfilled request pending", bus->clock);
                        break;
                    }
                    // Check the reading address, if its below 0xF000, then its addressing ram
                    Device_t* device_mmio = bus_find_writable_device_by_mmio_address(bus, device->address);
                    if (!device_mmio) {
                        device->processed = 1;
                        device->device_state = DS_IDLE;
                        //log_msg(LP_DEBUG, "BUS %d: No MMIO device attached to the BUS, that is responding on writes to address $%.4x [%s:%d]", bus->clock, device->address, __FILE__, __LINE__);
                        // what do now?
                        break;
                    }
                    //log_msg(LP_DEBUG, "BUS %d: Found RAM device to store data to. Making request", bus->clock);
                    // lets just straight up overwrite it and see what happens
                    if (device_mmio->device_state == DS_IDLE) {
                        device_mmio->address = device->address;
                        device_mmio->data = device->data;
                        device_mmio->device_target_id = device->device_id;
                        device_mmio->device_state = DS_STORE;
                        device_mmio->processed = 0;
                    } else {
                        //log_msg(LP_DEBUG, "BUS %d: RAM device is Idle, need to wait", bus->clock);
                    }
                    break;
                }
                
                case DS_INTERRUPT:
                    // CPU will handle... I guess
                    break;
                
                default:
                    //log_msg(LP_DEBUG, "BUS %d: The CPU is in an unknown state %d", bus->clock, device_state);
                    break;
            }
            break;
        
        
        
        
        case DT_RAM:
            //log_msg(LP_DEBUG, "BUS %d: Attending to a RAM", bus->clock);
            
            // figure out what the CPU wants
            switch (device_state) {
                case DS_IDLE:
                    //log_msg(LP_DEBUG, "BUS %d: The RAM is idle", bus->clock);
                    break;
                
                case DS_FETCH: {
                    //log_msg(LP_DEBUG, "BUS %d: The RAM is retrieving data", bus->clock);
                    if (device->processed == 0) {
                        //log_msg(LP_DEBUG, "BUS %d: The RAM is not done with the request yet", bus->clock);
                        break;
                    }
                    Device_t* device_target = bus_find_device_by_id(bus, device->device_target_id);
                    if (!device_target) {
                        //log_msg(LP_ERROR, "BUS %d: Target device is not attached to the BUS", bus->clock [%s:%d]", __FILE__, __LINE__);
                        break;
                    }
                    //log_msg(LP_DEBUG, "BUS %d: Found Target device to send data to", bus->clock);
                    device_target->data = device->data;
                    device_target->processed = 1;
                    device->device_state = DS_IDLE;
                    break;
                }
                
                case DS_STORE: {
                    //log_msg(LP_DEBUG, "BUS %d: The RAM is storing data", bus->clock);
                    if (device->processed == 0) {
                        //log_msg(LP_DEBUG, "BUS %d: The RAM is not done with the request yet", bus->clock);
                        break;
                    }
                    Device_t* device_target = bus_find_device_by_id(bus, device->device_target_id);
                    if (!device_target) {
                        //log_msg(LP_ERROR, "BUS %d: Target device is not attached to the BUS", bus->clock [%s:%d]", __FILE__, __LINE__);
                        break;
                    }
                    //log_msg(LP_DEBUG, "BUS %d: Found Target device to validate", bus->clock);
                    device_target->processed = 1;
                    device->device_state = DS_IDLE;
                    break;
                }
                
                default:
                    //log_msg(LP_DEBUG, "BUS %d: The RAM is in an unknown state %d", bus->clock, device_state);
                    break;
            }
            break;



        
        case DT_CLOCK:
            //log_msg(LP_DEBUG, "BUS %d: Attending to a Clock", bus->clock);
            switch (device_state) {
                case DS_IDLE:
                    //log_msg(LP_DEBUG, "BUS %d: The CLOCK is idle", bus->clock);
                    break;
                
                case DS_INTERRUPT: {
                    //log_msg(LP_DEBUG, "BUS %d: The CLOCK has sent an interrupt signal", bus->clock);
                    Device_t* device_target = bus_find_device_by_type(bus, DT_CPU);
                    if (!device_target) {
                        //log_msg(LP_WARNING, "BUS %d: The CLOCK did not find a CPU to notify [%s:%d]", bus->clock, __FILE__, __LINE__);
                        device->device_state = DS_IDLE;
                        break;
                    }
                    if (device_target->device_state != DS_IDLE) {
                        //log_msg(LP_WARNING, "BUS %d: The target device (CPU) is not idle [%s:%d]", bus->clock, __FILE__, __LINE__);
                        break;
                    }
                    device_target->device_state = DS_INTERRUPT;
                    device_target->address = device->address;
                    device->device_state = DS_IDLE;
                    //log_msg(LP_DEBUG, "BUS %d: Target device (CPU) notified", bus->clock);
                    break;
                }
                
                default:
                    //log_msg(LP_DEBUG, "BUS %d: The CLOCK is in an unknown state %d", bus->clock, device_state);
                    break;
                }
            break;



        
        
        case DT_TERMINAL:
            //log_msg(LP_DEBUG, "BUS %d: Attending to a Terminal", bus->clock);
            // figure out what the CPU wants
            switch (device_state) {
                case DS_IDLE:
                    //log_msg(LP_DEBUG, "BUS %d: The Terminal is idle", bus->clock);
                    break;
                
                case DS_FETCH:
                    break;
                
                case DS_STORE: {
                    //log_msg(LP_DEBUG, "BUS %d: The Terminal is storing data", bus->clock);
                    if (device->processed == 0) {
                        //log_msg(LP_DEBUG, "BUS %d: The Terminal is not done with the request yet", bus->clock);
                        break;
                    }
                    Device_t* device_target = bus_find_device_by_id(bus, device->device_target_id);
                    if (!device_target) {
                        //log_msg(LP_ERROR, "BUS %d: Terminal target device is not attached to the BUS", bus->clock [%s:%d]", __FILE__, __LINE__);
                        break;
                    }
                    //log_msg(LP_DEBUG, "BUS %d: Found Terminal target device to validate", bus->clock);
                    device_target->processed = 1;
                    device->device_state = DS_IDLE;
                    break;
                }
                
                default:
                    //log_msg(LP_DEBUG, "BUS %d: The Terminal is in an unknown state %d", bus->clock, device_state);
                    break;
            }
            break;


        default:
            //log_msg(LP_WARNING, "BUS %d: Attending to an unknown device %d / %llu [%s:%d]", bus->clock, device_type, device->device_id, __FILE__, __LINE__);
            break;
    }


    // attending to possible request of the next device
    bus->attended_device_index = (bus->attended_device_index + 1) % bus->device_count;
    bus->clock ++;
    return;
}



