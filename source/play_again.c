#include "play_again.h"

#include <nds.h>
#include <string.h>

#include "color.h"
#include "context.h"
#include "game_types.h"
#include "gameplay.h"
#include "gameplay_logic.h"
#include "playagain.h"
#include "sound.h"

//=============================================================================
// Private constants / config
//=============================================================================
#define PA_BTN_COUNT 2
#define PA_SELECTION_PAL_BASE 240  // Base palette index for selection tiles

//=============================================================================
// Private module state
//=============================================================================
static PlayAgainButton selected = PA_BTN_YES;  // Default to YES
static PlayAgainButton lastSelected = PA_BTN_NONE;

//=============================================================================
// Private assets / tables (tiles for highlighting)
//=============================================================================
static const u8 selectionTileYes[64] = {[0 ... 63] = PA_SELECTION_PAL_BASE};      // YES
static const u8 selectionTileNo[64] = {[0 ... 63] = PA_SELECTION_PAL_BASE + 1};   // NO

//=============================================================================
// Private function prototypes
//=============================================================================
static void configureGraphics_Sub_PA(void);
static void configBG_Sub_PA(void);
static void handleDPadInput_PA(void);
static void handleTouchInput_PA(void);
static void PA_setSelectionTint(PlayAgainButton btn, bool show);
static void drawSelectionRect(PlayAgainButton btn, u16 tileIndex);

//=============================================================================
// Public API implementation
//=============================================================================

void PlayAgain_Initialize(void) {
    selected = PA_BTN_YES;  // Default to YES
    lastSelected = PA_BTN_NONE;

    configureGraphics_Sub_PA();
    configBG_Sub_PA();
}

GameState PlayAgain_Update(void) {
    scanKeys();
    handleDPadInput_PA();
    handleTouchInput_PA();

    // Update highlight when selection changes
    if (selected != lastSelected) {
        if (lastSelected != PA_BTN_NONE)
            PA_setSelectionTint(lastSelected, false);
        if (selected != PA_BTN_NONE)
            PA_setSelectionTint(selected, true);
        lastSelected = selected;
    }

    // Handle button activation on release
    if (keysUp() & (KEY_A | KEY_TOUCH)) {
        switch (selected) {
            case PA_BTN_YES:
                PlayCLICKSFX();
                return GAMEPLAY;  // Restart race
            case PA_BTN_NO:
                PlayCLICKSFX();
                return HOME_PAGE;  // Go to home
            default:
                break;
        }
    }

    // Allow SELECT to quit
    if (keysDown() & KEY_SELECT) {
        return HOME_PAGE;
    }

    return PLAYAGAIN;  // Stay on PLAYAGAIN state
}

void PlayAgain_OnVBlank(void) {
    // No special VBlank logic needed for now
    // Could add animations here later
}

//=============================================================================
// GRAPHICS SETUP
//=============================================================================

static void configureGraphics_Sub_PA(void) {
    REG_DISPCNT_SUB = MODE_0_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE;
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
}

static void configBG_Sub_PA(void) {
    // BG0: Play Again screen layer (front)
    BGCTRL_SUB[0] =
        BG_32x32 | BG_MAP_BASE(0) | BG_TILE_BASE(1) | BG_COLOR_256 | BG_PRIORITY(0);
    dmaCopy(playagainPal, BG_PALETTE_SUB, playagainPalLen);
    dmaCopy(playagainTiles, BG_TILE_RAM_SUB(1), playagainTilesLen);
    dmaCopy(playagainMap, BG_MAP_RAM_SUB(0), playagainMapLen);

    // BG1: Selection highlight layer (behind)
    BGCTRL_SUB[1] =
        BG_32x32 | BG_COLOR_256 | BG_MAP_BASE(1) | BG_TILE_BASE(3) | BG_PRIORITY(1);

    // Load selection tiles
    dmaCopy(selectionTileYes, (u8*)BG_TILE_RAM_SUB(3) + (0 * 64), 64);
    dmaCopy(selectionTileNo, (u8*)BG_TILE_RAM_SUB(3) + (1 * 64), 64);

    // Clear BG1 map
    memset(BG_MAP_RAM_SUB(1), 0, 32 * 24 * 2);

    // Set selection colors (start as black/transparent)
    BG_PALETTE_SUB[PA_SELECTION_PAL_BASE] = BLACK;
    BG_PALETTE_SUB[PA_SELECTION_PAL_BASE + 1] = BLACK;

    // Draw selection areas
    drawSelectionRect(PA_BTN_YES, 0);  // Tile index 0 for YES
    drawSelectionRect(PA_BTN_NO, 1);   // Tile index 1 for NO
    
    // Immediately show YES as selected (like map selection does)
    PA_setSelectionTint(PA_BTN_YES, true);
    lastSelected = PA_BTN_YES;
}

static void drawSelectionRect(PlayAgainButton btn, u16 tileIndex) {
    u16* map = BG_MAP_RAM_SUB(1);
    int startX, startY, endX, endY;

    switch (btn) {
        case PA_BTN_YES:  // YES button circle
            startX = 6;
            startY = 10;
            endX = 16;
            endY = 20;
            break;
            
        case PA_BTN_NO:   // NO button circle
            startX = 17;
            startY = 10;
            endX = 27;
            endY = 20;
            break;
            
        default:
            return;
    }

    for (int row = startY; row < endY; row++)
        for (int col = startX; col < endX; col++)
            map[row * 32 + col] = tileIndex;
}

static void PA_setSelectionTint(PlayAgainButton btn, bool show) {
    if (btn < 0 || btn >= PA_BTN_COUNT)
        return;
    
    int paletteIndex = PA_SELECTION_PAL_BASE + btn;
    
    // Use different highlight colors
    if (show) {
        if (btn == PA_BTN_YES) {
            // Blue/cyan highlight for YES
            BG_PALETTE_SUB[paletteIndex] = ARGB16(1, 0, 20, 31);  // Blue
        } else {
            // Red highlight for NO
            BG_PALETTE_SUB[paletteIndex] = ARGB16(1, 31, 0, 0);  // Red
        }
    } else {
        BG_PALETTE_SUB[paletteIndex] = BLACK;  // No highlight
    }
}

//=============================================================================
// INPUT HANDLING
//=============================================================================

static void handleDPadInput_PA(void) {
    int keys = keysDown();

    if (keys & KEY_LEFT) {
        selected = PA_BTN_YES;
    }

    if (keys & KEY_RIGHT) {
        selected = PA_BTN_NO;
    }

    // Also allow up/down for accessibility
    if (keys & (KEY_UP | KEY_DOWN)) {
        // Toggle between YES and NO
        selected = (selected == PA_BTN_YES) ? PA_BTN_NO : PA_BTN_YES;
    }
}

static void handleTouchInput_PA(void) {
    if (!(keysHeld() & KEY_TOUCH))
        return;

    touchPosition touch;
    touchRead(&touch);

    if (touch.px < 0 || touch.px > 256 || touch.py < 0 || touch.py > 192) {
        return;
    }

    // YES button - approximate circle + text
    // Center at x=85, y=120, radius=35, plus label below
    if (touch.px >= 50 && touch.px <= 120 && touch.py >= 85 && touch.py <= 175) {
        selected = PA_BTN_YES;
        return;
    }

    // NO button - approximate circle + text
    // Center at x=171, y=120, radius=35, plus label below
    if (touch.px >= 136 && touch.px <= 206 && touch.py >= 85 && touch.py <= 175) {
        selected = PA_BTN_NO;
        return;
    }
}