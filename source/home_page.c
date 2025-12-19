#include "home_page.h"

#include <stdio.h>

#include "ds_menu.h"
#include "game.h"
#include "game_types.h"
#include "home_top.h"
#include "kart_home.h"
#include "settings.h"

static HomeKartSprite homeKart;
static enum HomeButtonselected buttonSelected = NONE_button;

// Contour box dimensions in tiles
#define CONTOUR_X 4       // Starting tile column (32px)
#define CONTOUR_WIDTH 24  // Width in tiles (192px)
#define CONTOUR_HEIGHT 5  // Height in tiles (40px)

// Y positions for each button (tile rows)
#define SINGLE_PLAYER_Y 3
#define MULTIPLAYER_Y 10
#define SETTINGS_Y 16

//---------------buttom screeen -------------------------

void HomePage_initialize() {
    configureGraphics_Sub_home_page();
    configBG2_Sub_homepage();
    configureGraphics_MAIN_home_page();
    configBG0_Sub_contour();
    configBG_Main_homepage();
    configurekartSpritehome();
}

void configureGraphics_Sub_home_page() {
    // Configure the SUB engine in mode 5 and activate background 2
    REG_DISPCNT_SUB = MODE_5_2D | DISPLAY_BG2_ACTIVE | DISPLAY_BG0_ACTIVE;

    // Configure the corresponding VRAM memory bank correctly
    // VRAM_H for tiled BG
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
    VRAM_H_CR = VRAM_ENABLE | VRAM_H_SUB_BG;
}

void configBG2_Sub_homepage() {
    // Configure background BG2 in extended rotoscale mode using 8bit pixels
    // BMP_BASE(2) = offset 0x08000 (32KB offset)
    BGCTRL_SUB[2] = BG_BMP_BASE(2) | BgSize_B8_256x256 | BG_PRIORITY(1);
    

    // Transfer image and palette to the corresponding memory locations.
    dmaCopy(ds_menuBitmap, BG_BMP_RAM_SUB(2), ds_menuBitmapLen);
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

void button_controls_home_page(u16* keys) {
    enum HomeButtonselected previousSelection = buttonSelected;
    // Navigate down
    if (*keys & KEY_DOWN) {
        if (buttonSelected == NONE_button || buttonSelected == SETTINGS_button) {
            buttonSelected = SINGLE_PLAYER_button;
        } else if (buttonSelected == SINGLE_PLAYER_button) {
            buttonSelected = MULTIPLAYER_button;
        } else if (buttonSelected == MULTIPLAYER_button) {
            buttonSelected = SETTINGS_button;
        }
    }

    // Navigate up
    if (*keys & KEY_UP) {
        if (buttonSelected == NONE_button || buttonSelected == SINGLE_PLAYER_button) {
            buttonSelected = SETTINGS_button;
        } else if (buttonSelected == MULTIPLAYER_button) {
            buttonSelected = SINGLE_PLAYER_button;
        } else if (buttonSelected == SETTINGS_button) {
            buttonSelected = MULTIPLAYER_button;
        }
    }

    // Update overlay only if selection changed
    if (buttonSelected != previousSelection) {
        updateSelectionOverlay(buttonSelected);
    }

    // Select with A button
    if (*keys & KEY_A) {
        switch (buttonSelected) {
            case SINGLE_PLAYER_button:
                single_player_pressed();
                break;
            case MULTIPLAYER_button:
                multiplayer_pressed();
                break;
            case SETTINGS_button:
                settings_pressed();
                break;
            default:
                break;
        }
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

// Add to home_page.c

// Tile definitions (8x8, 8-bit indexed)
// Palette index 0 = transparent, index 1 = blue contour color

// Tile 0: Empty (transparent)
static const u8 tile_empty[64] = {0};

// Tile 1: Top-left corner
static const u8 tile_corner_tl[64] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
    1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1,
    0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0,
};

// Tile 2: Top edge (horizontal)
static const u8 tile_edge_top[64] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

// Tile 3: Top-right corner
static const u8 tile_corner_tr[64] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
    0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
    1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
};

// Tile 4: Left edge (vertical)
static const u8 tile_edge_left[64] = {
    0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
    0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1,
    0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0,
};

// Tile 5: Right edge (vertical)
static const u8 tile_edge_right[64] = {
    0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1,
    0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
    1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
};

// Tile 6: Bottom-left corner
static const u8 tile_corner_bl[64] = {
    0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
    0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1,
    1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

// Tile 7: Bottom edge (horizontal)
static const u8 tile_edge_bottom[64] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

// Tile 8: Bottom-right corner
static const u8 tile_corner_br[64] = {
    0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1,
    0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1,
    1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};



void configBG0_Sub_contour() {
    // Configure BG0 in tiled mode, 256 color, 32x32 tiles
    // Map base at slot 1, tile base at slot 0
    BGCTRL_SUB[0] =
        BG_32x32 | BG_COLOR_256 | BG_MAP_BASE(1) | BG_TILE_BASE(0) | BG_PRIORITY(0);

    // Copy tiles to VRAM (tile base 0 = 0x06200000 for sub)
    // Each tile is 64 bytes in 256-color mode
    dmaCopy(tile_empty, &BG_TILE_RAM_SUB(0)[0 * 32], 64);
    dmaCopy(tile_corner_tl, &BG_TILE_RAM_SUB(0)[1 * 32], 64);
    dmaCopy(tile_edge_top, &BG_TILE_RAM_SUB(0)[2 * 32], 64);
    dmaCopy(tile_corner_tr, &BG_TILE_RAM_SUB(0)[3 * 32], 64);
    dmaCopy(tile_edge_left, &BG_TILE_RAM_SUB(0)[4 * 32], 64);
    dmaCopy(tile_edge_right, &BG_TILE_RAM_SUB(0)[5 * 32], 64);
    dmaCopy(tile_corner_bl, &BG_TILE_RAM_SUB(0)[6 * 32], 64);
    dmaCopy(tile_edge_bottom, &BG_TILE_RAM_SUB(0)[7 * 32], 64);
    dmaCopy(tile_corner_br, &BG_TILE_RAM_SUB(0)[8 * 32], 64);

    // Set palette index 1 to blue
    BG_PALETTE_SUB[1] = RGB15(0, 10, 31);

    // Clear tilemap to all empty tiles
    for (int i = 0; i < 32 * 32; i++) {
        BG_MAP_RAM_SUB(1)[i] = 0;
    }
}

void updateSelectionOverlay(enum HomeButtonselected selection) {
    int y;

    // Clear tilemap first
    for (int i = 0; i < 32 * 32; i++) {
        BG_MAP_RAM_SUB(1)[i] = 0;
    }

    // If nothing selected, leave it empty
    if (selection == NONE_button) {
        return;
    }

    // Determine Y position based on selection
    switch (selection) {
        case SINGLE_PLAYER_button:
            y = SINGLE_PLAYER_Y;
            break;
        case MULTIPLAYER_button:
            y = MULTIPLAYER_Y;
            break;
        case SETTINGS_button:
            y = SETTINGS_Y;
            break;
        default:
            return;
    }

    // Draw contour rectangle
    // Top row
    BG_MAP_RAM_SUB(1)[y * 32 + CONTOUR_X] = 1;  // Top-left corner
    for (int x = 1; x < CONTOUR_WIDTH - 1; x++) {
        BG_MAP_RAM_SUB(1)[y * 32 + CONTOUR_X + x] = 2;  // Top edge
    }
    BG_MAP_RAM_SUB(1)[y * 32 + CONTOUR_X + CONTOUR_WIDTH - 1] = 3;  // Top-right corner

    // Middle rows (left and right edges)
    for (int row = 1; row < CONTOUR_HEIGHT - 1; row++) {
        BG_MAP_RAM_SUB(1)[(y + row) * 32 + CONTOUR_X] = 4;  // Left edge
        BG_MAP_RAM_SUB(1)
        [(y + row) * 32 + CONTOUR_X + CONTOUR_WIDTH - 1] = 5;  // Right edge
    }

    // Bottom row
    BG_MAP_RAM_SUB(1)
    [(y + CONTOUR_HEIGHT - 1) * 32 + CONTOUR_X] = 6;  // Bottom-left corner
    for (int x = 1; x < CONTOUR_WIDTH - 1; x++) {
        BG_MAP_RAM_SUB(1)
        [(y + CONTOUR_HEIGHT - 1) * 32 + CONTOUR_X + x] = 7;  // Bottom edge
    }
    BG_MAP_RAM_SUB(1)
    [(y + CONTOUR_HEIGHT - 1) * 32 + CONTOUR_X + CONTOUR_WIDTH - 1] =
        8;  // Bottom-right corner
}