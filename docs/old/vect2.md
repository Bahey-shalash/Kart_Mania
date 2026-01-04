# vect2 - Fixed-Point 2D Vector Math for Nintendo DS

## Overview

`vect2` is a fixed-point 2D math library designed for Nintendo DS homebrew development. It provides efficient vector and matrix operations without using floating-point arithmetic, which is critical for the DS Lite's ARM9 processor that lacks a hardware FPU.

---

## Design Choices

### 1. Fixed-Point Format: Q16.8

We needed a number format that could:
- Represent coordinates in a 1024×1024 world map
- Provide subpixel precision for smooth movement
- Use fast integer arithmetic

#### Formats Considered

| Format | Integer Bits | Fractional Bits | Range | Precision | Verdict |
|--------|-------------|-----------------|-------|-----------|---------|
| Q8.8 | 8 | 8 | ±127 | 1/256 px | ❌ Too small for 1024×1024 map |
| Q12.4 | 12 | 4 | ±2047 | 1/16 px | ⚠️ Range OK, but precision may feel choppy |
| **Q16.8** | 16 | 8 | ±32767 | 1/256 px | ✅ Good range + precision |
| Q20.12 | 20 | 12 | ±524287 | 1/4096 px | ❌ Overkill, wastes bits |

#### Decision: Q16.8

- **Range**: ±32767 covers the 1024×1024 map with plenty of headroom
- **Precision**: 1/256 ≈ 0.004 pixels allows smooth subpixel movement
- **Storage**: Fits in `int32_t`, efficient on 32-bit ARM9
- **Arithmetic**: Simple shift-based multiply/divide

---

### 2. Angle Format: Binary Angle (0-511)

We needed an angle representation that:
- Covers a full 360° rotation
- Wraps efficiently (no expensive modulo)
- Works well with a sin/cos lookup table

#### Formats Considered

| Format | Steps | Resolution | Wrapping | LUT Size | Verdict |
|--------|-------|------------|----------|----------|---------|
| Degrees (0-359) | 360 | 1° | Modulo 360 (slow) | Awkward | ❌ Inefficient wrapping |
| 8-bit (0-255) | 256 | 1.406° | `& 0xFF` | 65 entries | ⚠️ May feel choppy |
| **9-bit (0-511)** | 512 | 0.703° | `& 0x1FF` | 129 entries | ✅ Smooth + efficient |
| Float radians | ∞ | ∞ | `fmod()` | N/A | ❌ No FPU on DS |

#### Decision: 9-bit Binary Angle (0-511)

- **Resolution**: 512 steps = 0.703° per step, smooth enough for steering
- **Wrapping**: `angle & 511` is a single AND instruction (free)
- **LUT-friendly**: Quarter-wave table needs only 129 entries
- **No floats**: Avoids slow software FPU emulation

---

### 3. Trigonometry: Quarter-Wave LUT vs Polynomial

We needed sin/cos functions without floating point.

#### Approaches Considered

| Approach | ROM Cost | Speed | Determinism | Complexity |
|----------|----------|-------|-------------|------------|
| **Quarter-wave LUT** | 258 bytes | O(1) lookup | Exact every time | Simple |
| Taylor series | ~50 bytes | O(n) multiplies | Rounding drift | Medium |
| Chebyshev approx | ~100 bytes | O(n) multiplies | Better precision | Complex |
| CORDIC | ~200 bytes | O(16) iterations | Good | Complex |

#### Decision: Quarter-Wave LUT

- **Size**: 129 entries × 2 bytes = 258 bytes (trivial on DS with 4MB+ ROM)
- **Speed**: Single array lookup + maybe one negation
- **Determinism**: Exact same result every call (important for gameplay)
- **Simplicity**: Easy to verify, debug, and maintain

#### How the LUT Works

We store sin values from 0° to 90° (indices 0-128). The full sine wave is reconstructed using symmetry:

```
Quadrant 0 (0-127):   sin[idx]        (direct lookup)
Quadrant 1 (128-255): sin[128 - idx]  (mirror)
Quadrant 2 (256-383): -sin[idx]       (negate)
Quadrant 3 (384-511): -sin[128 - idx] (mirror + negate)
```

Cosine is just sine phase-shifted by 90°: `cos(x) = sin(x + 128)`

---

### 4. No Floating Point Anywhere

The Nintendo DS Lite uses an ARM9 processor without a hardware floating-point unit (FPU). All float operations are emulated in software, making them 10-100x slower than integer operations.

#### What This Means

- Positions: `Q16_8` (fixed-point)
- Velocities: `Q16_8` (fixed-point)
- Angles: `int` (binary angle 0-511)
- Trig: LUT-based (returns `Q16_8`)
- No `float` or `double` anywhere in the hot path

---

### 5. Macros vs Inline Functions

We use macros for core arithmetic operations:

```c
#define FixedMul(a, b)  ((Q16_8)(((int64_t)(a) * (int64_t)(b)) >> FIXED_SHIFT))
#define FixedDiv(a, b)  ((Q16_8)(((int64_t)(a) << FIXED_SHIFT) / (b)))
```

#### Why Macros?

- **Guaranteed inlining**: No function call overhead
- **Type flexibility**: Works with any integer type
- **Compiler optimization**: Constant folding, dead code elimination
- **Tradition**: Follows the pattern from existing code  (fractal.h example see PW3B--Fixed_point)

#### Why Not `static inline`?

We use `static inline` for compound operations (Vec2_Add, etc.) where:
- The operation is more complex
- Type safety matters
- Debuggability is important

---

### 6. Public Struct Members

We expose `Vec2.x` and `Vec2.y` directly instead of using getter/setter functions.

#### Rationale

- **Simplicity**: `v.x` is clearer than `Vec2_GetX(v)`
- **Performance**: No function call overhead
- **Transparency**: Nothing to hide in a 2D vector

---

## API Reference

### Types

| Type | Description |
|------|-------------|
| `Q16_8` | Fixed-point number (16.8 format), stored as `int32_t` |
| `Vec2` | 2D vector with `Q16_8 x, y` |
| `Mat2` | 2×2 matrix with `Q16_8 m00, m01, m10, m11` |

### Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `FIXED_SHIFT` | 8 | Fractional bits in Q16.8 |
| `FIXED_ONE` | 256 | 1.0 in Q16.8 |
| `FIXED_HALF` | 128 | 0.5 in Q16.8 |
| `ANGLE_FULL` | 512 | Full rotation (360°) |
| `ANGLE_HALF` | 256 | Half rotation (180°) |
| `ANGLE_QUARTER` | 128 | Quarter rotation (90°) |
| `ANGLE_MASK` | 511 | For wrapping: `angle & ANGLE_MASK` |

### Conversion Macros

| Macro | Description |
|-------|-------------|
| `IntToFixed(i)` | Integer → Q16.8 |
| `FixedToInt(f)` | Q16.8 → Integer (truncates toward zero) |
| `FixedMul(a, b)` | Q16.8 × Q16.8 → Q16.8 |
| `FixedDiv(a, b)` | Q16.8 ÷ Q16.8 → Q16.8 |
| `FixedAbs(a)` | Absolute value |

### Vec2 Functions (Inline)

| Function | Description |
|----------|-------------|
| `Vec2_Create(x, y)` | Create vector from Q16.8 values |
| `Vec2_Zero()` | Create zero vector |
| `Vec2_FromInt(x, y)` | Create vector from integers |
| `Vec2_Add(a, b)` | Vector addition |
| `Vec2_Sub(a, b)` | Vector subtraction |
| `Vec2_Neg(a)` | Negate vector |
| `Vec2_Scale(a, s)` | Scale by Q16.8 scalar |
| `Vec2_Dot(a, b)` | Dot product |
| `Vec2_LenSquared(a)` | Length squared (cheap) |
| `Vec2_IsZero(a)` | Check if zero vector |
| `Vec2_DistanceSquared(a, b)` | Squared distance between points (cheap) |
| `Vec2_Perp(v)` | CCW perpendicular `(-y, x)` |
| `Vec2_PerpCW(v)` | CW perpendicular `(y, -x)` |
| `Vec2_Reflect(v, normal)` | Reflect vector off surface |

### Vec2 Functions (in vect2.c)

| Function | Description |
|----------|-------------|
| `Vec2_Len(a)` | Vector length (uses integer sqrt) |
| `Vec2_Normalize(a)` | Unit vector (or zero if input is zero) |
| `Vec2_ClampLen(v, max)` | Clamp magnitude to max |
| `Vec2_FromAngle(angle)` | Unit vector from binary angle |
| `Vec2_ToAngle(v)` | Binary angle from vector |
| `Vec2_Rotate(v, angle)` | Rotate vector by binary angle |
| `Vec2_Distance(a, b)` | Distance between points (uses sqrt) |
| `Vec2_RotateAround(point, pivot, angle)` | Rotate point around pivot |
| `Vec2_Project(v, onto)` | Project v onto another vector |
| `Vec2_Reject(v, from)` | Component of v perpendicular to from |

### Mat2 Functions

| Function | Description |
|----------|-------------|
| `Mat2_Create(...)` | Create matrix from 4 values |
| `Mat2_Identity()` | Identity matrix |
| `Mat2_MulVec(m, v)` | Matrix × vector |
| `Mat2_Mul(a, b)` | Matrix × matrix |
| `Mat2_Scale(sx, sy)` | Scaling matrix |
| `Mat2_Rotate(angle)` | Rotation matrix |

### Trig Functions

| Function | Description |
|----------|-------------|
| `Fixed_Sin(angle)` | Sine of binary angle, returns Q16.8 |
| `Fixed_Cos(angle)` | Cosine of binary angle, returns Q16.8 |

---

## Usage Examples

### Basic Vector Math

```c
Vec2 pos = Vec2_FromInt(100, 200);    /* (100.0, 200.0) */
Vec2 vel = Vec2_Create(128, 64);      /* (0.5, 0.25) in Q16.8 */

pos = Vec2_Add(pos, vel);             /* pos += vel */
```

### Angle and Rotation

```c
int facing = 0;                        /* 0° (facing right) */

/* Steer left */
facing = (facing + 8) & ANGLE_MASK;    /* Add ~5.6° */

/* Get direction vector */
Vec2 dir = Vec2_FromAngle(facing);

/* Rotate a vector */
Vec2 rotated = Vec2_Rotate(vel, facing);
```

### Speed Clamping

```c
Q16_8 maxSpeed = IntToFixed(5);        /* 5 pixels/frame */
vel = Vec2_ClampLen(vel, maxSpeed);
```

### Matrix Transforms

```c
Mat2 rot = Mat2_Rotate(facing);
Mat2 scale = Mat2_Scale(IntToFixed(2), IntToFixed(2));
Mat2 transform = Mat2_Mul(rot, scale);

Vec2 transformed = Mat2_MulVec(transform, original);
```

### Perpendicular and Reflection

```c
/* Get perpendicular for wall sliding */
Vec2 wallNormal = Vec2_Normalize(wallDir);
Vec2 slideDir = Vec2_Perp(wallNormal);

/* Bounce off wall */
Vec2 bounced = Vec2_Reflect(velocity, wallNormal);
```

### Rotate Around Pivot

```c
/* Rotate kart's corner points around its center */
Vec2 corner = Vec2_FromInt(16, 8);  /* offset from center */
Vec2 center = kart.position;
Vec2 worldCorner = Vec2_RotateAround(corner, Vec2_Zero(), kart.facing);
worldCorner = Vec2_Add(worldCorner, center);
```

### Projection and Rejection

```c
/* Split velocity into components along and perpendicular to slope */
Vec2 slopeDir = Vec2_Normalize(slope);
Vec2 alongSlope = Vec2_Project(velocity, slopeDir);   /* sliding component */
Vec2 intoSlope = Vec2_Reject(velocity, slopeDir);     /* penetration component */
```

### Distance Checks

```c
/* Cheap distance comparison (no sqrt) */
Q16_8 distSq = Vec2_DistanceSquared(player.pos, enemy.pos);
Q16_8 rangeSq = FixedMul(IntToFixed(100), IntToFixed(100));
if (distSq < rangeSq) {
    /* Within 100 pixels */
}

/* Actual distance when you need the value */
Q16_8 dist = Vec2_Distance(player.pos, checkpoint.pos);
```

---

## File Structure

```
vect2.h     Header with types, macros, inline functions, prototypes
vect2.c     Implementation of heavy operations (LUT, sqrt, etc.)
```

---

## Performance Notes

- **Inline operations**: Vec2_Add, Vec2_Scale, etc. compile to a few instructions
- **LUT trig**: Fixed_Sin/Fixed_Cos are O(1) lookups
- **Integer sqrt**: Used only in Vec2_Len, Vec2_Normalize, Vec2_ClampLen
- **No allocations**: All operations work on stack values
- **No floats**: Everything is integer arithmetic

