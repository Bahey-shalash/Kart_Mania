#include "gameplay.h"

#include <nds.h>
#include <string.h>

#include "context.h"
#include "game_types.h"
#include "gameplay_logic.h"
#include "kart_sprite.h"
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
// Private state
//=============================================================================
static int scrollX = 0;
static int scrollY = 0;
static QuadrantID currentQuadrant = QUAD_TL;

static u16* playerKartGfx = NULL;

//=============================================================================
// Quadrant data
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
    {scorching_sands_BRTiles, scorching_sands_BRMap, scorching_sands_BRTilesLen}
};

//=============================================================================
// Private prototypes
//=============================================================================
static void configureGraphics(void);
static void configureBackground(void);
static void configureSprite(void);
static void loadQuadrant(QuadrantID quad);
static QuadrantID determineQuadrant(int x, int y);

//=============================================================================
// Public API
//=============================================================================
void Graphical_Gameplay_initialize(void) {
    configureGraphics();
    configureBackground();
    configureSprite();

    // Start the race backend
    Map selectedMap = GameContext_GetMap();
    Race_Init(selectedMap, SinglePlayer);

    // Initial scroll position centered on player
    Car* player = Race_GetPlayerCar();
    scrollX = FixedToInt(player->position.x) - (SCREEN_WIDTH / 2);
    scrollY = FixedToInt(player->position.y) - (SCREEN_HEIGHT / 2);

    if (scrollX < 0) scrollX = 0;
    if (scrollY < 0) scrollY = 0;
    if (scrollX > MAX_SCROLL_X) scrollX = MAX_SCROLL_X;
    if (scrollY > MAX_SCROLL_Y) scrollY = MAX_SCROLL_Y;

    currentQuadrant = determineQuadrant(scrollX, scrollY);
    loadQuadrant(currentQuadrant);
}

GameState Gameplay_update(void) {
    // Only handle state transitions here
    scanKeys();
    int keys = keysDown();

    //!START to exit (placeholder for pause menu)
    if (keys & KEY_START) {
        stop_Race();
        return HOME_PAGE;
        //TODO
    }

    // Check if race finished
    RaceState* state = Race_GetState();
    if (state->raceFinished) {
        stop_Race();
        return HOME_PAGE;  // TODO: results screen
    }

    return GAMEPLAY;
}

void Gameplay_OnVBlank(void) {
    Car* player = Race_GetPlayerCar();

    // Convert Q16.8 to pixels
    int carX = FixedToInt(player->position.x);
    int carY = FixedToInt(player->position.y);

    // Center camera on car
    scrollX = carX - (SCREEN_WIDTH / 2);
    scrollY = carY - (SCREEN_HEIGHT / 2);

    // Clamp scroll
    if (scrollX < 0) scrollX = 0;
    if (scrollY < 0) scrollY = 0;
    if (scrollX > MAX_SCROLL_X) scrollX = MAX_SCROLL_X;
    if (scrollY > MAX_SCROLL_Y) scrollY = MAX_SCROLL_Y;

    // Update quadrant if needed
    QuadrantID newQuadrant = determineQuadrant(scrollX, scrollY);
    if (newQuadrant != currentQuadrant) {
        loadQuadrant(newQuadrant);
        currentQuadrant = newQuadrant;
    }

    // Set BG scroll offset
    int col = currentQuadrant % 3;
    int row = currentQuadrant / 3;
    BG_OFFSET[0].x = scrollX - (col * QUAD_OFFSET);
    BG_OFFSET[0].y = scrollY - (row * QUAD_OFFSET);

    // Update sprite position (car position relative to screen)
    int screenX = carX - scrollX - 16;  // -16 to center 32x32 sprite
    int screenY = carY - scrollY - 16;

    oamSet(&oamMain, 0, screenX, screenY, 0, 0,
           SpriteSize_32x32, SpriteColorFormat_256Color,
           playerKartGfx, -1, false, false, false, false, false);

    oamUpdate(&oamMain);
}

//=============================================================================
// Private functions
//=============================================================================
static void configureGraphics(void) {
    REG_DISPCNT = MODE_0_2D | DISPLAY_BG0_ACTIVE | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D;
    VRAM_A_CR = VRAM_ENABLE | VRAM_A_MAIN_BG;
    VRAM_B_CR = VRAM_ENABLE | VRAM_B_MAIN_SPRITE;
}

static void configureBackground(void) {
    Map selectedMap = GameContext_GetMap();
    if (selectedMap != ScorchingSands) return;

    BGCTRL[0] = BG_64x64 | BG_COLOR_256 | BG_MAP_BASE(0) | BG_TILE_BASE(1);
    dmaCopy(scorching_sands_TLPal, BG_PALETTE, scorching_sands_TLPalLen);
}

static void configureSprite(void) {
    oamInit(&oamMain, SpriteMapping_1D_32, false);
    playerKartGfx = oamAllocateGfx(&oamMain, SpriteSize_32x32, SpriteColorFormat_256Color);

    // Load sprite graphics
    swiCopy(kart_spritePal, SPRITE_PALETTE, kart_spritePalLen / 2);
    swiCopy(kart_spriteTiles, playerKartGfx, kart_spriteTilesLen / 2);
}

static void loadQuadrant(QuadrantID quad) {
    const QuadrantData* data = &quadrantData[quad];

    dmaCopy(data->tiles, BG_TILE_RAM(1), data->tilesLen);

    // 64x64 tile map -> 4 screen blocks
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