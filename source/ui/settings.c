/**
 * File: settings.c
 * ----------------
 * Description: Implementation of the Settings screen. Provides interactive
 *              controls for WiFi, music, and sound effects with visual toggle
 *              indicators (green pill = ON, red pill = OFF). Supports touch
 *              and D-pad input with selection highlighting. Handles persistent
 *              storage save/load and factory reset via START+SELECT combo.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 05.01.2026
 */

#include "settings.h"

#include <nds.h>
#include <string.h>

#include "../audio/sound.h"
#include "../core/context.h"
#include "../graphics/color.h"
#include "../storage/storage.h"
#include "nds_settings.h"
#include "settings_top.h"

//=============================================================================
// PRIVATE TYPES
//=============================================================================

/**
 * Settings screen button identifiers.
 */
typedef enum {
    SETTINGS_BTN_NONE = -1,
    SETTINGS_BTN_WIFI = 0,
    SETTINGS_BTN_MUSIC = 1,
    SETTINGS_BTN_SOUND_FX = 2,
    SETTINGS_BTN_SAVE = 3,
    SETTINGS_BTN_BACK = 4,
    SETTINGS_BTN_HOME = 5,
    SETTINGS_BTN_COUNT
} SettingsButtonSelected;

/**
 * Tile indices for settings UI elements.
 */
typedef enum {
    TILE_RED = 3,
    TILE_GREEN = 4,
    TILE_SEL_WIFI = 5,
    TILE_SEL_MUSIC = 6,
    TILE_SEL_SOUNDFX = 7,
    TILE_SEL_SAVE = 8,
    TILE_SEL_BACK = 9,
    TILE_SEL_HOME = 10
} SettingsTileIndex;

//=============================================================================
// PRIVATE CONSTANTS
//=============================================================================

#define SETTINGS_SELECTION_PAL_BASE 244  // Base palette index for selection tiles

//=============================================================================
// PRIVATE STATE
//=============================================================================

static SettingsButtonSelected selected = SETTINGS_BTN_NONE;
static SettingsButtonSelected lastSelected = SETTINGS_BTN_NONE;

//=============================================================================
// PRIVATE ASSETS
//=============================================================================

// Toggle indicator tiles (mapped to palette indices 254=red, 255=green)
static const u8 RedTile[64] = {[0 ... 63] = 254};
static const u8 GreenTile[64] = {[0 ... 63] = 255};

// Selection highlight tiles (one per button, mapped to sequential palettes)
static const u8 selectionTile0[64] = {[0 ... 63] = 244};
static const u8 selectionTile1[64] = {[0 ... 63] = 245};
static const u8 selectionTile2[64] = {[0 ... 63] = 246};
static const u8 selectionTile3[64] = {[0 ... 63] = 247};
static const u8 selectionTile4[64] = {[0 ... 63] = 248};
static const u8 selectionTile5[64] = {[0 ... 63] = 249};

//=============================================================================
// PRIVATE FUNCTION PROTOTYPES
//=============================================================================

static void Settings_ConfigureGraphicsMain(void);
static void Settings_ConfigureBackgroundMain(void);
static void Settings_ConfigureGraphicsSub(void);
static void Settings_ConfigureBackgroundSub(void);
static void Settings_HandleDPadInput(void);
static void Settings_HandleTouchInput(void);
static void Settings_SetSelectionTint(SettingsButtonSelected btn, bool show);
static void Settings_DrawSelectionRect(SettingsButtonSelected btn, u16 tileIndex);
static void Settings_DrawToggleRect(SettingsButtonSelected btn, bool enabled);
static void Settings_RefreshUI(void);
static void Settings_OnSavePressed(void);

//=============================================================================
// PUBLIC API
//=============================================================================

void Settings_Initialize(void) {
    selected = SETTINGS_BTN_NONE;
    lastSelected = SETTINGS_BTN_NONE;
    Settings_ConfigureGraphicsMain();
    Settings_ConfigureBackgroundMain();
    Settings_ConfigureGraphicsSub();
    Settings_ConfigureBackgroundSub();
}

GameState Settings_Update(void) {
    scanKeys();
    Settings_HandleDPadInput();
    Settings_HandleTouchInput();

    GameContext* ctx = GameContext_Get();

    // Update highlight when selection changes
    if (selected != lastSelected) {
        if (lastSelected != SETTINGS_BTN_NONE)
            Settings_SetSelectionTint(lastSelected, false);
        if (selected != SETTINGS_BTN_NONE)
            Settings_SetSelectionTint(selected, true);
        lastSelected = selected;
    }

    // Handle button activation on release
    if (keysUp() & (KEY_A | KEY_TOUCH)) {
        switch (selected) {
            case SETTINGS_BTN_WIFI: {
                bool wifiShouldBeEnabled = !ctx->userSettings.wifiEnabled;
                GameContext_SetWifiEnabled(wifiShouldBeEnabled);
                Settings_DrawToggleRect(SETTINGS_BTN_WIFI, wifiShouldBeEnabled);
                PlayDingSFX();
                break;
            }

            case SETTINGS_BTN_MUSIC: {
                bool musicShouldBeEnabled = !ctx->userSettings.musicEnabled;
                GameContext_SetMusicEnabled(musicShouldBeEnabled);
                Settings_DrawToggleRect(SETTINGS_BTN_MUSIC, musicShouldBeEnabled);
                PlayDingSFX();
                break;
            }

            case SETTINGS_BTN_SOUND_FX: {
                bool soundFxShouldBeEnabled = !ctx->userSettings.soundFxEnabled;
                PlayDingSFX();  // Play before potentially muting
                GameContext_SetSoundFxEnabled(soundFxShouldBeEnabled);
                Settings_DrawToggleRect(SETTINGS_BTN_SOUND_FX, soundFxShouldBeEnabled);
                break;
            }

            case SETTINGS_BTN_SAVE:
                Settings_OnSavePressed();
                PlayDingSFX();
                break;

            case SETTINGS_BTN_BACK:
                PlayCLICKSFX();
                return HOME_PAGE;

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
// SETTINGS MANAGEMENT
//=============================================================================

static void Settings_RefreshUI(void) {
    GameContext* ctx = GameContext_Get();

    // Update toggle visuals
    Settings_DrawToggleRect(SETTINGS_BTN_WIFI, ctx->userSettings.wifiEnabled);
    Settings_DrawToggleRect(SETTINGS_BTN_MUSIC, ctx->userSettings.musicEnabled);
    Settings_DrawToggleRect(SETTINGS_BTN_SOUND_FX, ctx->userSettings.soundFxEnabled);

    // Apply settings (triggers side effects)
    GameContext_SetWifiEnabled(ctx->userSettings.wifiEnabled);
    GameContext_SetMusicEnabled(ctx->userSettings.musicEnabled);
    GameContext_SetSoundFxEnabled(ctx->userSettings.soundFxEnabled);
}

static void Settings_OnSavePressed(void) {
    if ((keysHeld() & KEY_START) && (keysHeld() & KEY_SELECT)) {
        Storage_ResetToDefaults();
        Settings_RefreshUI();
    } else {
        Storage_SaveSettings();
    }
}

//=============================================================================
// GRAPHICS SETUP - MAIN SCREEN
//=============================================================================

static void Settings_ConfigureGraphicsMain(void) {
    REG_DISPCNT = MODE_5_2D | DISPLAY_BG2_ACTIVE;
    VRAM_A_CR = VRAM_ENABLE | VRAM_A_MAIN_BG;
}

static void Settings_ConfigureBackgroundMain(void) {
    BGCTRL[2] = BG_BMP_BASE(0) | BgSize_B8_256x256;
    dmaCopy(settings_topBitmap, BG_BMP_RAM(0), settings_topBitmapLen);
    dmaCopy(settings_topPal, BG_PALETTE, settings_topPalLen);
    REG_BG2PA = 256;
    REG_BG2PC = 0;
    REG_BG2PB = 0;
    REG_BG2PD = 256;
}

//=============================================================================
// TOGGLE RENDERING
//=============================================================================

static void Settings_DrawToggleRect(SettingsButtonSelected btn, bool enabled) {
    u16* map = BG_MAP_RAM_SUB(1);
    u16 tile = enabled ? TILE_GREEN : TILE_RED;

    int startX = 21;
    int width = 9;
    int startY, endY;

    if (btn == SETTINGS_BTN_WIFI) {
        startY = 1;
        endY = 5;
    } else if (btn == SETTINGS_BTN_MUSIC) {
        startY = 5;
        endY = 9;
    } else if (btn == SETTINGS_BTN_SOUND_FX) {
        startY = 9;
        endY = 13;
    } else {
        return;
    }

    for (int row = startY; row < endY; row++)
        for (int col = 0; col < width; col++)
            map[row * 32 + (startX + col)] = tile;
}

//=============================================================================
// SELECTION RENDERING
//=============================================================================

static void Settings_DrawSelectionRect(SettingsButtonSelected btn, u16 tileIndex) {
    u16* map = BG_MAP_RAM_SUB(1);
    int startX, startY, endX, endY;

    switch (btn) {
        case SETTINGS_BTN_WIFI:
            startX = 2;
            startY = 1;
            endX = 7;
            endY = 4;
            break;
        case SETTINGS_BTN_MUSIC:
            startX = 2;
            startY = 5;
            endX = 9;
            endY = 8;
            break;
        case SETTINGS_BTN_SOUND_FX:
            startX = 2;
            startY = 9;
            endX = 13;
            endY = 12;
            break;
        case SETTINGS_BTN_SAVE:
            startX = 4;
            startY = 15;
            endX = 14;
            endY = 23;
            break;
        case SETTINGS_BTN_BACK:
            startX = 12;
            startY = 15;
            endX = 20;
            endY = 23;
            break;
        case SETTINGS_BTN_HOME:
            startX = 20;
            startY = 15;
            endX = 28;
            endY = 23;
            break;
        default:
            return;
    }

    for (int row = startY; row < endY; row++)
        for (int col = startX; col < endX; col++)
            map[row * 32 + col] = tileIndex;
}

static void Settings_SetSelectionTint(SettingsButtonSelected btn, bool show) {
    if (btn < 0 || btn >= SETTINGS_BTN_COUNT)
        return;
    int paletteIndex = SETTINGS_SELECTION_PAL_BASE + btn;
    BG_PALETTE_SUB[paletteIndex] = show ? SETTINGS_SELECT_COLOR : BLACK;
}

//=============================================================================
// GRAPHICS SETUP - SUB SCREEN
//=============================================================================

static void Settings_ConfigureGraphicsSub(void) {
    REG_DISPCNT_SUB = MODE_0_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE;
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
}

static void Settings_ConfigureBackgroundSub(void) {
    // BG0: Menu layer (front) - Static graphics with setting labels
    BGCTRL_SUB[0] =
        BG_32x32 | BG_MAP_BASE(0) | BG_TILE_BASE(1) | BG_COLOR_256 | BG_PRIORITY(0);
    dmaCopy(nds_settingsPal, BG_PALETTE_SUB, nds_settingsPalLen);
    dmaCopy(nds_settingsTiles, BG_TILE_RAM_SUB(1), nds_settingsTilesLen);
    dmaCopy(nds_settingsMap, BG_MAP_RAM_SUB(0), nds_settingsMapLen);

    // BG1: Toggle and selection layer (back) - Dynamic highlights
    BGCTRL_SUB[1] =
        BG_32x32 | BG_COLOR_256 | BG_MAP_BASE(1) | BG_TILE_BASE(2) | BG_PRIORITY(1);

    // Load toggle indicator tiles
    dmaCopy(RedTile, (u8*)BG_TILE_RAM_SUB(2) + (3 * 64), 64);
    dmaCopy(GreenTile, (u8*)BG_TILE_RAM_SUB(2) + (4 * 64), 64);

    // Set toggle palette colors
    BG_PALETTE_SUB[254] = TOGGLE_OFF_COLOR;
    BG_PALETTE_SUB[255] = TOGGLE_ON_COLOR;

    // Load selection highlight tiles
    dmaCopy(selectionTile0, (u8*)BG_TILE_RAM_SUB(2) + (5 * 64), 64);
    dmaCopy(selectionTile1, (u8*)BG_TILE_RAM_SUB(2) + (6 * 64), 64);
    dmaCopy(selectionTile2, (u8*)BG_TILE_RAM_SUB(2) + (7 * 64), 64);
    dmaCopy(selectionTile3, (u8*)BG_TILE_RAM_SUB(2) + (8 * 64), 64);
    dmaCopy(selectionTile4, (u8*)BG_TILE_RAM_SUB(2) + (9 * 64), 64);
    dmaCopy(selectionTile5, (u8*)BG_TILE_RAM_SUB(2) + (10 * 64), 64);

    // Clear BG1 map
    memset(BG_MAP_RAM_SUB(1), 0, 32 * 24 * 2);

    GameContext* ctx = GameContext_Get();

    // Draw initial toggle states
    Settings_DrawToggleRect(SETTINGS_BTN_WIFI, ctx->userSettings.wifiEnabled);
    Settings_DrawToggleRect(SETTINGS_BTN_MUSIC, ctx->userSettings.musicEnabled);
    Settings_DrawToggleRect(SETTINGS_BTN_SOUND_FX, ctx->userSettings.soundFxEnabled);

    // Draw selection regions for all buttons
    Settings_DrawSelectionRect(SETTINGS_BTN_WIFI, TILE_SEL_WIFI);
    Settings_DrawSelectionRect(SETTINGS_BTN_MUSIC, TILE_SEL_MUSIC);
    Settings_DrawSelectionRect(SETTINGS_BTN_SOUND_FX, TILE_SEL_SOUNDFX);
    Settings_DrawSelectionRect(SETTINGS_BTN_SAVE, TILE_SEL_SAVE);
    Settings_DrawSelectionRect(SETTINGS_BTN_BACK, TILE_SEL_BACK);
    Settings_DrawSelectionRect(SETTINGS_BTN_HOME, TILE_SEL_HOME);
}

//=============================================================================
// INPUT HANDLING
//=============================================================================

static void Settings_HandleDPadInput(void) {
    int keys = keysDown();

    if (keys & KEY_UP)
        selected = (selected - 1 + SETTINGS_BTN_COUNT) % SETTINGS_BTN_COUNT;

    if (keys & KEY_DOWN)
        selected = (selected + 1) % SETTINGS_BTN_COUNT;

    if (keys & KEY_LEFT) {
        if (selected == SETTINGS_BTN_SAVE)
            selected = SETTINGS_BTN_HOME;
        else if (selected == SETTINGS_BTN_BACK)
            selected = SETTINGS_BTN_SAVE;
        else if (selected == SETTINGS_BTN_HOME)
            selected = SETTINGS_BTN_BACK;
    }

    if (keys & KEY_RIGHT) {
        if (selected == SETTINGS_BTN_SAVE)
            selected = SETTINGS_BTN_BACK;
        else if (selected == SETTINGS_BTN_BACK)
            selected = SETTINGS_BTN_HOME;
        else if (selected == SETTINGS_BTN_HOME)
            selected = SETTINGS_BTN_SAVE;
    }
}

static void Settings_HandleTouchInput(void) {
    if (!(keysHeld() & KEY_TOUCH))
        return;

    touchPosition touch;
    touchRead(&touch);

    // Validate touch coordinates
    if (touch.px < 0 || touch.px > 256 || touch.py < 0 || touch.py > 192)
        return;

    // WiFi text label
    if (touch.px > 23 && touch.px < 53 && touch.py > 10 && touch.py < 25) {
        selected = SETTINGS_BTN_WIFI;
        return;
    }

    // WiFi toggle pill
    if (touch.px > 175 && touch.px < 240 && touch.py > 10 && touch.py < 37) {
        selected = SETTINGS_BTN_WIFI;
        return;
    }

    // Music text label
    if (touch.px > 24 && touch.px < 69 && touch.py > 40 && touch.py < 55) {
        selected = SETTINGS_BTN_MUSIC;
        return;
    }

    // Music toggle pill
    if (touch.px > 175 && touch.px < 240 && touch.py > 40 && touch.py < 67) {
        selected = SETTINGS_BTN_MUSIC;
        return;
    }

    // Sound FX text label
    if (touch.px > 23 && touch.px < 99 && touch.py > 70 && touch.py < 85) {
        selected = SETTINGS_BTN_SOUND_FX;
        return;
    }

    // Sound FX toggle pill
    if (touch.px > 175 && touch.px < 240 && touch.py > 70 && touch.py < 97) {
        selected = SETTINGS_BTN_SOUND_FX;
        return;
    }

    // Save button (circular hitbox: center=64,152, diameter=48)
    if (touch.px > 40 && touch.px < 88 && touch.py > 128 && touch.py < 176) {
        selected = SETTINGS_BTN_SAVE;
        return;
    }

    // Back button (circular hitbox: center=128,152, diameter=48)
    if (touch.px > 104 && touch.px < 152 && touch.py > 128 && touch.py < 176) {
        selected = SETTINGS_BTN_BACK;
        return;
    }

    // Home button (circular hitbox: center=192,152, diameter=48)
    if (touch.px > 168 && touch.px < 216 && touch.py > 128 && touch.py < 176) {
        selected = SETTINGS_BTN_HOME;
        return;
    }
}
