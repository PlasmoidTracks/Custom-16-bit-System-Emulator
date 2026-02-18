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
typedef uint16_t bfloat16_t;
typedef uint16_t fint16_t;

extern float16_t f16_ZERO;
extern bfloat16_t bf16_ZERO;

extern float16_t f16_ONE;
extern bfloat16_t bf16_ONE;

extern float16_t f16_TWO;
extern bfloat16_t bf16_TWO;

extern float16_t f16_INF;
extern bfloat16_t bf16_INF;

extern float16_t f16_MAX_INT;
extern bfloat16_t bf16_MAX_INT;


extern float16_t f16_from_float(float f32);
extern float float_from_f16(float16_t f16);
extern float16_t f16_add(float16_t f16_a, float16_t f16_b);
extern float16_t f16_sub(float16_t f16_a, float16_t f16_b);
extern float16_t f16_mult(float16_t f16_a, float16_t f16_b);
extern float16_t f16_div(float16_t f16_a, float16_t f16_b);
extern float16_t f16_neg(float16_t f16);
extern float16_t f16_abs(float16_t f16);


extern bfloat16_t bf16_from_float(float f32);
extern float float_from_bf16(bfloat16_t bf16);
extern bfloat16_t bf16_add(bfloat16_t bf16_a, bfloat16_t bf16_b);
extern bfloat16_t bf16_sub(bfloat16_t bf16_a, bfloat16_t bf16_b);
extern bfloat16_t bf16_mult(bfloat16_t bf16_a, bfloat16_t bf16_b);
extern bfloat16_t bf16_div(bfloat16_t bf16_a, bfloat16_t bf16_b);
extern bfloat16_t bf16_neg(bfloat16_t bf16);
extern bfloat16_t bf16_abs(bfloat16_t bf16);


extern fint16_t fi16_from_int(int i32);
extern fint16_t fi16_from_long_long(long long int i64);
extern int int_from_fi16(fint16_t fi16);
extern long long int long_long_from_fi16(fint16_t fi16);
extern fint16_t fi16_add(fint16_t fi16_a, fint16_t fi16_b);
extern fint16_t fi16_sub(fint16_t fi16_a, fint16_t fi16_b);
extern fint16_t fi16_mult(fint16_t fi16_a, fint16_t fi16_b);
extern fint16_t fi16_div(fint16_t fi16_a, fint16_t fi16_b);
extern bfloat16_t bf16_neg(bfloat16_t bf16);
extern bfloat16_t bf16_abs(bfloat16_t bf16);

#endif
