/**
 * settings.c
 * Settings screen implementation with WiFi/Music/Sound FX toggles
 */

#include "../ui/settings.h"

#include <nds.h>
#include <string.h>

#include "../audio/sound.h"
#include "../core/context.h"
#include "../core/game_constants.h"
#include "../graphics/color.h"
#include "../storage/storage.h"
#include "nds_settings.h"
#include "settings_top.h"

//=============================================================================
// Private Assets
//=============================================================================

static const u8 RedTile[64] = {[0 ... 63] = 254};
static const u8 GreenTile[64] = {[0 ... 63] = 255};

static const u8 selectionTile0[64] = {[0 ... 63] = SETTINGS_SELECTION_PAL_BASE};
static const u8 selectionTile1[64] = {[0 ... 63] = SETTINGS_SELECTION_PAL_BASE + 1};
static const u8 selectionTile2[64] = {[0 ... 63] = SETTINGS_SELECTION_PAL_BASE + 2};
static const u8 selectionTile3[64] = {[0 ... 63] = SETTINGS_SELECTION_PAL_BASE + 3};
static const u8 selectionTile4[64] = {[0 ... 63] = SETTINGS_SELECTION_PAL_BASE + 4};
static const u8 selectionTile5[64] = {[0 ... 63] = SETTINGS_SELECTION_PAL_BASE + 5};

//=============================================================================
// Private State
//=============================================================================

static SettingsButtonSelected selected = SETTINGS_BTN_NONE;
static SettingsButtonSelected lastSelected = SETTINGS_BTN_NONE;

//=============================================================================
// Private Function Prototypes
//=============================================================================

// Main screen (top)
static void Settings_ConfigureMainGraphics(void);
static void Settings_ConfigureMainBackground(void);

// Sub screen (bottom)
static void Settings_ConfigureSubGraphics(void);
static void Settings_ConfigureSubBackground(void);
static void Settings_LoadSelectionTiles(void);
static void Settings_InitializeToggleStates(void);

// Input handling
static void Settings_HandleDPadInput(void);
static void Settings_HandleTouchInput(void);
static void Settings_CheckWifiTouch(touchPosition* touch);
static void Settings_CheckMusicTouch(touchPosition* touch);
static void Settings_CheckSoundFxTouch(touchPosition* touch);
static void Settings_CheckButtonTouch(touchPosition* touch);

// Rendering
static void Settings_SetSelectionTint(SettingsButtonSelected btn, bool show);
static void Settings_DrawSelectionRect(SettingsButtonSelected btn, u16 tileIndex);
static void Settings_DrawToggleRect(SettingsButtonSelected btn, bool enabled);

// Logic
static void Settings_RefreshUI(void);
static void Settings_OnSavePressed(void);

//=============================================================================
// Public API
//=============================================================================

/** Initialize the Settings screen */
void Settings_Initialize(void) {
    selected = SETTINGS_BTN_NONE;
    lastSelected = SETTINGS_BTN_NONE;

    Settings_ConfigureMainGraphics();
    Settings_ConfigureMainBackground();
    Settings_ConfigureSubGraphics();
    Settings_ConfigureSubBackground();
}

/** Update Settings screen (returns next GameState) */
GameState Settings_Update(void) {
    scanKeys();
    Settings_HandleDPadInput();
    Settings_HandleTouchInput();

    GameContext* ctx = GameContext_Get();

    // Update highlight when selection changes
    if (selected != lastSelected) {
        if (lastSelected != SETTINGS_BTN_NONE) {
            Settings_SetSelectionTint(lastSelected, false);
        }
        if (selected != SETTINGS_BTN_NONE) {
            Settings_SetSelectionTint(selected, true);
        }
        lastSelected = selected;
    }

    // Handle button activation
    if (keysUp() & (KEY_A | KEY_TOUCH)) {
        switch (selected) {
            case SETTINGS_BTN_WIFI: {
                bool wifiEnabled = !ctx->userSettings.wifiEnabled;
                GameContext_SetWifiEnabled(wifiEnabled);
                Settings_DrawToggleRect(SETTINGS_BTN_WIFI, wifiEnabled);
                PlayDingSFX();
                break;
            }
            case SETTINGS_BTN_MUSIC: {
                bool musicEnabled = !ctx->userSettings.musicEnabled;
                GameContext_SetMusicEnabled(musicEnabled);
                Settings_DrawToggleRect(SETTINGS_BTN_MUSIC, musicEnabled);
                PlayDingSFX();
                break;
            }
            case SETTINGS_BTN_SOUND_FX: {
                bool soundFxEnabled = !ctx->userSettings.soundFxEnabled;
                PlayDingSFX();  // Play before potentially muting
                GameContext_SetSoundFxEnabled(soundFxEnabled);
                Settings_DrawToggleRect(SETTINGS_BTN_SOUND_FX, soundFxEnabled);
                break;
            }
            case SETTINGS_BTN_SAVE:
                Settings_OnSavePressed();
                PlayDingSFX();
                break;
            case SETTINGS_BTN_BACK:
            case SETTINGS_BTN_HOME:
                PlayCLICKSFX();
                return HOME_PAGE;
            default:
                break;
        }
    }

    return SETTINGS;
}

//=============================================================================
// Logic
//=============================================================================

/** Refresh UI to match current settings state */
static void Settings_RefreshUI(void) {
    GameContext* ctx = GameContext_Get();

    Settings_DrawToggleRect(SETTINGS_BTN_WIFI, ctx->userSettings.wifiEnabled);
    Settings_DrawToggleRect(SETTINGS_BTN_MUSIC, ctx->userSettings.musicEnabled);
    Settings_DrawToggleRect(SETTINGS_BTN_SOUND_FX, ctx->userSettings.soundFxEnabled);

    // Apply settings (triggers side effects)
    GameContext_SetWifiEnabled(ctx->userSettings.wifiEnabled);
    GameContext_SetMusicEnabled(ctx->userSettings.musicEnabled);
    GameContext_SetSoundFxEnabled(ctx->userSettings.soundFxEnabled);
}

/** Handle save button press (START+SELECT resets to defaults) */
static void Settings_OnSavePressed(void) {
    if ((keysHeld() & KEY_START) && (keysHeld() & KEY_SELECT)) {
        Storage_ResetToDefaults();
        Settings_RefreshUI();
    } else {
        Storage_SaveSettings();
    }
}

//=============================================================================
// Main Screen (Top) Graphics Setup
//=============================================================================

/** Configure main screen display registers */
static void Settings_ConfigureMainGraphics(void) {
    REG_DISPCNT = MODE_5_2D | DISPLAY_BG2_ACTIVE;
    VRAM_A_CR = VRAM_ENABLE | VRAM_A_MAIN_BG;
}

/** Load main screen background bitmap */
static void Settings_ConfigureMainBackground(void) {
    BGCTRL[2] = BG_BMP_BASE(0) | BgSize_B8_256x256;
    dmaCopy(settings_topBitmap, BG_BMP_RAM(0), settings_topBitmapLen);
    dmaCopy(settings_topPal, BG_PALETTE, settings_topPalLen);
    REG_BG2PA = 256;
    REG_BG2PC = 0;
    REG_BG2PB = 0;
    REG_BG2PD = 256;
}

//=============================================================================
// Sub Screen (Bottom) Graphics Setup
//=============================================================================

/** Configure sub screen display registers */
static void Settings_ConfigureSubGraphics(void) {
    REG_DISPCNT_SUB = MODE_0_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE;
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
}

/** Configure sub screen background layers and load assets */
static void Settings_ConfigureSubBackground(void) {
    // BG0: Main settings screen graphics
    BGCTRL_SUB[0] = BG_32x32 | BG_MAP_BASE(0) | BG_TILE_BASE(1) |
                    BG_COLOR_256 | BG_PRIORITY(0);
    dmaCopy(nds_settingsPal, BG_PALETTE_SUB, nds_settingsPalLen);
    dmaCopy(nds_settingsTiles, BG_TILE_RAM_SUB(1), nds_settingsTilesLen);
    dmaCopy(nds_settingsMap, BG_MAP_RAM_SUB(0), nds_settingsMapLen);

    // BG1: Toggle and selection overlay layer
    BGCTRL_SUB[1] = BG_32x32 | BG_COLOR_256 | BG_MAP_BASE(1) |
                    BG_TILE_BASE(2) | BG_PRIORITY(1);

    // Load toggle tiles
    dmaCopy(RedTile, (u8*)BG_TILE_RAM_SUB(2) + (3 * 64), 64);
    dmaCopy(GreenTile, (u8*)BG_TILE_RAM_SUB(2) + (4 * 64), 64);
    BG_PALETTE_SUB[254] = TOGGLE_OFF_COLOR;
    BG_PALETTE_SUB[255] = TOGGLE_ON_COLOR;

    Settings_LoadSelectionTiles();
    memset(BG_MAP_RAM_SUB(1), 0, 32 * 24 * 2);
    Settings_InitializeToggleStates();
}

/** Load selection highlight tiles into VRAM */
static void Settings_LoadSelectionTiles(void) {
    dmaCopy(selectionTile0, (u8*)BG_TILE_RAM_SUB(2) + (5 * 64), 64);
    dmaCopy(selectionTile1, (u8*)BG_TILE_RAM_SUB(2) + (6 * 64), 64);
    dmaCopy(selectionTile2, (u8*)BG_TILE_RAM_SUB(2) + (7 * 64), 64);
    dmaCopy(selectionTile3, (u8*)BG_TILE_RAM_SUB(2) + (8 * 64), 64);
    dmaCopy(selectionTile4, (u8*)BG_TILE_RAM_SUB(2) + (9 * 64), 64);
    dmaCopy(selectionTile5, (u8*)BG_TILE_RAM_SUB(2) + (10 * 64), 64);
}

/** Initialize toggle states and draw selection areas */
static void Settings_InitializeToggleStates(void) {
    GameContext* ctx = GameContext_Get();

    // Draw initial toggle states
    Settings_DrawToggleRect(SETTINGS_BTN_WIFI, ctx->userSettings.wifiEnabled);
    Settings_DrawToggleRect(SETTINGS_BTN_MUSIC, ctx->userSettings.musicEnabled);
    Settings_DrawToggleRect(SETTINGS_BTN_SOUND_FX, ctx->userSettings.soundFxEnabled);

    // Draw selection areas
    Settings_DrawSelectionRect(SETTINGS_BTN_WIFI, TILE_SEL_WIFI);
    Settings_DrawSelectionRect(SETTINGS_BTN_MUSIC, TILE_SEL_MUSIC);
    Settings_DrawSelectionRect(SETTINGS_BTN_SOUND_FX, TILE_SEL_SOUNDFX);
    Settings_DrawSelectionRect(SETTINGS_BTN_SAVE, TILE_SEL_SAVE);
    Settings_DrawSelectionRect(SETTINGS_BTN_BACK, TILE_SEL_BACK);
    Settings_DrawSelectionRect(SETTINGS_BTN_HOME, TILE_SEL_HOME);
}

//=============================================================================
// Rendering
//=============================================================================

/** Draw toggle pill (ON=green, OFF=red) */
static void Settings_DrawToggleRect(SettingsButtonSelected btn, bool enabled) {
    u16* map = BG_MAP_RAM_SUB(1);
    u16 tile = enabled ? TILE_GREEN : TILE_RED;
    int startY, endY;

    if (btn == SETTINGS_BTN_WIFI) {
        startY = SETTINGS_WIFI_TOGGLE_Y_START;
        endY = SETTINGS_WIFI_TOGGLE_Y_END;
    } else if (btn == SETTINGS_BTN_MUSIC) {
        startY = SETTINGS_MUSIC_TOGGLE_Y_START;
        endY = SETTINGS_MUSIC_TOGGLE_Y_END;
    } else if (btn == SETTINGS_BTN_SOUND_FX) {
        startY = SETTINGS_SOUNDFX_TOGGLE_Y_START;
        endY = SETTINGS_SOUNDFX_TOGGLE_Y_END;
    } else {
        return;
    }

    for (int row = startY; row < endY; row++) {
        for (int col = 0; col < SETTINGS_TOGGLE_WIDTH; col++) {
            map[row * 32 + (SETTINGS_TOGGLE_START_X + col)] = tile;
        }
    }
}

/** Draw selection rectangle on BG1 for a button */
static void Settings_DrawSelectionRect(SettingsButtonSelected btn, u16 tileIndex) {
    u16* map = BG_MAP_RAM_SUB(1);
    int startX, startY, endX, endY;

    switch (btn) {
        case SETTINGS_BTN_WIFI:
            startX = SETTINGS_WIFI_RECT_X_START;
            startY = SETTINGS_WIFI_RECT_Y_START;
            endX = SETTINGS_WIFI_RECT_X_END;
            endY = SETTINGS_WIFI_RECT_Y_END;
            break;
        case SETTINGS_BTN_MUSIC:
            startX = SETTINGS_MUSIC_RECT_X_START;
            startY = SETTINGS_MUSIC_RECT_Y_START;
            endX = SETTINGS_MUSIC_RECT_X_END;
            endY = SETTINGS_MUSIC_RECT_Y_END;
            break;
        case SETTINGS_BTN_SOUND_FX:
            startX = SETTINGS_SOUNDFX_RECT_X_START;
            startY = SETTINGS_SOUNDFX_RECT_Y_START;
            endX = SETTINGS_SOUNDFX_RECT_X_END;
            endY = SETTINGS_SOUNDFX_RECT_Y_END;
            break;
        case SETTINGS_BTN_SAVE:
            startX = SETTINGS_SAVE_RECT_X_START;
            startY = SETTINGS_SAVE_RECT_Y_START;
            endX = SETTINGS_SAVE_RECT_X_END;
            endY = SETTINGS_SAVE_RECT_Y_END;
            break;
        case SETTINGS_BTN_BACK:
            startX = SETTINGS_BACK_RECT_X_START;
            startY = SETTINGS_BACK_RECT_Y_START;
            endX = SETTINGS_BACK_RECT_X_END;
            endY = SETTINGS_BACK_RECT_Y_END;
            break;
        case SETTINGS_BTN_HOME:
            startX = SETTINGS_HOME_RECT_X_START;
            startY = SETTINGS_HOME_RECT_Y_START;
            endX = SETTINGS_HOME_RECT_X_END;
            endY = SETTINGS_HOME_RECT_Y_END;
            break;
        default:
            return;
    }

    for (int row = startY; row < endY; row++) {
        for (int col = startX; col < endX; col++) {
            map[row * 32 + col] = tileIndex;
        }
    }
}

/** Set highlight color for a button */
static void Settings_SetSelectionTint(SettingsButtonSelected btn, bool show) {
    if (btn < 0 || btn >= SETTINGS_BTN_COUNT) {
        return;
    }
    int paletteIndex = SETTINGS_SELECTION_PAL_BASE + btn;
    BG_PALETTE_SUB[paletteIndex] = show ? SETTINGS_SELECT_COLOR : BLACK;
}

//=============================================================================
// Input Handling
//=============================================================================

/** Handle D-Pad input for button selection */
static void Settings_HandleDPadInput(void) {
    int keys = keysDown();

    if (keys & KEY_UP) {
        selected = (selected - 1 + SETTINGS_BTN_COUNT) % SETTINGS_BTN_COUNT;
    }
    if (keys & KEY_DOWN) {
        selected = (selected + 1) % SETTINGS_BTN_COUNT;
    }

    // Handle left/right navigation for bottom row buttons
    if (keys & KEY_LEFT) {
        if (selected == SETTINGS_BTN_SAVE) {
            selected = SETTINGS_BTN_HOME;
        } else if (selected == SETTINGS_BTN_BACK) {
            selected = SETTINGS_BTN_SAVE;
        } else if (selected == SETTINGS_BTN_HOME) {
            selected = SETTINGS_BTN_BACK;
        }
    }
    if (keys & KEY_RIGHT) {
        if (selected == SETTINGS_BTN_SAVE) {
            selected = SETTINGS_BTN_BACK;
        } else if (selected == SETTINGS_BTN_BACK) {
            selected = SETTINGS_BTN_HOME;
        } else if (selected == SETTINGS_BTN_HOME) {
            selected = SETTINGS_BTN_SAVE;
        }
    }
}

/** Handle touch screen input for button selection */
static void Settings_HandleTouchInput(void) {
    if (!(keysHeld() & KEY_TOUCH)) {
        return;
    }

    touchPosition touch;
    touchRead(&touch);

    // Validate touch coordinates
    if (touch.px < 0 || touch.px > 256 || touch.py < 0 || touch.py > 192) {
        return;
    }

    Settings_CheckWifiTouch(&touch);
    Settings_CheckMusicTouch(&touch);
    Settings_CheckSoundFxTouch(&touch);
    Settings_CheckButtonTouch(&touch);
}

/** Check if touch is within WiFi button areas */
static void Settings_CheckWifiTouch(touchPosition* touch) {
    // WiFi text hitbox
    if (touch->px >= SETTINGS_WIFI_TEXT_X_MIN && touch->px <= SETTINGS_WIFI_TEXT_X_MAX &&
        touch->py >= SETTINGS_WIFI_TEXT_Y_MIN && touch->py <= SETTINGS_WIFI_TEXT_Y_MAX) {
        selected = SETTINGS_BTN_WIFI;
        return;
    }
    // WiFi pill hitbox
    if (touch->px >= SETTINGS_WIFI_PILL_X_MIN && touch->px <= SETTINGS_WIFI_PILL_X_MAX &&
        touch->py >= SETTINGS_WIFI_PILL_Y_MIN && touch->py <= SETTINGS_WIFI_PILL_Y_MAX) {
        selected = SETTINGS_BTN_WIFI;
    }
}

/** Check if touch is within Music button areas */
static void Settings_CheckMusicTouch(touchPosition* touch) {
    // Music text hitbox
    if (touch->px >= SETTINGS_MUSIC_TEXT_X_MIN && touch->px <= SETTINGS_MUSIC_TEXT_X_MAX &&
        touch->py >= SETTINGS_MUSIC_TEXT_Y_MIN && touch->py <= SETTINGS_MUSIC_TEXT_Y_MAX) {
        selected = SETTINGS_BTN_MUSIC;
        return;
    }
    // Music pill hitbox
    if (touch->px >= SETTINGS_MUSIC_PILL_X_MIN && touch->px <= SETTINGS_MUSIC_PILL_X_MAX &&
        touch->py >= SETTINGS_MUSIC_PILL_Y_MIN && touch->py <= SETTINGS_MUSIC_PILL_Y_MAX) {
        selected = SETTINGS_BTN_MUSIC;
    }
}

/** Check if touch is within Sound FX button areas */
static void Settings_CheckSoundFxTouch(touchPosition* touch) {
    // Sound FX text hitbox
    if (touch->px >= SETTINGS_SOUNDFX_TEXT_X_MIN && touch->px <= SETTINGS_SOUNDFX_TEXT_X_MAX &&
        touch->py >= SETTINGS_SOUNDFX_TEXT_Y_MIN && touch->py <= SETTINGS_SOUNDFX_TEXT_Y_MAX) {
        selected = SETTINGS_BTN_SOUND_FX;
        return;
    }
    // Sound FX pill hitbox
    if (touch->px >= SETTINGS_SOUNDFX_PILL_X_MIN && touch->px <= SETTINGS_SOUNDFX_PILL_X_MAX &&
        touch->py >= SETTINGS_SOUNDFX_PILL_Y_MIN && touch->py <= SETTINGS_SOUNDFX_PILL_Y_MAX) {
        selected = SETTINGS_BTN_SOUND_FX;
    }
}

/** Check if touch is within Save/Back/Home button areas */
static void Settings_CheckButtonTouch(touchPosition* touch) {
    // Save button
    if (touch->px >= SETTINGS_SAVE_TOUCH_X_MIN && touch->px <= SETTINGS_SAVE_TOUCH_X_MAX &&
        touch->py >= SETTINGS_SAVE_TOUCH_Y_MIN && touch->py <= SETTINGS_SAVE_TOUCH_Y_MAX) {
        selected = SETTINGS_BTN_SAVE;
        return;
    }
    // Back button
    if (touch->px >= SETTINGS_BACK_TOUCH_X_MIN && touch->px <= SETTINGS_BACK_TOUCH_X_MAX &&
        touch->py >= SETTINGS_BACK_TOUCH_Y_MIN && touch->py <= SETTINGS_BACK_TOUCH_Y_MAX) {
        selected = SETTINGS_BTN_BACK;
        return;
    }
    // Home button
    if (touch->px >= SETTINGS_HOME_TOUCH_X_MIN && touch->px <= SETTINGS_HOME_TOUCH_X_MAX &&
        touch->py >= SETTINGS_HOME_TOUCH_Y_MIN && touch->py <= SETTINGS_HOME_TOUCH_Y_MAX) {
        selected = SETTINGS_BTN_HOME;
    }
}
