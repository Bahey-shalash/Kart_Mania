#include "settings.h"

#include <nds.h>
#include <string.h>

#include "color.h"
#include "nds_settings.h"
#include "settings_top.h"

//=============================================================================
// DUMMY FUNCTIONS - implement these later
//=============================================================================

void onWifiToggle(ToggleState wifiEnabled) {
    // TODO: enable/disable wifi based on state
    if (wifiEnabled == TOGGLE_ON) {
        // Enable wifi
    } else {
        // Disable wifi
    }
}

void onMusicToggle(ToggleState musicEnabled) {
    // TODO: enable/disable music based on state
    if (musicEnabled == TOGGLE_ON) {
        // Enable music
    } else {
        // Disable music
    }
}

void onSoundFxToggle(ToggleState soundFxEnabled) {
    // TODO: enable/disable sound effects based on state
    if (soundFxEnabled == TOGGLE_ON) {
        // Enable sound effects
    } else {
        // Disable sound effects
    }
}

void onSavePressed(void) {
    // TODO: save settings to extrenal storage
}

//=============================================================================
// CONSTANTS - Hitbox coordinates from the PNG
//=============================================================================
#define SETTINGS_MENU_COUNT SETTINGS_BTN_COUNT

//=============================================================================
// GLOBAL STATE
//=============================================================================

static SettingsButtonSelected selected = SETTINGS_BTN_NONE;
static SettingsButtonSelected lastSelected = SETTINGS_BTN_NONE;

// Toggle states (persisted)
static ToggleState wifiEnabled = TOGGLE_ON;
static ToggleState musicEnabled = TOGGLE_ON;
static ToggleState soundFxEnabled = TOGGLE_ON;

//=============================================================================
// HITBOXES
//=============================================================================

//=============================================================================
// MAIN ENGINE (Top Screen)
//=============================================================================

void configureGraphics_MAIN_Settings(void) {
    REG_DISPCNT = MODE_5_2D | DISPLAY_BG2_ACTIVE;
    VRAM_A_CR = VRAM_ENABLE | VRAM_A_MAIN_BG;
}

void configBG_Main_Settings(void) {
    BGCTRL[2] = BG_BMP_BASE(0) | BgSize_B8_256x256;
    dmaCopy(settings_topBitmap, BG_BMP_RAM(0), settings_topBitmapLen);
    dmaCopy(settings_topPal, BG_PALETTE, settings_topPalLen);
    REG_BG2PA = 256;
    REG_BG2PC = 0;
    REG_BG2PB = 0;
    REG_BG2PD = 256;
}

//=============================================================================
// TOGGLE STATE LAYER (pills - bitmap mode)
//=============================================================================

// Pill pixel coordinates with ~2px margin inward
static const int pillCoords[3][4] = {
    // { x1, y1, x2, y2 }
    {176, 12, 236, 36},  // wifi
    {176, 44, 236, 63},  // music
    {176, 73, 236, 92},  // soundfx
};

static void drawRectBitmap(u16* fb, int x1, int y1, int x2, int y2, u16 color) {
    for (int y = y1; y < y2; y++) {
        for (int x = x1; x < x2; x++) {
            fb[y * 256 + x] = color;
        }
    }
}

void Settings_setToggleVisual(SettingsButtonSelected btn, ToggleState state) {
    if (btn < SETTINGS_BTN_WIFI || btn > SETTINGS_BTN_SOUND_FX)
        return;
    u16* fb = (u16*)BG_BMP_RAM_SUB(2);
    u16 color = (state == TOGGLE_ON) ? GREEN : RED;
    drawRectBitmap(fb, pillCoords[btn][0], pillCoords[btn][1], pillCoords[btn][2],
                   pillCoords[btn][3], color);
}

//=============================================================================
// SUB ENGINE (Bottom Screen)
//=============================================================================
// todo: tiles to show slection
// todo: tiles to show if option is enabled/disabled for music/soundfx/wifi
void configGraphics_Sub_SETTINGS(void) {
    REG_DISPCNT_SUB = MODE_5_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG2_ACTIVE;
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
}

void configBackground_Sub_SETTINGS(void) {
    // BG0: Menu layer (front)
    BGCTRL_SUB[0] =
        BG_32x32 | BG_MAP_BASE(0) | BG_TILE_BASE(1) | BG_COLOR_256 | BG_PRIORITY(0);
    dmaCopy(nds_settingsPal, BG_PALETTE_SUB, nds_settingsPalLen);
    dmaCopy(nds_settingsTiles, BG_TILE_RAM_SUB(1), nds_settingsTilesLen);
    dmaCopy(nds_settingsMap, BG_MAP_RAM_SUB(0), nds_settingsMapLen);

    // todo: highlight layer (behind)

    // BG2: 16bpp bitmap for toggle colors (behind)
    BGCTRL_SUB[2] = BG_BMP_BASE(2) | BgSize_B16_256x256 | BG_PRIORITY(1);
    // Clear bitmap to black
    

    // Affine identity
    REG_BG2PA_SUB = 256;
    REG_BG2PB_SUB = 0;
    REG_BG2PC_SUB = 0;
    REG_BG2PD_SUB = 256;

    // Draw initial toggle states
    Settings_setToggleVisual(SETTINGS_BTN_WIFI, wifiEnabled);
    Settings_setToggleVisual(SETTINGS_BTN_MUSIC, musicEnabled);
    Settings_setToggleVisual(SETTINGS_BTN_SOUND_FX, soundFxEnabled);
}

//=============================================================================
// INPUT HANDLING
//=============================================================================

void handleDPadInputSettings(void) {
    int keys = keysDown();
    if (keys & KEY_UP)
        selected = (selected - 1 + SETTINGS_MENU_COUNT) % SETTINGS_MENU_COUNT;
    if (keys & KEY_DOWN)
        selected = (selected + 1) % SETTINGS_MENU_COUNT;
}

void handleTouchInputSettings(void) {
    if (!(keysHeld() & KEY_TOUCH))
        return;

    touchPosition touch;
    touchRead(&touch);

    if (touch.px < 0 || touch.px > 256 || touch.py < 0 || touch.py > 192) {
        return;  // sanity check
    }
    // wifi text
    if (touch.px > 24 && touch.px < 53 && touch.py > 10 && touch.py < 28) {
        selected = SETTINGS_BTN_WIFI;
        return;
    }
    // wifi pill
    if (touch.px > 174 && touch.px < 238 && touch.py > 10 && touch.py < 38) {
        selected = SETTINGS_BTN_WIFI;
        return;
    }
    // music text
    if (touch.px > 24 && touch.px < 69 && touch.py > 42 && touch.py < 54) {
        selected = SETTINGS_BTN_MUSIC;
        return;
    }
    // music pill
    if (touch.px > 174 && touch.px < 238 && touch.py > 42 && touch.py < 65) {
        selected = SETTINGS_BTN_MUSIC;
        return;
    }
    // sound fx text
    if (touch.px > 24 && touch.px < 98 && touch.py > 70 && touch.py < 82) {
        selected = SETTINGS_BTN_SOUND_FX;
        return;
    }
    // sound fx pill
    if (touch.px > 174 && touch.px < 238 && touch.py > 71 && touch.py < 94) {
        selected = SETTINGS_BTN_SOUND_FX;
        return;
    }
    // save button
    if (touch.px > 37 && touch.px < 90 && touch.py > 126 && touch.py < 178) {
        selected = SETTINGS_BTN_SAVE;
        return;
    }
    // back button
    if (touch.px > 101 && touch.px < 153 && touch.py > 126 && touch.py < 178) {
        selected = SETTINGS_BTN_BACK;
        return;
    }
    // home button
    if (touch.px > 165 && touch.px < 217 && touch.py > 126 && touch.py < 178) {
        selected = SETTINGS_BTN_HOME;
        return;
    }
}

//=============================================================================
// PUBLIC API
//=============================================================================

void Settings_initialize(void) {
    selected = SETTINGS_BTN_NONE;
    lastSelected = SETTINGS_BTN_NONE;

    // Main Screen
    configBG_Main_Settings();
    configureGraphics_MAIN_Settings();

    // Sub Screen
    configGraphics_Sub_SETTINGS();
    configBackground_Sub_SETTINGS();
}

GameState Settings_update(void) {
    scanKeys();
    handleDPadInputSettings();
    handleTouchInputSettings();

    // Handle button activation on release
    if (keysUp() & (KEY_A | KEY_TOUCH)) {
        switch (selected) {
            case SETTINGS_BTN_WIFI:
                wifiEnabled = !wifiEnabled;
                Settings_setToggleVisual(SETTINGS_BTN_WIFI, wifiEnabled);
                onWifiToggle(wifiEnabled);
                break;
            case SETTINGS_BTN_MUSIC:
                musicEnabled = !musicEnabled;
                Settings_setToggleVisual(SETTINGS_BTN_MUSIC, musicEnabled);
                onMusicToggle(musicEnabled);
                break;
            case SETTINGS_BTN_SOUND_FX:
                soundFxEnabled = !soundFxEnabled;
                Settings_setToggleVisual(SETTINGS_BTN_SOUND_FX, soundFxEnabled);
                onSoundFxToggle(soundFxEnabled);
                break;
            case SETTINGS_BTN_SAVE:
                onSavePressed();
                break;
            case SETTINGS_BTN_BACK:
                return HOME_PAGE;
            case SETTINGS_BTN_HOME:
                return HOME_PAGE;
            default:
                break;
        }
    }

    return SETTINGS;
}