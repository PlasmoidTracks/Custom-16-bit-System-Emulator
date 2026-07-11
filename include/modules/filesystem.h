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
} Mode_t;

typedef struct FileSystem_t {
    char path[64];
    uint8_t path_cursor_index;
    uint8_t mode;               // mode of operation, changes behavior on read and write
    FILE* file_stream;


    uint64_t clock;
    Device_t device;
} FileSystem_t;

extern const uint16_t MMIO_MODE_REGISTER_ADDRESS;
extern const uint16_t MMIO_REGISTER_ADDRESS2;
extern const uint16_t MMIO_REGISTER_ADDRESS3;

extern FileSystem_t* filesystem_create(void);

extern void filesystem_delete(FileSystem_t** memory_bank);

extern void filesystem_clock(FileSystem_t* memory_bank);

#endif // _FILESYSTEM_H_
