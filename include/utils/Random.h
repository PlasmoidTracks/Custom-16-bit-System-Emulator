#ifndef _RANDOM_H_
#define _RANDOM_H_

#include <stdint.h>

extern uint64_t CRAND_SEED;
extern void (*CRAND_FUNCTION)(void);


// this function sets the seed of the custom random function
extern void rand_set_seed(uint64_t seed);

// this function returns the current seed
uint64_t rand_get_seed(void);

// this function sets the seed to the current (milli-)second in UTC
extern void randomize(void);


// this sampler uniformly chooses a random number. 
extern void RAND_UPDATE_UNIFORM(void);

// this sampler chooses every possible number exactly once before repeating the sequence
// NOTE: This behaviour is only given for powers of two. Modulo operations may exibit very 
// non-random behavior. 
extern void RAND_UPDATE_DETERMINISTIC_CYCLIC(void);


// returns a uniformly random value of 8, 16, 32 or 64 bit full size
// NOTE: these random functions return a unique value every time it is called 
// until all possible values were called, before repeating the sequence. 
// this does not necessarily include modulo operations. 
extern uint64_t rand64(void);

#endif
