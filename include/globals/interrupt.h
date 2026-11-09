#ifndef _INTERRUPT_H_
#define _INTERRUPT_H_

/*
A unified header for all sorts of interrupt types with their respective vector offset. 
They are REQUIRED to be a multiple of 2
*/

typedef enum {
    INT_RESET = 0, 
    INT_CLOCK = 2,          // this type is called by Ticker_t

} InterruptType_t;


#endif
