#ifndef _COPROCESSOR_H_
#define _COPROCESSOR_H_

#include <stdint.h>

#include "modules/device.h"

typedef struct Coprocessor_t {
    uint64_t clock;
    Device_t device;
} Coprocessor_t;

extern const uint16_t MMIO_COPROCESSOR_MODE_REGISTER;
extern const uint16_t MMIO_COPROCESSOR_STATUS_REGISTER;
extern const uint16_t MMIO_COPROCESSOR_INPUT_REGISTER;
extern const uint16_t MMIO_COPROCESSOR_OUTPUT_REGISTER;

extern Coprocessor_t* coprocessor_create(void);

extern void coprocessor_delete(Coprocessor_t** memory_bank);

extern void coprocessor_clock(Coprocessor_t* memory_bank);

#endif
