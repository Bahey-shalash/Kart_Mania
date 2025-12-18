#include "graphics.h"

#include <nds.h>

#include "ds_menu.h"

void configureGraphics_Sub() {
    // Configure the SUB engine in mode 5 and activate background 2
    REG_DISPCNT_SUB = MODE_5_2D | DISPLAY_BG2_ACTIVE;

    // Configure the corresponding VRAM memory bank correctly
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
}

void configBG2_Sub() {
    // Configure background BG2 in extended rotoscale mode using 8bit pixels
    BGCTRL_SUB[2] = BG_MAP_BASE(0) | BgSize_B8_256x256;
    // Transfer image and palette to the corresponding memory locations.
    swiCopy(ds_menuBitmap, BG_BMP_RAM_SUB(0), ds_menuBitmapLen / 2);
    swiCopy(ds_menuPal, BG_PALETTE_SUB, ds_menuPalLen / 2);
    //! DMA COPY would have been faster :)
    // Set up affine matrix
    REG_BG2PA_SUB = 256;
    REG_BG2PC_SUB = 0;
    REG_BG2PB_SUB = 0;
    REG_BG2PD_SUB = 256;
}