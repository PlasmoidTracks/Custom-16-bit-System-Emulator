#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "globals/memory_layout.h"

#include "modules/device.h"
#include "modules/filesystem.h"

const uint16_t MMIO_MODE_REGISTER_ADDRESS = SEGMENT_MMIO + 6;       // sets the general operation mode (r/w)
const uint16_t MMIO_MODIFIER_REGISTER_ADDRESS = SEGMENT_MMIO + 7;   // sets modifiers for the operation mode (r/w)
const uint16_t MMIO_INPUT_REGISTER_ADDRESS = SEGMENT_MMIO + 8;      // accepts user input, use depends on operation mode (w)
const uint16_t MMIO_OUTPUT_REGISTER_ADDRESS = SEGMENT_MMIO + 9;     // returns misc. information, depends on operaion mode (r)

FileSystem_t* filesystem_create(void) {
    FileSystem_t* filesystem = malloc(sizeof(FileSystem_t));
    filesystem->device = device_create(DT_FILESYSTEM);
    device_add_listening_region(
        &filesystem->device, 
        listening_region_create(MMIO_MODE_REGISTER_ADDRESS, MMIO_MODE_REGISTER_ADDRESS, LR_READ | LR_WRITE)
    );
    device_add_listening_region(
        &filesystem->device, 
        listening_region_create(MMIO_MODIFIER_REGISTER_ADDRESS, MMIO_MODIFIER_REGISTER_ADDRESS, LR_READ | LR_WRITE)
    );
    device_add_listening_region(
        &filesystem->device, 
        listening_region_create(MMIO_INPUT_REGISTER_ADDRESS, MMIO_INPUT_REGISTER_ADDRESS, LR_WRITE)
    );
    device_add_listening_region(
        &filesystem->device, 
        listening_region_create(MMIO_OUTPUT_REGISTER_ADDRESS, MMIO_OUTPUT_REGISTER_ADDRESS, LR_READ)
    );

    for (int i = 0; i < 64; i++) {
        filesystem->path[i] = '\0';
    }
    filesystem->path_cursor_index = 0;
    filesystem->mode = M_BASE;
    filesystem->mode_modifier = M_BASE;
    filesystem->file_stream = NULL;

    filesystem->output = 0;

    filesystem->clock = 0ULL;
    filesystem->device.device_state = DS_IDLE;

    return filesystem;
}

void filesystem_delete(FileSystem_t** filesystem) {
    if (!filesystem) {return;}
    free(*filesystem);
    *filesystem = NULL;
}

void filesystem_clock(FileSystem_t* filesystem) {
    //log_msg(LP_DEBUG, "%lld, address: %.4x, state: %d, processed: %d", filesystem->clock, filesystem->device.address, filesystem->device.device_state, filesystem->device.processed);
    if (filesystem->device.processed == 1) {
        //log_msg(LP_INFO, "Filesystem %d: Waiting for confirmation, nothing to do", filesystem->clock);
        filesystem->clock ++;
        return;
    }

    if (filesystem->device.address == MMIO_MODE_REGISTER_ADDRESS) {
        // Here we handle writes to the mode register, 
        // which is responsible for setting up the action, like setting the filename to access, resetting filename, operning files, etc. 
        
        
        // Only allow to set value when it was set to 0 before, else ignore
        if (filesystem->device.device_state == DS_STORE) {
            if (filesystem->mode != 0) {
                //log_msg(LP_DEBUG, "Filesystem %lld: Couldnt update mode, as mode is not 0 (%d)", filesystem->clock, filesystem->mode);
            } else {
                filesystem->mode = filesystem->device.data;
                //log_msg(LP_DEBUG, "Filesystem %lld: Update mode to %d", filesystem->clock, filesystem->mode);
                
                // only do something on mode set
                switch (filesystem->mode) {
                    case M_PATH_RESET:
                        //log_msg(LP_DEBUG, "Filesystem %lld: Reset path", filesystem->clock);
                        for (int i = 0; i < 64; i++) {
                            filesystem->path[i] = '\0';
                        }
                        filesystem->path_cursor_index = 0;
                        filesystem->mode = 0;
                        break;
        
                    case M_PROBE_FILE: {
                        FILE* temp = fopen(filesystem->path, "r");
                        filesystem->output = temp != NULL;
                        if (temp) {
                            int err = fclose(temp);
                            if (err) {
                                //log_msg(LP_ERROR, "Filesystem: file \"%s\" could not be closed [%s:%d]", filesystem->path, __FILE__, __LINE__);
                                exit(1);
                            }
                        }
                        filesystem->mode = 0;
                        //log_msg(LP_DEBUG, "Filesystem %lld: probe path: %d", filesystem->clock, filesystem->output);
                        break;
                    }
        
                    case M_CREATE_FILE: {
                        // TODO, probe file and set output value accordingly
                        FILE* temp = fopen(filesystem->path, "w");
                        if (temp) {
                            int err = fclose(temp);
                            if (err) {
                                //log_msg(LP_ERROR, "Filesystem: file \"%s\" could not be closed [%s:%d]", filesystem->path, __FILE__, __LINE__);
                                exit(1);
                            }
                        }
                        filesystem->output = 1;
                        filesystem->mode = 0;
                        //log_msg(LP_DEBUG, "Filesystem %lld: create file: %d", filesystem->clock, filesystem->output);
                        break;
                    }
                    
                    default:
                        //log_msg(LP_DEBUG, "Filesystem %lld: unknown mode (%d)", filesystem->clock, filesystem->mode);
                        break;
                }
            }
        } else if (filesystem->device.device_state == DS_FETCH) {
            //log_msg(LP_DEBUG, "Filesystem %lld: fetched mode %d", filesystem->clock, filesystem->mode);
            filesystem->device.data = filesystem->mode;
        }

        filesystem->device.processed = 1;
    
    } else if (filesystem->device.address == MMIO_MODIFIER_REGISTER_ADDRESS) {
        filesystem->device.processed = 1;
    
    } else if (filesystem->device.address == MMIO_INPUT_REGISTER_ADDRESS) {
        switch (filesystem->mode) {
            case M_BASE:
                //log_msg(LP_DEBUG, "Filesystem %lld: Written to input without a mode set!", filesystem->clock);
                break;

            case M_SET_PATH:
                if (filesystem->path_cursor_index < 63) {
                    filesystem->path[filesystem->path_cursor_index++] = filesystem->device.data;
                    filesystem->path[filesystem->path_cursor_index] = '\0';
                }
                //log_msg(LP_DEBUG, "Filesystem %lld: Set path - \"%s\"", filesystem->clock, filesystem->path);
                break;
            
            case M_PATH_RESET:
                //log_msg(LP_DEBUG, "Filesystem %lld: Reset path", filesystem->clock);
                filesystem->path_cursor_index = 0;
                break;
            
            default:
                //log_msg(LP_DEBUG, "Filesystem %lld: Unknown mode (%d)", filesystem->clock, filesystem->mode);
                break;
        }
        filesystem->mode = 0;
        filesystem->device.processed = 1;
        //log_msg(LP_DEBUG, "Filesystem %lld: Update mode after successful operation to %d", filesystem->clock, filesystem->mode);
    
    } else if (filesystem->device.address == MMIO_OUTPUT_REGISTER_ADDRESS) {
        filesystem->device.data = filesystem->output;
        filesystem->device.processed = 1;
        //log_msg(LP_DEBUG, "Filesystem %lld: fetched output register %d", filesystem->clock, filesystem->mode);
    }

    filesystem->clock ++;
}
