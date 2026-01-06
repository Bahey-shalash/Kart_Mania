# Gameplay Module Documentation

**File:** `source/gameplay/gameplay.c` / `gameplay.h`
**Authors:** Bahey Shalash, Hugo Svolgaard
**Version:** 1.0
**Date:** 06.01.2026

---

## Overview

The **Gameplay** module handles all graphics rendering and visual updates for the racing screen. It manages the main screen background (race track), sub-screen display (timer, lap counter, item display), sprite rendering, camera scrolling, and quadrant-based map loading. This module is purely concerned with **presentation** and coordinates with `gameplay_logic.c` for the actual race state and physics.

### Key Responsibilities

- **Graphics Initialization**: Configure VRAM, backgrounds, and sprite systems for both screens
- **Camera Management**: Scroll the track to follow the player's kart
- **Quadrant Loading**: Dynamically load 256×256 pixel track sections to conserve VRAM
- **Sprite Rendering**: Render player kart(s) and items with rotation
- **Sub-Screen Display**: Show lap counter, race timer, and current item
- **Countdown Display**: Render "3, 2, 1, GO!" sequence before race starts
- **Final Time Display**: Show final race time and personal best for 2.5 seconds after finish

---

## Architecture

### Separation of Concerns

```
┌─────────────────────────────────────────────────────────────┐
│                      GAMEPLAY ARCHITECTURE                   │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌───────────────────┐         ┌───────────────────┐        │
│  │   gameplay.c      │         │ gameplay_logic.c  │        │
│  │  (RENDERING)      │ ◄─────► │  (PHYSICS/STATE)  │        │
│  └───────────────────┘         └───────────────────┘        │
│          │                              │                    │
│          │                              │                    │
│  ┌───────▼──────────┐          ┌───────▼──────────┐        │
│  │ - Camera scroll  │          │ - Car physics    │        │
│  │ - Quadrant load  │          │ - Input handling │        │
│  │ - Sprite render  │          │ - Collision      │        │
│  │ - Timer display  │          │ - Lap tracking   │        │
│  │ - Final time     │          │ - Network sync   │        │
│  └──────────────────┘          └──────────────────┘        │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

### Data Flow

```
VBlank (60 Hz)
    │
    ├─► Gameplay_OnVBlank()
    │       │
    │       ├─► Race finished? → Display final time (2.5s)
    │       │
    │       ├─► Countdown active? → Render countdown (3/2/1/GO)
    │       │       │
    │       │       └─► Update camera, render cars, no physics
    │       │
    │       └─► Racing phase
    │               │
    │               ├─► Check finish line crossing → Update lap/complete race
    │               ├─► Update camera position
    │               ├─► Load quadrant if needed
    │               ├─► Render cars (single/multiplayer)
    │               ├─► Render items
    │               └─► Update sub-screen item display
    │
    └─► Timer ISR updates chrono/lap display (via timer.c)
```

---

## Public API

### Lifecycle Functions

#### `void Gameplay_Initialize(void)`

Initializes all graphics, sprites, and race state for the gameplay screen.

**Setup:**
- Main screen background (race track quadrants)
- Sub-screen display (timer, lap counter, item display)
- Kart sprites and OAM
- Race state via `Race_Init()`
- Camera scroll position

**Must be called before** `Gameplay_Update()` or `Gameplay_OnVBlank()`.

---

#### `GameState Gameplay_Update(void)`

Updates gameplay logic and handles input (non-rendering updates).

**Handles:**
- SELECT key to exit to home
- Best time saving when race finishes
- Finish display timer countdown
- State transitions

**Returns:**
- `GAMEPLAY` - Continue racing
- `PLAYAGAIN` - Race finished (single player)
- `HOME_PAGE` - User pressed SELECT or race finished (multiplayer)

---

#### `void Gameplay_OnVBlank(void)`

VBlank interrupt handler for all rendering updates (60 Hz).

**Renders:**
- Camera scroll (follows player kart)
- Quadrant loading and switching
- Countdown display (3, 2, 1, GO!)
- Kart sprites (single or multiplayer)
- Items on track
- Final time display (2.5 seconds after finish)
- Sub-screen updates (timer, lap, item)

**Called automatically** by VBlank interrupt (configured in `timer.c`).

---

#### `void Gameplay_Cleanup(void)`

Cleans up graphics resources when exiting gameplay screen.

**Frees:**
- Kart sprite graphics
- Item sprite graphics
- Sub-screen item display sprite

---

### Timer Access Functions

#### `int Gameplay_GetRaceMin(void)`

Gets current lap minutes (resets each lap).

#### `int Gameplay_GetRaceSec(void)`

Gets current lap seconds (0-59, resets each lap).

#### `int Gameplay_GetRaceMsec(void)`

Gets current lap milliseconds (0-999, resets each lap).

#### `int Gameplay_GetCurrentLap(void)`

Gets current lap number (1-based).

#### `void Gameplay_IncrementTimer(void)`

Increments race timers (both lap and total time).
**Called by timer interrupt at 1000 Hz** (configured in `timer.c`).

---

### Sub-Screen Display Functions

#### `void Gameplay_UpdateLapDisplay(int currentLap, int totalLaps)`

Updates lap display on sub-screen (e.g., "3:5" for lap 3 of 5).

#### `void Gameplay_UpdateChronoDisplay(int min, int sec, int msec)`

Updates chronometer display on sub-screen (MM:SS.mmm format).

#### `void Gameplay_ChangeDisplayColor(uint16 c)`

Changes sub-screen background color.

**Colors:**
- `BLACK` - Normal racing
- `DARK_GREEN` - New record achieved

#### `void Gameplay_PrintDigit(u16* map, int number, int x, int y)`

Renders a single digit (0-9) or separator (:, .) to tilemap.
Used internally by chrono/lap display functions.

---

## Private Implementation

### State Variables

```c
// Race timers (LAP time, resets each lap)
static int raceMin = 0;
static int raceSec = 0;
static int raceMsec = 0;
static int currentLap = 1;

// TOTAL race time (never resets)
static int totalRaceMin = 0;
static int totalRaceSec = 0;
static int totalRaceMsec = 0;

// Camera scroll position
static int scrollX = 0;
static int scrollY = 0;
static QuadrantID currentQuadrant = QUAD_BR;

// Personal best tracking
static int bestRaceMin = -1;  // -1 = no record exists
static int bestRaceSec = -1;
static int bestRaceMsec = -1;
static bool isNewRecord = false;
static bool hasSavedBestTime = false;

// Countdown/finish display state
static bool countdownCleared = false;
static int finishDisplayCounter = 0;

// Sprite graphics pointers
static u16* kartGfx = NULL;
static u16* itemDisplayGfx_Sub = NULL;
```

---

### Quadrant System

The 1024×1024 pixel world is divided into **9 quadrants** of 256×256 pixels each:

```
   TL  |  TC  |  TR
  -----+------+-----
   ML  |  MC  |  MR
  -----+------+-----
   BL  |  BC  |  BR
```

**Why?** The Nintendo DS has limited VRAM. Loading the entire 1024×1024 map at once is not feasible, so the game dynamically loads only the visible quadrant.

#### Quadrant Data Structure

```c
typedef struct {
    const unsigned int* tiles;      // Tile graphics data
    const unsigned short* map;      // Tilemap layout
    const unsigned short* palette;  // Color palette (unique per quadrant)
    unsigned int tilesLen;          // Length of tile data
    unsigned int paletteLen;        // Length of palette data
} QuadrantData;
```

#### Quadrant Loading

```c
static void Gameplay_LoadQuadrant(QuadrantID quad) {
    const QuadrantData* data = &quadrantData[quad];

    // Clear palette to avoid color pollution
    memset(BG_PALETTE, 0, 512);

    // Load tiles
    dmaCopy(data->tiles, BG_TILE_RAM(1), data->tilesLen);

    // Load palette
    dmaCopy(data->palette, BG_PALETTE, data->paletteLen);

    // Load map (64×64 split into 4 32×32 sections)
    for (int i = 0; i < 32; i++) {
        dmaCopy(&data->map[i * 64], &BG_MAP_RAM(0)[i * 32], 64);
        dmaCopy(&data->map[i * 64 + 32], &BG_MAP_RAM(1)[i * 32], 64);
        dmaCopy(&data->map[(i + 32) * 64], &BG_MAP_RAM(2)[i * 32], 64);
        dmaCopy(&data->map[(i + 32) * 64 + 32], &BG_MAP_RAM(3)[i * 32], 64);
    }
}
```

#### Quadrant Switching

When the camera moves, the quadrant is automatically switched:

```c
static void Gameplay_ApplyCameraScroll(void) {
    QuadrantID newQuadrant = Gameplay_DetermineQuadrant(scrollX, scrollY);

    if (newQuadrant != currentQuadrant) {
        Gameplay_LoadQuadrant(newQuadrant);
        currentQuadrant = newQuadrant;
        Race_SetLoadedQuadrant(newQuadrant);  // Notify gameplay_logic
    }

    // Apply scroll offset relative to quadrant origin
    int col = currentQuadrant % 3;
    int row = currentQuadrant / 3;
    BG_OFFSET[0].x = scrollX - (col * QUAD_OFFSET);
    BG_OFFSET[0].y = scrollY - (row * QUAD_OFFSET);
}
```

---

### Camera Management

The camera follows the player's kart, keeping it centered on screen.

#### Camera Update

```c
static void Gameplay_UpdateCameraPosition(const Car* player) {
    int carX = FixedToInt(player->position.x) + CAR_SPRITE_CENTER_OFFSET;
    int carY = FixedToInt(player->position.y) + CAR_SPRITE_CENTER_OFFSET;

    // Center camera on car
    scrollX = carX - (SCREEN_WIDTH / 2);
    scrollY = carY - (SCREEN_HEIGHT / 2);

    // Clamp to map bounds
    if (scrollX < 0) scrollX = 0;
    if (scrollY < 0) scrollY = 0;
    if (scrollX > MAX_SCROLL_X) scrollX = MAX_SCROLL_X;
    if (scrollY > MAX_SCROLL_Y) scrollY = MAX_SCROLL_Y;
}
```

---

### Sprite Rendering

#### Single Player

```c
static void Gameplay_RenderSinglePlayerCar(const Car* player, int carX, int carY) {
    // Convert world coordinates to screen coordinates
    int screenX = carX - scrollX - 16;
    int screenY = carY - scrollY - 16;

    // Convert 512-based angle to DS angle (65536 units = 360°)
    int dsAngle = -(player->angle512 << 6);
    oamRotateScale(&oamMain, 0, dsAngle, (1 << 8), (1 << 8));

    // Render sprite with rotation
    oamSet(&oamMain, 41, screenX, screenY, OBJPRIORITY_0, 0,
           SpriteSize_32x32, SpriteColorFormat_16Color,
           player->gfx, 0, true, false, false, false, false);
}
```

#### Multiplayer

```c
static void Gameplay_RenderMultiplayerCars(const RaceState* state) {
    for (int i = 0; i < state->carCount; i++) {
        // Skip disconnected players
        if (!Multiplayer_IsPlayerConnected(i)) {
            oamSet(&oamMain, 41 + i, 0, 192, ...);  // Hide sprite
            continue;
        }

        const Car* car = &state->cars[i];

        // Convert to screen coordinates
        int carWorldX = FixedToInt(car->position.x);
        int carWorldY = FixedToInt(car->position.y);
        int carScreenX = carWorldX - scrollX - 16;
        int carScreenY = carWorldY - scrollY - 16;

        // Setup rotation
        int dsAngle = -(car->angle512 << 6);
        oamRotateScale(&oamMain, i, dsAngle, (1 << 8), (1 << 8));

        // Only render if on screen
        bool onScreen = (carScreenX >= -32 && carScreenX < SCREEN_WIDTH &&
                         carScreenY >= -32 && carScreenY < SCREEN_HEIGHT);

        if (onScreen) {
            oamSet(&oamMain, 41 + i, carScreenX, carScreenY, ...);
        } else {
            oamSet(&oamMain, 41 + i, -64, -64, ...);  // Off-screen
        }
    }
}
```

---

### Countdown System

Before the race starts, a countdown is displayed: **3 → 2 → 1 → GO!**

```c
static void Gameplay_RenderCountdown(CountdownState state) {
    u16* map = BG_MAP_RAM_SUB(0);

    // Clear previous countdown
    for (int i = 16; i < 24; i++) {
        for (int j = 12; j < 20; j++) {
            map[i * 32 + j] = 32;  // Empty tile
        }
    }

    int centerX = 14;
    int centerY = 10;

    switch (state) {
        case COUNTDOWN_3:
            Gameplay_PrintDigit(map, 3, centerX, centerY);
            break;
        case COUNTDOWN_2:
            Gameplay_PrintDigit(map, 2, centerX, centerY);
            break;
        case COUNTDOWN_1:
            Gameplay_PrintDigit(map, 1, centerX, centerY);
            break;
        case COUNTDOWN_GO:
            Gameplay_PrintDigit(map, 0, centerX, centerY);  // "GO!" graphic
            break;
        case COUNTDOWN_FINISHED:
            break;
    }
}
```

**During countdown:**
- Camera updates to follow spawned cars
- Cars are rendered at spawn positions
- **No physics updates** (handled by `gameplay_logic.c`)

---

### Final Time Display

After crossing the finish line on the final lap, the final time is displayed for **2.5 seconds** (150 frames).

```c
static void Gameplay_DisplayFinalTime(int min, int sec, int msec) {
    u16* map = BG_MAP_RAM_SUB(0);

    // Clear screen
    memset(map, 32, 32 * 32 * 2);

    // Display FINAL TIME at y = 8
    Gameplay_PrintTime(map, min, sec, msec, 8);

    // Display PERSONAL BEST at y = 16 (if exists)
    if (bestRaceMin >= 0) {
        Gameplay_PrintTime(map, bestRaceMin, bestRaceSec, bestRaceMsec, 16);
    }

    // Green background if new record, black otherwise
    Gameplay_ChangeDisplayColor(isNewRecord ? DARK_GREEN : BLACK);
}
```

---

### Sub-Screen Item Display

The sub-screen shows the currently held item in the top-right corner.

```c
static void Gameplay_UpdateItemDisplay_Sub(void) {
    const Car* player = Race_GetPlayerCar();

    if (player->item == ITEM_NONE) {
        // Hide sprite
        oamSet(&oamSub, 0, 0, 192, 0, 0, SpriteSize_32x32, ...);
    } else {
        // Get item sprite properties
        const unsigned int* itemTiles = NULL;
        int paletteNum = 0;
        SpriteSize spriteSize = SpriteSize_16x16;
        int itemX = 0;

        Gameplay_GetItemSpriteInfo(player->item, &itemTiles,
                                   &paletteNum, &spriteSize, &itemX);

        // Copy item graphics to VRAM
        int tilesLen = (player->item == ITEM_MISSILE) ? missileTilesLen
                     : (player->item == ITEM_OIL)     ? oil_slickTilesLen
                                                      : bananaTilesLen;
        dmaCopy(itemTiles, itemDisplayGfx_Sub, tilesLen);

        // Display sprite
        oamSet(&oamSub, 0, itemX, 8, 0, paletteNum, spriteSize, ...);
    }

    oamUpdate(&oamSub);
}
```

---

## VBlank Execution Flow

```c
void Gameplay_OnVBlank(void) {
    const Car* player = Race_GetPlayerCar();
    const RaceState* state = Race_GetState();

    // Phase 1: Final Time Display (2.5 seconds after finish)
    if (state->raceFinished && finishDisplayCounter < FINISH_DISPLAY_FRAMES) {
        Gameplay_DisplayFinalTime(totalRaceMin, totalRaceSec, totalRaceMsec);
        return;
    }

    // Phase 2: Countdown (3, 2, 1, GO!)
    if (Race_IsCountdownActive()) {
        Race_UpdateCountdown();
        Gameplay_RenderCountdown(Race_GetCountdownState());

        // Update camera (no car movement during countdown)
        int carX = FixedToInt(player->position.x);
        int carY = FixedToInt(player->position.y);
        scrollX = carX - (SCREEN_WIDTH / 2);
        scrollY = carY - (SCREEN_HEIGHT / 2);
        // ... clamp scrollX/scrollY ...

        Gameplay_ApplyCameraScroll();

        // Render cars at spawn
        if (state->gameMode == SinglePlayer) {
            Gameplay_RenderSinglePlayerCar(player, carX, carY);
        } else {
            Gameplay_RenderMultiplayerCars(state);
        }

        oamUpdate(&oamMain);
        return;
    }

    // Clear countdown display (once)
    if (!countdownCleared) {
        Gameplay_ClearCountdownDisplay();
        countdownCleared = true;
    }

    // Phase 3: Racing
    Gameplay_HandleFinishLineCrossing(player);
    Gameplay_UpdateCameraPosition(player);
    Gameplay_ApplyCameraScroll();

    // Render cars
    if (state->gameMode == SinglePlayer) {
        int carX = FixedToInt(player->position.x);
        int carY = FixedToInt(player->position.y);
        Gameplay_RenderSinglePlayerCar(player, carX, carY);
    } else {
        Gameplay_RenderMultiplayerCars(state);
    }

    // Render items and update displays
    Items_Render(scrollX, scrollY);
    Gameplay_UpdateItemDisplay_Sub();
    oamUpdate(&oamMain);
}
```

---

## Timer System Integration

The gameplay module integrates with `timer.c` for race timing:

**VBlank ISR (60 Hz)** - Configured in `timer.c`:
```c
case GAMEPLAY:
    if (Race_IsCountdownActive()) {
        Race_CountdownTick();  // Network sync only
    }
    Gameplay_OnVBlank();  // Sprite updates

    // Update chrono/lap displays (only during racing)
    if (!Race_IsCountdownActive() && !Race_IsCompleted()) {
        Gameplay_UpdateChronoDisplay(Gameplay_GetRaceMin(),
                                     Gameplay_GetRaceSec(),
                                     Gameplay_GetRaceMsec());
        const RaceState* state = Race_GetState();
        Gameplay_UpdateLapDisplay(Gameplay_GetCurrentLap(),
                                  state->totalLaps);
    }
    break;
```

**Chrono ISR (1000 Hz)** - Timer increments:
```c
static void ChronoTick_ISR(void) {
    Gameplay_IncrementTimer();  // Increment race time by 1ms
}
```

---

## Best Time Tracking

When a race finishes, the final time is compared against the personal best:

```c
GameState Gameplay_Update(void) {
    // ... handle input ...

    if (Race_IsCompleted()) {
        finishDisplayCounter++;

        // Save best time (once per race)
        if (!hasSavedBestTime) {
            if (StoragePB_SaveBestTime(GameContext_GetMap(),
                                       totalRaceMin, totalRaceSec,
                                       totalRaceMsec)) {
                isNewRecord = true;  // Green background!
            }
            hasSavedBestTime = true;
        }

        // After 5 seconds, transition to PLAYAGAIN or HOME_PAGE
        if (finishDisplayCounter >= FINISH_DELAY_FRAMES) {
            if (isMultiplayerRace) {
                return HOME_PAGE;
            } else {
                return PLAYAGAIN;
            }
        }
    }

    return GAMEPLAY;
}
```

---

## Constants

```c
#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 192
#define MAP_SIZE 1024
#define QUAD_OFFSET 256
#define MAX_SCROLL_X (MAP_SIZE - SCREEN_WIDTH)  // 768
#define MAX_SCROLL_Y (MAP_SIZE - SCREEN_HEIGHT) // 832
```

From `game_constants.h`:
```c
#define FINISH_DISPLAY_FRAMES 150      // 2.5 seconds at 60fps
#define FINISH_DELAY_FRAMES (5 * 60)   // 5 seconds before transition
#define CAR_SPRITE_CENTER_OFFSET 16    // Half of 32px sprite size
```

---

## Debug Mode

Uncomment `#define console_on_debug` to enable debug console on sub-screen:

```c
#define console_on_debug  // Enable debug output
```

**Debug Output:**
- Red shell positions and waypoints
- Player position
- Target car index for homing projectiles

**Note:** Debug mode disables sub-screen item display and timer graphics.

---

## Dependencies

### Internal Modules
- `gameplay_logic.h` - Race state, car physics, countdown system
- `items/Items.h` - Item rendering and collision
- `core/context.h` - Game context (map, multiplayer mode)
- `core/timer.h` - Timer initialization
- `storage/storage_pb.h` - Personal best saving/loading
- `network/multiplayer.h` - Network state queries

### Graphics Assets
- `kart_sprite.h` - Player kart sprite (32×32, 16 color)
- `numbers.h` - Sub-screen digit tileset
- `scorching_sands_*.h` - Track quadrant graphics (9 files)
- `banana.h`, `bomb.h`, `green_shell.h`, `red_shell.h`, `missile.h`, `oil_slick.h` - Item sprites

### libnds
- `nds.h` - Hardware registers, VRAM, OAM, interrupts

---

## See Also

- [gameplay_logic.md](gameplay_logic.md) - Race physics and state management
- [game_constants.h](../source/core/game_constants.h) - Timing and display constants
- [Items.md](Items.md) - Item system documentation
- [storage_pb.md](storage_pb.md) - Best time persistence
