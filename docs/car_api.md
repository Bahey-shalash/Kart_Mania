# Car API Reference

## Overview

This document provides detailed documentation for all public functions in the Car system. For a high-level overview, see [Car System Overview](car_overview.md).

---

## Table of Contents

1. [Lifecycle Management](#lifecycle-management)
2. [Physics Control](#physics-control)
3. [Read-Only Queries](#read-only-queries)
4. [Special Operations](#special-operations)
5. [Game Events](#game-events)
6. [Constructor Helpers](#constructor-helpers)

---

## Lifecycle Management

### Car_Init

```c
void Car_Init(Car* car, Vec2 pos, const char* name, Q16_8 maxSpeed,
              Q16_8 accelRate, Q16_8 friction);
```

**Description:** Initializes a car with starting position, name, and physics parameters. Sets all race state to default values.

**Parameters:**
- `car` - Pointer to car structure to initialize
- `pos` - Starting position ([Q16.8 fixed-point](fixedmath.md))
- `name` - Car name string (null-terminated, max 31 chars)
- `maxSpeed` - Maximum speed ([Q16.8 fixed-point](fixedmath.md))
- `accelRate` - Acceleration/braking rate ([Q16.8 fixed-point](fixedmath.md))
- `friction` - Friction multiplier (0-256, where 256 = 100%)

**Behavior:**
1. Sets position to `pos`
2. Sets speed to 0
3. Sets angle512 to 0 (facing right/east)
4. Sets maxSpeed, accelRate, friction (clamped to [0, FIXED_ONE])
5. Sets Lap to 0, rank to 0, lastCheckpoint to -1
6. Sets item to ITEM_NONE
7. Copies name to carname (with bounds checking)

**When to call:** When creating a new car at race start or when loading a car profile.

**Example:**
```c
Car player;
Car_Init(&player,
         Vec2_Create(IntToFixed(904), IntToFixed(580)),
         "Lightning",
         SPEED_50CC,
         ACCEL_50CC,
         FRICTION_50CC);
```

**See:** [Car.c:114-131](../source/gameplay/Car.c#L114-L131)

---

### Car_Reset

```c
void Car_Reset(Car* car, Vec2 spawnPos);
```

**Description:** Resets car to spawn position with zeroed race state. Preserves physics parameters (maxSpeed, accelRate, friction) and car name.

**Parameters:**
- `car` - Pointer to car to reset
- `spawnPos` - Respawn position ([Q16.8 fixed-point](fixedmath.md))

**Behavior:**
1. Sets position to `spawnPos`
2. Sets speed to 0
3. Sets angle512 to 0 (facing right/east)
4. Sets Lap to 0, rank to 0, lastCheckpoint to -1
5. Sets item to ITEM_NONE
6. **Preserves:** maxSpeed, accelRate, friction, carname

**When to call:** Mid-race respawns (after falling off track, stuck, etc.)

**Example:**
```c
// Respawn at last checkpoint
if (fellOffTrack) {
    Car_Reset(&player, lastCheckpointPos);
    Car_SetAngle(&player, checkpointFacingAngle);
}
```

**See:** [Car.c:139-152](../source/gameplay/Car.c#L139-L152)

---

## Physics Control

### Car_Accelerate

```c
void Car_Accelerate(Car* car);
```

**Description:** Increases car speed by accelRate in the current facing direction. Speed is capped at maxSpeed.

**Parameters:**
- `car` - Pointer to car to accelerate

**Behavior:**
1. Adds `accelRate` to `speed`
2. Caps speed to `maxSpeed` if exceeded
3. Works even when car is stopped

**Frame rate:** Call every frame while accelerating (typically 60 FPS)

**Example:**
```c
// In game loop
if (keysHeld() & KEY_A) {
    Car_Accelerate(&player);
}
```

**See:** [Car.c:163-174](../source/gameplay/Car.c#L163-L174)

---

### Car_Brake

```c
void Car_Brake(Car* car);
```

**Description:** Decreases car speed by accelRate. Speed cannot go negative (no reverse).

**Parameters:**
- `car` - Pointer to car to brake

**Behavior:**
1. If speed <= 0, returns immediately (already stopped)
2. If accelRate >= speed, sets speed to 0 (clean stop)
3. Otherwise subtracts accelRate from speed

**Frame rate:** Call every frame while braking (typically 60 FPS)

**Example:**
```c
// In game loop
if (keysHeld() & KEY_B) {
    Car_Brake(&player);
}
```

**See:** [Car.c:181-196](../source/gameplay/Car.c#L181-L196)

---

### Car_Steer

```c
void Car_Steer(Car* car, int deltaAngle512);
```

**Description:** Rotates the car's facing angle. Movement direction follows the facing angle since speed is scalar.

**Parameters:**
- `car` - Pointer to car to steer
- `deltaAngle512` - Angle change in [512-based system](fixedmath.md#angles) (positive = clockwise)

**Behavior:**
1. Adds `deltaAngle512` to `angle512`
2. Wraps angle using `ANGLE_MASK` (0-511)
3. Works even when stopped
4. Movement direction changes immediately

**Frame rate:** Call every frame while steering (typically 60 FPS)

**Important:** ALWAYS use this function for steering! Never modify `angle512` directly unless spawning/respawning.

**Example:**
```c
// In game loop
if (keysHeld() & KEY_LEFT) {
    Car_Steer(&player, -TURN_STEP_50CC);  // Turn left
}
if (keysHeld() & KEY_RIGHT) {
    Car_Steer(&player, TURN_STEP_50CC);   // Turn right
}
```

**See:** [Car.c:204-211](../source/gameplay/Car.c#L204-L211)

---

### Car_Update

```c
void Car_Update(Car* car);
```

**Description:** Updates car physics for one frame. Applies friction, snaps low speeds to zero, and integrates velocity into position.

**Parameters:**
- `car` - Pointer to car to update

**Behavior:**
1. Clamps friction to [0, FIXED_ONE]
2. Applies friction: `speed = speed * friction`
3. Snaps speed to 0 if below `MIN_SPEED_THRESHOLD`
4. Caps speed to maxSpeed (safety net)
5. Builds velocity vector from speed + angle512
6. Integrates position: `position += velocity`

**Frame rate:** Call once per physics tick (60 Hz)

**Call order:** After all input processing (accelerate, brake, steer) but before rendering

**Example:**
```c
// Game loop structure
void updatePhysics() {
    // 1. Process input for all cars
    handlePlayerInput(&player);
    updateAI(opponents, 7);

    // 2. Update physics for all cars
    Car_Update(&player);
    for (int i = 0; i < 7; i++) {
        Car_Update(&opponents[i]);
    }

    // 3. Handle collisions, terrain, etc.
    checkCollisions();
}
```

**See:** [Car.c:219-241](../source/gameplay/Car.c#L219-L241)

---

## Read-Only Queries

### Car_GetAngle

```c
int Car_GetAngle(const Car* car);
```

**Description:** Returns the car's facing angle for sprite rotation.

**Parameters:**
- `car` - Pointer to car (const - not modified)

**Returns:**
- Facing angle in [512-based system](fixedmath.md#angles) (0-511)
- Returns 0 if car is NULL

**Use case:** Rotating car sprite to match facing direction

**Example:**
```c
int spriteRotation = Car_GetAngle(&player);
oamSet(&oamMain, 0, playerX, playerY, 0, 0,
       SpriteSize_32x32, SpriteColorFormat_256Color,
       gfx, -1, false, false, false, false, false);
oamSetAffineIndex(&oamMain, 0, spriteRotation, false);
```

**See:** [Car.c:252-257](../source/gameplay/Car.c#L252-L257)

---

### Car_GetVelocityAngle

```c
int Car_GetVelocityAngle(const Car* car);
```

**Description:** Returns the car's movement direction. With scalar speed + angle physics, this always matches the facing angle when moving.

**Parameters:**
- `car` - Pointer to car (const - not modified)

**Returns:**
- Movement angle in [512-based system](fixedmath.md#angles) (0-511)
- Returns 0 if car is NULL

**Use case:** Debugging, particle effects that follow movement direction

**Note:** In this physics model, velocity angle == facing angle. This function exists for API completeness and potential future enhancements (e.g., drift mechanics where facing ≠ movement).

**Example:**
```c
// Spawn exhaust particles behind car
int moveAngle = Car_GetVelocityAngle(&player);
int backwardAngle = (moveAngle + ANGLE_HALF) & ANGLE_MASK;
Vec2 exhaustPos = Vec2_Add(player.position,
                            Vec2_FromAngle(backwardAngle));
spawnParticle(exhaustPos);
```

**See:** [Car.c:265-270](../source/gameplay/Car.c#L265-L270)

---

### Car_IsMoving

```c
bool Car_IsMoving(const Car* car);
```

**Description:** Checks if car speed is above the movement threshold.

**Parameters:**
- `car` - Pointer to car (const - not modified)

**Returns:**
- `true` if speed > MIN_MOVING_SPEED (25 in Q16.8)
- `false` if stopped or nearly stopped
- `false` if car is NULL

**Use case:** Conditional logic for animations, sound effects, physics

**Example:**
```c
// Only check terrain collisions if moving
if (Car_IsMoving(&player)) {
    checkTerrainEffects(&player);
}

// Engine sound based on movement
if (Car_IsMoving(&player)) {
    playEngineSound(player.speed);
} else {
    stopEngineSound();
}
```

**See:** [Car.c:277-282](../source/gameplay/Car.c#L277-L282)

---

### Car_GetSpeed

```c
Q16_8 Car_GetSpeed(const Car* car);
```

**Description:** Returns the car's current speed magnitude.

**Parameters:**
- `car` - Pointer to car (const - not modified)

**Returns:**
- Speed in [Q16.8 fixed-point](fixedmath.md)
- Returns 0 if car is NULL

**Use case:** UI speedometer, AI decision making, physics calculations

**Example:**
```c
// Display speed in UI
Q16_8 speed = Car_GetSpeed(&player);
int speedInt = FixedToInt(speed);
printf("Speed: %d\n", speedInt);

// AI: Brake before sharp turn
if (Car_GetSpeed(&cpu) > TURN_SAFE_SPEED && sharpTurnAhead) {
    Car_Brake(&cpu);
}
```

**See:** [Car.c:289-294](../source/gameplay/Car.c#L289-L294)

---

## Special Operations

### Car_SetPosition

```c
void Car_SetPosition(Car* car, Vec2 pos);
```

**Description:** Directly sets car position. Use for respawn/teleport effects.

**Parameters:**
- `car` - Pointer to car
- `pos` - New position ([Q16.8 fixed-point](fixedmath.md))

**Behavior:**
- Sets `car->position = pos`
- Does not affect speed, angle, or other state

**When to call:**
- Respawning at checkpoints
- Teleporting to start line
- Fixing stuck cars

**Example:**
```c
// Respawn at checkpoint
Car_SetPosition(&player, checkpointPos);
Car_SetAngle(&player, ANGLE_UP);
player.speed = 0;
```

**See:** [Car.c:305-310](../source/gameplay/Car.c#L305-L310)

---

### Car_SetVelocity

```c
void Car_SetVelocity(Car* car, Vec2 velocity);
```

**Description:** Sets car speed and direction from a velocity vector. Use for boosts, collisions, and hazard effects. Speed is capped to maxSpeed.

**Parameters:**
- `car` - Pointer to car
- `velocity` - New velocity vector ([Q16.8 fixed-point](fixedmath.md))

**Behavior:**
1. If velocity is zero, sets speed to 0
2. Otherwise:
   - Calculates magnitude: `speed = Vec2_Len(velocity)`
   - Calculates direction: `angle512 = Vec2_ToAngle(velocity)`
   - Caps speed to maxSpeed

**When to call:**
- Item effects (speed boosts)
- Wall collisions
- Hazard effects (oil slick, banana)

**Use with caution:** Prefer `Car_Accelerate`, `Car_Brake`, `Car_Steer` for normal control. This is for external forces.

**Example:**
```c
// Speed boost doubles current velocity
Vec2 currentVel = Vec2_FromAngle(player.angle512);
currentVel = Vec2_Scale(currentVel, player.speed);
Vec2 boostedVel = Vec2_Scale(currentVel, IntToFixed(2));
Car_SetVelocity(&player, boostedVel);
```

**See:** [Car.c:318-323](../source/gameplay/Car.c#L318-L323)

---

### Car_ApplyImpulse

```c
void Car_ApplyImpulse(Car* car, Vec2 impulse);
```

**Description:** Applies an instant velocity change to the car. Useful for collision responses and item effects. Speed is capped to maxSpeed.

**Parameters:**
- `car` - Pointer to car
- `impulse` - Velocity impulse to add ([Q16.8 fixed-point](fixedmath.md))

**Behavior:**
1. Builds current velocity from speed + angle
2. Adds impulse: `newVelocity = currentVelocity + impulse`
3. Converts back to speed/angle representation
4. Caps speed to maxSpeed

**When to call:**
- Collision knockback
- Shell hits
- Bomb explosions

**Example:**
```c
// Shell hit - spin the car
int shellAngle = Vec2_ToAngle(Vec2_Sub(player.position, shellPos));
Vec2 knockback = Vec2_FromAngle(shellAngle);
knockback = Vec2_Scale(knockback, IntToFixed(5));
Car_ApplyImpulse(&player, knockback);

// Bomb explosion - radial knockback
Vec2 toPlayer = Vec2_Sub(player.position, bombPos);
Q16_8 dist = Vec2_Len(toPlayer);
if (dist < BOMB_EXPLOSION_RADIUS) {
    Vec2 knockback = Vec2_Normalize(toPlayer);
    knockback = Vec2_Scale(knockback, BOMB_KNOCKBACK_IMPULSE);
    Car_ApplyImpulse(&player, knockback);
}
```

**See:** [Car.c:331-338](../source/gameplay/Car.c#L331-L338)

---

### Car_SetAngle

```c
void Car_SetAngle(Car* car, int angle512);
```

**Description:** Directly sets car facing angle. Movement direction will follow this angle on the next update. Use for spawning and respawn orientation.

**Parameters:**
- `car` - Pointer to car
- `angle512` - New facing angle ([512-based system](fixedmath.md#angles), 0-511)

**Behavior:**
- Sets `angle512 = angle512 & ANGLE_MASK`
- Wraps to 0-511 range
- Movement direction follows on next `Car_Update()`

**WARNING:** For normal steering, ALWAYS use `Car_Steer()` instead! This function is for spawning/respawning only.

**When to call:**
- Spawning cars at race start
- Respawning at checkpoints
- Setting initial orientation

**Example:**
```c
// Spawn cars facing north at start
for (int i = 0; i < 8; i++) {
    Car_Init(&cars[i], startPositions[i], names[i],
             SPEED_50CC, ACCEL_50CC, FRICTION_50CC);
    Car_SetAngle(&cars[i], ANGLE_UP);
}

// Respawn facing checkpoint direction
Car_Reset(&player, checkpointPos);
Car_SetAngle(&player, checkpointAngle);
```

**See:** [Car.c:346-351](../source/gameplay/Car.c#L346-L351)

---

## Game Events

### Car_LapComplete

```c
void Car_LapComplete(Car* car);
```

**Description:** Increments the car's lap counter when a lap is completed.

**Parameters:**
- `car` - Pointer to car

**Behavior:**
- Increments `car->Lap`

**When to call:**
- When car crosses finish line
- After validating checkpoint progression

**Example:**
```c
// Check finish line crossing
if (crossedFinishLine(&player) &&
    player.lastCheckpoint == FINAL_CHECKPOINT) {
    Car_LapComplete(&player);

    if (player.Lap >= LAPS_SCORCHING_SANDS) {
        raceComplete();
    }
}
```

**See:** [Car.c:362-367](../source/gameplay/Car.c#L362-L367)

---

## Constructor Helpers

### CarCreate

```c
static inline Car CarCreate(Vec2 pos, Q16_8 speed, Q16_8 SpeedMax,
                            Q16_8 accel_rate, Q16_8 frictionn,
                            Item init_item, const char* name);
```

**Description:** Creates a new car with specified physics parameters and initial state. Inline helper defined in Car.h.

**Parameters:**
- `pos` - Initial position ([Q16.8 fixed-point](fixedmath.md))
- `speed` - Initial speed magnitude ([Q16.8](fixedmath.md))
- `SpeedMax` - Maximum speed ([Q16.8](fixedmath.md))
- `accel_rate` - Acceleration/braking rate ([Q16.8](fixedmath.md))
- `frictionn` - Friction multiplier (0-256)
- `init_item` - Initial item in inventory
- `name` - Car name string (null-terminated)

**Returns:**
- Initialized Car structure (by value)

**Behavior:**
1. Creates Car with all specified values
2. Copies name with bounds checking (max 31 chars)
3. Sets angle512 to 0, Lap to 0, rank to 0, lastCheckpoint to -1

**When to call:** When you want to create a car with specific initial values (including non-zero speed or specific item).

**Example:**
```c
// Create player car with starting boost
Car player = CarCreate(
    startPos,
    SPEED_50CC / 2,  // Start at half speed
    SPEED_50CC,
    ACCEL_50CC,
    FRICTION_50CC,
    ITEM_SPEEDBOOST, // Start with speed boost
    "Lightning"
);
```

**See:** [Car.h:94-114](../source/gameplay/Car.h#L94-L114)

---

### emptyCar

```c
static inline Car emptyCar(const char* name);
```

**Description:** Creates a car with all physics values initialized to zero. Inline helper defined in Car.h.

**Parameters:**
- `name` - Car name string (null-terminated)

**Returns:**
- Car structure with zeroed physics state (by value)

**Behavior:**
1. Creates Car with all values set to 0
2. Copies name with bounds checking
3. Must set maxSpeed, accelRate, friction later before use

**When to call:** When you want to create a placeholder car or set physics values separately.

**Example:**
```c
// Create CPU opponent, set physics later
Car cpu = emptyCar("CPU 1");
cpu.maxSpeed = SPEED_50CC;
cpu.accelRate = ACCEL_50CC;
cpu.friction = FRICTION_50CC;

Vec2 spawnPos = getAIStartPosition(1);
Car_SetPosition(&cpu, spawnPos);
Car_SetAngle(&cpu, ANGLE_UP);
```

**See:** [Car.h:127-145](../source/gameplay/Car.h#L127-L145)

---

## Usage Examples

### Complete Race Initialization

```c
// Initialize player
Car player;
Car_Init(&player,
         Vec2_Create(IntToFixed(904), IntToFixed(580)),
         "Player 1",
         SPEED_50CC,
         ACCEL_50CC,
         FRICTION_50CC);
Car_SetAngle(&player, ANGLE_UP);

// Initialize CPU opponents
Car opponents[7];
for (int i = 0; i < 7; i++) {
    char name[32];
    snprintf(name, 32, "CPU %d", i + 1);

    Vec2 spawnPos = getStartingGridPosition(i + 1);
    Car_Init(&opponents[i], spawnPos, name,
             SPEED_50CC, ACCEL_50CC, FRICTION_50CC);
    Car_SetAngle(&opponents[i], ANGLE_UP);
}
```

### Game Loop Integration

```c
void updateGame() {
    // 1. Process player input
    if (keysHeld() & KEY_A) Car_Accelerate(&player);
    if (keysHeld() & KEY_B) Car_Brake(&player);
    if (keysHeld() & KEY_LEFT) Car_Steer(&player, -TURN_STEP_50CC);
    if (keysHeld() & KEY_RIGHT) Car_Steer(&player, TURN_STEP_50CC);

    // 2. Update AI
    for (int i = 0; i < 7; i++) {
        updateAI(&opponents[i]);
    }

    // 3. Update physics for all cars
    Car_Update(&player);
    for (int i = 0; i < 7; i++) {
        Car_Update(&opponents[i]);
    }

    // 4. Handle collisions, items, terrain
    handleCollisions();
    checkItemBoxPickups();
    applyTerrainEffects();

    // 5. Update lap tracking
    updateLapTracking();

    // 6. Render
    renderRace();
}
```

### Terrain Effect Application

```c
void applyTerrainEffects(Car* car) {
    TerrainType terrain = getTerrainAt(car->position);

    switch (terrain) {
        case TERRAIN_SAND:
            car->friction = SAND_FRICTION;
            if (car->speed > SAND_MAX_SPEED) {
                car->speed = SAND_MAX_SPEED;
            }
            break;

        case TERRAIN_BOOST_PAD:
            if (car->speed < car->maxSpeed * 2) {
                car->speed = car->maxSpeed * 2;
            }
            break;

        case TERRAIN_ROAD:
        default:
            car->friction = FRICTION_50CC;
            break;
    }
}
```

---

## Related Documentation

- **[Car System Overview](car_overview.md)** - High-level system description
- **[Items System](items_overview.md)** - Item effects on cars
- **[Fixed-Point Math](fixedmath.md)** - Q16.8 arithmetic
- **[Game Constants](../source/core/game_constants.h)** - Physics tuning values

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
