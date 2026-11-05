

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "globals/memory_layout.h"

#include "modules/device.h"
#include "modules/coprocessor.h"

const uint16_t MMIO_MODE_REGISTER_ADDRESS = SEGMENT_MMIO + 6;       // sets the general operation mode (r/w)

Coprocessor_t* coprocessor_create(void) {
    Coprocessor_t* coprocessor = malloc(sizeof(Coprocessor_t));
    coprocessor->device = device_create(DT_FILESYSTEM);

    device_add_listening_region(
        &coprocessor->device, 
        listening_region_create(MMIO_MODE_REGISTER_ADDRESS, MMIO_MODE_REGISTER_ADDRESS, LR_READ | LR_WRITE)
    );

    coprocessor->clock = 0ULL;
    coprocessor->device.device_state = DS_IDLE;

    return coprocessor;
}

void coprocessor_delete(Coprocessor_t** coprocessor) {
    if (!coprocessor) {return;}
    free(*coprocessor);
    *coprocessor = NULL;
}

void coprocessor_clock(Coprocessor_t* coprocessor) {
    if (coprocessor->device.processed == 1) {
        coprocessor->clock ++;
        return;
    }

    coprocessor->clock ++;
}

