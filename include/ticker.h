#ifndef _TICKER_H_
#define _TICKER_H_

#include <stdint.h>
#include "device.h"

/*
The general idea is that the CPU will get an interrupt every like 10ms. 
What the CPU does with that interrupt is up to the CPU, but time keeping will 
be done on the CPU side, not this clock device
*/

typedef struct Ticker_t {
    double time;
    double last_time;
    double intervall;
    Device_t device;
} Ticker_t;


extern Ticker_t* ticker_create(float frequency);

extern void ticker_clock(Ticker_t* ticker);


#endif

