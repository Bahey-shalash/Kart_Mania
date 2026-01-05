/**
 * gameplay.c
 * Main gameplay screen with race logic, rendering, and UI
 */

#include "../gameplay/gameplay.h"

#include <nds.h>
#include <stdio.h>
#include <string.h>

#include "../core/context.h"
#include "../core/game_constants.h"
#include "../core/game_types.h"
#include "../gameplay/gameplay_logic.h"
#include "../gameplay/items/Items.h"
#include "../gameplay/wall_collision.h"
#include "../network/multiplayer.h"
#include "../storage/storage_pb.h"
#include "banana.h"
#include "bomb.h"
#include "green_shell.h"
#include "kart_sprite.h"
#include "missile.h"
#include "numbers.h"
#include "oil_slick.h"
#include "red_shell.h"
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
// Private Types
//=============================================================================

typedef struct {
    const unsigned int* tiles;
    const unsigned short* map;
    const unsigned short* palette;
    unsigned int tilesLen;
    unsigned int paletteLen;
} QuadrantData;

//=============================================================================
// Private Constants
//=============================================================================
#define FINISH_DISPLAY_FRAMES 150  // 2.5 seconds at 60fps to show final time
// TODO: Make these kinds of variables dependent on the game frequency,
// so if we change the frequency, everything updates accordingly.

// Uncomment to enable debug console on sub screen
// #define CONSOLE_DEBUG_MODE

//=============================================================================
// Private State - Timer
//=============================================================================

static int raceMin = 0;
static int raceSec = 0;
static int raceMsec = 0;
static int currentLap = 1;

static int totalRaceMin = 0;
static int totalRaceSec = 0;
static int totalRaceMsec = 0;

static int bestRaceMin = -1;  // -1 means no best time exists
static int bestRaceSec = -1;
static int bestRaceMsec = -1;

//=============================================================================
// Private State - Rendering
//=============================================================================

static int scrollX = 0;
static int scrollY = 0;
static QuadrantID currentQuadrant = QUAD_BR;

static u16* kartGfx = NULL;

#ifndef CONSOLE_DEBUG_MODE
static u16* itemDisplayGfx = NULL;
#endif

//=============================================================================
// Private State - Race Progress
//=============================================================================

//=============================================================================
// Private State - Race Progress
//=============================================================================

static bool countdownCleared = false;
static int finishDisplayCounter = 0;
static bool isNewRecord = false;
static bool hasSavedBestTime = false;

//=============================================================================
// Quadrant Data
//=============================================================================

static const QuadrantData quadrantData[9] = {
    {scorching_sands_TLTiles, scorching_sands_TLMap, scorching_sands_TLPal,
     scorching_sands_TLTilesLen, scorching_sands_TLPalLen},
    {scorching_sands_TCTiles, scorching_sands_TCMap, scorching_sands_TCPal,
     scorching_sands_TCTilesLen, scorching_sands_TCPalLen},
    {scorching_sands_TRTiles, scorching_sands_TRMap, scorching_sands_TRPal,
     scorching_sands_TRTilesLen, scorching_sands_TRPalLen},
    {scorching_sands_MLTiles, scorching_sands_MLMap, scorching_sands_MLPal,
     scorching_sands_MLTilesLen, scorching_sands_MLPalLen},
    {scorching_sands_MCTiles, scorching_sands_MCMap, scorching_sands_MCPal,
     scorching_sands_MCTilesLen, scorching_sands_MCPalLen},
    {scorching_sands_MRTiles, scorching_sands_MRMap, scorching_sands_MRPal,
     scorching_sands_MRTilesLen, scorching_sands_MRPalLen},
    {scorching_sands_BLTiles, scorching_sands_BLMap, scorching_sands_BLPal,
     scorching_sands_BLTilesLen, scorching_sands_BLPalLen},
    {scorching_sands_BCTiles, scorching_sands_BCMap, scorching_sands_BCPal,
     scorching_sands_BCTilesLen, scorching_sands_BCPalLen},
    {scorching_sands_BRTiles, scorching_sands_BRMap, scorching_sands_BRPal,
     scorching_sands_BRTilesLen, scorching_sands_BRPalLen}};

//=============================================================================
// Private Function Prototypes
//=============================================================================

// Graphics setup
static void Gameplay_ConfigureGraphics(void);
static void Gameplay_ConfigureMainBackground(void);
static void Gameplay_ConfigureSubBackground(void);
static void Gameplay_ConfigureSprites(void);
static void Gameplay_FreeSprites(void);

// Quadrant management
static void Gameplay_LoadQuadrant(QuadrantID quad);
static QuadrantID Gameplay_DetermineQuadrant(int x, int y);

// Camera and scrolling
static void Gameplay_UpdateCamera(const Car* player);
static void Gameplay_ClampScroll(void);

// Rendering
static void Gameplay_RenderCountdown(CountdownState state);
static void Gameplay_ClearCountdownDisplay(void);
static void Gameplay_RenderSinglePlayerCar(const Car* player);
static void Gameplay_RenderMultiplayerCars(const RaceState* state);
static void Gameplay_RenderAllCars(const RaceState* state);

// Display
static void Gameplay_DisplayFinalTime(int min, int sec, int msec);
static void Gameplay_PrintDigit(u16* map, int number, int x, int y);

// Item display (sub screen)
#ifndef CONSOLE_DEBUG_MODE
static void Gameplay_LoadItemDisplay(void);
static void Gameplay_UpdateItemDisplay(void);
#endif

#ifdef CONSOLE_DEBUG_MODE
static void Gameplay_ConfigureConsole(void);
#endif

//=============================================================================
// Public API - Lifecycle
//=============================================================================

/** Initialize the gameplay screen */
void Gameplay_Initialize(void) {
    // Reset timers
    raceMin = 0;
    raceSec = 0;
    raceMsec = 0;
    totalRaceMin = 0;
    totalRaceSec = 0;
    totalRaceMsec = 0;
    currentLap = 1;

    // Reset state flags
    countdownCleared = false;
    finishDisplayCounter = 0;
    hasSavedBestTime = false;
    isNewRecord = false;

    // Load best time for current map
    Map selectedMap = GameContext_GetMap();
    if (!StoragePB_LoadBestTime(selectedMap, &bestRaceMin, &bestRaceSec,
                                 &bestRaceMsec)) {
        bestRaceMin = -1;
        bestRaceSec = -1;
        bestRaceMsec = -1;
    }

    // Setup graphics
    Gameplay_ConfigureGraphics();
    Gameplay_ConfigureMainBackground();
    Gameplay_ConfigureSubBackground();

    // Initialize race
    GameContext* ctx = GameContext_Get();
    GameMode mode = ctx->isMultiplayerMode ? MultiPlayer : SinglePlayer;
    Race_Init(selectedMap, mode);

    // Setup sprites
    Gameplay_ConfigureSprites();

    // Initialize camera
    const Car* player = Race_GetPlayerCar();
    Gameplay_UpdateCamera(player);
    Gameplay_ClampScroll();

    currentQuadrant = Gameplay_DetermineQuadrant(scrollX, scrollY);
    Gameplay_LoadQuadrant(currentQuadrant);
}

/* Update gameplay screen logic */
GameState Gameplay_Update(void) {
//=============================================================================
// Public API - Update
//=============================================================================
    scanKeys();

    // Allow exit anytime with SELECT
    if (keysDown() & KEY_SELECT) {
        Race_Stop();
        return HOME_PAGE;
    }

    const RaceState* state = Race_GetState();

    // Save best time when race finishes (only once)
    if (state->raceFinished && !hasSavedBestTime) {
        Map currentMap = GameContext_GetMap();
        isNewRecord = StoragePB_SaveBestTime(currentMap, totalRaceMin, totalRaceSec,
                                             totalRaceMsec);

        // Reload actual best time from storage
        if (!StoragePB_LoadBestTime(currentMap, &bestRaceMin, &bestRaceSec,
                                    &bestRaceMsec)) {
            bestRaceMin = totalRaceMin;
            bestRaceSec = totalRaceSec;
            bestRaceMsec = totalRaceMsec;
        }

        hasSavedBestTime = true;
    }

    // Handle race finish display
    if (state->raceFinished && state->finishDelayTimer == 0) {
        finishDisplayCounter++;

        if (finishDisplayCounter >= FINISH_DISPLAY_FRAMES) {
            return (state->gameMode == MultiPlayer) ? HOME_PAGE : PLAYAGAIN;
        }
    }

    return GAMEPLAY;
}

/** VBlank callback for rendering gameplay elements */
void Gameplay_OnVBlank(void) {
    const Car* player = Race_GetPlayerCar();
    const RaceState* state = Race_GetState();

    // Handle final time display
    if (state->raceFinished && finishDisplayCounter < FINISH_DISPLAY_FRAMES) {
        Gameplay_DisplayFinalTime(totalRaceMin, totalRaceSec, totalRaceMsec);
        return;
    }

#ifdef CONSOLE_DEBUG_MODE
    // Debug console output
    consoleClear();
    printf("=== RED SHELL DEBUG ===\n");
    int itemCount = 0;
    const TrackItem* items = Items_GetActiveItems(&itemCount);
    int redShellCount = 0;
    for (int i = 0; i < itemCount; i++) {
        if (items[i].active && items[i].type == ITEM_RED_SHELL) {
            printf("Shell %d: (%d, %d)\n", redShellCount,
                   FixedToInt(items[i].position.x), FixedToInt(items[i].position.y));
            printf("  Angle: %d\n", items[i].angle512);
            printf("  Target: %d\n", items[i].targetCarIndex);
            printf("  Waypoint: %d\n", items[i].currentWaypoint);
            redShellCount++;
        }
    }
    if (redShellCount == 0) {
        printf("No red shells active\n");
    }
    printf("\nPlayer: (%d, %d)\n", FixedToInt(player->position.x),
           FixedToInt(player->position.y));
#endif

    // Handle countdown
    if (Race_IsCountdownActive()) {
        Race_UpdateCountdown();
        Gameplay_RenderCountdown(Race_GetCountdownState());
        Gameplay_UpdateCamera(player);
        Gameplay_ClampScroll();

        QuadrantID newQuadrant = Gameplay_DetermineQuadrant(scrollX, scrollY);
        if (newQuadrant != currentQuadrant) {
            Gameplay_LoadQuadrant(newQuadrant);
            currentQuadrant = newQuadrant;
            Race_SetLoadedQuadrant(newQuadrant);
        }

        int col = currentQuadrant % 3;
        int row = currentQuadrant / 3;
        BG_OFFSET[0].x = scrollX - (col * QUAD_SIZE);
        BG_OFFSET[0].y = scrollY - (row * QUAD_SIZE);

        Gameplay_RenderAllCars(state);
        oamUpdate(&oamMain);
        return;
    }

    // Clear countdown once
    if (!countdownCleared) {
        Gameplay_ClearCountdownDisplay();
        countdownCleared = true;
    }

    // Check finish line crossing
    if (Race_CheckFinishLineCross(player)) {
        if (currentLap < Race_GetLapCount()) {
            currentLap++;
            raceMin = 0;
            raceSec = 0;
            raceMsec = 0;
        } else {
            Race_MarkAsCompleted(totalRaceMin, totalRaceSec, totalRaceMsec);
            finishDisplayCounter = 0;
#ifndef CONSOLE_DEBUG_MODE
            // Hide item display when race finishes
            oamSet(&oamSub, 0, 0, 192, 0, 0, SpriteSize_32x32,
                   SpriteColorFormat_16Color, itemDisplayGfx, -1, true, false, false,
                   false, false);
            oamUpdate(&oamSub);
#endif
        }
    }

    // Update camera and quadrant
    Gameplay_UpdateCamera(player);
    Gameplay_ClampScroll();

    QuadrantID newQuadrant = Gameplay_DetermineQuadrant(scrollX, scrollY);
    if (newQuadrant != currentQuadrant) {
        Gameplay_LoadQuadrant(newQuadrant);
        currentQuadrant = newQuadrant;
        Race_SetLoadedQuadrant(newQuadrant);
    }

    int col = currentQuadrant % 3;
    int row = currentQuadrant / 3;
    BG_OFFSET[0].x = scrollX - (col * QUAD_SIZE);
    BG_OFFSET[0].y = scrollY - (row * QUAD_SIZE);

    // Render cars based on mode
    Gameplay_RenderAllCars(state);

    // Render items and update UI
    Items_Render(scrollX, scrollY);
#ifndef CONSOLE_DEBUG_MODE
    Gameplay_UpdateItemDisplay();
#endif
    oamUpdate(&oamMain);
}

/** Cleanup gameplay resources */
void Gameplay_Cleanup(void) {
    Gameplay_FreeSprites();
#ifndef CONSOLE_DEBUG_MODE
    if (itemDisplayGfx) {
        oamFreeGfx(&oamSub, itemDisplayGfx);
        itemDisplayGfx = NULL;
    }
#endif
}

//=============================================================================
// Public API - Timer Access
//=============================================================================

int Gameplay_GetRaceMin(void) {
    return raceMin;
}

int Gameplay_GetRaceSec(void) {
    return raceSec;
}

int Gameplay_GetRaceMsec(void) {
    return raceMsec;
}

int Gameplay_GetCurrentLap(void) {
    return currentLap;
}

void Gameplay_IncrementTimer(void) {
    if (Race_IsCompleted()) {
        return;
    }

    // Increment lap time
    raceMsec = (raceMsec + 1) % MS_PER_SECOND;
    if (raceMsec == 0) {
        raceSec = (raceSec + 1) % SECONDS_PER_MINUTE;
        if (raceSec == 0) {
            raceMin++;
        }
    }

    // Increment total time
    totalRaceMsec = (totalRaceMsec + 1) % MS_PER_SECOND;
    if (totalRaceMsec == 0) {
        totalRaceSec = (totalRaceSec + 1) % SECONDS_PER_MINUTE;
        if (totalRaceSec == 0) {
            totalRaceMin++;
        }
    }
}

//=============================================================================
// Public API - Sub Screen Display
//=============================================================================

void Gameplay_UpdateLapDisplay(int currentLap, int totalLaps) {
    u16* map = BG_MAP_RAM_SUB(0);
    int x, y;

    // Current lap
    x = 0;
    y = 0;
    if (currentLap >= 0 && currentLap <= 9) {
        Gameplay_PrintDigit(map, currentLap, x, y);
    }

    // Separator ":"
    x = 4;
    y = 0;
    Gameplay_PrintDigit(map, 10, x, y);

    // Total laps
    x = 6;
    y = 0;
    if (totalLaps >= 0 && totalLaps <= 9) {
        Gameplay_PrintDigit(map, totalLaps, x, y);
    } else if (totalLaps >= 10) {
        Gameplay_PrintDigit(map, totalLaps / 10, x, y);
        x = 10;
        y = 0;
        Gameplay_PrintDigit(map, totalLaps % 10, x, y);
    }
}

void Gameplay_UpdateChronoDisplay(int min, int sec, int msec) {
    u16* map = BG_MAP_RAM_SUB(0);
    int x = 0, y = 0;
    int number;

    // Minutes
    number = min;
    if (min > 59) {
        min = number = -1;
    }
    x = 0;
    y = 8;
    if (min >= 0) {
        number = min / 10;
    }
    Gameplay_PrintDigit(map, number, x, y);
    x = 4;
    y = 8;
    if (min >= 0) {
        number = min % 10;
    }
    Gameplay_PrintDigit(map, number, x, y);

    // Separator ":"
    x = 8;
    y = 8;
    number = 10;
    Gameplay_PrintDigit(map, number, x, y);

    // Seconds
    number = sec;
    if (sec > 59) {
        sec = number = -1;
    }
    x = 10;
    y = 8;
    if (sec >= 0) {
        number = sec / 10;
    }
    Gameplay_PrintDigit(map, number, x, y);
    x = 14;
    y = 8;
    if (sec >= 0) {
        number = sec % 10;
    }
    Gameplay_PrintDigit(map, number, x, y);

    // Separator "."
    x = 18;
    y = 8;
    number = 11;
    Gameplay_PrintDigit(map, number, x, y);

    // Milliseconds
    number = msec;
    if (msec > 999) {
        msec = number = -1;
    }
    x = 20;
    y = 8;
    if (msec >= 0) {
        number = msec / 100;
    }
    Gameplay_PrintDigit(map, number, x, y);
    x = 24;
    y = 8;
    if (msec >= 0) {
        number = (msec % 100) / 10;
    }
    Gameplay_PrintDigit(map, number, x, y);
    x = 28;
    y = 8;
    if (msec >= 0) {
        number = (msec % 10) % 10;
    }
    Gameplay_PrintDigit(map, number, x, y);
}

void Gameplay_ChangeDisplayColor(uint16 color) {
    BG_PALETTE_SUB[0] = color;
}

//=============================================================================
// Graphics Setup
//=============================================================================

/** Configure main and sub screen display registers */
static void Gameplay_ConfigureGraphics(void) {
    REG_DISPCNT = MODE_0_2D | DISPLAY_BG0_ACTIVE | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D;
    VRAM_A_CR = VRAM_ENABLE | VRAM_A_MAIN_BG;
    VRAM_B_CR = VRAM_ENABLE | VRAM_B_MAIN_SPRITE;

#ifdef CONSOLE_DEBUG_MODE
    REG_DISPCNT_SUB = MODE_0_2D | DISPLAY_BG0_ACTIVE;
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
#else
    REG_DISPCNT_SUB =
        MODE_0_2D | DISPLAY_BG0_ACTIVE | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D;
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
    VRAM_D_CR = VRAM_ENABLE | VRAM_D_SUB_SPRITE;
#endif
}

/** Configure main screen background */
static void Gameplay_ConfigureMainBackground(void) {
    Map selectedMap = GameContext_GetMap();
    if (selectedMap != ScorchingSands) {
        return;
    }

    // Priority 3 (lowest) so all sprites appear above the background
    BGCTRL[0] =
        BG_64x64 | BG_COLOR_256 | BG_MAP_BASE(0) | BG_TILE_BASE(1) | BG_PRIORITY(1);
}

/** Configure sub screen background and display */
static void Gameplay_ConfigureSubBackground(void) {
#ifdef CONSOLE_DEBUG_MODE
    Gameplay_ConfigureConsole();
#else
    BGCTRL_SUB[0] = BG_32x32 | BG_COLOR_256 | BG_MAP_BASE(0) | BG_TILE_BASE(1);
    swiCopy(numbersTiles, BG_TILE_RAM_SUB(1), numbersTilesLen);
    swiCopy(numbersPal, BG_PALETTE_SUB, numbersPalLen);
    BG_PALETTE_SUB[0] = BLACK;
    BG_PALETTE_SUB[255] = DARK_GRAY;  // neutral sub background
    memset(BG_MAP_RAM_SUB(0), 32, 32 * 32 * 2);
    // Initialize displays
    Gameplay_UpdateChronoDisplay(0, 0, 0);
    Gameplay_UpdateLapDisplay(1, Race_GetLapCount());
    Gameplay_LoadItemDisplay();
#endif
}

/** Configure sprites for karts and items */
static void Gameplay_ConfigureSprites(void) {
    oamInit(&oamMain, SpriteMapping_1D_32, false);
    dmaCopy(kart_spritePal, SPRITE_PALETTE, kart_spritePalLen);

    if (kartGfx) {
        oamFreeGfx(&oamMain, kartGfx);
    }

    kartGfx = oamAllocateGfx(&oamMain, SpriteSize_32x32, SpriteColorFormat_16Color);
    dmaCopy(kart_spriteTiles, kartGfx, kart_spriteTilesLen);

    const RaceState* state = Race_GetState();
    for (int i = 0; i < state->carCount; i++) {
        Race_SetCarGfx(i, kartGfx);
    }

    Items_LoadGraphics();
}

/** Free allocated sprite graphics */
static void Gameplay_FreeSprites(void) {
    if (kartGfx) {
        oamFreeGfx(&oamMain, kartGfx);
        kartGfx = NULL;
    }
    Items_FreeGraphics();
}

#ifdef CONSOLE_DEBUG_MODE
/** Configure debug console on sub screen */
static void Gameplay_ConfigureConsole(void) {
    consoleInit(NULL, 0, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);
    printf("\x1b[2J");
    printf("=== KART DEBUG ===\n");
    printf("SELECT = exit\n\n");
}
#endif

//=============================================================================
// Quadrant Management
//=============================================================================

/** Load quadrant tiles, palette, and map data */
static void Gameplay_LoadQuadrant(QuadrantID quad) {
    const QuadrantData* data = &quadrantData[quad];

    // Clear palette to avoid color pollution
    memset(BG_PALETTE, 0, 512);

    // Load tiles and palette
    dmaCopy(data->tiles, BG_TILE_RAM(1), data->tilesLen);
    dmaCopy(data->palette, BG_PALETTE, data->paletteLen);

    // Load map data (4 sub-maps for 64x64 background)
    for (int i = 0; i < 32; i++) {
        dmaCopy(&data->map[i * 64], &BG_MAP_RAM(0)[i * 32], 64);
        dmaCopy(&data->map[i * 64 + 32], &BG_MAP_RAM(1)[i * 32], 64);
        dmaCopy(&data->map[(i + 32) * 64], &BG_MAP_RAM(2)[i * 32], 64);
        dmaCopy(&data->map[(i + 32) * 64 + 32], &BG_MAP_RAM(3)[i * 32], 64);
    }
}

/** Determine which quadrant contains the given position */
static QuadrantID Gameplay_DetermineQuadrant(int x, int y) {
    int col = (x < QUAD_SIZE) ? 0 : (x < 2 * QUAD_SIZE) ? 1 : 2;
    int row = (y < QUAD_SIZE) ? 0 : (y < 2 * QUAD_SIZE) ? 1 : 2;
    return (QuadrantID)(row * QUADRANT_GRID_SIZE + col);
}

//=============================================================================
// Camera and Scrolling
//=============================================================================

/** Update camera to follow player */
static void Gameplay_UpdateCamera(const Car* player) {
    int carCenterX = FixedToInt(player->position.x) + CAR_SPRITE_CENTER_OFFSET;
    int carCenterY = FixedToInt(player->position.y) + CAR_SPRITE_CENTER_OFFSET;

    scrollX = carCenterX - (SCREEN_WIDTH / 2);
    scrollY = carCenterY - (SCREEN_HEIGHT / 2);
}

/** Clamp scroll position to map bounds */
static void Gameplay_ClampScroll(void) {
    if (scrollX < 0) {
        scrollX = 0;
    }
    if (scrollY < 0) {
        scrollY = 0;
    }
    if (scrollX > MAX_SCROLL_X) {
        scrollX = MAX_SCROLL_X;
    }
    if (scrollY > MAX_SCROLL_Y) {
        scrollY = MAX_SCROLL_Y;
    }
}

//=============================================================================
// Rendering
//=============================================================================

/** Render countdown numbers on sub screen */
static void Gameplay_RenderCountdown(CountdownState state) {
#ifndef CONSOLE_DEBUG_MODE
    u16* map = BG_MAP_RAM_SUB(0);

    // Clear entire screen first to remove any previous displays
    memset(map, 32, 32 * 32 * 2);

    int centerX = 14;
    int centerY = 10;

    switch (state) {
        case COUNTDOWN_3:
            Gameplay_PrintDigit(map, 3, centerX, centerY);
            break;
        case COUNTDOWN_2:
            Gameplay_PrintDigit(map, 2, centerX, centerY);
            break;
        case COUNTDOWN_1:
            Gameplay_PrintDigit(map, 1, centerX, centerY);
            break;
        case COUNTDOWN_GO:
            Gameplay_PrintDigit(map, 0, centerX, centerY);
            break;
        case COUNTDOWN_FINISHED:
            break;
    }
#endif
}

/** Clear countdown display from sub screen */
static void Gameplay_ClearCountdownDisplay(void) {
#ifndef CONSOLE_DEBUG_MODE
    u16* map = BG_MAP_RAM_SUB(0);
    // Clear entire screen to remove any countdown remnants
    for (int i = 0; i < 32; i++) {
        for (int j = 0; j < 32; j++) {
            map[i * 32 + j] = 32;
        }
    }
    // Redraw the timer display
    Gameplay_UpdateChronoDisplay(
        Gameplay_GetRaceMin(),
        Gameplay_GetRaceSec(),
        Gameplay_GetRaceMsec()
    );
    // Redraw the lap display
    Gameplay_UpdateLapDisplay(Gameplay_GetCurrentLap(), Race_GetLapCount());
#endif
}

/** Render single player car */
static void Gameplay_RenderSinglePlayerCar(const Car* player) {
    int carX = FixedToInt(player->position.x);
    int carY = FixedToInt(player->position.y);
    int screenX = carX - scrollX - 16;
    int screenY = carY - scrollY - 16;

    int dsAngle = -(player->angle512 << 6);
    oamRotateScale(&oamMain, 0, dsAngle, (1 << 8), (1 << 8));

    oamSet(&oamMain, 41, screenX, screenY, 0, 0, SpriteSize_32x32,
           SpriteColorFormat_16Color, player->gfx, 0, true, false, false, false,
           false);
}

/** Render all multiplayer cars */
static void Gameplay_RenderMultiplayerCars(const RaceState* state) {
    for (int i = 0; i < state->carCount; i++) {
        int oamSlot = 41 + i;

        if (!Multiplayer_IsPlayerConnected(i)) {
            oamSet(&oamMain, oamSlot, 0, 192, 0, 0, SpriteSize_32x32,
                   SpriteColorFormat_16Color, NULL, -1, true, false, false, false,
                   false);
            continue;
        }

        const Car* car = &state->cars[i];
        int carWorldX = FixedToInt(car->position.x);
        int carWorldY = FixedToInt(car->position.y);
        int carScreenX = carWorldX - scrollX - 16;
        int carScreenY = carWorldY - scrollY - 16;

        int dsAngle = -(car->angle512 << 6);
        oamRotateScale(&oamMain, i, dsAngle, (1 << 8), (1 << 8));

        bool onScreen = (carScreenX >= -32 && carScreenX < SCREEN_WIDTH &&
                         carScreenY >= -32 && carScreenY < SCREEN_HEIGHT);

        if (onScreen) {
            oamSet(&oamMain, oamSlot, carScreenX, carScreenY, 0, 0, SpriteSize_32x32,
                   SpriteColorFormat_16Color, car->gfx, i, true, false, false, false,
                   false);
        } else {
            oamSet(&oamMain, oamSlot, -64, -64, 0, 0, SpriteSize_32x32,
                   SpriteColorFormat_16Color, car->gfx, i, true, false, false, false,
                   false);
        }
    }
}

/** Render all cars based on game mode */
static void Gameplay_RenderAllCars(const RaceState* state) {
    const Car* player = Race_GetPlayerCar();

    if (state->gameMode == SinglePlayer) {
        Gameplay_RenderSinglePlayerCar(player);
    } else {
        Gameplay_RenderMultiplayerCars(state);
    }
}

//=============================================================================
// Display Functions
//=============================================================================

/** Display final race time on sub screen */
static void Gameplay_DisplayFinalTime(int min, int sec, int msec) {
#ifndef CONSOLE_DEBUG_MODE
    u16* map = BG_MAP_RAM_SUB(0);

    // Clear screen
    memset(map, 32, 32 * 32 * 2);

    // Display final time at y = 8
    int x = 0, y = 8;

    // Minutes
    Gameplay_PrintDigit(map, min / 10, x, y);
    x = 4;
    Gameplay_PrintDigit(map, min % 10, x, y);

    // Separator ":"
    x = 8;
    Gameplay_PrintDigit(map, 10, x, y);

    // Seconds
    x = 10;
    Gameplay_PrintDigit(map, sec / 10, x, y);
    x = 14;
    Gameplay_PrintDigit(map, sec % 10, x, y);

    // Separator "."
    x = 18;
    Gameplay_PrintDigit(map, 11, x, y);

    // Milliseconds (first digit only)
    x = 20;
    Gameplay_PrintDigit(map, msec / 100, x, y);

    // Display personal best below at y = 16
    if (bestRaceMin >= 0) {
        x = 0;
        y = 16;

        Gameplay_PrintDigit(map, bestRaceMin / 10, x, y);
        x = 4;
        Gameplay_PrintDigit(map, bestRaceMin % 10, x, y);

        x = 8;
        Gameplay_PrintDigit(map, 10, x, y);

        x = 10;
        Gameplay_PrintDigit(map, bestRaceSec / 10, x, y);
        x = 14;
        Gameplay_PrintDigit(map, bestRaceSec % 10, x, y);

        x = 18;
        Gameplay_PrintDigit(map, 11, x, y);

        x = 20;
        Gameplay_PrintDigit(map, bestRaceMsec / 100, x, y);
    }

    // Set background color based on new record status
    if (isNewRecord) {
        Gameplay_ChangeDisplayColor(ARGB16(1, 0, 20, 0));  // Green
    } else {
        Gameplay_ChangeDisplayColor(ARGB16(1, 0, 0, 0));  // Black
    }
#endif
}

/** Print a digit on the map at given position */
static void Gameplay_PrintDigit(u16* map, int number, int x, int y) {
    if (number < 10) {
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 4; j++) {
                map[(i + y) * 32 + j + x] =
                    (number >= 0) ? (u16)(i * 4 + j) + 32 * number : 32;
            }
        }
    } else if (number == 10) {
        // Colon separator
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 2; j++) {
                map[(i + y) * 32 + j + x] = (u16)(i * 4 + j) + 32 * 10 + 2;
            }
        }
    } else if (number == 11) {
        // Period separator
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 2; j++) {
                map[(i + y) * 32 + j + x] = (u16)(i * 4 + j) + 32 * 10;
            }
        }
    }
}

//=============================================================================
// Item Display (Sub Screen)
//=============================================================================

#ifndef CONSOLE_DEBUG_MODE
/** Load item display sprite on sub screen */
static void Gameplay_LoadItemDisplay(void) {
    oamInit(&oamSub, SpriteMapping_1D_32, false);

    itemDisplayGfx =
        oamAllocateGfx(&oamSub, SpriteSize_32x32, SpriteColorFormat_16Color);

    // Copy all item palettes to sub screen
    dmaCopy(bananaPal, &SPRITE_PALETTE_SUB[32], bananaPalLen);
    dmaCopy(bombPal, &SPRITE_PALETTE_SUB[48], bombPalLen);
    dmaCopy(green_shellPal, &SPRITE_PALETTE_SUB[64], green_shellPalLen);
    dmaCopy(red_shellPal, &SPRITE_PALETTE_SUB[80], red_shellPalLen);
    dmaCopy(missilePal, &SPRITE_PALETTE_SUB[96], missilePalLen);
    dmaCopy(oil_slickPal, &SPRITE_PALETTE_SUB[112], oil_slickPalLen);

    // Initially hide sprite
    oamSet(&oamSub, 0, 0, 192, 0, 0, SpriteSize_32x32,
           SpriteColorFormat_16Color, itemDisplayGfx, -1, true, false, false, false,
           false);
    oamUpdate(&oamSub);
}

/** Update item display sprite based on player's current item */
static void Gameplay_UpdateItemDisplay(void) {
    const Car* player = Race_GetPlayerCar();

    if (player->item == ITEM_NONE) {
        // Hide sprite when no item
        oamSet(&oamSub, 0, 0, 192, 0, 0, SpriteSize_32x32,
               SpriteColorFormat_16Color, itemDisplayGfx, -1, true, false, false,
               false, false);
    } else {
        int itemX = 220;
        int itemY = 8;

        const unsigned int* itemTiles = NULL;
        int paletteNum = 0;
        SpriteSize spriteSize = SpriteSize_16x16;

        switch (player->item) {
            case ITEM_BANANA:
                itemTiles = bananaTiles;
                paletteNum = 2;
                spriteSize = SpriteSize_16x16;
                break;
            case ITEM_BOMB:
                itemTiles = bombTiles;
                paletteNum = 3;
                spriteSize = SpriteSize_16x16;
                break;
            case ITEM_GREEN_SHELL:
                itemTiles = green_shellTiles;
                paletteNum = 4;
                spriteSize = SpriteSize_16x16;
                break;
            case ITEM_RED_SHELL:
                itemTiles = red_shellTiles;
                paletteNum = 5;
                spriteSize = SpriteSize_16x16;
                break;
            case ITEM_MISSILE:
                itemTiles = missileTiles;
                paletteNum = 6;
                spriteSize = SpriteSize_16x32;
                break;
            case ITEM_OIL:
                itemTiles = oil_slickTiles;
                paletteNum = 7;
                spriteSize = SpriteSize_32x32;
                itemX = 208;
                break;
            case ITEM_MUSHROOM:
                itemTiles = bananaTiles;
                paletteNum = 2;
                spriteSize = SpriteSize_16x16;
                break;
            case ITEM_SPEEDBOOST:
                itemTiles = red_shellTiles;
                paletteNum = 5;
                spriteSize = SpriteSize_16x16;
                break;
            default:
                itemTiles = NULL;
                break;
        }

        if (itemTiles != NULL) {
            int tilesLen = (player->item == ITEM_MISSILE)   ? missileTilesLen
                           : (player->item == ITEM_OIL)     ? oil_slickTilesLen
                                                            : bananaTilesLen;
            dmaCopy(itemTiles, itemDisplayGfx, tilesLen);

            oamSet(&oamSub, 0, itemX, itemY, 0, paletteNum, spriteSize,
                   SpriteColorFormat_16Color, itemDisplayGfx, -1, false, false, false,
                   false, false);
        }
    }

    oamUpdate(&oamSub);
}
#endif
