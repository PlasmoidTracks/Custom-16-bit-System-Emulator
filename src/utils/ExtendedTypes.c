#include <stdint.h>

#include "utils/ExtendedTypes.h"


float16_t f16_ZERO = (uint16_t) 0x0000;
bfloat16_t bf16_ZERO = (uint16_t) 0x0000;

float16_t f16_ONE = (uint16_t) 0x3c00;
bfloat16_t bf16_ONE = (uint16_t) 0x3f80;

float16_t f16_TWO = (uint16_t) 0x4000;
bfloat16_t bf16_TWO = (uint16_t) 0x4000;



// *************************************************
//float16_t

// Converts a 32-bit float to a 16-bit half-precision float,
// including proper treatment of subnormals.
float16_t f16_from_float(float f32) {
    uint32_t f32_bits = *((uint32_t*)(&f32));
    uint32_t sign = f32_bits >> 31;
    int32_t exp32 = (f32_bits >> 23) & 0xFF;
    uint32_t mant32 = f32_bits & 0x007FFFFF;

    uint16_t sign16 = (uint16_t)sign;
    uint16_t exp16 = 0;
    uint16_t mant16 = 0;

    if (exp32 == 0xFF) {
        // Handle Inf/NaN.
        exp16 = 0x1F;
        mant16 = (mant32 ? 0x200 : 0);
    } else if (exp32 == 0) {
        // Zero or float32 subnormal -- flush to zero.
        exp16 = 0;
        mant16 = 0;
    } else {
        // Normalized float32 number.
        int32_t new_exp = exp32 - 127 + 15;
        if (new_exp >= 0x1F) {
            // Overflow: set to infinity.
            exp16 = 0x1F;
            mant16 = 0;
        } else if (new_exp <= 0) {
            // Underflow: produce a subnormal float16.
            // Add the implicit 1 to mantissa.
            uint32_t sub_mant = mant32 | 0x00800000;
            // Calculate how many bits to shift: shift = (1 - new_exp).
            int shift = 1 - new_exp;
            if (shift > 24) {
                // Too small: flush to zero.
                exp16 = 0;
                mant16 = 0;
            } else {
                // Add rounding bias: half the value of the last bit to be shifted out.
                uint32_t rounding = 1 << (shift - 1);
                sub_mant += rounding;
                // Shift to get the 10-bit mantissa.
                mant16 = sub_mant >> (shift + 13);
                // If rounding causes the mantissa to overflow (i.e. equals 0x400),
                // then we must bump the exponent to 1 (normalized minimal)
                if (mant16 == 0x400) {
                    mant16 = 0;
                    exp16 = 1;
                } else {
                    exp16 = 0;
                }
            }
        } else {
            // Normal case: represent as normalized.
            exp16 = new_exp;
            // Round the mantissa: add rounding bias (1 << 12) then shift.
            uint32_t rounding = 1 << 12;
            uint32_t round_mant = mant32 + rounding;
            mant16 = round_mant >> 13;
            if (mant16 == 0x400) { // Rounding overflow.
                mant16 = 0;
                exp16++;
            }
        }
    }
    return (sign16 << 15) | (exp16 << 10) | (mant16 & 0x3FF);
}


// Converts a 16-bit half-precision float to a 32-bit float,
// properly normalizing subnormal numbers.
float float_from_f16(float16_t f16) {
    uint16_t sign = f16 >> 15;
    uint16_t exp  = (f16 >> 10) & 0x1F;
    uint16_t mant = f16 & 0x3FF;
    
    uint32_t sign32 = sign << 31;
    uint32_t exp32, mant32;
    
    if (exp == 0) {
        if (mant == 0) {
            // Zero value.
            exp32 = 0;
            mant32 = 0;
        } else {
            // Subnormal half: normalize it.
            int shift = 0;
            while ((mant & 0x400) == 0) { // while the 11th bit is not set
                mant <<= 1;
                shift++;
            }
            mant &= 0x3FF; // remove the hidden bit now
            exp32 = (127 - 14) - shift;  // effective exponent: -14 - shift, then add bias (127)
            mant32 = mant << 13;
        }
    } else if (exp == 0x1F) {
        // Inf or NaN.
        exp32 = 0xFF;
        mant32 = (mant ? (mant << 13) : 0);
    } else {
        // Normalized half.
        exp32 = exp - 15 + 127;
        mant32 = mant << 13;
    }
    
    uint32_t f32 = sign32 | (exp32 << 23) | mant32;
    return *((float*)&f32);
}

// Unpacks a float16_t into its sign, effective exponent, and mantissa.
// For normalized numbers, the mantissa has the implicit bit added;
// for subnormals, the effective exponent is set to -14.
static void f16_unpack(float16_t f, int *sign, int *exp, uint32_t *mant) {
    *sign = (f >> 15) & 1;
    int e = (f >> 10) & 0x1F;
    uint32_t m = f & 0x3FF;
    if (e == 0) {
        // For subnormals or zero:
        *exp = (m == 0) ? -15 : -14;
        *mant = m;
    } else {
        *exp = e - 15;
        *mant = m | 0x400;  // add implicit 1 (bit 10)
    }
}

// Packs sign, effective exponent, and mantissa into a float16_t.
// If the effective exponent is below -14, the number is packed as subnormal.
static float16_t f16_pack(int sign, int exp, uint32_t mant) {
    // If the result is zero, pack and return.
    if (mant == 0)
        return (sign << 15);
    
    // Normalize the mantissa so that it fits in 11 bits (with the implicit bit)
    while (mant >= (1 << 11)) { // too large: shift right and increase exponent
        mant >>= 1;
        exp++;
    }
    while (mant && mant < (1 << 10)) { // too small: shift left and decrease exponent
        mant <<= 1;
        exp--;
    }
    
    if (exp > 15) { // overflow: set to infinity
        return (sign << 15) | (0x1F << 10);
    }
    
    if (exp < -14) {
        // Subnormal: shift the mantissa to the right.
        int shift = (-14) - exp;
        if (shift < 32)
            mant = (mant + (1 << (shift - 1))) >> shift;  // rounding
        else
            mant = 0;
        // Encoded exponent for subnormals is zero.
        return (sign << 15) | (0) | (mant & 0x3FF);
    }
    
    // Normalized result: remove the implicit bit.
    int encoded_exp = exp + 15;
    mant = mant - (1 << 10);
    return (sign << 15) | ((encoded_exp & 0x1F) << 10) | (mant & 0x3FF);
}

// Adjusts exponents by shifting mantissas so that the operands have the same effective exponent.
static int adjust_exponent16(uint32_t *mant1, int exp1, uint32_t *mant2, int exp2) {
    if (exp1 > exp2) {
        *mant2 >>= (exp1 - exp2);
        return 0; // first operand has the greater effective exponent
    } else if (exp2 > exp1) {
        *mant1 >>= (exp2 - exp1);
        return 1; // second operand has the greater effective exponent
    }
    return -1; // exponents are equal
}

// Adds two float16_t values.
float16_t f16_add(float16_t f16_a, float16_t f16_b) {
    int sign1, sign2, exp1, exp2;
    uint32_t mant1, mant2;
    f16_unpack(f16_a, &sign1, &exp1, &mant1);
    f16_unpack(f16_b, &sign2, &exp2, &mant2);
    
    // Align exponents by shifting the smaller mantissa
    if (adjust_exponent16(&mant1, exp1, &mant2, exp2))
        exp1 = exp2;
    else
        exp2 = exp1;
    
    uint32_t result_mant;
    int result_sign;
    if (sign1 == sign2) {
        result_mant = mant1 + mant2;
        result_sign = sign1;
    } else {
        if (mant1 > mant2) {
            result_mant = mant1 - mant2;
            result_sign = sign1;
        } else {
            result_mant = mant2 - mant1;
            result_sign = sign2;
        }
    }
    int result_exp = exp1; // effective exponent
    return f16_pack(result_sign, result_exp, result_mant);
}

// Subtracts two float16_t values.
float16_t f16_sub(float16_t f16_a, float16_t f16_b) {
    // Subtraction is addition with the second operand negated.
    return f16_add(f16_a, f16_neg(f16_b));
}

// Multiplies two float16_t values.
float16_t f16_mult(float16_t f16_a, float16_t f16_b) {
    int sign1, sign2, exp1, exp2;
    uint32_t mant1, mant2;
    f16_unpack(f16_a, &sign1, &exp1, &mant1);
    f16_unpack(f16_b, &sign2, &exp2, &mant2);
    
    int result_sign = sign1 ^ sign2;
    // Multiply the mantissas (each up to 11 bits) using 64-bit precision.
    uint64_t result_mant = (uint64_t)mant1 * mant2;
    int result_exp = exp1 + exp2;  // effective exponents add
    
    // The result mantissa now is roughly 22 bits; shift it to 11 bits.
    if (result_mant & (1ULL << 21)) {
        result_mant >>= 11;
        result_exp++;
    } else {
        result_mant >>= 10;
    }

    return f16_pack(result_sign, result_exp, (uint32_t)result_mant);
}

// Divides two float16_t values.
float16_t f16_div(float16_t f16_a, float16_t f16_b) {
    int sign1, sign2, exp1, exp2;
    uint32_t mant1, mant2;
    f16_unpack(f16_a, &sign1, &exp1, &mant1);
    f16_unpack(f16_b, &sign2, &exp2, &mant2);
    
    // Check for division by zero.
    if (mant2 == 0)
        return f16_pack(sign1 ^ sign2, 16, 0); // yields infinity
    
    // Normalize the divisor’s mantissa if needed.
    int shift = 0;
    while (mant2 < (1 << 10)) {
        mant2 <<= 1;
        shift++;
    }
    uint64_t dividend = (uint64_t)mant1 << (10 + shift);  // extra precision for division
    uint32_t result_mant = (uint32_t)(dividend / mant2);
    int result_exp = exp1 - exp2 - shift;
    
    return f16_pack(sign1 ^ sign2, result_exp, result_mant);
}

// Negates the float16_t value.
float16_t f16_neg(float16_t f16) {
    return f16 ^ 0x8000;
}

// Returns the absolute value.
float16_t f16_abs(float16_t f16) {
    return f16 & 0x7FFF;
}





// *************************************************
// bfloat16_t

// Converts float to bfloat16
bfloat16_t bf16_from_float(float f32) {
    uint32_t f32_bits = *((uint32_t*)(&f32));

    // Extract the sign, exponent, and mantissa
    uint32_t sign32 = (f32_bits >> 31) & 0x1;
    uint32_t exponent32 = (f32_bits >> 23) & 0xFF;
    uint32_t mantissa32 = (f32_bits >> 16) & 0x7F;  // bf16 has 7 mantissa bits

    // Round the mantissa according to the cutoff bit (the 8th bit of mantissa in float32)
    mantissa32 += (f32_bits >> 15) & 1;

    // Pack into bfloat16: sign (1) | exponent (8) | mantissa (7)
    return (sign32 << 15) | (exponent32 << 7) | mantissa32;
}

// Converts bfloat16 to float
float float_from_bf16(bfloat16_t bf16) {
    // Extract the sign, exponent, and mantissa
    uint32_t sign16 = (bf16 >> 15) & 0x1;
    uint32_t exponent16 = (bf16 >> 7) & 0xFF;
    uint32_t mantissa16 = bf16 & 0x7F;

    // Convert to float32 format: sign (1) | exponent (8) | mantissa (23)
    uint32_t f32_bits = (sign16 << 31) | (exponent16 << 23) | (mantissa16 << 16);

    return *((float*)(&f32_bits));
}

// Unpacks a bfloat16 into sign, exponent, and mantissa
static void bf16_unpack(bfloat16_t bf16, int *sign, int *exp, uint32_t *mant) {
    *sign = (bf16 >> 15) & 1;
    *exp = (bf16 >> 7) & 0xFF;
    *mant = bf16 & 0x7F;
    if (*exp != 0) {
        *mant |= (1 << 7);  // Implicit leading one for normalized values
    }
}

// Packs sign, exponent, and mantissa into a bfloat16
static bfloat16_t bf16_pack(int sign, int exp, uint32_t mant) {
    if (exp <= 0) {  // Handle subnormals or zero
        mant >>= 1 - exp;  // Right shift mantissa to fit into subnormal range
        exp = 0;
    } else if (exp >= 255) {  // Handle overflow to infinity
        mant = 0;
        exp = 255;
    } else {
        // Normalize mantissa
        while (mant >= (1 << 8)) {
            mant >>= 1;
            exp++;
        }
        while (mant && mant < (1 << 7)) {
            mant <<= 1;
            exp--;
        }
    }
    return ((sign & 1) << 15) | ((exp & 0xFF) << 7) | (mant & 0x7F);
}

// Adjusts exponents by shifting mantissas
static int adjust_exponent_bf16(uint32_t *mant1, int exp1, uint32_t *mant2, int exp2) {
    if (exp1 > exp2) {
        *mant2 >>= (exp1 - exp2);
        return 0;  // Indicates the first operand has the greater exponent
    } else if (exp2 > exp1) {
        *mant1 >>= (exp2 - exp1);
        return 1;  // Indicates the second operand has the greater exponent
    }
    return -1;  // Indicates exponents are equal
}

// Adds two bfloat16 values
bfloat16_t bf16_add(bfloat16_t bf16_a, bfloat16_t bf16_b) {
    int sign1, sign2, exp1, exp2;
    uint32_t mant1, mant2;

    bf16_unpack(bf16_a, &sign1, &exp1, &mant1);
    bf16_unpack(bf16_b, &sign2, &exp2, &mant2);

    // Align mantissas
    if (adjust_exponent_bf16(&mant1, exp1, &mant2, exp2)) {
        exp1 = exp2;
    } else {
        exp2 = exp1;
    }

    // Perform addition or subtraction based on sign
    uint32_t result_mant;
    int result_sign;
    if (sign1 == sign2) {
        result_mant = mant1 + mant2;
        result_sign = sign1;
    } else {
        if (mant1 > mant2) {
            result_mant = mant1 - mant2;
            result_sign = sign1;
        } else {
            result_mant = mant2 - mant1;
            result_sign = sign2;
        }
    }

    // Normalize result
    return bf16_pack(result_sign, exp1, result_mant);
}

// Subtracts two bfloat16 values
bfloat16_t bf16_sub(bfloat16_t bf16_a, bfloat16_t bf16_b) {
    return bf16_add(bf16_a, bf16_neg(bf16_b));
}

// Multiplies two bfloat16 values
bfloat16_t bf16_mult(bfloat16_t bf16_a, bfloat16_t bf16_b) {
    int sign1, sign2, exp1, exp2;
    uint32_t mant1, mant2;

    bf16_unpack(bf16_a, &sign1, &exp1, &mant1);
    bf16_unpack(bf16_b, &sign2, &exp2, &mant2);

    // Calculate the sign of the result
    int result_sign = sign1 ^ sign2;

    // Multiply the mantissas
    uint64_t result_mant = (uint64_t)mant1 * (uint64_t)mant2;

    // Adjust exponent
    int result_exp = exp1 + exp2 - 127;
    if (result_mant >= (1ULL << 15)) {
        result_mant >>= 8;
        result_exp++;
    } else {
        result_mant >>= 7;
    }

    return bf16_pack(result_sign, result_exp, (uint32_t)result_mant);
}

// Divides two bfloat16 values
bfloat16_t bf16_div(bfloat16_t bf16_a, bfloat16_t bf16_b) {
    int sign1, sign2, exp1, exp2;
    uint32_t mant1, mant2;

    bf16_unpack(bf16_a, &sign1, &exp1, &mant1);
    bf16_unpack(bf16_b, &sign2, &exp2, &mant2);

    if (mant2 == 0) {  // Handle division by zero
        return bf16_pack(sign1 ^ sign2, 255, 0);  // Return infinity with appropriate sign
    }

    // Normalize the divisor to improve division accuracy
    int shift = 0;
    while (mant2 < (1 << 7)) {
        mant2 <<= 1;
        shift++;
    }

    uint64_t dividend = (uint64_t)mant1 << (7 + shift);  // Increase precision for the division
    uint32_t result_mant = (uint32_t)(dividend / mant2);  // Perform the division

    int result_exp = exp1 - exp2 - shift + 127;

    return bf16_pack(sign1 ^ sign2, result_exp, result_mant);
}

bfloat16_t bf16_neg(bfloat16_t bf16) {
    return bf16 ^= 0x8000;
}

bfloat16_t bf16_abs(bfloat16_t bf16) {
    return bf16 &= 0x7fff;
}
