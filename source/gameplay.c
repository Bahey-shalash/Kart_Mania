#include "map_selection.h"

#include <nds.h>
#include <string.h>

//=============================================================================
// Private function prototypes
//=============================================================================
static void configureGraphics_MAIN_GAMEPLAY(void);
static void configBG_Main_GAMEPLAY(void);

//=============================================================================
// Public API implementation
//=============================================================================

void Gameplay_initialize(void) {
    configureGraphics_MAIN_GAMEPLAY();
    configBG_Main_GAMEPLAY();
}

//=============================================================================
// GRAPHICS SETUP
//=============================================================================
static void configureGraphics_MAIN_GAMEPLAY(void) {
    REG_DISPCNT = MODE_0_2D | DISPLAY_BG0_ACTIVE;
    VRAM_A_CR = VRAM_ENABLE | VRAM_A_MAIN_BG;
}

static void configBG_Main_GAMEPLAY(void) {
    BGCTRL[0] = BG_64x64 | BG_COLOR_256 | BG_MAP_BASE(0) | BG_TILE_BASE(1);
}
