#include "gameplay.h"

#include <nds.h>
#include <string.h>

#include "context.h"
#include "game_types.h"
#include "map_selection.h"
#include "scorching_sands_BC.h"
#include "scorching_sands_BL.h"
#include "scorching_sands_BR.h"
#include "scorching_sands_MC.h"
#include "scorching_sands_ML.h"
#include "scorching_sands_MR.h"
#include "scorching_sands_TC.h"
#include "scorching_sands_TL.h"
#include "scorching_sands_TR.h"

//=============================================================================
// Private variables
//=============================================================================
static int scrollX = 0;
static int scrollY = 0;
static QuadrantID currentQuadrant = QUAD_TL;

//=============================================================================
// Quadrant data structure
//=============================================================================
typedef struct {
    const unsigned int* tiles;
    const unsigned short* map;
    unsigned int tilesLen;
} QuadrantData;

static const QuadrantData quadrantData[9] = {
    {scorching_sands_TLTiles, scorching_sands_TLMap, scorching_sands_TLTilesLen},
    {scorching_sands_TCTiles, scorching_sands_TCMap, scorching_sands_TCTilesLen},
    {scorching_sands_TRTiles, scorching_sands_TRMap, scorching_sands_TRTilesLen},
    {scorching_sands_MLTiles, scorching_sands_MLMap, scorching_sands_MLTilesLen},
    {scorching_sands_MCTiles, scorching_sands_MCMap, scorching_sands_MCTilesLen},
    {scorching_sands_MRTiles, scorching_sands_MRMap, scorching_sands_MRTilesLen},
    {scorching_sands_BLTiles, scorching_sands_BLMap, scorching_sands_BLTilesLen},
    {scorching_sands_BCTiles, scorching_sands_BCMap, scorching_sands_BCTilesLen},
    {scorching_sands_BRTiles, scorching_sands_BRMap, scorching_sands_BRTilesLen}};

//=============================================================================
// Private function prototypes
//=============================================================================
static void configureGraphics_MAIN_GAMEPLAY(void);
static void configBG_Main_GAMEPLAY(void);
static void updateScroll(void);
static void loadQuadrant(QuadrantID quad);
static QuadrantID determineQuadrant(int x, int y);
static void clampScroll(int* scroll, int maxScroll);
static void configurekartSprites(void);

//=============================================================================
// Public API implementation
//=============================================================================
void Graphical_Gameplay_initialize(void) {
    configureGraphics_MAIN_GAMEPLAY();
    configBG_Main_GAMEPLAY();
    scrollX = scrollY = 0;
    currentQuadrant = QUAD_TL;
}

GameState Gameplay_update(void) { // will remove later 
    scanKeys();
    int keys = keysHeld();

    // Update scroll position based on input
    if (keys & KEY_LEFT)
        scrollX -= SCROLL_SPEED;
    if (keys & KEY_RIGHT)
        scrollX += SCROLL_SPEED;
    if (keys & KEY_UP)
        scrollY -= SCROLL_SPEED;
    if (keys & KEY_DOWN)
        scrollY += SCROLL_SPEED;

    // Clamp scroll to valid range
    clampScroll(&scrollX, MAX_SCROLL_X);
    clampScroll(&scrollY, MAX_SCROLL_Y);

    updateScroll();

    return GAMEPLAY;
}

//=============================================================================
// Private functions
//=============================================================================
static void configureGraphics_MAIN_GAMEPLAY(void) {
    REG_DISPCNT = MODE_0_2D | DISPLAY_BG0_ACTIVE;
    VRAM_A_CR = VRAM_ENABLE | VRAM_A_MAIN_BG;
}

static void configBG_Main_GAMEPLAY(void) {
    Map selectedMap = GameContext_GetMap();

    if (selectedMap != ScorchingSands)
        return;

    BGCTRL[0] = BG_64x64 | BG_COLOR_256 | BG_MAP_BASE(0) | BG_TILE_BASE(1);
    dmaCopy(scorching_sands_TLPal, BG_PALETTE, scorching_sands_TLPalLen);
    loadQuadrant(QUAD_TL);
}

static void loadQuadrant(QuadrantID quad) {
    const QuadrantData* data = &quadrantData[quad];

    // Load tiles
    dmaCopy(data->tiles, BG_TILE_RAM(1), data->tilesLen);

    // Load map (64x64 tile map split into 4 32x32 map bases)
    for (int i = 0; i < 32; i++) {
        dmaCopy(&data->map[i * 64], &BG_MAP_RAM(0)[i * 32], 64);
        dmaCopy(&data->map[i * 64 + 32], &BG_MAP_RAM(1)[i * 32], 64);
        dmaCopy(&data->map[(i + 32) * 64], &BG_MAP_RAM(2)[i * 32], 64);
        dmaCopy(&data->map[(i + 32) * 64 + 32], &BG_MAP_RAM(3)[i * 32], 64);
    }
}

static QuadrantID determineQuadrant(int x, int y) {
    int col = (x < QUAD_OFFSET) ? 0 : (x < 2 * QUAD_OFFSET) ? 1 : 2;
    int row = (y < QUAD_OFFSET) ? 0 : (y < 2 * QUAD_OFFSET) ? 1 : 2;
    return (QuadrantID)(row * 3 + col);
}

static void updateScroll(void) {
    QuadrantID newQuadrant = determineQuadrant(scrollX, scrollY);

    if (newQuadrant != currentQuadrant) {
        loadQuadrant(newQuadrant);
        currentQuadrant = newQuadrant;
    }

    // Calculate local offset within current quadrant
    int col = currentQuadrant % 3;
    int row = currentQuadrant / 3;

    BG_OFFSET[0].x = scrollX - (col * QUAD_OFFSET);
    BG_OFFSET[0].y = scrollY - (row * QUAD_OFFSET);
}

static void clampScroll(int* scroll, int maxScroll) {
    if (*scroll < 0)
        *scroll = 0;
    if (*scroll > maxScroll)
        *scroll = maxScroll;
}

static void configurekartSprites(void) {
    VRAM_B_CR = VRAM_ENABLE | VRAM_B_MAIN_SPRITE;
    // todo: complete it
}