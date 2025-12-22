#include "game.h"

#include <nds.h>
#include <string.h>

#include "color.h"
#include "game_types.h"
#include "map_bottom.h"
#include "map_top.h"
#include "map_top_clouds.h"
#include "sound.h"

//=============================================================================
// CONSTANTS
//=============================================================================
#define SINGLEPLAYER_BTN_COUNT SP_BTN_COUNT
#define SP_SELECTION_PAL_BASE 240  // Base palette index for selection tiles

//=============================================================================
// GLOBAL STATE
//=============================================================================
static SingleplayerButton selected = SP_BTN_NONE;
static SingleplayerButton lastSelected = SP_BTN_NONE;
static int cloudOffset = 0;  // Track cloud scrolling

//=============================================================================
// GRAPHICS SETUP
//=============================================================================
void configureGraphics_MAIN_Singleplayer(void) {
    REG_DISPCNT = MODE_0_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE;
    VRAM_A_CR = VRAM_ENABLE | VRAM_A_MAIN_BG;
}

void configBG_Main_Singleplayer(void) {
    BGCTRL[0] = BG_32x32 | BG_COLOR_256 | BG_MAP_BASE(0) |BG_TILE_BASE(1) | BG_PRIORITY(1);
    BGCTRL[1] = BG_32x32 | BG_COLOR_256 | BG_MAP_BASE(1) |BG_TILE_BASE(3) | BG_PRIORITY(0);
    dmaCopy(map_topMap, BG_MAP_RAM(0), map_topMapLen);
    dmaCopy(map_topTiles, BG_TILE_RAM(1), map_topTilesLen);
    dmaCopy(map_topPal, BG_PALETTE, map_topPalLen);
    dmaCopy(map_top_cloudsMap, BG_MAP_RAM(1), map_top_cloudsMapLen);
    dmaCopy(map_top_cloudsTiles, BG_TILE_RAM(3), map_top_cloudsTilesLen);
}

void configureGraphics_Sub_Singleplayer(void) {
    REG_DISPCNT_SUB = MODE_0_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE;
}

//=============================================================================
// SELECTION HIGHLIGHT TILES
//=============================================================================

static const u8 selectionTile0[64] = {[0 ... 63] = 240};  // MAP1
static const u8 selectionTile1[64] = {[0 ... 63] = 241};  // MAP2
static const u8 selectionTile2[64] = {[0 ... 63] = 242};  // MAP3
static const u8 selectionTile3[64] = {[0 ... 63] = 243};  // HOME

static void drawSelectionRect(SingleplayerButton btn, u16 tileIndex) {
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

static void Singleplayer_setSelectionTint(SingleplayerButton btn, bool show) {
    if (btn < 0 || btn >= SINGLEPLAYER_BTN_COUNT)
        return;
    int paletteIndex = SP_SELECTION_PAL_BASE + btn;
    BG_PALETTE_SUB[paletteIndex] = show ? SP_SELECT_COLOR : BLACK;
}

void configBG_Sub_Singleplayer(void) {
    // BG0: Menu layer (front)
    BGCTRL_SUB[0] =
        BG_32x32 | BG_MAP_BASE(0) | BG_TILE_BASE(1) | BG_COLOR_256 | BG_PRIORITY(0);
    dmaCopy(map_bottomPal, BG_PALETTE_SUB, map_bottomPalLen);
    dmaCopy(map_bottomTiles, BG_TILE_RAM_SUB(1), map_bottomTilesLen);
    dmaCopy(map_bottomMap, BG_MAP_RAM_SUB(0), map_bottomMapLen);

    // BG1: Selection highlight layer (behind)
    BGCTRL_SUB[1] = BG_32x32 | BG_COLOR_256 | BG_MAP_BASE(1) | BG_TILE_BASE(3) | BG_PRIORITY(1);

    // Load selection tiles
    dmaCopy(selectionTile0, (u8*)BG_TILE_RAM_SUB(3) + (0 * 64), 64);
    dmaCopy(selectionTile1, (u8*)BG_TILE_RAM_SUB(3) + (1 * 64), 64);
    dmaCopy(selectionTile2, (u8*)BG_TILE_RAM_SUB(3) + (2 * 64), 64);
    dmaCopy(selectionTile3, (u8*)BG_TILE_RAM_SUB(3) + (3 * 64), 64);

    // Clear BG1 map
    memset(BG_MAP_RAM_SUB(1), 0, 32 * 24 * 2);
    BG_PALETTE_SUB[240] = BLACK;
    BG_PALETTE_SUB[241] = BLACK;
    BG_PALETTE_SUB[242] = BLACK;
    BG_PALETTE_SUB[243] = BLACK;

    // Draw selection areas
    drawSelectionRect(SP_BTN_MAP1, TILE_SEL_MAP1);
    drawSelectionRect(SP_BTN_MAP2, TILE_SEL_MAP2);
    drawSelectionRect(SP_BTN_MAP3, TILE_SEL_MAP3);
    drawSelectionRect(SP_BTN_HOME, TILE_SEL_SP_HOME);
}

//=============================================================================
// INPUT HANDLING
//=============================================================================
void handleDPadInputSingleplayer(void) {
    int keys = keysDown();

    if (keys & KEY_UP) {
        selected = (selected - 1 + SINGLEPLAYER_BTN_COUNT) % SINGLEPLAYER_BTN_COUNT;
    }

    if (keys & KEY_DOWN) {
        selected = (selected + 1) % SINGLEPLAYER_BTN_COUNT;
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

void handleTouchInputSingleplayer(void) {
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

//=============================================================================
// PUBLIC API
//=============================================================================
void Singleplayer_initialize(void) {
    selected = SP_BTN_NONE;
    lastSelected = SP_BTN_NONE;

    configureGraphics_MAIN_Singleplayer();
    configBG_Main_Singleplayer();
    configureGraphics_Sub_Singleplayer();
    configBG_Sub_Singleplayer();

    // sound effects
    /*  LoadClickSoundFX(); */
}

GameState Singleplayer_update(void) {
    scanKeys();
    handleDPadInputSingleplayer();
    handleTouchInputSingleplayer();

    // Update highlight when selection changes
    if (selected != lastSelected) {
        if (lastSelected != SP_BTN_NONE)
            Singleplayer_setSelectionTint(lastSelected, false);
        if (selected != SP_BTN_NONE)
            Singleplayer_setSelectionTint(selected, true);
        lastSelected = selected;
    }

    static int cloudSubPixel = 0;  // Track sub-pixel movement
    
    cloudSubPixel++;
    if (cloudSubPixel >= 2) {  // Move 1 pixel every 4 frames (adjust this number)
        cloudSubPixel = 0;
        cloudOffset++;
        if (cloudOffset > 255) {
            cloudOffset = 0;
        }
    }
    REG_BG1HOFS = cloudOffset;

    // Handle button activation on release
    if (keysUp() & (KEY_A | KEY_TOUCH)) {
        switch (selected) {
            case SP_BTN_MAP1:
                // TODO: Load Scorching Sands map
                // return GAMEPLAY; // when ready
                PlayCLICKSFX();
                break;
            case SP_BTN_MAP2:
                // TODO: Load Alpine Rush map
                // return GAMEPLAY; // when ready
                PlayCLICKSFX();
                break;
            case SP_BTN_MAP3:
                // TODO: Load Neon Circuit map
                // return GAMEPLAY; // when ready
                PlayCLICKSFX();
                break;
            case SP_BTN_HOME:
                PlayCLICKSFX();
                return HOME_PAGE;
            default:
                break;
        }
    }

    return SINGLEPLAYER;
}
