#include "gameplay.h"

#include <nds.h>
#include <stdio.h>
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
    {scorching_sands_BRTiles, scorching_sands_BRMap, scorching_sands_BRTilesLen}};

//=============================================================================
// Private prototypes
//=============================================================================
static void configureGraphics(void);
static void configureBackground(void);
static void configureSprite(void);
static void configureConsole(void);
static void loadQuadrant(QuadrantID quad);
static QuadrantID determineQuadrant(int x, int y);

//=============================================================================
// Public API
//=============================================================================
void Graphical_Gameplay_initialize(void) {
    configureGraphics();
    configureBackground();
    configureSprite();
    configureConsole();

    // Start the race backend
    Map selectedMap = GameContext_GetMap();
    Race_Init(selectedMap, SinglePlayer);

    // Initial scroll position centered on player
    const Car* player = Race_GetPlayerCar();  // Now const
    scrollX = FixedToInt(player->position.x) - (SCREEN_WIDTH / 2);
    scrollY = FixedToInt(player->position.y) - (SCREEN_HEIGHT / 2);

    if (scrollX < 0)
        scrollX = 0;
    if (scrollY < 0)
        scrollY = 0;
    if (scrollX > MAX_SCROLL_X)
        scrollX = MAX_SCROLL_X;
    if (scrollY > MAX_SCROLL_Y)
        scrollY = MAX_SCROLL_Y;

    currentQuadrant = determineQuadrant(scrollX, scrollY);
    loadQuadrant(currentQuadrant);
}

GameState Gameplay_update(void) {
    // Only handle state transitions here
    scanKeys();
    int keys = keysDown();

    // Select to exit (placeholder for pause menu)
    if (keys & KEY_SELECT) {
        Race_Stop();  // Renamed from stop_Race
        return HOME_PAGE;
    }

    // Check if race finished
    const RaceState* state = Race_GetState();  // Now const
    if (state->raceFinished) {
        Race_Stop();       // Renamed from stop_Race
        return HOME_PAGE;  // TODO: results screen
    }

    return GAMEPLAY;
}

void Gameplay_OnVBlank(void) {
    const Car* player = Race_GetPlayerCar();  // Now const - read-only access

    int carX = FixedToInt(player->position.x);
    int carY = FixedToInt(player->position.y);

    scrollX = carX - (SCREEN_WIDTH / 2);
    scrollY = carY - (SCREEN_HEIGHT / 2);

    if (scrollX < 0)
        scrollX = 0;
    if (scrollY < 0)
        scrollY = 0;
    if (scrollX > MAX_SCROLL_X)
        scrollX = MAX_SCROLL_X;
    if (scrollY > MAX_SCROLL_Y)
        scrollY = MAX_SCROLL_Y;

    QuadrantID newQuadrant = determineQuadrant(scrollX, scrollY);
    if (newQuadrant != currentQuadrant) {
        loadQuadrant(newQuadrant);
        currentQuadrant = newQuadrant;
    }

    int col = currentQuadrant % 3;
    int row = currentQuadrant / 3;
    BG_OFFSET[0].x = scrollX - (col * QUAD_OFFSET);
    BG_OFFSET[0].y = scrollY - (row * QUAD_OFFSET);

    // DEBUG: Movement telemetry
    int facingAngle = player->angle512;
    int velocityAngle = Car_GetVelocityAngle(player);
    Q16_8 speed = Car_GetSpeed(player);
    bool moving = Car_IsMoving(player);
    Vec2 velocityVec = Vec2_Scale(Vec2_FromAngle(player->angle512), speed);

    // Clear and print debug info
    consoleClear();
    printf("=== CAR DEBUG ===\n");
    printf("Facing:   %3d\n", facingAngle);
    printf("Velocity: %3d\n", velocityAngle);
    printf("Speed:    %d.%02d\n", (int)(speed >> 8),
           (int)(((speed & 0xFF) * 100) >> 8));
    printf("Moving:   %s\n", moving ? "YES" : "NO");

    if (moving) {
        int angleDiff = (velocityAngle - facingAngle) & ANGLE_MASK;
        if (angleDiff > 256)
            angleDiff -= 512;  // Convert to -256..+256
        printf("Diff:     %+4d", angleDiff);
        if (angleDiff > 32 || angleDiff < -32) {
            iprintf(" <!>");  // Warning marker
        }
        printf("\n");
    }

    printf("\nPos: %d,%d\n", carX, carY);
    printf("Vel: %d,%d\n", FixedToInt(velocityVec.x), FixedToInt(velocityVec.y));

    // Your system: CW angles (0=right, 128=down, 256=left, 384=up)
    // DS system: CCW angles
    // Solution: negate
   int dsAngle = -(player->angle512 << 6);  // CW to CCW conversion

    oamRotateScale(&oamMain, 0, dsAngle, (1 << 8), (1 << 8));

    // sizeDouble=true means 32x32 sprite renders in 64x64 bounding box
    int screenX = carX - scrollX - 32;
    int screenY = carY - scrollY - 32;

    oamSet(&oamMain, 0, screenX, screenY, 0, 0, SpriteSize_32x32,
           SpriteColorFormat_256Color, playerKartGfx,
           0,             // affineIndex = 0
           true,          // sizeDouble
           false,         // hide
           false, false,  // hflip, vflip
           false);        // mosaic

    oamUpdate(&oamMain);
}

//=============================================================================
// Private functions
//=============================================================================
static void configureGraphics(void) {
    // Main screen: gameplay
    REG_DISPCNT = MODE_0_2D | DISPLAY_BG0_ACTIVE | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D;
    VRAM_A_CR = VRAM_ENABLE | VRAM_A_MAIN_BG;
    VRAM_B_CR = VRAM_ENABLE | VRAM_B_MAIN_SPRITE;

    // Sub screen: console
    REG_DISPCNT_SUB = MODE_0_2D | DISPLAY_BG0_ACTIVE;
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
}

static void configureBackground(void) {
    Map selectedMap = GameContext_GetMap();
    if (selectedMap != ScorchingSands)
        return;

    BGCTRL[0] = BG_64x64 | BG_COLOR_256 | BG_MAP_BASE(0) | BG_TILE_BASE(1);
    dmaCopy(scorching_sands_TLPal, BG_PALETTE, scorching_sands_TLPalLen);
}

static void configureSprite(void) {
    oamInit(&oamMain, SpriteMapping_1D_32, false);
    playerKartGfx =
        oamAllocateGfx(&oamMain, SpriteSize_32x32, SpriteColorFormat_256Color);

    // Load sprite graphics
    swiCopy(kart_spritePal, SPRITE_PALETTE, kart_spritePalLen / 2);
    swiCopy(kart_spriteTiles, playerKartGfx, kart_spriteTilesLen / 2);
}

static void configureConsole(void) {
    // Initialize console on sub (bottom) screen
    // layer=0, type=Text4bpp, size=256x256, mapBase=30, tileBase=0,
    // mainDisplay=false (use sub), loadGraphics=true
    consoleInit(NULL, 0, BgType_Text4bpp, BgSize_T_256x256, 30, 0, false, true);

    printf("\x1b[2J");  // Clear screen
    printf("=== KART DEBUG ===\n");
    printf("SELECT = exit\n\n");
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
