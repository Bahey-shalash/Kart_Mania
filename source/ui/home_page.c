/**
 * home_page.c
 * Main menu/home page with single player, multiplayer, and settings options
 */

#include "../ui/home_page.h"

#include <string.h>

#include "../audio/sound.h"
#include "../core/context.h"
#include "../core/game_constants.h"
#include "../core/timer.h"
#include "../graphics/color.h"
#include "../network/multiplayer.h"
#include "ds_menu.h"
#include "home_top.h"
#include "kart_home.h"

//=============================================================================
// Private Macros
//=============================================================================

// Macro to define menu item hitboxes
#define MENU_ITEM_ROW(i)                                                        \
    {HOME_MENU_X, HOME_MENU_Y_START + (i)*HOME_MENU_SPACING, HOME_MENU_WIDTH, \
     HOME_MENU_HEIGHT}

#define MENU_COUNT HOME_BTN_COUNT

//=============================================================================
// Private Assets
//=============================================================================

// Highlight layer tiles (indices 251-253)
static const u8 selectionMaskTile0[64] = {[0 ... 63] = HOME_HL_PAL_BASE + 0};
static const u8 selectionMaskTile1[64] = {[0 ... 63] = HOME_HL_PAL_BASE + 1};
static const u8 selectionMaskTile2[64] = {[0 ... 63] = HOME_HL_PAL_BASE + 2};

// Highlight Y positions (in tiles) for each menu button
static const int highlightTileY[MENU_COUNT] = {4, 10, 17};

// Touch hitboxes for menu buttons
static const MenuItemHitBox homeBtnHitbox[MENU_COUNT] = {
    [HOME_BTN_SINGLEPLAYER] = MENU_ITEM_ROW(0),
    [HOME_BTN_MULTIPLAYER] = MENU_ITEM_ROW(1),
    [HOME_BTN_SETTINGS] = MENU_ITEM_ROW(2),
};

//=============================================================================
// Private State
//=============================================================================

static HomeButtonSelected selected = HOME_BTN_NONE;
static HomeButtonSelected lastSelected = HOME_BTN_NONE;
static HomeKartSprite homeKart;

//=============================================================================
// Private Function Prototypes
//=============================================================================

// Main screen (top)
static void HomePage_ConfigureMainGraphics(void);
static void HomePage_ConfigureMainBackground(void);
static void HomePage_ConfigureKartSprite(void);
static void HomePage_MoveKart(void);

// Sub screen (bottom)
static void HomePage_ConfigureSubGraphics(void);
static void HomePage_ConfigureSubBackground(void);

// Rendering
static void HomePage_DrawSelectionRect(HomeButtonSelected btn, u16 tileIndex);
static void HomePage_SetSelectionTint(int buttonIndex, bool show);

// Input handling
static void HomePage_HandleDPadInput(void);
static void HomePage_HandleTouchInput(void);

//=============================================================================
// Public API
//=============================================================================

/** Initialize the Home Page screen */
void HomePage_Initialize(void) {
    selected = HOME_BTN_NONE;
    lastSelected = HOME_BTN_NONE;

    HomePage_ConfigureMainGraphics();
    HomePage_ConfigureMainBackground();
    HomePage_ConfigureKartSprite();
    initTimer();

    HomePage_ConfigureSubGraphics();
    HomePage_ConfigureSubBackground();
}

/** Update Home Page screen (returns next GameState) */
GameState HomePage_Update(void) {
    scanKeys();
    HomePage_HandleDPadInput();
    HomePage_HandleTouchInput();

    // Update highlight when selection changes
    if (selected != lastSelected) {
        if (lastSelected != HOME_BTN_NONE) {
            HomePage_SetSelectionTint(lastSelected, false);
        }
        if (selected != HOME_BTN_NONE) {
            HomePage_SetSelectionTint(selected, true);
        }
        lastSelected = selected;
    }

    // Handle button activation
    if (keysUp() & (KEY_A | KEY_TOUCH)) {
        if (selected != HOME_BTN_NONE) {
            PlayCLICKSFX();
        }
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
                if (playerID == -1) {
                    return REINIT_HOME;
                }
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

/** VBlank callback for animating home screen sprite */
void HomePage_OnVBlank(void) {
    HomePage_MoveKart();
}

/** Cleanup Home Page resources */
void HomePage_Cleanup(void) {
    if (homeKart.gfx) {
        oamFreeGfx(&oamMain, homeKart.gfx);
        homeKart.gfx = NULL;
    }
}

//=============================================================================
// Main Screen (Top) Graphics Setup
//=============================================================================

/** Configure main screen display registers */
static void HomePage_ConfigureMainGraphics(void) {
    REG_DISPCNT = MODE_5_2D | DISPLAY_BG2_ACTIVE;
    VRAM_A_CR = VRAM_ENABLE | VRAM_A_MAIN_BG;
}

/** Load main screen background bitmap */
static void HomePage_ConfigureMainBackground(void) {
    BGCTRL[2] = BG_BMP_BASE(0) | BgSize_B8_256x256;
    dmaCopy(home_topBitmap, BG_BMP_RAM(0), home_topBitmapLen);
    dmaCopy(home_topPal, BG_PALETTE, home_topPalLen);
    REG_BG2PA = 256;
    REG_BG2PC = 0;
    REG_BG2PB = 0;
    REG_BG2PD = 256;
}

/** Configure animated kart sprite */
static void HomePage_ConfigureKartSprite(void) {
    VRAM_B_CR = VRAM_ENABLE | VRAM_B_MAIN_SPRITE;
    oamInit(&oamMain, SpriteMapping_1D_32, false);
    homeKart.id = 0;
    homeKart.x = HOME_KART_INITIAL_X;
    homeKart.y = HOME_KART_Y;
    homeKart.gfx =
        oamAllocateGfx(&oamMain, SpriteSize_64x64, SpriteColorFormat_256Color);
    swiCopy(kart_homePal, SPRITE_PALETTE, kart_homePalLen / 2);
    swiCopy(kart_homeTiles, homeKart.gfx, kart_homeTilesLen / 2);
}

/** Animate kart sprite (scrolling left to right) */
static void HomePage_MoveKart(void) {
    oamSet(&oamMain, homeKart.id, homeKart.x, homeKart.y, 0, 0, SpriteSize_64x64,
           SpriteColorFormat_256Color, homeKart.gfx, -1, false, false, false, false,
           false);
    homeKart.x++;
    if (homeKart.x > HOME_KART_OFFSCREEN_X) {
        homeKart.x = HOME_KART_INITIAL_X;
    }
    oamUpdate(&oamMain);
}

//=============================================================================
// Sub Screen (Bottom) Graphics Setup
//=============================================================================

/** Configure sub screen display registers */
static void HomePage_ConfigureSubGraphics(void) {
    REG_DISPCNT_SUB = MODE_0_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE;
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
}

/** Configure sub screen background layers and load assets */
static void HomePage_ConfigureSubBackground(void) {
    // BG0: Menu layer (front)
    BGCTRL_SUB[0] = BG_32x32 | BG_MAP_BASE(0) | BG_TILE_BASE(1) |
                    BG_COLOR_256 | BG_PRIORITY(0);
    dmaCopy(ds_menuPal, BG_PALETTE_SUB, ds_menuPalLen);
    dmaCopy(ds_menuTiles, BG_TILE_RAM_SUB(1), ds_menuTilesLen);
    dmaCopy(ds_menuMap, BG_MAP_RAM_SUB(0), ds_menuMapLen);

    // BG1: Highlight layer (behind)
    BGCTRL_SUB[1] = BG_32x32 | BG_MAP_BASE(1) | BG_TILE_BASE(2) |
                    BG_COLOR_256 | BG_PRIORITY(1);

    // Initialize highlight palette slots
    BG_PALETTE_SUB[HOME_HL_PAL_BASE + 0] = BLACK;
    BG_PALETTE_SUB[HOME_HL_PAL_BASE + 1] = BLACK;
    BG_PALETTE_SUB[HOME_HL_PAL_BASE + 2] = BLACK;

    // Clear first tile
    memset((u8*)BG_TILE_RAM_SUB(2) + 0 * 64, 0, 64);

    // Load highlight tiles
    dmaCopy(selectionMaskTile0, (u8*)BG_TILE_RAM_SUB(2) + (1 * 64), 64);
    dmaCopy(selectionMaskTile1, (u8*)BG_TILE_RAM_SUB(2) + (2 * 64), 64);
    dmaCopy(selectionMaskTile2, (u8*)BG_TILE_RAM_SUB(2) + (3 * 64), 64);

    // Draw highlight rectangles for all buttons
    HomePage_DrawSelectionRect(HOME_BTN_SINGLEPLAYER, 1);
    HomePage_DrawSelectionRect(HOME_BTN_MULTIPLAYER, 2);
    HomePage_DrawSelectionRect(HOME_BTN_SETTINGS, 3);
}

//=============================================================================
// Rendering
//=============================================================================

/** Draw selection rectangle on BG1 for a button */
static void HomePage_DrawSelectionRect(HomeButtonSelected btn, u16 tileIndex) {
    if (btn < 0 || btn >= HOME_BTN_COUNT) {
        return;
    }
    u16* map = BG_MAP_RAM_SUB(1);
    int startY = highlightTileY[btn];
    for (int row = 0; row < HOME_HIGHLIGHT_TILE_HEIGHT; row++) {
        for (int col = 0; col < HOME_HIGHLIGHT_TILE_WIDTH; col++) {
            map[(startY + row) * 32 + (HOME_HIGHLIGHT_TILE_X + col)] = tileIndex;
        }
    }
}

/** Set highlight color for a button */
static void HomePage_SetSelectionTint(int buttonIndex, bool show) {
    if (buttonIndex < 0 || buttonIndex >= MENU_COUNT) {
        return;
    }
    int paletteIndex = HOME_HL_PAL_BASE + buttonIndex;
    BG_PALETTE_SUB[paletteIndex] =
        show ? MENU_BUTTON_HIGHLIGHT_COLOR : MENU_HIGHLIGHT_OFF_COLOR;
}

//=============================================================================
// Input Handling
//=============================================================================

/** Handle D-Pad input for button selection */
static void HomePage_HandleDPadInput(void) {
    int keys = keysDown();
    if (keys & KEY_UP) {
        selected = (selected - 1 + MENU_COUNT) % MENU_COUNT;
    }
    if (keys & KEY_DOWN) {
        selected = (selected + 1) % MENU_COUNT;
    }
}

/** Handle touch screen input for button selection */
static void HomePage_HandleTouchInput(void) {
    if (!(keysHeld() & KEY_TOUCH)) {
        return;
    }
    touchPosition touch;
    touchRead(&touch);
    for (int i = 0; i < MENU_COUNT; i++) {
        if (touch.px >= homeBtnHitbox[i].x &&
            touch.px < homeBtnHitbox[i].x + homeBtnHitbox[i].width &&
            touch.py >= homeBtnHitbox[i].y &&
            touch.py < homeBtnHitbox[i].y + homeBtnHitbox[i].height) {
            selected = i;
            return;
        }
    }
}
