#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "utils/Random.h"


uint64_t CRAND_SEED = 0;
void (*CRAND_FUNCTION)(void) = RAND_UPDATE_UNIFORM;

void RAND_UPDATE_DETERMINISTIC_CYCLIC() {
    CRAND_SEED ^= 0x5c6d7e8f90a1b2c3;
    CRAND_SEED *= 0x93a4b5c6d7e8f90b;
    CRAND_SEED += 0x15263748596a7b8c;
    CRAND_SEED ^= 0xc3b2a1908f7e6d5c;
}

void RAND_UPDATE_UNIFORM() {
    CRAND_SEED ^= 0x5c6d7e8f90a1b2c3;
    CRAND_SEED *= 0x93a4b5c6d7e8f90b;
    //CRAND_SEED = ((CRAND_SEED & 0xf0f0f0f0f0f0f0f0) >> 4) | ((CRAND_SEED & 0x0f0f0f0f0f0f0f0f) << 4);
    //CRAND_SEED = ((CRAND_SEED & 0xaaaaaaaaaaaaaaaa) >> 1) | ((CRAND_SEED & 0x5555555555555555) << 1);
    //CRAND_SEED += 0x15263748596a7b8c;
    //CRAND_SEED = ((CRAND_SEED & 0xaaaaaaaaaaaaaaaa) >> 1) | ((CRAND_SEED & 0x5555555555555555) << 1);
    //CRAND_SEED = ((CRAND_SEED & 0xe38e38e38e38e38e) >> 3) | ((CRAND_SEED & 0x1c71c71c71c71c71) << 3);
    //CRAND_SEED = ((CRAND_SEED & 0xcccccccccccccccc) >> 2) | ((CRAND_SEED & 0x3333333333333333) << 2);
    //CRAND_SEED ^= 0xc3b2a1908f7e6d5c;
    //CRAND_SEED = ((CRAND_SEED & 0xffffffff00000000) >> 32) | ((CRAND_SEED & 0x00000000ffffffff) << 32);
    CRAND_SEED = (CRAND_SEED & ~0x0000100000001000) | ((CRAND_SEED & 0x0000100000000000) >> 32) | ((CRAND_SEED & 0x0000000000001000) << 32);
    CRAND_SEED = (CRAND_SEED & ~0x0000000100000001) | ((CRAND_SEED & 0x0000000100000000) >> 32) | ((CRAND_SEED & 0x0000000000000001) << 32);
}

void rand_set_sampler(void(*function)(void)) {
    CRAND_FUNCTION = function;
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

void randomize() {
    CRAND_SEED = (uint64_t) time(NULL);
}


int rand1() {
    rand_update();
    return CRAND_SEED & 0x01;
}

uint8_t rand8() {
    rand_update();
    return CRAND_SEED & 0xff;
}

uint16_t rand16() {
    rand_update();
    return CRAND_SEED & 0xffff;
}

uint32_t rand32() {
    rand_update();
    return CRAND_SEED & 0xffffffff;
}

uint64_t rand64() {
    rand_update();
    return CRAND_SEED;
}


int8_t rands8() {
    rand_update();
    return CRAND_SEED & 0xff;
}

int16_t rands16() {
    rand_update();
    return CRAND_SEED & 0xffff;
}

int32_t rands32() {
    rand_update();
    return CRAND_SEED & 0xffffffff;
}

int64_t rands64() {
    rand_update();
    return CRAND_SEED;
}


float frandf() {
    return (float)((double) rand64() / (double) 0xffffffffffffffff);
}
double frand() {
    return (double)((double) rand64() / (double) 0xffffffffffffffff);
}
long double frandl() {
    return (long double)((double) rand64() / (long double) 0xffffffffffffffff);
}


uint8_t uniform8(uint8_t min, uint8_t max) {
    return rand8() % (max - min) + min;
}
uint16_t uniform16(uint16_t min, uint16_t max) {
    return rand16() % (max - min) + min;
}
uint32_t uniform32(uint32_t min, uint32_t max) {
    return rand32() % (max - min) + min;
}
uint64_t uniform64(uint64_t min, uint64_t max) {
    return rand64() % (max - min) + min;
}


float funiformf(float min, float max) {
    return (max - min) * frandf() + min;
}
double funiform(double min, double max) {
    return (max - min) * frand() + min;
}
long double funiforml(long double min, long double max) {
    return (max - min) * frandl() + min;
}


void* choose_ext(void* list, size_t elem_size, uint64_t array_length) {
    if (!list) {return NULL;}
    uint64_t index = rand64() % array_length;
    void* ptr = (uint8_t*) list + index * elem_size;
    return ptr;
}


void shuffle_ext(void* list, size_t elem_size, uint64_t array_length) {
    if (!list) {printf("\tERROR: Null pointer exception\n"); return;}
    void* tmp = malloc(elem_size);
    if (!tmp || !list) {return;}
    for (uint64_t index_1 = 0; index_1 < elem_size; index_1++) {
        uint64_t index_2 = rand64() % array_length;
        memcpy(tmp, (uint8_t*) list + elem_size*index_1, elem_size);
        memcpy((uint8_t*) list + elem_size*index_1, (uint8_t*) list + elem_size*index_2, elem_size);
        memcpy((uint8_t*) list + elem_size*index_2, tmp, elem_size);
    }
    free(tmp);
}

