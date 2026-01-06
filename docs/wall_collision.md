# Wall Collision Module

## Overview

The wall collision module detects collisions between circular kart hitboxes and axis-aligned wall segments that define racing track boundaries. It provides collision detection and normal vector calculation for bounce physics. Each of the 9 track quadrants has pre-defined wall geometry stored in global world coordinates.

**Key Features:**
- **Axis-Aligned Walls**: All walls are either horizontal or vertical segments
- **Circle-to-Segment Collision**: Fast perpendicular distance testing with range checks
- **Global Coordinates**: Wall geometry pre-converted to world space (no runtime transforms)
- **Collision Normals**: Computes unit normal vectors for realistic bounce physics
- **Per-Quadrant Geometry**: 9 quadrants with 4-9 wall segments each (total: 55 walls)

## Architecture

### Wall Representation

Walls are represented as axis-aligned line segments with a fixed coordinate on one axis and a range on the other:

**Horizontal Walls:**
```
Y = fixed_coord (constant)
X ∈ [min_range, max_range]

Example: {WALL_HORIZONTAL, 160, 256, 512}
→ Horizontal wall at Y=160, spanning X from 256 to 512
```

**Vertical Walls:**
```
X = fixed_coord (constant)
Y ∈ [min_range, max_range]

Example: {WALL_VERTICAL, 735, 8, 160}
→ Vertical wall at X=735, spanning Y from 8 to 160
```

### Quadrant System

The 1024×1024 pixel race track is divided into 9 quadrants (3×3 grid). Each quadrant spans a 256px origin offset with 256px overlap (double-quadrant size 512px) to allow safe collision checks near edges:

| Grid Position | Quadrant ID | Offset (X, Y) | Wall Count |
|---------------|-------------|---------------|------------|
| Top-Left | QUAD_TL (0) | (0, 0) | 6 walls |
| Top-Center | QUAD_TC (1) | (256, 0) | 8 walls |
| Top-Right | QUAD_TR (2) | (512, 0) | 8 walls |
| Middle-Left | QUAD_ML (3) | (0, 256) | 9 walls |
| Middle-Center | QUAD_MC (4) | (256, 256) | 8 walls |
| Middle-Right | QUAD_MR (5) | (512, 256) | 7 walls |
| Bottom-Left | QUAD_BL (6) | (0, 512) | 11 walls |
| Bottom-Center | QUAD_BC (7) | (256, 512) | 7 walls |
| Bottom-Right | QUAD_BR (8) | (512, 512) | 5 walls |

**Total Walls:** 69 segments across 9 quadrants (see `source/gameplay/wall_collision.c`)

### Collision Detection Algorithm

**Circle-to-Segment Test:**

1. **Perpendicular Distance Check**
   - For horizontal walls: `distY = |carY - fixed_coord|`
   - For vertical walls: `distX = |carX - fixed_coord|`
   - If distance > radius, no collision (early reject)

2. **Range Overlap Check**
   - For horizontal walls: Check if `[carX - radius, carX + radius]` overlaps `[min_range, max_range]`
   - For vertical walls: Check if `[carY - radius, carY + radius]` overlaps `[min_range, max_range]`
   - Collision occurs if both checks pass

**Collision Normal Calculation:**

1. Iterate through all walls in quadrant
2. Find wall with minimum perpendicular distance to kart center
3. Check if kart's position on variable axis is within wall's range
4. Return unit normal pointing away from wall:
   - Horizontal walls: `(0, ±1)` - up or down
   - Vertical walls: `(±1, 0)` - left or right

### Data Structure

```c
typedef enum {
    WALL_HORIZONTAL,  // Constant Y, X range
    WALL_VERTICAL     // Constant X, Y range
} WallType;

typedef struct {
    WallType type;        // Orientation
    int fixed_coord;      // X for vertical, Y for horizontal
    int min_range;        // Start of variable axis range
    int max_range;        // End of variable axis range
} WallSegment;

typedef struct {
    const WallSegment* segments;  // Array of wall segments
    int count;                     // Number of segments
} QuadrantWalls;
```

## Public API

### Wall_CheckCollision

```c
bool Wall_CheckCollision(int carX, int carY, int carRadius, QuadrantID quad);
```

**Location:** [wall_collision.c:178](../source/gameplay/wall_collision.c#L178)

Checks if a circular kart hitbox collides with any walls in the quadrant.

**Parameters:**
- `carX` - Kart center X coordinate in world space
- `carY` - Kart center Y coordinate in world space
- `carRadius` - Kart collision radius in pixels (typically 16 for 32×32 sprite)
- `quad` - Current quadrant ID (0-8)

**Returns:**
- `true` - Collision detected with at least one wall
- `false` - No collision or invalid quadrant

**Algorithm:**
1. Validate quadrant ID is in range [QUAD_TL, QUAD_BR]
2. Retrieve wall array for quadrant
3. Test each wall segment using Wall_SegmentCollision()
4. Return true on first collision, false if none found

**Example Usage:**
```c
// During kart physics update
QuadrantID kart_quadrant = Kart_GetQuadrant(&kart);
int kart_x = kart.position.x;
int kart_y = kart.position.y;

bool hit_wall = Wall_CheckCollision(kart_x, kart_y, CAR_RADIUS, kart_quadrant);

if (hit_wall) {
    // Handle collision (stop movement, apply bounce)
    Kart_RevertPosition(&kart);
}
```

### Wall_GetCollisionNormal

```c
void Wall_GetCollisionNormal(int carX, int carY, QuadrantID quad, int* nx, int* ny);
```

**Location:** [wall_collision.c:193](../source/gameplay/wall_collision.c#L193)

Determines the collision normal vector for the nearest wall.

**Parameters:**
- `carX` - Kart center X coordinate in world space
- `carY` - Kart center Y coordinate in world space
- `quad` - Current quadrant ID (0-8)
- `nx` - Output: X component of normal vector (-1, 0, or 1)
- `ny` - Output: Y component of normal vector (-1, 0, or 1)

**Normal Directions:**
- Horizontal walls: `ny = ±1, nx = 0` (points up or down from wall)
- Vertical walls: `nx = ±1, ny = 0` (points left or right from wall)

**Sign Convention:**
- If `carY > wall.fixed_coord` (below horizontal wall): `ny = +1` (push down)
- If `carY < wall.fixed_coord` (above horizontal wall): `ny = -1` (push up)
- If `carX > wall.fixed_coord` (right of vertical wall): `nx = +1` (push right)
- If `carX < wall.fixed_coord` (left of vertical wall): `nx = -1` (push left)

**Algorithm:**
1. Validate quadrant ID, return (0, 0) if invalid
2. Initialize minimum distance to large value (9999)
3. For each wall in quadrant:
   - Calculate perpendicular distance to kart center
   - Check if kart position on variable axis is within wall range
   - If distance < current minimum, update best normal
4. Return best normal found

**Example Usage:**
```c
// After detecting collision, compute bounce direction
int normal_x, normal_y;
Wall_GetCollisionNormal(kart_x, kart_y, kart_quadrant, &normal_x, &normal_y);

// Apply bounce physics (reflect velocity)
// v_new = v_old - 2 * (v_old · n) * n
int dot_product = kart.velocity_x * normal_x + kart.velocity_y * normal_y;
kart.velocity_x -= 2 * dot_product * normal_x * BOUNCE_DAMPING;
kart.velocity_y -= 2 * dot_product * normal_y * BOUNCE_DAMPING;
```

## Private Implementation

### Wall_SegmentCollision

```c
static inline bool Wall_SegmentCollision(const WallSegment* wall, int carX, int carY,
                                          int radius);
```

**Location:** [wall_collision.c:157](../source/gameplay/wall_collision.c#L157)

Tests if a circular kart hitbox collides with a single axis-aligned wall segment.

**Parameters:**
- `wall` - Wall segment to test against
- `carX` - Kart center X coordinate
- `carY` - Kart center Y coordinate
- `radius` - Kart collision radius

**Returns:** `true` if collision detected, `false` otherwise

**Algorithm:**

**For Horizontal Walls (WALL_HORIZONTAL):**
```c
1. distY = |carY - wall.fixed_coord|      // Perpendicular distance
2. if (distY > radius) return false        // Too far vertically
3. Check X range overlap:
   return (carX + radius >= wall.min_range && carX - radius <= wall.max_range)
```

**For Vertical Walls (WALL_VERTICAL):**
```c
1. distX = |carX - wall.fixed_coord|      // Perpendicular distance
2. if (distX > radius) return false        // Too far horizontally
3. Check Y range overlap:
   return (carY + radius >= wall.min_range && carY - radius <= wall.max_range)
```

**Optimization:** Uses `inline` for performance (called in tight loop)

## Wall Geometry Data

### Quadrant Wall Arrays

All wall segments are pre-defined as static const arrays in [wall_collision.c:20-135](../source/gameplay/wall_collision.c#L20-L135). Coordinates are already converted to global world space.

**Example: Top-Left Quadrant (TL)**
```c
static const WallSegment walls_TL[] = {
    {WALL_VERTICAL, 8, 0, 512},        // Left border at X=8, Y: 0-512
    {WALL_HORIZONTAL, 8, 0, 512},      // Top border at Y=8, X: 0-512
    {WALL_VERTICAL, 167, 160, 512},    // Interior wall at X=167, Y: 160-512
    {WALL_HORIZONTAL, 160, 167, 273},  // Interior wall at Y=160, X: 167-273
    {WALL_VERTICAL, 273, 162, 375},    // Interior wall at X=273, Y: 162-375
    {WALL_HORIZONTAL, 375, 273, 512}   // Interior wall at Y=375, X: 273-512
};
```

**Coordinate Transformations:**

Walls were originally designed in local quadrant space (0-255) and transformed:
- **TL** (offset 0, 0): No transformation
- **TC** (offset 256, 0): Add 256 to all X coordinates
- **TR** (offset 512, 0): Add 512 to all X coordinates
- **ML** (offset 0, 256): Add 256 to all Y coordinates
- **MC** (offset 256, 256): Add 256 to X and Y
- **MR** (offset 512, 256): Add 512 to X, 256 to Y
- **BL** (offset 0, 512): Add 512 to all Y coordinates
- **BC** (offset 256, 512): Add 256 to X, 512 to Y
- **BR** (offset 512, 512): Add 512 to X and Y

**Lookup Table:**
```c
static const QuadrantWalls quadrantWalls[9] = {
    {walls_TL, sizeof(walls_TL) / sizeof(WallSegment)},
    {walls_TC, sizeof(walls_TC) / sizeof(WallSegment)},
    // ... (all 9 quadrants)
};
```

Indexed directly by QuadrantID enum (0-8).

## Usage Patterns

### Basic Collision Check

```c
// Check for wall collision before moving kart
int new_x = kart.x + velocity_x;
int new_y = kart.y + velocity_y;

if (Wall_CheckCollision(new_x, new_y, CAR_RADIUS, kart.quadrant)) {
    // Collision detected, don't move
    new_x = kart.x;
    new_y = kart.y;
} else {
    // Safe to move
    kart.x = new_x;
    kart.y = new_y;
}
```

### Collision with Bounce Physics

```c
// Detect collision
if (Wall_CheckCollision(kart.x, kart.y, CAR_RADIUS, kart.quadrant)) {
    // Get collision normal
    int nx, ny;
    Wall_GetCollisionNormal(kart.x, kart.y, kart.quadrant, &nx, &ny);

    // Reflect velocity: v' = v - 2(v·n)n
    int dot = kart.vx * nx + kart.vy * ny;
    kart.vx -= 2 * dot * nx;
    kart.vy -= 2 * dot * ny;

    // Apply damping (lose energy on bounce)
    kart.vx = (kart.vx * 7) / 10;  // 30% energy loss
    kart.vy = (kart.vy * 7) / 10;

    // Push kart slightly away from wall to prevent sticking
    kart.x += nx * 2;
    kart.y += ny * 2;
}
```

### Predictive Collision (Look-Ahead)

```c
// Check if next position will collide
int next_x = kart.x + kart.velocity_x;
int next_y = kart.y + kart.velocity_y;

if (Wall_CheckCollision(next_x, next_y, CAR_RADIUS, kart.quadrant)) {
    // Collision imminent, stop or slow down
    kart.velocity_x = 0;
    kart.velocity_y = 0;
} else {
    // Safe to continue
    kart.x = next_x;
    kart.y = next_y;
}
```

### Multi-Point Collision (Front and Rear)

```c
// Check front and rear of kart separately for better accuracy
typedef struct {
    int front_x, front_y;
    int rear_x, rear_y;
} KartCollisionPoints;

KartCollisionPoints pts = Kart_GetCollisionPoints(&kart);

bool front_collision = Wall_CheckCollision(pts.front_x, pts.front_y,
                                            CAR_RADIUS/2, kart.quadrant);
bool rear_collision = Wall_CheckCollision(pts.rear_x, pts.rear_y,
                                           CAR_RADIUS/2, kart.quadrant);

if (front_collision || rear_collision) {
    // Partial collision - apply rotation penalty or stop
    if (front_collision && !rear_collision) {
        // Front hit wall, rotate kart backwards
        kart.angle += 180;
    }
}
```

### Cross-Quadrant Collision

```c
// Handle kart near quadrant boundaries
QuadrantID primary_quad = kart.quadrant;
QuadrantID adjacent_quads[4];
int adjacent_count = Quadrant_GetAdjacent(primary_quad, adjacent_quads);

bool collision = Wall_CheckCollision(kart.x, kart.y, CAR_RADIUS, primary_quad);

// Check adjacent quadrants if near boundary
if (!collision && Quadrant_NearBoundary(kart.x, kart.y, CAR_RADIUS)) {
    for (int i = 0; i < adjacent_count; i++) {
        if (Wall_CheckCollision(kart.x, kart.y, CAR_RADIUS, adjacent_quads[i])) {
            collision = true;
            break;
        }
    }
}
```

## Design Notes

### Axis-Aligned Walls Only

**Why no diagonal walls?**

1. **Simplicity**: Axis-aligned segments use simple absolute distance calculations
2. **Performance**: No trigonometry, rotation, or dot products needed for detection
3. **Track Design**: Racing tracks naturally have mostly straight sections
4. **Normal Computation**: Trivial to compute (0,±1) or (±1,0)

Diagonal walls could be approximated with staircase patterns using multiple short axis-aligned segments.

### Global Coordinates

**Why pre-transform walls to world space?**

1. **Performance**: Transformation done once at compile time, not every frame
2. **Simplicity**: No need to convert kart coordinates to local quadrant space
3. **Memory Trade-off**: Small data size (69 wall segments × 16 bytes = 1104 bytes total)
4. **Consistency**: Kart positions already in global coordinates

Alternative (local coordinates) would save ~400 bytes but add runtime overhead.

### Collision Radius

**Default CAR_RADIUS = 16 pixels (for 32×32 sprite)**

- **Sprite Size**: 32×32 pixels
- **Hitbox**: Inscribed circle with radius 16
- **Rationale**: Circular hitbox simplifies collision math, avoids rotation issues
- **Adjustable**: Can pass custom radius to Wall_CheckCollision() for different kart sizes

### Normal Direction Convention

**Normals point AWAY from walls (push direction):**

```
       ny = -1
          ↑
    ------+------  (Horizontal wall)
          ↓
       ny = +1

nx = -1 ← | → nx = +1
       (Vertical wall)
```

This convention matches physics engine expectations: normal points from surface into free space.

### Minimum Distance Search

**Wall_GetCollisionNormal() finds nearest wall:**

- Initializes `minDist = 9999` (larger than any possible distance in 768×768 world)
- Iterates all walls, updating best normal when closer wall found
- Returns (0, 0) if no walls in range (shouldn't happen in valid quadrant)

**Why not return collision wall from Wall_CheckCollision()?**

- Separation of concerns: detection vs normal computation
- CheckCollision early-exits on first hit (faster)
- GetCollisionNormal needs to find closest wall regardless

### Edge Case Handling

**Invalid Quadrant ID:**
- Both functions return safe defaults (`false` and `(0,0)`)
- Physics system should prevent invalid quadrants

**Kart Exactly on Wall:**
- Distance = 0, which is ≤ radius, so collision detected
- Normal points away based on side (using `> fixed_coord` check)

**Corner Collisions:**
- Kart can collide with multiple walls simultaneously
- CheckCollision returns true if ANY wall hit
- GetCollisionNormal returns normal of NEAREST wall

## Performance & Integration

### Performance Characteristics

- **Wall_CheckCollision**: O(n) where n = walls in quadrant (average n ≈ 7.7)
- **Wall_GetCollisionNormal**: O(n) full iteration (no early exit)
- **Memory**: 1104 bytes const data, ~40 bytes stack per call
- **Typical Frame Budget**: 4 karts × 2 calls = 8 calls per frame (~60 wall tests)

**Optimizations:**
- `inline` function for Wall_SegmentCollision
- Early-exit on first collision in Wall_CheckCollision
- No heap allocations
- Static const data in flash (not RAM)

### Dependencies

**Required Headers:**
- `stdbool.h` - bool type
- `game_types.h` - QuadrantID enum (QUAD_TL through QUAD_BR)

**QuadrantID Values:**
```c
typedef enum {
    QUAD_TL = 0,  // Top-Left
    QUAD_TC = 1,  // Top-Center
    QUAD_TR = 2,  // Top-Right
    QUAD_ML = 3,  // Middle-Left
    QUAD_MC = 4,  // Middle-Center
    QUAD_MR = 5,  // Middle-Right
    QUAD_BL = 6,  // Bottom-Left
    QUAD_BC = 7,  // Bottom-Center
    QUAD_BR = 8   // Bottom-Right
} QuadrantID;
```

### Integration Points

**Called by:**
- Physics engine during kart movement updates
- AI pathfinding to avoid walls
- Collision resolution system
- Game logic for wall-based events (sparks, sound effects)

**Coordinates:**
- Expects kart positions in global world coordinates (0-767 range)
- Quadrant must match kart's current position (caller responsibility)
- Returns results relative to global coordinate system

### Testing Considerations

**Test Cases:**
1. **Basic Collision**: Kart centered on wall
2. **Edge Collision**: Kart at min_range and max_range boundaries
3. **Near Miss**: Kart at radius + 1 distance (should not collide)
4. **Invalid Quadrant**: quad = -1 or quad = 10 (should return safe defaults)
5. **Corner Collision**: Kart at intersection of horizontal and vertical walls
6. **Normal Directions**: All 4 cardinal directions (up, down, left, right)
7. **Out of Range**: Kart position outside wall's variable axis range
8. **Exact Match**: Kart position exactly at wall fixed_coord

**Manual Testing:**
- Place kart near walls, observe collision detection
- Check bounce physics use correct normals
- Verify no tunneling through walls at high speeds
- Test quadrant boundary transitions

**Debug Visualization:**
- Render wall segments as colored lines
- Draw kart collision circle
- Display collision normal as arrow when collision detected
