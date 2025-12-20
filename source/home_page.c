#include "home_page.h"

#include "ds_menu.h"
#include "home_top.h"
#include "kart_home.h"

// Solid tiles - each uses its own palette index (251, 252, 253)
static u8 selectionMaskTile0[64] = {
    251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251,
    251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251,
    251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251,
    251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251,
};
static u8 selectionMaskTile1[64] = {
    252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252,
    252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252,
    252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252,
    252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252, 252,
};
static u8 selectionMaskTile2[64] = {
    253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253,
    253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253,
    253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253,
    253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253,
};

static const int highlightTileY[MENU_COUNT] = {4, 10, 17};

static HomeKartSprite homeKart;

static const MenuItemHitBox homeBtnHitbox[MENU_COUNT] = {
    [HOME_BTN_SINGLEPLAYER] = MENU_ITEM_ROW(0),
    [HOME_BTN_MULTIPLAYER] = MENU_ITEM_ROW(1),
    [HOME_BTN_SETTINGS] = MENU_ITEM_ROW(2),
};

static HomeButtonSelected selected = HOME_BTN_NONE;
static HomeButtonSelected lastSelected = HOME_BTN_NONE;

static void drawSelectionUnderlayRect(int buttonIndex, u16 tileIndex) {
    u16* map = BG_MAP_RAM_SUB(1);  // was 2 changed to 1 for testing
    int startY = highlightTileY[buttonIndex];
    for (int row = 0; row < HIGHLIGHT_TILE_HEIGHT; row++) {
        for (int col = 0; col < HIGHLIGHT_TILE_WIDTH; col++) {
            map[(startY + row) * 32 + (HIGHLIGHT_TILE_X + col)] = tileIndex;
        }
    }
}

void HomePage_setSelectionTint(int buttonIndex, bool show) {
    if (buttonIndex < 0 || buttonIndex >= MENU_COUNT) {
        return;
    }
    int paletteIndex = 251 + buttonIndex;
    BG_PALETTE_SUB[paletteIndex] =
        show ? MENU_BUTTON_HIGHLIGHT_COLOR : MENU_HIGHLIGHT_OFF_COLOR;
}

//----------Initialization & Cleanup----------

void HomePage_initialize(void) {
    configGraphics_Sub();
    configBackground_Sub();
    configureGraphics_MAIN_home_page();
    configBG_Main_homepage();
    configurekartSpritehome();
}

void HomePage_cleanup(void) {
    REG_DISPCNT_SUB &= ~(DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE);
    memset(BG_MAP_RAM_SUB(0), 0, 32 * 32 * sizeof(u16));
    memset(BG_MAP_RAM_SUB(1), 0,
           32 * 32 * sizeof(u16));  // was 2 changed to 1 for testing
    selected = HOME_BTN_NONE;
    lastSelected = HOME_BTN_NONE;
}

//----------Configuration Functions (Main Engine)----------

void configureGraphics_MAIN_home_page(void) {
    REG_DISPCNT = MODE_5_2D | DISPLAY_BG2_ACTIVE;
    VRAM_A_CR = VRAM_ENABLE | VRAM_A_MAIN_BG;
}

void configBG_Main_homepage(void) {
    BGCTRL[2] = BG_BMP_BASE(0) | BgSize_B8_256x256;
    dmaCopy(home_topBitmap, BG_BMP_RAM(0), home_topBitmapLen);
    dmaCopy(home_topPal, BG_PALETTE, home_topPalLen);
    REG_BG2PA = 256;
    REG_BG2PC = 0;
    REG_BG2PB = 0;
    REG_BG2PD = 256;
}

void configurekartSpritehome(void) {
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

void move_homeKart(void) {
    oamSet(&oamMain, homeKart.id, homeKart.x, homeKart.y, 0, 0, SpriteSize_64x64,
           SpriteColorFormat_256Color, homeKart.gfx, -1, false, false, false, false,
           false);
    homeKart.x++;
    if (homeKart.x > 256)
        homeKart.x = -64;
    oamUpdate(&oamMain);
}

//----------Configuration Functions (Sub Engine)----------

void configGraphics_Sub(void) {
    REG_DISPCNT_SUB = MODE_0_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE;
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
}

void configBackground_Sub(void) {
    // BG0: Menu (in front)
    BGCTRL_SUB[0] =
        BG_32x32 | BG_MAP_BASE(0) | BG_TILE_BASE(1) | BG_COLOR_256 | BG_PRIORITY(0);
    dmaCopy(ds_menuPal, BG_PALETTE_SUB, ds_menuPalLen);
    dmaCopy(ds_menuTiles, BG_TILE_RAM_SUB(1), ds_menuTilesLen);
    dmaCopy(ds_menuMap, BG_MAP_RAM_SUB(0), ds_menuMapLen);

    // BG1: Highlight layer (behind)
    BGCTRL_SUB[1] =
        BG_32x32 | BG_MAP_BASE(1) | BG_TILE_BASE(2) | BG_COLOR_256 | BG_PRIORITY(1);
    // test to change tile base from 4 to 2
    // test to change Map base from 2 to 1

    // Initial colors: all black
    BG_PALETTE_SUB[251] = BLACK;
    BG_PALETTE_SUB[252] = BLACK;
    BG_PALETTE_SUB[253] = BLACK;

    memset(BG_MAP_RAM_SUB(1), 0, 32 * 32 * sizeof(u16));

    // Load tiles
    dmaCopy(selectionMaskTile0, (u8*)BG_TILE_RAM_SUB(2) + (1 * 64), 64);
    dmaCopy(selectionMaskTile1, (u8*)BG_TILE_RAM_SUB(2) + (2 * 64), 64);
    dmaCopy(selectionMaskTile2, (u8*)BG_TILE_RAM_SUB(2) + (3 * 64), 64);

    // Draw all button backgrounds
    drawSelectionUnderlayRect(0, 1);
    drawSelectionUnderlayRect(1, 2);
    drawSelectionUnderlayRect(2, 3);
}

//----------Input Handling----------

void handleDPadInputHOME(void) {
    int keys = keysDown();
    if (keys & KEY_UP)
        selected = (selected - 1 + MENU_COUNT) % MENU_COUNT;
    if (keys & KEY_DOWN)
        selected = (selected + 1) % MENU_COUNT;
}

void handleTouchInputHOME(void) {
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

enum GameState HomePage_update(void) {
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
        switch (selected) {
            case HOME_BTN_SINGLEPLAYER:
                return SINGLEPLAYER;
            case HOME_BTN_MULTIPLAYER:
                return MULTIPLAYER;
            case HOME_BTN_SETTINGS:
                return SETTINGS;
            default:
                break;
        }
    }

    return HOME_PAGE;
}