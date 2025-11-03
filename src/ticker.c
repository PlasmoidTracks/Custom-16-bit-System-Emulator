#include <stdlib.h>
#include <time.h>       // for clock_gettime
#include <stdint.h>

#include "utils/Log.h"
#include "interrupt.h"
#include "device.h"
#include "ticker.h"

int n = 0;

static inline double get_time_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    double t = ts.tv_sec + ((double) ts.tv_nsec / 1e9);
    return t;
}

Ticker_t* ticker_create(float frequency) {
    Ticker_t* ticker = malloc(sizeof(Ticker_t));
    ticker->time = 0.0;
    ticker->intervall = 1.0 / frequency;
    ticker->last_time = get_time_seconds();
    ticker->device = device_create(DT_CLOCK);
    ticker->device.device_state = DS_IDLE;

    return ticker;
}

void ticker_clock(Ticker_t* ticker) {
    double current_time = get_time_seconds();
    double delta_time = current_time - ticker->last_time;
    //log_msg(LP_DEBUG, "Ticker: lt %f, ct %f, dt %f, t %f, i %f", ticker->last_time, current_time, delta_time, ticker->time, ticker->intervall);
    ticker->last_time = current_time;
    ticker->time += delta_time;

    if (ticker->time >= ticker->intervall) {
        //log_msg(LP_DEBUG, "Ticker: INTERRUPTING!");
        ticker->device.device_state = DS_INTERRUPT;
        ticker->device.address = INT_CLOCK;
        ticker->device.data = 0;
        while (ticker->time >= ticker->intervall) {
            ticker->time -= ticker->intervall;
        }
    }
}
