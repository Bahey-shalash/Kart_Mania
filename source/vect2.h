/*
 * vect2.h - Fixed-point 2D vector math for Nintendo DS
 *
 * =============================================================================
 * FIXED-POINT FORMAT: Q16.8
 * =============================================================================
 *
 * For a 1024×1024 world map we need:
 *   - Integer range: at least ±1024
 *   - Subpixel precision: smooth movement at low speeds
 *
 * Q16.8 (16 integer bits, 8 fractional bits) stored in int32_t gives us:
 *   - Integer range: ±32767 (plenty of headroom for 1024×1024)
 *   - Precision: 1/256 ≈ 0.004 pixels (smooth subpixel movement)
 *   - Arithmetic: fast integer ops, no FPU needed
 *
 * Comparison of formats considered:
 *   Q8.8   - range ±127      - too small for 1024×1024 map
 *   Q12.4  - range ±2047     - sufficient range but only 1/16 px precision
 *   Q16.8  - range ±32767    - good range + good precision  ← chosen
 *   Q20.12 - range ±524287   - overkill, wastes bits
 *
 * =============================================================================
 * ANGLE FORMAT: Binary Angle (0-511)
 * =============================================================================
 *
 * For rotation and steering we need:
 *   - Full circle representation
 *   - Efficient wrapping (no modulo)
 *   - Sufficient resolution for smooth turning
 *   - LUT-friendly for sin/cos
 *
 * 9-bit binary angle (0-511) gives us:
 *   - Resolution: 512 steps per revolution = 0.703° per step
 *   - Wrapping: angle & 511 (free, no division)
 *   - LUT size: 129 entries for quarter-wave = 258 bytes
 *   - No floating point anywhere
 *
 * Comparison of formats considered:
 *   Degrees (0-359)  - needs modulo 360, awkward for LUT
 *   256 steps        - 1.406° resolution, may feel choppy
 *   512 steps        - 0.703° resolution, smooth enough  ← chosen
 *   Float radians    - requires FPU (DS has none), slow conversion
 *
 * =============================================================================
 * TRIG IMPLEMENTATION: Quarter-Wave LUT
 * =============================================================================
 *
 * Two common approaches for sin/cos without FPU:
 *
 * 1. Polynomial approximation (Taylor/Chebyshev series)
 *    - Smaller code size (~50 bytes)
 *    - Multiple multiplications per call
 *    - Rounding errors can accumulate
 *
 * 2. Lookup table (LUT)
 *    - Quarter-wave table: 129 entries × 2 bytes = 258 bytes ROM
 *    - Single lookup + conditional negate
 *    - Deterministic: exact same result every call
 *    - Faster: O(1) lookup vs O(n) multiplies
 *
 * We chose LUT because:
 *    - 258 bytes is trivial on DS (4MB RAM, 32MB+ ROM)
 *    - Speed matters for per-frame physics
 *    - Determinism matters for consistent gameplay
 *    - Simpler to verify and debug
 *
 * =============================================================================
 * DESIGN PRINCIPLES
 * =============================================================================
 *
 * 1. No floating point - DS Lite has no FPU, all float ops are emulated
 * 2. Macros for core ops - FixedMul, FixedDiv inline for speed
 * 3. LUT for trig - 129-entry quarter-wave table, mirror for full circle
 * 4. Public struct members - no getters, direct access to Vec2.x, Vec2.y
 *
 */

#ifndef VECT2_H
#define VECT2_H

#include <stdbool.h>
#include <stdint.h>

/*=============================================================================
 * FIXED-POINT TYPE: Q16.8
 *===========================================================================*/

typedef int32_t Q16_8;

#define FIXED_SHIFT 8
#define FIXED_ONE (1 << FIXED_SHIFT)
#define FIXED_HALF (1 << (FIXED_SHIFT - 1))

/* Conversion macros */
#define IntToFixed(i) ((Q16_8)((i) << FIXED_SHIFT))
#define FixedToInt(f) ((int)((f) >> FIXED_SHIFT))
#define FixedMul(a, b) ((Q16_8)(((int64_t)(a) * (int64_t)(b)) >> FIXED_SHIFT))
#define FixedDiv(a, b) ((Q16_8)(((int64_t)(a) << FIXED_SHIFT) / (b)))
#define FixedAbs(a) ((a) < 0 ? -(a) : (a))

/*=============================================================================
 * ANGLE CONSTANTS (Binary angle, 0-511)
 *===========================================================================*/

#define ANGLE_FULL 512    /* 360° */
#define ANGLE_HALF 256    /* 180° */
#define ANGLE_QUARTER 128 /* 90°  */
#define ANGLE_MASK 511    /* for wrapping: angle & ANGLE_MASK */

/*=============================================================================
 * VEC2: 2D Vector (Q16.8)
 *===========================================================================*/

typedef struct {
    Q16_8 x;
    Q16_8 y;
} Vec2;

/* Constructors */
static inline Vec2 Vec2_Create(Q16_8 x, Q16_8 y) {
    return (Vec2){x, y};
}

static inline Vec2 Vec2_Zero(void) {
    return (Vec2){0, 0};
}

static inline Vec2 Vec2_FromInt(int x, int y) {
    return (Vec2){IntToFixed(x), IntToFixed(y)};
}

/* Basic operations */
static inline Vec2 Vec2_Add(Vec2 a, Vec2 b) {
    return Vec2_Create(a.x + b.x, a.y + b.y);
}

static inline Vec2 Vec2_Sub(Vec2 a, Vec2 b) {
    return Vec2_Create(a.x - b.x, a.y - b.y);
}

static inline Vec2 Vec2_Neg(Vec2 a) {
    return Vec2_Create(-a.x, -a.y);
}

static inline Vec2 Vec2_Scale(Vec2 a, Q16_8 s) {
    return Vec2_Create(FixedMul(a.x, s), FixedMul(a.y, s));
}

static inline Q16_8 Vec2_Dot(Vec2 a, Vec2 b) {
    return FixedMul(a.x, b.x) + FixedMul(a.y, b.y);
}

static inline Q16_8 Vec2_LenSquared(Vec2 a) {
    return Vec2_Dot(a, a);
}

static inline bool Vec2_IsZero(Vec2 a) {
    return (a.x == 0 && a.y == 0);
}

/*=============================================================================
 * MAT2: 2x2 Matrix (Q16.8)
 *
 * Layout:
 *   | m00  m01 |
 *   | m10  m11 |
 *===========================================================================*/

typedef struct {
    Q16_8 m00, m01;
    Q16_8 m10, m11;
} Mat2;

static inline Mat2 Mat2_Create(Q16_8 m00, Q16_8 m01, Q16_8 m10, Q16_8 m11) {
    return (Mat2){m00, m01, m10, m11};
}

static inline Mat2 Mat2_Identity(void) {
    return Mat2_Create(FIXED_ONE, 0, 0, FIXED_ONE);
}

static inline Vec2 Mat2_MulVec(Mat2 m, Vec2 v) {
    return Vec2_Create(FixedMul(m.m00, v.x) + FixedMul(m.m01, v.y),
                       FixedMul(m.m10, v.x) + FixedMul(m.m11, v.y));
}

static inline Mat2 Mat2_Mul(Mat2 a, Mat2 b) {
    return Mat2_Create(FixedMul(a.m00, b.m00) + FixedMul(a.m01, b.m10),
                       FixedMul(a.m00, b.m01) + FixedMul(a.m01, b.m11),
                       FixedMul(a.m10, b.m00) + FixedMul(a.m11, b.m10),
                       FixedMul(a.m10, b.m01) + FixedMul(a.m11, b.m11));
}

/*=============================================================================
 * VEC2: Additional Operations (inline)
 *===========================================================================*/

/* Distance squared - cheap, no sqrt, good for comparisons */
static inline Q16_8 Vec2_DistanceSquared(Vec2 a, Vec2 b) {
    Vec2 diff = Vec2_Sub(a, b);
    return Vec2_LenSquared(diff);
}

/* Perpendicular - CCW 90° rotation: (x, y) -> (-y, x) */
static inline Vec2 Vec2_Perp(Vec2 v) {
    return Vec2_Create(-v.y, v.x);
}

/* Perpendicular - CW 90° rotation: (x, y) -> (y, -x) */
static inline Vec2 Vec2_PerpCW(Vec2 v) {
    return Vec2_Create(v.y, -v.x);
}

/*
 * Reflect vector off surface with given normal.
 * Formula: v - 2 * dot(v, n) * n
 * Note: normal should be normalized for correct results.
 */
static inline Vec2 Vec2_Reflect(Vec2 v, Vec2 normal) {
    Q16_8 dot2 = FixedMul(Vec2_Dot(v, normal), IntToFixed(2));
    return Vec2_Sub(v, Vec2_Scale(normal, dot2));
}

/*=============================================================================
 * FUNCTION PROTOTYPES (implemented in vect2.c)
 *===========================================================================*/

/* Trig (LUT-based) */
Q16_8 Fixed_Sin(int angle);
Q16_8 Fixed_Cos(int angle);

/* Vec2 heavy operations */
Q16_8 Vec2_Len(Vec2 a);
Vec2 Vec2_Normalize(Vec2 a);
Vec2 Vec2_ClampLen(Vec2 v, Q16_8 maxLen);

/* Vec2 angle operations */
Vec2 Vec2_FromAngle(int angle);
int Vec2_ToAngle(Vec2 v);
Vec2 Vec2_Rotate(Vec2 v, int angle);

/* Mat2 constructors */
Mat2 Mat2_Scale(Q16_8 sx, Q16_8 sy);
Mat2 Mat2_Rotate(int angle);

/* Vec2 additional operations */
Q16_8 Vec2_Distance(Vec2 a, Vec2 b);
Vec2 Vec2_RotateAround(Vec2 point, Vec2 pivot, int angle);
Vec2 Vec2_Project(Vec2 v, Vec2 onto);
Vec2 Vec2_Reject(Vec2 v, Vec2 from);

#endif  // VECT2_H
