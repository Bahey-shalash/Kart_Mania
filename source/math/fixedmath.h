
/**
 * File: fixedmath.h
 * -----------------
 * Description: Fixed-point 2D vector math library for Nintendo DS. Provides Q16.8
 *              fixed-point arithmetic, 2D vectors, 2x2 matrices, and trigonometric
 *              functions using lookup tables. Designed for fast, deterministic math
 *              without floating-point operations.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 04.01.2026
 */

/*
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

#ifndef FIXED_MATH_H
#define FIXED_MATH_H

#include <stdbool.h>
#include <stdint.h>

/*=============================================================================
 * FIXED-POINT TYPE: Q16.8
 *===========================================================================*/

typedef int32_t Q16_8;

#define FIXED_SHIFT 8
#define FIXED_ONE (1 << FIXED_SHIFT)
#define FIXED_HALF (1 << (FIXED_SHIFT - 1))

/**
 * Conversion Macros
 * -----------------
 * IntToFixed(i)    - Convert integer to Q16.8 fixed-point
 * FixedToInt(f)    - Convert Q16.8 fixed-point to integer (truncates)
 * FixedMul(a, b)   - Multiply two Q16.8 values (uses 64-bit intermediate)
 * FixedDiv(a, b)   - Divide two Q16.8 values (uses 64-bit intermediate)
 * FixedAbs(a)      - Absolute value of Q16.8
 */
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

/**
 * Vec2 Constructors
 * -----------------
 */

/** Creates a 2D vector from Q16.8 fixed-point coordinates */
static inline Vec2 Vec2_Create(Q16_8 x, Q16_8 y) {
    return (Vec2){x, y};
}

/** Creates a zero vector (0, 0) */
static inline Vec2 Vec2_Zero(void) {
    return (Vec2){0, 0};
}

/** Creates a 2D vector from integer coordinates (converts to Q16.8) */
static inline Vec2 Vec2_FromInt(int x, int y) {
    return (Vec2){IntToFixed(x), IntToFixed(y)};
}

/**
 * Vec2 Basic Operations
 * ---------------------
 */

/** Vector addition: a + b */
static inline Vec2 Vec2_Add(Vec2 a, Vec2 b) {
    return Vec2_Create(a.x + b.x, a.y + b.y);
}

/** Vector subtraction: a - b */
static inline Vec2 Vec2_Sub(Vec2 a, Vec2 b) {
    return Vec2_Create(a.x - b.x, a.y - b.y);
}

/** Vector negation: -a */
static inline Vec2 Vec2_Neg(Vec2 a) {
    return Vec2_Create(-a.x, -a.y);
}

/** Vector scalar multiplication: a * s */
static inline Vec2 Vec2_Scale(Vec2 a, Q16_8 s) {
    return Vec2_Create(FixedMul(a.x, s), FixedMul(a.y, s));
}

/** Dot product: a · b (returns Q16.8) */
static inline Q16_8 Vec2_Dot(Vec2 a, Vec2 b) {
    return FixedMul(a.x, b.x) + FixedMul(a.y, b.y);
}

/** Squared length of vector (avoids expensive sqrt, good for comparisons) */
static inline Q16_8 Vec2_LenSquared(Vec2 a) {
    return Vec2_Dot(a, a);
}

/** Checks if vector is exactly zero */
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

/** Creates a 2x2 matrix from Q16.8 components */
static inline Mat2 Mat2_Create(Q16_8 m00, Q16_8 m01, Q16_8 m10, Q16_8 m11) {
    return (Mat2){m00, m01, m10, m11};
}

/** Creates an identity matrix (1 on diagonal, 0 elsewhere) */
static inline Mat2 Mat2_Identity(void) {
    return Mat2_Create(FIXED_ONE, 0, 0, FIXED_ONE);
}

/** Matrix-vector multiplication: M * v */
static inline Vec2 Mat2_MulVec(Mat2 m, Vec2 v) {
    return Vec2_Create(FixedMul(m.m00, v.x) + FixedMul(m.m01, v.y),
                       FixedMul(m.m10, v.x) + FixedMul(m.m11, v.y));
}

/** Matrix-matrix multiplication: A * B */
static inline Mat2 Mat2_Mul(Mat2 a, Mat2 b) {
    return Mat2_Create(FixedMul(a.m00, b.m00) + FixedMul(a.m01, b.m10),
                       FixedMul(a.m00, b.m01) + FixedMul(a.m01, b.m11),
                       FixedMul(a.m10, b.m00) + FixedMul(a.m11, b.m10),
                       FixedMul(a.m10, b.m01) + FixedMul(a.m11, b.m11));
}

/*=============================================================================
 * VEC2: Additional Operations (inline)
 *===========================================================================*/

/**
 * Function: Vec2_DistanceSquared
 * -------------------------------
 * Squared distance between two points (avoids expensive sqrt).
 * Good for distance comparisons without needing exact distance.
 *
 * Returns: Distance squared in Q16.8
 */
static inline Q16_8 Vec2_DistanceSquared(Vec2 a, Vec2 b) {
    Vec2 diff = Vec2_Sub(a, b);
    return Vec2_LenSquared(diff);
}

/**
 * Function: Vec2_Perp
 * -------------------
 * Counter-clockwise 90° rotation: (x, y) → (-y, x)
 */
static inline Vec2 Vec2_Perp(Vec2 v) {
    return Vec2_Create(-v.y, v.x);
}

/**
 * Function: Vec2_PerpCW
 * ---------------------
 * Clockwise 90° rotation: (x, y) → (y, -x)
 */
static inline Vec2 Vec2_PerpCW(Vec2 v) {
    return Vec2_Create(v.y, -v.x);
}

/**
 * Function: Vec2_Reflect
 * ----------------------
 * Reflects vector off surface with given normal.
 *
 * Formula: v - 2 * dot(v, n) * n
 *
 * Parameters:
 *   v      - Incident vector
 *   normal - Surface normal (should be normalized for correct results)
 *
 * Returns: Reflected vector
 */
static inline Vec2 Vec2_Reflect(Vec2 v, Vec2 normal) {
    Q16_8 dot2 = FixedMul(Vec2_Dot(v, normal), IntToFixed(2));
    return Vec2_Sub(v, Vec2_Scale(normal, dot2));
}

/*=============================================================================
 * FUNCTION PROTOTYPES (implemented in fixedmath.c)
 *===========================================================================*/

/**
 * Trigonometry Functions (LUT-based)
 * -----------------------------------
 * All trig functions use a 129-entry quarter-wave lookup table.
 * Angles are in binary format (0-511 = 0-360°).
 */

/** Sine function using quarter-wave LUT. Angle in binary (0-511) */
Q16_8 Fixed_Sin(int angle);

/** Cosine function using quarter-wave LUT. Angle in binary (0-511) */
Q16_8 Fixed_Cos(int angle);

/**
 * Vec2 Heavy Operations
 * ---------------------
 * These functions involve square roots or normalization (expensive operations).
 */

/** Length of vector (uses sqrt, expensive) */
Q16_8 Vec2_Len(Vec2 a);

/** Normalize vector to unit length (length = 1.0) */
Vec2 Vec2_Normalize(Vec2 a);

/** Clamp vector length to maxLen (preserves direction) */
Vec2 Vec2_ClampLen(Vec2 v, Q16_8 maxLen);

/**
 * Vec2 Angle Operations
 * ---------------------
 * Convert between vectors and binary angles (0-511).
 */

/** Create unit vector from binary angle (0-511) */
Vec2 Vec2_FromAngle(int angle);

/** Convert vector to binary angle (0-511) using atan2 */
int Vec2_ToAngle(Vec2 v);

/** Rotate vector by binary angle (0-511) */
Vec2 Vec2_Rotate(Vec2 v, int angle);

/**
 * Mat2 Constructors
 * -----------------
 */

/** Create scaling matrix with separate X/Y scale factors */
Mat2 Mat2_Scale(Q16_8 sx, Q16_8 sy);

/** Create rotation matrix from binary angle (0-511) */
Mat2 Mat2_Rotate(int angle);

/**
 * Vec2 Additional Operations
 * ---------------------------
 */

/** Distance between two points (uses sqrt, expensive) */
Q16_8 Vec2_Distance(Vec2 a, Vec2 b);

/** Rotate point around pivot by binary angle */
Vec2 Vec2_RotateAround(Vec2 point, Vec2 pivot, int angle);

/** Project vector v onto vector 'onto' */
Vec2 Vec2_Project(Vec2 v, Vec2 onto);

/** Reject vector v from vector 'from' (orthogonal component) */
Vec2 Vec2_Reject(Vec2 v, Vec2 from);

#endif  // FIXED_MATH_H
