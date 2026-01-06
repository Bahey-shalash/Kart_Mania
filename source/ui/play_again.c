/**
 * File: play_again.c
 * ------------------
 * Description: Implementation of the post-race Play Again screen. Displays
 *              YES/NO buttons with selection highlighting and handles race
 *              restart or exit to home menu. Manages cleanup of multiplayer
 *              connections and race timers when exiting.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 05.01.2026
 */

#include "play_again.h"

#include <nds.h>
#include <string.h>

#include "../audio/sound.h"
#include "../core/context.h"
#include "../gameplay/gameplay.h"
#include "../gameplay/gameplay_logic.h"
#include "../graphics/color.h"
#include "../network/multiplayer.h"
#include "data/ui/playagain.h"

//=============================================================================
// PRIVATE TYPES
//=============================================================================

typedef enum {
    PA_BTN_YES = 0,
    PA_BTN_NO = 1,
    PA_BTN_COUNT = 2,
    PA_BTN_NONE = -1
} PlayAgainButton;

//=============================================================================
// PRIVATE CONSTANTS
//=============================================================================

#define PA_SELECTION_PAL_BASE 240  // Base palette index for selection tiles

//=============================================================================
// PRIVATE STATE
//=============================================================================

static PlayAgainButton selected = PA_BTN_YES;
static PlayAgainButton lastSelected = PA_BTN_NONE;

//=============================================================================
// PRIVATE ASSETS
//=============================================================================

static const u8 selectionTileYes[64] = {[0 ... 63] = PA_SELECTION_PAL_BASE};
static const u8 selectionTileNo[64] = {[0 ... 63] = PA_SELECTION_PAL_BASE + 1};

//=============================================================================
// PRIVATE FUNCTION PROTOTYPES
//=============================================================================

static void PlayAgain_ConfigureGraphics(void);
static void PlayAgain_ConfigureBackground(void);
static void PlayAgain_HandleDPadInput(void);
static void PlayAgain_HandleTouchInput(void);
static void PlayAgain_SetSelectionTint(PlayAgainButton btn, bool show);
static void PlayAgain_DrawSelectionRect(PlayAgainButton btn, u16 tileIndex);
static void PlayAgain_CleanupAndExit(void);

//=============================================================================
// PUBLIC API
//=============================================================================

void PlayAgain_Initialize(void) {
    selected = PA_BTN_YES;
    lastSelected = PA_BTN_NONE;
    PlayAgain_ConfigureGraphics();
    PlayAgain_ConfigureBackground();
}

GameState PlayAgain_Update(void) {
    scanKeys();
    PlayAgain_HandleDPadInput();
    PlayAgain_HandleTouchInput();

    // Update highlight when selection changes
    if (selected != lastSelected) {
        if (lastSelected != PA_BTN_NONE)
            PlayAgain_SetSelectionTint(lastSelected, false);
        if (selected != PA_BTN_NONE)
            PlayAgain_SetSelectionTint(selected, true);
        lastSelected = selected;
    }

    // Handle button activation on release
    if (keysUp() & (KEY_A | KEY_TOUCH)) {
        switch (selected) {
            case PA_BTN_YES:
                PlayCLICKSFX();
                return GAMEPLAY;

            case PA_BTN_NO:
                PlayCLICKSFX();
                PlayAgain_CleanupAndExit();
                return HOME_PAGE;

            default:
                break;
        }
    }

    // SELECT button quick exit to home
    if (keysDown() & KEY_SELECT) {
        PlayAgain_CleanupAndExit();
        return HOME_PAGE;
    }

    return PLAYAGAIN;
}

void PlayAgain_OnVBlank(void) {
    // Reserved for future animations
}

//=============================================================================
// GRAPHICS SETUP
//=============================================================================

static void PlayAgain_ConfigureGraphics(void) {
    REG_DISPCNT_SUB = MODE_0_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE;
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
}

static void PlayAgain_ConfigureBackground(void) {
    // BG0: Play Again screen (front layer)
    BGCTRL_SUB[0] =
        BG_32x32 | BG_MAP_BASE(0) | BG_TILE_BASE(1) | BG_COLOR_256 | BG_PRIORITY(0);
    dmaCopy(playagainPal, BG_PALETTE_SUB, playagainPalLen);
    dmaCopy(playagainTiles, BG_TILE_RAM_SUB(1), playagainTilesLen);
    dmaCopy(playagainMap, BG_MAP_RAM_SUB(0), playagainMapLen);

    // BG1: Selection highlight layer (back layer)
    BGCTRL_SUB[1] =
        BG_32x32 | BG_COLOR_256 | BG_MAP_BASE(1) | BG_TILE_BASE(3) | BG_PRIORITY(1);

    // Load selection tiles
    dmaCopy(selectionTileYes, (u8*)BG_TILE_RAM_SUB(3) + (0 * 64), 64);
    dmaCopy(selectionTileNo, (u8*)BG_TILE_RAM_SUB(3) + (1 * 64), 64);

    // Clear BG1 map
    memset(BG_MAP_RAM_SUB(1), 0, 32 * 24 * 2);

    // Initialize palette colors to transparent
    BG_PALETTE_SUB[PA_SELECTION_PAL_BASE] = BLACK;
    BG_PALETTE_SUB[PA_SELECTION_PAL_BASE + 1] = BLACK;

    // Draw selection rectangles
    PlayAgain_DrawSelectionRect(PA_BTN_YES, 0);
    PlayAgain_DrawSelectionRect(PA_BTN_NO, 1);

    // Show YES as selected by default
    PlayAgain_SetSelectionTint(PA_BTN_YES, true);
    lastSelected = PA_BTN_YES;
}

static void PlayAgain_DrawSelectionRect(PlayAgainButton btn, u16 tileIndex) {
    u16* map = BG_MAP_RAM_SUB(1);
    int startX, startY, endX, endY;

    switch (btn) {
        case PA_BTN_YES:
            startX = 6;
            startY = 10;
            endX = 16;
            endY = 20;
            break;

        case PA_BTN_NO:
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

static void PlayAgain_SetSelectionTint(PlayAgainButton btn, bool show) {
    if (btn < 0 || btn >= PA_BTN_COUNT)
        return;

    int paletteIndex = PA_SELECTION_PAL_BASE + btn;

    if (show) {
        // Blue for YES, red for NO
        BG_PALETTE_SUB[paletteIndex] = (btn == PA_BTN_YES) ? TEAL : RED;
    } else {
        BG_PALETTE_SUB[paletteIndex] = BLACK;
    }
}

//=============================================================================
// INPUT HANDLING
//=============================================================================

static void PlayAgain_HandleDPadInput(void) {
    int keys = keysDown();

    if (keys & KEY_LEFT)
        selected = PA_BTN_YES;
    if (keys & KEY_RIGHT)
        selected = PA_BTN_NO;

    // Allow up/down to toggle for accessibility
    if (keys & (KEY_UP | KEY_DOWN))
        selected = (selected == PA_BTN_YES) ? PA_BTN_NO : PA_BTN_YES;
}

static void PlayAgain_HandleTouchInput(void) {
    if (!(keysHeld() & KEY_TOUCH))
        return;

    touchPosition touch;
    touchRead(&touch);

    // Validate touch coordinates
    if (touch.px < 0 || touch.px > 256 || touch.py < 0 || touch.py > 192)
        return;

    // YES button (left circle + text)
    if (touch.px >= 50 && touch.px <= 120 && touch.py >= 85 && touch.py <= 175) {
        selected = PA_BTN_YES;
        return;
    }

    // NO button (right circle + text)
    if (touch.px >= 136 && touch.px <= 206 && touch.py >= 85 && touch.py <= 175) {
        selected = PA_BTN_NO;
        return;
    }
}

//=============================================================================
// CLEANUP
//=============================================================================

static void PlayAgain_CleanupAndExit(void) {
    // CRITICAL: Stop timers before multiplayer cleanup
    RaceTick_TimerStop();

    if (GameContext_IsMultiplayerMode()) {
        Multiplayer_Cleanup();
        GameContext_SetMultiplayerMode(false);
    }
}
