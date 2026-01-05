/**
 * File: home_page.c
 * -----------------
 * Description: Implementation of the Home Page screen. Main menu interface
 *              with three options: Singleplayer, Multiplayer, and Settings.
 *              Features animated kart sprite on top screen and interactive
 *              menu with selection highlighting on bottom screen. Handles
 *              WiFi initialization for multiplayer mode.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 05.01.2026
 */

#include "home_page.h"

#include <string.h>

#include "../audio/sound.h"
#include "../core/context.h"
#include "../core/timer.h"
#include "../graphics/color.h"
#include "../network/multiplayer.h"
#include "ds_menu.h"
#include "home_top.h"
#include "kart_home.h"

//=============================================================================
// PRIVATE CONSTANTS
//=============================================================================

// Selection highlight palette
#define HOME_HL_PAL_BASE 251  // Base palette index for selection tiles

// Menu layout constants
#define HOME_MENU_X 32          // Menu left edge (pixels)
#define HOME_MENU_WIDTH 192     // Menu item width (pixels)
#define HOME_MENU_HEIGHT 40     // Menu item height (pixels)
#define HOME_MENU_SPACING 54    // Vertical spacing between items (pixels)
#define HOME_MENU_Y_START 24    // First menu item Y position (pixels)

// Highlight tile positioning
#define HIGHLIGHT_TILE_X 6       // Highlight rectangle left edge (tile cols)
#define HIGHLIGHT_TILE_WIDTH 20  // Highlight rectangle width (tiles)
#define HIGHLIGHT_TILE_HEIGHT 3  // Highlight rectangle height (tiles)

// UI layout macro for menu item hitboxes
#define MENU_ITEM_ROW(i)                                                        \
    {HOME_MENU_X, HOME_MENU_Y_START + (i)*HOME_MENU_SPACING, HOME_MENU_WIDTH, \
     HOME_MENU_HEIGHT}

//=============================================================================
// PRIVATE STATE
//=============================================================================

static HomeButtonSelected selected = HOME_BTN_NONE;
static HomeButtonSelected lastSelected = HOME_BTN_NONE;
static HomeKartSprite homeKart;

//=============================================================================
// PRIVATE ASSETS
//=============================================================================

// Selection highlight tiles (one per menu item, mapped to sequential palettes 251-253)
static const u8 selectionMaskTile0[64] = {[0 ... 63] = HOME_HL_PAL_BASE + 0};
static const u8 selectionMaskTile1[64] = {[0 ... 63] = HOME_HL_PAL_BASE + 1};
static const u8 selectionMaskTile2[64] = {[0 ... 63] = HOME_HL_PAL_BASE + 2};

// Highlight tile Y positions (rows) for each menu item
static const int highlightTileY[HOME_BTN_COUNT] = {4, 10, 17};

// Touch hitboxes for each menu item
static const MenuItemHitBox homeBtnHitbox[HOME_BTN_COUNT] = {
    [HOME_BTN_SINGLEPLAYER] = MENU_ITEM_ROW(0),
    [HOME_BTN_MULTIPLAYER] = MENU_ITEM_ROW(1),
    [HOME_BTN_SETTINGS] = MENU_ITEM_ROW(2),
};
//=============================================================================
// PRIVATE FUNCTION PROTOTYPES
//=============================================================================

static void HomePage_ConfigureGraphicsMain(void);
static void HomePage_ConfigureBackgroundMain(void);
static void HomePage_ConfigureKartSprite(void);
static void HomePage_MoveKartSprite(void);
static void HomePage_ConfigureGraphicsSub(void);
static void HomePage_ConfigureBackgroundSub(void);
static void HomePage_DrawSelectionRect(HomeButtonSelected btn, u16 tileIndex);
static void HomePage_SetSelectionTint(int buttonIndex, bool show);
static void HomePage_HandleDPadInput(void);
static void HomePage_HandleTouchInput(void);

//=============================================================================
// PUBLIC API
//=============================================================================

void HomePage_Initialize(void) {
    selected = HOME_BTN_NONE;
    lastSelected = HOME_BTN_NONE;
    HomePage_ConfigureGraphicsMain();
    HomePage_ConfigureBackgroundMain();
    HomePage_ConfigureKartSprite();
    initTimer();
    HomePage_ConfigureGraphicsSub();
    HomePage_ConfigureBackgroundSub();
}

GameState HomePage_Update(void) {
    scanKeys();
    HomePage_HandleDPadInput();
    HomePage_HandleTouchInput();

    // Update highlight when selection changes
    if (selected != lastSelected) {
        if (lastSelected != HOME_BTN_NONE)
            HomePage_SetSelectionTint(lastSelected, false);
        if (selected != HOME_BTN_NONE)
            HomePage_SetSelectionTint(selected, true);
        lastSelected = selected;
    }

    // Handle button activation on release
    if (keysUp() & (KEY_A | KEY_TOUCH)) {
        if (selected != HOME_BTN_NONE)
            PlayCLICKSFX();

        switch (selected) {
            case HOME_BTN_SINGLEPLAYER:
                GameContext_SetMultiplayerMode(false);
                return MAPSELECTION;

            case HOME_BTN_MULTIPLAYER: {
                GameContext* ctx = GameContext_Get();
                if (!ctx->userSettings.wifiEnabled) {
                    PlayDingSFX();
                    return HOME_PAGE;
                }
                int playerID = Multiplayer_Init();
                if (playerID == -1)
                    return REINIT_HOME;
                GameContext_SetMultiplayerMode(true);
                return MULTIPLAYER_LOBBY;
            }

            case HOME_BTN_SETTINGS:
                return SETTINGS;

            default:
                break;
        }
    }

    return HOME_PAGE;
}

void HomePage_OnVBlank(void) {
    HomePage_MoveKartSprite();
}

void HomePage_Cleanup(void) {
    if (homeKart.gfx) {
        oamFreeGfx(&oamMain, homeKart.gfx);
        homeKart.gfx = NULL;
    }
}

//=============================================================================
// GRAPHICS SETUP - MAIN SCREEN
//=============================================================================

static void HomePage_ConfigureGraphicsMain(void) {
    REG_DISPCNT = MODE_5_2D | DISPLAY_BG2_ACTIVE;
    VRAM_A_CR = VRAM_ENABLE | VRAM_A_MAIN_BG;
}

static void HomePage_ConfigureBackgroundMain(void) {
    BGCTRL[2] = BG_BMP_BASE(0) | BgSize_B8_256x256;
    dmaCopy(home_topBitmap, BG_BMP_RAM(0), home_topBitmapLen);
    dmaCopy(home_topPal, BG_PALETTE, home_topPalLen);
    REG_BG2PA = 256;
    REG_BG2PC = 0;
    REG_BG2PB = 0;
    REG_BG2PD = 256;
}

//=============================================================================
// SPRITE ANIMATION
//=============================================================================

static void HomePage_ConfigureKartSprite(void) {
    VRAM_B_CR = VRAM_ENABLE | VRAM_B_MAIN_SPRITE;
    oamInit(&oamMain, SpriteMapping_1D_32, false);
    homeKart.id = 0;
    homeKart.x = -64;
    homeKart.y = 120;
    homeKart.gfx =
        oamAllocateGfx(&oamMain, SpriteSize_64x64, SpriteColorFormat_256Color);
    swiCopy(kart_homePal, SPRITE_PALETTE, kart_homePalLen / 2);
    swiCopy(kart_homeTiles, homeKart.gfx, kart_homeTilesLen / 2);
}

static void HomePage_MoveKartSprite(void) {
    oamSet(&oamMain, homeKart.id, homeKart.x, homeKart.y, 0, 0, SpriteSize_64x64,
           SpriteColorFormat_256Color, homeKart.gfx, -1, false, false, false, false,
           false);
    homeKart.x++;
    if (homeKart.x > 256)
        homeKart.x = -64;
    oamUpdate(&oamMain);
}

//=============================================================================
// GRAPHICS SETUP - SUB SCREEN
//=============================================================================

static void HomePage_ConfigureGraphicsSub(void) {
    REG_DISPCNT_SUB = MODE_0_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE;
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
}

static void HomePage_ConfigureBackgroundSub(void) {
    // BG0: Menu layer (front) - Static menu graphics
    BGCTRL_SUB[0] =
        BG_32x32 | BG_MAP_BASE(0) | BG_TILE_BASE(1) | BG_COLOR_256 | BG_PRIORITY(0);
    dmaCopy(ds_menuPal, BG_PALETTE_SUB, ds_menuPalLen);
    dmaCopy(ds_menuTiles, BG_TILE_RAM_SUB(1), ds_menuTilesLen);
    dmaCopy(ds_menuMap, BG_MAP_RAM_SUB(0), ds_menuMapLen);

    // BG1: Selection highlight layer (back) - Dynamic highlights
    BGCTRL_SUB[1] =
        BG_32x32 | BG_MAP_BASE(1) | BG_TILE_BASE(2) | BG_COLOR_256 | BG_PRIORITY(1);

    // Initialize highlight palette slots to black (invisible)
    BG_PALETTE_SUB[HOME_HL_PAL_BASE + 0] = BLACK;
    BG_PALETTE_SUB[HOME_HL_PAL_BASE + 1] = BLACK;
    BG_PALETTE_SUB[HOME_HL_PAL_BASE + 2] = BLACK;

    // Clear tile 0 (transparent tile)
    memset((u8*)BG_TILE_RAM_SUB(2) + 0 * 64, 0, 64);

    // Load selection highlight tiles
    dmaCopy(selectionMaskTile0, (u8*)BG_TILE_RAM_SUB(2) + (1 * 64), 64);
    dmaCopy(selectionMaskTile1, (u8*)BG_TILE_RAM_SUB(2) + (2 * 64), 64);
    dmaCopy(selectionMaskTile2, (u8*)BG_TILE_RAM_SUB(2) + (3 * 64), 64);

    // Draw selection rectangles for all menu items
    HomePage_DrawSelectionRect(HOME_BTN_SINGLEPLAYER, 1);
    HomePage_DrawSelectionRect(HOME_BTN_MULTIPLAYER, 2);
    HomePage_DrawSelectionRect(HOME_BTN_SETTINGS, 3);
}

//=============================================================================
// SELECTION RENDERING
//=============================================================================

static void HomePage_DrawSelectionRect(HomeButtonSelected btn, u16 tileIndex) {
    if (btn < 0 || btn >= HOME_BTN_COUNT)
        return;

    u16* map = BG_MAP_RAM_SUB(1);
    int startY = highlightTileY[btn];

    for (int row = 0; row < HIGHLIGHT_TILE_HEIGHT; row++)
        for (int col = 0; col < HIGHLIGHT_TILE_WIDTH; col++)
            map[(startY + row) * 32 + (HIGHLIGHT_TILE_X + col)] = tileIndex;
}

static void HomePage_SetSelectionTint(int buttonIndex, bool show) {
    if (buttonIndex < 0 || buttonIndex >= HOME_BTN_COUNT)
        return;

    int paletteIndex = HOME_HL_PAL_BASE + buttonIndex;
    BG_PALETTE_SUB[paletteIndex] =
        show ? MENU_BUTTON_HIGHLIGHT_COLOR : MENU_HIGHLIGHT_OFF_COLOR;
}

//=============================================================================
// INPUT HANDLING
//=============================================================================

static void HomePage_HandleDPadInput(void) {
    int keys = keysDown();

    if (keys & KEY_UP)
        selected = (selected - 1 + HOME_BTN_COUNT) % HOME_BTN_COUNT;

    if (keys & KEY_DOWN)
        selected = (selected + 1) % HOME_BTN_COUNT;
}

static void HomePage_HandleTouchInput(void) {
    if (!(keysHeld() & KEY_TOUCH))
        return;

    touchPosition touch;
    touchRead(&touch);

    for (int i = 0; i < HOME_BTN_COUNT; i++) {
        if (touch.px >= homeBtnHitbox[i].x &&
            touch.px < homeBtnHitbox[i].x + homeBtnHitbox[i].width &&
            touch.py >= homeBtnHitbox[i].y &&
            touch.py < homeBtnHitbox[i].y + homeBtnHitbox[i].height) {
            selected = i;
            return;
        }
    }
}
