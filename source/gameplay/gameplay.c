/**
 * File: gameplay.c
 * ----------------
 * Description: Main gameplay screen implementation for racing. Handles graphics
 *              initialization, VBlank rendering updates, camera management,
 *              quadrant loading/switching, timer updates, sub-screen display
 *              (lap counter, chrono, item display), and final time rendering.
 *              Coordinates with gameplay_logic.c for race state.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 06.01.2026
 */

#include "gameplay.h"

#include <nds.h>
#include <stdio.h>
#include <string.h>
#include "items/items_api.h"
#include "../core/context.h"
#include "../core/game_constants.h"
#include "../core/game_types.h"
#include "../graphics/color.h"
#include "../network/multiplayer.h"
#include "../storage/storage_pb.h"
#include "../ui/play_again.h"
#include "data/items/banana.h"
#include "data/items/bomb.h"
#include "gameplay_logic.h"
#include "data/items/green_shell.h"
#include "data/sprites/kart_sprite.h"
#include "data/items/missile.h"
#include "data/sprites/numbers.h"
#include "data/items/oil_slick.h"
#include "data/items/red_shell.h"
#include "data/tracks/scorching_sands_BC.h"
#include "data/tracks/scorching_sands_BL.h"
#include "data/tracks/scorching_sands_BR.h"
#include "data/tracks/scorching_sands_MC.h"
#include "data/tracks/scorching_sands_ML.h"
#include "data/tracks/scorching_sands_MR.h"
#include "data/tracks/scorching_sands_TC.h"
#include "data/tracks/scorching_sands_TL.h"
#include "data/tracks/scorching_sands_TR.h"
#include "wall_collision.h"

//=============================================================================
// PRIVATE CONSTANTS
//=============================================================================

//! DEBUGGING FLAG
// #define console_on_debug  // Uncomment to enable debug console on sub screen
//! DEBUGGING FLAG

//=============================================================================
// PRIVATE STATE
//=============================================================================
static int raceMin = 0;
static int raceSec = 0;
static int raceMsec = 0;
static int currentLap = 1;

static int scrollX = 0;
static int scrollY = 0;
static QuadrantID currentQuadrant = QUAD_BR;

// Sprite graphics pointer (allocated during configureSprite)
static u16* kartGfx = NULL;
#ifndef console_on_debug
static u16* itemDisplayGfx_Sub = NULL;
#endif

static bool countdownCleared = false;
static int finishDisplayCounter = 0;  // NEW: Count frames showing final time

static int totalRaceMin = 0;
static int totalRaceSec = 0;
static int totalRaceMsec = 0;

static int bestRaceMin = -1;  // -1 means no best time exists
static int bestRaceSec = -1;
static int bestRaceMsec = -1;
static bool isNewRecord = false;

static bool hasSavedBestTime = false;

//=============================================================================
// PRIVATE TYPES - Quadrant Data
//=============================================================================
typedef struct {
    const unsigned int* tiles;
    const unsigned short* map;
    const unsigned short* palette;  // ADDED: Each quadrant has its own palette
    unsigned int tilesLen;
    unsigned int paletteLen;  // ADDED: Palette length
} QuadrantData;

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
// PRIVATE HELPER PROTOTYPES
//=============================================================================
static void Gameplay_ConfigureGraphics(void);
static void Gameplay_ConfigureBackground(void);
static void Gameplay_ConfigureSprite(void);
static void Gameplay_FreeSprites(void);
#ifdef console_on_debug
static void Gameplay_ConfigureConsole(void);
#endif
static void Gameplay_LoadQuadrant(QuadrantID quad);
static QuadrantID Gameplay_DetermineQuadrant(int x, int y);
static void Gameplay_RenderCountdown(CountdownState state);
static void Gameplay_ClearCountdownDisplay(void);
static void Gameplay_DisplayFinalTime(int min, int sec, int msec);
static void Gameplay_UpdateChronoDisp(u16* map, int min, int sec, int msec);
#ifndef console_on_debug
static void Gameplay_LoadItemDisplay_Sub(void);
static void Gameplay_UpdateItemDisplay_Sub(void);
#endif
// VBlank rendering helpers
static void Gameplay_UpdateCameraPosition(const Car* player);
static void Gameplay_ApplyCameraScroll(void);
static void Gameplay_RenderSinglePlayerCar(const Car* player, int carX, int carY);
static void Gameplay_RenderMultiplayerCars(const RaceState* state);
static void Gameplay_HandleFinishLineCrossing(const Car* player);
static bool Gameplay_HandleFinishDisplay(const RaceState* state);
#ifdef console_on_debug
static void Gameplay_DebugPrintRedShells(const Car* player);
#endif
static bool Gameplay_HandleCountdownPhase(const Car* player, const RaceState* state);
static void Gameplay_ClearCountdownDisplayOnce(void);
static void Gameplay_HandleRacePhase(const Car* player, const RaceState* state);
static void Gameplay_RenderCarsForMode(const RaceState* state, const Car* player,
                                       int carX, int carY);

//=============================================================================
// PUBLIC API - Timer Access
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
// PUBLIC API - Lifecycle
//=============================================================================
// Helper: Reset all race state variables
static void Gameplay_ResetRaceState(void) {
    raceMin = 0;
    raceSec = 0;
    raceMsec = 0;
    totalRaceMin = 0;
    totalRaceSec = 0;
    totalRaceMsec = 0;
    currentLap = 1;
    countdownCleared = false;
    finishDisplayCounter = 0;
    hasSavedBestTime = false;
    isNewRecord = false;
}

// Helper: Load best time for current map
static void Gameplay_LoadBestTime(Map map) {
    if (!StoragePB_LoadBestTime(map, &bestRaceMin, &bestRaceSec, &bestRaceMsec)) {
        bestRaceMin = -1;
        bestRaceSec = -1;
        bestRaceMsec = -1;
    }
}

// Helper: Initialize camera to player position
static void Gameplay_InitializeCamera(const Car* player) {
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

    currentQuadrant = Gameplay_DetermineQuadrant(scrollX, scrollY);
    Gameplay_LoadQuadrant(currentQuadrant);
}

void Gameplay_Initialize(void) {
    // Configure graphics and background
    Gameplay_ConfigureGraphics();
    Gameplay_ConfigureBackground();

    // Reset race state variables
    Gameplay_ResetRaceState();

    // Get map and mode from context
    Map selectedMap = GameContext_GetMap();
    GameContext* ctx = GameContext_Get();
    GameMode mode = ctx->isMultiplayerMode ? MultiPlayer : SinglePlayer;

    // Load best time for this map
    Gameplay_LoadBestTime(selectedMap);

    // Clear sub screen display
#ifndef console_on_debug
    u16* map = BG_MAP_RAM_SUB(0);
    memset(map, 32, 32 * 32 * 2);
    Gameplay_ChangeDisplayColor(BLACK);
#endif

    // Initialize race logic and configure sprites
    Race_Init(selectedMap, mode);
    Gameplay_ConfigureSprite();

    // Initialize camera to follow player
    const Car* player = Race_GetPlayerCar();
    Gameplay_InitializeCamera(player);
}

GameState Gameplay_Update(void) {
    scanKeys();
    int keysdown = keysDown();

    // Handle SELECT to exit anytime
    if (keysdown & KEY_SELECT) {
        Race_Stop();
        return HOME_PAGE;
    }

    const RaceState* state = Race_GetState();

    // CHANGED: Fixed best time saving and display logic
    // Save best time once when race finishes (NOT in VBlank - safe here!)
    if (state->raceFinished && !hasSavedBestTime) {
        Map currentMap = GameContext_GetMap();

        // Try to save the time (returns true if it's a new record)
        isNewRecord = StoragePB_SaveBestTime(currentMap, totalRaceMin, totalRaceSec,
                                             totalRaceMsec);

        // CHANGED: Load the actual best time from file instead of always using current
        // time This ensures we display the real best time, not just the current race
        // time
        if (!StoragePB_LoadBestTime(currentMap, &bestRaceMin, &bestRaceSec,
                                    &bestRaceMsec)) {
            // No best time exists (shouldn't happen after save, but handle it)
            bestRaceMin = totalRaceMin;
            bestRaceSec = totalRaceSec;
            bestRaceMsec = totalRaceMsec;
        }

        hasSavedBestTime = true;
    }

    // Check if race finished and counting display frames
    if (state->raceFinished && state->finishDelayTimer == 0) {
        finishDisplayCounter++;

        // After 2.5 seconds, transition to PLAYAGAIN state
        if (finishDisplayCounter >= FINISH_DISPLAY_FRAMES) {
            if (state->gameMode == MultiPlayer) {
                return HOME_PAGE;
            } else {
                return PLAYAGAIN;
            }
        }
    }

    return GAMEPLAY;
}

//=============================================================================
// Helper: Update Camera Position
//=============================================================================
static void Gameplay_UpdateCameraPosition(const Car* player) {
    int carX = FixedToInt(player->position.x) + CAR_SPRITE_CENTER_OFFSET;
    int carY = FixedToInt(player->position.y) + CAR_SPRITE_CENTER_OFFSET;

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
}

//=============================================================================
// Helper: Apply Camera Scroll to Background
//=============================================================================
static void Gameplay_ApplyCameraScroll(void) {
    QuadrantID newQuadrant = Gameplay_DetermineQuadrant(scrollX, scrollY);
    if (newQuadrant != currentQuadrant) {
        Gameplay_LoadQuadrant(newQuadrant);
        currentQuadrant = newQuadrant;
        Race_SetLoadedQuadrant(newQuadrant);
    }

    int col = currentQuadrant % 3;
    int row = currentQuadrant / 3;
    BG_OFFSET[0].x = scrollX - (col * QUAD_OFFSET);
    BG_OFFSET[0].y = scrollY - (row * QUAD_OFFSET);
}

//=============================================================================
// Helper: Render Single Player Car
//=============================================================================
static void Gameplay_RenderSinglePlayerCar(const Car* player, int carX, int carY) {
    int screenX = carX - scrollX - 16;
    int screenY = carY - scrollY - 16;

    int dsAngle = -(player->angle512 << 6);
    oamRotateScale(&oamMain, 0, dsAngle, (1 << 8), (1 << 8));

    oamSet(&oamMain, 41, screenX, screenY, OBJPRIORITY_0, 0, SpriteSize_32x32,
           SpriteColorFormat_16Color, player->gfx, 0, true, false, false, false,
           false);
}

//=============================================================================
// Helper: Render Multiplayer Cars
//=============================================================================
static void Gameplay_RenderMultiplayerCars(const RaceState* state) {
    for (int i = 0; i < state->carCount; i++) {
        int oamSlot = 41 + i;

        if (!Multiplayer_IsPlayerConnected(i)) {
            oamSet(&oamMain, oamSlot, 0, 192, OBJPRIORITY_0, 0, SpriteSize_32x32,
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
            oamSet(&oamMain, oamSlot, carScreenX, carScreenY, OBJPRIORITY_0, 0,
                   SpriteSize_32x32, SpriteColorFormat_16Color, car->gfx, i, true,
                   false, false, false, false);
        } else {
            oamSet(&oamMain, oamSlot, -64, -64, OBJPRIORITY_0, 0, SpriteSize_32x32,
                   SpriteColorFormat_16Color, car->gfx, i, true, false, false, false,
                   false);
        }
    }
}

//=============================================================================
// Helper: Handle Finish Line Crossing
//=============================================================================
static void Gameplay_HandleFinishLineCrossing(const Car* player) {
    if (!Race_CheckFinishLineCross(player))
        return;

    if (currentLap < Race_GetLapCount()) {
        // Normal lap completion - reset LAP timer (but total keeps running)
        currentLap++;
        raceMin = 0;
        raceSec = 0;
        raceMsec = 0;
    } else {
        // RACE COMPLETED!
        Race_MarkAsCompleted(totalRaceMin, totalRaceSec, totalRaceMsec);
        finishDisplayCounter = 0;
#ifndef console_on_debug
        // Hide item sprite when race finishes
        oamSet(&oamSub, 0, 0, 192, 0, 0, SpriteSize_32x32, SpriteColorFormat_16Color,
               itemDisplayGfx_Sub, -1, true, false, false, false, false);
        oamUpdate(&oamSub);
#endif
    }
}

static bool Gameplay_HandleFinishDisplay(const RaceState* state) {
    if (state->raceFinished && finishDisplayCounter < FINISH_DISPLAY_FRAMES) {
        Gameplay_DisplayFinalTime(totalRaceMin, totalRaceSec, totalRaceMsec);
        return true;
    }

    return false;
}

#ifdef console_on_debug
static void Gameplay_DebugPrintRedShells(const Car* player) {
    consoleClear();
    printf("=== RED SHELL DEBUG ===\n");
    int itemCount = 0;
    const TrackItem* items = Items_GetActiveItems(&itemCount);
    int redShellCount = 0;
    for (int i = 0; i < itemCount; i++) {
        if (items[i].active && items[i].type == ITEM_RED_SHELL) {
            printf("Shell %d: (%d, %d)\n", redShellCount,
                   FixedToInt(items[i].position.x), FixedToInt(items[i].position.y));
            printf("  Angle: %d, Target: %d, Waypoint: %d\n", items[i].angle512,
                   items[i].targetCarIndex, items[i].currentWaypoint);
            redShellCount++;
        }
    }
    if (redShellCount == 0)
        printf("No red shells active\n");
    printf("\nPlayer: (%d, %d)\n", FixedToInt(player->position.x),
           FixedToInt(player->position.y));
}
#endif

static void Gameplay_RenderCarsForMode(const RaceState* state, const Car* player,
                                       int carX, int carY) {
    if (state->gameMode == SinglePlayer) {
        Gameplay_RenderSinglePlayerCar(player, carX, carY);
    } else {
        Gameplay_RenderMultiplayerCars(state);
    }
}

static bool Gameplay_HandleCountdownPhase(const Car* player, const RaceState* state) {
    if (!Race_IsCountdownActive()) {
        return false;
    }

    Race_UpdateCountdown();
    Gameplay_RenderCountdown(Race_GetCountdownState());

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

    Gameplay_ApplyCameraScroll();
    Gameplay_RenderCarsForMode(state, player, carX, carY);

    oamUpdate(&oamMain);
    return true;
}

static void Gameplay_ClearCountdownDisplayOnce(void) {
    if (!countdownCleared) {
        Gameplay_ClearCountdownDisplay();
        countdownCleared = true;
    }
}

static void Gameplay_HandleRacePhase(const Car* player, const RaceState* state) {
    Gameplay_HandleFinishLineCrossing(player);
    Gameplay_UpdateCameraPosition(player);
    Gameplay_ApplyCameraScroll();

    int carX = FixedToInt(player->position.x);
    int carY = FixedToInt(player->position.y);
    Gameplay_RenderCarsForMode(state, player, carX, carY);

    Items_Render(scrollX, scrollY);
#ifndef console_on_debug
    Gameplay_UpdateItemDisplay_Sub();
#endif
    oamUpdate(&oamMain);
}

void Gameplay_OnVBlank(void) {
    const Car* player = Race_GetPlayerCar();
    const RaceState* state = Race_GetState();

    if (Gameplay_HandleFinishDisplay(state)) {
        return;
    }

#ifdef console_on_debug
    Gameplay_DebugPrintRedShells(player);
#endif

    if (Gameplay_HandleCountdownPhase(player, state)) {
        return;
    }

    Gameplay_ClearCountdownDisplayOnce();
    Gameplay_HandleRacePhase(player, state);
}

void Gameplay_Cleanup(void) {
    Gameplay_FreeSprites();
#ifndef console_on_debug
    if (itemDisplayGfx_Sub) {
        oamFreeGfx(&oamSub, itemDisplayGfx_Sub);
        itemDisplayGfx_Sub = NULL;
    }
#endif
}

//=============================================================================
// PRIVATE HELPERS - Final Time Display
//=============================================================================

// Helper: Display time at given y position (format: MM:SS.D)
static void Gameplay_PrintTime(u16* map, int min, int sec, int msec, int y) {
    // Minutes
    Gameplay_PrintDigit(map, min / 10, 0, y);
    Gameplay_PrintDigit(map, min % 10, 4, y);

    // Separator ":"
    Gameplay_PrintDigit(map, 10, 8, y);

    // Seconds
    Gameplay_PrintDigit(map, sec / 10, 10, y);
    Gameplay_PrintDigit(map, sec % 10, 14, y);

    // Separator "."
    Gameplay_PrintDigit(map, 11, 18, y);

    // Milliseconds (only first digit)
    Gameplay_PrintDigit(map, msec / 100, 20, y);
}

static void Gameplay_DisplayFinalTime(int min, int sec, int msec) {
#ifndef console_on_debug
    u16* map = BG_MAP_RAM_SUB(0);

    // Clear screen
    memset(map, 32, 32 * 32 * 2);

    // Display FINAL TIME at top (y = 8)
    Gameplay_PrintTime(map, min, sec, msec, 8);

    // Display PERSONAL BEST below (y = 16)
    if (bestRaceMin >= 0) {
        Gameplay_PrintTime(map, bestRaceMin, bestRaceSec, bestRaceMsec, 16);
    }

    // Set background color based on whether it's a new record
    Gameplay_ChangeDisplayColor(isNewRecord ? DARK_GREEN : BLACK);
#endif
}

//=============================================================================
// PRIVATE HELPERS - Countdown Display
//=============================================================================
static void Gameplay_RenderCountdown(CountdownState state) {
#ifndef console_on_debug
    u16* map = BG_MAP_RAM_SUB(0);

    // Clear previous countdown display
    for (int i = 16; i < 24; i++) {
        for (int j = 12; j < 20; j++) {
            map[i * 32 + j] = 32;  // Empty tile
        }
    }

    // Center position for large countdown numbers
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

static void Gameplay_ClearCountdownDisplay(void) {
#ifndef console_on_debug
    u16* map = BG_MAP_RAM_SUB(0);

    for (int i = 16; i < 24; i++) {
        for (int j = 12; j < 20; j++) {
            map[i * 32 + j] = 32;
        }
    }
#endif
}

//=============================================================================
// PRIVATE HELPERS - Graphics Setup
//=============================================================================
static void Gameplay_ConfigureGraphics(void) {
    REG_DISPCNT = MODE_0_2D | DISPLAY_BG0_ACTIVE | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D;
    VRAM_A_CR = VRAM_ENABLE | VRAM_A_MAIN_BG;
    VRAM_B_CR = VRAM_ENABLE | VRAM_B_MAIN_SPRITE;

#ifdef console_on_debug
    // Debug mode: Set up console on sub screen
    REG_DISPCNT_SUB = MODE_0_2D | DISPLAY_BG0_ACTIVE;
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
#else
    // Normal mode: Enable sprites on sub screen
    REG_DISPCNT_SUB =
        MODE_0_2D | DISPLAY_BG0_ACTIVE | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D;
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
    VRAM_D_CR = VRAM_ENABLE | VRAM_D_SUB_SPRITE;  // NEW: Sub sprite VRAM
#endif
}

static void Gameplay_ConfigureBackground(void) {
    Map selectedMap = GameContext_GetMap();
    if (selectedMap != ScorchingSands)
        return;

    // Priority 3 (lowest) so all sprites appear above the background
    BGCTRL[0] =
        BG_64x64 | BG_COLOR_256 | BG_MAP_BASE(0) | BG_TILE_BASE(1) | BG_PRIORITY(3);

#ifdef console_on_debug
    // Debug mode: Set up console
    Gameplay_ConfigureConsole();
#else
    // Normal mode: Sub screen setup with numbers tileset
    BGCTRL_SUB[0] = BG_32x32 | BG_COLOR_256 | BG_MAP_BASE(0) | BG_TILE_BASE(1);
    swiCopy(numbersTiles, BG_TILE_RAM_SUB(1), numbersTilesLen);
    swiCopy(numbersPal, BG_PALETTE_SUB, numbersPalLen);
    BG_PALETTE_SUB[0] = BLACK;
    BG_PALETTE_SUB[255] = DARK_GRAY;  // neutral sub background
    memset(BG_MAP_RAM_SUB(0), 32, 32 * 32 * 2);
    Gameplay_UpdateChronoDisplay(-1, -1, -1);
    Gameplay_LoadItemDisplay_Sub();
#endif
}

static void Gameplay_ConfigureSprite(void) {
    oamInit(&oamMain, SpriteMapping_1D_32, false);

    dmaCopy(kart_spritePal, SPRITE_PALETTE, kart_spritePalLen);

    if (kartGfx) {
        oamFreeGfx(&oamMain, kartGfx);
    }

    kartGfx = oamAllocateGfx(&oamMain, SpriteSize_32x32, SpriteColorFormat_16Color);
    dmaCopy(kart_spriteTiles, kartGfx, kart_spriteTilesLen);

    const RaceState* state = Race_GetState();
    // Set graphics for ALL cars
    for (int i = 0; i < (state->carCount); i++) {
        Race_SetCarGfx(i, kartGfx);
    }

    Items_LoadGraphics();
}

static void Gameplay_FreeSprites(void) {
    if (kartGfx) {
        oamFreeGfx(&oamMain, kartGfx);
        kartGfx = NULL;
    }
    Items_FreeGraphics();
}

#ifdef console_on_debug
// DEBUG Console setup (active for gameplay debug)
static void Gameplay_ConfigureConsole(void) {
    // Use sub screen BG0 for console
    consoleInit(NULL, 0, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);
    printf("\x1b[2J");
    printf("=== KART DEBUG ===\n");
    printf("SELECT = exit\n\n");
}
#endif

//=============================================================================
// PRIVATE HELPERS - Sub Screen Item Display
//=============================================================================
#ifndef console_on_debug
static void Gameplay_LoadItemDisplay_Sub(void) {
    // Initialize sub screen OAM
    oamInit(&oamSub, SpriteMapping_1D_32, false);

    // CHANGED: Allocate for largest sprite size (32x32) to handle oil slick
    itemDisplayGfx_Sub =
        oamAllocateGfx(&oamSub, SpriteSize_32x32, SpriteColorFormat_16Color);

    // Copy all item palettes to sub screen sprite palette
    dmaCopy(bananaPal, &SPRITE_PALETTE_SUB[32], bananaPalLen);
    dmaCopy(bombPal, &SPRITE_PALETTE_SUB[48], bombPalLen);
    dmaCopy(green_shellPal, &SPRITE_PALETTE_SUB[64], green_shellPalLen);
    dmaCopy(red_shellPal, &SPRITE_PALETTE_SUB[80], red_shellPalLen);
    dmaCopy(missilePal, &SPRITE_PALETTE_SUB[96], missilePalLen);
    dmaCopy(oil_slickPal, &SPRITE_PALETTE_SUB[112], oil_slickPalLen);

    // Initially hide the item sprite
    oamSet(&oamSub, 0, 0, 192, 0, 0, SpriteSize_32x32,  // CHANGED: to 32x32
           SpriteColorFormat_16Color, itemDisplayGfx_Sub, -1, true, false, false,
           false, false);
    oamUpdate(&oamSub);
}

// Helper: Get item sprite properties for a given item type
static void Gameplay_GetItemSpriteInfo(Item item, const unsigned int** outTiles,
                                       int* outPalette, SpriteSize* outSize,
                                       int* outX) {
    *outX = 220;  // Default position (256 - 16 - margin)

    switch (item) {
        case ITEM_BANANA:
            *outTiles = bananaTiles;
            *outPalette = 2;
            *outSize = SpriteSize_16x16;
            break;
        case ITEM_BOMB:
            *outTiles = bombTiles;
            *outPalette = 3;
            *outSize = SpriteSize_16x16;
            break;
        case ITEM_GREEN_SHELL:
            *outTiles = green_shellTiles;
            *outPalette = 4;
            *outSize = SpriteSize_16x16;
            break;
        case ITEM_RED_SHELL:
            *outTiles = red_shellTiles;
            *outPalette = 5;
            *outSize = SpriteSize_16x16;
            break;
        case ITEM_MISSILE:
            *outTiles = missileTiles;
            *outPalette = 6;
            *outSize = SpriteSize_16x32;
            break;
        case ITEM_OIL:
            *outTiles = oil_slickTiles;
            *outPalette = 7;
            *outSize = SpriteSize_32x32;
            *outX = 208;  // Adjust for larger sprite (256 - 32 - margin)
            break;
        case ITEM_MUSHROOM:
            *outTiles = bananaTiles;
            *outPalette = 2;
            *outSize = SpriteSize_16x16;
            break;
        case ITEM_SPEEDBOOST:
            *outTiles = red_shellTiles;
            *outPalette = 5;
            *outSize = SpriteSize_16x16;
            break;
        default:
            *outTiles = NULL;
            break;
    }
}

static void Gameplay_UpdateItemDisplay_Sub(void) {
    const Car* player = Race_GetPlayerCar();

    if (player->item == ITEM_NONE) {
        // Hide sprite when no item
        oamSet(&oamSub, 0, 0, 192, 0, 0, SpriteSize_32x32, SpriteColorFormat_16Color,
               itemDisplayGfx_Sub, -1, true, false, false, false, false);
    } else {
        // Get item sprite properties
        const unsigned int* itemTiles = NULL;
        int paletteNum = 0;
        SpriteSize spriteSize = SpriteSize_16x16;
        int itemX = 0;

        Gameplay_GetItemSpriteInfo(player->item, &itemTiles, &paletteNum, &spriteSize,
                                   &itemX);

        if (itemTiles != NULL) {
            // Copy item graphics to sub screen sprite memory
            int tilesLen = (player->item == ITEM_MISSILE) ? missileTilesLen
                           : (player->item == ITEM_OIL)   ? oil_slickTilesLen
                                                          : bananaTilesLen;
            dmaCopy(itemTiles, itemDisplayGfx_Sub, tilesLen);

            // Display sprite with appropriate size
            oamSet(&oamSub, 0, itemX, 8, 0, paletteNum, spriteSize,
                   SpriteColorFormat_16Color, itemDisplayGfx_Sub, -1, false, false,
                   false, false, false);
        }
    }

    oamUpdate(&oamSub);
}
#endif

//=============================================================================
// PRIVATE HELPERS - Quadrant Management
//=============================================================================
static void Gameplay_LoadQuadrant(QuadrantID quad) {
    const QuadrantData* data = &quadrantData[quad];
    // CHANGED: Clear the entire palette first to avoid color pollution
    memset(BG_PALETTE, 0, 512);  // 256 colors × 2 bytes = 512 bytes
    // CHANGED: Load tiles for this quadrant
    dmaCopy(data->tiles, BG_TILE_RAM(1), data->tilesLen);

    // CHANGED: Load palette for this quadrant
    dmaCopy(data->palette, BG_PALETTE, data->paletteLen);

    // Load the map data for this quadrant
    for (int i = 0; i < 32; i++) {
        dmaCopy(&data->map[i * 64], &BG_MAP_RAM(0)[i * 32], 64);
        dmaCopy(&data->map[i * 64 + 32], &BG_MAP_RAM(1)[i * 32], 64);
        dmaCopy(&data->map[(i + 32) * 64], &BG_MAP_RAM(2)[i * 32], 64);
        dmaCopy(&data->map[(i + 32) * 64 + 32], &BG_MAP_RAM(3)[i * 32], 64);
    }
}

static QuadrantID Gameplay_DetermineQuadrant(int x, int y) {
    int col = (x < QUAD_OFFSET) ? 0 : (x < 2 * QUAD_OFFSET) ? 1 : 2;
    int row = (y < QUAD_OFFSET) ? 0 : (y < 2 * QUAD_OFFSET) ? 1 : 2;
    return (QuadrantID)(row * QUADRANT_GRID_SIZE + col);
}

//=============================================================================
// PUBLIC API - Sub-Screen Display
//=============================================================================
void Gameplay_PrintDigit(u16* map, int number, int x, int y) {
    int i, j;
    if (number < 10) {
        for (i = 0; i < 8; i++)
            for (j = 0; j < 4; j++)
                map[(i + y) * 32 + j + x] =
                    (number >= 0) ? (u16)(i * 4 + j) + 32 * number : 32;
    }
    if (number == 10) {
        for (i = 0; i < 8; i++)
            for (j = 0; j < 2; j++)
                map[(i + y) * 32 + j + x] = (u16)(i * 4 + j) + 32 * 10 + 2;
    }
    if (number == 11) {
        for (i = 0; i < 8; i++)
            for (j = 0; j < 2; j++)
                map[(i + y) * 32 + j + x] = (u16)(i * 4 + j) + 32 * 10;
    }
}

static void Gameplay_UpdateChronoDisp(u16* map, int min, int sec, int msec) {
    int x = 0, y = 0;
    int number;

    // Minutes
    number = min;
    if (min > 59)
        min = number = -1;
    x = 0;
    y = 8;
    if (min >= 0)
        number = min / 10;
    Gameplay_PrintDigit(map, number, x, y);
    x = 4;
    y = 8;
    if (min >= 0)
        number = min % 10;
    Gameplay_PrintDigit(map, number, x, y);

    // Separator ":"
    x = 8;
    y = 8;
    number = 10;
    Gameplay_PrintDigit(map, number, x, y);

    // Seconds
    number = sec;
    if (sec > 59)
        sec = number = -1;
    x = 10;
    y = 8;
    if (sec >= 0)
        number = sec / 10;
    Gameplay_PrintDigit(map, number, x, y);
    x = 14;
    y = 8;
    if (sec >= 0)
        number = sec % 10;
    Gameplay_PrintDigit(map, number, x, y);

    // Separator "."
    x = 18;
    y = 8;
    number = 11;
    Gameplay_PrintDigit(map, number, x, y);

    // Milliseconds
    number = msec;
    if (msec > 999)
        msec = number = -1;
    x = 20;
    y = 8;
    if (msec >= 0)
        number = msec / 100;
    Gameplay_PrintDigit(map, number, x, y);
    x = 24;
    y = 8;
    if (msec >= 0)
        number = (msec % 100) / 10;
    Gameplay_PrintDigit(map, number, x, y);
    x = 28;
    y = 8;
    if (msec >= 0)
        number = (msec % 10) % 10;
    Gameplay_PrintDigit(map, number, x, y);
}

void Gameplay_UpdateChronoDisplay(int min, int sec, int msec) {
    Gameplay_UpdateChronoDisp(BG_MAP_RAM_SUB(0), min, sec, msec);
}

void Gameplay_ChangeDisplayColor(uint16 c) {
    BG_PALETTE_SUB[0] = c;
}

void Gameplay_UpdateLapDisplay(int currentLap, int totalLaps) {
    int x, y;

    // Current lap
    x = 0;
    y = 0;
    if (currentLap >= 0 && currentLap <= 9) {
        Gameplay_PrintDigit(BG_MAP_RAM_SUB(0), currentLap, x, y);
    }

    // Separator ":"
    x = 4;
    y = 0;
    Gameplay_PrintDigit(BG_MAP_RAM_SUB(0), 10, x, y);

    // Total laps
    x = 6;
    y = 0;
    if (totalLaps >= 0 && totalLaps <= 9) {
        Gameplay_PrintDigit(BG_MAP_RAM_SUB(0), totalLaps, x, y);
    } else if (totalLaps >= 10) {
        Gameplay_PrintDigit(BG_MAP_RAM_SUB(0), totalLaps / 10, x, y);
        x = 10;
        y = 0;
        Gameplay_PrintDigit(BG_MAP_RAM_SUB(0), totalLaps % 10, x, y);
    }
}
