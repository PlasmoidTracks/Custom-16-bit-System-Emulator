#ifndef _TERMINAL_H_
#define _TERMINAL_H_

#include <stdint.h>
#include "device.h"

/*
The terminal will be the intersection between the CPU and the real terminal, for IO interactions
*/

typedef struct Terminal_t {
    uint64_t clock;
    Device_t device;
} Terminal_t;


extern Terminal_t* terminal_create(void);

extern void terminal_clock(Terminal_t* terminal);


#endif

