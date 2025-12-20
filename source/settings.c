#include "settings.h"

#include <nds.h>

#include "nds_settings.h"
#include "settings_top.h"

//=============================================================================
// CONSTANTS
//=============================================================================

#define BG_SCROLL_MAX 320
#define BG_SCROLL_STEP 8

//=============================================================================
// STATE
//=============================================================================

static int scroll_y;

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

static void configGraphics_Sub(void) {
    REG_DISPCNT_SUB = MODE_0_2D | DISPLAY_BG2_ACTIVE;
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
}

static void configBackground_Sub(void) {
    BGCTRL_SUB[2] = BG_32x64 | BG_MAP_BASE(0) | BG_TILE_BASE(2) | BG_COLOR_256;

    swiCopy(nds_settingsPal, BG_PALETTE_SUB, nds_settingsPalLen / 2);
    swiCopy(nds_settingsTiles, BG_TILE_RAM_SUB(2), nds_settingsTilesLen / 2);

    for (int i = 0; i < 32; i++)
        dmaCopy(&nds_settingsMap[i * 32], &((u16*)BG_MAP_RAM_SUB(0))[i * 32], 64);

    for (int i = 0; i < 32; i++)
        dmaCopy(&nds_settingsMap[(i + 32) * 32], &((u16*)BG_MAP_RAM_SUB(1))[i * 32],
                64);

    REG_BG2VOFS_SUB = 0;
}

//=============================================================================
// INPUT
//=============================================================================

static void handleInput(void) {
    int keys = keysHeld();

    if (keys & KEY_UP) {
        scroll_y -= BG_SCROLL_STEP;
        if (scroll_y < 0)
            scroll_y = 0;
    } else if (keys & KEY_DOWN) {
        scroll_y += BG_SCROLL_STEP;
        if (scroll_y > BG_SCROLL_MAX)
            scroll_y = BG_SCROLL_MAX;
    }

    REG_BG2VOFS_SUB = scroll_y;
}

//=============================================================================
// PUBLIC API
//=============================================================================

void Settings_initialize(void) {
    scroll_y = 0;
    configBG_Main_Settings();
    configureGraphics_MAIN_Settings();
    configGraphics_Sub();
    configBackground_Sub();
}

GameState Settings_update(void) {
    scanKeys();
    handleInput();

    if (keysDown() & KEY_B) {
        return HOME_PAGE;
    }

    return SETTINGS;
}