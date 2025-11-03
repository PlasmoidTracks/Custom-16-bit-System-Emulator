#ifndef _EXTENDEDTYPES_H_
#define _EXTENDEDTYPES_H_

#include <stdint.h>

/*
NOTE: 

all floating point types besides float16_t do NOT have the denormalized (subnormals) implemented. They also are the only one with NaN and inf support afaik. 
Ask Chatgpt to adjust the others to include subnormals, and givt the code of float16_t operations as a template for a working version. 
Extensive testing hasnt been done yet. 
*/

typedef uint16_t float16_t;

extern float16_t f16_ZERO;

extern float16_t f16_ONE;

extern float16_t f16_TWO;


extern float16_t f16_from_float(float f32);
extern float float_from_f16(float16_t f16);
extern float16_t f16_add(float16_t f16_a, float16_t f16_b);
extern float16_t f16_sub(float16_t f16_a, float16_t f16_b);
extern float16_t f16_mult(float16_t f16_a, float16_t f16_b);
extern float16_t f16_div(float16_t f16_a, float16_t f16_b);
extern float16_t f16_neg(float16_t f16);
extern float16_t f16_abs(float16_t f16);


#endif
