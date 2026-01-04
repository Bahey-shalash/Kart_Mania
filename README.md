# Kart Mania

A Nintendo DS kart racing game with single-player and multiplayer support.

---

## Code Documentation

### Play Again Screen (`ui/play_again.c`)

#### Purpose
Post-race screen allowing players to restart the race or return to the main menu.

#### Features
- **YES/NO button selection** with visual highlighting
- **Multiple input methods**: D-pad, touchscreen, and SELECT button
- **Automatic cleanup**: Stops race timers and cleans up multiplayer sessions when exiting

#### Architecture

**Graphics System:**
- **BG0 (Priority 0)**: Main "Play Again?" screen graphics
- **BG1 (Priority 1)**: Selection highlight layer (blue for YES, red for NO)
- Uses palette indices 240-241 for dynamic highlight tinting

**Input Handling:**

| Input | Action |
|-------|--------|
| LEFT / RIGHT | Select YES / NO |
| UP / DOWN | Toggle selection |
| A Button | Confirm selection |
| Touch | Select button by touching hitbox |
| SELECT | Quit directly to home (skip confirmation) |

**State Management:**
- Tracks current and previous selection to update highlights efficiently
- Only one button can be selected at a time

#### Usage
```c
// Initialize the screen
PlayAgain_Initialize();

// In main loop
while (1) {
    GameState next = PlayAgain_Update();
    if (next == GAMEPLAY) {
        // Restart race
    } else if (next == HOME_PAGE) {
        // Return to menu
    }
}
```

#### Constants (see `game_constants.h`)
- `PA_SELECTION_PAL_BASE` - Palette base index (240)
- `PA_YES_TOUCH_X/Y_MIN/MAX` - YES button touchscreen hitbox
- `PA_NO_TOUCH_X/Y_MIN/MAX` - NO button touchscreen hitbox
- `PA_YES/NO_RECT_X/Y_START/END` - Selection rectangle tile coordinates

#### Design Notes
- **Critical cleanup order**: Race timers must be stopped *before* multiplayer cleanup to avoid race conditions
- **Highlight rendering**: Uses BG layer instead of sprites for smooth, flicker-free highlighting
- **Touch validation**: Coordinates are checked against screen bounds before processing

---

### Settings Screen (`ui/settings.c`)

#### Purpose
Configuration screen for WiFi, music, and sound effects settings with persistent storage.

#### Features
- **Three toggle settings**: WiFi, Music, Sound FX (ON/OFF pills)
- **Save functionality**: Persist settings to storage (START+SELECT to reset to defaults)
- **Back/Home buttons**: Return to main menu
- **Dual-screen UI**: Bitmap graphics on top screen, interactive controls on bottom screen

#### Architecture

**Graphics System:**
- **Main Screen (Top)**: Static bitmap showing settings title/logo
- **Sub Screen (Bottom)**:
  - **BG0 (Priority 0)**: Main settings interface graphics
  - **BG1 (Priority 1)**: Toggle pills (red=OFF, green=ON) and selection highlights
- Uses palette indices 244-249 for selection highlighting
- Uses palette indices 254-255 for toggle pills (red/green)

**Input Handling:**

| Input | Action |
|-------|--------|
| UP / DOWN | Cycle through buttons |
| LEFT / RIGHT | Navigate bottom row (Save/Back/Home) |
| A Button | Toggle setting or activate button |
| Touch | Select by touching text labels or pills |
| START + SELECT + A | Reset to default settings |

**State Management:**
- Tracks current and previous selection for highlighting
- Settings changes are immediate (visual + functional)
- Persists to storage only when Save button is pressed

#### Usage
```c
// Initialize the screen
Settings_Initialize();

// In main loop
while (1) {
    GameState next = Settings_Update();
    if (next == HOME_PAGE) {
        // Return to menu
    }
}
```

#### Constants (see `game_constants.h`)
- `SETTINGS_SELECTION_PAL_BASE` - Palette base index (244)
- `SETTINGS_TOGGLE_START_X/WIDTH` - Toggle pill position and size
- `SETTINGS_WIFI/MUSIC/SOUNDFX_TOGGLE_Y_START/END` - Toggle pill Y coordinates
- `SETTINGS_WIFI/MUSIC/SOUNDFX_RECT_*` - Selection rectangle tile coordinates
- `SETTINGS_SAVE/BACK/HOME_RECT_*` - Button selection rectangle coordinates
- `SETTINGS_*_TEXT/PILL_X/Y_MIN/MAX` - Touch hitbox coordinates

#### Design Notes
- **Dual hitboxes**: Each toggle has two touch areas (text label + pill) for better UX
- **Sound FX timing**: Plays ding sound *before* potentially muting itself
- **Secret reset**: Hold START+SELECT while pressing A on Save to reset to defaults
- **Immediate application**: Settings take effect immediately, but only persist on explicit save
- **Long function split**: Touch input handling split into 4 helper functions for maintainability

---

### Home Page Screen (`ui/home_page.c`)

#### Purpose
Main menu screen with single player, multiplayer, and settings options, featuring animated kart sprite.

#### Features
- **Three menu buttons**: Single Player, Multiplayer, Settings
- **Animated sprite**: Kart scrolling across top screen
- **WiFi check**: Prevents multiplayer if WiFi is disabled in settings
- **Dual-screen UI**: Animated graphics on top, interactive menu on bottom

#### Architecture

**Graphics System:**
- **Main Screen (Top)**: Bitmap background with animated kart sprite (64x64px, scrolls left-to-right)
- **Sub Screen (Bottom)**:
  - **BG0 (Priority 0)**: Main menu graphics
  - **BG1 (Priority 1)**: Selection highlight layer
- Uses palette indices 251-253 for button highlights
- VBlank interrupt for smooth sprite animation

**Input Handling:**

| Input | Action |
|-------|--------|
| UP / DOWN | Cycle through menu buttons |
| A Button | Activate selected button |
| Touch | Select button by touching hitbox |

**State Management:**
- Tracks current and previous selection for highlighting
- Multiplayer button checks WiFi settings before proceeding
- Handles multiplayer init failure with REINIT_HOME state

#### Usage
```c
// Initialize the screen
HomePage_Initialize();

// In VBlank ISR
HomePage_OnVBlank();  // Animate kart sprite

// In main loop
while (1) {
    GameState next = HomePage_Update();
    if (next == MAPSELECTION) {
        // Go to map selection
    } else if (next == MULTIPLAYER_LOBBY) {
        // Go to multiplayer lobby
    } else if (next == SETTINGS) {
        // Go to settings
    } else if (next == REINIT_HOME) {
        // Multiplayer init failed, reinit home
    }
}

// When exiting
HomePage_Cleanup();  // Free sprite graphics
```

#### Constants (see `game_constants.h`)
- `HOME_HL_PAL_BASE` - Palette base index (251)
- `HOME_MENU_X/WIDTH/HEIGHT/SPACING/Y_START` - Menu layout parameters
- `HOME_HIGHLIGHT_TILE_X/WIDTH/HEIGHT` - Highlight tile parameters
- `HOME_KART_INITIAL_X/Y/OFFSCREEN_X` - Kart sprite animation parameters

#### Design Notes
- **WiFi validation**: Plays ding sound and stays on home page if WiFi disabled for multiplayer
- **Sprite cleanup**: Must call `HomePage_Cleanup()` before state transition to free OAM graphics
- **VBlank callback**: `HomePage_OnVBlank()` called from timer ISR for smooth animation
- **Array-driven hitboxes**: Touch detection uses array lookup for maintainability
- **Reinit state**: REINIT_HOME allows multiplayer failure recovery without losing home page state

---

## Storage Subsystem

The storage subsystem handles game settings persistence and personal best times tracking using the Nintendo DS FAT filesystem (SD card storage).

### Features
- **Settings Persistence**: Saves WiFi, Music, and Sound FX preferences to `settings.txt`
- **Personal Best Times**: Tracks fastest race times per map in `best_times.txt`
- **Default Settings**: Maintains `default_settings.txt` as a fallback configuration
- **FAT Filesystem**: Uses libfat for SD card access
- **Auto-Creation**: Creates directory structure and files automatically on first run

### Architecture

#### File Organization
- `/kart-mania/settings.txt` - Current user settings
- `/kart-mania/default_settings.txt` - Default configuration
- `/kart-mania/best_times.txt` - Personal best times for all maps

#### Settings File Format
```
wifi=1
music=1
soundfx=1
```

#### Personal Best Times Format
```
ScorchingSands=00:34.123
AlpinRush=01:02.456
NeonCircuit=00:58.789
```

### API Usage

#### Initialization
```c
// Initialize storage subsystem (call once at startup)
if (!Storage_Init()) {
    // SD card not available - use in-memory defaults
}

// Load settings from file into GameContext
if (!Storage_LoadSettings()) {
    // Settings file corrupted - defaults already applied
}
```

#### Settings Management
```c
// Save current GameContext settings to file
if (!Storage_SaveSettings()) {
    // Write failed - SD card removed or full
}

// Reset to default settings
if (!Storage_ResetToDefaults()) {
    // Reset failed
}
```

#### Personal Best Times
```c
// Initialize personal best times storage
if (!StoragePB_Init()) {
    // Failed to create best times file
}

// Load best time for a map
int min, sec, msec;
if (StoragePB_LoadBestTime(ScorchingSands, &min, &sec, &msec)) {
    // Display as MM:SS.mmm
    iprintf("Best: %02d:%02d.%03d\n", min, sec, msec);
} else {
    // No record exists for this map
}

// Save a new time (only if faster than existing record)
if (StoragePB_SaveBestTime(ScorchingSands, 0, 32, 456)) {
    // New record set!
    PlayRecordBeatSFX();
} else {
    // Not faster than existing record
}
```

### Constants

**File Paths** (see [storage.h:15](source/storage/storage.h#L15))
- `STORAGE_DIR` - `/kart-mania`
- `SETTINGS_FILE` - `/kart-mania/settings.txt`
- `DEFAULT_SETTINGS_FILE` - `/kart-mania/default_settings.txt`
- `BEST_TIMES_FILE` - `/kart-mania/best_times.txt`

**Storage Limits** (see [game_constants.h:372](source/core/game_constants.h#L372))
- `STORAGE_MAX_MAP_RECORDS` - Maximum number of map records (10)

### Design Notes
- **No Side Effects**: Storage functions only read/write files and mutate GameContext data. They never trigger audio, graphics, or network operations.
- **Graceful Degradation**: If SD card is unavailable, game uses in-memory defaults
- **Atomic Writes**: Settings files are written completely or not at all (uses `w+` mode)
- **Record Validation**: `StoragePB_SaveBestTime()` compares times before writing to prevent recording slower times
- **In-Memory Processing**: Personal best updates read entire file into memory (limited to 10 records), modify, then write back to avoid corruption
- **File Format Flexibility**: Text-based format allows manual editing for debugging

---

### Terrain Detection (`gameplay/terrain_detection.c`)

#### Purpose
Detects whether a car is on sand (off-track) or on the gray track surface by analyzing pixel colors from the background tilemap.

#### Features
- **Color-based detection**: Uses 5-bit RGB palette colors to identify terrain types
- **Quadrant-aware**: Works with the 3x3 quadrant grid system for efficient map access
- **Tolerance matching**: Allows small color variations for robust detection
- **Track exclusion**: Explicitly excludes gray track colors to prevent false positives

#### Architecture

**Detection Algorithm:**
1. Converts world coordinates to local quadrant coordinates
2. Maps to tile and pixel positions within the quadrant
3. Extracts palette color from background tilemap
4. Converts to 5-bit RGB values
5. Checks against known track colors (gray = track, sand colors = off-track)

**Color Matching:**
- Uses tolerance-based matching (`TERRAIN_COLOR_TOLERANCE_5BIT = 1`) to handle slight color variations
- Checks two gray shades: main gray (12,12,12) and light gray (14,14,14)
- Checks two sand shades: primary (20,18,12) and secondary (22,20,14)

#### Usage
```c
// Check if car position is on sand
int carX = FixedToInt(car->position.x) + CAR_SPRITE_CENTER_OFFSET;
int carY = FixedToInt(car->position.y) + CAR_SPRITE_CENTER_OFFSET;
QuadrantID quad = determineCarQuadrant(carX, carY);

if (Terrain_IsOnSand(carX, carY, quad)) {
    // Apply sand physics (reduced speed, higher friction)
    car->friction = SAND_FRICTION;
    if (car->speed > SAND_MAX_SPEED) {
        car->speed -= (excessSpeed / SAND_SPEED_DIVISOR);
    }
}
```

#### Constants (see `game_constants.h`)
- `TERRAIN_GRAY_MAIN_R5/G5/B5` - Main gray track color (12,12,12)
- `TERRAIN_GRAY_LIGHT_R5/G5/B5` - Light gray track color (14,14,14)
- `TERRAIN_SAND_PRIMARY_R5/G5/B5` - Primary sand color (20,18,12)
- `TERRAIN_SAND_SECONDARY_R5/G5/B5` - Secondary sand color (22,20,14)
- `TERRAIN_COLOR_TOLERANCE_5BIT` - Color matching tolerance (1)
- `QUAD_SIZE` - Quadrant size in pixels (256)
- `QUAD_SIZE_DOUBLE` - Double quadrant size for bounds checking (512)
- `QUADRANT_GRID_SIZE` - Size of quadrant grid (3x3)
- `TILE_SIZE` - Tile size in pixels (8)
- `SCREEN_SIZE_TILES` - Screen size in tiles (32)
- `TILE_INDEX_MASK` - Mask for extracting tile index (0x3FF)
- `TILE_DATA_SIZE` - Size of tile data in bytes (64)
- `TILE_WIDTH_PIXELS` - Width of tile in pixels (8)
- `COLOR_5BIT_MASK` - Mask for 5-bit color channel (0x1F)
- `COLOR_RED_SHIFT/GREEN_SHIFT/BLUE_SHIFT` - Bit shifts for color channels

#### Design Notes
- **Quadrant-based lookup**: Uses quadrant system to efficiently access the correct tilemap region
- **Two-stage detection**: First excludes track colors, then checks for sand colors to prevent false positives
- **Palette-based**: Reads directly from DS background palette, not from rendered pixels
- **Pixel-perfect**: Checks exact pixel position within tile for accurate detection
- **No side effects**: Pure function that only returns boolean result

---

### Wall Collision (`gameplay/wall_collision.c`)

#### Purpose
Detects collisions between cars and track walls using a quadrant-based wall segment system, and provides collision normals for physics response.

#### Features
- **Quadrant-based detection**: Divides the track into 9 quadrants (3x3 grid) for efficient collision checking
- **Wall segment system**: Uses horizontal and vertical wall segments with precise coordinate ranges
- **Collision normal calculation**: Returns collision normals for proper bounce physics
- **Radius-based detection**: Uses circular collision detection with configurable radius

#### Architecture

**Wall Data Structure:**
- Each quadrant has a static array of `WallSegment` structures
- Wall segments are either `WALL_HORIZONTAL` (constant Y, X range) or `WALL_VERTICAL` (constant X, Y range)
- Segments store: type, fixed coordinate, and min/max range
- Quadrants are indexed by `QuadrantID` enum (QUAD_TL through QUAD_BR)

**Collision Detection Algorithm:**
1. Validates quadrant ID is within bounds
2. Looks up wall segments for the given quadrant
3. For each segment, checks if car's circular hitbox intersects
4. Returns true on first collision found

**Collision Normal Calculation:**
1. Finds the closest wall segment to the car
2. Determines normal direction based on wall type:
   - Horizontal walls: normal is (0, ±1) depending on which side car is on
   - Vertical walls: normal is (±1, 0) depending on which side car is on
3. Returns normalized direction vector for physics response

#### Usage
```c
// Check for collision
int carX = FixedToInt(car->position.x) + CAR_SPRITE_CENTER_OFFSET;
int carY = FixedToInt(car->position.y) + CAR_SPRITE_CENTER_OFFSET;
QuadrantID quad = determineCarQuadrant(carX, carY);

if (Wall_CheckCollision(carX, carY, CAR_RADIUS, quad)) {
    // Get collision normal for bounce
    int nx, ny;
    Wall_GetCollisionNormal(carX, carY, quad, &nx, &ny);
    
    if (nx != 0 || ny != 0) {
        // Push car away from wall
        int pushDistance = CAR_RADIUS;
        car->position.x += IntToFixed(nx * pushDistance);
        car->position.y += IntToFixed(ny * pushDistance);
        
        // Stop car and apply lockout
        car->speed = 0;
        collisionLockoutTimer[carIndex] = COLLISION_LOCKOUT_FRAMES;
    }
}
```

#### Constants (see `game_constants.h`)
- `CAR_RADIUS` - Collision radius for cars (16 pixels)
- `COLLISION_LOCKOUT_FRAMES` - Frames to disable acceleration after wall hit (60)
- `QUAD_SIZE` - Quadrant size in pixels (256)
- `QUADRANT_GRID_SIZE` - Size of quadrant grid (3x3)

#### Design Notes
- **Static wall data**: All wall segments are compile-time constants for performance
- **Quadrant optimization**: Only checks walls in the current quadrant, not the entire map
- **Circular hitbox**: Uses radius-based detection for smooth collision response
- **Normal calculation**: Finds closest wall for accurate bounce direction
- **No side effects**: Pure collision detection functions; physics response handled by caller
- **Coordinate system**: All wall coordinates are in global map space (0-1024 pixels)

---

### Gameplay Screen (`gameplay/gameplay.c`)

#### Purpose
Main racing screen handling real-time gameplay, rendering, physics, and race progression for both single-player and multiplayer modes.

#### Features
- **Dual-screen racing interface**: Top screen shows the race track with car sprites; bottom screen displays race timer, lap counter, and current item
- **Dynamic camera system**: Smooth scrolling that follows the player's kart with automatic quadrant loading
- **Countdown sequence**: 3-2-1-GO! countdown before race start with visual display
- **Lap tracking**: Monitors finish line crossings and manages lap progression
- **Personal best times**: Saves and displays best race times per map with visual feedback for new records
- **Item display**: Shows currently held item in top-right corner of sub screen
- **Race completion**: Displays final time with personal best comparison (green background for new records)
- **Debug mode**: Optional console output for development (see `CONSOLE_DEBUG_MODE`)

#### Architecture

**Graphics System:**
- **Main Screen (Top)**:
  - **BG0 (64x64, Priority 1)**: Dynamic race track with quadrant-based streaming
  - **Sprites**: Kart sprites with rotation for all players (OAM slots 41+)
  - **Item sprites**: Rendered dynamically across the track

- **Sub Screen (Bottom)**:
  - **BG0 (32x32)**: Timer and lap counter display using number tiles
  - **Sprite (Slot 0)**: Current item icon (16x16 or 32x32 depending on item)
  - Background color changes: Black (normal), Green (new record)

**Quadrant Streaming:**
The 1024×1024 map is divided into a 3×3 grid of 256×256 quadrants:
```
[TL] [TC] [TR]
[ML] [MC] [MR]
[BL] [BC] [BR]
```
- Only the visible quadrant is loaded into VRAM at any time
- Automatic transitions when camera crosses quadrant boundaries
- Each quadrant has independent tiles, map data, and palette

**Input Handling:**

| Input | Action |
|-------|--------|
| SELECT | Exit to home screen (emergency quit) |
| D-pad / A / B | Race controls (handled by `Race_Update` in gameplay_logic) |

**State Management:**
- **Timer system**: Tracks both lap time (resets per lap) and total race time
- **Lap progression**: Increments lap counter on finish line cross, completes race on final lap
- **Best time tracking**: Loads best time on init, saves new records on race completion
- **Display states**: Countdown → Racing → Final Time Display → Play Again/Home

#### Camera System

**Scrolling Logic:**
```c
// Center camera on player
scrollX = playerX - (SCREEN_WIDTH / 2);
scrollY = playerY - (SCREEN_HEIGHT / 2);

// Clamp to map boundaries
if (scrollX < 0) scrollX = 0;
if (scrollY < 0) scrollY = 0;
if (scrollX > MAX_SCROLL_X) scrollX = MAX_SCROLL_X;
if (scrollY > MAX_SCROLL_Y) scrollY = MAX_SCROLL_Y;
```

**Quadrant Detection:**
```c
int col = (x < 256) ? 0 : (x < 512) ? 1 : 2;
int row = (y < 256) ? 0 : (y < 512) ? 1 : 2;
quadrant = row * 3 + col;
```

#### Rendering Modes

**Single-Player Mode:**
- Renders only the player's kart at OAM slot 41
- Uses affine matrix 0 for rotation
- Simpler rendering path for performance

**Multiplayer Mode:**
- Renders all connected players (slots 41-44)
- Each car uses its own affine matrix (0-3)
- Off-screen cars are positioned at (-64, -64)
- Disconnected players are hidden

**Countdown Rendering:**
- All cars are rendered during countdown (prevents pop-in)
- Camera follows player even before race starts
- Countdown numbers displayed on sub screen

#### Usage
```c
// Initialize gameplay screen
Gameplay_Initialize();

// In main loop (60 FPS)
while (1) {
    swiWaitForVBlank();
    Gameplay_OnVBlank();  // Render frame
    
    GameState next = Gameplay_Update();  // Update logic
    if (next != GAMEPLAY) {
        Gameplay_Cleanup();
        // Transition to next state
        break;
    }
}

// In timer interrupt (1000 Hz)
void timerIRQ(void) {
    Gameplay_IncrementTimer();
    Gameplay_UpdateChronoDisplay(
        Gameplay_GetRaceMin(),
        Gameplay_GetRaceSec(), 
        Gameplay_GetRaceMsec()
    );
}
```

#### Public API

**Lifecycle:**
- `Gameplay_Initialize()` - Set up graphics, load map, initialize race
- `Gameplay_Update()` - Handle input and state transitions
- `Gameplay_OnVBlank()` - Render frame (camera, sprites, items, UI)
- `Gameplay_Cleanup()` - Free sprites and allocated memory

**Timer Access:**
- `Gameplay_GetRaceMin/Sec/Msec()` - Get current lap time
- `Gameplay_GetCurrentLap()` - Get lap number (1-indexed)
- `Gameplay_IncrementTimer()` - Called by timer interrupt (1000 Hz)

**Display Functions:**
- `Gameplay_UpdateChronoDisplay(min, sec, msec)` - Update race timer
- `Gameplay_UpdateLapDisplay(current, total)` - Update lap counter
- `Gameplay_ChangeDisplayColor(color)` - Set sub screen background color

#### Constants (see `game_constants.h`)

**Display Timing:**
- `FINISH_DISPLAY_FRAMES` - Duration to show final time (150 frames = 2.5s)
- `COUNTDOWN_NUMBER_DURATION` - Duration for each countdown number (60 frames)
- `COUNTDOWN_GO_DURATION` - Duration for "GO!" display (60 frames)

**Map & Rendering:**
- `MAP_SIZE` - Full map size (1024×1024 pixels)
- `QUAD_SIZE` - Quadrant size (256×256 pixels)
- `SCREEN_WIDTH/HEIGHT` - Screen dimensions (256×192)
- `MAX_SCROLL_X/Y` - Maximum scroll positions

**Sprite & OAM:**
- `CAR_SPRITE_SIZE` - Kart sprite size (32×32)
- `CAR_SPRITE_CENTER_OFFSET` - Centering offset (16 pixels)
- OAM slots 41-44 reserved for player karts

#### Design Notes

**Performance Optimization:**
- **Quadrant streaming**: Only one 256×256 section loaded at a time (saves VRAM)
- **Palette isolation**: Each quadrant has independent palette to prevent color pollution
- **Off-screen culling**: Multiplayer cars outside viewport are positioned off-screen
- **Single update per frame**: All rendering happens in VBlank callback

**Critical Timing:**
- **Best time saving**: Happens in `Update()`, NOT in `OnVBlank()` (storage is not VBlank-safe)
- **Timer increment**: Must be called from timer interrupt at 1000 Hz for accurate milliseconds
- **Display counter**: Uses frame counter for final time display (60 FPS)

**Debug Mode:**
- Define `CONSOLE_DEBUG_MODE` to enable debug console on sub screen
- Shows red shell positions, angles, waypoints, and target information
- Disables normal sub-screen UI when active

**Memory Management:**
- Kart sprite graphics allocated once, shared by all cars
- Item graphics loaded by `Items_LoadGraphics()`
- All sprites freed in `Gameplay_Cleanup()`
- Item display sprite (sub screen) uses palette slots 32-127

**Multiplayer Considerations:**
- Camera always follows local player (not necessarily player 0)
- Other players' karts rendered relative to local player's camera
- Disconnected players hidden but their OAM slots remain allocated
- Race completion returns to home (no play-again in multiplayer)

#### State Flow
```
Initialize
    ↓
Countdown (3-2-1-GO!)
    ↓
Racing
    ↓
Lap Complete → Reset lap timer, increment lap counter
    ↓
Race Complete → Display final time (2.5s)
    ↓
→ Play Again (single-player)
→ Home Page (multiplayer)
```

#### Item Display System

**Item Sprites:**
- Loaded to sub-screen OAM at initialization
- Palette slots: Banana(32), Bomb(48), Green Shell(64), Red Shell(80), Missile(96), Oil(112)
- Positioned in top-right corner (220, 8) or (208, 8) for larger items
- Hidden when `player->item == ITEM_NONE`

**Sprite Sizes:**
- Most items: 16×16
- Missile: 16×32
- Oil Slick: 32×32

**Update Frequency:**
- Updated every frame in `OnVBlank()`
- Graphics copied dynamically based on current item
- Automatically hides when race finishes

---

### Gameplay Logic (`gameplay/gameplay_logic.c`)

#### Purpose
Core race logic engine handling physics updates, lap tracking, input processing, and race state management for both single-player and multiplayer modes.

#### Features
- **Race state management**: Complete lifecycle from initialization to completion
- **Physics engine**: Car movement, terrain effects, collision detection, and wall bouncing
- **Lap validation system**: Checkpoint-based lap counting with anti-cheat measures
- **Countdown system**: 3-2-1-GO! sequence before race start
- **Input handling**: Player controls with item usage and inverted control effects
- **Network synchronization**: 15Hz multiplayer state updates
- **Pause functionality**: START button interrupt-based pause system with debouncing
- **Collision lockout**: Temporary acceleration disable after wall hits

#### Architecture

**Race State Structure:**
```c
typedef struct {
    bool raceStarted;           // Race active flag
    bool raceFinished;          // Race completed flag
    GameMode gameMode;          // SinglePlayer or MultiPlayer
    Map currentMap;             // Current race track
    
    int carCount;               // Number of cars (1 or MAX_CARS)
    int playerIndex;            // Local player index
    Car cars[MAX_CARS];         // All car states
    
    int totalLaps;              // Laps for this race
    int finishDelayTimer;       // Countdown before finish menu
    int finalTimeMin/Sec/Msec;  // Final race time
} RaceState;
```

**Checkpoint Progression System:**
Cars must pass through 4 checkpoints in sequence to validate a lap:
```
START → NEED_LEFT → NEED_DOWN → NEED_RIGHT → READY_FOR_LAP
  |         |            |             |              |
  └─────────┴────────────┴─────────────┴──────────────┘
           (Crossing finish line here counts as lap)
```

This prevents lap skipping by driving backwards or cutting corners.

**State Tracking Arrays:**
```c
// Per-car tracking (indexed by car index)
bool wasAboveFinishLine[MAX_CARS];      // Finish line position
bool hasCompletedFirstCrossing[MAX_CARS]; // Ignore spawn crossing
CheckpointProgressState cpState[MAX_CARS]; // Current checkpoint state
bool wasOnLeftSide[MAX_CARS];           // Left checkpoint state
bool wasOnTopSide[MAX_CARS];            // Top checkpoint state
int collisionLockoutTimer[MAX_CARS];    // Wall collision cooldown
```

#### Update Loops

**Main Race Loop (`Race_Tick`):**
```
60 FPS Update Cycle:
├─ Check if race finished → only decrement delay timer
├─ Handle player input
├─ Apply terrain effects (sand friction/speed cap)
├─ Update items system
├─ Check item collisions
├─ Update car physics
├─ Handle wall collisions + lockout
├─ Check checkpoint progression
└─ Network sync (every 4 frames in multiplayer)
```

**Countdown Loop (`Race_CountdownTick`):**
```
During Countdown (multiplayer only):
├─ Network sync only (share spawn positions)
└─ No physics or game logic
```

**Physics Update Order:**
1. Player input → steering/acceleration
2. Terrain effects → friction/speed modification
3. Items update → projectile movement
4. Item collisions → pickup/hit detection
5. Car physics → velocity/position update
6. Wall collision → pushback + lockout
7. Checkpoint tracking → lap validation

#### Input Handling

**Control Mapping:**

| Input | Action | Notes |
|-------|--------|-------|
| A | Accelerate | Disabled during collision lockout |
| B | Brake | Only when moving forward |
| LEFT/RIGHT | Steer | Only works when accelerating |
| L | Use item | Press and release to fire |
| DOWN + L | Drop item behind | Hazards (banana, oil, bomb) |
| START | Pause/Resume | Interrupt-driven with debounce |

**Control Inversion:**
When hit by mushroom item, steering is inverted:
```c
bool invertControls = effects->confusionActive;
int turnAmount = invertControls ? TURN_STEP_50CC : -TURN_STEP_50CC;
```

#### Lap Validation System

**Checkpoint Zones:**
- **X Divide**: 512 pixels (left vs right side)
- **Y Divide**: 512 pixels (top vs bottom side)
- **Finish Line**: Y = 540 pixels

**Validation Sequence:**
1. Car spawns at bottom → `CP_STATE_START`
2. Crosses Y=512 going up → `CP_STATE_NEED_LEFT`
3. Crosses X=512 going left → `CP_STATE_NEED_DOWN`
4. Crosses Y=512 going down → `CP_STATE_NEED_RIGHT`
5. Crosses X=512 going right → `CP_STATE_READY_FOR_LAP`
6. Crosses finish line (Y=540) → **Lap counted!**
7. Reset to `CP_STATE_START`

**Anti-Cheat Measures:**
- First finish line crossing ignored (spawn position)
- Must complete full checkpoint sequence
- Cannot skip checkpoints
- Prevents backwards driving exploits

#### Network Synchronization

**Update Frequency:**
- **Rate**: 15 Hz (every 4 frames at 60 FPS)
- **Data**: Car position, velocity, angle, item state

**Multiplayer Mode:**
```c
if (networkUpdateCounter >= 4) {
    Multiplayer_SendCarState(myPlayer);
    Multiplayer_ReceiveCarStates(allCars, carCount);
    networkUpdateCounter = 0;
}
```

**Spawn Position Assignment:**
- Connected players sorted by index
- Spawn positions assigned sequentially
- Disconnected players placed off-map (-1000, -1000)
- Two columns: even indices left, odd indices right

#### Terrain System

**Sand Effects:**
```c
if (Terrain_IsOnSand(carX, carY, quadrant)) {
    car->friction = SAND_FRICTION;        // 150/256 = 58.6%
    if (car->speed > SAND_MAX_SPEED) {
        car->speed -= excess / 2;         // Gradual slowdown
    }
}
```

**Normal Track:**
```c
car->friction = FRICTION_50CC;  // 240/256 = 93.75%
```

#### Collision System

**Wall Collision:**
1. Check if car center within CAR_RADIUS of wall
2. Get collision normal vector (nx, ny)
3. Push car away by CAR_RADIUS pixels
4. Set speed to 0
5. Enable collision lockout for 60 frames (1 second)

**Lockout Effect:**
- Acceleration disabled
- Prevents wall grinding exploits
- Visual feedback: car stops moving forward

**Map Boundaries:**
```c
// Account for sprite center offset
minPos = -16;           // Allow sprite to reach edge
maxPos = 1024 - 16;     // Map size minus offset
```

#### Countdown System

**Sequence:**
```
COUNTDOWN_3  (60 frames) → "3"
COUNTDOWN_2  (60 frames) → "2"
COUNTDOWN_1  (60 frames) → "1"
COUNTDOWN_GO (60 frames) → "GO!"
COUNTDOWN_FINISHED       → Race starts, timer begins
```

**Timing:**
- Each number displays for 1 second (60 frames)
- Total countdown: 4 seconds
- Timer starts exactly when GO! disappears
- Player input blocked until COUNTDOWN_FINISHED

#### Pause System

**Interrupt-Based Implementation:**
```c
// IRQ handler for START button
void Race_PauseISR(void) {
    if (debounceFrames > 0) return;      // Prevent bounce
    if (!(keysHeld() & KEY_START)) return; // Verify press
    
    isPaused = !isPaused;
    debounceFrames = 15;  // 250ms debounce
    
    isPaused ? RaceTick_TimerPause() : RaceTick_TimerEnable();
}
```

**Debouncing:**
- 15 frame cooldown (~250ms at 60 FPS)
- Prevents accidental double-toggles
- Updated each frame in main loop

#### Race Modes

**Single-Player:**
- 1 car (player only)
- Player index: 0
- Standard lap counts per map
- No network synchronization

**Multiplayer:**
- Up to 8 cars (MAX_CARS)
- Player index: from `Multiplayer_GetMyPlayerID()`
- Special lap count: 5 laps on Scorching Sands
- 15Hz network updates
- Spawn positions based on connected players

#### Usage

```c
// Initialize race
Race_Init(ScorchingSands, SinglePlayer);

// Main game loop (60 FPS)
while (1) {
    swiWaitForVBlank();
    
    // Update countdown or race
    if (Race_IsCountdownActive()) {
        Race_CountdownTick();  // MP only: network sync
        Race_UpdateCountdown(); // Advance countdown
    } else {
        Race_Tick();  // Normal race update
    }
    
    Race_UpdatePauseDebounce();  // Update pause cooldown
    
    // Check for race completion
    if (Race_IsCompleted()) {
        int min, sec, msec;
        Race_GetFinalTime(&min, &sec, &msec);
        // Show results screen
    }
}

// Cleanup
Race_Stop();
```

#### Public API

**Lifecycle:**
- `Race_Init(map, mode)` - Initialize new race
- `Race_Tick()` - Update race logic (60 FPS)
- `Race_CountdownTick()` - Update countdown network sync
- `Race_Reset()` - Reset to initial state
- `Race_Stop()` - Cleanup and disable timers
- `Race_MarkAsCompleted(min, sec, msec)` - Mark race finished

**State Queries:**
- `Race_GetState()` - Get full race state (read-only)
- `Race_GetPlayerCar()` - Get player's car (read-only)
- `Race_IsActive()` - Check if race is running
- `Race_IsCompleted()` - Check if race is finished
- `Race_GetLapCount()` - Get total laps
- `Race_CheckFinishLineCross(car)` - Check lap completion
- `Race_GetFinalTime(&min, &sec, &msec)` - Get final time

**Countdown:**
- `Race_UpdateCountdown()` - Advance countdown state
- `Race_IsCountdownActive()` - Check if still counting
- `Race_GetCountdownState()` - Get current countdown phase

**Configuration:**
- `Race_SetCarGfx(index, gfx)` - Set car sprite graphics
- `Race_SetLoadedQuadrant(quad)` - Update loaded map section

**Pause System:**
- `Race_InitPauseInterrupt()` - Enable pause button
- `Race_CleanupPauseInterrupt()` - Disable pause button
- `Race_UpdatePauseDebounce()` - Update bounce prevention
- `Race_PauseISR()` - Interrupt service routine (internal)

#### Constants (see `game_constants.h`)

**Physics:**
- `TURN_STEP_50CC` - Steering angle delta (3 units)
- `SPEED_50CC` - Maximum speed (3.0 px/frame in Q16.8)
- `ACCEL_50CC` - Acceleration rate (1.0 px/frame² in Q16.8)
- `FRICTION_50CC` - Normal friction (240/256 = 93.75%)
- `SAND_FRICTION` - Sand friction (150/256 = 58.6%)
- `COLLISION_LOCKOUT_FRAMES` - Wall hit cooldown (60 frames)

**Race Layout:**
- `START_LINE_X/Y` - Spawn position (904, 580)
- `START_FACING_ANGLE` - Initial direction (ANGLE_UP = 270°)
- `FINISH_LINE_Y` - Finish line position (540)
- `CHECKPOINT_DIVIDE_X/Y` - Checkpoint boundaries (512, 512)

**Lap Counts:**
- `LAPS_SCORCHING_SANDS` - 2 laps (5 in multiplayer)
- `LAPS_ALPIN_RUSH` - 10 laps
- `LAPS_NEON_CIRCUIT` - 10 laps

**Timing:**
- `COUNTDOWN_NUMBER_DURATION` - 60 frames per number
- `COUNTDOWN_GO_DURATION` - 60 frames for GO!
- `FINISH_DELAY_FRAMES` - 300 frames (5 seconds) post-race

#### Design Notes

**Critical Update Order:**
Physics updates must follow this exact order to prevent bugs:
1. Input → Terrain → Items → Collisions → Physics → Walls → Checkpoints

**Multiplayer Spawn Algorithm:**
```c
// Build sorted list of connected players
connectedIndices = [0, 2, 5, 7];  // Example: 4 players

// Assign spawn positions sequentially
Player 0 → spawnPosition 0 → (904, 580)  // Left column, first
Player 2 → spawnPosition 1 → (936, 604)  // Right column, second
Player 5 → spawnPosition 2 → (904, 628)  // Left column, third
Player 7 → spawnPosition 3 → (936, 652)  // Right column, fourth
```

**Collision Lockout Rationale:**
Prevents exploit where players could "ride" walls by holding accelerate. Forces player to back away before accelerating again.

**First Crossing Ignore:**
Cars spawn above finish line (Y=580 > 540). Without this check, spawning would immediately count as a lap completion.

**Network Update Rate:**
15Hz chosen as balance between:
- Bandwidth (lower = less data)
- Responsiveness (higher = smoother)
- Frame budget (60 FPS / 4 = 15 updates/sec)

**Pause Interrupt Priority:**
Uses key interrupt (IRQ_KEYS) for instant response. Alternative polling-based approach would have 1-frame delay and could miss quick presses.

**Terrain Detection Optimization:**
Checks quadrant ID to avoid loading wrong map data. Sand detection uses pixel color sampling from loaded quadrant tiles.

**Memory Safety:**
- All array accesses bounds-checked
- Car index validated before gfx assignment
- Disconnected players safely handled with off-map positions

#### State Flow

```
Race_Init()
    ↓
COUNTDOWN_3 (1 second)
    ↓
COUNTDOWN_2 (1 second)
    ↓
COUNTDOWN_1 (1 second)
    ↓
COUNTDOWN_GO (1 second)
    ↓
COUNTDOWN_FINISHED
    ↓
Race Active (Race_Tick runs)
    ↓
Lap Complete → Reset lap timer, continue racing
    ↓
Final Lap Complete → Race_MarkAsCompleted()
    ↓
Delay Timer (5 seconds showing final time)
    ↓
finishDelayTimer reaches 0
    ↓
→ PLAYAGAIN (single-player)
→ HOME_PAGE (multiplayer)
```

#### Common Integration Patterns

**Timer Interrupt Integration:**
```c
// In timer ISR (1000 Hz)
void timerIRQ(void) {
    if (Race_IsActive() && !Race_IsCountdownActive()) {
        Gameplay_IncrementTimer();  // Lap + total time
    }
}
```

**Input Blocking:**
```c
// Player input only processed when:
// - Race is active (not finished)
// - Countdown has finished
// - Not paused
if (Race_IsActive() && !Race_IsCountdownActive() && !isPaused) {
    Race_Tick();
}
```

**Lap Completion Handling:**
```c
// In gameplay update loop
if (Race_CheckFinishLineCross(player)) {
    if (currentLap < Race_GetLapCount()) {
        currentLap++;      // Next lap
        // Reset lap timer (but not total time)
    } else {
        // Final lap - race complete!
        Race_MarkAsCompleted(totalMin, totalSec, totalMsec);
    }
}
```
---
## Getting started

To make it easy for you to get started with GitLab, here's a list of recommended next steps.

Already a pro? Just edit this README.md and make it your own. Want to make it easy? [Use the template at the bottom](#editing-this-readme)!

## Add your files

- [ ] [Create](https://docs.gitlab.com/ee/user/project/repository/web_editor.html#create-a-file) or [upload](https://docs.gitlab.com/ee/user/project/repository/web_editor.html#upload-a-file) files
- [ ] [Add files using the command line](https://docs.gitlab.com/topics/git/add_files/#add-files-to-a-git-repository) or push an existing Git repository with the following command:

```
cd existing_repo
git remote add origin https://gitlab.epfl.ch/shalash/kart-mania.git
git branch -M main
git push -uf origin main
```

## Integrate with your tools

- [ ] [Set up project integrations](https://gitlab.epfl.ch/shalash/kart-mania/-/settings/integrations)

## Collaborate with your team

- [ ] [Invite team members and collaborators](https://docs.gitlab.com/ee/user/project/members/)
- [ ] [Create a new merge request](https://docs.gitlab.com/ee/user/project/merge_requests/creating_merge_requests.html)
- [ ] [Automatically close issues from merge requests](https://docs.gitlab.com/ee/user/project/issues/managing_issues.html#closing-issues-automatically)
- [ ] [Enable merge request approvals](https://docs.gitlab.com/ee/user/project/merge_requests/approvals/)
- [ ] [Set auto-merge](https://docs.gitlab.com/user/project/merge_requests/auto_merge/)

## Test and Deploy

Use the built-in continuous integration in GitLab.

- [ ] [Get started with GitLab CI/CD](https://docs.gitlab.com/ee/ci/quick_start/)
- [ ] [Analyze your code for known vulnerabilities with Static Application Security Testing (SAST)](https://docs.gitlab.com/ee/user/application_security/sast/)
- [ ] [Deploy to Kubernetes, Amazon EC2, or Amazon ECS using Auto Deploy](https://docs.gitlab.com/ee/topics/autodevops/requirements.html)
- [ ] [Use pull-based deployments for improved Kubernetes management](https://docs.gitlab.com/ee/user/clusters/agent/)
- [ ] [Set up protected environments](https://docs.gitlab.com/ee/ci/environments/protected_environments.html)

***

# Editing this README

When you're ready to make this README your own, just edit this file and use the handy template below (or feel free to structure it however you want - this is just a starting point!). Thanks to [makeareadme.com](https://www.makeareadme.com/) for this template.

## Suggestions for a good README

Every project is different, so consider which of these sections apply to yours. The sections used in the template are suggestions for most open source projects. Also keep in mind that while a README can be too long and detailed, too long is better than too short. If you think your README is too long, consider utilizing another form of documentation rather than cutting out information.

## Name
Choose a self-explaining name for your project.

## Description
Let people know what your project can do specifically. Provide context and add a link to any reference visitors might be unfamiliar with. A list of Features or a Background subsection can also be added here. If there are alternatives to your project, this is a good place to list differentiating factors.

## Badges
On some READMEs, you may see small images that convey metadata, such as whether or not all the tests are passing for the project. You can use Shields to add some to your README. Many services also have instructions for adding a badge.

## Visuals
Depending on what you are making, it can be a good idea to include screenshots or even a video (you'll frequently see GIFs rather than actual videos). Tools like ttygif can help, but check out Asciinema for a more sophisticated method.

## Installation
Within a particular ecosystem, there may be a common way of installing things, such as using Yarn, NuGet, or Homebrew. However, consider the possibility that whoever is reading your README is a novice and would like more guidance. Listing specific steps helps remove ambiguity and gets people to using your project as quickly as possible. If it only runs in a specific context like a particular programming language version or operating system or has dependencies that have to be installed manually, also add a Requirements subsection.

## Usage
Use examples liberally, and show the expected output if you can. It's helpful to have inline the smallest example of usage that you can demonstrate, while providing links to more sophisticated examples if they are too long to reasonably include in the README.

## Support
Tell people where they can go to for help. It can be any combination of an issue tracker, a chat room, an email address, etc.

## Roadmap
If you have ideas for releases in the future, it is a good idea to list them in the README.

## Contributing
State if you are open to contributions and what your requirements are for accepting them.

For people who want to make changes to your project, it's helpful to have some documentation on how to get started. Perhaps there is a script that they should run or some environment variables that they need to set. Make these steps explicit. These instructions could also be useful to your future self.

You can also document commands to lint the code or run tests. These steps help to ensure high code quality and reduce the likelihood that the changes inadvertently break something. Having instructions for running tests is especially helpful if it requires external setup, such as starting a Selenium server for testing in a browser.

## Authors and acknowledgment
Show your appreciation to those who have contributed to the project.

## License
For open source projects, say how it is licensed.

## Project status
If you have run out of energy or time for your project, put a note at the top of the README saying that development has slowed down or stopped completely. Someone may choose to fork your project or volunteer to step in as a maintainer or owner, allowing your project to keep going. You can also make an explicit request for maintainers.
