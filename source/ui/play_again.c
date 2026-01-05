/**
 * play_again.c
 * Post-race "Play Again?" screen implementation
 */

#include "../ui/play_again.h"

#include <nds.h>
#include <string.h>

#include "../audio/sound.h"
#include "../core/context.h"
#include "../core/game_constants.h"
#include "../core/game_types.h"
#include "../gameplay/gameplay.h"
#include "../gameplay/gameplay_logic.h"
#include "../graphics/color.h"
#include "../network/multiplayer.h"
#include "playagain.h"  // Generated asset header

//=============================================================================
// Private Types
//=============================================================================

typedef enum {
    PA_BTN_YES = 0,
    PA_BTN_NO = 1,
    PA_BTN_COUNT = 2,
    PA_BTN_NONE = -1
} PlayAgainButton;

//=============================================================================
// Private State
//=============================================================================

static PlayAgainButton selected = PA_BTN_YES;
static PlayAgainButton lastSelected = PA_BTN_NONE;

//=============================================================================
// Private Assets
//=============================================================================

static const u8 selectionTileYes[64] = {[0 ... 63] = PA_SELECTION_PAL_BASE};
static const u8 selectionTileNo[64] = {[0 ... 63] = PA_SELECTION_PAL_BASE + 1};

//=============================================================================
// Private Function Prototypes
//=============================================================================

static void PlayAgain_ConfigureGraphics(void);
static void PlayAgain_ConfigureBackground(void);
static void PlayAgain_HandleDPadInput(void);
static void PlayAgain_HandleTouchInput(void);
static void PlayAgain_SetSelectionTint(PlayAgainButton btn, bool show);
static void PlayAgain_DrawSelectionRect(PlayAgainButton btn, u16 tileIndex);
static void PlayAgain_CleanupAndExit(void);

//=============================================================================
// Public API
//=============================================================================

/** Initialize the Play Again screen */
void PlayAgain_Initialize(void) {
    selected = PA_BTN_YES;
    lastSelected = PA_BTN_NONE;
    PlayAgain_ConfigureGraphics();
    PlayAgain_ConfigureBackground();
}

/** Update Play Again screen (returns next GameState) */
GameState PlayAgain_Update(void) {
    scanKeys();
    PlayAgain_HandleDPadInput();
    PlayAgain_HandleTouchInput();

    // Update visual highlight when selection changes
    if (selected != lastSelected) {
        if (lastSelected != PA_BTN_NONE) {
            PlayAgain_SetSelectionTint(lastSelected, false);
        }
        if (selected != PA_BTN_NONE) {
            PlayAgain_SetSelectionTint(selected, true);
        }
        lastSelected = selected;
    }

    // Handle button activation
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

    // SELECT quits directly to home
    if (keysDown() & KEY_SELECT) {
        PlayAgain_CleanupAndExit();
        return HOME_PAGE;
    }

    return PLAYAGAIN;
}

//=============================================================================
// Graphics Setup
//=============================================================================

/** Configure display control registers for sub screen */
static void PlayAgain_ConfigureGraphics(void) {
    REG_DISPCNT_SUB = MODE_0_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE;
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
}

/** Configure background layers and load graphics assets */
static void PlayAgain_ConfigureBackground(void) {
    // BG0: Main Play Again screen graphics
    BGCTRL_SUB[0] = BG_32x32 | BG_MAP_BASE(0) | BG_TILE_BASE(1) |
                    BG_COLOR_256 | BG_PRIORITY(0);
    dmaCopy(playagainPal, BG_PALETTE_SUB, playagainPalLen);
    dmaCopy(playagainTiles, BG_TILE_RAM_SUB(1), playagainTilesLen);
    dmaCopy(playagainMap, BG_MAP_RAM_SUB(0), playagainMapLen);

    // BG1: Selection highlight layer
    BGCTRL_SUB[1] = BG_32x32 | BG_COLOR_256 | BG_MAP_BASE(1) |
                    BG_TILE_BASE(3) | BG_PRIORITY(1);
    dmaCopy(selectionTileYes, (u8*)BG_TILE_RAM_SUB(3) + (0 * 64), 64);
    dmaCopy(selectionTileNo, (u8*)BG_TILE_RAM_SUB(3) + (1 * 64), 64);
    memset(BG_MAP_RAM_SUB(1), 0, 32 * 24 * 2);

    // Initialize selection colors
    BG_PALETTE_SUB[PA_SELECTION_PAL_BASE] = BLACK;
    BG_PALETTE_SUB[PA_SELECTION_PAL_BASE + 1] = BLACK;

    // Draw selection rectangles and show YES as initially selected
    PlayAgain_DrawSelectionRect(PA_BTN_YES, 0);
    PlayAgain_DrawSelectionRect(PA_BTN_NO, 1);
    PlayAgain_SetSelectionTint(PA_BTN_YES, true);
    lastSelected = PA_BTN_YES;
}

/** Draw selection rectangle on BG1 for a button */
static void PlayAgain_DrawSelectionRect(PlayAgainButton btn, u16 tileIndex) {
    u16* map = BG_MAP_RAM_SUB(1);
    int startX, startY, endX, endY;

    switch (btn) {
        case PA_BTN_YES:
            startX = PA_YES_RECT_X_START;
            startY = PA_YES_RECT_Y_START;
            endX = PA_YES_RECT_X_END;
            endY = PA_YES_RECT_Y_END;
            break;
        case PA_BTN_NO:
            startX = PA_NO_RECT_X_START;
            startY = PA_NO_RECT_Y_START;
            endX = PA_NO_RECT_X_END;
            endY = PA_NO_RECT_Y_END;
            break;
        default:
            return;
    }

    // Fill rectangle with tile index
    for (int row = startY; row < endY; row++) {
        for (int col = startX; col < endX; col++) {
            map[row * 32 + col] = tileIndex;
        }
    }
}

/** Set highlight color for a button */
static void PlayAgain_SetSelectionTint(PlayAgainButton btn, bool show) {
    if (btn < 0 || btn >= PA_BTN_COUNT) {
        return;
    }

    int paletteIndex = PA_SELECTION_PAL_BASE + btn;

    if (show) {
        if (btn == PA_BTN_YES) {
            // Blue/cyan highlight for YES
            BG_PALETTE_SUB[paletteIndex] = TEAL;  // Blue
        } else {
            // Red highlight for NO
            BG_PALETTE_SUB[paletteIndex] = RED;  // Red
        }
    } else {
        BG_PALETTE_SUB[paletteIndex] = BLACK;  // Transparent
    }
}

//=============================================================================
// Input Handling
//=============================================================================

/** Handle D-Pad input for button selection */
static void PlayAgain_HandleDPadInput(void) {
    int keys = keysDown();

    if (keys & KEY_LEFT) {
        selected = PA_BTN_YES;
    }
    if (keys & KEY_RIGHT) {
        selected = PA_BTN_NO;
    }
    if (keys & (KEY_UP | KEY_DOWN)) {
        selected = (selected == PA_BTN_YES) ? PA_BTN_NO : PA_BTN_YES;
    }
}

/** Handle touch screen input for button selection */
static void PlayAgain_HandleTouchInput(void) {
    if (!(keysHeld() & KEY_TOUCH)) {
        return;
    }

    touchPosition touch;
    touchRead(&touch);

    // Validate touch coordinates
    if (touch.px < 0 || touch.px > 256 || touch.py < 0 || touch.py > 192) {
        return;
    }

    // Check YES button hitbox
    if (touch.px >= PA_YES_TOUCH_X_MIN && touch.px <= PA_YES_TOUCH_X_MAX &&
        touch.py >= PA_YES_TOUCH_Y_MIN && touch.py <= PA_YES_TOUCH_Y_MAX) {
        selected = PA_BTN_YES;
        return;
    }

    // Check NO button hitbox
    if (touch.px >= PA_NO_TOUCH_X_MIN && touch.px <= PA_NO_TOUCH_X_MAX &&
        touch.py >= PA_NO_TOUCH_Y_MIN && touch.py <= PA_NO_TOUCH_Y_MAX) {
        selected = PA_BTN_NO;
        return;
    }
}

//=============================================================================
// Cleanup
//=============================================================================

/** Cleanup race state and multiplayer before exiting to home */
static void PlayAgain_CleanupAndExit(void) {
    RaceTick_TimerStop();  // CRITICAL: Stop timers before multiplayer cleanup

    if (GameContext_IsMultiplayerMode()) {
        Multiplayer_Cleanup();
        GameContext_SetMultiplayerMode(false);
    }
}
