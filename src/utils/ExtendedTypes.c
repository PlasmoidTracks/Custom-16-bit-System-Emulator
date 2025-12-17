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

#include <stdint.h>

typedef uint16_t float16_t;

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

#include <stdint.h>

typedef uint16_t bfloat16_t;

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

