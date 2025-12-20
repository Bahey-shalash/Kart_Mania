#include "game.h"
#include <nds.h>
#include "map_bottom.h"
#include "map_top.h"

//=============================================================================
// CONSTANTS
//=============================================================================
#define SINGLEPLAYER_BTN_COUNT 4

//=============================================================================
// GLOBAL STATE
//=============================================================================
static SingleplayerButton selected = SP_BTN_NONE;
static SingleplayerButton lastSelected = SP_BTN_NONE;

//=============================================================================
// GRAPHICS SETUP
//=============================================================================
void configureGraphics_MAIN_Singleplayer(void)
{
    REG_DISPCNT = MODE_5_2D | DISPLAY_BG2_ACTIVE;
    VRAM_A_CR = VRAM_ENABLE | VRAM_A_MAIN_BG;
}

void configBG_Main_Singleplayer(void)
{
    BGCTRL[2] = BG_BMP_BASE(0) | BgSize_B8_256x256;
    dmaCopy(map_topBitmap, BG_BMP_RAM(0), map_topBitmapLen);
    dmaCopy(map_topPal, BG_PALETTE, map_topPalLen);
    REG_BG2PA = 256;
    REG_BG2PC = 0;
    REG_BG2PB = 0;
    REG_BG2PD = 256;
}

void configureGraphics_Sub_Singleplayer(void)
{
    REG_DISPCNT_SUB = MODE_5_2D | DISPLAY_BG0_ACTIVE;
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
}

void configBG_Sub_Singleplayer(void)
{
    // BG0: Menu layer (front)
    BGCTRL_SUB[0] = BG_32x32 | BG_MAP_BASE(0) | BG_TILE_BASE(1) | BG_COLOR_256 | BG_PRIORITY(0);
    dmaCopy(map_bottomPal, BG_PALETTE_SUB, map_bottomPalLen);
    dmaCopy(map_bottomTiles, BG_TILE_RAM_SUB(1), map_bottomTilesLen);
    dmaCopy(map_bottomMap, BG_MAP_RAM_SUB(0), map_bottomMapLen);
}

//=============================================================================
// INPUT HANDLING
//=============================================================================
void handleDPadInputSingleplayer(void)
{
    int keys = keysDown();
    if (keys & KEY_UP)
        selected = (selected - 1 + SINGLEPLAYER_BTN_COUNT) % SINGLEPLAYER_BTN_COUNT;
    if (keys & KEY_DOWN)
        selected = (selected + 1) % SINGLEPLAYER_BTN_COUNT;
}

void handleTouchInputSingleplayer(void)
{
    if (!(keysHeld() & KEY_TOUCH))
        return;

    touchPosition touch;
    touchRead(&touch);

    if (touch.px < 0 || touch.px > 256 || touch.py < 0 || touch.py > 192)
    {
        return; // sanity check
    }

    // Map 1 - Scorching Sands (circle + text)
    // TODO: Get exact coordinates from your PNG using click_coords.py
    // Placeholder hitbox - update with actual coordinates
    if (touch.px >= 20 && touch.px <= 80 && touch.py >= 80 && touch.py <= 165)
    {
        selected = SP_BTN_MAP1;
        return;
    }

    // Map 2 - Alpine Rush (circle + text)
    // TODO: Get exact coordinates from your PNG
    if (touch.px >= 98 && touch.px <= 158 && touch.py >= 80 && touch.py <= 165)
    {
        selected = SP_BTN_MAP2;
        return;
    }

    // Map 3 - Neon Circuit (circle + text)
    // TODO: Get exact coordinates from your PNG
    if (touch.px >= 176 && touch.px <= 236 && touch.py >= 80 && touch.py <= 165)
    {
        selected = SP_BTN_MAP3;
        return;
    }

    // Home button (bottom right corner)
    if (touch.px >= 224 && touch.px <= 251 && touch.py >= 161 && touch.py <= 188)
    {
        selected = SP_BTN_HOME;
        return;
    }
}

//=============================================================================
// PUBLIC API
//=============================================================================
void Singleplayer_initialize(void)
{
    selected = SP_BTN_NONE;
    lastSelected = SP_BTN_NONE;

    configureGraphics_MAIN_Singleplayer();
    configBG_Main_Singleplayer();
    configureGraphics_Sub_Singleplayer();
    configBG_Sub_Singleplayer();
}

GameState Singleplayer_update(void)
{
    scanKeys();
    handleDPadInputSingleplayer();
    handleTouchInputSingleplayer();

    // Handle button activation on release
    if (keysUp() & (KEY_A | KEY_TOUCH))
    {
        switch (selected)
        {
        case SP_BTN_MAP1:
            // TODO: Load Scorching Sands map
            // return GAMEPLAY; // when ready
            break;
        case SP_BTN_MAP2:
            // TODO: Load Alpine Rush map
            // return GAMEPLAY; // when ready
            break;
        case SP_BTN_MAP3:
            // TODO: Load Neon Circuit map
            // return GAMEPLAY; // when ready
            break;
        case SP_BTN_HOME:
            return HOME_PAGE;
        default:
            break;
        }
    }

    return SINGLEPLAYER;
}