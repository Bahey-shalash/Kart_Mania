#include "settings.h"

#include <nds.h>
#include <string.h>

#include "color.h"
#include "nds_settings.h"
#include "settings_top.h"

//=============================================================================
// DUMMY FUNCTIONS - implement these later
//=============================================================================

void onWifiToggle(ToggleState state) {
    // TODO: enable/disable wifi based on state
    (void)state;
}

void onMusicToggle(ToggleState state) {
    // TODO: enable/disable music based on state
    (void)state;
}

void onSoundFxToggle(ToggleState state) {
    // TODO: enable/disable sound effects based on state
    (void)state;
}

void onSavePressed(void) {
    // TODO: save settings to SRAM/file
}

//=============================================================================
// CONSTANTS - Hitbox coordinates from the PNG
//=============================================================================



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
// SUB ENGINE (Bottom Screen)
//=============================================================================
// todo: tiles to show slection
// todo: tiles to show if option is enabled/disabled for music/soundfx/wifi
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

     //todo: highlight layer (behind)
}

//=============================================================================
// PUBLIC API
//=============================================================================

void Settings_initialize(void) {
    configBG_Main_Settings();
    configureGraphics_MAIN_Settings();
    configGraphics_Sub_SETTINGS();
    configBackground_Sub_SETTINGS();
}

GameState Settings_update(void) {
    // scanKeys();
    // handleInput();

    return SETTINGS;
}