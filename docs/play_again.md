# Play Again Screen

## Overview

The Play Again screen is the post-race interface that appears after completing a race. It presents two large circular buttons (YES and NO) allowing players to either restart the race or return to the home menu.

The system is implemented in [play_again.c](../source/ui/play_again.c) and [play_again.h](../source/ui/play_again.h), using dual-layer graphics with selection highlighting and supporting both D-pad and touchscreen input.

**Key Features:**
- Simple YES/NO decision interface
- Color-coded highlights (blue for YES, red for NO)
- D-pad and touchscreen input support
- Automatic multiplayer cleanup when exiting
- Race timer cleanup to prevent resource leaks

## Architecture

### Graphics System

The Play Again screen uses a dual-layer 2D graphics setup on the sub screen (bottom screen):

**Layer Structure:**
- **BG0 (Priority 0)**: Front layer containing the static "Play Again?" graphics with YES/NO buttons
- **BG1 (Priority 1)**: Back layer containing selection highlight tiles that show through transparent areas

**Highlight System:**
The selection highlighting uses the same technique as other UI screens:
1. BG0 graphics have transparent circular areas around the YES/NO buttons
2. BG1 contains solid-color tiles positioned under these transparent areas
3. Palette colors control visibility: BLACK (transparent/invisible) vs colored highlight (visible)
4. YES button uses TEAL highlight, NO button uses RED highlight

### Button Layout

**Coordinate System:**

| Button | Purpose | Tile Area (cols x rows) | Touch Hitbox (pixels) |
|--------|---------|------------------------|----------------------|
| YES | Restart race | cols 6-16, rows 10-20 | x: 50-120, y: 85-175 |
| NO | Return home | cols 17-27, rows 10-20 | x: 136-206, y: 85-175 |

The buttons are circular graphics with labels, positioned side-by-side across the center of the screen.

### State Machine

**Selection States:**

```c
typedef enum {
    PA_BTN_YES = 0,      // Restart race button
    PA_BTN_NO = 1,       // Return to home button
    PA_BTN_COUNT = 2,    // Total number of buttons
    PA_BTN_NONE = -1     // No selection (invalid state)
} PlayAgainButton;
```

**Default State:** YES button is pre-selected when screen loads

**State Flow:**
```
Initialize → YES selected (highlighted blue)
   ↓
User navigates → Update highlight (deselect old, select new)
   ↓
User confirms (A button or touch) → Execute action:
   - YES → Return GAMEPLAY state (restart)
   - NO → Cleanup and return HOME_PAGE state
```

## Public API

### PlayAgain_Initialize

**Signature:** `void PlayAgain_Initialize(void)`
**Defined in:** [play_again.c:74-79](../source/ui/play_again.c#L74-L79)

Initializes the Play Again screen. Called once when transitioning to PLAYAGAIN state.

**Actions:**
1. Resets selection state to YES
2. Configures sub screen display registers
3. Loads graphics assets (BG0 and BG1)
4. Sets up selection highlight tiles
5. Shows YES button as selected by default

```c
void PlayAgain_Initialize(void) {
    selected = PA_BTN_YES;
    lastSelected = PA_BTN_NONE;
    PlayAgain_ConfigureGraphics();
    PlayAgain_ConfigureBackground();
}
```

### PlayAgain_Update

**Signature:** `GameState PlayAgain_Update(void)`
**Defined in:** [play_again.c:81-119](../source/ui/play_again.c#L81-L119)

Updates the Play Again screen every frame. Handles input and returns the next game state.

**Input Handling:**
- **D-Pad Left**: Select YES
- **D-Pad Right**: Select NO
- **D-Pad Up/Down**: Toggle between YES and NO
- **Touch**: Direct selection by touching button area
- **A Button**: Confirm current selection
- **SELECT Button**: Quick exit to home (same as NO)

**Return Values:**

| Return Value | Condition | Side Effects |
|-------------|-----------|--------------|
| `GAMEPLAY` | YES selected and confirmed | None (race restarts with same settings) |
| `HOME_PAGE` | NO selected and confirmed OR SELECT pressed | Stops race timers, cleans up multiplayer |
| `PLAYAGAIN` | No action taken | None (stay on screen) |

**Critical Cleanup:**
When exiting to home, the function calls `PlayAgain_CleanupAndExit()` which:
1. Stops race timers (`RaceTick_TimerStop()`) - MUST happen before multiplayer cleanup
2. Cleans up multiplayer connection if in multiplayer mode
3. Sets multiplayer mode to false in context

```c
// Example: User presses NO button
if (keysUp() & (KEY_A | KEY_TOUCH)) {
    switch (selected) {
        case PA_BTN_NO:
            PlayCLICKSFX();
            PlayAgain_CleanupAndExit();  // Stop timers, cleanup multiplayer
            return HOME_PAGE;
    }
}
```

### PlayAgain_OnVBlank

**Signature:** `void PlayAgain_OnVBlank(void)`
**Defined in:** [play_again.c:121-123](../source/ui/play_again.c#L121-L123)

VBlank callback for the Play Again screen. Currently a no-op but reserved for future animations.

**Called:** From `timerISRVblank()` when `currentGameState == PLAYAGAIN`

## Private Implementation Details

### Graphics Configuration

#### PlayAgain_ConfigureGraphics

**Signature:** `static void PlayAgain_ConfigureGraphics(void)`
**Defined in:** [play_again.c:129-132](../source/ui/play_again.c#L129-L132)

Configures the sub screen display control registers.

```c
static void PlayAgain_ConfigureGraphics(void) {
    REG_DISPCNT_SUB = MODE_0_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE;
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
}
```

#### PlayAgain_ConfigureBackground

**Signature:** `static void PlayAgain_ConfigureBackground(void)`
**Defined in:** [play_again.c:134-164](../source/ui/play_again.c#L134-L164)

Sets up both background layers and loads all graphics assets.

**BG0 Setup (Front Layer):**
- 32x32 tilemap mode
- Map base: 0, Tile base: 1
- Loads palette, tiles, and map from `playagain.h` generated assets

**BG1 Setup (Back Layer):**
- 32x32 tilemap mode, 256-color
- Map base: 1, Tile base: 3
- Loads two selection tiles (YES and NO) at tile indices 0 and 1
- Initializes palette colors to BLACK (transparent)

**Initial State:**
- Draws selection rectangles for both buttons using `PlayAgain_DrawSelectionRect()`
- Highlights YES button by default using `PlayAgain_SetSelectionTint()`

### Selection Rendering

#### PlayAgain_DrawSelectionRect

**Signature:** `static void PlayAgain_DrawSelectionRect(PlayAgainButton btn, u16 tileIndex)`
**Defined in:** [play_again.c:166-192](../source/ui/play_again.c#L166-L192)

Draws a rectangular region of tiles on BG1 for button highlighting.

**Parameters:**
- `btn`: Which button (PA_BTN_YES or PA_BTN_NO)
- `tileIndex`: Which tile to use (0 for YES, 1 for NO)

**Tile Regions:**
- YES: 10×10 tiles (cols 6-16, rows 10-20)
- NO: 10×10 tiles (cols 17-27, rows 10-20)

```c
// YES button region
startX = 6;   startY = 10;
endX = 16;    endY = 20;

// NO button region
startX = 17;  startY = 10;
endX = 27;    endY = 20;
```

#### PlayAgain_SetSelectionTint

**Signature:** `static void PlayAgain_SetSelectionTint(PlayAgainButton btn, bool show)`
**Defined in:** [play_again.c:194-206](../source/ui/play_again.c#L194-L206)

Controls the highlight color for a button by modifying its palette entry.

**Palette System:**
- Base palette index: 240 (`PA_SELECTION_PAL_BASE`)
- YES uses palette 240, NO uses palette 241

**Colors:**
- **Show YES**: TEAL (RGB15(0, 20, 31)) - Blue/cyan color
- **Show NO**: RED (RGB15(31, 0, 0)) - Red color
- **Hide**: BLACK (RGB15(0, 0, 0)) - Transparent

```c
// Simplified logic
if (show) {
    BG_PALETTE_SUB[paletteIndex] = (btn == PA_BTN_YES) ? TEAL : RED;
} else {
    BG_PALETTE_SUB[paletteIndex] = BLACK;
}
```

### Input Handling

#### PlayAgain_HandleDPadInput

**Signature:** `static void PlayAgain_HandleDPadInput(void)`
**Defined in:** [play_again.c:212-223](../source/ui/play_again.c#L212-L223)

Processes D-pad input for button selection.

**Controls:**
- **LEFT**: Select YES
- **RIGHT**: Select NO
- **UP/DOWN**: Toggle between YES and NO (accessibility feature)

```c
if (keys & KEY_LEFT)
    selected = PA_BTN_YES;
if (keys & KEY_RIGHT)
    selected = PA_BTN_NO;
if (keys & (KEY_UP | KEY_DOWN))
    selected = (selected == PA_BTN_YES) ? PA_BTN_NO : PA_BTN_YES;
```

#### PlayAgain_HandleTouchInput

**Signature:** `static void PlayAgain_HandleTouchInput(void)`
**Defined in:** [play_again.c:225-247](../source/ui/play_again.c#L225-L247)

Processes touchscreen input for direct button selection.

**Touch Validation:**
1. Check if KEY_TOUCH is held
2. Validate touch coordinates are within screen bounds (0-256 x 0-192)
3. Check if touch falls within button hitbox

**Hitboxes:**
- YES: x ∈ [50, 120], y ∈ [85, 175] (covers circle + label)
- NO: x ∈ [136, 206], y ∈ [85, 175] (covers circle + label)

### Cleanup

#### PlayAgain_CleanupAndExit

**Signature:** `static void PlayAgain_CleanupAndExit(void)`
**Defined in:** [play_again.c:253-261](../source/ui/play_again.c#L253-L261)

Performs critical cleanup when exiting to home menu. This function prevents resource leaks and ensures proper multiplayer disconnection.

**Cleanup Order (CRITICAL):**
1. **Stop race timers first**: Calls `RaceTick_TimerStop()`
   - Stops TIMER0 (physics updates)
   - Stops TIMER1 (chronometer)
   - Prevents ISRs from running during multiplayer cleanup
2. **Clean up multiplayer** (if applicable):
   - Calls `Multiplayer_Cleanup()` to disconnect WiFi
   - Sets multiplayer mode to false in context

**Why Order Matters:**
The timers must be stopped BEFORE multiplayer cleanup. If timers continue running, they may access multiplayer state while it's being torn down, causing race conditions or crashes.

```c
static void PlayAgain_CleanupAndExit(void) {
    RaceTick_TimerStop();  // CRITICAL: Stop timers first

    if (GameContext_IsMultiplayerMode()) {
        Multiplayer_Cleanup();
        GameContext_SetMultiplayerMode(false);
    }
}
```

## Usage Patterns

### Basic Flow

```c
// In state machine (main.c)
case PLAYAGAIN:
    return PlayAgain_Update();

// When transitioning TO PlayAgain state
static void init_state(GameState state) {
    switch (state) {
        case PLAYAGAIN:
            PlayAgain_Initialize();
            break;
    }
}
```

### Restart Race

When user selects YES:
```c
// User presses A or touches screen while YES selected
PlayCLICKSFX();
return GAMEPLAY;  // State machine restarts race with same settings
```

### Exit to Home

When user selects NO or presses SELECT:
```c
PlayCLICKSFX();
PlayAgain_CleanupAndExit();  // Clean up timers and multiplayer
return HOME_PAGE;  // State machine transitions to home
```

### Multiplayer Session Handling

The Play Again screen intelligently handles multiplayer cleanup:

```c
// Singleplayer: Only stops timers
if (!GameContext_IsMultiplayerMode()) {
    RaceTick_TimerStop();
    // No multiplayer cleanup needed
}

// Multiplayer: Stops timers AND disconnects
if (GameContext_IsMultiplayerMode()) {
    RaceTick_TimerStop();           // Stop first
    Multiplayer_Cleanup();          // Then cleanup network
    GameContext_SetMultiplayerMode(false);  // Clear flag
}
```

## Design Notes

- **Pre-selected YES**: Defaults to YES button to encourage replaying (positive reinforcement)
- **Color Psychology**: Blue (YES) is calming/affirmative, red (NO) is stopping/negative
- **Quick Exit**: SELECT button provides fast path to home without navigating
- **Multiplayer Safety**: Cleanup function ensures proper resource release
- **No VBlank Logic**: Currently no animations, but structure allows future enhancements
- **Dual Input**: Full support for both D-pad (traditional) and touch (modern) input
- **Critical Ordering**: Timer stop before multiplayer cleanup is non-negotiable for stability

## Performance Notes

- **Lightweight**: Only processes input and updates one palette entry per frame
- **No Allocations**: All state is static, no dynamic memory
- **Instant Response**: Selection changes reflected immediately (single palette write)
- **Clean Transitions**: Proper cleanup prevents memory leaks in multiplayer sessions
