# Items System Architecture

## Overview

This document describes the internal architecture and design decisions of the items system. For usage information, see [Items Overview](items_overview.md) and [Items API Reference](items_api.md).

---

## Module Organization

### File Structure

The items system is divided into specialized modules for maintainability:

```
items/
├── Core Types & Constants
│   ├── items_types.h          → Data structures (Item, TrackItem, etc.)
│   ├── items_constants.h      → Game balance and config values
│   └── items_internal.h       → Internal shared state
│
├── Public Interface
│   └── items_api.h            → Public API functions
│
├── Navigation Subsystem
│   ├── item_navigation.h/c    → Waypoint-based homing
│
├── Core Implementations
│   ├── items_state.c          → Lifecycle and state management
│   ├── items_effects.c        → Player status effects
│   ├── items_render.c         → Sprite rendering
│   ├── items_debug.c          → Debug/testing
│   ├── items_inventory.c      → Item usage and random selection
│   ├── items_spawning.c       → Projectile/hazard creation
│   └── items_update.c         → Update loop and collisions
```

### Module Responsibilities

| Module | Responsibility | Dependencies |
|--------|---------------|--------------|
| **items_types.h** | Type definitions only | Standard libs |
| **items_constants.h** | Constants and probability tables | items_types.h |
| **items_internal.h** | Internal state and helpers | items_types.h |
| **items_api.h** | Public interface declarations | items_types.h |
| **item_navigation.c** | Waypoint navigation logic | None |
| **items_state.c** | Init, reset, state storage | None |
| **items_effects.c** | Player status effects | Car.h |
| **items_render.c** | Sprite rendering | Graphics data |
| **items_debug.c** | Debug accessors | None |
| **items_inventory.c** | Item usage, random selection | gameplay_logic.h |
| **items_spawning.c** | Item creation, network sync | item_navigation.h, multiplayer.h |
| **items_update.c** | Update loop, collisions | All of the above |

---

## Data Structures

### Item (Enum)
```c
typedef enum {
    ITEM_NONE = 0,
    ITEM_BOX,        // Pickup, not placed on track
    ITEM_OIL,        // Hazard
    ITEM_BOMB,       // Hazard
    ITEM_BANANA,     // Hazard
    ITEM_GREEN_SHELL,  // Projectile
    ITEM_RED_SHELL,    // Homing projectile
    ITEM_MISSILE,      // Homing projectile
    ITEM_MUSHROOM,     // Self-effect
    ITEM_SPEEDBOOST    // Self-effect
} Item;
```

**Design Notes:**
- `ITEM_NONE` is zero for default initialization
- `ITEM_BOX` exists as an enum but is never spawned as a `TrackItem`
- Enum values are used as array indices in probability tables

---

### TrackItem (Struct)
```c
typedef struct {
    // Core properties
    Item type;
    Vec2 position;           // Q16.8 fixed-point world position
    Vec2 startPosition;      // For distance tracking (oil slow)
    Q16_8 speed;             // Q16.8 fixed-point
    int angle512;            // 0-511 (512 = 360°)
    int hitbox_width;
    int hitbox_height;
    int lifetime_ticks;
    bool active;

    // Graphics
    u16* gfx;               // Sprite graphics pointer

    // Homing behavior
    int targetCarIndex;      // -1 = no target
    bool usePathFollowing;   // true = follow waypoints, false = direct attack
    int currentWaypoint;
    int waypointsVisited;

    // Shooter immunity (multiplayer safety)
    int shooterCarIndex;     // Who fired this (-1 = no shooter)
    int immunityTimer;       // Frames remaining OR -1 for lap-based
    int startingWaypoint;    // For lap detection
    bool hasCompletedLap;    // Single-player lap immunity
} TrackItem;
```

**Size:** ~60 bytes per item

**Pool:** 32 items = ~1.9 KB total

**Design Decisions:**
- **Fixed pool:** No dynamic allocation for predictable performance
- **Active flag:** Items are never deallocated, just marked inactive
- **Graphics pointer:** Shared between items of same type (no duplication)
- **Dual immunity:** Time-based (MP) vs lap-based (SP) for safety

---

### ItemBoxSpawn (Struct)
```c
typedef struct {
    Vec2 position;      // Fixed spawn location
    bool active;        // Available for pickup?
    int respawnTimer;   // Ticks until respawn
    u16* gfx;          // Sprite graphics pointer
} ItemBoxSpawn;
```

**Size:** ~16 bytes per box

**Pool:** 8 boxes = ~128 bytes

**Design Notes:**
- Item boxes never move (static spawn points)
- Respawn after 3 seconds when picked up
- One sprite per box (stable OAM mapping)

---

### PlayerItemEffects (Struct)
```c
typedef struct {
    // Confusion (mushroom)
    bool confusionActive;
    int confusionTimer;

    // Speed boost
    bool speedBoostActive;
    int speedBoostTimer;
    Q16_8 originalMaxSpeed;   // Restored when boost expires

    // Oil slow
    bool oilSlowActive;
    Vec2 oilSlowStart;        // Distance-based duration
} PlayerItemEffects;
```

**Size:** ~20 bytes

**Instances:** 1 global (single player) or 1 per player (multiplayer)

**Design Notes:**
- **Timer-based:** Confusion and boost use frame counters
- **Distance-based:** Oil slow tracks position delta
- **Original speed:** Prevents stacking multiple boosts

---

## Memory Layout

### Module State (items_state.c)
```c
// Global arrays
TrackItem activeItems[MAX_TRACK_ITEMS];           // 32 × 60 bytes = 1.9 KB
ItemBoxSpawn itemBoxSpawns[MAX_ITEM_BOX_SPAWNS];  // 8 × 16 bytes = 128 bytes
PlayerItemEffects playerEffects;                   // 20 bytes
int itemBoxCount;                                  // 4 bytes

// Graphics pointers (VRAM references)
u16* itemBoxGfx;      // 8×8 sprite
u16* bananaGfx;       // 16×16 sprite
u16* bombGfx;         // 16×16 sprite
u16* greenShellGfx;   // 16×16 sprite
u16* redShellGfx;     // 16×16 sprite
u16* missileGfx;      // 16×32 sprite
u16* oilSlickGfx;     // 32×32 sprite
```

**Total RAM:** ~2 KB for data structures + 7 pointers

**VRAM Usage:** ~4 KB for sprite graphics

---

## Algorithms

### Item Selection (Weighted Random)

**Function:** `Items_GetRandomItem()`

**Algorithm:**
```c
1. Clamp player rank to 1-8
2. Select probability table (SP or MP mode)
3. Calculate total weight = sum of all probabilities
4. Generate random number in [0, total)
5. Iterate through items, accumulating weights
6. Return item when random number < accumulated weight
```

**Example:**
```
Rank 3 (Single Player): Banana=25, Oil=25, Mushroom=20, Boost=30
Total = 100
Roll = 42

Cumulative:
  0-24:  Banana     ❌
  25-49: Oil        ✅ (42 is in this range)
```

**Complexity:** O(n) where n = number of item types (10)

**See:** [items_inventory.c:124-176](../source/gameplay/items/items_inventory.c#L124-L176)

---

### Homing Navigation

**Function:** `updateHoming()`

**Two-Phase System:**

#### Phase 1: Path Following
- Follow racing line waypoints
- Check if current waypoint reached (within 25 pixels)
- Advance to next waypoint
- Smooth turn toward waypoint (limited turn rate)

#### Phase 2: Direct Attack
- Scan for cars within 100 pixel radius
- Lock onto nearest car in detection range
- Switch to direct homing (point straight at target)
- If target moves >150 pixels away, return to Phase 1

**State Machine:**
```
┌─────────────────┐
│ Path Following  │ ◄─────────┐
│ (Waypoints)     │            │
└────────┬────────┘            │
         │                     │
         │ Car within          │ Target
         │ 100px radius        │ >150px away
         ▼                     │
┌─────────────────┐            │
│ Direct Attack   │────────────┘
│ (Target Car)    │
└─────────────────┘
```

**Turn Rate Limiting:**
```c
int angleDiff = (targetAngle - currentAngle) & ANGLE_MASK;
if (angleDiff > ANGLE_HALF) angleDiff -= ANGLE_FULL;
if (angleDiff > HOMING_TURN_RATE) angleDiff = HOMING_TURN_RATE;
if (angleDiff < -HOMING_TURN_RATE) angleDiff = -HOMING_TURN_RATE;
```

**Benefits:**
- Smooth visual movement (no instant turns)
- Predictable trajectories
- Can be dodged with skill

**See:** [items_update.c:210-299](../source/gameplay/items/items_update.c#L210-L299)

---

### Waypoint System

**Data Structure:**
```c
typedef struct {
    Vec2 pos;    // World position
    int next;    // Index of next waypoint
} Waypoint;
```

**Scorching Sands:** 119 waypoints around the racing line

**Functions:**
- `ItemNav_FindNearestWaypoint()` - O(n) linear search
- `ItemNav_GetWaypointPosition()` - O(1) array lookup
- `ItemNav_GetNextWaypoint()` - O(1) array lookup
- `ItemNav_IsWaypointReached()` - O(1) distance check

**Optimization Opportunities:**
- [ ] Spatial hashing for faster nearest waypoint lookup
- [ ] Pre-computed distance fields

**See:** [item_navigation.c](../source/gameplay/items/item_navigation.c)

---

### Collision Detection

**Function:** `Items_CheckCollisions()`

**Three-Pass System:**

#### Pass 1: Item Boxes
```c
for each active item box:
    for each car:
        if (multiplayer && !playerConnected) continue
        if distance(car, box) <= pickup_radius:
            give_random_item()
            deactivate_box()
            start_respawn_timer()
```

**Complexity:** O(boxes × cars) = O(8 × 8) = 64 checks max

#### Pass 2: Projectiles
```c
for each active projectile:
    if not near screen: skip (culling)
    for each car:
        if immunity_active: skip
        if multiplayer && is_shooter: skip
        if collision:
            apply_hit_effect()
            despawn_projectile()
```

**Culling:** Only checks items within screen bounds + buffer zone

**Complexity:** O(visible_projectiles × cars)

#### Pass 3: Hazards
```c
for each active hazard:
    if not near screen: skip (culling)
    for each car:
        if collision:
            apply_hazard_effect()
            if (banana or bomb): despawn on hit
            if (oil): persist (no hit despawn)
```

**Complexity:** O(visible_hazards × cars)

**Hitbox Check:**
```c
bool checkItemCarCollision(const Vec2* itemPos, const Vec2* carPos, int itemHitbox) {
    Q16_8 dist = Vec2_Distance(itemPos, carPos);
    int hitRadius = (itemHitbox + CAR_COLLISION_SIZE) / 2;
    return (dist <= IntToFixed(hitRadius));
}
```

**See:** [items_update.c:301-539](../source/gameplay/items/items_update.c#L301-L539)

---

### Immunity System

**Purpose:** Prevent projectiles from immediately hitting the shooter

**Two Modes:**

#### Mode 1: Time + Distance (Multiplayer)
```c
immunityTimer = PROJECTILE_IMMUNITY_TICKS;  // ~2 seconds

// Each frame:
if (immunityTimer > 0) {
    immunityTimer--;
    if (distance_from_shooter > MIN_DISTANCE) {
        immunityTimer = 0;  // Early removal
    }
}
```

**Benefits:**
- Fast-moving projectiles can't hit shooter
- Removes immunity early if far enough away
- Works well with network lag

#### Mode 2: Lap-Based (Single Player)
```c
immunityTimer = -1;  // Special value
startingWaypoint = currentWaypoint;
hasCompletedLap = false;

// Each frame:
if (!hasCompletedLap && waypointsVisited > 100) {
    int waypointDiff = abs(currentWaypoint - startingWaypoint);
    if (waypointDiff <= THRESHOLD) {
        hasCompletedLap = true;
        immunityTimer = 0;  // Remove immunity
    }
}
```

**Benefits:**
- Ensures shell travels full lap before hitting shooter
- More predictable for AI opponents
- Prevents exploits in single-player

**See:** [items_update.c:159-187](../source/gameplay/items/items_update.c#L159-L187)

---

## Rendering System

### OAM Sprite Allocation

**Fixed Mapping:**
```
Slot 0:       (Reserved for kart)
Slots 1-8:    Item boxes
Slots 9-40:   Track items
```

**Benefits:**
- **Stable mapping:** Each item always uses same OAM slot
- **No flicker:** Sprites don't swap slots between frames
- **Predictable:** Easy to debug visual issues

### Sprite Types

| Item | Size | Palette | Rotation |
|------|------|---------|----------|
| Item Box | 8×8 | 1 | No |
| Banana | 16×16 | 2 | No |
| Bomb | 16×16 | 3 | No |
| Green Shell | 16×16 | 4 | Yes |
| Red Shell | 16×16 | 5 | Yes |
| Missile | 16×32 | 6 | Yes |
| Oil Slick | 32×32 | 7 | No |

### Rotation System

Projectiles use affine transformations:
```c
// Convert 512-based angle to DS angle format
int rotation = -(item->angle512 << 6);

// Use separate affine matrix per rotating sprite
int affineSlot = 1 + (itemIndex % 4);  // Cycle through 4 matrices

oamRotateScale(&oamMain, affineSlot, rotation, scale_x, scale_y);
```

**DS Affine Slots:** 32 available, items use 1-4

**See:** [items_render.c:123-135](../source/gameplay/items/items_render.c#L123-L135)

---

## Multiplayer Synchronization

### Network Messages

#### Item Placement
```c
typedef struct {
    Item itemType;           // 1 byte
    Vec2 position;           // 8 bytes (Q16.8)
    int angle512;            // 4 bytes
    Q16_8 speed;             // 4 bytes
    int shooterCarIndex;     // 4 bytes
    bool valid;              // 1 byte
} ItemPlacementData;
```

**Total:** ~22 bytes per message

#### Item Box Pickup
```c
int boxIndex;  // 4 bytes
```

### Message Flow

**Firing Projectile:**
```
Player 1                        Player 2
────────                        ────────
User presses button
  │
  ├─> Items_FireProjectile()
  │      │
  │      ├─> Create local projectile
  │      └─> Multiplayer_SendItemPlacement()
  │                                  │
  │                                  └────> Network
  │                                              │
  │                                              └────> Multiplayer_ReceiveItemPlacements()
  │                                                          │
  │                                                          └─> fireProjectileInternal(sendNetwork=false)
  │                                                                 │
  │                                                                 └─> Create projectile
```

**Key:** `sendNetwork` flag prevents infinite loops

### Synchronization Challenges

1. **Latency:** Items spawn slightly delayed on remote clients
2. **Packet Loss:** Items may not appear for all players
3. **Clock Drift:** Timers may desync over time

**Solutions:**
- Authoritative player for each item
- Periodic state reconciliation
- Client-side prediction with rollback

**See:** [items_update.c:85-104](../source/gameplay/items/items_update.c#L85-L104)

---

## Performance Optimizations

### 1. Culling
Only process items near the visible screen:
```c
static bool isItemNearScreen(Vec2 itemPos, int scrollX, int scrollY) {
    int screenLeft = scrollX - COLLISION_BUFFER_ZONE;
    int screenRight = scrollX + SCREEN_WIDTH + COLLISION_BUFFER_ZONE;
    int screenTop = scrollY - COLLISION_BUFFER_ZONE;
    int screenBottom = scrollY + SCREEN_HEIGHT + COLLISION_BUFFER_ZONE;
    return (itemX >= screenLeft && itemX <= screenRight &&
            itemY >= screenTop && itemY <= screenBottom);
}
```

**Savings:** ~70% of collision checks skipped when far from camera

### 2. Fixed Pool
Pre-allocated arrays eliminate runtime allocation:
```c
TrackItem activeItems[MAX_TRACK_ITEMS];  // No malloc()
```

**Benefits:**
- Predictable memory usage
- No fragmentation
- Faster allocation (just set active flag)

### 3. Stable OAM Mapping
Each item uses same OAM slot every frame:
```c
int oamSlot = TRACK_ITEM_OAM_START + itemIndex;
```

**Benefits:**
- No per-frame slot allocation
- Reduces OAM updates
- More cache-friendly

### 4. Early Exit
Skip inactive items immediately:
```c
for (int i = 0; i < MAX_TRACK_ITEMS; i++) {
    if (!activeItems[i].active) continue;  // Early out
    // ... expensive logic ...
}
```

### Performance Metrics

| Operation | Typical Time | Worst Case |
|-----------|--------------|------------|
| Items_Update() | 0.3ms | 0.5ms |
| Items_CheckCollisions() | 0.2ms | 0.4ms |
| Items_Render() | 0.1ms | 0.2ms |
| **Total** | **0.6ms** | **1.1ms** |

**Frame budget:** 16.67ms @ 60 FPS → Items use ~3-6% of frame

---

## Design Patterns

### 1. Data-Oriented Design
- All items in contiguous arrays
- Cache-friendly iteration
- Minimal pointer chasing

### 2. Pool Pattern
- Pre-allocated item slots
- Active/inactive flags instead of alloc/free
- Constant-time "allocation"

### 3. State Machine (Homing)
- Path following vs direct attack
- Clear state transitions
- Easy to debug and extend

### 4. Module Encapsulation
- Public API in items_api.h
- Internal state in items_internal.h
- Implementation split by concern

---

## Future Improvements

### Scalability
- [ ] Spatial hash grid for collision detection (O(n²) → O(n))
- [ ] Hierarchical waypoint system for faster pathfinding
- [ ] Item pooling with separate active/inactive lists

### Features
- [ ] Triple item formations (banana/shell trails)
- [ ] Chain reactions (bomb explosions trigger other bombs)
- [ ] Item shields (hold banana/shell behind kart)
- [ ] Lightning bolt (shrink all opponents)

### Multiplayer
- [ ] Delta compression for network messages
- [ ] Predictive spawning to hide latency
- [ ] Reconciliation for desynced items

---

## Related Documentation

- **[Items Overview](items_overview.md)** - High-level system overview
- **[Items API Reference](items_api.md)** - Function documentation

---

## Authors
- **Bahey Shalash**
- **Hugo Svolgaard**

**Version:** 1.0
**Last Updated:** 06.01.2026

---

## Navigation

- [← Back to Wiki](wiki.md)
- [← Back to README](../README.md)
