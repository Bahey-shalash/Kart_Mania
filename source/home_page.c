#include "home_page.h"

#include <stdio.h>

#include "ds_menu.h"
#include "game.h"
#include "game_types.h"
#include "home_top.h"
#include "kart_home.h"
#include "settings.h"

// Left edge of button highlight (rounded left side with dark border)
u8 highlightLeftTile[64] = {0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0,
                            1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
                            1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
                            0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1};

// Middle section (repeatable, just top and bottom borders)
u8 highlightMiddleTile[64] = {1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1};

// Right edge of button highlight (rounded right side with dark border)
u8 highlightRightTile[64] = {1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0,
                             0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1,
                             0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1,
                             0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0};
static HomeKartSprite homeKart;
u16* highlightGfx;


#define MENU_COUNT 3

MenuItem menu[MENU_COUNT] = {
    {32, 24, 192, 40},   // Single Player
    {32, 78, 192, 40},   // Multiplayer
    {32, 132, 192, 40},  // Settings
};

int selectedButton = 0;
int lastSelectedButton = -1;
bool pressed = false;

//----------Initialization & Cleanup----------
void HomePage_initialize() {
    configGraphics_Sub();
    configBackground_Sub();
    configureGraphics_MAIN_home_page();
    configBG_Main_homepage();
    configurekartSpritehome();
}

void HomePage_cleanup(void) {
    // Disable sub BGs
    REG_DISPCNT_SUB &= ~(DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE);

    // Clear BG maps (important!)
    memset(BG_MAP_RAM_SUB(0), 0, 32 * 32 * sizeof(u16));
    memset(BG_MAP_RAM_SUB(2), 0, 32 * 32 * sizeof(u16));

    // Optional: reset selection
    selectedButton = 0;
    lastSelectedButton = -1;
}

//----------Configuration Functions (Main Engine)----------

void configureGraphics_MAIN_home_page() {
    // Configure the MAIN engine in mode 5 and activate background 2
    REG_DISPCNT = MODE_5_2D | DISPLAY_BG2_ACTIVE;

    // Configure VRAM memory bank A for MAIN
    VRAM_A_CR = VRAM_ENABLE | VRAM_A_MAIN_BG;
}

void configBG_Main_homepage() {
    BGCTRL[2] = BG_BMP_BASE(0) | BgSize_B8_256x256;

    // Transfer image and palette to the corresponding memory locations.
    dmaCopy(home_topBitmap, BG_BMP_RAM(0), home_topBitmapLen);
    dmaCopy(home_topPal, BG_PALETTE, home_topPalLen);

    // Set up affine matrix
    REG_BG2PA = 256;
    REG_BG2PC = 0;
    REG_BG2PB = 0;
    REG_BG2PD = 256;
}

void configurekartSpritehome() {
    VRAM_B_CR = VRAM_ENABLE | VRAM_B_MAIN_SPRITE;

    // Initialize sprite manager and the engine
    oamInit(&oamMain, SpriteMapping_1D_32, false);
    homeKart.id = 0;
    homeKart.x = -64;
    homeKart.y = 120;

    // Allocate space for the graphic to show in the sprite
    homeKart.gfx =
        oamAllocateGfx(&oamMain, SpriteSize_64x64, SpriteColorFormat_256Color);

    // Copy data for the graphic (palette and bitmap)
    swiCopy(kart_homePal, SPRITE_PALETTE, kart_homePalLen / 2);
    swiCopy(kart_homeTiles, homeKart.gfx, kart_homeTilesLen / 2);
}

void move_homeKart() {
    // Update sprite attributes
    oamSet(&oamMain,                    // Main OAM
           homeKart.id,                 // Sprite number
           homeKart.x,                  // X position
           homeKart.y,                  // Y position
           0,                           // Priority
           0,                           // Palette to use
           SpriteSize_64x64,            // Size of the sprite
           SpriteColorFormat_256Color,  // Color format
           homeKart.gfx,                // Pointer to the graphics
           -1,                          // Affine rotation/scaling index (-1 = none)
           false,                       // Double size if rotating
           false,                       // Hide this sprite
           false, false,                // Horizontal or vertical flip
           false                        // Mosaic
    );

    homeKart.x++;  // Move right
    if (homeKart.x > 256) {
        homeKart.x = -64;  // Reset position when it goes off-screen
    }

    // Update OAM to apply changes
    oamUpdate(&oamMain);
}

//----------Configuration Functions (Sub Engine)----------

void configGraphics_Sub(void) {
    REG_DISPCNT_SUB = MODE_5_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE;
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
}

void configBackground_Sub(void) {
    BGCTRL_SUB[0] = BG_32x32 | BG_MAP_BASE(0) | BG_TILE_BASE(1) | BG_COLOR_256;
    swiCopy(ds_menuPal, BG_PALETTE_SUB, ds_menuPalLen / 2);
    swiCopy(ds_menuTiles, BG_TILE_RAM_SUB(1), ds_menuTilesLen / 2);
    dmaCopy(ds_menuMap, BG_MAP_RAM_SUB(0), ds_menuMapLen);

    BGCTRL_SUB[1] = BG_32x32 | BG_MAP_BASE(2) | BG_TILE_BASE(2) | BG_COLOR_256;
    // Set palette color for border
    BG_PALETTE_SUB[1] = RGB15(10, 10, 10);

    // Copy highlight tiles to VRAM (tiles 1, 2, 3)
    dmaCopy(highlightLeftTile, BG_TILE_RAM_SUB(2) + (1 * 64), 64);
    dmaCopy(highlightMiddleTile, BG_TILE_RAM_SUB(2) + (2 * 64), 64);
    dmaCopy(highlightRightTile, BG_TILE_RAM_SUB(2) + (3 * 64), 64);

    // Show initial highlight on button 0
    setButtonOverlay(0, true);
}

void setButtonOverlay(int buttonIndex, bool show) {
    u16* overlayMap = (u16*)BG_MAP_RAM_SUB(2);
    MenuItem* m = &menu[buttonIndex];

    int tilesWide = m->w / 8;  // 192/8 = 24 tiles
    int tilesHigh = m->h / 8;  // 40/8 = 5 tiles
    int mapX = m->x / 8;       // 32/8 = 4
    int mapY = m->y / 8;

    for (int ty = 0; ty < tilesHigh; ty++) {
        for (int tx = 0; tx < tilesWide; tx++) {
            int mapIndex = (mapY + ty) * 32 + (mapX + tx);
            u16 tileValue = 0;

            if (show) {
                // Left edge
                if (tx == 0)
                    tileValue = 1;
                // Right edge
                else if (tx == tilesWide - 1)
                    tileValue = 3;
                // Middle
                else
                    tileValue = 2;
            }

            overlayMap[mapIndex] = tileValue;
        }
    }
}

void handleDPadInput(void) {
    int keys = keysDown();

    if (keys & KEY_UP) {
        selectedButton = (selectedButton - 1 + MENU_COUNT) % MENU_COUNT;
    }

    if (keys & KEY_DOWN) {
        selectedButton = (selectedButton + 1) % MENU_COUNT;
    }

    if (keys & KEY_A) {
        pressed = true;  // visual feedback only
    }
}

void handleTouchInput(void) {
    touchPosition touch;
    touchRead(&touch);

    if (!(keysHeld() & KEY_TOUCH))
        return;

    for (int i = 0; i < MENU_COUNT; i++) {
        MenuItem* m = &menu[i];

        if (touch.px >= m->x && touch.px < m->x + m->w && touch.py >= m->y &&
            touch.py < m->y + m->h) {
            selectedButton = i;
            pressed = true;
            return;
        }
    }
}

enum GameState HomePage_update(void) {
    scanKeys();
    pressed = false;

    handleDPadInput();
    handleTouchInput();

    // Update highlight
    if (selectedButton != lastSelectedButton) {
        if (lastSelectedButton != -1)
            setButtonOverlay(lastSelectedButton, false);

        setButtonOverlay(selectedButton, true);
        lastSelectedButton = selectedButton;
    }

    // ACTIVATE on release (A or touch)
    if (keysUp() & KEY_A || keysUp() & KEY_TOUCH) {
        switch (selectedButton) {
            case 0:
                return SINGLEPLAYER;
            case 1:
                return MULTIPLAYER;
            case 2:
                return SETTINGS;
        }
    }

    return HOME_PAGE;
}
