#include <stdint.h>

#include "utils/ExtendedTypes.h"


float16_t f16_ZERO = (uint16_t) 0x0000;
bfloat16_t bf16_ZERO = (uint16_t) 0x0000;

float16_t f16_ONE = (uint16_t) 0x3c00;
bfloat16_t bf16_ONE = (uint16_t) 0x3f80;

float16_t f16_TWO = (uint16_t) 0x4000;
bfloat16_t bf16_TWO = (uint16_t) 0x4000;

float16_t f16_INF = (uint16_t) 0x7c00;
bfloat16_t bf16_INF = (uint16_t) 0x7f80;

float16_t f16_MAX_INT = (uint16_t) 0x6800;   /* 2048 */
bfloat16_t bf16_MAX_INT = (uint16_t) 0x4380; /* 256 */



// *************************************************
//float16_t

/* ============================================================
   CLASSIFICATION
   ============================================================ */
typedef enum {
    F16_ZERO,
    F16_SUBNORMAL,
    F16_NORMAL,
    F16_INF,
    F16_NAN
} f16_class;

/* ============================================================
   FLOAT32 -> FLOAT16
   ============================================================ */
float16_t f16_from_float(float f32) {
    uint32_t bits = *(uint32_t*)&f32;
    uint32_t sign = bits >> 31;
    int exp32 = (bits >> 23) & 0xFF;
    uint32_t mant32 = bits & 0x7FFFFF;

    uint16_t exp16, mant16;

    if (exp32 == 0xFF) {
        exp16 = 0x1F;
        mant16 = mant32 ? 0x200 : 0;
    } else if (exp32 == 0) {
        exp16 = 0;
        mant16 = 0;
    } else {
        int new_exp = exp32 - 127 + 15;
        if (new_exp >= 0x1F) {
            exp16 = 0x1F;
            mant16 = 0;
        } else if (new_exp <= 0) {
            uint32_t m = mant32 | 0x800000;
            int shift = 1 - new_exp;
            if (shift > 24) {
                exp16 = 0;
                mant16 = 0;
            } else {
                m += 1u << (shift - 1);
                mant16 = m >> (shift + 13);
                exp16 = 0;
            }
        } else {
            mant32 += 1u << 12;
            mant16 = mant32 >> 13;
            exp16 = new_exp;
            if (mant16 == 0x400) {
                mant16 = 0;
                exp16++;
            }
        }
    }

    return (sign << 15) | (exp16 << 10) | (mant16 & 0x3FF);
}

/* ============================================================
   FLOAT16 -> FLOAT32
   ============================================================ */
float float_from_f16(float16_t f16) {
    uint32_t sign = (f16 >> 15) & 1;
    uint32_t exp = (f16 >> 10) & 0x1F;
    uint32_t mant = f16 & 0x3FF;

    uint32_t exp32, mant32;

    if (exp == 0) {
        if (mant == 0) {
            exp32 = 0;
            mant32 = 0;
        } else {
            int shift = 0;
            while ((mant & 0x400) == 0) {
                mant <<= 1;
                shift++;
            }
            mant &= 0x3FF;
            exp32 = 127 - 14 - shift;
            mant32 = mant << 13;
        }
    } else if (exp == 0x1F) {
        exp32 = 0xFF;
        mant32 = mant << 13;
    } else {
        exp32 = exp - 15 + 127;
        mant32 = mant << 13;
    }

    uint32_t bits = (sign << 31) | (exp32 << 23) | mant32;
    return *(float*)&bits;
}

/* ============================================================
   UNPACK + CLASSIFY
   ============================================================ */
static f16_class f16_unpack(float16_t f, int *sign, int *exp, uint32_t *mant) {
    *sign = (f >> 15) & 1;
    uint16_t e = (f >> 10) & 0x1F;
    uint32_t m = f & 0x3FF;

    if (e == 0) {
        if (m == 0) {
            *exp = 0;
            *mant = 0;
            return F16_ZERO;
        }
        *exp = -14;
        *mant = m;
        return F16_SUBNORMAL;
    }

    if (e == 0x1F) {
        *exp = 0;
        *mant = m;
        return m ? F16_NAN : F16_INF;
    }

    *exp = e - 15;
    *mant = m | 0x400;
    return F16_NORMAL;
}

/* ============================================================
   PACK
   ============================================================ */
static float16_t f16_pack(int sign, int exp, uint32_t mant) {
    if (mant == 0)
        return sign << 15;

    while (mant >= (1u << 11)) {
        mant >>= 1;
        exp++;
    }
    while (mant < (1u << 10)) {
        mant <<= 1;
        exp--;
    }

    if (exp > 15)
        return (sign << 15) | (0x1F << 10);

    if (exp < -14) {
        int shift = (-14) - exp;
        mant = (shift >= 32) ? 0 : (mant + (1u << (shift - 1))) >> shift;
        return (sign << 15) | (mant & 0x3FF);
    }

    return (sign << 15) | ((exp + 15) << 10) | (mant & 0x3FF);
}

/* ============================================================
   ADD / SUB / MUL / DIV
   ============================================================ */
float16_t f16_add(float16_t a, float16_t b) {
    int sa, sb, ea, eb;
    uint32_t ma, mb;
    f16_class ca = f16_unpack(a, &sa, &ea, &ma);
    f16_class cb = f16_unpack(b, &sb, &eb, &mb);

    if (ca == F16_NAN || cb == F16_NAN) return 0x7E00;

    if (ca == F16_INF || cb == F16_INF) {
        if (ca == cb && sa != sb) return 0x7E00;
        int s = (ca == F16_INF) ? sa : sb;
        return (s << 15) | (0x1F << 10);
    }

    if (ca == F16_ZERO) return b;
    if (cb == F16_ZERO) return a;

    if (ea > eb) mb >>= (ea - eb);
    else if (eb > ea) ma >>= (eb - ea);

    int exp = ea > eb ? ea : eb;
    uint32_t mant;
    int sign;

    if (sa == sb) {
        mant = ma + mb;
        sign = sa;
    } else {
        if (ma >= mb) {
            mant = ma - mb;
            sign = sa;
        } else {
            mant = mb - ma;
            sign = sb;
        }
    }

    return f16_pack(sign, exp, mant);
}

float16_t f16_sub(float16_t a, float16_t b) {
    return f16_add(a, b ^ 0x8000);
}

float16_t f16_mult(float16_t a, float16_t b) {
    int sa, sb, ea, eb;
    uint32_t ma, mb;
    f16_class ca = f16_unpack(a, &sa, &ea, &ma);
    f16_class cb = f16_unpack(b, &sb, &eb, &mb);
    int sign = sa ^ sb;

    if (ca == F16_NAN || cb == F16_NAN) return 0x7E00;
    if ((ca == F16_INF && cb == F16_ZERO) ||
        (cb == F16_INF && ca == F16_ZERO)) return 0x7E00;
    if (ca == F16_INF || cb == F16_INF)
        return (sign << 15) | (0x1F << 10);
    if (ca == F16_ZERO || cb == F16_ZERO)
        return sign << 15;

    uint64_t prod = (uint64_t)ma * mb;
    int exp = ea + eb;

    if (prod & (1ULL << 21)) {
        prod >>= 11;
        exp++;
    } else {
        prod >>= 10;
    }

    return f16_pack(sign, exp, (uint32_t)prod);
}

float16_t f16_div(float16_t a, float16_t b) {
    int sa, sb, ea, eb;
    uint32_t ma, mb;
    f16_class ca = f16_unpack(a, &sa, &ea, &ma);
    f16_class cb = f16_unpack(b, &sb, &eb, &mb);
    int sign = sa ^ sb;

    if (ca == F16_NAN || cb == F16_NAN) return 0x7E00;
    if (ca == F16_INF && cb == F16_INF) return 0x7E00;
    if (cb == F16_INF) return sign << 15;
    if (ca == F16_INF) return (sign << 15) | (0x1F << 10);
    if (cb == F16_ZERO) return (sign << 15) | (0x1F << 10);
    if (ca == F16_ZERO) return sign << 15;

    int shift = 0;
    while (mb < (1u << 10)) {
        mb <<= 1;
        shift++;
    }

    uint64_t dividend = (uint64_t)ma << (10 + shift);
    uint32_t mant = dividend / mb;
    int exp = ea - eb - shift;

    return f16_pack(sign, exp, mant);
}

/* ============================================================
   UNARY
   ============================================================ */
float16_t f16_neg(float16_t x) { return x ^ 0x8000; }
float16_t f16_abs(float16_t x) { return x & 0x7FFF; }






// *************************************************
// bfloat16_t

/* ============================================================
   CLASSIFICATION
   ============================================================ */
typedef enum {
    BF16_ZERO,
    BF16_SUBNORMAL,   /* practically unused in bf16, but kept for symmetry */
    BF16_NORMAL,
    BF16_INF,
    BF16_NAN
} bf16_class;

/* ============================================================
   FLOAT32 -> BFLOAT16  (round-to-nearest-even)
   ============================================================ */
bfloat16_t bf16_from_float(float f32) {
    uint32_t bits = *(uint32_t*)&f32;
    uint32_t lsb = (bits >> 16) & 1;
    uint32_t rounding_bias = 0x7FFF + lsb;
    bits += rounding_bias;
    return (bfloat16_t)(bits >> 16);
}

/* ============================================================
   BFLOAT16 -> FLOAT32
   ============================================================ */
float float_from_bf16(bfloat16_t bf16) {
    uint32_t bits = ((uint32_t)bf16) << 16;
    return *(float*)&bits;
}

/* ============================================================
   UNPACK + CLASSIFY
   ============================================================ */
static bf16_class bf16_unpack(
    bfloat16_t f,
    int *sign,
    int *exp,
    uint32_t *mant
) {
    *sign = (f >> 15) & 1;
    uint16_t e = (f >> 7) & 0xFF;
    uint32_t m = f & 0x7F;

    if (e == 0) {
        if (m == 0) {
            *exp = 0;
            *mant = 0;
            return BF16_ZERO;
        }
        *exp = 1 - 127;
        *mant = m;
        return BF16_SUBNORMAL;
    }

    if (e == 0xFF) {
        *exp = 0;
        *mant = m;
        return m ? BF16_NAN : BF16_INF;
    }

    *exp = e - 127;
    *mant = m | 0x80;
    return BF16_NORMAL;
}

/* ============================================================
   PACK
   ============================================================ */
static bfloat16_t bf16_pack(int sign, int exp, uint32_t mant) {
    if (mant == 0)
        return sign << 15;

    while (mant >= (1u << 8)) {
        mant >>= 1;
        exp++;
    }
    while (mant < (1u << 7)) {
        mant <<= 1;
        exp--;
    }

    if (exp >= 128)
        return (sign << 15) | (0xFF << 7);

    if (exp <= -127) {
        int shift = (-127) - exp + 1;
        if (shift >= 32)
            mant = 0;
        else
            mant >>= shift;
        return (sign << 15) | (mant & 0x7F);
    }

    return (sign << 15) | ((exp + 127) << 7) | (mant & 0x7F);
}

/* ============================================================
   ADD
   ============================================================ */
bfloat16_t bf16_add(bfloat16_t a, bfloat16_t b) {
    int sa, sb, ea, eb;
    uint32_t ma, mb;
    bf16_class ca = bf16_unpack(a, &sa, &ea, &ma);
    bf16_class cb = bf16_unpack(b, &sb, &eb, &mb);

    if (ca == BF16_NAN || cb == BF16_NAN)
        return 0x7FC0;

    if (ca == BF16_INF || cb == BF16_INF) {
        if (ca == cb && sa != sb)
            return 0x7FC0;
        int s = (ca == BF16_INF) ? sa : sb;
        return (s << 15) | (0xFF << 7);
    }

    if (ca == BF16_ZERO) return b;
    if (cb == BF16_ZERO) return a;

    if (ea > eb) mb >>= (ea - eb);
    else if (eb > ea) ma >>= (eb - ea);

    int exp = (ea > eb) ? ea : eb;

    uint32_t mant;
    int sign;

    if (sa == sb) {
        mant = ma + mb;
        sign = sa;
    } else {
        if (ma >= mb) {
            mant = ma - mb;
            sign = sa;
        } else {
            mant = mb - ma;
            sign = sb;
        }
    }

    return bf16_pack(sign, exp, mant);
}

/* ============================================================
   SUB
   ============================================================ */
bfloat16_t bf16_sub(bfloat16_t a, bfloat16_t b) {
    return bf16_add(a, b ^ 0x8000);
}

/* ============================================================
   MUL
   ============================================================ */
bfloat16_t bf16_mult(bfloat16_t a, bfloat16_t b) {
    int sa, sb, ea, eb;
    uint32_t ma, mb;
    bf16_class ca = bf16_unpack(a, &sa, &ea, &ma);
    bf16_class cb = bf16_unpack(b, &sb, &eb, &mb);

    int sign = sa ^ sb;

    if (ca == BF16_NAN || cb == BF16_NAN)
        return 0x7FC0;

    if ((ca == BF16_INF && cb == BF16_ZERO) ||
        (cb == BF16_INF && ca == BF16_ZERO))
        return 0x7FC0;

    if (ca == BF16_INF || cb == BF16_INF)
        return (sign << 15) | (0xFF << 7);

    if (ca == BF16_ZERO || cb == BF16_ZERO)
        return sign << 15;

    uint64_t prod = (uint64_t)ma * mb;
    int exp = ea + eb;

    if (prod & (1ULL << 15)) {
        prod >>= 8;
        exp++;
    } else {
        prod >>= 7;
    }

    return bf16_pack(sign, exp, (uint32_t)prod);
}

/* ============================================================
   DIV
   ============================================================ */
bfloat16_t bf16_div(bfloat16_t a, bfloat16_t b) {
    int sa, sb, ea, eb;
    uint32_t ma, mb;
    bf16_class ca = bf16_unpack(a, &sa, &ea, &ma);
    bf16_class cb = bf16_unpack(b, &sb, &eb, &mb);

    int sign = sa ^ sb;

    if (ca == BF16_NAN || cb == BF16_NAN)
        return 0x7FC0;

    if (ca == BF16_INF && cb == BF16_INF)
        return 0x7FC0;

    if (cb == BF16_INF)
        return sign << 15;

    if (ca == BF16_INF)
        return (sign << 15) | (0xFF << 7);

    if (cb == BF16_ZERO)
        return (sign << 15) | (0xFF << 7);

    if (ca == BF16_ZERO)
        return sign << 15;

    int shift = 0;
    while (mb < (1u << 7)) {
        mb <<= 1;
        shift++;
    }

    uint64_t dividend = (uint64_t)ma << (7 + shift);
    uint32_t mant = dividend / mb;
    int exp = ea - eb - shift;

    return bf16_pack(sign, exp, mant);
}

/* ============================================================
   UNARY
   ============================================================ */
bfloat16_t bf16_neg(bfloat16_t x) { return x ^ 0x8000; }
bfloat16_t bf16_abs(bfloat16_t x) { return x & 0x7FFF; }






// *************************************************
// fint16_t
// *************************************************

fint16_t fi16_from_int(int i32) {
    const int mbits = 10;
    fint16_t fi16;
    
    int sign = (i32 < 0) ? 1 : 0;
    int abs_i32 = (i32 < 0) ? -i32 : i32;
    
    if (abs_i32 == 0) {
        return 0x0000;
    }
    
    int msb_pos = 0;
    int temp = abs_i32;
    while (temp >>= 1) {
        msb_pos++;
    }
    
    int exponent, mantissa;
    
    if (msb_pos < mbits) {
        exponent = 0;
        mantissa = abs_i32;
    } else {
        exponent = msb_pos - 9;
        int shift = exponent - 1;
        mantissa = (abs_i32 >> shift) - (1 << mbits);
    }
    
    fi16 = (sign << 15) | (exponent << mbits) | mantissa;
    
    if (mantissa < ((1 << mbits) - 1) || exponent < 31) {
        fint16_t fi16_plus_1 = fi16 + 1;
        
        int reconstructed = int_from_fi16(fi16);
        int reconstructed_plus_1 = int_from_fi16(fi16_plus_1);
        
        int dist_current = abs_i32 - reconstructed;
        if (dist_current < 0) dist_current = -dist_current;
        
        int dist_next = abs_i32 - reconstructed_plus_1;
        if (dist_next < 0) dist_next = -dist_next;
        
        if (dist_next < dist_current) {
            fi16 = fi16_plus_1;
        }
    }
    
    return fi16;
}

fint16_t fi16_from_long_long(long long int i64) {
    const int mbits = 10;
    fint16_t fi16;
    
    int sign = (i64 < 0) ? 1 : 0;
    long long int abs_i64 = (i64 < 0) ? -i64 : i64;
    
    if (abs_i64 == 0) {
        return 0x0000;
    }
    
    int msb_pos = 0;
    long long int temp = abs_i64;
    while (temp >>= 1) {
        msb_pos++;
    }
    
    int exponent, mantissa;
    
    if (msb_pos < mbits) {
        exponent = 0;
        mantissa = abs_i64;
    } else {
        exponent = msb_pos - 9;
        int shift = exponent - 1;
        mantissa = (abs_i64 >> shift) - (1 << mbits);
    }
    
    fi16 = (sign << 15) | (exponent << mbits) | mantissa;
    
    if (mantissa < ((1 << mbits) - 1) || exponent < 31) {
        fint16_t fi16_plus_1 = fi16 + 1;
        
        long long int reconstructed = long_long_from_fi16(fi16);
        long long int reconstructed_plus_1 = long_long_from_fi16(fi16_plus_1);
        
        long long int dist_current = abs_i64 - reconstructed;
        if (dist_current < 0) dist_current = -dist_current;
        
        long long int dist_next = abs_i64 - reconstructed_plus_1;
        if (dist_next < 0) dist_next = -dist_next;
        
        if (dist_next < dist_current) {
            fi16 = fi16_plus_1;
        }
    }
    
    return fi16;
}

int int_from_fi16(fint16_t fi16) {
    const int mbits = 10;
    int sign = fi16 & 0x8000;
    int exponent = (fi16 & 0x7fff) >> mbits;
    int mantissa = fi16 & ((1 << mbits) - 1);
    if (exponent == 0) {
        return sign ? -mantissa : mantissa;
    }
    return ((mantissa + (1 << mbits)) << (exponent - 1)) * (sign ? -1 : 1);
}

long long int long_long_from_fi16(fint16_t fi16) {
    const long long int mbits = 10;
    long long int sign = fi16 & 0x8000;
    long long int exponent = (fi16 & 0x7fff) >> mbits;
    long long int mantissa = fi16 & ((1 << mbits) - 1);
    if (exponent == 0) {
        return sign ? -mantissa : mantissa;
    }
    return ((mantissa + (1 << mbits)) << (exponent - 1)) * (sign ? -1 : 1);
}

fint16_t fi16_add(fint16_t fi16_a, fint16_t fi16_b) {
    const int mbits = 10;
    
    // Extract fields from both operands
    uint16_t sign_a = (fi16_a >> 15) & 1;
    uint16_t exp_a = (fi16_a >> mbits) & 0x1F;
    uint16_t mant_a = fi16_a & 0x3FF;
    
    uint16_t sign_b = (fi16_b >> 15) & 1;
    uint16_t exp_b = (fi16_b >> mbits) & 0x1F;
    uint16_t mant_b = fi16_b & 0x3FF;
    
    // Convert to actual mantissa values (add implicit 1 for normalized)
    uint32_t actual_mant_a = (exp_a == 0) ? mant_a : (mant_a + (1 << mbits));
    uint32_t actual_mant_b = (exp_b == 0) ? mant_b : (mant_b + (1 << mbits));
    
    // Handle special case: both are subnormal
    if (exp_a == 0 && exp_b == 0) {
        if (sign_a == sign_b) {
            int result_val = actual_mant_a + actual_mant_b;
            return fi16_from_int(sign_a ? -result_val : result_val);
        } else {
            int result_val, result_sign;
            if (actual_mant_a >= actual_mant_b) {
                result_val = actual_mant_a - actual_mant_b;
                result_sign = sign_a;
            } else {
                result_val = actual_mant_b - actual_mant_a;
                result_sign = sign_b;
            }
            return fi16_from_int(result_sign ? -result_val : result_val);
        }
    }
    
    // Align exponents
    uint16_t exp_result;
    if (exp_a > exp_b) {
        uint16_t shift = (exp_a > 0 ? exp_a - 1 : 0) - (exp_b > 0 ? exp_b - 1 : 0);
        actual_mant_b >>= shift;
        exp_result = exp_a;
    } else if (exp_b > exp_a) {
        uint16_t shift = (exp_b > 0 ? exp_b - 1 : 0) - (exp_a > 0 ? exp_a - 1 : 0);
        actual_mant_a >>= shift;
        exp_result = exp_b;
    } else {
        exp_result = exp_a;
    }
    
    // Perform addition/subtraction based on signs
    uint32_t result_mant;
    uint16_t result_sign;
    
    if (sign_a == sign_b) {
        result_mant = actual_mant_a + actual_mant_b;
        result_sign = sign_a;
    } else {
        if (actual_mant_a >= actual_mant_b) {
            result_mant = actual_mant_a - actual_mant_b;
            result_sign = sign_a;
        } else {
            result_mant = actual_mant_b - actual_mant_a;
            result_sign = sign_b;
        }
    }
    
    // Convert back to actual value
    int result_val;
    if (exp_result == 0) {
        result_val = result_mant;
    } else {
        result_val = result_mant << (exp_result - 1);
    }
    
    return fi16_from_int(result_sign ? -result_val : result_val);
}


fint16_t fi16_sub(fint16_t fi16_a, fint16_t fi16_b) {
    // Negate fi16_b by flipping its sign bit
    fint16_t fi16_b_negated = fi16_b ^ 0x8000;
    return fi16_add(fi16_a, fi16_b_negated);
}


fint16_t fi16_mult(fint16_t fi16_a, fint16_t fi16_b) {
    const int mbits = 10;
    
    // Handle zero
    if (fi16_a == 0 || fi16_b == 0) {
        return 0;
    }
    
    // Extract fields
    uint16_t sign_a = (fi16_a >> 15) & 1;
    uint16_t exp_a = (fi16_a >> mbits) & 0x1F;
    uint16_t mant_a = fi16_a & 0x3FF;
    
    uint16_t sign_b = (fi16_b >> 15) & 1;
    uint16_t exp_b = (fi16_b >> mbits) & 0x1F;
    uint16_t mant_b = fi16_b & 0x3FF;
    
    // Result sign (XOR of input signs)
    uint16_t result_sign = sign_a ^ sign_b;
    
    // Get actual mantissa values and shifts
    uint32_t actual_mant_a, shift_a;
    if (exp_a == 0) {
        actual_mant_a = mant_a;
        shift_a = 0;
    } else {
        actual_mant_a = mant_a + (1 << mbits);
        shift_a = exp_a - 1;
    }
    
    uint32_t actual_mant_b, shift_b;
    if (exp_b == 0) {
        actual_mant_b = mant_b;
        shift_b = 0;
    } else {
        actual_mant_b = mant_b + (1 << mbits);
        shift_b = exp_b - 1;
    }
    
    // Multiply mantissas (32-bit intermediate to avoid overflow)
    uint32_t result_mant = actual_mant_a * actual_mant_b;
    
    // Combine shifts
    uint32_t total_shift = shift_a + shift_b;
    
    // Compute result value (may need long long for large shifts)
    long long int result_val = ((long long int) result_mant) << total_shift;
    
    return fi16_from_long_long(result_sign ? -result_val : result_val);
}


fint16_t fi16_div(fint16_t fi16_a, fint16_t fi16_b) {
    const int mbits = 10;
    
    // Handle division by zero - return max value with appropriate sign
    if (fi16_b == 0) {
        return (fi16_a & 0x8000) ? 0xFFFF : 0x7FFF;
    }
    
    // Handle zero dividend
    if (fi16_a == 0) {
        return 0;
    }
    
    // Extract fields
    uint16_t sign_a = (fi16_a >> 15) & 1;
    uint16_t exp_a = (fi16_a >> mbits) & 0x1F;
    uint16_t mant_a = fi16_a & 0x3FF;
    
    uint16_t sign_b = (fi16_b >> 15) & 1;
    uint16_t exp_b = (fi16_b >> mbits) & 0x1F;
    uint16_t mant_b = fi16_b & 0x3FF;
    
    // Result sign (XOR of input signs)
    uint16_t result_sign = sign_a ^ sign_b;
    
    // Get actual values
    int val_a, val_b;
    if (exp_a == 0) {
        val_a = mant_a;
    } else {
        val_a = (mant_a + (1 << mbits)) << (exp_a - 1);
    }
    
    if (exp_b == 0) {
        val_b = mant_b;
    } else {
        val_b = (mant_b + (1 << mbits)) << (exp_b - 1);
    }
    
    // Perform division
    if (val_b == 0) {
        return result_sign ? 0xFFFF : 0x7FFF;
    }
    
    int result_val = val_a / val_b;
    
    return fi16_from_int(result_sign ? -result_val : result_val);
}

fint16_t fi16_neg(fint16_t x) { return x ^ 0x8000; }
fint16_t fi16_abs(fint16_t x) { return x & 0x7FFF; }



