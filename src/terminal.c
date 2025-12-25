#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "memory_layout.h"

#include "device.h"
#include "terminal.h"

Terminal_t* terminal_create(void) {
    Terminal_t* ticker = malloc(sizeof(Terminal_t));
    ticker->device = device_create(DT_TERMINAL, 0, 1, SEGMENT_MMIO + 2, SEGMENT_MMIO + 2);
    ticker->device.device_state = DS_IDLE;

    return ticker;
}

void terminal_delete(Terminal_t** terminal) {
    if (!terminal) {return;}
    free(*terminal);
    *terminal = NULL;
}

void terminal_clock(Terminal_t* terminal) {
    // check device for commands
    if (terminal->device.processed == 1) {
        //log_msg(LP_INFO, "Terminal %d: Waiting for confirmation, nothing to do", terminal->clock);
        terminal->clock ++;
        return;
    }
    //log_msg(LP_INFO, "Terminal %d: state %d", terminal->clock, terminal->device.device_state);
    if (terminal->device.device_state == DS_FETCH) {
        terminal->device.processed = 1;
    }
    if (terminal->device.device_state == DS_STORE) {
        //log_msg(LP_INFO, "Terminal %d: recieved store request", terminal->clock);
        uint8_t data = terminal->device.data;
        printf("%c", data);
        terminal->device.processed = 1;
        //log_msg(LP_INFO, "Terminal %d: written %.2x", terminal->clock, terminal->device.data);
    }

    terminal->clock ++;
}
