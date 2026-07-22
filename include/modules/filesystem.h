#ifndef _FILESYSTEM_H_
#define _FILESYSTEM_H_

#include <stdint.h>
#include <stdio.h>

#include "modules/device.h"

/*
The Filesystem is an abstract module, that uses a state machine to make file accessess available in an asychronous way. 
*/

typedef enum {
    M_BASE, 
    M_SET_PATH,     // in this mode, any input is treated as an append to the destination path
    M_PATH_RESET,   // this triggers a full reset of the path
    M_PROBE_FILE,   // Checks whether the file already exists (return 0) or not (return 1) [YES, IT'S INVERTED! THIS IS BECAUSE IT'S BETTER TO WAIT FOR A CHANGE IN VALUE THAN TO EXPECT NO CHANGE, AS THE OUTPUT IS 0 BY DEFAULT, Y'KNOW?]
    M_CREATE_FILE,  // if the path doesnt exist, create a file with the path name, otherwise leave file alone
    M_MOUNT_FILE,   // take the current set path and (return 1 on success, else 0)
    M_CLOSE_FILE,   
} FileSystemMode_t;

// set cursor position as value (where negative is like pythons [-1])
    // is -1 the very end, or one before the very end?
// set cursor to beginning (same as set 0)
// set cursor to end (same as set -1?)
// insert
// overwrite
// delete
// read (+increment pointer)

/*
// maybe split into more basic and more specific: 
PATH (reset, set)
FILE (probe, create, mount, close)

why? Because a single write to two 8-bit registers can set the modes directly!

Maybe we should have a status register
*/

typedef enum {
    MM_EXPECT_EXISTING = 1 << 0, 
    MM_CREATE_ON_MISSING = 1 << 1, 

    MM_OTHER = 1 << 0, 
} FileSystemModeModifiert_t;

typedef struct FileSystem_t {
    char path[64];
    uint8_t path_cursor_index;
    uint8_t mode;               // mode of operation, changes behavior on read and write
    uint8_t mode_modifier;      // modifier for the mode of operation
    FILE* file_stream;

    uint8_t output;

    uint64_t clock;
    Device_t device;
} FileSystem_t;

extern const uint16_t MMIO_MODE_REGISTER_ADDRESS;       // sets the general operation mode (r/w)
extern const uint16_t MMIO_MODIFIER_REGISTER_ADDRESS;   // sets modifiers for the operation mode (r/w)
extern const uint16_t MMIO_INPUT_REGISTER_ADDRESS;      // accepts user input, use depends on operation mode (w)
extern const uint16_t MMIO_OUTPUT_REGISTER_ADDRESS;     // returns misc. information, depends on operaion mode (r)

extern FileSystem_t* filesystem_create(void);

extern void filesystem_delete(FileSystem_t** filesystem);

extern void filesystem_clock(FileSystem_t* filesystem);

#endif // _FILESYSTEM_H_
