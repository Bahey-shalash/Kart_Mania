#include "home_page.h"

#include <stdio.h>

#include "ds_menu.h"
#include "game.h"
#include "home_top.h"
#include "kart_home.h"
#include "settings.h"

//---------------buttom screeen -------------------------

void HomePage_initialize() {
    configureGraphics_Sub_home_page();
    configBG2_Sub_homepage();
    configureGraphics_MAIN_home_page();
    configBG_Main_homepage();
    configurekartSpritehome();
}

void configureGraphics_Sub_home_page() {
    // Configure the SUB engine in mode 5 and activate background 2
    REG_DISPCNT_SUB = MODE_5_2D | DISPLAY_BG2_ACTIVE;

    // Configure the corresponding VRAM memory bank correctly
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
}

void configBG2_Sub_homepage() {
    // Configure background BG2 in extended rotoscale mode using 8bit pixels
    BGCTRL_SUB[2] = BG_BMP_BASE(0) | BgSize_B8_256x256;

    // Transfer image and palette to the corresponding memory locations.
    dmaCopy(ds_menuBitmap, BG_BMP_RAM_SUB(0), ds_menuBitmapLen);
    dmaCopy(ds_menuPal, BG_PALETTE_SUB, ds_menuPalLen);

    // Set up affine matrix
    REG_BG2PA_SUB = 256;
    REG_BG2PC_SUB = 0;
    REG_BG2PB_SUB = 0;
    REG_BG2PD_SUB = 256;
}

void touchscreen_controls_home_page(touchPosition* touch) {
    // Single Player button (y: 23-61)
    if (touch->px >= 33 && touch->px <= 223 && touch->py >= 23 && touch->py <= 61) {
        single_player_pressed();
    }
    // Multiplayer button (y: 77-115)
    else if (touch->px >= 33 && touch->px <= 223 && touch->py >= 77 &&
             touch->py <= 115) {
        multiplayer_pressed();
    }
    // Settings button (y: 131-169)
    else if (touch->px >= 33 && touch->px <= 223 && touch->py >= 131 &&
             touch->py <= 169) {
        settings_pressed();
    }
}
//---------------top screeen -------------------------

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
u16* gfx;

void configurekartSpritehome() {
    VRAM_B_CR = VRAM_ENABLE | VRAM_B_MAIN_SPRITE;

    // Initialize sprite manager and the engine
    oamInit(&oamMain, SpriteMapping_1D_32, false);

    // Allocate space for the graphic to show in the sprite
    gfx = oamAllocateGfx(&oamMain, SpriteSize_64x64, SpriteColorFormat_256Color);

    // Copy data for the graphic (palette and bitmap)
    swiCopy(kart_homePal, SPRITE_PALETTE, kart_homePalLen / 2);
    swiCopy(kart_homeTiles, gfx, kart_homeTilesLen / 2);
}

void move_homeKart() {
    

    static int x=-64;
    // Update sprite attributes
    oamSet(&oamMain,                    // Main OAM
           0,                           // Sprite number
           x,                           // X position
           120,                         // Y position
           0,                           // Priority
           0,                           // Palette to use
           SpriteSize_64x64,            // Size of the sprite
           SpriteColorFormat_256Color,  // Color format
           gfx,                         // Pointer to the graphics
           -1,                          // Affine rotation/scaling index (-1 = none)
           false,                       // Double size if rotating
           false,                       // Hide this sprite
           false, false,                // Horizontal or vertical flip
           false                        // Mosaic
    );

    x += 1;
    if (x > 256) {
        x = -64;  // Reset position when it goes off-screen
    }

    // Update OAM to apply changes
    oamUpdate(&oamMain);
}
