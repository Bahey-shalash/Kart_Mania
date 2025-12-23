#include "map_selection.h"

#include <nds.h>
#include <string.h>

#include "color.h"
#include "combined.h"
#include "game_types.h"
#include "map_bottom.h"
#include "map_top.h"
#include "map_top_clouds.h"
#include "sound.h"
#include "context.h"
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

void Map_Selection_initialize(void) {
    selected = SP_BTN_NONE;
    lastSelected = SP_BTN_NONE;

    configureGraphics_MAIN_MAP_SEL();
    configBG_Main_MAP_SEL();
    configureGraphics_Sub_MAP_SEL();
    configBG_Sub_MAP_SEL();
}

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
                return GAMEPLAY;
                break;
            case SP_BTN_MAP3:
                GameContext_SetMap(NeonCircuit);
                PlayCLICKSFX();
                return GAMEPLAY;
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
static void configureGraphics_MAIN_MAP_SEL(void) {
    REG_DISPCNT = MODE_0_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE;
    // REG_DISPCNT = MODE_3_2D | DISPLAY_BG3_ACTIVE | DISPLAY_BG1_ACTIVE;
    VRAM_A_CR = VRAM_ENABLE | VRAM_A_MAIN_BG;
}

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

static void configureGraphics_Sub_MAP_SEL(void) {
    REG_DISPCNT_SUB = MODE_0_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE;
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
}

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

static void MAP_SEL_setSelectionTint(MapSelectionButton btn, bool show) {
    if (btn < 0 || btn >= MAPSELECTION_BTN_COUNT)
        return;
    int paletteIndex = MAP_SEL_SELECTION_PAL_BASE + btn;
    BG_PALETTE_SUB[paletteIndex] = show ? SP_SELECT_COLOR : BLACK;
}

//=============================================================================
// INPUT HANDLING
//=============================================================================
static void handleDPadInputMAP_SEL(void) {
    int keys = keysDown();

    if (keys & KEY_UP) {
        selected = (selected - 1 + MAPSELECTION_BTN_COUNT) % MAPSELECTION_BTN_COUNT;
    }

    if (keys & KEY_DOWN) {
        selected = (selected + 1) % MAPSELECTION_BTN_COUNT;
    }

    if (keys & KEY_LEFT) {
        if (selected == SP_BTN_MAP2) {
            selected = SP_BTN_MAP1;
        } else if (selected == SP_BTN_MAP3) {
            selected = SP_BTN_MAP2;
        } else if (selected == SP_BTN_HOME) {
            selected = SP_BTN_MAP3;
        }
    }

    if (keys & KEY_RIGHT) {
        if (selected == SP_BTN_MAP1) {
            selected = SP_BTN_MAP2;
        } else if (selected == SP_BTN_MAP2) {
            selected = SP_BTN_MAP3;
        } else if (selected == SP_BTN_MAP3) {
            selected = SP_BTN_HOME;
        }
    }
}

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
