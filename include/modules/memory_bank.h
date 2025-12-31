#ifndef _MEMORY_BANK_H_
#define _MEMORY_BANK_H_

#include <stdint.h>

#include "ram.h"
#include "device.h"

/*
The Memory Bank occupies a range of memory, but with swappable address spaces. 
The Banks can be swapped out such that the same address space shows new data. 
*/

typedef struct MemoryBank_t {
    RAM_t* ram;
    uint64_t clock;
    Device_t device;
} MemoryBank_t;


extern MemoryBank_t* memory_bank_create(void);

extern void memory_bank_delete(MemoryBank_t** memory_bank);

extern void memory_bank_clock(MemoryBank_t* memory_bank);

#endif // _MEMORY_BANK_H_
