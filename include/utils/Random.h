#ifndef _RANDOM_H_
#define _RANDOM_H_

#include <stdlib.h>
#include <stdint.h>

extern uint64_t CRAND_SEED;
extern void (*CRAND_FUNCTION)(void);


// this function sets the seed of the custom random function
extern void rand_set_seed(uint64_t seed);

// this function returns the current seed
uint64_t rand_get_seed(void);

// this function sets the seed to the current (milli-)second in UTC
extern void randomize(void);

extern void rand_set_sampler(void(*function)(void));


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
extern int rand1(void);
extern uint8_t rand8(void);
extern uint16_t rand16(void);
extern uint32_t rand32(void);
extern uint64_t rand64(void);

extern int8_t rands8(void);
extern int16_t rands16(void);
extern int32_t rands32(void);
extern int64_t rands64(void);

// returns a uniformly random float value between 0.0 and 1.0
extern float frandf(void);
extern double frand(void);
extern long double frandl(void);

// returns a uniformly random float value between min and max
extern float funiformf(float min, float max);
extern double funiform(double min, double max);
extern long double funiforml(long double min, long double max);

// returns a uniformly random float value between min and max 
// (including min, excluding max)
extern uint8_t uniform8(uint8_t min, uint8_t max);
extern uint16_t uniform16(uint16_t min, uint16_t max);
extern uint32_t uniform32(uint32_t min, uint32_t max);
extern uint64_t uniform64(uint64_t min, uint64_t max);

extern void* choose_ext(void* list, size_t elem_size, uint64_t array_length);
#define choose(list, type, array_length) *(type*)choose_ext(list, sizeof(type), array_length)

extern void shuffle_ext(void* list, size_t elem_size, uint64_t array_length);
#define shuffle(list, array_length) do{shuffle_ext(list, sizeof(list[0]), array_length);}while(0)

#endif
