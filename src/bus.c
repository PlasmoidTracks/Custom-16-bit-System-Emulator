#include <stdlib.h>
#include <stdio.h>

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

void bus_delete(BUS_t* bus) {
    free(bus);
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
                
                case DS_FETCH:
                    //log_msg(LP_DEBUG, "BUS %d: The CPU is fetching data", bus->clock);
                    if (device->processed == 1) {
                        //log_msg(LP_DEBUG, "BUS %d: The CPU has a fulfilled request pending", bus->clock);
                        break;
                    }
                    // Check the reading address, if its below 0xF000, then its addressing ram
                    if (device->address < 0xf000) {
                        // Ok, so lets find the RAM device, to route the request to it
                        Device_t* device_ram = bus_find_device_by_type(bus, DT_RAM);
                        if (!device_ram) {
                            //log_msg(LP_ERROR, "BUS %d: No RAM device attached to the BUS", bus->clock);
                            // what do now?
                            break;
                        }
                        //log_msg(LP_DEBUG, "BUS %d: Found RAM device to fetch data from. Making request", bus->clock);
                        // lets just straight up overwrite it and see what happens
                        if (device_ram->device_state == DS_IDLE) {
                            device_ram->address = device->address;
                            device_ram->device_target_id = device->device_id;
                            device_ram->device_state = DS_FETCH;
                            device_ram->processed = 0;
                        } else {
                            //log_msg(LP_DEBUG, "BUS %d: RAM device is Idle, need to wait", bus->clock);
                        }
                    } else {
                        //log_msg(LP_DEBUG, "BUS %d: Read request forwarding to Dummy MMIO at address $%.4X", bus->clock, device->address);
                        // Since address is between 0xF000 and 0xFFFF, we are now addressing MMIO
                        // Here we differentiate the devices we want to address
                        // 0xF000 is unused
                        // 0xF002 is terminal output (write only)
                        // 0xF004 is terminal input
                        // 0xF006 is a request to write to disk with the writing option flags set in 0xF008 and the filename being pointed to by r0?
                        // 0xF008 is a request to read from disk with the same parameters

                        // So far use dummy, as there are no writable devices as of now
                        device->processed = 1;
                        device->device_state = DS_IDLE;
                    }
                    break;
                
                case DS_STORE:
                    //log_msg(LP_DEBUG, "BUS %d: The CPU is storing data", bus->clock);
                    if (device->processed == 1) {
                        //log_msg(LP_SUCCESS, "BUS %d: The CPU has a fulfilled request pending", bus->clock);
                        break;
                    }
                    // Check the reading address, if its below 0xF000, then its addressing ram
                    if (device->address < 0xf000) {
                        // Ok, so lets find the RAM device, to route the request to it
                        Device_t* device_ram = bus_find_device_by_type(bus, DT_RAM);
                        if (!device_ram) {
                            //log_msg(LP_ERROR, "BUS %d: No RAM device attached to the BUS", bus->clock);
                            // what do now?
                            break;
                        }
                        //log_msg(LP_DEBUG, "BUS %d: Found RAM device to store data to. Making request", bus->clock);
                        // lets just straight up overwrite it and see what happens
                        if (device_ram->device_state == DS_IDLE) {
                            device_ram->address = device->address;
                            device_ram->data = device->data;
                            device_ram->device_target_id = device->device_id;
                            device_ram->device_state = DS_STORE;
                            device_ram->processed = 0;
                        } else {
                            //log_msg(LP_DEBUG, "BUS %d: RAM device is Idle, need to wait", bus->clock);
                        }
                    } else {
                        //log_msg(LP_DEBUG, "BUS %d: Write request forwarding to MMIO at address $%.4X with data ", bus->clock, device->address);
                        // Since address is between 0xF000 and 0xFFFF, we are now addressing MMIO
                        // Here we differentiate the devices we want to address
                        // 0xF000 is unused
                        // 0xF002 is terminal output
                        // 0xF004 is terminal input (read only)
                        if (device->address == 0xf002) {
                            //log_msg(LP_DEBUG, "BUS %d: Addressing Terminal as MMIO target", bus->clock);
                            Device_t* device_terminal = bus_find_device_by_type(bus, DT_TERMINAL);
                            if (!device_terminal) {
                                //log_msg(LP_ERROR, "BUS %d: No Terminal device attached to the BUS", bus->clock);
                                // what do now?
                                break;
                            }
                            if (device_terminal->device_state == DS_IDLE) {
                                device_terminal->data = device->data;
                                device_terminal->device_target_id = device->device_id;
                                device_terminal->device_state = DS_STORE;
                                device_terminal->processed = 0;
                            }

                        } else {
                            //log_msg(LP_ERROR, "BUS %d: Addressing Dummy as MMIO target", bus->clock);
                            // For any other device, use dummy
                            device->processed = 1;
                            device->device_state = DS_IDLE;
                        }
                    }
                    break;
                
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
                
                case DS_FETCH:
                    //log_msg(LP_DEBUG, "BUS %d: The RAM is retrieving data", bus->clock);
                    if (device->processed == 0) {
                        //log_msg(LP_DEBUG, "BUS %d: The RAM is not done with the request yet", bus->clock);
                        break;
                    }
                    {
                        Device_t* device_target = bus_find_device_by_id(bus, device->device_target_id);
                        if (!device_target) {
                            //log_msg(LP_ERROR, "BUS %d: Target device is not attached to the BUS", bus->clock);
                            break;
                        }
                        //log_msg(LP_DEBUG, "BUS %d: Found Target device to send data to", bus->clock);
                        device_target->data = device->data;
                        device_target->processed = 1;
                        device->device_state = DS_IDLE;
                    }
                    break;
                
                case DS_STORE:
                    //log_msg(LP_DEBUG, "BUS %d: The RAM is storing data", bus->clock);
                    if (device->processed == 0) {
                        //log_msg(LP_DEBUG, "BUS %d: The RAM is not done with the request yet", bus->clock);
                        break;
                    }
                    {
                        Device_t* device_target = bus_find_device_by_id(bus, device->device_target_id);
                        if (!device_target) {
                            //log_msg(LP_ERROR, "BUS %d: Target device is not attached to the BUS", bus->clock);
                            break;
                        }
                        //log_msg(LP_DEBUG, "BUS %d: Found Target device to validate", bus->clock);
                        device_target->processed = 1;
                        device->device_state = DS_IDLE;
                    }
                    break;
                
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
                
                case DS_INTERRUPT:
                    //log_msg(LP_DEBUG, "BUS %d: The CLOCK has sent an interrupt signal", bus->clock);
                    {
                        Device_t* device_target = bus_find_device_by_type(bus, DT_CPU);
                        if (!device_target) {
                            //log_msg(LP_WARNING, "BUS %d: The CLOCK did not find a CPU to notify", bus->clock);
                            device->device_state = DS_IDLE;
                            break;
                        }
                        if (device_target->device_state != DS_IDLE) {
                            //log_msg(LP_WARNING, "BUS %d: The target device (CPU) is not idle", bus->clock);
                            break;
                        }
                        device_target->device_state = DS_INTERRUPT;
                        device_target->address = device->address;
                        device->device_state = DS_IDLE;
                        //log_msg(LP_DEBUG, "BUS %d: Target device (CPU) notified", bus->clock);
                    }
                    break;
                
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
                
                case DS_STORE:
                    //log_msg(LP_DEBUG, "BUS %d: The Terminal is storing data", bus->clock);
                    if (device->processed == 0) {
                        //log_msg(LP_DEBUG, "BUS %d: The Terminal is not done with the request yet", bus->clock);
                        break;
                    }
                    {
                        Device_t* device_target = bus_find_device_by_id(bus, device->device_target_id);
                        if (!device_target) {
                            //log_msg(LP_ERROR, "BUS %d: Terminal target device is not attached to the BUS", bus->clock);
                            break;
                        }
                        //log_msg(LP_DEBUG, "BUS %d: Found Terminal target device to validate", bus->clock);
                        device_target->processed = 1;
                        device->device_state = DS_IDLE;
                    }
                    break;
                
                default:
                    //log_msg(LP_DEBUG, "BUS %d: The Terminal is in an unknown state %d", bus->clock, device_state);
                    break;
            }
            break;


        default:
            //log_msg(LP_WARNING, "BUS %d: Attending to an unknown device %d / %llu", bus->clock, device_type, device->device_id);
            break;
    }


    // attending to possible request of the next device
    bus->attended_device_index = (bus->attended_device_index + 1) % bus->device_count;
    bus->clock ++;
    return;
}



