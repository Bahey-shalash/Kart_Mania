#include "home_page.h"

#include <nds.h>

#include "ds_menu.h"
#include "game.h"
#include "settings.h"

void HomePage_initialize() {
    configureGraphics_Sub_home_page();
    configBG2_Sub_homepage();
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

void touchscreen_controlls_home_page(touchPosition* touch) {
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