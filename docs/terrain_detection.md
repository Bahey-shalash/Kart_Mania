# Terrain Detection Module

## Overview

The terrain detection module determines surface type (track vs sand) at specific world coordinates by sampling the background tilemap and analyzing pixel colors. This enables terrain-specific physics effects such as speed reduction when karts drive off-track onto sand.

**Key Features:**
- **Color-Based Classification**: Uses 5-bit RGB values with tolerance matching to identify surface types
- **Two-Phase Detection**: First rejects track colors, then matches sand colors
- **Quadrant Coordinate System**: Supports large game worlds split into 256×256 pixel quadrants
- **Tilemap Sampling**: Direct hardware palette lookups from background tile data
- **Fast Performance**: O(1) single palette lookup per detection call

## Architecture

### Coordinate Transformation Pipeline

The module converts world coordinates through multiple spaces to reach pixel data:

```
World Coordinates (x, y, quadrant)
    ↓
Quadrant-Local Coordinates (localX, localY)
    ↓
Tile Coordinates (tileX, tileY)
    ↓
Screen-Local Tile Coordinates (localTileX, localTileY, screenBase)
    ↓
Pixel-Within-Tile Coordinates (pixelX, pixelY)
    ↓
Palette Index → 5-bit RGB Color
```

### Quadrant System

The game world is divided into a grid of quadrants:

| Parameter | Value | Description |
|-----------|-------|-------------|
| Quadrant size | 256×256 px | Each quadrant is 256 pixels square |
| Grid layout | Variable | Defined by `QUADRANT_GRID_SIZE` |
| Quadrant offset | 256 px | Spacing between quadrant origins |
| Screen layout | 2×2 screens | Each quadrant uses 4 screen bases |

**Quadrant ID to Grid Conversion:**
```c
int col = quad % QUADRANT_GRID_SIZE;  // Column in grid
int row = quad / QUADRANT_GRID_SIZE;  // Row in grid
```

### Tilemap Structure

Each quadrant's background is rendered using 8×8 pixel tiles:

| Component | Value | Description |
|-----------|-------|-------------|
| Tile size | 8×8 px | Each tile is 8 pixels square |
| Screen size | 32×32 tiles | Standard DS screen map size |
| Tile data size | 64 bytes | 8×8 pixels, 1 byte per pixel (palette index) |
| Screen bases | 0-3 | Four screen maps for 2×2 layout |
| Map base RAM | `BG_MAP_RAM(screenBase)` | Tile indices (u16) |
| Tile data RAM | `BG_TILE_RAM(1)` | Pixel palette indices (u8) |

### Color Detection System

**5-bit RGB Format:**
- DS uses 15-bit color: 5 bits per channel (0-31 range)
- Palette entry format: `0bBBBBBGGGGGRRRRR`
- Bit masks: Red (bits 0-4), Green (bits 5-9), Blue (bits 10-14)

**Known Colors:**

| Surface Type | Color Name | R | G | B | 5-bit RGB |
|--------------|-----------|---|---|---|-----------|
| Track | Main gray | 12 | 12 | 12 | (12,12,12) |
| Track | Light gray | 14 | 14 | 14 | (14,14,14) |
| Sand | Primary sand | 20 | 18 | 12 | (20,18,12) |
| Sand | Secondary sand | 22 | 20 | 14 | (22,20,14) |

**Tolerance:** ±1 unit per channel in 5-bit space allows for slight palette variations.

## Public API

### Terrain_IsOnSand

```c
bool Terrain_IsOnSand(int x, int y, QuadrantID quad);
```

**Location:** [terrain_detection.c:97](../source/gameplay/terrain_detection.c#L97)

Determines if a world position is on sand terrain (off-track).

**Parameters:**
- `x` - World X coordinate in pixels
- `y` - World Y coordinate in pixels
- `quad` - Quadrant ID (identifies which 256×256 section of map)

**Returns:**
- `true` - Position is on sand (off-track, applies speed penalty)
- `false` - Position is on track or out of bounds

**Algorithm:**
1. Convert quadrant ID to grid coordinates (row, column)
2. Calculate quadrant's world offset
3. Convert to quadrant-local coordinates
4. Perform bounds check (reject if outside quadrant)
5. Convert pixel coordinates to tile coordinates
6. Determine which screen base (0-3) contains the tile
7. Read tile index from screen map base
8. Calculate pixel position within tile
9. Read palette index from tile data
10. Extract 5-bit RGB from palette entry
11. Check if color matches track gray (return false if yes)
12. Check if color matches sand colors (return result)

**Example Usage:**
```c
// Check if kart at position (512, 384) in quadrant 5 is on sand
int kart_x = 512;
int kart_y = 384;
QuadrantID current_quad = 5;

if (Terrain_IsOnSand(kart_x, kart_y, current_quad)) {
    // Kart is off-track, apply speed penalty
    kart_speed *= SAND_SPEED_MULTIPLIER;
}
```

## Private Implementation

### Terrain_ColorMatches5bit

```c
static inline bool Terrain_ColorMatches5bit(int r5, int g5, int b5,
                                             int targetR5, int targetG5, int targetB5,
                                             int tolerance);
```

**Location:** [terrain_detection.c:61](../source/gameplay/terrain_detection.c#L61)

Checks if a 5-bit RGB color matches a target color within tolerance.

**Parameters:**
- `r5, g5, b5` - Color to test (5-bit values, 0-31)
- `targetR5, targetG5, targetB5` - Target color (5-bit values, 0-31)
- `tolerance` - Maximum difference per channel

**Returns:** `true` if all channels within tolerance, `false` otherwise

**Implementation:**
```c
return (abs(r5 - targetR5) <= tolerance &&
        abs(g5 - targetG5) <= tolerance &&
        abs(b5 - targetB5) <= tolerance);
```

### Terrain_IsGrayTrack5bit

```c
static inline bool Terrain_IsGrayTrack5bit(int r5, int g5, int b5);
```

**Location:** [terrain_detection.c:79](../source/gameplay/terrain_detection.c#L79)

Checks if a 5-bit RGB color represents gray track surface.

**Parameters:**
- `r5, g5, b5` - Color to test (5-bit values, 0-31)

**Returns:** `true` if color matches gray track, `false` otherwise

**Checked Colors:**
- Main gray: (12, 12, 12) with ±1 tolerance
- Light gray: (14, 14, 14) with ±1 tolerance

## Color Detection System

### Two-Phase Checking

The detection uses a two-phase approach for efficiency and accuracy:

**Phase 1: Track Rejection**
```c
if (Terrain_IsGrayTrack5bit(r5, g5, b5))
    return false;  // Definitely NOT sand
```

Quickly eliminates track pixels by checking against known gray colors. This phase prioritizes performance for the common case (karts on track).

**Phase 2: Sand Matching**
```c
return Terrain_ColorMatches5bit(r5, g5, b5, SAND_PRIMARY_R5, SAND_PRIMARY_G5,
                                 SAND_PRIMARY_B5, COLOR_TOLERANCE_5BIT) ||
       Terrain_ColorMatches5bit(r5, g5, b5, SAND_SECONDARY_R5, SAND_SECONDARY_G5,
                                 SAND_SECONDARY_B5, COLOR_TOLERANCE_5BIT);
```

Matches against two known sand colors. Returns `true` if either matches, `false` otherwise (includes other terrain types like grass, barriers, etc.).

### Color Constants

**Private Constants** ([terrain_detection.c:22-45](../source/gameplay/terrain_detection.c#L22-L45)):

```c
// Gray track colors (5-bit RGB, 0-31 range)
#define GRAY_MAIN_R5 12    // Main gray track R channel
#define GRAY_MAIN_G5 12    // Main gray track G channel
#define GRAY_MAIN_B5 12    // Main gray track B channel
#define GRAY_LIGHT_R5 14   // Light gray track R channel
#define GRAY_LIGHT_G5 14   // Light gray track G channel
#define GRAY_LIGHT_B5 14   // Light gray track B channel

// Sand colors (5-bit RGB, 0-31 range)
#define SAND_PRIMARY_R5 20     // Primary sand R channel
#define SAND_PRIMARY_G5 18     // Primary sand G channel
#define SAND_PRIMARY_B5 12     // Primary sand B channel
#define SAND_SECONDARY_R5 22   // Secondary sand R channel
#define SAND_SECONDARY_G5 20   // Secondary sand G channel
#define SAND_SECONDARY_B5 14   // Secondary sand B channel

// Color matching tolerance (5-bit space)
#define COLOR_TOLERANCE_5BIT 1  // ±1 unit tolerance per channel
```

### Palette Extraction

**Reading Pixel Color from Tilemap:**

```c
// 1. Get palette index from tile data (8-bit)
u8* tileData = (u8*)BG_TILE_RAM(1);
u8 paletteIndex = tileData[tileIndex * TILE_DATA_SIZE + pixelY * TILE_WIDTH_PIXELS + pixelX];

// 2. Read 15-bit color from palette
u16 paletteColor = BG_PALETTE[paletteIndex];

// 3. Extract 5-bit RGB channels
int r5 = (paletteColor >> COLOR_RED_SHIFT) & COLOR_5BIT_MASK;    // Bits 0-4
int g5 = (paletteColor >> COLOR_GREEN_SHIFT) & COLOR_5BIT_MASK;  // Bits 5-9
int b5 = (paletteColor >> COLOR_BLUE_SHIFT) & COLOR_5BIT_MASK;   // Bits 10-14
```

**Constants from game_constants.h:**
- `COLOR_RED_SHIFT` = 0
- `COLOR_GREEN_SHIFT` = 5
- `COLOR_BLUE_SHIFT` = 10
- `COLOR_5BIT_MASK` = 0x1F (binary: 0b11111)

## Usage Patterns

### Basic Terrain Check

```c
// During kart physics update
QuadrantID kart_quadrant = Kart_GetQuadrant(&kart);
int kart_x = kart.position.x;
int kart_y = kart.position.y;

bool on_sand = Terrain_IsOnSand(kart_x, kart_y, kart_quadrant);

if (on_sand) {
    // Apply sand physics penalty
    kart.speed *= 0.7;  // 30% speed reduction
}
```

### Multiple Sample Points

```c
// Check front and rear of kart for better accuracy
typedef struct {
    int front_x, front_y;
    int rear_x, rear_y;
} KartSamplePoints;

KartSamplePoints samples = Kart_GetSamplePoints(&kart);
QuadrantID quad = kart.quadrant;

bool front_on_sand = Terrain_IsOnSand(samples.front_x, samples.front_y, quad);
bool rear_on_sand = Terrain_IsOnSand(samples.rear_x, samples.rear_y, quad);

if (front_on_sand || rear_on_sand) {
    // At least partial contact with sand
    ApplySandPenalty(&kart, front_on_sand && rear_on_sand);
}
```

### Bounds Check Handling

```c
// Function returns false for out-of-bounds positions
bool on_sand = Terrain_IsOnSand(kart_x, kart_y, kart_quadrant);

// Out-of-bounds is treated as "not sand" (safe default)
// Physics system should have separate bounds checking for collisions
```

### Integration with Physics Loop

```c
// In main physics update (called every frame)
void Physics_UpdateKart(Kart* kart) {
    // ... velocity calculations ...

    // Terrain detection
    bool on_sand = Terrain_IsOnSand(kart->x, kart->y, kart->quadrant);

    // Apply terrain-specific modifier
    if (on_sand) {
        kart->acceleration *= SAND_ACCELERATION_PENALTY;
        kart->max_speed *= SAND_MAX_SPEED_PENALTY;
    }

    // ... collision detection ...
}
```

## Design Notes

### Two-Phase Detection Rationale

**Why check track colors before sand colors?**

1. **Common Case Optimization**: Karts spend most time on-track, so rejecting track pixels first minimizes unnecessary sand checks
2. **Clear Classification**: Track colors are more uniform (two gray shades), making rejection faster and more reliable
3. **False Positive Prevention**: Without track rejection, other terrain elements (barriers, grass borders) might match sand tolerance

### Tolerance Value

**Why ±1 unit in 5-bit space?**

- **Compression Artifacts**: PNG to GRIT conversion may introduce slight color variations
- **Palette Quantization**: DS palette has limited color slots, causing rounding
- **Balance**: Tolerance of 1 catches variations while avoiding false matches (tolerance of 2 would overlap gray and sand ranges)

### Out-of-Bounds Behavior

Returns `false` for positions outside quadrant bounds, treating them as "not sand." This is a safe default because:

1. Physics system handles collision separately
2. Out-of-bounds positions indicate larger logic errors (should be caught elsewhere)
3. Returning `false` prevents accidental sand penalties from coordinate bugs

### Screen Base Layout

**2×2 Screen Arrangement:**

```
+--------+--------+
| Base 0 | Base 1 |  Each screen: 32×32 tiles (256×256 px)
+--------+--------+
| Base 2 | Base 3 |  Total: 512×512 px (exceeds quadrant size)
+--------+--------+
```

**Screen Base Calculation:**
```c
int screenX = tileX / 32;  // Which horizontal screen (0 or 1)
int screenY = tileY / 32;  // Which vertical screen (0 or 1)
int screenBase = screenY * 2 + screenX;  // 0, 1, 2, or 3
```

The quadrant (256×256 px) uses only the top-left portion of the 2×2 screen layout.

### Alternative Approaches Not Used

**Bitmask Lookup Table:**
- Could pre-compute sand/track bitmask for entire quadrant
- Would be faster (bitwise check vs palette lookup)
- Not used: Memory cost (8KB per quadrant), static terrain assumption

**Hardware Collision Detection:**
- DS supports sprite-background collision
- Not used: Requires sprite at check position, hardware limitations

**Tile-Level Detection:**
- Could tag entire tiles as sand/track
- Not used: Loses pixel precision at tile boundaries

## Performance & Integration

### Performance Characteristics

- **Time Complexity**: O(1) per call (single palette lookup)
- **Memory Access**: 3 reads (map entry, tile data, palette)
- **Typical Frame Budget**: ~100 calls per frame (4 karts × ~25 sample points)
- **No Allocations**: All arithmetic on stack

### Dependencies

**Required Headers:**
- `nds.h` - DS hardware registers (BG_MAP_RAM, BG_TILE_RAM, BG_PALETTE)
- `stdlib.h` - abs() for tolerance checking
- `game_constants.h` - Tile/quadrant size constants, color masks/shifts
- `game_types.h` - QuadrantID type definition

**Constants from game_constants.h:**
```c
QUADRANT_GRID_SIZE     // Grid dimensions
QUAD_SIZE_DOUBLE       // Quadrant pixel size (256)
TILE_SIZE             // Tile size in pixels (8)
SCREEN_SIZE_TILES     // Screen size in tiles (32)
TILE_INDEX_MASK       // Mask for tile index bits
TILE_DATA_SIZE        // Bytes per tile (64)
TILE_WIDTH_PIXELS     // Tile width (8)
COLOR_RED_SHIFT       // Bit position for red (0)
COLOR_GREEN_SHIFT     // Bit position for green (5)
COLOR_BLUE_SHIFT      // Bit position for blue (10)
COLOR_5BIT_MASK       // 5-bit channel mask (0x1F)
```

### Integration Points

**Called by:**
- Physics engine during velocity/acceleration calculations
- AI system for pathfinding (avoid sand for optimal racing line)
- Particle effects (dust clouds on sand, sparks on track)

**State Dependencies:**
- Requires active background layer with tilemap loaded
- Palette must contain track/sand colors as specified
- Quadrant system must be initialized

### Testing Considerations

**Test Cases:**
1. On-track gray pixels (should return `false`)
2. On-sand beige pixels (should return `true`)
3. Out-of-bounds positions (should return `false`)
4. Quadrant boundaries (test edge coordinates)
5. Screen base transitions (tileX/tileY = 31 → 32)
6. Tolerance edge cases (colors ±1 unit from targets)

**Manual Testing:**
- Place kart on different terrain types
- Monitor speed changes in debug overlay
- Verify no false positives on grass/barriers

---

## Navigation

- [← Back to Wiki](wiki.md)
- [← Back to README](../README.md)
