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

u8 RedTile[64] = {254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254,
                  254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254,
                  254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254,
                  254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254,
                  254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254};

u8 GreenTile[64] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
                    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};

#define TILE_RED 3
#define TILE_GREEN 4

static void drawToggleRect(int toggleIndex, ToggleState state) {
    u16* map = BG_MAP_RAM_SUB(1);
    u16 tile = (state == TOGGLE_ON) ? TILE_GREEN : TILE_RED;

    int startX = 21;
    int width = 9;
    int startY, endY;

    if (toggleIndex == SETTINGS_BTN_WIFI) {
        startY = 1;
        endY = 5;
    } else if (toggleIndex == SETTINGS_BTN_MUSIC) {
        startY = 5;
        endY = 9;
    } else if (toggleIndex == SETTINGS_BTN_SOUND_FX) {
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
// SELECTION HIGHLIGHT TILES (BG2)
//=============================================================================

#define SETTINGS_SELECTION_PAL_BASE 244

static const u8 selectionTile0[64] = {[0 ... 63] = 244};
static const u8 selectionTile1[64] = {[0 ... 63] = 245};
static const u8 selectionTile2[64] = {[0 ... 63] = 246};
static const u8 selectionTile3[64] = {[0 ... 63] = 247};
static const u8 selectionTile4[64] = {[0 ... 63] = 248};
static const u8 selectionTile5[64] = {[0 ... 63] = 249};

static void drawSelectionRect(SettingsButtonSelected btn, u16 tileIndex) {
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
            return;  // includes SETTINGS_BTN_NONE
    }

    for (int row = startY; row < endY; row++)
        for (int col = startX; col < endX; col++)
            map[row * 32 + col] = tileIndex;
}

static void Settings_setSelectionTint(SettingsButtonSelected btn, bool show) {
    if (btn < 0 || btn >= SETTINGS_BTN_COUNT)
        return;
    int paletteIndex = SETTINGS_SELECTION_PAL_BASE + btn;
    BG_PALETTE_SUB[paletteIndex] = show ? SETTINGS_SELECT_COLOR : BLACK;
}

//=============================================================================
// SUB ENGINE (Bottom Screen)
//=============================================================================
void configGraphics_Sub_SETTINGS(void) {
    REG_DISPCNT_SUB = MODE_0_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE;
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
}

void configBackground_Sub_SETTINGS(void) {
    // BG0: Menu layer (front)
    BGCTRL_SUB[0] =
        BG_32x32 | BG_MAP_BASE(0) | BG_TILE_BASE(1) | BG_COLOR_256 | BG_PRIORITY(0);
    dmaCopy(nds_settingsPal, BG_PALETTE_SUB, nds_settingsPalLen);
    dmaCopy(nds_settingsTiles, BG_TILE_RAM_SUB(1), nds_settingsTilesLen);
    dmaCopy(nds_settingsMap, BG_MAP_RAM_SUB(0), nds_settingsMapLen);

    BGCTRL_SUB[1] =
        BG_32x32 | BG_COLOR_256 | BG_MAP_BASE(1) | BG_TILE_BASE(2) | BG_PRIORITY(1);

    dmaCopy(RedTile, (u8*)BG_TILE_RAM_SUB(2) + (3 * 64), 64);
    dmaCopy(GreenTile, (u8*)BG_TILE_RAM_SUB(2) + (4 * 64), 64);

    BG_PALETTE_SUB[254] = TOGGLE_OFF_COLOR;  // Red
    BG_PALETTE_SUB[255] = TOGGLE_ON_COLOR;   // Green

    /* for (int row = 0; row < 24; row++)
        for (int col = 0; col < 32; col++)
            BG_MAP_RAM_SUB(1)[row * 32 + col] = ((row * 32 + col + (row % 2)) % 2) + 3;
     */
    // for testing only

    // Load selection tiles
    dmaCopy(selectionTile0, (u8*)BG_TILE_RAM_SUB(2) + (5 * 64), 64);
    dmaCopy(selectionTile1, (u8*)BG_TILE_RAM_SUB(2) + (6 * 64), 64);
    dmaCopy(selectionTile2, (u8*)BG_TILE_RAM_SUB(2) + (7 * 64), 64);
    dmaCopy(selectionTile3, (u8*)BG_TILE_RAM_SUB(2) + (8 * 64), 64);
    dmaCopy(selectionTile4, (u8*)BG_TILE_RAM_SUB(2) + (9 * 64), 64);
    dmaCopy(selectionTile5, (u8*)BG_TILE_RAM_SUB(2) + (10 * 64), 64);

    // Clear BG1 map
    memset(BG_MAP_RAM_SUB(1), 0, 32 * 24 * 2);
    // Draw initial toggle states
    drawToggleRect(SETTINGS_BTN_WIFI, wifiEnabled);
    drawToggleRect(SETTINGS_BTN_MUSIC, musicEnabled);
    drawToggleRect(SETTINGS_BTN_SOUND_FX, soundFxEnabled);

    // Draw selection areas
    drawSelectionRect(SETTINGS_BTN_WIFI, TILE_SEL_WIFI);
    drawSelectionRect(SETTINGS_BTN_MUSIC, TILE_SEL_MUSIC);
    drawSelectionRect(SETTINGS_BTN_SOUND_FX, TILE_SEL_SOUNDFX);
    drawSelectionRect(SETTINGS_BTN_SAVE, TILE_SEL_SAVE);
    drawSelectionRect(SETTINGS_BTN_BACK, TILE_SEL_BACK);
    drawSelectionRect(SETTINGS_BTN_HOME, TILE_SEL_HOME);
}

//=============================================================================
// INPUT HANDLING
//=============================================================================

void handleDPadInputSettings(void) {
    int keys = keysDown();

    if (keys & KEY_UP) {
        selected = (selected - 1 + SETTINGS_BTN_COUNT) % SETTINGS_BTN_COUNT;
    }

    if (keys & KEY_DOWN) {
        selected = (selected + 1) % SETTINGS_BTN_COUNT;
    }

    if (keys & KEY_LEFT) {
        if (selected == SETTINGS_BTN_SAVE) {
            selected = SETTINGS_BTN_HOME;  // wrap to right
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
            selected = SETTINGS_BTN_SAVE;  // wrap to left
        }
    }
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
    if (touch.px > 23 && touch.px < 53 && touch.py > 10 && touch.py < 25) {
        selected = SETTINGS_BTN_WIFI;
        return;
    }
    // wifi pill
    if (touch.px > 175 && touch.px < 240 && touch.py > 10 && touch.py < 37) {
        selected = SETTINGS_BTN_WIFI;
        return;
    }
    // music text
    if (touch.px > 24 && touch.px < 69 && touch.py > 40 && touch.py < 55) {
        selected = SETTINGS_BTN_MUSIC;
        return;
    }
    // music pill
    if (touch.px > 175 && touch.px < 240 && touch.py > 40 && touch.py < 67) {
        selected = SETTINGS_BTN_MUSIC;
        return;
    }
    // sound fx text
    if (touch.px > 23 && touch.px < 99 && touch.py > 70 && touch.py < 85) {
        selected = SETTINGS_BTN_SOUND_FX;
        return;
    }
    // sound fx pill
    if (touch.px > 175 && touch.px < 240 && touch.py > 70 && touch.py < 97) {
        selected = SETTINGS_BTN_SOUND_FX;
        return;
    }
    // save button (circle: center=64,152 diameter=48)
    if (touch.px > 40 && touch.px < 88 && touch.py > 128 && touch.py < 176) {
        selected = SETTINGS_BTN_SAVE;
        return;
    }
    // back button (circle: center=128,152 diameter=48)
    if (touch.px > 104 && touch.px < 152 && touch.py > 128 && touch.py < 176) {
        selected = SETTINGS_BTN_BACK;
        return;
    }
    // home button (circle: center=192,152 diameter=48)
    if (touch.px > 168 && touch.px < 216 && touch.py > 128 && touch.py < 176) {
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
    configureGraphics_MAIN_Settings();
    configBG_Main_Settings();

    // Sub Screen
    configGraphics_Sub_SETTINGS();
    configBackground_Sub_SETTINGS();
}

GameState Settings_update(void) {
    scanKeys();
    handleDPadInputSettings();
    handleTouchInputSettings();

    // Update highlight when selection changes
    if (selected != lastSelected) {
        if (lastSelected != SETTINGS_BTN_NONE)
            Settings_setSelectionTint(lastSelected, false);
        if (selected != SETTINGS_BTN_NONE)
            Settings_setSelectionTint(selected, true);
        lastSelected = selected;
    }

    // Handle button activation on release
    if (keysUp() & (KEY_A | KEY_TOUCH)) {
        switch (selected) {
            case SETTINGS_BTN_WIFI:
                wifiEnabled = !wifiEnabled;
                drawToggleRect(SETTINGS_BTN_WIFI, wifiEnabled);
                onWifiToggle(wifiEnabled);
                break;
            case SETTINGS_BTN_MUSIC:
                musicEnabled = !musicEnabled;
                drawToggleRect(SETTINGS_BTN_MUSIC, musicEnabled);
                onMusicToggle(musicEnabled);
                break;
            case SETTINGS_BTN_SOUND_FX:
                soundFxEnabled = !soundFxEnabled;
                drawToggleRect(SETTINGS_BTN_SOUND_FX, soundFxEnabled);
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