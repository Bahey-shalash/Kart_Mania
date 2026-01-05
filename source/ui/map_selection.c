/**
 * File: map_selection.c
 * ----------------------
 * Description: Implementation of map selection screen. Uses dual-layer graphics with
 *              transparency to create dynamic selection highlights. The sub screen has
 *              a menu UI (BG0) with transparent areas, and selection tiles (BG1) underneath
 *              that change color to indicate which map is selected. The main screen shows
 *              map thumbnails with animated clouds scrolling over them.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 05.01.2026
 *
 * Graphics Architecture:
 *
 *   Main Screen (Top):
 *     - BG0: Combined tilemap with all three map thumbnails (priority 1, back layer)
 *     - BG1: Scrolling clouds (priority 0, front layer)
 *     - Clouds scroll horizontally at 0.5 px/frame creating atmospheric effect
 *
 *   Sub Screen (Bottom):
 *     - BG0: Menu UI with map names and buttons (priority 0, front layer)
 *            This layer has TRANSPARENT AREAS where highlights should appear
 *     - BG1: Selection highlight tiles (priority 1, back layer)
 *            Solid-color tiles positioned exactly under transparent areas
 *            Color controlled via palette: BLACK (invisible) or WHITE (visible)
 *
 * Selection Highlight System:
 *   1. Four selection tiles created at palette indices 240-243 (one per button)
 *   2. Each tile is 8x8 pixels of solid color (64 bytes, all same palette index)
 *   3. Tiles drawn on BG1 map to cover selection areas (using drawSelectionRect)
 *   4. Palette starts as BLACK (tiles invisible through transparent BG0)
 *   5. On selection, palette changes to WHITE (tiles become visible)
 *   6. This creates instant highlight without redrawing front layer graphics
 *
 * Cloud Animation:
 *   - cloudSubPixel counter increments every frame
 *   - Every 2 frames, cloudOffset increments (achieving 0.5 px/frame movement)
 *   - cloudOffset wraps at 256 to prevent overflow
 *   - REG_BG1HOFS hardware register updated in VBlank for smooth scrolling
 *
 * Input Handling:
 *   - D-Pad: Up/Down cycles through all 4 buttons with wraparound
 *            Left/Right navigates between adjacent map buttons
 *   - Touch: Touch detection uses pixel hitboxes for each map thumbnail
 *            Coordinates compared against hardcoded bounds
 *   - A button or touch release: Confirms selection and transitions to next state
 *
 * Map Button Layout (Sub Screen):
 *   - MAP1 (Scorching Sands): Left thumbnail, selection area rows 9-21, cols 2-12
 *   - MAP2 (Alpine Rush): Center thumbnail, selection area rows 9-21, cols 11-21
 *   - MAP3 (Neon Circuit): Right thumbnail, selection area rows 9-21, cols 20-30
 *   - HOME button: Bottom-right corner, selection area rows 20-24, cols 28-32
 *
 * Implementation Notes:
 *   - Only MAP1 currently leads to GAMEPLAY state (others return to HOME_PAGE)
 *   - Selection state tracked as MapSelectionButton enum (SP_BTN_MAP1/2/3/HOME/NONE)
 *   - lastSelected used to detect state changes and update highlighting efficiently
 *   - Sound effect (PlayCLICKSFX) plays on all button activations
 *   - GameContext_SetMap() called before transitioning to GAMEPLAY to set active track
 */

#include "map_selection.h"

#include <nds.h>
#include <string.h>

#include "../graphics/color.h"
#include "combined.h"
#include "../core/context.h"
#include "../core/game_types.h"
#include "map_bottom.h"
#include "map_top.h"
#include "map_top_clouds.h"
#include "../audio/sound.h"
//=============================================================================
// Private constants / config
//=============================================================================
#define MAPSELECTION_BTN_COUNT SP_BTN_COUNT
#define MAP_SEL_SELECTION_PAL_BASE 240  // Base palette index for selection tiles

//=============================================================================
// Private module state
//=============================================================================
static MapSelectionButton selected = SP_BTN_NONE;
static MapSelectionButton lastSelected = SP_BTN_NONE;
static int cloudOffset = 0;  // Track cloud scrolling

//=============================================================================
// Private assets / tables (tiles, hitboxes, coordinates)
//=============================================================================
static const u8 selectionTile0[64] = {[0 ... 63] =
                                          MAP_SEL_SELECTION_PAL_BASE};  // MAP1
static const u8 selectionTile1[64] = {[0 ... 63] =
                                          MAP_SEL_SELECTION_PAL_BASE + 1};  // MAP2
static const u8 selectionTile2[64] = {[0 ... 63] =
                                          MAP_SEL_SELECTION_PAL_BASE + 2};  // MAP3
static const u8 selectionTile3[64] = {[0 ... 63] =
                                          MAP_SEL_SELECTION_PAL_BASE + 3};  // HOME

//=============================================================================
// Private function prototypes
//=============================================================================
static void configureGraphics_MAIN_MAP_SEL(void);
static void configBG_Main_MAP_SEL(void);
static void configureGraphics_Sub_MAP_SEL(void);
static void configBG_Sub_MAP_SEL(void);

static void handleDPadInputMAP_SEL(void);
static void handleTouchInputMAP_SEL(void);

static void MAP_SEL_setSelectionTint(MapSelectionButton btn, bool show);
static void drawSelectionRect(MapSelectionButton btn, u16 tileIndex);

//=============================================================================
// Public API implementation
//=============================================================================

/**
 * Initialize map selection screen
 *
 * Sets up graphics configuration for both screens:
 * - Main screen: Configures BG0 (thumbnails) and BG1 (clouds)
 * - Sub screen: Configures BG0 (menu UI) and BG1 (selection highlights)
 *
 * Initializes module state:
 * - Resets selection to SP_BTN_NONE (no button selected)
 * - Clears lastSelected for clean state tracking
 * - Cloud offset starts at 0
 *
 * Called once when entering MAPSELECTION state.
 */
void Map_Selection_initialize(void) {
    selected = SP_BTN_NONE;
    lastSelected = SP_BTN_NONE;

    configureGraphics_MAIN_MAP_SEL();
    configBG_Main_MAP_SEL();
    configureGraphics_Sub_MAP_SEL();
    configBG_Sub_MAP_SEL();
}

/**
 * Update map selection screen (call every frame)
 *
 * Input processing:
 * - Scans for D-pad, touch, and button input
 * - D-pad navigation updates selected variable
 * - Touch input checks pixel hitboxes and updates selected
 *
 * Visual feedback:
 * - Detects when selection changes (selected != lastSelected)
 * - Turns off old highlight by setting palette to BLACK
 * - Turns on new highlight by setting palette to WHITE
 *
 * Activation handling:
 * - Waits for KEY_A or KEY_TOUCH release (keysUp)
 * - Calls GameContext_SetMap() to store chosen track
 * - Plays click sound effect
 * - Returns next game state
 *
 * Returns:
 * - GAMEPLAY: If MAP1 (Scorching Sands) selected and confirmed
 * - HOME_PAGE: If MAP2, MAP3, or HOME button selected and confirmed
 * - MAPSELECTION: If no confirmation yet (stay on this screen)
 */
GameState Map_selection_update(void) {
    scanKeys();
    handleDPadInputMAP_SEL();
    handleTouchInputMAP_SEL();

    // Update highlight when selection changes
    if (selected != lastSelected) {
        if (lastSelected != SP_BTN_NONE)
            MAP_SEL_setSelectionTint(lastSelected, false);
        if (selected != SP_BTN_NONE)
            MAP_SEL_setSelectionTint(selected, true);
        lastSelected = selected;
    }

    // Handle button activation on release
    if (keysUp() & (KEY_A | KEY_TOUCH)) {
        switch (selected) {
            case SP_BTN_MAP1:
                GameContext_SetMap(ScorchingSands);
                PlayCLICKSFX();
                return GAMEPLAY;
                break;
            case SP_BTN_MAP2:
                GameContext_SetMap(AlpinRush);
                PlayCLICKSFX();
                return HOME_PAGE;
                break;
            case SP_BTN_MAP3:
                GameContext_SetMap(NeonCircuit);
                PlayCLICKSFX();
                return HOME_PAGE;
                break;
            case SP_BTN_HOME:
                PlayCLICKSFX();
                return HOME_PAGE;
            default:
                break;
        }
    }

    return MAPSELECTION;
}

/**
 * VBlank interrupt handler for map selection screen
 *
 * Animates cloud scrolling on main screen BG1:
 * - cloudSubPixel increments every frame (60 times per second)
 * - Every 2 frames, cloudOffset increments by 1 pixel
 * - This achieves 0.5 pixels per frame movement (30 pixels/second)
 * - cloudOffset wraps at 256 using bitwise AND to prevent overflow
 * - REG_BG1HOFS hardware register controls horizontal scroll
 *
 * Must be called from IRQ_VBLANK handler every frame.
 */
void Map_selection_OnVBlank(void) {
    static int cloudSubPixel = 0;
    cloudSubPixel++;
    if (cloudSubPixel >= 2) {
        cloudSubPixel = 0;
        cloudOffset = (cloudOffset + 1) & 255;
    }
    REG_BG1HOFS = cloudOffset;
}

//=============================================================================
// GRAPHICS SETUP
//=============================================================================

/**
 * Configure main screen display mode
 *
 * Sets up MODE_0_2D with two background layers:
 * - BG0: Map thumbnails (enabled)
 * - BG1: Scrolling clouds (enabled)
 *
 * Maps VRAM_A to main screen background memory.
 */
static void configureGraphics_MAIN_MAP_SEL(void) {
    REG_DISPCNT = MODE_0_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE;
    // REG_DISPCNT = MODE_3_2D | DISPLAY_BG3_ACTIVE | DISPLAY_BG1_ACTIVE;
    VRAM_A_CR = VRAM_ENABLE | VRAM_A_MAIN_BG;
}

/**
 * Configure main screen background layers
 *
 * BG0 (priority 1, back layer):
 * - 32x32 tilemap, 256-color mode
 * - Map base 0, tile base 1
 * - Contains combined map thumbnail graphics
 *
 * BG1 (priority 0, front layer):
 * - 32x32 tilemap, 256-color mode
 * - Map base 1, tile base 1 (shares tiles with BG0)
 * - Contains scrolling cloud graphics
 *
 * Loads combined graphics (all three map thumbnails merged):
 * - Tile data to BG_TILE_RAM(1)
 * - Palette data to BG_PALETTE
 * - Map data split between BG0 and BG1 (64x24 bytes each)
 */
static void configBG_Main_MAP_SEL(void) {
    BGCTRL[0] =
        BG_32x32 | BG_COLOR_256 | BG_MAP_BASE(0) | BG_TILE_BASE(1) | BG_PRIORITY(1);

    BGCTRL[1] =
        BG_32x32 | BG_COLOR_256 | BG_MAP_BASE(1) | BG_TILE_BASE(1) | BG_PRIORITY(0);
    dmaCopy(combinedTiles, BG_TILE_RAM(1), combinedTilesLen);
    dmaCopy(combinedPal, BG_PALETTE, combinedPalLen);
    dmaCopy(&combinedMap[0], BG_MAP_RAM(0), 64 * 24);
    dmaCopy(&combinedMap[32 * 24], BG_MAP_RAM(1), 64 * 24);

    /*
    BGCTRL[3] = BG_BMP_BASE(2) | BgSize_B8_256x256 | BG_PRIORITY(0);

    dmaCopy(map_topBitmap, BG_BMP_RAM(2), map_topBitmapLen);
    dmaCopy(map_topPal, BG_PALETTE, map_topPalLen);

    dmaCopy(map_top_cloudsTiles, BG_TILE_RAM(1), map_top_cloudsTilesLen);
    dmaCopy(map_top_cloudsMap, BG_MAP_RAM(0), map_top_cloudsMapLen);

    REG_BG3PA = 256;
    REG_BG3PC = 0;
    REG_BG3PB = 0;
    REG_BG3PD = 256;
    */
}

/**
 * Configure sub screen display mode
 *
 * Sets up MODE_0_2D with two background layers:
 * - BG0: Menu UI with transparent areas (enabled)
 * - BG1: Selection highlight tiles (enabled)
 *
 * Maps VRAM_C to sub screen background memory.
 */
static void configureGraphics_Sub_MAP_SEL(void) {
    REG_DISPCNT_SUB = MODE_0_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE;
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
}

/**
 * Configure sub screen background layers
 *
 * BG0 (priority 0, front layer):
 * - 32x32 tilemap, 256-color mode
 * - Map base 0, tile base 1
 * - Contains menu UI with transparent areas
 * - Loads map_bottom graphics (map names, buttons, UI elements)
 *
 * BG1 (priority 1, back layer):
 * - 32x32 tilemap, 256-color mode
 * - Map base 1, tile base 3 (separate from BG0)
 * - Contains selection highlight tiles
 *
 * Selection tile setup:
 * - Four 8x8 solid-color tiles loaded to tile base 3 (64 bytes each)
 * - Each tile uses unique palette index (240-243)
 * - Tiles placed on BG1 map covering selection areas
 * - Palette colors initialized to BLACK (invisible)
 * - When selected, palette changes to WHITE (visible through BG0 transparency)
 */
static void configBG_Sub_MAP_SEL(void) {
    // BG0: Menu layer (front)
    BGCTRL_SUB[0] =
        BG_32x32 | BG_MAP_BASE(0) | BG_TILE_BASE(1) | BG_COLOR_256 | BG_PRIORITY(0);
    dmaCopy(map_bottomPal, BG_PALETTE_SUB, map_bottomPalLen);
    dmaCopy(map_bottomTiles, BG_TILE_RAM_SUB(1), map_bottomTilesLen);
    dmaCopy(map_bottomMap, BG_MAP_RAM_SUB(0), map_bottomMapLen);

    // BG1: Selection highlight layer (behind)
    BGCTRL_SUB[1] =
        BG_32x32 | BG_COLOR_256 | BG_MAP_BASE(1) | BG_TILE_BASE(3) | BG_PRIORITY(1);

    // Load selection tiles
    dmaCopy(selectionTile0, (u8*)BG_TILE_RAM_SUB(3) + (0 * 64), 64);
    dmaCopy(selectionTile1, (u8*)BG_TILE_RAM_SUB(3) + (1 * 64), 64);
    dmaCopy(selectionTile2, (u8*)BG_TILE_RAM_SUB(3) + (2 * 64), 64);
    dmaCopy(selectionTile3, (u8*)BG_TILE_RAM_SUB(3) + (3 * 64), 64);

    // Clear BG1 map
    memset(BG_MAP_RAM_SUB(1), 0, 32 * 24 * 2);

    // color selection
    BG_PALETTE_SUB[MAP_SEL_SELECTION_PAL_BASE] = BLACK;
    BG_PALETTE_SUB[MAP_SEL_SELECTION_PAL_BASE + 1] = BLACK;
    BG_PALETTE_SUB[MAP_SEL_SELECTION_PAL_BASE + 2] = BLACK;
    BG_PALETTE_SUB[MAP_SEL_SELECTION_PAL_BASE + 3] = BLACK;

    // Draw selection areas
    drawSelectionRect(SP_BTN_MAP1, TILE_SEL_MAP1);
    drawSelectionRect(SP_BTN_MAP2, TILE_SEL_MAP2);
    drawSelectionRect(SP_BTN_MAP3, TILE_SEL_MAP3);
    drawSelectionRect(SP_BTN_HOME, TILE_SEL_SP_HOME);
}

/**
 * Draw selection rectangle on BG1 tilemap
 *
 * Fills a rectangular area on the BG1 map with the specified tile index.
 * The tile's color is controlled by its palette entry (240-243), which
 * is changed between BLACK (invisible) and WHITE (visible) for highlighting.
 *
 * @param btn - Which button's selection area to draw
 * @param tileIndex - Tile index to use (TILE_SEL_MAP1/2/3/HOME)
 *
 * Selection area coordinates (in tiles):
 * - MAP1: rows 9-21, cols 2-12 (left thumbnail)
 * - MAP2: rows 9-21, cols 11-21 (center thumbnail)
 * - MAP3: rows 9-21, cols 20-30 (right thumbnail)
 * - HOME: rows 20-24, cols 28-32 (bottom-right button)
 */
static void drawSelectionRect(MapSelectionButton btn, u16 tileIndex) {
    u16* map = BG_MAP_RAM_SUB(1);
    int startX, startY, endX, endY;

    switch (btn) {
        case SP_BTN_MAP1:  // Scorching Sands
            startX = 2;
            startY = 9;
            endX = 12;
            endY = 21;
            break;
        case SP_BTN_MAP2:  // Alpine Rush
            startX = 11;
            startY = 9;
            endX = 21;
            endY = 21;
            break;
        case SP_BTN_MAP3:  // Neon Circuit
            startX = 20;
            startY = 9;
            endX = 30;
            endY = 21;
            break;
        case SP_BTN_HOME:  // Home button
            startX = 28;
            startY = 20;
            endX = 32;
            endY = 24;
            break;
        default:
            return;
    }

    for (int row = startY; row < endY; row++)
        for (int col = startX; col < endX; col++)
            map[row * 32 + col] = tileIndex;
}

/**
 * Set selection highlighting for a button
 *
 * Changes the palette color of the button's selection tile between
 * BLACK (invisible) and WHITE (visible). Since the selection tiles
 * are positioned on BG1 behind transparent areas of BG0, changing
 * the color creates an instant highlight effect.
 *
 * @param btn - Which button to highlight (SP_BTN_MAP1/2/3/HOME)
 * @param show - true to show highlight (WHITE), false to hide (BLACK)
 *
 * Palette indices:
 * - 240: MAP1 selection tile color
 * - 241: MAP2 selection tile color
 * - 242: MAP3 selection tile color
 * - 243: HOME button selection tile color
 */
static void MAP_SEL_setSelectionTint(MapSelectionButton btn, bool show) {
    if (btn < 0 || btn >= MAPSELECTION_BTN_COUNT)
        return;
    int paletteIndex = MAP_SEL_SELECTION_PAL_BASE + btn;
    BG_PALETTE_SUB[paletteIndex] = show ? SP_SELECT_COLOR : BLACK;
}

//=============================================================================
// INPUT HANDLING
//=============================================================================

/**
 * Handle D-pad navigation
 *
 * UP/DOWN: Cycles through all 4 buttons with wraparound
 * - Uses modulo arithmetic for circular navigation
 * - -1 + COUNT mod COUNT wraps from MAP1 to HOME
 * - +1 mod COUNT wraps from HOME to MAP1
 *
 * LEFT/RIGHT: Navigates between adjacent buttons with wraparound
 * - LEFT: MAP2→MAP1, MAP3→MAP2, HOME→MAP3, MAP1→HOME (wraps)
 * - RIGHT: MAP1→MAP2, MAP2→MAP3, MAP3→HOME, HOME→MAP1 (wraps)
 * - Provides circular horizontal navigation for convenience
 */
static void handleDPadInputMAP_SEL(void) {
    int keys = keysDown();

    if (keys & KEY_UP) {
        selected = (selected - 1 + MAPSELECTION_BTN_COUNT) % MAPSELECTION_BTN_COUNT;
    }

    if (keys & KEY_DOWN) {
        selected = (selected + 1) % MAPSELECTION_BTN_COUNT;
    }

    if (keys & KEY_LEFT) {
        // Wrap around: MAP1 goes to HOME
        selected = (selected - 1 + MAPSELECTION_BTN_COUNT) % MAPSELECTION_BTN_COUNT;
    }

    if (keys & KEY_RIGHT) {
        // Wrap around: HOME goes to MAP1
        selected = (selected + 1) % MAPSELECTION_BTN_COUNT;
    }
}

/**
 * Handle touch screen input
 *
 * Checks if touch is within any button's hitbox and updates selection.
 * Hitboxes are defined in pixel coordinates for each button:
 *
 * MAP1 (Scorching Sands):
 * - X: 20-80, Y: 70-165 (left thumbnail area)
 *
 * MAP2 (Alpine Rush):
 * - X: 98-158, Y: 70-165 (center thumbnail area)
 *
 * MAP3 (Neon Circuit):
 * - X: 176-236, Y: 70-165 (right thumbnail area)
 *
 * HOME button:
 * - X: 224-251, Y: 161-188 (bottom-right corner)
 *
 * Returns immediately after first match (priority order: MAP1→MAP2→MAP3→HOME)
 */
static void handleTouchInputMAP_SEL(void) {
    if (!(keysHeld() & KEY_TOUCH))
        return;

    touchPosition touch;
    touchRead(&touch);

    if (touch.px < 0 || touch.px > 256 || touch.py < 0 || touch.py > 192) {
        return;
    }

    // Map 1 - Scorching Sands (circle + text)
    if (touch.px >= 20 && touch.px <= 80 && touch.py >= 70 && touch.py <= 165) {
        selected = SP_BTN_MAP1;
        return;
    }

    // Map 2 - Alpine Rush (circle + text)
    if (touch.px >= 98 && touch.px <= 158 && touch.py >= 70 && touch.py <= 165) {
        selected = SP_BTN_MAP2;
        return;
    }

    // Map 3 - Neon Circuit (circle + text)
    if (touch.px >= 176 && touch.px <= 236 && touch.py >= 70 && touch.py <= 165) {
        selected = SP_BTN_MAP3;
        return;
    }

    // Home button (bottom right corner)
    if (touch.px >= 224 && touch.px <= 251 && touch.py >= 161 && touch.py <= 188) {
        selected = SP_BTN_HOME;
        return;
    }
}
