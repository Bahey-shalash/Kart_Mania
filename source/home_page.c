#include "home_page.h"

#include <string.h>

#include "color.h"
#include "ds_menu.h"
#include "home_top.h"
#include "kart_home.h"
#include "sound.h"
#include "timer.h"

// Highlight layer / VRAM indices
#define HOME_HL_PAL_BASE 251
#define HOME_MENU_X 32
#define HOME_MENU_WIDTH 192
#define HOME_MENU_HEIGHT 40
#define HOME_MENU_SPACING 54
#define HOME_MENU_Y_START 24
#define HIGHLIGHT_TILE_X 6
#define HIGHLIGHT_TILE_WIDTH 20
#define HIGHLIGHT_TILE_HEIGHT 3

// Macro to define menu item hit boxes
#define MENU_COUNT HOME_BTN_COUNT

// UI layout (touch/hitboxes)
#define MENU_ITEM_ROW(i)                                                        \
    {HOME_MENU_X, HOME_MENU_Y_START + (i) * HOME_MENU_SPACING, HOME_MENU_WIDTH, \
     HOME_MENU_HEIGHT}

//=============================================================================
// ASSETS / TABLES (tiles, coordinates, hitboxes)
//=============================================================================

// Highlight layer tiles (8bpp, indices 251-253)
static const u8 selectionMaskTile0[64] = {[0 ... 63] = HOME_HL_PAL_BASE + 0};
static const u8 selectionMaskTile1[64] = {[0 ... 63] = HOME_HL_PAL_BASE + 1};
static const u8 selectionMaskTile2[64] = {[0 ... 63] = HOME_HL_PAL_BASE + 2};

static const int highlightTileY[MENU_COUNT] = {4, 10, 17};

static const MenuItemHitBox homeBtnHitbox[MENU_COUNT] = {
    [HOME_BTN_SINGLEPLAYER] = MENU_ITEM_ROW(0),
    [HOME_BTN_MULTIPLAYER] = MENU_ITEM_ROW(1),
    [HOME_BTN_SETTINGS] = MENU_ITEM_ROW(2),
};

//=============================================================================
// GLOBAL STATE
//=============================================================================

static HomeButtonSelected selected = HOME_BTN_NONE;
static HomeButtonSelected lastSelected = HOME_BTN_NONE;
static HomeKartSprite homeKart;
//=============================================================================
// PRIVATE PROTOTYPES
//=============================================================================

static void configureGraphics_MAIN_home_page(void);
static void configBG_Main_homepage(void);
static void configurekartSpritehome(void);
static void move_homeKart(void);
static void drawSelectionUnderlayRect(HomeButtonSelected btn, u16 tileIndex);
static void HomePage_setSelectionTint(int buttonIndex, bool show);
static void configGraphics_Sub_HOME(void);
static void configBackground_Sub_HOME(void);
static void handleDPadInputHOME(void);
static void handleTouchInputHOME(void);

//=============================================================================
// Public API implementation
//=============================================================================

void HomePage_initialize(void) {
    selected = HOME_BTN_NONE;
    lastSelected = HOME_BTN_NONE;

    // Main engine (top screen)
    configureGraphics_MAIN_home_page();
    configBG_Main_homepage();
    configurekartSpritehome();

    // move_homeKart();
    initTimer();

    // Sub engine (bottom screen)
    configGraphics_Sub_HOME();
    configBackground_Sub_HOME();
}

GameState HomePage_update(void) {
    scanKeys();
    handleDPadInputHOME();
    handleTouchInputHOME();

    if (selected != lastSelected) {
        if (lastSelected != HOME_BTN_NONE)
            HomePage_setSelectionTint(lastSelected, false);
        if (selected != HOME_BTN_NONE)
            HomePage_setSelectionTint(selected, true);
        lastSelected = selected;
    }

    if (keysUp() & (KEY_A | KEY_TOUCH)) {
        if (selected != HOME_BTN_NONE) {
            PlayCLICKSFX();  // CLICK sound effect
        }
        switch (selected) {
            case HOME_BTN_SINGLEPLAYER:
                return MAPSELECTION;
            case HOME_BTN_MULTIPLAYER:
                return GAMEPLAY;
            case HOME_BTN_SETTINGS:
                return SETTINGS;
            default:
                break;
        }
    }

    return HOME_PAGE;
}

void HomePage_OnVBlank(void) {
    move_homeKart();
}

//=============================================================================
// IMPLEMENTATION: MAIN (top)
//=============================================================================

static void configureGraphics_MAIN_home_page(void) {
    REG_DISPCNT = MODE_5_2D | DISPLAY_BG2_ACTIVE;
    VRAM_A_CR = VRAM_ENABLE | VRAM_A_MAIN_BG;
}

static void configBG_Main_homepage(void) {
    BGCTRL[2] = BG_BMP_BASE(0) | BgSize_B8_256x256;
    dmaCopy(home_topBitmap, BG_BMP_RAM(0), home_topBitmapLen);
    dmaCopy(home_topPal, BG_PALETTE, home_topPalLen);
    REG_BG2PA = 256;
    REG_BG2PC = 0;
    REG_BG2PB = 0;
    REG_BG2PD = 256;
}

static void configurekartSpritehome(void) {
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

static void move_homeKart(void) {
    oamSet(&oamMain, homeKart.id, homeKart.x, homeKart.y, 0, 0, SpriteSize_64x64,
           SpriteColorFormat_256Color, homeKart.gfx, -1, false, false, false, false,
           false);
    homeKart.x++;
    if (homeKart.x > 256)
        homeKart.x = -64;
    oamUpdate(&oamMain);
}

//=============================================================================
// IMPLEMENTATION: SUB (bottom)
//=============================================================================

static void configGraphics_Sub_HOME(void) {
    REG_DISPCNT_SUB = MODE_0_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE;
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
}

static void configBackground_Sub_HOME(void) {
    // BG0: Menu layer (front)
    BGCTRL_SUB[0] =
        BG_32x32 | BG_MAP_BASE(0) | BG_TILE_BASE(1) | BG_COLOR_256 | BG_PRIORITY(0);
    dmaCopy(ds_menuPal, BG_PALETTE_SUB, ds_menuPalLen);
    dmaCopy(ds_menuTiles, BG_TILE_RAM_SUB(1), ds_menuTilesLen);
    dmaCopy(ds_menuMap, BG_MAP_RAM_SUB(0), ds_menuMapLen);

    // BG1: Highlight layer (behind)
    BGCTRL_SUB[1] =
        BG_32x32 | BG_MAP_BASE(1) | BG_TILE_BASE(2) | BG_COLOR_256 | BG_PRIORITY(1);

    // Initialize highlight palette slots to black
    BG_PALETTE_SUB[HOME_HL_PAL_BASE + 0] = BLACK;
    BG_PALETTE_SUB[HOME_HL_PAL_BASE + 1] = BLACK;
    BG_PALETTE_SUB[HOME_HL_PAL_BASE + 2] = BLACK;

    memset((u8*)BG_TILE_RAM_SUB(2) + 0 * 64, 0, 64);

    // Load highlight tiles
    dmaCopy(selectionMaskTile0, (u8*)BG_TILE_RAM_SUB(2) + (1 * 64), 64);
    dmaCopy(selectionMaskTile1, (u8*)BG_TILE_RAM_SUB(2) + (2 * 64), 64);
    dmaCopy(selectionMaskTile2, (u8*)BG_TILE_RAM_SUB(2) + (3 * 64), 64);

    // Draw highlight rectangles for all buttons
    drawSelectionUnderlayRect(HOME_BTN_SINGLEPLAYER, 1);
    drawSelectionUnderlayRect(HOME_BTN_MULTIPLAYER, 2);
    drawSelectionUnderlayRect(HOME_BTN_SETTINGS, 3);
}

static void drawSelectionUnderlayRect(HomeButtonSelected btn, u16 tileIndex) {
    if (btn < 0 || btn >= HOME_BTN_COUNT) {
        return;
    }
    u16* map = BG_MAP_RAM_SUB(1);
    int startY = highlightTileY[btn];
    for (int row = 0; row < HIGHLIGHT_TILE_HEIGHT; row++) {
        for (int col = 0; col < HIGHLIGHT_TILE_WIDTH; col++) {
            map[(startY + row) * 32 + (HIGHLIGHT_TILE_X + col)] = tileIndex;
        }
    }
}

static void HomePage_setSelectionTint(int buttonIndex, bool show) {
    if (buttonIndex < 0 || buttonIndex >= MENU_COUNT) {
        return;
    }
    int paletteIndex = HOME_HL_PAL_BASE + buttonIndex;
    BG_PALETTE_SUB[paletteIndex] =
        show ? MENU_BUTTON_HIGHLIGHT_COLOR : MENU_HIGHLIGHT_OFF_COLOR;
}

//=============================================================================
// IMPLEMENTATION: INPUT
//=============================================================================
static void handleDPadInputHOME(void) {
    int keys = keysDown();
    if (keys & KEY_UP)
        selected = (selected - 1 + MENU_COUNT) % MENU_COUNT;
    if (keys & KEY_DOWN)
        selected = (selected + 1) % MENU_COUNT;
}

static void handleTouchInputHOME(void) {
    if (!(keysHeld() & KEY_TOUCH))
        return;
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
