#include "settings.h"
#include <nds.h>
#include <string.h>
#include "nds_settings.h"

/* =========================
   Cursor tiles (OBVIOUS)
   ========================= */
u8 cursorArrowTile[64] = {
    254, 254, 254, 254, 254, 254, 254, 254,
    254, 254, 254, 254, 254, 254, 254, 254,
    254, 254, 254, 254, 254, 254, 254, 254,
    254, 254, 254, 254, 254, 254, 254, 254,
    254, 254, 254, 254, 254, 254, 254, 254,
    254, 254, 254, 254, 254, 254, 254, 254,
    254, 254, 254, 254, 254, 254, 254, 254,
    254, 254, 254, 254, 254, 254, 254, 254};

u8 emptyTile[64] = {0};

/* =========================
   Settings list
   ========================= */
typedef struct
{
    int x;
    int y;
    int scrollTarget;
} SettingItem;

#define SETTING_COUNT 9

static SettingItem settings[SETTING_COUNT] = {
    {8, 22, 0},
    {8, 62, 0},
    {8, 102, 0},
    {8, 167, 18},
    {8, 202, 53},
    {8, 237, 88},
    {8, 272, 123},
    {8, 307, 158},
    {8, 342, 193},
};

static int selectedSetting = 0;
static int lastSelectedSetting = -1;

/* =========================
   Forward declarations
   ========================= */
static void setCursorOverlay(int settingIndex, bool show);

/* =========================
   Init / Config
   ========================= */
void Settings_initialize(void)
{
    memset(BG_MAP_RAM_SUB(0), 0, 32 * 32 * 2);
    memset(BG_MAP_RAM_SUB(1), 0, 32 * 32 * 2);
    memset(BG_MAP_RAM_SUB(3), 0, 32 * 32 * 2);

    selectedSetting = 0;
    lastSelectedSetting = -1;

    Settings_configGraphics_Sub();
    Settings_configBackground_Sub();

    // Draw initial cursor on first setting
    setCursorOverlay(0, true);
    lastSelectedSetting = 0;
}

void Settings_configGraphics_Sub(void)
{
    REG_DISPCNT_SUB =
        MODE_0_2D |
        DISPLAY_BG2_ACTIVE |
        DISPLAY_BG1_ACTIVE;

    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
}

void Settings_configBackground_Sub(void)
{
    /* =========================
       BG2: scrolling background
       ========================= */
    BGCTRL_SUB[2] =
        BG_32x64 |
        BG_MAP_BASE(0) |
        BG_TILE_BASE(2) |
        BG_COLOR_256;  // Lower priority than BG1 so cursor shows on top

    swiCopy(nds_settingsPal, BG_PALETTE_SUB, nds_settingsPalLen / 2);
    swiCopy(nds_settingsTiles, BG_TILE_RAM_SUB(2), nds_settingsTilesLen / 2);

    for (int i = 0; i < 32; i++)
        dmaCopy(&nds_settingsMap[i * 32],
                BG_MAP_RAM_SUB(0) + i * 32, 64);

    for (int i = 0; i < 32; i++)
        dmaCopy(&nds_settingsMap[(i + 32) * 32],
                BG_MAP_RAM_SUB(1) + i * 32, 64);

    REG_BG2VOFS_SUB = 0;

    /* =========================
       BG1: cursor overlay (FRONT)
       ========================= */
    BGCTRL_SUB[1] =
        BG_32x32 |
        BG_MAP_BASE(3) |
        BG_TILE_BASE(1) |
        BG_COLOR_256: // Higher priority (lower number) = renders on top

    // Copy cursor tiles
    dmaCopy(emptyTile, BG_TILE_RAM_SUB(1), 64);
    dmaCopy(cursorArrowTile, BG_TILE_RAM_SUB(1) + 64, 64);

    // Set neon green cursor color - use index 254 to avoid conflicts
    // Set it AFTER settings palette to ensure it's not overwritten
    BG_PALETTE_SUB[254] = RGB15(0, 31, 0); // neon green

    u16 *map = (u16 *)BG_MAP_RAM_SUB(3);
    memset(map, 0, 32 * 32 * 2);
}

/* =========================
   Cursor drawing helper
   ========================= */
static void setCursorOverlay(int settingIndex, bool show)
{
    u16 *overlayMap = (u16 *)BG_MAP_RAM_SUB(3);
    SettingItem *s = &settings[settingIndex];

    // Settings coordinates are background-relative, but BG1 is screen-relative
    // So we need to account for the scroll offset
    int scrollOffset = s->scrollTarget;
    int screenX = s->x;
    int screenY = s->y - scrollOffset;  // Convert to screen coordinates

    // Convert pixel coordinates to tile coordinates
    int mapX = screenX / 8;
    int mapY = screenY / 8;

    // Only draw if on screen (y >= 0)
    if (screenY >= 0 && screenY < 192)
    {
        // Draw a 2x2 tile cursor (16x16 pixels)
        for (int ty = 0; ty < 2; ty++)
        {
            for (int tx = 0; tx < 2; tx++)
            {
                int finalY = mapY + ty;
                if (finalY >= 0 && finalY < 24)  // Screen is 24 tiles tall
                {
                    int mapIndex = finalY * 32 + (mapX + tx);
                    if (mapIndex >= 0 && mapIndex < 32 * 32)  // Bounds check
                    {
                        overlayMap[mapIndex] = show ? 1 : 0; // 1 = cursor tile, 0 = empty
                    }
                }
            }
        }
    }
}

/* =========================
   Update
   ========================= */
void Settings_update(void)
{
    scanKeys();
    int keys = keysDown();

    if (keys & KEY_UP)
        selectedSetting = (selectedSetting - 1 + SETTING_COUNT) % SETTING_COUNT;
    if (keys & KEY_DOWN)
        selectedSetting = (selectedSetting + 1) % SETTING_COUNT;

    if (selectedSetting != lastSelectedSetting)
    {
        // Clear old cursor
        if (lastSelectedSetting != -1)
            setCursorOverlay(lastSelectedSetting, false);

        // Update scroll position for BG2
        int scrollTarget = settings[selectedSetting].scrollTarget;
        REG_BG2VOFS_SUB = scrollTarget;
        // Draw new cursor
        setCursorOverlay(selectedSetting, true);

        lastSelectedSetting = selectedSetting;
    }
}

void Settings_cleanup(void)
{
    REG_DISPCNT_SUB &= ~(DISPLAY_BG1_ACTIVE | DISPLAY_BG2_ACTIVE);

    memset(BG_MAP_RAM_SUB(0), 0, 32 * 32 * 2);
    memset(BG_MAP_RAM_SUB(1), 0, 32 * 32 * 2);
    memset(BG_MAP_RAM_SUB(3), 0, 32 * 32 * 2);

    selectedSetting = 0;
    lastSelectedSetting = -1;
}
