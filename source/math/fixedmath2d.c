
// BAHEY------
/*
 * fixedmath2d.c - Implementation of heavy vector operations
 *
 * Contains:
 *   - Quarter-wave sin LUT (129 entries)
 *   - Fixed_Sin, Fixed_Cos
 *   - Integer square root
 *   - Vec2_Len, Vec2_Normalize, Vec2_ClampLen
 *   - Vec2_FromAngle, Vec2_ToAngle, Vec2_Rotate
 *   - Mat2_Scale, Mat2_Rotate
 */

#include "fixedmath2d.h"

/*=============================================================================
 * SIN/COS LOOKUP TABLE
 *
 * Quarter-wave table storing sin(0°) to sin(90°) in Q16.8 format.
 * Full sine wave reconstructed by symmetry:
 *   - sin(90° + x)  =  sin(90° - x)   [mirror]
 *   - sin(180° + x) = -sin(x)         [negate]
 *
 * 129 entries: indices 0-128 map to angles 0-128 (0° to 90°)
 * Values range from 0 to 256 (FIXED_ONE)
 *
 * Generated with:
 *   for i in range(129):
 *       rad = i * (pi / 256)
 *       val = int(round(sin(rad) * 256))
 *===========================================================================*/

static const int16_t sin_lut[129] = {
    0,   3,   6,   9,   13,  16,  19,  22,  25,  28,  31,  34,  38,  41,  44,
    47,  50,  53,  56,  59,  62,  65,  68,  71,  74,  77,  80,  83,  86,  89,
    92,  95,  98,  101, 104, 107, 109, 112, 115, 118, 121, 123, 126, 129, 132,
    134, 137, 140, 142, 145, 147, 150, 152, 155, 157, 160, 162, 165, 167, 170,
    172, 174, 177, 179, 181, 183, 185, 188, 190, 192, 194, 196, 198, 200, 202,
    204, 206, 207, 209, 211, 213, 215, 216, 218, 220, 221, 223, 224, 226, 227,
    229, 230, 231, 233, 234, 235, 237, 238, 239, 240, 241, 242, 243, 244, 245,
    246, 247, 248, 248, 249, 250, 250, 251, 252, 252, 253, 253, 254, 254, 254,
    255, 255, 255, 256, 256, 256, 256, 256, 256,
};
/* Note: sin_lut[128] = 256 = FIXED_ONE (sin 90° = 1.0) */

/*=============================================================================
 * TRIG FUNCTIONS
 *===========================================================================*/

Q16_8 Fixed_Sin(int angle) {
    /* Wrap to 0-511 */
    int a = angle & ANGLE_MASK;

    /* Determine quadrant (0-3) and index within quadrant */
    int quadrant = a >> 7;             /* a / 128 */
    int idx = a & (ANGLE_QUARTER - 1); /* a % 128 */

    Q16_8 val;

    /* Use symmetry to get value from quarter-wave table */
    if (quadrant & 1) {
        /* Quadrants 1, 3: mirror (count down from 128) */
        val = sin_lut[ANGLE_QUARTER - idx];
    } else {
        /* Quadrants 0, 2: direct lookup */
        val = sin_lut[idx];
    }

    /* Quadrants 2, 3: negate */
    if (quadrant >= 2) {
        val = -val;
    }

    return val;
}

Q16_8 Fixed_Cos(int angle) {
    /* cos(x) = sin(x + 90°) */
    return Fixed_Sin(angle + ANGLE_QUARTER);
}

/*=============================================================================
 * INTEGER SQUARE ROOT
 *
 * Classic bitwise algorithm. No floating point.
 * Returns floor(sqrt(n)).
 *===========================================================================*/

static uint32_t isqrt(uint64_t n) {
    uint64_t res = 0;
    uint64_t bit = 1ull << 62; /* Highest power of 4 <= 2^64 */

    /* Find highest bit */
    while (bit > n) {
        bit >>= 2;
    }

    /* Compute sqrt bit by bit */
    while (bit != 0) {
        if (n >= res + bit) {
            n -= res + bit;
            res = (res >> 1) + bit;
        } else {
            res >>= 1;
        }
        bit >>= 2;
    }

    return (uint32_t)res;
}

/*=============================================================================
 * VEC2 HEAVY OPERATIONS
 *===========================================================================*/

Q16_8 Vec2_Len(Vec2 a) {
    Q16_8 len2 = Vec2_LenSquared(a);
    if (len2 <= 0) {
        return 0;
    }

    /*
     * len2 is Q16.8 (result of two Q16.8 multiplied and shifted)
     * To get correct Q16.8 length:
     *   1. Shift len2 up by FIXED_SHIFT to get Q24.16
     *   2. Take integer sqrt -> already Q16.8 (no further shift needed)
     */
    uint64_t len2_shifted = ((uint64_t)len2) << FIXED_SHIFT;
    uint32_t sqrt_result = isqrt(len2_shifted);  // Q16.8
    return (Q16_8)sqrt_result;
}

Vec2 Vec2_Normalize(Vec2 a) {
    if (Vec2_IsZero(a)) {
        return Vec2_Zero();
    }

    Q16_8 len = Vec2_Len(a);
    if (len == 0) {
        return Vec2_Zero();
    }

    return Vec2_Create(FixedDiv(a.x, len), FixedDiv(a.y, len));
}

Vec2 Vec2_ClampLen(Vec2 v, Q16_8 maxLen) {
    if (maxLen <= 0) {
        return Vec2_Zero();
    }

    Q16_8 len2 = Vec2_LenSquared(v);
    Q16_8 max2 = FixedMul(maxLen, maxLen);

    if (len2 <= max2) {
        return v;
    }

    /* Scale down to maxLen */
    Vec2 normalized = Vec2_Normalize(v);
    return Vec2_Scale(normalized, maxLen);
}

/*=============================================================================
 * VEC2 ANGLE OPERATIONS
 *===========================================================================*/

Vec2 Vec2_FromAngle(int angle) {
    return Vec2_Create(Fixed_Cos(angle), Fixed_Sin(angle));
}

int Vec2_ToAngle(Vec2 v) {
    if (Vec2_IsZero(v)) {
        return 0;
    }

    /*
     * Compute atan2 using binary search on the sin/cos LUT.
     * This is approximate but avoids floating point.
     *
     * Strategy:
     *   1. Normalize to unit vector
     *   2. Determine quadrant from signs
     *   3. Binary search in first quadrant
     *   4. Adjust for actual quadrant
     */

    /* Get absolute values for first-quadrant lookup */
    // Q16_8 ax = FixedAbs(v.x);
    Q16_8 ay = FixedAbs(v.y);

    /* Normalize (approximately - we just need the ratio) */
    Q16_8 len = Vec2_Len(v);
    if (len == 0) {
        return 0;
    }

    /* Compute sin of angle: opposite/hypotenuse = ay/len */
    Q16_8 sinVal = FixedDiv(ay, len);

    /* Binary search for angle in first quadrant (0 to 128) */
    int lo = 0;
    int hi = ANGLE_QUARTER;
    while (lo < hi) {
        int mid = (lo + hi + 1) >> 1;
        if (sin_lut[mid] <= sinVal) {
            lo = mid;
        } else {
            hi = mid - 1;
        }
    }

    int angle = lo;

    /* Adjust based on quadrant */
    if (v.x < 0 && v.y >= 0) {
        /* Quadrant 2: 128-256 */
        angle = ANGLE_HALF - angle;
    } else if (v.x < 0 && v.y < 0) {
        /* Quadrant 3: 256-384 */
        angle = ANGLE_HALF + angle;
    } else if (v.x >= 0 && v.y < 0) {
        /* Quadrant 4: 384-512 */
        angle = ANGLE_FULL - angle;
    }
    /* Quadrant 1 (x >= 0, y >= 0): angle stays as-is */

    return angle & ANGLE_MASK;
}

Vec2 Vec2_Rotate(Vec2 v, int angle) {
    Q16_8 c = Fixed_Cos(angle);
    Q16_8 s = Fixed_Sin(angle);

    return Vec2_Create(FixedMul(v.x, c) - FixedMul(v.y, s),
                       FixedMul(v.x, s) + FixedMul(v.y, c));
}

/*=============================================================================
 * MAT2 CONSTRUCTORS
 *===========================================================================*/

Mat2 Mat2_Scale(Q16_8 sx, Q16_8 sy) {
    return Mat2_Create(sx, 0, 0, sy);
}

Mat2 Mat2_Rotate(int angle) {
    Q16_8 c = Fixed_Cos(angle);
    Q16_8 s = Fixed_Sin(angle);

    /*
     * Rotation matrix:
     *   | cos  -sin |
     *   | sin   cos |
     */
    return Mat2_Create(c, -s, s, c);
}

/*=============================================================================
 * VEC2 ADDITIONAL OPERATIONS
 *===========================================================================*/

Q16_8 Vec2_Distance(Vec2 a, Vec2 b) {
    Vec2 diff = Vec2_Sub(a, b);
    return Vec2_Len(diff);
}

Vec2 Vec2_RotateAround(Vec2 point, Vec2 pivot, int angle) {
    /* Translate point so pivot is at origin */
    Vec2 offset = Vec2_Sub(point, pivot);

    /* Rotate around origin */
    Vec2 rotated = Vec2_Rotate(offset, angle);

    /* Translate back */
    return Vec2_Add(rotated, pivot);
}

Vec2 Vec2_Project(Vec2 v, Vec2 onto) {
    /*
     * Project v onto another vector.
     * Formula: (dot(v, onto) / dot(onto, onto)) * onto
     *
     * Returns the component of v that lies along onto.
     */
    if (Vec2_IsZero(onto)) {
        return Vec2_Zero();
    }

    Q16_8 dot_v_onto = Vec2_Dot(v, onto);
    Q16_8 dot_onto_onto = Vec2_LenSquared(onto);

    Q16_8 scalar = FixedDiv(dot_v_onto, dot_onto_onto);
    return Vec2_Scale(onto, scalar);
}

Vec2 Vec2_Reject(Vec2 v, Vec2 from) {
    /*
     * Reject v from another vector (component perpendicular to from).
     * Formula: v - project(v, from)
     *
     * Returns the component of v that is perpendicular to from.
     */
    Vec2 projected = Vec2_Project(v, from);
    return Vec2_Sub(v, projected);
}
