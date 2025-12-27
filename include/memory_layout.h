#ifndef _MEMORY_LAYOUT_H_
#define _MEMORY_LAYOUT_H_

#include <stdint.h>

/*
Address space       Size    Device      Purpose/Info
0x0000 - 0x7fff     32768B  RAM         code and stack lives from 0x0000 onwards and 0x3fff downwards respectively
0x8000 - 0xeeff     28415B  RAM         static memory and heap resides in this spectrum
0xef00 - 0xefff     256B    RAM         IRQ vectors
0xf000 - 0xffff     4096B   MMIO        different devices have their direct memory communication on these addresses

To be added
Memory banks (probably 4KB sector with the first few bytes written to to select memory bank id, kinda like the NES memory banks in the cartridges)
ppu memory (aka. VRAM) for future ppu device. May as well be part of the MMIO section from 0xf000 to 0xffff? But thats kinda small...

*/

typedef enum {
    SEGMENT_CODE          = 0x0000,
    SEGMENT_STACK         = 0x7FFF,
    SEGMENT_IRQ_TABLE     = 0xEF00,
    SEGMENT_MMIO          = 0xF000,
} MemoryLayout_t;


#endif // _MEMORY_LAYOUT_H_
