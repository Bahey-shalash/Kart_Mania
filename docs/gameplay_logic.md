# Gameplay Logic Module Documentation

**File:** `source/gameplay/gameplay_logic.c` / `gameplay_logic.h`
**Authors:** Bahey Shalash, Hugo Svolgaard
**Version:** 1.0
**Date:** 06.01.2026

---

## Overview

The **Gameplay Logic** module is the core racing engine. It manages race state, car physics, countdown system, checkpoint progression, finish line detection, and multiplayer synchronization. This module handles all game **logic and physics**, while `gameplay.c` handles **rendering**.

### Key Responsibilities

- **Race State Management**: Track race status, lap counts, finish times
- **Car Physics**: Update car positions, velocities, acceleration
- **Player Input**: Process steering, acceleration, braking, item usage
- **Countdown System**: Manage pre-race countdown (3, 2, 1, GO!)
- **Checkpoint Progression**: Track player progress through lap sections
- **Finish Line Detection**: Detect lap completions and race completion
- **Terrain Effects**: Apply sand slowdown based on loaded quadrant
- **Wall Collision**: Bounce cars off walls and apply collision lockout
- **Item System Integration**: Handle item usage, collisions, and effects
- **Multiplayer Sync**: Send/receive car states over network (15 Hz)
- **Pause System**: Handle START button interrupt for pause/resume

---

## Architecture

### Race State Machine

```
┌─────────────────────────────────────────────────────────────┐
│                    RACE STATE FLOW                           │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  Race_Init()                                                 │
│       │                                                       │
│       ├─► COUNTDOWN_3   (1 second at 60fps)                 │
│       │                                                       │
│       ├─► COUNTDOWN_2   (1 second)                           │
│       │                                                       │
│       ├─► COUNTDOWN_1   (1 second)                           │
│       │                                                       │
│       ├─► COUNTDOWN_GO  (1 second)                           │
│       │                                                       │
│       └─► COUNTDOWN_FINISHED                                 │
│                  │                                            │
│                  └─► Race_Tick() starts                      │
│                           │                                   │
│                           ├─► Racing (physics updates)       │
│                           │                                   │
│                           └─► Finish line crossed            │
│                                   │                           │
│                                   ├─► Lap < totalLaps?       │
│                                   │   └─► Next lap           │
│                                   │                           │
│                                   └─► Final lap              │
│                                       └─► Race_MarkAsCompleted()
│                                           │                   │
│                                           └─► Wait 5 seconds │
│                                               └─► Race_Stop()│
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

### Checkpoint System

To prevent lap shortcuts, players must pass through checkpoints:

```
       START/FINISH (y=540)
              │
              │  CP_STATE_START
              ▼
       ┌──────────────┐
       │              │
       │   Quadrant   │   CP_STATE_NEED_LEFT
LEFT ──┤   Division   ├── RIGHT
(x<512)│   (x=512)    │   (x>512)
       │              │
       └──────────────┘
              │
              │  CP_STATE_NEED_DOWN
              ▼
       BOTTOM (y>512)
              │
              │  CP_STATE_NEED_RIGHT
              ▼
       RIGHT (x>512)
              │
              │  CP_STATE_READY_FOR_LAP
              ▼
       FINISH (y<540) = LAP COMPLETE!
```

**State Progression:**
1. `CP_STATE_START` → Player spawns above finish line
2. `CP_STATE_NEED_LEFT` → Must cross left of x=512
3. `CP_STATE_NEED_DOWN` → Must cross below y=512
4. `CP_STATE_NEED_RIGHT` → Must cross right of x=512
5. `CP_STATE_READY_FOR_LAP` → Can cross finish line for lap completion

---

## Public API

### Lifecycle Functions

#### `void Race_Init(Map map, GameMode mode)`

Initializes race state for a new race.

**Parameters:**
- `map` - Track to race on (`ScorchingSands`, `AlpinRush`, `NeonCircuit`)
- `mode` - `SinglePlayer` or `MultiPlayer`

**Setup:**
- Car spawns (single or multiplayer positioning)
- Lap count (map-specific or 5 laps for multiplayer)
- Countdown sequence (3, 2, 1, GO!)
- Items system via `Items_Init()`
- Pause interrupt via `Race_InitPauseInterrupt()`

**Spawn Positions:**
```
SinglePlayer:  Player at (904, 580) facing UP
MultiPlayer:   Players in 2 columns:
               Left column:  x=904
               Right column: x=936
               Spacing: 24px per player
```

---

#### `void Race_Tick(void)`

Main race logic update (called every frame at 60 Hz).

**Handles:**
- Player input (steering, acceleration, item usage)
- Car physics updates via `Car_Update()`
- Terrain effects (sand slowdown)
- Wall collision
- Checkpoint progression
- Item collisions and effects
- Multiplayer network sync (every 4 frames = 15 Hz)

**Called by:** Timer interrupt (configured in `timer.c`)

---

#### `void Race_Reset(void)`

Resets race to initial state (restart same race).

**Resets:**
- Car positions to spawn
- Timer state
- Countdown sequence
- Items on track

**Does NOT:**
- Change map or game mode
- Reset best times

---

#### `void Race_Stop(void)`

Stops current race and cleans up resources.

**Cleans up:**
- Pause interrupt via `Race_CleanupPauseInterrupt()`
- Race timer via `RaceTick_TimerStop()`
- Sets `raceStarted = false`

---

#### `void Race_MarkAsCompleted(int min, int sec, int msec)`

Marks race as completed and stores final time.

**Parameters:**
- `min` - Final minutes
- `sec` - Final seconds (0-59)
- `msec` - Final milliseconds (0-999)

**Actions:**
- Sets `raceFinished = true`
- Starts 5-second delay timer
- Stores final time
- Stops race timer interrupt

---

### Countdown System Functions

#### `void Race_UpdateCountdown(void)`

Updates countdown sequence (3 → 2 → 1 → GO → FINISHED).

**Called by:** `Gameplay_OnVBlank()` during countdown phase.

**Timing:** Each step lasts 60 frames (1 second at 60 Hz).

---

#### `bool Race_IsCountdownActive(void)`

Returns `true` if countdown is still active (not `COUNTDOWN_FINISHED`).

---

#### `CountdownState Race_GetCountdownState(void)`

Returns current countdown state.

**Enum Values:**
```c
typedef enum {
    COUNTDOWN_3 = 0,       // Display "3"
    COUNTDOWN_2,           // Display "2"
    COUNTDOWN_1,           // Display "1"
    COUNTDOWN_GO,          // Display "GO!"
    COUNTDOWN_FINISHED     // Race started, countdown done
} CountdownState;
```

---

#### `void Race_CountdownTick(void)`

Network sync during countdown (multiplayer only).

**Purpose:** Share spawn positions every 4 frames to ensure all players see synchronized countdown.

**Called by:** VBlank interrupt during countdown.

---

### State Query Functions

#### `const Car* Race_GetPlayerCar(void)`

Gets read-only pointer to player's car (local player).

**Single Player:** Always car index 0
**Multiplayer:** Car index from `Multiplayer_GetMyPlayerID()`

---

#### `const RaceState* Race_GetState(void)`

Gets read-only pointer to complete race state.

**RaceState Structure:**
```c
typedef struct {
    bool raceStarted;          // Race has been initialized
    bool raceFinished;         // Race completed (all laps done)

    GameMode gameMode;         // SinglePlayer or MultiPlayer
    Map currentMap;            // Current track being raced

    int carCount;              // 1 for single, up to 8 for multi
    int playerIndex;           // Local player index
    Car cars[MAX_CARS];        // All car states

    int totalLaps;             // Laps required to complete race

    int checkpointCount;       // Number of checkpoints (unused)
    CheckpointBox checkpoints[MAX_CHECKPOINTS];

    int finishDelayTimer;      // Frames before showing end screen
    int finalTimeMin;          // Final race time (minutes)
    int finalTimeSec;          // Final race time (seconds)
    int finalTimeMsec;         // Final race time (milliseconds)
} RaceState;
```

---

#### `bool Race_IsActive(void)`

Returns `true` if race is started and not finished.

---

#### `bool Race_IsCompleted(void)`

Returns `true` if race is finished (all laps completed).

---

#### `void Race_GetFinalTime(int* min, int* sec, int* msec)`

Gets final race time (only valid after `Race_MarkAsCompleted()`).

---

#### `int Race_GetLapCount(void)`

Gets total lap count for current map.

**Lap Counts:**
```c
ScorchingSands:  2 laps (10 laps in multiplayer)
AlpinRush:       10 laps
NeonCircuit:     10 laps
```

---

#### `bool Race_CheckFinishLineCross(const Car* car)`

Checks if player car crossed finish line.

**Called by:** `Gameplay_OnVBlank()` every frame.

**Detection:**
- Must be in `CP_STATE_READY_FOR_LAP` state (passed all checkpoints)
- Must cross from `y >= FINISH_LINE_Y` to `y < FINISH_LINE_Y`

---

### Graphics Integration Functions

#### `void Race_SetCarGfx(int index, u16* gfx)`

Sets sprite graphics pointer for a specific car.

**Called by:** `gameplay.c` after loading kart sprite graphics.

---

#### `void Race_SetLoadedQuadrant(QuadrantID quad)`

Updates currently loaded track quadrant (for terrain detection).

**Called by:** `gameplay.c` when quadrant switches.

**Used for:** Determining terrain type for sand slowdown.

---

### Pause System Functions

#### `void Race_InitPauseInterrupt(void)`

Initializes START key interrupt for pause functionality.

**Setup:**
- Configures `REG_KEYCNT` for START key
- Installs ISR via `irqSet(IRQ_KEYS, Race_PauseISR)`
- Enables key interrupt

---

#### `void Race_PauseISR(void)`

ISR for pause toggle (START key pressed).

**Behavior:**
- Debounces key press (15 frames = ~250ms)
- Toggles `isPaused` flag
- Pauses/resumes race timer via `RaceTick_TimerPause()` / `RaceTick_TimerEnable()`

**Note:** Does NOT pause chrono timer (race time keeps running).

---

#### `void Race_UpdatePauseDebounce(void)`

Updates pause debounce timer (prevents button bounce).

**Called by:** VBlank interrupt every frame.

---

#### `void Race_CleanupPauseInterrupt(void)`

Cleans up pause interrupt when exiting race.

**Actions:**
- Disables key interrupt
- Clears interrupt flags
- Resets pause state

---

## Private Implementation

### Module State

```c
static RaceState KartMania;  // Global race state

// Finish line detection (per car)
static bool wasAboveFinishLine[MAX_CARS] = {false};
static bool hasCompletedFirstCrossing[MAX_CARS] = {false};

// Checkpoint progression (per car)
static CheckpointProgressState cpState[MAX_CARS] = {CP_STATE_START};
static bool wasOnLeftSide[MAX_CARS] = {false};
static bool wasOnTopSide[MAX_CARS] = {false};

// Input tracking
static bool itemButtonHeldLast = false;

// Collision lockout (per car)
static int collisionLockoutTimer[MAX_CARS] = {0};

// Terrain detection
static QuadrantID loadedQuadrant = QUAD_BR;

// Network sync
static int networkUpdateCounter = 0;
static bool isMultiplayerRace = false;

// Countdown state
static CountdownState countdownState = COUNTDOWN_3;
static int countdownTimer = 0;
static bool raceCanStart = false;

// Pause system
static volatile bool isPaused = false;
static volatile int debounceFrames = 0;
```

---

### Race Initialization

#### Single Player Setup

```c
static void Race_InitSinglePlayerCars(void) {
    KartMania.playerIndex = 0;
    KartMania.carCount = 1;

    for (int i = 0; i < KartMania.carCount; i++) {
        initCarAtSpawn(&KartMania.cars[i], i);  // Spawn at position 0
        collisionLockoutTimer[i] = 0;
    }
}
```

#### Multiplayer Setup

```c
static void Race_InitMultiplayerCars(void) {
    KartMania.playerIndex = Multiplayer_GetMyPlayerID();
    KartMania.carCount = MAX_CARS;

    // Build sorted list of connected player indices
    int connectedIndices[MAX_CARS];
    int connectedCount = 0;

    for (int i = 0; i < MAX_CARS; i++) {
        if (Multiplayer_IsPlayerConnected(i)) {
            connectedIndices[connectedCount++] = i;
        }
    }

    // Initialize cars with spawn positions
    for (int i = 0; i < MAX_CARS; i++) {
        if (Multiplayer_IsPlayerConnected(i)) {
            // Find spawn position (rank among connected players)
            int spawnPosition = 0;
            for (int j = 0; j < connectedCount; j++) {
                if (connectedIndices[j] == i) {
                    spawnPosition = j;
                    break;
                }
            }
            initCarAtSpawn(&KartMania.cars[i], spawnPosition);
        } else {
            initCarAtSpawn(&KartMania.cars[i], -1);  // Off-map
        }
        collisionLockoutTimer[i] = 0;
    }
}
```

---

### Countdown System Implementation

```c
static void updateCountdown(void) {
    countdownTimer++;

    if (countdownTimer >= COUNTDOWN_FRAMES_PER_STEP) {  // 60 frames
        countdownTimer = 0;
        countdownState++;

        if (countdownState > COUNTDOWN_GO) {
            countdownState = COUNTDOWN_FINISHED;
            raceCanStart = true;
            RaceTick_TimerEnable();  // Start race timer
        }
    }
}

bool Race_IsCountdownActive(void) {
    return countdownState != COUNTDOWN_FINISHED;
}

CountdownState Race_GetCountdownState(void) {
    return countdownState;
}
```

---

### Player Input Handling

```c
static void handlePlayerInput(Car* car, int carIndex) {
    if (isPaused) {
        return;  // No input during pause
    }

    scanKeys();
    int held = keysHeld();

    // Steering (LEFT/RIGHT)
    if (held & KEY_LEFT) {
        car->angle512 = (car->angle512 + TURN_STEP_50CC) & ANGLE_MASK;
    }
    if (held & KEY_RIGHT) {
        car->angle512 = (car->angle512 - TURN_STEP_50CC) & ANGLE_MASK;
    }

    // Acceleration (A button)
    if (held & KEY_A) {
        if (collisionLockoutTimer[carIndex] == 0) {
            car->accelActive = true;
        }
    } else {
        car->accelActive = false;
    }

    // Braking (B button)
    car->brakeActive = (held & KEY_B) ? true : false;

    // Item usage (L or R button, single press)
    if ((held & (KEY_L | KEY_R)) && !itemButtonHeldLast) {
        Items_UseItem(car, carIndex);
        itemButtonHeldLast = true;
    } else if (!(held & (KEY_L | KEY_R))) {
        itemButtonHeldLast = false;
    }
}
```

---

### Terrain Effects

```c
static void applyTerrainEffects(Car* car) {
    TerrainType terrain = Terrain_GetTypeAtPosition(car->position, loadedQuadrant);

    if (terrain == TERRAIN_SAND) {
        // Apply sand friction (slower deceleration)
        car->friction = SAND_FRICTION;

        // Cap speed on sand
        if (car->speed > SAND_MAX_SPEED) {
            car->speed = SAND_MAX_SPEED;
        }
    } else {
        // Normal friction
        car->friction = FRICTION_50CC;
    }
}
```

**Terrain Detection:**
- Uses loaded quadrant ID to determine which terrain tileset is active
- Reads tile data at car position
- Tile ID determines terrain type (road, sand, wall)

---

### Wall Collision

```c
static void clampToMapBounds(Car* car, int carIndex) {
    int carX = FixedToInt(car->position.x);
    int carY = FixedToInt(car->position.y);

    // Check wall collision
    if (Wall_IsColliding(carX, carY, loadedQuadrant)) {
        // Bounce back to previous position
        car->position = car->prevPosition;

        // Reduce speed significantly
        car->speed = car->speed / BANANA_SPEED_DIVISOR;  // /3

        // Lock out acceleration for 60 frames (1 second)
        collisionLockoutTimer[carIndex] = COLLISION_LOCKOUT_FRAMES;

        return;
    }

    // Clamp to world bounds
    if (car->position.x < 0) car->position.x = 0;
    if (car->position.y < 0) car->position.y = 0;
    if (car->position.x > IntToFixed(MAP_SIZE - CAR_SPRITE_SIZE))
        car->position.x = IntToFixed(MAP_SIZE - CAR_SPRITE_SIZE);
    if (car->position.y > IntToFixed(MAP_SIZE - CAR_SPRITE_SIZE))
        car->position.y = IntToFixed(MAP_SIZE - CAR_SPRITE_SIZE);
}
```

---

### Checkpoint Progression

```c
static void checkCheckpointProgression(Car* car, int carIndex) {
    int carX = FixedToInt(car->position.x);
    int carY = FixedToInt(car->position.y);

    bool onLeftSide = (carX < CHECKPOINT_DIVIDE_X);   // x < 512
    bool onTopSide = (carY < CHECKPOINT_DIVIDE_Y);    // y < 512

    switch (cpState[carIndex]) {
        case CP_STATE_START:
            // Wait for player to move left
            if (onLeftSide && !wasOnLeftSide[carIndex]) {
                cpState[carIndex] = CP_STATE_NEED_LEFT;
            }
            break;

        case CP_STATE_NEED_LEFT:
            // Wait for player to cross bottom (y > 512)
            if (!onTopSide && wasOnTopSide[carIndex]) {
                cpState[carIndex] = CP_STATE_NEED_DOWN;
            }
            break;

        case CP_STATE_NEED_DOWN:
            // Wait for player to cross right (x > 512)
            if (!onLeftSide && wasOnLeftSide[carIndex]) {
                cpState[carIndex] = CP_STATE_NEED_RIGHT;
            }
            break;

        case CP_STATE_NEED_RIGHT:
            // Ready for finish line crossing
            cpState[carIndex] = CP_STATE_READY_FOR_LAP;
            break;

        case CP_STATE_READY_FOR_LAP:
            // Handled by checkFinishLineCross()
            break;
    }

    // Update previous position tracking
    wasOnLeftSide[carIndex] = onLeftSide;
    wasOnTopSide[carIndex] = onTopSide;
}
```

---

### Finish Line Detection

```c
static bool checkFinishLineCross(const Car* car, int carIndex) {
    int carY = FixedToInt(car->position.y) + CAR_SPRITE_CENTER_OFFSET;

    bool nowAboveLine = (carY < FINISH_LINE_Y);  // y < 540
    bool wasBelowLine = wasAboveFinishLine[carIndex] == false;

    // Detect upward crossing (below → above)
    if (nowAboveLine && wasBelowLine) {
        // First crossing after spawn is ignored
        if (!hasCompletedFirstCrossing[carIndex]) {
            hasCompletedFirstCrossing[carIndex] = true;
            wasAboveFinishLine[carIndex] = nowAboveLine;
            return false;
        }

        // Must be ready for lap (passed all checkpoints)
        if (cpState[carIndex] == CP_STATE_READY_FOR_LAP) {
            wasAboveFinishLine[carIndex] = nowAboveLine;
            cpState[carIndex] = CP_STATE_NEED_LEFT;  // Reset for next lap
            return true;  // LAP COMPLETE!
        }
    }

    wasAboveFinishLine[carIndex] = nowAboveLine;
    return false;
}
```

**Why First Crossing is Ignored:**
- Players spawn above the finish line
- Without this check, race would instantly complete on spawn

---

### Multiplayer Synchronization

```c
static void Race_UpdateNetworkSync(Car* player) {
    if (!isMultiplayerRace) return;

    networkUpdateCounter++;

    if (networkUpdateCounter >= 4) {  // Every 4 frames = 15 Hz
        Multiplayer_SendCarState(player);
        Multiplayer_ReceiveCarStates(KartMania.cars, KartMania.carCount);
        networkUpdateCounter = 0;
    }
}
```

**Why 15 Hz?**
- 60 Hz would saturate wireless bandwidth
- 15 Hz provides smooth visual updates with minimal latency
- Each car state is ~32 bytes (position, velocity, angle, item)

---

### Main Race Loop

```c
void Race_Tick(void) {
    // If race is finished, only count down delay timer
    if (KartMania.raceFinished) {
        if (KartMania.finishDelayTimer > 0) {
            KartMania.finishDelayTimer--;
        }
        return;
    }

    if (!Race_IsActive()) return;

    Car* player = &KartMania.cars[KartMania.playerIndex];

    // Handle player input and environment
    handlePlayerInput(player, KartMania.playerIndex);
    applyTerrainEffects(player);
    Items_Update();  // Update all items on track

    // Calculate scroll for collision checks
    int scrollX, scrollY;
    Race_CalculateScroll(player, &scrollX, &scrollY);

    // Item collision and effects
    Items_CheckCollisions(KartMania.cars, KartMania.carCount, scrollX, scrollY);
    Items_UpdatePlayerEffects(player, Items_GetPlayerEffects());

    // Update car physics and check boundaries/checkpoints
    Car_Update(player);
    clampToMapBounds(player, KartMania.playerIndex);
    checkCheckpointProgression(player, KartMania.playerIndex);

    // Decrement collision lockout timer
    if (collisionLockoutTimer[KartMania.playerIndex] > 0) {
        collisionLockoutTimer[KartMania.playerIndex]--;
    }

    // Network synchronization for multiplayer
    Race_UpdateNetworkSync(player);
}
```

---

## Pause System

The pause system uses **key interrupts** instead of polling for instant response.

### Interrupt Setup

```c
void Race_InitPauseInterrupt(void) {
    // BIT(14) = enable key interrupt
    REG_KEYCNT = BIT(14) | KEY_START;  // Interrupt on START key
    irqSet(IRQ_KEYS, Race_PauseISR);
    irqEnable(IRQ_KEYS);
}
```

### Interrupt Handler

```c
#define DEBOUNCE_DELAY 15  // ~250ms at 60Hz

void Race_PauseISR(void) {
    // Check if in debounce period
    if (debounceFrames > 0) {
        return;  // Ignore bounces
    }

    // Verify START is actually pressed (not released)
    scanKeys();
    if (!(keysHeld() & KEY_START)) {
        return;
    }

    // Toggle pause state
    isPaused = !isPaused;
    debounceFrames = DEBOUNCE_DELAY;

    // Pause/resume race timer
    if (isPaused) {
        RaceTick_TimerPause();
    } else {
        RaceTick_TimerEnable();
    }
}
```

### Debounce Update

```c
void Race_UpdatePauseDebounce(void) {
    if (debounceFrames > 0) {
        debounceFrames--;
    }
}
```

**Called by:** VBlank interrupt every frame (via `timer.c`).

---

## Constants

From `game_constants.h`:

```c
// Race layout
#define START_LINE_X 904
#define START_LINE_Y 580
#define START_FACING_ANGLE ANGLE_UP  // 270° (binary angle)
#define FINISH_LINE_Y 540

// Checkpoint divisions
#define CHECKPOINT_DIVIDE_X 512
#define CHECKPOINT_DIVIDE_Y 512

// Lap counts
#define LAPS_SCORCHING_SANDS 2
#define LAPS_ALPIN_RUSH 10
#define LAPS_NEON_CIRCUIT 10

// Physics
#define TURN_STEP_50CC 3
#define SPEED_50CC (FIXED_ONE * 3)   // 3.0 px/frame in Q16.8
#define ACCEL_50CC IntToFixed(1)     // 1.0 px/frame^2
#define FRICTION_50CC 240            // 240/256 = 0.9375
#define COLLISION_LOCKOUT_FRAMES 60  // 1 second at 60fps

// Terrain
#define SAND_FRICTION 150            // 150/256 = 0.586
#define SAND_MAX_SPEED (SPEED_50CC / SAND_SPEED_DIVISOR)

// Timing
#define FINISH_DELAY_FRAMES (5 * 60)     // 5 seconds before end screen
#define COUNTDOWN_FRAMES_PER_STEP 60     // 1 second per countdown step
```

---

## Race State Lifecycle

```
1. Race_Init(map, mode)
   ├─► Initialize cars at spawn
   ├─► Set lap count
   ├─► Start countdown (COUNTDOWN_3)
   ├─► Initialize items
   └─► Enable pause interrupt

2. Countdown Phase (3 seconds)
   ├─► Race_UpdateCountdown() (VBlank)
   ├─► Race_CountdownTick() (network sync)
   └─► NO physics updates (cars frozen at spawn)

3. Racing Phase
   ├─► Race_Tick() (60 Hz)
   │   ├─► Player input
   │   ├─► Car physics
   │   ├─► Terrain effects
   │   ├─► Wall collision
   │   ├─► Checkpoint progression
   │   ├─► Item updates
   │   └─► Network sync (15 Hz)
   │
   └─► Race_CheckFinishLineCross() (VBlank)
       ├─► Lap complete? → Reset lap timer
       └─► Final lap complete? → Race_MarkAsCompleted()

4. Finish Phase (5 seconds)
   ├─► Display final time (gameplay.c)
   ├─► Save best time (gameplay.c)
   └─► Transition to PLAYAGAIN or HOME_PAGE

5. Race_Stop()
   ├─► Cleanup pause interrupt
   ├─► Stop race timer
   └─► Set raceStarted = false
```

---

## Item System Integration

The race logic integrates with the items system:

```c
// Update all items on track (projectile movement, hazards)
Items_Update();

// Check collisions between items and cars
Items_CheckCollisions(KartMania.cars, KartMania.carCount, scrollX, scrollY);

// Apply item effects to player (speed boosts, spin, etc.)
Items_UpdatePlayerEffects(player, Items_GetPlayerEffects());

// Use held item (L or R button)
Items_UseItem(car, carIndex);
```

See [Items.md](Items.md) for detailed item system documentation.

---

## Dependencies

### Internal Modules
- `Car.h` - Car physics and state
- `items/Items.h` - Item system
- `core/game_constants.h` - Physics and timing constants
- `core/timer.h` - Timer initialization and control
- `network/multiplayer.h` - Network synchronization
- `terrain_detection.h` - Terrain type detection
- `wall_collision.h` - Wall collision detection

### libnds
- `nds.h` - Hardware registers, interrupts, input

---

## See Also

- [gameplay.md](gameplay.md) - Graphics rendering and camera management
- [Car.md](Car.md) - Car physics documentation
- [Items.md](Items.md) - Item system documentation
- [game_constants.h](../source/core/game_constants.h) - All game constants
