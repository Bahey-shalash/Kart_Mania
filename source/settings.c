#include "settings.h"

#include <nds.h>

#include "nds_settings.h"

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