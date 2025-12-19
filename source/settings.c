#include "settings.h"

#include <nds.h>
#include <string.h>

#include "nds_settings.h"
static const u8 emptyTile[64] =
    {
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0};

static const u8 RedToggleTile[64] = {
    2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2};

static const u8 GreenToggleTile[64] = {
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3};

static const u8 ATile[64] = {
    0, 0, 4, 4, 4, 4, 0, 0,
    0, 4, 4, 0, 0, 4, 4, 0,
    4, 4, 0, 0, 0, 0, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 0, 0, 0, 0, 4, 4,
    4, 4, 0, 0, 0, 0, 4, 4,
    4, 4, 0, 0, 0, 0, 4, 4,
    0, 0, 0, 0, 0, 0, 0, 0};

static const u8 BTile[64] = {
    4, 4, 4, 4, 4, 4, 0, 0,
    4, 4, 0, 0, 0, 4, 4, 0,
    4, 4, 0, 0, 0, 4, 4, 0,
    4, 4, 4, 4, 4, 4, 0, 0,
    4, 4, 0, 0, 0, 4, 4, 0,
    4, 4, 0, 0, 0, 4, 4, 0,
    4, 4, 4, 4, 4, 4, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0};

static const u8 XTile[64] = {
    4, 4, 0, 0, 0, 0, 4, 4,
    0, 4, 4, 0, 0, 4, 4, 0,
    0, 0, 4, 4, 4, 4, 0, 0,
    0, 0, 0, 4, 4, 0, 0, 0,
    0, 0, 4, 4, 4, 4, 0, 0,
    0, 4, 4, 0, 0, 4, 4, 0,
    4, 4, 0, 0, 0, 0, 4, 4,
    0, 0, 0, 0, 0, 0, 0, 0};

static const u8 YTile[64] = {
    4, 4, 0, 0, 0, 0, 4, 4,
    0, 4, 4, 0, 0, 4, 4, 0,
    0, 0, 4, 4, 4, 4, 0, 0,
    0, 0, 0, 4, 4, 0, 0, 0,
    0, 0, 0, 4, 4, 0, 0, 0,
    0, 0, 0, 4, 4, 0, 0, 0,
    0, 0, 0, 4, 4, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0};

static const u8 RTile[64] = {
    4, 4, 4, 4, 4, 4, 0, 0,
    4, 4, 0, 0, 0, 4, 4, 0,
    4, 4, 0, 0, 0, 4, 4, 0,
    4, 4, 4, 4, 4, 4, 0, 0,
    4, 4, 0, 0, 0, 4, 4, 0,
    4, 4, 0, 4, 4, 0, 0, 0,
    4, 4, 0, 0, 4, 4, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0};

static const u8 LTile[64] = {
    4, 4, 0, 0, 0, 0, 0, 0,
    4, 4, 0, 0, 0, 0, 0, 0,
    4, 4, 0, 0, 0, 0, 0, 0,
    4, 4, 0, 0, 0, 0, 0, 0,
    4, 4, 0, 0, 0, 0, 0, 0,
    4, 4, 0, 0, 0, 0, 0, 0,
    4, 4, 4, 4, 4, 4, 4, 4,
    0, 0, 0, 0, 0, 0, 0, 0};

static const u8 STTile[64] = {
    0, 4, 4, 4, 4, 4, 4, 0,
    4, 4, 0, 0, 0, 0, 0, 0,
    0, 4, 4, 4, 4, 4, 0, 0,
    0, 0, 0, 0, 0, 4, 4, 0,
    0, 4, 4, 4, 4, 4, 0, 0,
    4, 4, 0, 0, 0, 0, 0, 0,
    0, 4, 4, 4, 4, 4, 4, 0,
    0, 0, 0, 0, 0, 0, 0, 0};

static const u8 SETile[64] = {
    0, 4, 4, 4, 4, 4, 4, 0,
    4, 4, 0, 0, 0, 0, 0, 0,
    0, 4, 4, 4, 4, 4, 0, 0,
    4, 4, 0, 0, 0, 0, 0, 0,
    0, 4, 4, 4, 4, 4, 0, 0,
    4, 4, 0, 0, 0, 0, 0, 0,
    0, 4, 4, 4, 4, 4, 4, 0,
    0, 0, 0, 0, 0, 0, 0, 0};

int scroll_y = 0;
/* =========================
   Init / Config
   ========================= */
void Settings_initialize(void)
{
    Settings_configGraphics_Sub();
    Settings_configBackground_Sub();
}

void Settings_configGraphics_Sub(void)
{
    REG_DISPCNT_SUB = MODE_0_2D | DISPLAY_BG2_ACTIVE | DISPLAY_BG0_ACTIVE;

    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
}

void Settings_configBackground_Sub(void)
{
    BGCTRL_SUB[2] = BG_32x64 | BG_MAP_BASE(0) | BG_TILE_BASE(2) |
                    BG_COLOR_256 | BG_PRIORITY(1);

    swiCopy(nds_settingsPal, BG_PALETTE_SUB, nds_settingsPalLen / 2);
    swiCopy(nds_settingsTiles, BG_TILE_RAM_SUB(2), nds_settingsTilesLen / 2);

    for (int i = 0; i < 32; i++)
        dmaCopy(&nds_settingsMap[i * 32], &((u16 *)BG_MAP_RAM_SUB(0))[i * 32], 64);

    for (int i = 0; i < 32; i++)
        dmaCopy(&nds_settingsMap[(i + 32) * 32], &((u16 *)BG_MAP_RAM_SUB(1))[i * 32],
                64);

    REG_BG2VOFS_SUB = 0;

    BGCTRL_SUB[1] = BG_32x32 | BG_MAP_BASE(3) | BG_TILE_BASE(6) | BG_COLOR_256 | BG_PRIORITY(0);
    ;
    dmaCopy(emptyTile, (u8 *)BG_TILE_RAM(6), 64);
    dmaCopy(RedToggleTile, (u8 *)BG_TILE_RAM(6) + 64, 64);
    /*
    dmaCopy(ATile, (u8 *)BG_TILE_RAM(3), 64);
    dmaCopy(BTile, (u8 *)BG_TILE_RAM(3) + 64, 64);
    dmaCopy(XTile, (u8 *)BG_TILE_RAM(3) + 128, 64);
    dmaCopy(YTile, (u8 *)BG_TILE_RAM(3) + 192, 64);
    dmaCopy(LTile, (u8 *)BG_TILE_RAM(3) + 256, 64);
    dmaCopy(RTile, (u8 *)BG_TILE_RAM(3) + 320, 64);
    dmaCopy(STTile, (u8 *)BG_TILE_RAM(3) + 384, 64);
    dmaCopy(SETile, (u8 *)BG_TILE_RAM(3) + 448, 64);
    */

    BG_PALETTE_SUB[2] = ARGB16(1, 31, 0, 0);
    BG_PALETTE_SUB[3] = ARGB16(1, 0, 31, 0);
    BG_PALETTE_SUB[4] = ARGB16(1, 31, 31, 31);
    for (int i = 0; i < 24; i++)
        for (int j = 0; j < 32; j++)
        {
            BG_MAP_RAM_SUB(3)
            [i * 32 + j] = 1;
        }
    /*
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
        {
            BG_MAP_RAM_SUB(12)[i] = 1;
        }
    */
}

void Settings_update(void)
{
    scanKeys();
    int keys = keysHeld();

    if (keys & KEY_UP)
    {
        scroll_y -= BG_SCROLL_STEP;
        if (scroll_y < 0)
        {
            scroll_y = 0;
        }
    }
    else if (keys & KEY_DOWN)
    {
        scroll_y += BG_SCROLL_STEP;
        if (scroll_y > BG_SCROLL_MAX)
        {
            scroll_y = BG_SCROLL_MAX;
        }
    }

    REG_BG2VOFS_SUB = scroll_y;
}

void Settings_cleanup(void)
{
    REG_DISPCNT_SUB &= ~(DISPLAY_BG1_ACTIVE | DISPLAY_BG2_ACTIVE);
}
