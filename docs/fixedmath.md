# Fixed-Point Math Library

## Overview

The fixed-point math library provides fixed point arithmetics, 2D vector and matrix operations for the Nintendo DS without using floating-point arithmetic. It's designed for the ARM9 processor which lacks a hardware FPU, making integer-based math 10-100x faster than software-emulated floating point.

The implementation is in [fixedmath.c](../source/math/fixedmath.c) and [fixedmath.h](../source/math/fixedmath.h), used throughout the game for all physics, movement, and rendering calculations.

**Key Features:**
- Q16.8 fixed-point format for positions and velocities
- Binary angle system (0-511) for rotations
- Quarter-wave sine LUT for fast trigonometry
- Integer square root for vector length calculations
- 2D vector and 2x2 matrix operations
- All operations avoid floating point entirely

## Architecture

### Number Formats

The library uses two custom number formats optimized for the Nintendo DS:

#### Q16.8 Fixed-Point

**Format:** 16 integer bits, 8 fractional bits, stored in `int32_t`

**Properties:**
- Range: ±32767 (sufficient for 1024×1024 map)
- Precision: 1/256 ≈ 0.004 pixels (smooth subpixel movement)
- Representation: `FIXED_ONE = 256` represents 1.0

**Why Q16.8:**

For a 1024×1024 world map with smooth movement, we considered:

| Format | Integer Range | Precision | Verdict |
|--------|--------------|-----------|---------|
| Q8.8   | ±127         | 1/256 px  | ❌ Too small for map |
| Q12.4  | ±2047        | 1/16 px   | ⚠️ Range OK, choppy movement |
| **Q16.8** | **±32767** | **1/256 px** | **✅ Good range + smooth precision** |
| Q20.12 | ±524287      | 1/4096 px | ❌ Overkill, wastes bits |

**Decision:** Q16.8 provides ample headroom for the 1024×1024 map while maintaining subpixel precision for smooth kart movement at low speeds.

#### Binary Angles

**Format:** 9-bit integer (0-511) representing 0-360°

**Properties:**
- 512 steps per revolution = 0.703° per step
- Wrapping: `angle & ANGLE_MASK` (single AND instruction)
- LUT-friendly: quarter-wave table needs only 129 entries

**Why Binary Angles:**

For rotation and steering, we considered:

| Format | Steps | Resolution | Wrapping | Verdict |
|--------|-------|------------|----------|---------|
| Degrees (0-359) | 360 | 1° | Modulo 360 (slow) | ❌ Inefficient |
| 8-bit (0-255) | 256 | 1.406° | `& 0xFF` | ⚠️ May feel choppy |
| **9-bit (0-511)** | **512** | **0.703°** | **`& 0x1FF`** | **✅ Smooth + fast** |
| Float radians | ∞ | ∞ | `fmod()` | ❌ No FPU on DS |

**Decision:** 9-bit binary angles provide smooth rotation with free wrapping via bitwise AND, avoiding expensive modulo operations.

### Trigonometry Implementation

#### Quarter-Wave Lookup Table

**Approach:** 129-entry sine table covering 0° to 90° in Q16.8 format

**Why LUT over polynomial approximation:**

| Approach | ROM Cost | Speed | Determinism | Complexity |
|----------|----------|-------|-------------|------------|
| **Quarter-wave LUT** | **258 bytes** | **O(1)** | **Exact every time** | **Simple** |
| Taylor series | ~50 bytes | O(n) multiplies | Rounding drift | Medium |
| Chebyshev approx | ~100 bytes | O(n) multiplies | Better precision | Complex |
| CORDIC | ~200 bytes | O(16) iterations | Good | Complex |

**Decision:** LUT provides deterministic results (critical for gameplay consistency) with O(1) speed. 258 bytes is trivial on DS with 4MB+ ROM.

**Symmetry Reconstruction:**

The full sine wave is reconstructed using mathematical symmetry:

```
Quadrant 0 (0-127):   sin_lut[idx]           (direct lookup)
Quadrant 1 (128-255): sin_lut[128 - idx]     (mirror)
Quadrant 2 (256-383): -sin_lut[idx]          (negate)
Quadrant 3 (384-511): -sin_lut[128 - idx]    (mirror + negate)
```

Cosine is implemented as phase-shifted sine: `cos(x) = sin(x + 90°)`

### Design Principles

1. **No Floating Point** - DS Lite has no FPU, all float ops are 10-100x slower
2. **Macros for Core Ops** - FixedMul, FixedDiv inline for guaranteed zero overhead
3. **LUT for Trig** - 129-entry quarter-wave table, mirror for full circle
4. **Public Struct Members** - Direct access to Vec2.x, Vec2.y (no getters)
5. **Integer Square Root** - Bitwise algorithm avoiding floating point entirely

## Types and Constants

### Q16_8 Type

**Defined in:** [fixedmath.h:101](../source/math/fixedmath.h#L101)

```c
typedef int32_t Q16_8;
```

Fixed-point number with 16 integer bits and 8 fractional bits.

**Constants:**
- `FIXED_SHIFT = 8` - Number of fractional bits
- `FIXED_ONE = 256` - Represents 1.0 in Q16.8
- `FIXED_HALF = 128` - Represents 0.5 in Q16.8

### Vec2 Type

**Defined in:** [fixedmath.h:135-138](../source/math/fixedmath.h#L135-L138)

```c
typedef struct {
    Q16_8 x;
    Q16_8 y;
} Vec2;
```

2D vector with public Q16.8 members for direct access.

### Mat2 Type

**Defined in:** [fixedmath.h:208-211](../source/math/fixedmath.h#L208-L211)

```c
typedef struct {
    Q16_8 m00, m01;
    Q16_8 m10, m11;
} Mat2;
```

2×2 matrix in row-major layout:
```
| m00  m01 |
| m10  m11 |
```

### Angle Constants

**Defined in:** [fixedmath.h:126-129](../source/math/fixedmath.h#L126-L129)

- `ANGLE_FULL = 512` - Full rotation (360°)
- `ANGLE_HALF = 256` - Half rotation (180°)
- `ANGLE_QUARTER = 128` - Quarter rotation (90°)
- `ANGLE_MASK = 511` - For wrapping: `angle & ANGLE_MASK`

## Conversion Macros

All conversion macros are defined in [fixedmath.h:112-120](../source/math/fixedmath.h#L112-L120).

### IntToFixed

**Signature:** `Q16_8 IntToFixed(int i)`

Converts integer to Q16.8 fixed-point by shifting left 8 bits.

**Example:**
```c
Q16_8 five = IntToFixed(5);  // 5 << 8 = 1280 (represents 5.0)
```

### FixedToInt

**Signature:** `int FixedToInt(Q16_8 f)`

Converts Q16.8 to integer by shifting right 8 bits (truncates fractional part).

**Example:**
```c
int i = FixedToInt(1536);  // 1536 >> 8 = 6 (truncates 6.0 to 6)
```

### FixedMul

**Signature:** `Q16_8 FixedMul(Q16_8 a, Q16_8 b)`

Multiplies two Q16.8 values using 64-bit intermediate to prevent overflow.

**Implementation:**
```c
#define FixedMul(a, b) ((Q16_8)(((int64_t)(a) * (int64_t)(b)) >> FIXED_SHIFT))
```

**Why 64-bit intermediate:** Multiplying two 32-bit values can overflow. We multiply as 64-bit, then shift right to compensate for double scaling.

### FixedDiv

**Signature:** `Q16_8 FixedDiv(Q16_8 a, Q16_8 b)`

Divides two Q16.8 values using 64-bit intermediate for precision.

**Implementation:**
```c
#define FixedDiv(a, b) ((Q16_8)(((int64_t)(a) << FIXED_SHIFT) / (b)))
```

**Why shift before divide:** Pre-shifting the numerator compensates for the Q16.8 scaling in denominator.

### FixedAbs

**Signature:** `Q16_8 FixedAbs(Q16_8 a)`

Returns absolute value of a Q16.8 number.

**Implementation:**
```c
#define FixedAbs(a) ((a) < 0 ? -(a) : (a))
```

## Vec2 Inline Operations

All inline Vec2 functions are defined in [fixedmath.h](../source/math/fixedmath.h). They compile to just a few instructions with no function call overhead.

### Vec2 Constructors

#### Vec2_Create

**Signature:** `Vec2 Vec2_Create(Q16_8 x, Q16_8 y)`
**Defined in:** [fixedmath.h:146-148](../source/math/fixedmath.h#L146-L148)

Creates a 2D vector from Q16.8 coordinates.

**Example:**
```c
Vec2 v = Vec2_Create(IntToFixed(10), IntToFixed(20));  // (10.0, 20.0)
```

#### Vec2_Zero

**Signature:** `Vec2 Vec2_Zero(void)`
**Defined in:** [fixedmath.h:151-153](../source/math/fixedmath.h#L151-L153)

Creates a zero vector (0, 0).

#### Vec2_FromInt

**Signature:** `Vec2 Vec2_FromInt(int x, int y)`
**Defined in:** [fixedmath.h:156-158](../source/math/fixedmath.h#L156-L158)

Creates a vector from integer coordinates (automatically converts to Q16.8).

**Example:**
```c
Vec2 pos = Vec2_FromInt(100, 200);  // (100.0, 200.0)
```

### Vec2 Basic Operations

#### Vec2_Add

**Signature:** `Vec2 Vec2_Add(Vec2 a, Vec2 b)`
**Defined in:** [fixedmath.h:166-168](../source/math/fixedmath.h#L166-L168)

Vector addition: `a + b`

**Example:**
```c
pos = Vec2_Add(pos, velocity);  // pos += velocity
```

#### Vec2_Sub

**Signature:** `Vec2 Vec2_Sub(Vec2 a, Vec2 b)`
**Defined in:** [fixedmath.h:171-173](../source/math/fixedmath.h#L171-L173)

Vector subtraction: `a - b`

#### Vec2_Neg

**Signature:** `Vec2 Vec2_Neg(Vec2 a)`
**Defined in:** [fixedmath.h:176-178](../source/math/fixedmath.h#L176-L178)

Vector negation: `-a`

#### Vec2_Scale

**Signature:** `Vec2 Vec2_Scale(Vec2 a, Q16_8 s)`
**Defined in:** [fixedmath.h:181-183](../source/math/fixedmath.h#L181-L183)

Scalar multiplication: `a * s`

**Example:**
```c
Vec2 halfSpeed = Vec2_Scale(velocity, FIXED_HALF);  // velocity * 0.5
```

#### Vec2_Dot

**Signature:** `Q16_8 Vec2_Dot(Vec2 a, Vec2 b)`
**Defined in:** [fixedmath.h:186-188](../source/math/fixedmath.h#L186-L188)

Dot product: `a · b`

Returns Q16.8 scalar value.

#### Vec2_LenSquared

**Signature:** `Q16_8 Vec2_LenSquared(Vec2 a)`
**Defined in:** [fixedmath.h:191-193](../source/math/fixedmath.h#L191-L193)

Squared length of vector (avoids expensive sqrt).

**Use case:** Distance comparisons where exact distance isn't needed.

**Example:**
```c
Q16_8 distSq = Vec2_LenSquared(Vec2_Sub(a, b));
if (distSq < IntToFixed(100)) {  // Within 10 pixels (10^2 = 100)
    // ...
}
```

#### Vec2_IsZero

**Signature:** `bool Vec2_IsZero(Vec2 a)`
**Defined in:** [fixedmath.h:196-198](../source/math/fixedmath.h#L196-L198)

Checks if vector is exactly zero.

### Vec2 Additional Inline Operations

#### Vec2_DistanceSquared

**Signature:** `Q16_8 Vec2_DistanceSquared(Vec2 a, Vec2 b)`
**Defined in:** [fixedmath.h:249-252](../source/math/fixedmath.h#L249-L252)

Squared distance between two points (avoids expensive sqrt).

**Implementation:**
```c
Vec2 diff = Vec2_Sub(a, b);
return Vec2_LenSquared(diff);
```

#### Vec2_Perp

**Signature:** `Vec2 Vec2_Perp(Vec2 v)`
**Defined in:** [fixedmath.h:259-261](../source/math/fixedmath.h#L259-L261)

Counter-clockwise 90° rotation: `(x, y) → (-y, x)`

**Use case:** Getting perpendicular vector for wall sliding.

#### Vec2_PerpCW

**Signature:** `Vec2 Vec2_PerpCW(Vec2 v)`
**Defined in:** [fixedmath.h:268-270](../source/math/fixedmath.h#L268-L270)

Clockwise 90° rotation: `(x, y) → (y, -x)`

#### Vec2_Reflect

**Signature:** `Vec2 Vec2_Reflect(Vec2 v, Vec2 normal)`
**Defined in:** [fixedmath.h:285-288](../source/math/fixedmath.h#L285-L288)

Reflects vector off surface with given normal.

**Formula:** `v - 2 * dot(v, n) * n`

**Use case:** Bouncing off walls.

**Example:**
```c
Vec2 wallNormal = Vec2_Normalize(&wallDir);
Vec2 bounced = Vec2_Reflect(velocity, wallNormal);
```

## Mat2 Inline Operations

All inline Mat2 functions are defined in [fixedmath.h](../source/math/fixedmath.h).

### Mat2_Create

**Signature:** `Mat2 Mat2_Create(Q16_8 m00, Q16_8 m01, Q16_8 m10, Q16_8 m11)`
**Defined in:** [fixedmath.h:214-216](../source/math/fixedmath.h#L214-L216)

Creates a 2×2 matrix from Q16.8 components in row-major order.

### Mat2_Identity

**Signature:** `Mat2 Mat2_Identity(void)`
**Defined in:** [fixedmath.h:219-221](../source/math/fixedmath.h#L219-L221)

Creates an identity matrix:
```
| 1  0 |
| 0  1 |
```

### Mat2_MulVec

**Signature:** `Vec2 Mat2_MulVec(Mat2 m, Vec2 v)`
**Defined in:** [fixedmath.h:224-227](../source/math/fixedmath.h#L224-L227)

Matrix-vector multiplication: `M * v`

**Example:**
```c
Mat2 rot = Mat2_Rotate(angle);
Vec2 rotated = Mat2_MulVec(rot, original);
```

### Mat2_Mul

**Signature:** `Mat2 Mat2_Mul(Mat2 a, Mat2 b)`
**Defined in:** [fixedmath.h:230-235](../source/math/fixedmath.h#L230-L235)

Matrix-matrix multiplication: `A * B`

**Example:**
```c
Mat2 rot = Mat2_Rotate(facing);
Mat2 scale = Mat2_Scale(IntToFixed(2), IntToFixed(2));
Mat2 transform = Mat2_Mul(rot, scale);  // Rotate then scale
```

## Trigonometry Functions

### Fixed_Sin

**Signature:** `Q16_8 Fixed_Sin(int angle)`
**Defined in:** [fixedmath.c:73-98](../source/math/fixedmath.c#L73-L98)

Computes sine using quarter-wave lookup table with symmetry.

**Parameters:**
- `angle` - Binary angle (0-511 representing 0-360°)

**Returns:** Sine value in Q16.8 format (-256 to 256, representing -1.0 to 1.0)

**Implementation:**

1. **Wrap angle** to 0-511 using `ANGLE_MASK`
2. **Determine quadrant** (0-3) via `angle >> 7`
3. **Get index** within quadrant via `angle & (ANGLE_QUARTER - 1)`
4. **Mirror for odd quadrants** (1, 3): use `sin_lut[128 - idx]`
5. **Negate for quadrants 2, 3**: flip sign

**Example:**
```c
Q16_8 s = Fixed_Sin(0);          // sin(0°)   = 0
Q16_8 s = Fixed_Sin(128);        // sin(90°)  = 256 (1.0)
Q16_8 s = Fixed_Sin(256);        // sin(180°) = 0
Q16_8 s = Fixed_Sin(384);        // sin(270°) = -256 (-1.0)
```

### Fixed_Cos

**Signature:** `Q16_8 Fixed_Cos(int angle)`
**Defined in:** [fixedmath.c:110-113](../source/math/fixedmath.c#L110-L113)

Computes cosine using phase-shifted sine: `cos(x) = sin(x + 90°)`

**Parameters:**
- `angle` - Binary angle (0-511 representing 0-360°)

**Returns:** Cosine value in Q16.8 format (-256 to 256, representing -1.0 to 1.0)

**Implementation:**
```c
return Fixed_Sin(angle + ANGLE_QUARTER);
```

## Vec2 Heavy Operations

These functions are implemented in [fixedmath.c](../source/math/fixedmath.c) because they involve expensive operations (square root, division, normalization).

### Vec2_Len

**Signature:** `Q16_8 Vec2_Len(const Vec2* a)`
**Defined in:** [fixedmath.c:178-193](../source/math/fixedmath.c#L178-L193)

Computes length (magnitude) of a vector using integer square root.

**Returns:** Length in Q16.8 format

**Implementation:**

1. Compute `len² = Vec2_LenSquared(*a)` (cheap: just dot product)
2. Shift to Q24.16 for proper sqrt scaling: `len2_shifted = len2 << FIXED_SHIFT`
3. Use integer sqrt: `sqrt_result = isqrt(len2_shifted)` → already Q16.8
4. Return as Q16.8

**Why expensive:** Uses integer square root (bitwise algorithm with ~30 iterations).

**Example:**
```c
Q16_8 speed = Vec2_Len(&velocity);
```

### Vec2_Normalize

**Signature:** `Vec2 Vec2_Normalize(const Vec2* a)`
**Defined in:** [fixedmath.c:207-218](../source/math/fixedmath.c#L207-L218)

Normalizes vector to unit length (length = 1.0 in Q16.8 = 256).

**Returns:** Normalized vector, or zero vector if input is zero

**Implementation:**

1. Check if zero vector → return zero
2. Compute length via `Vec2_Len(a)`
3. Divide each component: `Vec2_Create(FixedDiv(a->x, len), FixedDiv(a->y, len))`

**Why expensive:** Uses both Vec2_Len (sqrt) and two divisions.

**Example:**
```c
Vec2 direction = Vec2_Normalize(&velocity);
```

### Vec2_ClampLen

**Signature:** `Vec2 Vec2_ClampLen(const Vec2* v, Q16_8 maxLen)`
**Defined in:** [fixedmath.c:233-248](../source/math/fixedmath.c#L233-L248)

Clamps vector length to maximum value, preserving direction.

**Parameters:**
- `v` - Input vector
- `maxLen` - Maximum length in Q16.8 format

**Returns:** Vector with same direction but clamped length

**Optimization:** Compares len² to avoid sqrt if length already within bounds

**Implementation:**

1. Compute `len² = Vec2_LenSquared(*v)`
2. Compute `max² = FixedMul(maxLen, maxLen)`
3. If `len² <= max²`: return original (no clamp needed)
4. Otherwise: normalize and scale to maxLen

**Example:**
```c
Q16_8 maxSpeed = IntToFixed(5);  // 5 pixels/frame
velocity = Vec2_ClampLen(&velocity, maxSpeed);
```

## Vec2 Angle Operations

### Vec2_FromAngle

**Signature:** `Vec2 Vec2_FromAngle(int angle)`
**Defined in:** [fixedmath.c:264-266](../source/math/fixedmath.c#L264-L266)

Creates a unit vector pointing in the given direction.

**Parameters:**
- `angle` - Binary angle (0-511 representing 0-360°)

**Returns:** Unit vector with `x = cos(angle)`, `y = sin(angle)`

**Example:**
```c
int facing = 64;  // 45°
Vec2 direction = Vec2_FromAngle(facing);  // (~0.707, ~0.707)
```

### Vec2_ToAngle

**Signature:** `int Vec2_ToAngle(const Vec2* v)`
**Defined in:** [fixedmath.c:288-333](../source/math/fixedmath.c#L288-L333)

Converts a vector to its direction angle using atan2 approximation.

**Parameters:**
- `v` - Input vector

**Returns:** Binary angle (0-511 representing 0-360°)

**Implementation:**

Instead of using `atan2()` (no FPU!), we use binary search on the sin LUT:

1. Compute `sin(angle) = |y| / length` (absolute value for first-quadrant lookup)
2. Binary search sin_lut[0-128] to find matching angle in first quadrant
3. Adjust for actual quadrant based on signs of x and y:
   - Quadrant 1 (x≥0, y≥0): 0-128 (as-is)
   - Quadrant 2 (x<0, y≥0): 128-256 (mirror: `angle = 256 - angle`)
   - Quadrant 3 (x<0, y<0): 256-384 (negate and shift: `angle = 256 + angle`)
   - Quadrant 4 (x≥0, y<0): 384-512 (reflect: `angle = 512 - angle`)

**Example:**
```c
Vec2 dir = Vec2_Create(IntToFixed(1), IntToFixed(1));
int angle = Vec2_ToAngle(&dir);  // ~64 (45°)
```

### Vec2_Rotate

**Signature:** `Vec2 Vec2_Rotate(const Vec2* v, int angle)`
**Defined in:** [fixedmath.c:350-356](../source/math/fixedmath.c#L350-L356)

Rotates a vector by a given angle using rotation matrix.

**Parameters:**
- `v` - Input vector
- `angle` - Binary angle (0-511 representing 0-360°)

**Returns:** Rotated vector

**Implementation:**

Uses standard 2D rotation matrix:
```
| cos  -sin | * | x |
| sin   cos |   | y |
```

```c
Q16_8 c = Fixed_Cos(angle);
Q16_8 s = Fixed_Sin(angle);
return Vec2_Create(FixedMul(v->x, c) - FixedMul(v->y, s),
                   FixedMul(v->x, s) + FixedMul(v->y, c));
```

**Example:**
```c
Vec2 rotated = Vec2_Rotate(&velocity, 128);  // Rotate 90°
```

## Mat2 Constructors

### Mat2_Scale

**Signature:** `Mat2 Mat2_Scale(Q16_8 sx, Q16_8 sy)`
**Defined in:** [fixedmath.c:374-376](../source/math/fixedmath.c#L374-L376)

Creates a scaling matrix with separate X and Y scale factors.

**Parameters:**
- `sx` - X scale factor (Q16.8)
- `sy` - Y scale factor (Q16.8)

**Returns:** Scaling matrix:
```
| sx  0  |
|  0  sy |
```

**Example:**
```c
Mat2 scale = Mat2_Scale(IntToFixed(2), IntToFixed(3));  // 2x horizontal, 3x vertical
```

### Mat2_Rotate

**Signature:** `Mat2 Mat2_Rotate(int angle)`
**Defined in:** [fixedmath.c:389-399](../source/math/fixedmath.c#L389-L399)

Creates a rotation matrix from binary angle.

**Parameters:**
- `angle` - Binary angle (0-511 representing 0-360°)

**Returns:** Rotation matrix:
```
| cos  -sin |
| sin   cos |
```

**Implementation:**
```c
Q16_8 c = Fixed_Cos(angle);
Q16_8 s = Fixed_Sin(angle);
return Mat2_Create(c, -s, s, c);
```

**Example:**
```c
Mat2 rot = Mat2_Rotate(128);  // 90° rotation matrix
```

## Vec2 Additional Operations

### Vec2_Distance

**Signature:** `Q16_8 Vec2_Distance(const Vec2* a, const Vec2* b)`
**Defined in:** [fixedmath.c:418-421](../source/math/fixedmath.c#L418-L421)

Computes Euclidean distance between two points.

**Parameters:**
- `a` - First point
- `b` - Second point

**Returns:** Distance in Q16.8 format

**Note:** Expensive (uses sqrt). Use `Vec2_DistanceSquared()` for comparisons.

**Implementation:**
```c
Vec2 diff = Vec2_Sub(*a, *b);
return Vec2_Len(&diff);
```

### Vec2_RotateAround

**Signature:** `Vec2 Vec2_RotateAround(const Vec2* point, const Vec2* pivot, int angle)`
**Defined in:** [fixedmath.c:440-449](../source/math/fixedmath.c#L440-L449)

Rotates a point around a pivot by given angle.

**Parameters:**
- `point` - Point to rotate
- `pivot` - Center of rotation
- `angle` - Binary angle (0-511 representing 0-360°)

**Returns:** Rotated point

**Implementation:**

1. Translate point so pivot is at origin: `offset = point - pivot`
2. Rotate around origin: `rotated = Vec2_Rotate(&offset, angle)`
3. Translate back: `return rotated + pivot`

**Example:**
```c
// Rotate kart's corner around its center
Vec2 corner = Vec2_FromInt(16, 8);
Vec2 worldCorner = Vec2_RotateAround(&corner, &kart.position, kart.facing);
```

### Vec2_Project

**Signature:** `Vec2 Vec2_Project(const Vec2* v, const Vec2* onto)`
**Defined in:** [fixedmath.c:464-474](../source/math/fixedmath.c#L464-L474)

Projects vector v onto another vector.

**Parameters:**
- `v` - Vector to project
- `onto` - Vector to project onto

**Returns:** Component of v that lies along 'onto'

**Formula:** `(dot(v, onto) / dot(onto, onto)) * onto`

**Example:**
```c
// Get velocity component along slope
Vec2 slopeDir = Vec2_Normalize(&slope);
Vec2 alongSlope = Vec2_Project(&velocity, &slopeDir);
```

### Vec2_Reject

**Signature:** `Vec2 Vec2_Reject(const Vec2* v, const Vec2* from)`
**Defined in:** [fixedmath.c:489-492](../source/math/fixedmath.c#L489-L492)

Computes rejection of v from another vector (perpendicular component).

**Parameters:**
- `v` - Vector to reject
- `from` - Vector to reject from

**Returns:** Component of v perpendicular to 'from'

**Formula:** `v - project(v, from)`

**Example:**
```c
// Get velocity component perpendicular to slope
Vec2 slopeDir = Vec2_Normalize(&slope);
Vec2 intoSlope = Vec2_Reject(&velocity, &slopeDir);
```

## Integer Square Root

### isqrt (private)

**Signature:** `static uint32_t isqrt(uint64_t n)`
**Defined in:** [fixedmath.c:135-156](../source/math/fixedmath.c#L135-L156)

Integer square root using classic bitwise algorithm. Private function used by `Vec2_Len()`.

**Parameters:**
- `n` - Input value (64-bit unsigned)

**Returns:** `floor(sqrt(n))` as 32-bit unsigned integer

**Algorithm:**

1. Start with highest power of 4 ≤ 2^64: `bit = 1ull << 62`
2. Find highest bit by shifting right until `bit <= n`
3. Compute square root bit by bit:
   - Test if adding current bit makes result too large
   - If not, subtract from n and update result
   - Shift right and repeat
4. Return final result

**Why needed:** No floating-point sqrt available on Nintendo DS.

## Usage Examples

### Basic Vector Math

```c
// Create vectors
Vec2 pos = Vec2_FromInt(100, 200);         // (100.0, 200.0)
Vec2 vel = Vec2_Create(128, 64);           // (0.5, 0.25) in Q16.8

// Update position
pos = Vec2_Add(pos, vel);                  // pos += vel

// Scale velocity
vel = Vec2_Scale(vel, FIXED_HALF);         // vel *= 0.5
```

### Angle and Rotation

```c
// Steering
int facing = 0;                            // 0° (facing right)
facing = (facing + 8) & ANGLE_MASK;        // Turn left ~5.6°

// Get direction vector
Vec2 dir = Vec2_FromAngle(facing);

// Rotate velocity
Vec2 rotated = Vec2_Rotate(&vel, facing);

// Get angle from vector
int angle = Vec2_ToAngle(&velocity);
```

### Speed Limiting

```c
// Clamp velocity to max speed
Q16_8 maxSpeed = IntToFixed(5);            // 5 pixels/frame
velocity = Vec2_ClampLen(&velocity, maxSpeed);

// Check if moving fast enough
Q16_8 minSpeedSq = FixedMul(IntToFixed(1), IntToFixed(1));
if (Vec2_LenSquared(velocity) < minSpeedSq) {
    // Too slow, apply boost
}
```

### Matrix Transforms

```c
// Combined rotation and scale
Mat2 rot = Mat2_Rotate(facing);
Mat2 scale = Mat2_Scale(IntToFixed(2), IntToFixed(2));
Mat2 transform = Mat2_Mul(rot, scale);

// Apply to vector
Vec2 transformed = Mat2_MulVec(transform, original);
```

### Wall Collision

```c
// Get wall normal
Vec2 wallNormal = Vec2_Normalize(&wallDir);

// Slide along wall (perpendicular component)
Vec2 slideDir = Vec2_Perp(wallNormal);
Vec2 slideVel = Vec2_Project(&velocity, &slideDir);

// Bounce off wall
Vec2 bounced = Vec2_Reflect(velocity, wallNormal);
```

### Slope Movement

```c
// Split velocity into components
Vec2 slopeDir = Vec2_Normalize(&slope);
Vec2 alongSlope = Vec2_Project(&velocity, &slopeDir);    // Sliding component
Vec2 intoSlope = Vec2_Reject(&velocity, &slopeDir);      // Penetration component

// Apply gravity along slope
Vec2 gravity = Vec2_Scale(slopeDir, IntToFixed(1));
alongSlope = Vec2_Add(alongSlope, gravity);
```

### Distance Checks

```c
// Cheap distance comparison (no sqrt)
Q16_8 distSq = Vec2_DistanceSquared(player.pos, enemy.pos);
Q16_8 rangeSq = FixedMul(IntToFixed(100), IntToFixed(100));
if (distSq < rangeSq) {
    // Within 100 pixels
}

// Actual distance when needed
Q16_8 dist = Vec2_Distance(&player.pos, &checkpoint.pos);
if (dist < IntToFixed(50)) {
    // Checkpoint reached
}
```

### Rotating Around Pivot

```c
// Rotate kart's corner points around center
Vec2 corners[4] = {
    Vec2_FromInt(-16, -8),   // Top-left
    Vec2_FromInt(16, -8),    // Top-right
    Vec2_FromInt(16, 8),     // Bottom-right
    Vec2_FromInt(-16, 8)     // Bottom-left
};

for (int i = 0; i < 4; i++) {
    Vec2 worldPos = Vec2_RotateAround(&corners[i], &kart.position, kart.facing);
    // Use worldPos for collision detection
}
```

## Performance Notes

### Inline Operations (Fast)

Operations defined in header with `static inline`:
- **Vec2 constructors** - Create, Zero, FromInt
- **Vec2 arithmetic** - Add, Sub, Neg, Scale, Dot
- **Vec2 comparisons** - IsZero, LenSquared, DistanceSquared
- **Vec2 perpendicular** - Perp, PerpCW, Reflect
- **Mat2 operations** - Create, Identity, MulVec, Mul

These compile to just a few instructions with zero function call overhead.

### LUT Operations (O(1))

- **Fixed_Sin()** - Single array lookup + maybe one negation
- **Fixed_Cos()** - Calls Fixed_Sin with shifted angle
- **Vec2_FromAngle()** - Two LUT lookups (sin + cos)

### Heavy Operations (Expensive)

- **Vec2_Len()** - Integer sqrt (~30 iterations)
- **Vec2_Normalize()** - Sqrt + 2 divisions
- **Vec2_ClampLen()** - Conditional sqrt + normalization
- **Vec2_ToAngle()** - Sqrt + binary search + division
- **Vec2_Distance()** - Sqrt via Vec2_Len()

**Optimization tip:** Use squared length (`Vec2_LenSquared`, `Vec2_DistanceSquared`) for comparisons to avoid expensive sqrt.

### Memory

- **No allocations** - All operations work on stack values
- **No floats** - Everything is integer arithmetic
- **Small footprint** - 258 bytes for sin LUT, minimal code size

## Design Rationale Summary

### Why Macros for FixedMul/FixedDiv?

**Pros:**
- Guaranteed inlining (no function call overhead)
- Type flexibility (works with any integer type)
- Compiler optimization (constant folding, dead code elimination)

**Cons:**
- No type safety
- Harder to debug (macro expansion in errors)

**Decision:** Performance matters more than type safety for core arithmetic. Use `static inline` for complex operations where type safety helps.

### Why Public Struct Members?

**Alternative:** Getters/setters like `Vec2_GetX(v)` and `Vec2_SetX(&v, x)`

**Pros of public members:**
- Simplicity: `v.x` is clearer than `Vec2_GetX(v)`
- Performance: No function call overhead
- Transparency: Nothing to hide in a 2D vector

**Decision:** Direct access is idiomatic for low-level math libraries.

### Why Integer Square Root?

**Alternative:** Use floating-point sqrt via `sqrtf()` with conversions

**Problem:** DS has no FPU, so `sqrtf()` is software-emulated and 10-100x slower

**Solution:** Bitwise integer sqrt algorithm
- Works entirely with integer operations
- Deterministic (no floating-point rounding)
- Reasonably fast (~30 iterations for 64-bit input)

**Decision:** Accepting ~30 iterations is better than software FPU emulation.

## Cross-References

- **Physics System:** gameplay/physics.c - Uses Vec2 for positions, velocities, accelerations
- **Kart Movement:** gameplay/kart.c - Uses angles for steering, Vec2 for movement
- **Collision Detection:** gameplay/collision.c - Uses Vec2_Distance, Vec2_Project for collision response
- **Rendering:** graphics/sprites.c - Converts Q16.8 positions to screen coordinates
- **Main Loop:** [main.md](main.md) - 60Hz VBlank synchronization for physics updates
- **Timer System:** [timer.md](timer.md) - TIMER0 drives physics at 60Hz using fixed-point math



---

## Navigation

- [← Back to Wiki](wiki.md)
- [← Back to README](../README.md)
