#ifndef _FILESYSTEM_H_
#define _FILESYSTEM_H_

#include <stdint.h>
#include <stdio.h>

#include "modules/device.h"

/*
The Filesystem is an abstract module, that uses a state machine to make file accessess available in an asychronous way. 
*/

typedef enum {
    M_BASE = 0, 
    M_SET_PATH = 1,     // in this mode, any input is treated as an append to the destination path
    M_PATH_RESET = 2,   // this triggers a full reset of the path
    M_PROBE_FILE = 3,   // Checks whether the file already exists (return 0) or not (return 1) [YES, IT'S INVERTED! THIS IS BECAUSE IT'S BETTER TO WAIT FOR A CHANGE IN VALUE THAN TO EXPECT NO CHANGE, AS THE OUTPUT IS 0 BY DEFAULT, Y'KNOW?]
    M_MOUNT_FILE = 4,   // take the current set path and 
} Mode_t;

typedef enum {
    MM_EXPECT_EXISTING = 1 << 0, 
    MM_CREATE_ON_MISSING = 1 << 1, 

    MM_OTHER = 1 << 0, 
} ModeModifiert_t;

typedef struct FileSystem_t {
    char path[64];
    uint8_t path_cursor_index;
    uint8_t mode;               // mode of operation, changes behavior on read and write
    uint8_t mode_modifier;      // modifier for the mode of operation
    FILE* file_stream;

    uint64_t clock;
    Device_t device;
} FileSystem_t;

extern const uint16_t MMIO_MODE_REGISTER_ADDRESS;       // sets the general operation mode (r/w)
extern const uint16_t MMIO_MODIFIER_REGISTER_ADDRESS;   // sets modifiers for the operation mode (r/w)
extern const uint16_t MMIO_INPUT_REGISTER_ADDRESS;      // accepts user input, use depends on operation mode (w)
extern const uint16_t MMIO_OUTPUT_REGISTER_ADDRESS;     // returns misc. information, depends on operaion mode (r)

extern FileSystem_t* filesystem_create(void);

extern void filesystem_delete(FileSystem_t** memory_bank);

extern void filesystem_clock(FileSystem_t* memory_bank);

#endif // _FILESYSTEM_H_
