# Home Page Screen

## Overview

The Home Page is the main menu interface and entry point of Kart-Mania. It presents three primary options—Singleplayer, Multiplayer, and Settings—displayed on the bottom screen, while featuring an animated kart sprite continuously scrolling across the top screen for visual appeal.

The system is implemented in [home_page.c](../source/ui/home_page.c) and [home_page.h](../source/ui/home_page.h), using dual-screen graphics with bitmap artwork on top and interactive menu on bottom, supporting both D-pad and touch input.

**Key Features:**
- Clean three-option main menu (Singleplayer, Multiplayer, Settings)
- Animated kart sprite on top screen (continuous horizontal scroll)
- Selection highlighting with color-coded feedback
- WiFi connectivity check before entering multiplayer
- Automatic multiplayer initialization and connection handling
- D-pad and touchscreen input support
- Resource cleanup when exiting (sprite graphics deallocation)

## Architecture

### Dual-Screen Layout

**Top Screen (Main):**
- Mode 5 bitmap graphics (BG2) with static home artwork
- Sprite layer (OAM) with 64×64 animated kart
- Kart scrolls left-to-right continuously (wraps around)

**Bottom Screen (Sub):**
- Mode 0 tiled graphics (BG0 + BG1)
- BG0 (Priority 0): Front layer with menu graphics and button labels
- BG1 (Priority 1): Back layer for selection highlighting

### Graphics System

**Top Screen Rendering:**
- **BG2**: 256×256 8-bit indexed bitmap showing title artwork
- **Sprites**: Single 64×64 kart sprite in 256-color mode
- **Animation**: VBlank callback updates sprite position each frame (60 Hz)

**Bottom Screen Rendering:**
The Home Page uses the same dual-layer transparency technique as other UI screens:

**Layer Structure:**
- **BG0 (Front)**: Static menu graphics with "Singleplayer", "Multiplayer", "Settings" labels
- **BG1 (Back)**: Selection highlight tiles positioned behind menu items

**Highlight System:**
1. BG0 graphics have transparent rectangular areas behind each menu item
2. BG1 contains solid-color tiles positioned under these transparent areas
3. Palette colors control visibility: BLACK (transparent/invisible) vs highlight color (visible)
4. Each menu item uses a unique tile with its own palette entry
5. Selection changes update only one palette entry (extremely efficient)

### Menu Layout

**Menu Item Positioning:**

| Item | Purpose | Tile Area | Touch Hitbox (pixels) | Tile Y Row |
|------|---------|-----------|----------------------|------------|
| Singleplayer | Start solo race | 20×3 tiles @ row 4 | x: 32-224, y: 24-64 | 4 |
| Multiplayer | Start WiFi race | 20×3 tiles @ row 10 | x: 32-224, y: 78-118 | 10 |
| Settings | Configure options | 20×3 tiles @ row 17 | x: 32-224, y: 132-172 | 17 |

**Layout Constants:**
- Menu X position: 32 pixels (left margin)
- Menu item width: 192 pixels
- Menu item height: 40 pixels
- Vertical spacing: 54 pixels
- First item Y: 24 pixels

**Highlight Tile Positioning:**
- Left edge: Column 6
- Width: 20 tiles (160 pixels)
- Height: 3 tiles (24 pixels)

### State Machine

**Selection States:**

```c
typedef enum {
    HOME_BTN_NONE = -1,         // No selection (invalid state)
    HOME_BTN_SINGLEPLAYER = 0,  // Start singleplayer mode
    HOME_BTN_MULTIPLAYER = 1,   // Start multiplayer mode
    HOME_BTN_SETTINGS = 2,      // Open settings screen
    HOME_BTN_COUNT              // Total number of buttons
} HomeButtonSelected;
```

**Default State:** No selection (HOME_BTN_NONE) when screen loads

**State Flow:**
```
Initialize → No selection, kart starts scrolling
   ↓
User navigates → Update highlight (deselect old, select new)
   ↓
User confirms (A button or touch release):
   - Singleplayer → Set singleplayer mode, transition to MAPSELECTION
   - Multiplayer → Check WiFi, initialize connection, transition to MULTIPLAYER_LOBBY or REINIT_HOME
   - Settings → Transition to SETTINGS
```

**Multiplayer Connection Flow:**
```
User selects Multiplayer
   ↓
Check WiFi enabled in settings → If disabled, play ding, stay on HOME_PAGE
   ↓
Call Multiplayer_Init() → Returns player ID or -1
   ↓
If connection failed (-1) → Return REINIT_HOME (triggers reinitialization)
If connection succeeded → Set multiplayer mode, return MULTIPLAYER_LOBBY
```

## Public API

### HomePage_Initialize

**Signature:** `void HomePage_Initialize(void)`
**Defined in:** [home_page.c:97-106](../source/ui/home_page.c#L97-L106)

Initializes the Home Page screen. Called once when transitioning to HOME_PAGE or REINIT_HOME state.

**Actions:**
1. Resets selection state to NONE
2. Configures main screen graphics (bitmap mode + sprite layer)
3. Loads top screen artwork
4. Configures and initializes animated kart sprite
5. Initializes VBlank timer for sprite animation
6. Configures sub screen graphics (dual-layer tiled mode)
7. Loads menu graphics and selection highlight tiles
8. Draws selection rectangles for all menu items (initially invisible)

```c
void HomePage_Initialize(void) {
    selected = HOME_BTN_NONE;
    lastSelected = HOME_BTN_NONE;
    HomePage_ConfigureGraphicsMain();
    HomePage_ConfigureBackgroundMain();
    HomePage_ConfigureKartSprite();
    initTimer();
    HomePage_ConfigureGraphicsSub();
    HomePage_ConfigureBackgroundSub();
}
```

**Note on REINIT_HOME:**
When multiplayer connection fails, the state machine returns REINIT_HOME, which triggers a full reinitialization of the Home Page. This ensures any partial multiplayer state is properly cleaned up before returning to the menu.

### HomePage_Update

**Signature:** `GameState HomePage_Update(void)`
**Defined in:** [home_page.c:108-154](../source/ui/home_page.c#L108-L154)

Updates the Home Page screen every frame. Handles input and returns the next game state.

**Input Handling:**
- **D-Pad Up/Down**: Cycle through menu items (Singleplayer ↔ Multiplayer ↔ Settings, with wraparound)
- **Touch**: Direct selection by touching menu item area
- **A Button**: Confirm current selection

**Return Values:**

| Return Value | Condition | Side Effects |
|-------------|-----------|--------------|
| `MAPSELECTION` | Singleplayer selected | Sets multiplayer mode to false |
| `MULTIPLAYER_LOBBY` | Multiplayer selected, WiFi enabled, connection succeeded | Initializes multiplayer connection, sets multiplayer mode to true |
| `REINIT_HOME` | Multiplayer selected, WiFi enabled, connection failed | None (triggers reinitialization on next frame) |
| `SETTINGS` | Settings selected | None |
| `HOME_PAGE` | No selection made or WiFi disabled for multiplayer | Plays ding sound if WiFi disabled |

**WiFi Check for Multiplayer:**
Before attempting multiplayer connection, the function checks if WiFi is enabled in user settings. If disabled, it plays a ding sound and stays on the home page, preventing unnecessary connection attempts.

**Multiplayer Initialization:**
When Multiplayer is selected and WiFi is enabled:
1. Calls `Multiplayer_Init()` to establish WiFi connection
2. If connection succeeds (returns player ID ≥ 0), sets multiplayer mode and transitions to lobby
3. If connection fails (returns -1), transitions to REINIT_HOME to reset the screen

```c
case HOME_BTN_MULTIPLAYER: {
    GameContext* ctx = GameContext_Get();
    if (!ctx->userSettings.wifiEnabled) {
        PlayDingSFX();  // WiFi disabled, cannot proceed
        return HOME_PAGE;
    }
    int playerID = Multiplayer_Init();
    if (playerID == -1)
        return REINIT_HOME;  // Connection failed, reinitialize
    GameContext_SetMultiplayerMode(true);
    return MULTIPLAYER_LOBBY;
}
```

### HomePage_OnVBlank

**Signature:** `void HomePage_OnVBlank(void)`
**Defined in:** [home_page.c:156-158](../source/ui/home_page.c#L156-L158)

VBlank callback for Home Page screen. Animates the kart sprite by updating its position.

**Called:** From `timerISRVblank()` when `currentGameState == HOME_PAGE` or `REINIT_HOME` (60 times per second)

**Animation Logic:**
- Updates sprite position in OAM
- Increments X position by 1 pixel per frame
- Wraps around when sprite exits right edge (x > 256 → x = -64)
- Updates OAM display

```c
void HomePage_OnVBlank(void) {
    HomePage_MoveKartSprite();
}
```

**Why VBlank:**
VBlank is the ideal time to update sprite positions, ensuring smooth animation without tearing artifacts.

### HomePage_Cleanup

**Signature:** `void HomePage_Cleanup(void)`
**Defined in:** [home_page.c:160-165](../source/ui/home_page.c#L160-L165)

Cleans up Home Page resources. Frees sprite graphics memory allocated for the animated kart.

**Called:** From `StateMachine_Cleanup()` when leaving HOME_PAGE or REINIT_HOME state

**Actions:**
1. Checks if sprite graphics are allocated
2. Frees graphics memory using `oamFreeGfx()`
3. Nulls the pointer to prevent double-free

```c
void HomePage_Cleanup(void) {
    if (homeKart.gfx) {
        oamFreeGfx(&oamMain, homeKart.gfx);
        homeKart.gfx = NULL;
    }
}
```

**Resource Management:**
This cleanup is critical for preventing memory leaks, as sprite graphics consume limited VRAM. The function is safe to call multiple times (checks for NULL).

## Private Implementation Details

### Graphics Configuration

#### HomePage_ConfigureGraphicsMain

**Signature:** `static void HomePage_ConfigureGraphicsMain(void)`
**Defined in:** [home_page.c:171-174](../source/ui/home_page.c#L171-L174)

Configures the main screen display control registers for bitmap mode.

```c
static void HomePage_ConfigureGraphicsMain(void) {
    REG_DISPCNT = MODE_5_2D | DISPLAY_BG2_ACTIVE;
    VRAM_A_CR = VRAM_ENABLE | VRAM_A_MAIN_BG;
}
```

**Display Mode:** Mode 5 (bitmap mode) with BG2 active
**VRAM:** Bank A mapped to main screen backgrounds

#### HomePage_ConfigureBackgroundMain

**Signature:** `static void HomePage_ConfigureBackgroundMain(void)`
**Defined in:** [home_page.c:176-184](../source/ui/home_page.c#L176-L184)

Sets up the main screen background layer and loads home artwork.

**BG2 Configuration:**
- Bitmap base: 0
- Size: 256×256 pixels, 8-bit indexed color
- Affine transformation: Identity matrix (no scaling/rotation)

```c
static void HomePage_ConfigureBackgroundMain(void) {
    BGCTRL[2] = BG_BMP_BASE(0) | BgSize_B8_256x256;
    dmaCopy(home_topBitmap, BG_BMP_RAM(0), home_topBitmapLen);
    dmaCopy(home_topPal, BG_PALETTE, home_topPalLen);
    REG_BG2PA = 256;  // 1.0 scale X
    REG_BG2PD = 256;  // 1.0 scale Y
    REG_BG2PB = 0;    // No shear
    REG_BG2PC = 0;    // No shear
}
```

#### HomePage_ConfigureKartSprite

**Signature:** `static void HomePage_ConfigureKartSprite(void)`
**Defined in:** [home_page.c:190-200](../source/ui/home_page.c#L190-L200)

Configures sprite system and loads the animated kart sprite.

**Sprite Configuration:**
- Size: 64×64 pixels
- Color format: 256-color indexed
- Mapping: 1D, 32-byte boundary
- Initial position: x = -64 (off-screen left), y = 120 (vertical center)

**Memory Allocation:**
- Allocates VRAM for sprite graphics
- Copies palette to SPRITE_PALETTE
- Copies tile data to allocated VRAM

```c
static void HomePage_ConfigureKartSprite(void) {
    VRAM_B_CR = VRAM_ENABLE | VRAM_B_MAIN_SPRITE;
    oamInit(&oamMain, SpriteMapping_1D_32, false);
    homeKart.id = 0;
    homeKart.x = -64;   // Start off-screen left
    homeKart.y = 120;   // Vertical center
    homeKart.gfx =
        oamAllocateGfx(&oamMain, SpriteSize_64x64, SpriteColorFormat_256Color);
    swiCopy(kart_homePal, SPRITE_PALETTE, kart_homePalLen / 2);
    swiCopy(kart_homeTiles, homeKart.gfx, kart_homeTilesLen / 2);
}
```

**HomeKartSprite Structure:**
```c
typedef struct {
    u16* gfx;  // Pointer to sprite graphics in VRAM
    int x, y;  // Sprite position
    int id;    // OAM entry index
} HomeKartSprite;
```

#### HomePage_MoveKartSprite

**Signature:** `static void HomePage_MoveKartSprite(void)`
**Defined in:** [home_page.c:202-210](../source/ui/home_page.c#L202-L210)

Updates the kart sprite position and display. Called every VBlank (60 Hz).

**Animation Algorithm:**
1. Updates OAM entry with current position
2. Increments X position by 1 pixel
3. Wraps around when sprite exits right edge (x > 256 → x = -64)
4. Updates OAM hardware registers

```c
static void HomePage_MoveKartSprite(void) {
    oamSet(&oamMain, homeKart.id, homeKart.x, homeKart.y, 0, 0, SpriteSize_64x64,
           SpriteColorFormat_256Color, homeKart.gfx, -1, false, false, false, false,
           false);
    homeKart.x++;
    if (homeKart.x > 256)
        homeKart.x = -64;
    oamUpdate(&oamMain);
}
```

**Wraparound Logic:**
- Sprite is 64 pixels wide
- Starts at x = -64 (fully off-screen left)
- Ends at x = 256 (fully off-screen right)
- Total journey: 320 pixels at 1 pixel/frame = ~5.3 seconds per cycle

#### HomePage_ConfigureGraphicsSub

**Signature:** `static void HomePage_ConfigureGraphicsSub(void)`
**Defined in:** [home_page.c:216-219](../source/ui/home_page.c#L216-L219)

Configures the sub screen display control registers for dual-layer tiled mode.

```c
static void HomePage_ConfigureGraphicsSub(void) {
    REG_DISPCNT_SUB = MODE_0_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE;
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
}
```

#### HomePage_ConfigureBackgroundSub

**Signature:** `static void HomePage_ConfigureBackgroundSub(void)`
**Defined in:** [home_page.c:221-250](../source/ui/home_page.c#L221-L250)

Sets up both sub screen background layers and loads all menu graphics.

**BG0 Setup (Front Layer - Menu Graphics):**
- 32×32 tilemap, 256-color mode
- Map base: 0, Tile base: 1
- Priority: 0 (front)
- Loads generated assets: `ds_menuPal`, `ds_menuTiles`, `ds_menuMap`

**BG1 Setup (Back Layer - Selection Highlights):**
- 32×32 tilemap, 256-color mode
- Map base: 1, Tile base: 2
- Priority: 1 (back)
- Loads 3 selection highlight tiles (one per menu item)
- Initializes palette entries to BLACK (invisible)
- Clears tile 0 (transparent)

**Initial State:**
- Draws selection rectangles for all 3 menu items
- All highlights initially invisible (palette = BLACK)

**Tile Memory Layout:**
- Tile 0: Transparent/clear tile
- Tiles 1-3: Selection highlight tiles (palettes 251-253)

### Selection Rendering

#### HomePage_DrawSelectionRect

**Signature:** `static void HomePage_DrawSelectionRect(HomeButtonSelected btn, u16 tileIndex)`
**Defined in:** [home_page.c:256-266](../source/ui/home_page.c#L256-L266)

Draws a rectangular region of selection tiles on BG1 for menu item highlighting.

**Parameters:**
- `btn`: Which menu item (HOME_BTN_SINGLEPLAYER, HOME_BTN_MULTIPLAYER, or HOME_BTN_SETTINGS)
- `tileIndex`: Which tile to use (1-3, one per menu item)

**Selection Regions (all 20×3 tiles):**

| Menu Item | Tile Area | Tile Index |
|-----------|-----------|------------|
| Singleplayer | cols 6-26, rows 4-7 | 1 (palette 251) |
| Multiplayer | cols 6-26, rows 10-13 | 2 (palette 252) |
| Settings | cols 6-26, rows 17-20 | 3 (palette 253) |

```c
static void HomePage_DrawSelectionRect(HomeButtonSelected btn, u16 tileIndex) {
    if (btn < 0 || btn >= HOME_BTN_COUNT)
        return;

    u16* map = BG_MAP_RAM_SUB(1);
    int startY = highlightTileY[btn];  // Lookup Y position from table

    for (int row = 0; row < HIGHLIGHT_TILE_HEIGHT; row++)
        for (int col = 0; col < HIGHLIGHT_TILE_WIDTH; col++)
            map[(startY + row) * 32 + (HIGHLIGHT_TILE_X + col)] = tileIndex;
}
```

**Y Position Table:**
```c
static const int highlightTileY[HOME_BTN_COUNT] = {4, 10, 17};
```

#### HomePage_SetSelectionTint

**Signature:** `static void HomePage_SetSelectionTint(int buttonIndex, bool show)`
**Defined in:** [home_page.c:268-275](../source/ui/home_page.c#L268-L275)

Controls the highlight color for a menu item by modifying its palette entry.

**Palette System:**
- Base palette index: 251 (`HOME_HL_PAL_BASE`)
- Each button uses: base + button_index (251-253)

**Colors:**
- **Show**: MENU_BUTTON_HIGHLIGHT_COLOR (bluish gray, RGB15(10, 15, 22))
- **Hide**: MENU_HIGHLIGHT_OFF_COLOR (black, invisible)

```c
static void HomePage_SetSelectionTint(int buttonIndex, bool show) {
    if (buttonIndex < 0 || buttonIndex >= HOME_BTN_COUNT)
        return;

    int paletteIndex = HOME_HL_PAL_BASE + buttonIndex;
    BG_PALETTE_SUB[paletteIndex] =
        show ? MENU_BUTTON_HIGHLIGHT_COLOR : MENU_HIGHLIGHT_OFF_COLOR;
}
```

**Efficiency:**
Changing a single palette entry (2-byte write) instantly updates all tiles using that palette across the entire 20×3 rectangle. This is far more efficient than writing 60 individual tile entries.

### Input Handling

#### HomePage_HandleDPadInput

**Signature:** `static void HomePage_HandleDPadInput(void)`
**Defined in:** [home_page.c:281-289](../source/ui/home_page.c#L281-L289)

Processes D-pad input for menu navigation.

**Controls:**
- **UP**: Previous menu item (with wraparound: Singleplayer → Settings → Multiplayer → ...)
- **DOWN**: Next menu item (with wraparound: Singleplayer → Multiplayer → Settings → Singleplayer)

**Navigation Logic:**
Uses modulo arithmetic for circular navigation through all 3 menu items.

```c
static void HomePage_HandleDPadInput(void) {
    int keys = keysDown();

    if (keys & KEY_UP)
        selected = (selected - 1 + HOME_BTN_COUNT) % HOME_BTN_COUNT;

    if (keys & KEY_DOWN)
        selected = (selected + 1) % HOME_BTN_COUNT;
}
```

#### HomePage_HandleTouchInput

**Signature:** `static void HomePage_HandleTouchInput(void)`
**Defined in:** [home_page.c:291-307](../source/ui/home_page.c#L291-L307)

Processes touchscreen input for direct menu item selection.

**Touch Validation:**
1. Check if KEY_TOUCH is held
2. Read touch coordinates
3. Iterate through menu items
4. Check if touch falls within any item's hitbox

**Hitbox Table:**
```c
static const MenuItemHitBox homeBtnHitbox[HOME_BTN_COUNT] = {
    [HOME_BTN_SINGLEPLAYER] = {32, 24, 192, 40},   // x, y, width, height
    [HOME_BTN_MULTIPLAYER] = {32, 78, 192, 40},
    [HOME_BTN_SETTINGS] = {32, 132, 192, 40},
};
```

**Hitbox Calculation Macro:**
```c
#define MENU_ITEM_ROW(i) \
    {HOME_MENU_X, HOME_MENU_Y_START + (i)*HOME_MENU_SPACING, \
     HOME_MENU_WIDTH, HOME_MENU_HEIGHT}
```

**Algorithm:**
```c
static void HomePage_HandleTouchInput(void) {
    if (!(keysHeld() & KEY_TOUCH))
        return;

    touchPosition touch;
    touchRead(&touch);

    for (int i = 0; i < HOME_BTN_COUNT; i++) {
        if (touch.px >= homeBtnHitbox[i].x &&
            touch.px < homeBtnHitbox[i].x + homeBtnHitbox[i].width &&
            touch.py >= homeBtnHitbox[i].y &&
            touch.py < homeBtnHitbox[i].y + homeBtnHitbox[i].height) {
            selected = i;
            return;
        }
    }
}
```

**Design:**
Large hitboxes (192×40 pixels) make touch selection forgiving and user-friendly.

## Usage Patterns

### Basic Flow

```c
// In state machine (main.c)
case HOME_PAGE:
case REINIT_HOME:
    return HomePage_Update();

// When transitioning TO Home Page state
static void init_state(GameState state) {
    switch (state) {
        case HOME_PAGE:
        case REINIT_HOME:
            HomePage_Initialize();
            break;
    }
}

// When transitioning FROM Home Page state
static void cleanup_state(GameState state) {
    switch (state) {
        case HOME_PAGE:
        case REINIT_HOME:
            HomePage_Cleanup();  // Free sprite graphics
            break;
    }
}
```

### Start Singleplayer

```c
// User selects Singleplayer and presses A
PlayCLICKSFX();
GameContext_SetMultiplayerMode(false);
return MAPSELECTION;  // State machine transitions to map selection
```

### Start Multiplayer (Success)

```c
// User selects Multiplayer, WiFi enabled, connection succeeds
GameContext* ctx = GameContext_Get();
if (ctx->userSettings.wifiEnabled) {
    int playerID = Multiplayer_Init();  // Returns 0 or 1
    if (playerID >= 0) {
        GameContext_SetMultiplayerMode(true);
        return MULTIPLAYER_LOBBY;  // Success, enter lobby
    }
}
```

### Start Multiplayer (Failure)

```c
// User selects Multiplayer, connection fails
int playerID = Multiplayer_Init();  // Returns -1
if (playerID == -1) {
    return REINIT_HOME;  // Trigger reinitialization
}
```

**REINIT_HOME Flow:**
```
Multiplayer_Init() fails → return REINIT_HOME
   ↓
State machine: state = REINIT_HOME
   ↓
Next frame update loop:
   - Cleanup: HomePage_Cleanup() called
   - Init: HomePage_Initialize() called (fresh start)
   - Update: HomePage_Update() called (back on menu)
```

### Start Multiplayer (WiFi Disabled)

```c
// User selects Multiplayer, WiFi disabled in settings
GameContext* ctx = GameContext_Get();
if (!ctx->userSettings.wifiEnabled) {
    PlayDingSFX();  // Audio feedback indicating cannot proceed
    return HOME_PAGE;  // Stay on home page
}
```

### Open Settings

```c
// User selects Settings and presses A
PlayCLICKSFX();
return SETTINGS;  // State machine transitions to settings screen
```

## Design Notes

- **Animated Kart**: Provides visual interest on otherwise static screen, enhances game feel
- **No Pre-selection**: Menu starts with no selection, preventing accidental choices
- **WiFi Guard**: Checks WiFi setting before attempting connection, provides clear feedback
- **REINIT_HOME Pattern**: Failed multiplayer connection triggers full reinit, ensuring clean state
- **Resource Management**: Sprite graphics properly allocated/freed, preventing VRAM leaks
- **Large Hitboxes**: Touch targets are generous (192×40 pixels), excellent for touchscreen usability
- **Dual Input**: Full support for both D-pad (traditional) and touch (modern) interaction
- **Audio Feedback**: Every interaction produces sound (click for navigation, ding for blocked action)

## Performance Notes

- **Lightweight Update**: Only processes input and updates one palette entry per frame
- **Efficient Animation**: Sprite update is single OAM write per frame (60 Hz)
- **No Allocations in Update**: All state is static, sprite allocated once during init
- **Instant Response**: Selection changes reflected immediately (single palette write)
- **Clean Transitions**: Proper cleanup prevents VRAM leaks when exiting

## Integration Points

**Dependencies:**
- **context.h/c**: Reads WiFi setting, sets multiplayer mode flag
- **multiplayer.h/c**: Initializes WiFi connection for multiplayer
- **sound.h/c**: Plays audio feedback (PlayCLICKSFX, PlayDingSFX)
- **timer.h/c**: Initializes VBlank timer for sprite animation
- **color.h**: Defines highlight colors

**Generated Assets:**
- **home_top.h**: Top screen bitmap artwork
- **kart_home.h**: Kart sprite graphics (64×64, 256-color)
- **ds_menu.h**: Bottom screen menu graphics

**State Machine Integration:**
- Initialized by `StateMachine_Init()` when entering HOME_PAGE or REINIT_HOME
- Updated by `StateMachine_Update()` every frame
- Cleaned up by `StateMachine_Cleanup()` when exiting
- Returns MAPSELECTION, MULTIPLAYER_LOBBY, REINIT_HOME, SETTINGS, or HOME_PAGE
- REINIT_HOME is special: triggers cleanup then re-initialization (fresh start)
