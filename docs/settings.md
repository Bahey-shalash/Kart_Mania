# Settings Screen

## Overview

The Settings screen provides an interactive configuration interface for game preferences including WiFi/multiplayer connectivity, background music, and sound effects. It features visual toggle indicators (green pill = ON, red pill = OFF) with both touch and D-pad input support, persistent storage capability, and a factory reset function.

The system is implemented in [settings.c](../source/ui/settings.c) and [settings.h](../source/ui/settings.h), using dual-screen graphics with a bitmap top screen and multi-layer bottom screen for interactive controls.

**Key Features:**
- Visual toggle pills showing ON (green) / OFF (red) states
- Dual input support: D-pad navigation and direct touch selection
- Persistent storage integration (save settings to flash memory)
- Factory reset by holding START+SELECT while pressing Save
- Immediate audio feedback (settings applied instantly)
- Gray selection highlighting for current focus

## Architecture

### Dual-Screen Layout

**Top Screen (Main):**
- Mode 5 bitmap graphics (BG2)
- 256×256 8-bit indexed color bitmap
- Static artwork showing settings header and decorative elements
- No interactivity

**Bottom Screen (Sub):**
- Mode 0 tiled graphics (BG0 + BG1)
- BG0 (Priority 0): Front layer with static setting labels and button graphics
- BG1 (Priority 1): Back layer for dynamic toggle pills and selection highlights

### Graphics System

The Settings screen uses a sophisticated three-layer rendering approach on the sub screen:

**Layer Structure:**
- **BG0 (Front)**: Static graphics with text labels ("WiFi", "Music", "Sound FX") and button outlines
- **BG1 (Back)**: Dynamic layer with two separate tile systems:
  - Toggle pills (tiles 3-4): Red/green rectangles showing ON/OFF states
  - Selection highlights (tiles 5-10): Gray rectangles for current selection

**Transparency Technique:**
The BG0 graphics have transparent areas where:
1. Toggle pill regions are transparent, allowing BG1 toggle tiles to show through
2. Selection regions are transparent, allowing BG1 highlight tiles to show through
3. BG1 tiles are drawn behind these transparent areas
4. Palette color changes control visibility (color = visible, BLACK = invisible)

**Toggle Pill System:**
- **Tile Indices**: TILE_RED (3), TILE_GREEN (4)
- **Palette Indices**: 254 (red), 255 (green)
- **Colors**: TOGGLE_OFF_COLOR (dark red), TOGGLE_ON_COLOR (dark green)
- **Location**: Right side of screen (columns 21-29)

**Selection Highlight System:**
- **Tile Indices**: 5-10 (one per button: WiFi, Music, Sound FX, Save, Back, Home)
- **Palette Base**: 244 (sequential: 244=WiFi, 245=Music, 246=Sound FX, 247=Save, 248=Back, 249=Home)
- **Color**: SETTINGS_SELECT_COLOR (gray) when selected, BLACK when not

### Button Layout

**Setting Toggles (Top Section):**

| Button | Purpose | Selection Area (tiles) | Touch Hitbox (pixels) | Toggle Pill Location |
|--------|---------|------------------------|----------------------|---------------------|
| WiFi | Enable/disable multiplayer | cols 2-7, rows 1-4 | Label: 23-53, 10-25<br>Pill: 175-240, 10-37 | rows 1-5 |
| Music | Enable/disable BGM | cols 2-9, rows 5-8 | Label: 24-69, 40-55<br>Pill: 175-240, 40-67 | rows 5-9 |
| Sound FX | Enable/disable SFX | cols 2-13, rows 9-12 | Label: 23-99, 70-85<br>Pill: 175-240, 70-97 | rows 9-13 |

**Action Buttons (Bottom Section):**

| Button | Purpose | Selection Area (tiles) | Touch Hitbox (pixels) |
|--------|---------|------------------------|----------------------|
| Save | Save settings to storage | cols 4-14, rows 15-23 | 40-88, 128-176 |
| Back | Return to home menu | cols 12-20, rows 15-23 | 104-152, 128-176 |
| Home | Return to home menu | cols 20-28, rows 15-23 | 168-216, 128-176 |

### State Machine

**Selection States:**

```c
typedef enum {
    SETTINGS_BTN_NONE = -1,    // No selection (invalid state)
    SETTINGS_BTN_WIFI = 0,     // WiFi toggle
    SETTINGS_BTN_MUSIC = 1,    // Music toggle
    SETTINGS_BTN_SOUND_FX = 2, // Sound effects toggle
    SETTINGS_BTN_SAVE = 3,     // Save button
    SETTINGS_BTN_BACK = 4,     // Back button
    SETTINGS_BTN_HOME = 5,     // Home button
    SETTINGS_BTN_COUNT         // Total number of buttons
} SettingsButtonSelected;
```

**Default State:** No selection (SETTINGS_BTN_NONE) when screen loads

**State Flow:**
```
Initialize → No selection
   ↓
User navigates → Update highlight (deselect old, select new)
   ↓
User confirms (A button or touch release):
   - Toggle buttons → Flip state, update visual, play sound
- Save button → Save to storage (or factory reset if START+SELECT held)
   - Back/Home → Return to HOME_PAGE state
```

**Navigation:**
- **UP/DOWN**: Cycle through all 6 buttons sequentially (with wraparound)
- **LEFT/RIGHT**: Navigate horizontally between bottom row buttons (Save ↔ Back ↔ Home, with wraparound)
- **Touch**: Direct selection by touching any button area

## Public API

### Settings_Initialize

**Signature:** `void Settings_Initialize(void)`
**Defined in:** [settings.c:76-83](../source/ui/settings.c#L76-L83)

Initializes the Settings screen. Called once when transitioning to SETTINGS state.

**Actions:**
1. Resets selection state to NONE
2. Configures main screen display (bitmap mode)
3. Loads top screen artwork
4. Configures sub screen display (tiled mode, dual-layer)
5. Loads all graphics assets (menu graphics, toggle tiles, selection tiles)
6. Draws initial toggle pill states based on current settings
7. Draws selection rectangles for all buttons (initially invisible)

```c
void Settings_Initialize(void) {
    selected = SETTINGS_BTN_NONE;
    lastSelected = SETTINGS_BTN_NONE;
    Settings_ConfigureGraphicsMain();
    Settings_ConfigureBackgroundMain();
    Settings_ConfigureGraphicsSub();
    Settings_ConfigureBackgroundSub();
}
```

### Settings_Update

**Signature:** `GameState Settings_Update(void)`
**Defined in:** [settings.c:85-147](../source/ui/settings.c#L85-L147)

Updates the Settings screen every frame. Handles input and returns the next game state.

**Input Handling:**
- **D-Pad Up/Down**: Cycle through all 6 buttons
- **D-Pad Left/Right**: Navigate between bottom row buttons (Save/Back/Home only)
- **Touch**: Direct selection by touching button areas (both labels and pills)
- **A Button**: Activate selected button (toggle setting or perform action)
- **START+SELECT held on Save**: Factory reset all settings to defaults

**Return Values:**

| Return Value | Condition | Side Effects |
|-------------|-----------|--------------|
| `HOME_PAGE` | Back or Home button pressed | None |
| `SETTINGS` | No navigation action | May toggle settings, update visuals, save to storage |

**Setting Toggle Behavior:**
When toggling WiFi, Music, or Sound FX:
1. Reads current setting state from context
2. Inverts the state
3. Calls `GameContext_Set*Enabled()` to update context and apply side effect
4. Calls `Settings_DrawToggleRect()` to update visual pill color
5. Plays confirmation sound (ding for toggles, click for navigation)

**Save Button Behavior:**
- **Normal press (A or touch release)**: Saves current settings to persistent storage via `Storage_SaveSettings()`
- **START+SELECT held during press**: Factory reset via `Storage_ResetToDefaults()`, then refreshes UI to show default states

**Critical Design Choice:**
Sound FX toggle plays the ding sound BEFORE potentially muting, ensuring user always hears feedback.

```c
// Example: User toggles Sound FX
case SETTINGS_BTN_SOUND_FX: {
    bool soundFxShouldBeEnabled = !ctx->userSettings.soundFxEnabled;
    PlayDingSFX();  // Play BEFORE muting
    GameContext_SetSoundFxEnabled(soundFxShouldBeEnabled);
    Settings_DrawToggleRect(SETTINGS_BTN_SOUND_FX, soundFxShouldBeEnabled);
    break;
}
```

## Private Implementation Details

### Graphics Configuration

#### Settings_ConfigureGraphicsMain

**Signature:** `static void Settings_ConfigureGraphicsMain(void)`
**Defined in:** [settings.c:179-182](../source/ui/settings.c#L179-L182)

Configures the main screen display control registers for bitmap mode.

```c
static void Settings_ConfigureGraphicsMain(void) {
    REG_DISPCNT = MODE_5_2D | DISPLAY_BG2_ACTIVE;
    VRAM_A_CR = VRAM_ENABLE | VRAM_A_MAIN_BG;
}
```

#### Settings_ConfigureBackgroundMain

**Signature:** `static void Settings_ConfigureBackgroundMain(void)`
**Defined in:** [settings.c:184-192](../source/ui/settings.c#L184-L192)

Sets up the main screen background layer and loads artwork.

**BG2 Configuration:**
- Bitmap base: 0
- Size: 256×256 pixels, 8-bit indexed color
- Affine transformation: Identity matrix (no scaling/rotation)

```c
static void Settings_ConfigureBackgroundMain(void) {
    BGCTRL[2] = BG_BMP_BASE(0) | BgSize_B8_256x256;
    dmaCopy(settings_topBitmap, BG_BMP_RAM(0), settings_topBitmapLen);
    dmaCopy(settings_topPal, BG_PALETTE, settings_topPalLen);
    REG_BG2PA = 256;  // 1.0 scale X
    REG_BG2PD = 256;  // 1.0 scale Y
    REG_BG2PB = 0;    // No shear
    REG_BG2PC = 0;    // No shear
}
```

#### Settings_ConfigureGraphicsSub

**Signature:** `static void Settings_ConfigureGraphicsSub(void)`
**Defined in:** [settings.c:289-292](../source/ui/settings.c#L289-L292)

Configures the sub screen display control registers for dual-layer tiled mode.

```c
static void Settings_ConfigureGraphicsSub(void) {
    REG_DISPCNT_SUB = MODE_0_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE;
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
}
```

#### Settings_ConfigureBackgroundSub

**Signature:** `static void Settings_ConfigureBackgroundSub(void)`
**Defined in:** [settings.c:294-339](../source/ui/settings.c#L294-L339)

Sets up both sub screen background layers and loads all interactive graphics. This is the most complex setup function.

**BG0 Setup (Front Layer - Static Graphics):**
- 32×32 tilemap, 256-color mode
- Map base: 0, Tile base: 1
- Priority: 0 (front)
- Loads generated assets: `nds_settingsPal`, `nds_settingsTiles`, `nds_settingsMap`

**BG1 Setup (Back Layer - Dynamic Graphics):**
- 32×32 tilemap, 256-color mode
- Map base: 1, Tile base: 2
- Priority: 1 (back)
- Loads toggle tiles at indices 3-4 (red/green)
- Loads selection tiles at indices 5-10 (one per button)
- Sets palette colors for toggles (254=red, 255=green)
- Clears map to all zeros (transparent)

**Initial State Setup:**
1. Reads current settings from context
2. Draws toggle pills for WiFi/Music/Sound FX matching current states
3. Draws selection rectangles for all 6 buttons (initially invisible)

**Tile Memory Layout:**
- Tiles 0-2: Unused
- Tile 3: Red toggle pill (palette 254)
- Tile 4: Green toggle pill (palette 255)
- Tiles 5-10: Selection highlights (palettes 244-249)

### Toggle Rendering

#### Settings_DrawToggleRect

**Signature:** `static void Settings_DrawToggleRect(SettingsButtonSelected btn, bool enabled)`
**Defined in:** [settings.c:198-222](../source/ui/settings.c#L198-L222)

Draws a toggle pill indicator for WiFi, Music, or Sound FX settings.

**Parameters:**
- `btn`: Which setting (SETTINGS_BTN_WIFI, SETTINGS_BTN_MUSIC, or SETTINGS_BTN_SOUND_FX)
- `enabled`: true for green pill (ON), false for red pill (OFF)

**Pill Regions (all at startX=21, width=9 tiles):**
- WiFi: rows 1-5
- Music: rows 5-9
- Sound FX: rows 9-13

**Algorithm:**
1. Determine which tile to use (TILE_GREEN if enabled, TILE_RED if not)
2. Look up row range for the button
3. Fill 9×4 tile region with the selected tile

```c
// Example: WiFi toggle at position (21, 1) with size 9×4
for (int row = 1; row < 5; row++)
    for (int col = 0; col < 9; col++)
        map[row * 32 + (21 + col)] = enabled ? TILE_GREEN : TILE_RED;
```

### Selection Rendering

#### Settings_DrawSelectionRect

**Signature:** `static void Settings_DrawSelectionRect(SettingsButtonSelected btn, u16 tileIndex)`
**Defined in:** [settings.c:228-276](../source/ui/settings.c#L228-L276)

Draws a rectangular region of selection tiles on BG1 for button highlighting.

**Parameters:**
- `btn`: Which button (SETTINGS_BTN_WIFI through SETTINGS_BTN_HOME)
- `tileIndex`: Which tile to use (5-10, one per button)

**Selection Regions:**

| Button | Tile Area (cols × rows) | Size |
|--------|------------------------|------|
| WiFi | 2-7, 1-4 | 5×3 tiles |
| Music | 2-9, 5-8 | 7×3 tiles |
| Sound FX | 2-13, 9-12 | 11×3 tiles |
| Save | 4-14, 15-23 | 10×8 tiles |
| Back | 12-20, 15-23 | 8×8 tiles |
| Home | 20-28, 15-23 | 8×8 tiles |

**Why Different Tiles per Button:**
Each button uses a unique tile index (5-10) mapped to its own palette entry (244-249). This allows independent color control: when a button is selected, only its palette entry changes to gray; others remain black (invisible).

#### Settings_SetSelectionTint

**Signature:** `static void Settings_SetSelectionTint(SettingsButtonSelected btn, bool show)`
**Defined in:** [settings.c:278-283](../source/ui/settings.c#L278-L283)

Controls the highlight color for a button by modifying its palette entry.

**Palette System:**
- Base palette index: 244 (`SETTINGS_SELECTION_PAL_BASE`)
- Each button uses: base + button_index (244-249)

**Colors:**
- **Show**: SETTINGS_SELECT_COLOR (gray, RGB15(20, 20, 20))
- **Hide**: BLACK (RGB15(0, 0, 0)) - appears as transparent

```c
static void Settings_SetSelectionTint(SettingsButtonSelected btn, bool show) {
    if (btn < 0 || btn >= SETTINGS_BTN_COUNT)
        return;
    int paletteIndex = SETTINGS_SELECTION_PAL_BASE + btn;
    BG_PALETTE_SUB[paletteIndex] = show ? SETTINGS_SELECT_COLOR : BLACK;
}
```

**Why This Works:**
All selection tiles for a given button reference the same palette index. Changing that single palette entry instantly updates all tiles for that button across the entire rectangle, making selection highlighting very efficient (single palette write vs. hundreds of tile writes).

### Input Handling

#### Settings_HandleDPadInput

**Signature:** `static void Settings_HandleDPadInput(void)`
**Defined in:** [settings.c:345-371](../source/ui/settings.c#L345-L371)

Processes D-pad input for button navigation.

**Controls:**
- **UP**: Previous button (with wraparound: WiFi → Home → Sound FX → ...)
- **DOWN**: Next button (with wraparound: WiFi → Music → Sound FX → Save → Back → Home → WiFi)
- **LEFT**: Navigate left on bottom row (Save ← Back ← Home, with wraparound)
- **RIGHT**: Navigate right on bottom row (Save → Back → Home, with wraparound)

**Navigation Logic:**
- UP/DOWN use modulo arithmetic for circular navigation
- LEFT/RIGHT only affect bottom row buttons (Save/Back/Home), forming a separate navigation group

```c
// UP/DOWN: Cycle through all buttons
if (keys & KEY_UP)
    selected = (selected - 1 + SETTINGS_BTN_COUNT) % SETTINGS_BTN_COUNT;
if (keys & KEY_DOWN)
    selected = (selected + 1) % SETTINGS_BTN_COUNT;

// LEFT/RIGHT: Navigate bottom row with wraparound
if (keys & KEY_LEFT) {
    if (selected == SETTINGS_BTN_SAVE)
        selected = SETTINGS_BTN_HOME;  // Wrap left
    else if (selected == SETTINGS_BTN_BACK)
        selected = SETTINGS_BTN_SAVE;
    else if (selected == SETTINGS_BTN_HOME)
        selected = SETTINGS_BTN_BACK;
}
```

#### Settings_HandleTouchInput

**Signature:** `static void Settings_HandleTouchInput(void)`
**Defined in:** [settings.c:373-437](../source/ui/settings.c#L373-L437)

Processes touchscreen input for direct button selection.

**Touch Validation:**
1. Check if KEY_TOUCH is held
2. Read touch coordinates
3. Validate coordinates are within screen bounds (0-256 × 0-192)
4. Check if touch falls within any button hitbox

**Hitbox Design:**
For toggle settings, two separate hitboxes:
- Text label (left side): Small rectangle over the setting name
- Toggle pill (right side): Larger rectangle over the ON/OFF pill

For action buttons, circular hitboxes are approximated with rectangles.

**Example Hitboxes:**
```c
// WiFi has two touch zones
if (touch.px > 23 && touch.px < 53 && touch.py > 10 && touch.py < 25) {
    selected = SETTINGS_BTN_WIFI;  // Text label
}
if (touch.px > 175 && touch.px < 240 && touch.py > 10 && touch.py < 37) {
    selected = SETTINGS_BTN_WIFI;  // Toggle pill
}
```

**Design Rationale:**
Large, overlapping hitboxes make the interface more forgiving and user-friendly on a small touchscreen.

### Settings Management

#### Settings_RefreshUI

**Signature:** `static void Settings_RefreshUI(void)`
**Defined in:** [settings.c:152-164](../source/ui/settings.c#L152-L164)

Refreshes the settings UI to match the current context state. Used after factory reset to update all visuals and apply settings.

**Actions:**
1. Reads all settings from context
2. Updates all three toggle pill visuals
3. Calls `GameContext_Set*Enabled()` for each setting to trigger side effects

**Purpose:**
Ensures the UI accurately reflects the context state, particularly important after loading settings from storage or performing factory reset.

```c
static void Settings_RefreshUI(void) {
    GameContext* ctx = GameContext_Get();

    // Update visuals
    Settings_DrawToggleRect(SETTINGS_BTN_WIFI, ctx->userSettings.wifiEnabled);
    Settings_DrawToggleRect(SETTINGS_BTN_MUSIC, ctx->userSettings.musicEnabled);
    Settings_DrawToggleRect(SETTINGS_BTN_SOUND_FX, ctx->userSettings.soundFxEnabled);

    // Apply settings (music starts/stops, sound mutes/unmutes)
    GameContext_SetWifiEnabled(ctx->userSettings.wifiEnabled);
    GameContext_SetMusicEnabled(ctx->userSettings.musicEnabled);
    GameContext_SetSoundFxEnabled(ctx->userSettings.soundFxEnabled);
}
```

#### Settings_OnSavePressed

**Signature:** `static void Settings_OnSavePressed(void)`
**Defined in:** [settings.c:166-173](../source/ui/settings.c#L166-L173)

Handles the Save button press, supporting both normal save and factory reset.

**Behavior:**
- **Normal case**: Calls `Storage_SaveSettings()` to write current settings to flash memory
- **Factory reset**: If START and SELECT are both held, calls `Storage_ResetToDefaults()` then `Settings_RefreshUI()`

**Factory Reset Sequence:**
1. `Storage_ResetToDefaults()` resets context to defaults and saves to storage
2. `Settings_RefreshUI()` updates all toggle visuals and applies settings

```c
static void Settings_OnSavePressed(void) {
    if ((keysHeld() & KEY_START) && (keysHeld() & KEY_SELECT)) {
        Storage_ResetToDefaults();  // Reset and save
        Settings_RefreshUI();        // Update UI
    } else {
        Storage_SaveSettings();      // Normal save
    }
}
```

**User Experience:**
The factory reset combo (holding START+SELECT while activating Save) requires deliberate input, preventing accidental resets while remaining discoverable for power users.

## Usage Patterns

### Basic Flow

```c
// In state machine (main.c)
case SETTINGS:
    return Settings_Update();

// When transitioning TO Settings state
static void init_state(GameState state) {
    switch (state) {
        case SETTINGS:
            Settings_Initialize();
            break;
    }
}
```

### Toggle Setting

When user toggles a setting:
```c
// User presses A while WiFi is selected
bool wifiShouldBeEnabled = !ctx->userSettings.wifiEnabled;
GameContext_SetWifiEnabled(wifiShouldBeEnabled);  // Updates context + side effect
Settings_DrawToggleRect(SETTINGS_BTN_WIFI, wifiShouldBeEnabled);  // Updates visual
PlayDingSFX();  // Audio feedback
```

**Immediate Application:**
Settings changes take effect instantly. Toggling music ON immediately starts playback; toggling sound FX OFF immediately mutes effects.

### Save Settings

```c
// User presses A on Save button (normal save)
Storage_SaveSettings();  // Writes context to flash memory
PlayDingSFX();
// Stay on settings screen
```

### Factory Reset

```c
// User holds START+SELECT while activating Save (A button or touch release)
Storage_ResetToDefaults();  // Context reset + flash write
Settings_RefreshUI();        // Visuals updated + settings applied
PlayDingSFX();
// Stay on settings screen, now showing default states
```

### Exit to Home

```c
// User presses Back or Home button
PlayCLICKSFX();
return HOME_PAGE;  // State machine transitions to home
```

**Note:** Exiting does NOT automatically save. Users must explicitly press Save to persist changes.

## Design Notes

- **Visual Feedback**: Toggle pills provide instant visual confirmation of setting states
- **Audio Feedback**: Every interaction produces sound (ding for toggles, click for navigation)
- **Dual Input**: Full support for both D-pad (traditional) and touch (modern) interaction
- **Color Coding**: Green = ON, Red = OFF follows universal convention
- **Large Touch Targets**: Hitboxes are generous, making touch input forgiving
- **Explicit Save**: Changes are not auto-saved, giving users control over persistence
- **Hidden Reset**: Factory reset requires deliberate combo, preventing accidents
- **Sound FX Smart Toggle**: Plays feedback before potentially muting, ensuring users always hear confirmation

## Performance Notes

- **Lightweight**: Only processes input and updates one palette entry per frame for selection
- **Efficient Toggles**: Pills updated via simple tile replacement (2 dmaCopy calls worst case)
- **No Allocations**: All state is static, no dynamic memory
- **Instant Response**: Selection changes reflected immediately (single palette write)
- **Minimal Redraws**: Only changed elements redrawn, not full screen refresh

## Integration Points

**Dependencies:**
- **context.h/c**: Reads/writes all settings to global context
- **storage.h/c**: Persists settings to flash memory
- **sound.h/c**: Plays audio feedback (PlayDingSFX, PlayCLICKSFX)
- **color.h**: Defines all color constants used

**Generated Assets:**
- **nds_settings.h**: Bottom screen menu graphics (labels, buttons)
- **settings_top.h**: Top screen decorative artwork

**State Machine Integration:**
- Initialized by `StateMachine_Init()` when entering SETTINGS state
- Updated by `StateMachine_Update()` every frame
- Returns HOME_PAGE to exit, SETTINGS to stay


---

## Navigation

- [← Back to Wiki](wiki.md)
- [← Back to README](../README.md)