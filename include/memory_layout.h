#ifndef _MEMORY_LAYOUT_H_
#define _MEMORY_LAYOUT_H_

typedef enum {
    SEGMENT_CODE          = 0x0000,
    SEGMENT_STACK         = 0x7FFF,
    SEGMENT_STATIC_MEMORY = 0x8000,
    SEGMENT_IRQ_TABLE     = 0xEF00,
    SEGMENT_MMIO          = 0xF000,
} MemoryLayout_t;


#endif // _MEMORY_LAYOUT_H_
