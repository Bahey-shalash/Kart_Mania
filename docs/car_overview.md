# Car System Overview

## Introduction

The Car system implements the physics and state management for racing karts in Kart Mania. It provides a simplified scalar speed + angle representation for tight, responsive controls suitable for the Nintendo DS hardware. The system handles acceleration, braking, steering, friction, and position updates at 60 FPS.

---

## Quick Facts

- **Location:** `source/gameplay/Car.h` and `source/gameplay/Car.c`
- **Physics Model:** Scalar speed (magnitude) + angle512 (direction)
- **Update Frequency:** 60 Hz (once per frame)
- **Coordinate System:** Q16.8 fixed-point (see [Fixed-Point Math](fixedmath.md))
- **Angle System:** 512-based (see [Fixed-Point Math - Angles](fixedmath.md#angles))
- **Max Car Name Length:** 31 characters + null terminator

---

## Physics Model

### Scalar Speed + Angle Representation

The Car system uses a simplified physics model where:
- **Speed** is a scalar magnitude (how fast)
- **Angle512** is the facing direction (which way)
- Movement direction **always follows** the facing angle

This differs from full 2D vector physics and provides:
- Tighter, more responsive controls
- Simpler state management
- Better performance on DS hardware
- Easier to implement power sliding and drifting

### Key Properties

For details on Q16.8 fixed-point format and angle representation, see [Fixed-Point Math](fixedmath.md).

| Property | Type | Description | Range |
|----------|------|-------------|-------|
| **position** | Vec2 (Q16.8) | World coordinates | Unlimited |
| **speed** | Q16.8 | Movement magnitude | 0 to maxSpeed |
| **angle512** | int | Facing direction | 0-511 (wraps) |
| **maxSpeed** | Q16.8 | Speed cap | Set per car |
| **accelRate** | Q16.8 | Acceleration/braking step | Set per car |
| **friction** | Q16.8 | Speed decay per frame | 0-256 (0-100%) |

### Physics Constants

```c
#define MIN_SPEED_THRESHOLD 5      // Speed below this snaps to 0
#define MIN_MOVING_SPEED 25        // Speed above this = "moving"
#define SPEED_50CC (FIXED_ONE * 3) // Example: 3.0 px/frame
#define FRICTION_50CC 240          // Example: 0.9375 multiplier
```

---

## Car Structure

```c
typedef struct Car {
    Vec2 position;       // World position (Q16.8)
    Q16_8 speed;         // Current speed magnitude
    Q16_8 maxSpeed;      // Maximum allowed speed
    Q16_8 accelRate;     // Acceleration/braking step
    Q16_8 friction;      // Speed multiplier per frame
    int angle512;        // Facing direction (0-511)
    int Lap;             // Current lap number
    int rank;            // Race position (1st, 2nd, etc.)
    int lastCheckpoint;  // Last checkpoint crossed (-1 = none)
    Item item;           // Currently held item
    char carname[32];    // Car name string
    u16* gfx;            // Sprite graphics pointer
} Car;
```

**Memory Size:** ~80 bytes per car

---

## Core Operations

### 1. Acceleration

**Function:** `Car_Accelerate(Car* car)`

Increases speed by `accelRate` in the current facing direction:

```
speed += accelRate
if (speed > maxSpeed):
    speed = maxSpeed
```

**Behavior:**
- Works even when car is stopped
- Always accelerates in facing direction
- Speed is capped at maxSpeed

### 2. Braking

**Function:** `Car_Brake(Car* car)`

Decreases speed by `accelRate`:

```
if (speed <= 0):
    return  // Already stopped
if (accelRate >= speed):
    speed = 0  // Stop completely
else:
    speed -= accelRate
```

**Behavior:**
- Cannot go negative (no reverse)
- Stops cleanly when speed would overshoot zero

### 3. Steering

**Function:** `Car_Steer(Car* car, int deltaAngle512)`

Rotates the facing angle:

```
angle512 = (angle512 + deltaAngle512) & ANGLE_MASK
```

**Behavior:**
- Works even when stopped
- Wraps around (511 → 0)
- Movement direction follows immediately

### 4. Update (Physics Integration)

**Function:** `Car_Update(Car* car)`

Called once per frame (60 Hz):

```
1. Apply friction: speed *= friction
2. Snap tiny speeds to 0 (prevent drift)
3. Cap speed to maxSpeed (safety)
4. Build velocity vector from speed + angle
5. Integrate: position += velocity
```

**Frame Budget:** ~0.1ms typical

---

## Update Loop Pattern

```c
// Every frame (60 FPS)
void gameLoop() {
    // 1. Process input
    if (buttonHeld(KEY_A)) {
        Car_Accelerate(&player);
    }
    if (buttonHeld(KEY_B)) {
        Car_Brake(&player);
    }
    if (buttonHeld(KEY_LEFT)) {
        Car_Steer(&player, -TURN_STEP_50CC);
    }
    if (buttonHeld(KEY_RIGHT)) {
        Car_Steer(&player, TURN_STEP_50CC);
    }

    // 2. Update physics
    Car_Update(&player);

    // 3. Check collisions, apply terrain effects, etc.
    // 4. Render
}
```

---

## Special Operations

### Position Manipulation

```c
// Respawn at checkpoint
Car_SetPosition(&player, checkpointPos);
Car_SetAngle(&player, ANGLE_UP);
```

### Velocity-Based Effects

```c
// Speed boost item (doubles max speed temporarily)
Vec2 currentVel = buildVelocityFromCar(&player);
Vec2 boostedVel = Vec2_Scale(currentVel, IntToFixed(2));
Car_SetVelocity(&player, boostedVel);

// Collision knockback
Vec2 knockback = Vec2_FromAngle(impactAngle);
knockback = Vec2_Scale(knockback, IntToFixed(5));
Car_ApplyImpulse(&player, knockback);
```

**Note:** Speed is always capped to `maxSpeed` in these operations.

---

## Lifecycle Management

### Initialization

```c
Car player;
Car_Init(&player,
         startPos,           // Spawn position
         "Player 1",         // Car name
         SPEED_50CC,         // Max speed
         ACCEL_50CC,         // Accel rate
         FRICTION_50CC);     // Friction
```

**Sets:**
- Position to startPos
- Speed to 0
- Angle to 0 (facing right/east)
- Lap, rank, lastCheckpoint to initial values
- Item to ITEM_NONE

### Reset (Mid-Race Respawn)

```c
Car_Reset(&player, respawnPos);
```

**Preserves:**
- maxSpeed, accelRate, friction (car properties)
- carname

**Resets:**
- Position to respawnPos
- Speed to 0
- Angle to 0
- Lap, rank, lastCheckpoint, item

---

## Constructor Helpers

### CarCreate

Creates a fully specified car:

```c
Car player = CarCreate(
    startPos,        // position
    0,               // initial speed
    SPEED_50CC,      // maxSpeed
    ACCEL_50CC,      // accelRate
    FRICTION_50CC,   // friction
    ITEM_NONE,       // initial item
    "Player 1"       // name
);
```

### emptyCar

Creates a zeroed car:

```c
Car cpu = emptyCar("CPU 1");
// All physics values = 0
// Must set maxSpeed, accelRate, friction later
```

---

## Read-Only Queries

```c
int angle = Car_GetAngle(&car);              // Facing direction (for sprite)
int velAngle = Car_GetVelocityAngle(&car);   // Movement direction (same as facing)
bool moving = Car_IsMoving(&car);            // Speed > MIN_MOVING_SPEED?
Q16_8 speed = Car_GetSpeed(&car);            // Current speed magnitude
```

---

## Ownership and Access Rules

**Ownership:**
- Cars are owned by `RaceState` in `gameplay_logic.c`
- Managed as an array of cars (player + opponents)

**Access Rules:**
- **Read:** Direct member access allowed (`car->position.x`, `car->speed`)
- **Modify:** Use `Car_*` functions to maintain invariants

**Concurrency:**
- Single-threaded only (DS hardware limitation)
- No mutex/locking needed

---

## Integration with Other Systems

### Items System

```c
// Item effects modify car state
if (player.item == ITEM_SPEEDBOOST) {
    Items_ApplySpeedBoost(&player, effects);
}

// Items affect player velocity
if (hitByShell) {
    Vec2 spinImpulse = Vec2_FromAngle(shellAngle);
    spinImpulse = Vec2_Scale(spinImpulse, IntToFixed(3));
    Car_ApplyImpulse(&player, spinImpulse);
}
```

### Terrain Effects

```c
// Sand reduces friction and caps max speed
if (onSandTerrain) {
    player.friction = SAND_FRICTION;  // 150/256 = 0.586
    if (player.speed > SAND_MAX_SPEED) {
        player.speed = SAND_MAX_SPEED;
    }
}
```

### Collision Detection

```c
// Car hitbox for item collisions
#define CAR_COLLISION_SIZE 32

bool checkCarItemCollision(Car* car, Vec2 itemPos, int itemHitbox) {
    int combinedRadius = (CAR_COLLISION_SIZE + itemHitbox) / 2;
    Q16_8 dist = Vec2_Distance(car->position, itemPos);
    return (dist <= IntToFixed(combinedRadius));
}
```

### Lap Tracking

```c
// Increment lap counter
if (crossedFinishLine) {
    Car_LapComplete(&player);
}
```

---

## Performance Considerations

### Memory Usage

```c
Car player;                  // ~80 bytes
Car opponents[7];            // ~560 bytes
// Total: ~640 bytes for 8 cars
```

### Frame Budget

**Per car per frame:**
- Input processing: ~0.01ms
- Physics update: ~0.05ms
- Collision checks: ~0.1ms (depends on active items)
- Total: ~0.16ms per car

**For 8 cars:** ~1.3ms total (safe for 60 FPS)

### Optimization Tips

1. **Minimize friction clamping:** Clamp once at init, not every frame (current implementation could be optimized)
2. **Batch position updates:** Update all cars before rendering
3. **Spatial culling:** Only process cars near camera
4. **Fixed-point math:** All calculations use Q16.8 (no float operations)

---

## Common Patterns

### Creating a Player Car

```c
Car player;
Car_Init(&player,
         Vec2_Create(IntToFixed(904), IntToFixed(580)),
         "Player",
         SPEED_50CC,
         ACCEL_50CC,
         FRICTION_50CC);

player.angle512 = ANGLE_UP;  // Face north at start
```

### Creating CPU Opponents

```c
Car opponents[7];
for (int i = 0; i < 7; i++) {
    char name[32];
    sprintf(name, "CPU %d", i + 1);

    Vec2 spawnPos = getStartingPosition(i + 1);  // Grid positions

    Car_Init(&opponents[i], spawnPos, name,
             SPEED_50CC, ACCEL_50CC, FRICTION_50CC);

    opponents[i].angle512 = ANGLE_UP;
}
```

### Handling Collisions with Walls

```c
// Snap to edge and stop
if (wallCollision) {
    Car_SetPosition(&car, validPosition);
    car.speed = 0;
}
```

### Implementing Boost Pads

```c
if (onBoostPad) {
    Q16_8 boostedSpeed = FixedMul(car.maxSpeed, IntToFixed(2));
    if (car.speed < boostedSpeed) {
        car.speed = boostedSpeed;  // Instant boost
    }
}
```

---

## Related Documentation

- **[Car API Reference](car_api.md)** - Detailed function documentation
- **[Items System](items_overview.md)** - Item effects on cars
- **[Fixed-Point Math](fixedmath.md)** - Q16.8 arithmetic system
- **[Game Constants](../source/core/game_constants.h)** - Physics tuning values

---

## Future Enhancements

### Planned Features
- [ ] Separate drifting state (power sliding)
- [ ] Air state for jumps
- [ ] Water physics (reduced control)
- [ ] Reverse gear for stuck players
- [ ] Car-specific stats (light vs heavy karts)

### Potential Improvements
- [ ] Optimize friction clamping (only on friction changes)
- [ ] Add rotation interpolation for smoother visuals
- [ ] Implement skid marks/particle effects
- [ ] Add suspension for terrain bumps

---

## Debugging

### Common Issues

**Problem:** Car doesn't move when accelerating
- Check: Is `maxSpeed > 0`?
- Check: Is `accelRate > 0`?
- Check: Is friction reasonable (not 0)?

**Problem:** Car slides forever after stopping input
- Check: Is friction applied every frame?
- Check: Is `MIN_SPEED_THRESHOLD` being used to snap to 0?

**Problem:** Steering doesn't affect movement
- Check: Is `Car_Update()` being called after `Car_Steer()`?
- Check: Is angle being masked correctly (`& ANGLE_MASK`)?

**Problem:** Speed exceeds maxSpeed
- Check: Is `Car_Update()` capping speed?
- Check: Are external systems modifying `car.speed` directly?

### Debug Visualization

```c
// Print car state
printf("Pos: (%d, %d) Speed: %d Angle: %d\n",
       FixedToInt(car.position.x),
       FixedToInt(car.position.y),
       FixedToInt(car.speed),
       car.angle512);
```

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
