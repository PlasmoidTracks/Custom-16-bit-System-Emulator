#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "utils/Random.h"


uint64_t CRAND_SEED = 0;
void (*CRAND_FUNCTION)(void) = RAND_UPDATE_UNIFORM;

void RAND_UPDATE_UNIFORM(void) {
    CRAND_SEED ^= 0x5c6d7e8f90a1b2c3;
    CRAND_SEED *= 0x93a4b5c6d7e8f90b;
    CRAND_SEED = (CRAND_SEED & ~0x0000100000001000) | ((CRAND_SEED & 0x0000100000000000) >> 32) | ((CRAND_SEED & 0x0000000000001000) << 32);
    CRAND_SEED = (CRAND_SEED & ~0x0000000100000001) | ((CRAND_SEED & 0x0000000100000000) >> 32) | ((CRAND_SEED & 0x0000000000000001) << 32);
}

static void rand_update(void) {
    CRAND_FUNCTION();
}

void rand_set_seed(uint64_t seed) {
    CRAND_SEED = seed;
}

uint64_t rand_get_seed(void) {
    return CRAND_SEED;
}

void randomize(void) {
    CRAND_SEED = (uint64_t) time(NULL);
}

uint64_t rand64(void) {
    rand_update();
    return CRAND_SEED;
}

