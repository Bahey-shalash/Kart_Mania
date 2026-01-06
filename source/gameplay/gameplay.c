#include "gameplay.h"

#include <nds.h>
#include <stdio.h>
#include <string.h>
// HUGO------
#include "items/items_api.h"
#include "../core/context.h"
#include "../core/game_constants.h"
#include "../core/game_types.h"
#include "gameplay_logic.h"
#include "kart_sprite.h"
#include "../network/multiplayer.h"
#include "numbers.h"
#include "../ui/play_again.h"
#include "scorching_sands_BC.h"
#include "scorching_sands_BL.h"
#include "scorching_sands_BR.h"
#include "scorching_sands_MC.h"
#include "scorching_sands_ML.h"
#include "scorching_sands_MR.h"
#include "scorching_sands_TC.h"
#include "scorching_sands_TL.h"
#include "scorching_sands_TR.h"
#include "wall_collision.h"
#include "../storage/storage_pb.h"
#include "../graphics/color.h"

#include "banana.h"
#include "bomb.h"
#include "green_shell.h"
#include "missile.h"
#include "oil_slick.h"
#include "red_shell.h"

//=============================================================================
// Constants
//=============================================================================
#define FINISH_DISPLAY_FRAMES 150  // 2.5 seconds at 60fps to show final time
// TODO: Make these kinds of variables dependent on the game frequency,
// so if we change the frequency, everything updates accordingly.

//! DEBUGGING FLAG
// #define console_on_debug  // Uncomment to enable debug console on sub screen
//! DEBUGGING FLAG
//=============================================================================
// Private State
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
// Quadrant Data
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
// Private Prototypes
//=============================================================================
static void configureGraphics(void);
static void configureBackground(void);
static void configureSprite(void);
static void freeSprites(void);
#ifdef console_on_debug
static void configureConsole(void);
#endif
static void loadQuadrant(QuadrantID quad);
static QuadrantID determineQuadrant(int x, int y);
static void renderCountdown(CountdownState state);
static void clearCountdownDisplay(void);
static void displayFinalTime(int min, int sec, int msec);
void printDigit(u16* map, int number, int x, int y);
#ifndef console_on_debug
static void loadItemDisplay_Sub(void);    // NEW
static void updateItemDisplay_Sub(void);  // NEW
#endif

//=============================================================================
// Timer Getters
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
// Public API - Initialization
//=============================================================================
void Graphical_Gameplay_initialize(void) {
    configureGraphics();
    configureBackground();
    // NOTE: configureSprite() is called from configureBackground() - we need to change
    // this

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

    // Load best time for this map
    Map selectedMap = GameContext_GetMap();

    // Check context to determine game mode
    GameContext* ctx = GameContext_Get();
    GameMode mode = ctx->isMultiplayerMode ? MultiPlayer : SinglePlayer;

    // Initialize race with appropriate mode
    if (!StoragePB_LoadBestTime(selectedMap, &bestRaceMin, &bestRaceSec,
                                &bestRaceMsec)) {
        bestRaceMin = -1;
        bestRaceSec = -1;
        bestRaceMsec = -1;
    }
    isNewRecord = false;

    // Clear any leftover display from previous race
#ifndef console_on_debug
    u16* map = BG_MAP_RAM_SUB(0);
    memset(map, 32, 32 * 32 * 2);
    changeColorDisp_Sub(BLACK);  // Reset to black
#endif

    Race_Init(selectedMap, mode);

    // NOW call configureSprite (remove it from wherever it's being called before)
    configureSprite();

    const Car* player = Race_GetPlayerCar();
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

//=============================================================================
// Public API - Update
//=============================================================================
GameState Gameplay_update(void) {
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
// Public API - VBlank (Graphics Update)
//=============================================================================
// void Gameplay_OnVBlank(void) {
//     // consoleClear();

//     const Car* player = Race_GetPlayerCar();
//     Vec2 velocityVec =
//         Vec2_Scale(Vec2_FromAngle(player->angle512), Car_GetSpeed(player));

//     if (Race_CheckFinishLineCross(player)) {
//         raceMin = 0;
//         raceSec = 0;
//         raceMsec = 0;

//         const RaceState* state = Race_GetState();
//         if (currentLap < state->totalLaps) {
//             currentLap++;
//         }
//     }

//     int carX = FixedToInt(player->position.x);
//     int carY = FixedToInt(player->position.y);

//     scrollX = carX - (SCREEN_WIDTH / 2);
//     scrollY = carY - (SCREEN_HEIGHT / 2);

//     if (scrollX < 0)
//         scrollX = 0;
//     if (scrollY < 0)
//         scrollY = 0;
//     if (scrollX > MAX_SCROLL_X)
//         scrollX = MAX_SCROLL_X;
//     if (scrollY > MAX_SCROLL_Y)
//         scrollY = MAX_SCROLL_Y;

//     QuadrantID newQuadrant = determineQuadrant(scrollX, scrollY);
//     if (newQuadrant != currentQuadrant) {
//         loadQuadrant(newQuadrant);
//         currentQuadrant = newQuadrant;
//         Race_SetLoadedQuadrant(newQuadrant);
//     }

//     int col = currentQuadrant % 3;
//     int row = currentQuadrant / 3;
//     BG_OFFSET[0].x = scrollX - (col * QUAD_OFFSET);
//     BG_OFFSET[0].y = scrollY - (row * QUAD_OFFSET);

//     /* DEBUG: Movement telemetry
//     int facingAngle = player->angle512;
//     int velocityAngle = Car_GetVelocityAngle(player);
//     Q16_8 speed = Car_GetSpeed(player);
//     bool moving = Car_IsMoving(player);
//     Vec2 velocityVec = Vec2_Scale(Vec2_FromAngle(player->angle512), speed);

//     consoleClear();
//     printf("=== CAR DEBUG ===\n");
//     printf("Facing:   %3d\n", facingAngle);
//     printf("Velocity: %3d\n", velocityAngle);
//     printf("Speed:    %d.%02d\n", (int)(speed >> 8), (int)(((speed & 0xFF) * 100) >>
//     8)); printf("Moving:   %s\n", moving ? "YES" : "NO");

//     if (moving) {
//         int angleDiff = (velocityAngle - facingAngle) & ANGLE_MASK;
//         if (angleDiff > 256) angleDiff -= 512;
//         printf("Diff:     %+4d", angleDiff);
//         if (angleDiff > 32 || angleDiff < -32) {
//             iprintf(" <!>");
//         }
//         printf("\n");
//     }
// */
//     /*
//         printf("\nPos: %d,%d\n", carX, carY);
//         printf("Vel: %d,%d\n", FixedToInt(velocityVec.x),
//         FixedToInt(velocityVec.y));

//         // Item debug
//         printf("\nItem: ");
//         switch (player->item) {
//             case ITEM_NONE:
//                 printf("NONE");
//                 break;
//             case ITEM_BANANA:
//                 printf("BANANA");
//                 break;
//             case ITEM_OIL:
//                 printf("OIL");
//                 break;
//             case ITEM_BOMB:
//                 printf("BOMB");
//                 break;
//             case ITEM_GREEN_SHELL:
//                 printf("GREEN_SHELL");
//                 break;
//             case ITEM_RED_SHELL:
//                 printf("RED_SHELL");
//                 break;
//             case ITEM_MISSILE:
//                 printf("MISSILE");
//                 break;
//             case ITEM_MUSHROOM:
//                 printf("MUSHROOM");
//                 break;
//             case ITEM_SPEEDBOOST:
//                 printf("SPEEDBOOST");
//                 break;
//             default:
//                 printf("???");
//                 break;
//         }
//         printf("\n");
//     */
//     //---------------
//     int dsAngle = -(player->angle512 << 6);
//     oamRotateScale(&oamMain, 0, dsAngle, (1 << 8), (1 << 8));
//     // Changed to make the sand interaction better (not too sure why -16 didnt work)
//     int screenX = carX - scrollX - 32;
//     int screenY = carY - scrollY - 32;

//     oamSet(&oamMain, 0, screenX, screenY, 0, 0, SpriteSize_32x32,
//            SpriteColorFormat_16Color, player->gfx, 0, true, false, false, false,
//            false);
//     Items_Render(scrollX, scrollY);
//     oamUpdate(&oamMain);
// }
void Gameplay_OnVBlank(void) {
    const Car* player = Race_GetPlayerCar();

    // Check if race is finished and showing final time
    const RaceState* state = Race_GetState();
    if (state->raceFinished && finishDisplayCounter < FINISH_DISPLAY_FRAMES) {
        // Display final time for 2.5 seconds
        displayFinalTime(totalRaceMin, totalRaceSec, totalRaceMsec);
        return;
    }

#ifdef console_on_debug
    // Debug: Print red shell positions
    consoleClear();
    printf("=== RED SHELL DEBUG ===\n");

    int itemCount = 0;
    const TrackItem* items = Items_GetActiveItems(&itemCount);

    int redShellCount = 0;
    for (int i = 0; i < itemCount; i++) {
        if (items[i].active && items[i].type == ITEM_RED_SHELL) {
            int x = FixedToInt(items[i].position.x);
            int y = FixedToInt(items[i].position.y);
            printf("Shell %d: (%d, %d)\n", redShellCount, x, y);
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

    // Check if countdown is active
    if (Race_IsCountdownActive()) {
        Race_UpdateCountdown();
        renderCountdown(Race_GetCountdownState());

        // Still update camera position during countdown
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
            Race_SetLoadedQuadrant(newQuadrant);
        }

        int col = currentQuadrant % 3;
        int row = currentQuadrant / 3;
        BG_OFFSET[0].x = scrollX - (col * QUAD_OFFSET);
        BG_OFFSET[0].y = scrollY - (row * QUAD_OFFSET);

        // CHANGED: Render all cars during countdown based on game mode
        if (state->gameMode == SinglePlayer) {
            // Single player - only render player's car
            int dsAngle = -(player->angle512 << 6);
            oamRotateScale(&oamMain, 0, dsAngle, (1 << 8), (1 << 8));
            int screenX = carX - scrollX - 16;
            int screenY = carY - scrollY - 16;

            oamSet(&oamMain, 41, screenX, screenY, 0, 0, SpriteSize_32x32,
                   SpriteColorFormat_16Color, player->gfx, 0, true, false, false,
                   false, false);
        } else {
            // MULTIPLAYER: Render ALL connected players during countdown
            for (int i = 0; i < state->carCount; i++) {
                int oamSlot = 41 + i;

                if (!Multiplayer_IsPlayerConnected(i)) {
                    // Hide disconnected players
                    oamSet(&oamMain, oamSlot, 0, 192, 0, 0, SpriteSize_32x32,
                           SpriteColorFormat_16Color, NULL, -1, true, false, false,
                           false, false);
                    continue;
                }

                const Car* car = &state->cars[i];
                int carWorldX = FixedToInt(car->position.x);
                int carWorldY = FixedToInt(car->position.y);
                int carScreenX = carWorldX - scrollX - 16;
                int carScreenY = carWorldY - scrollY - 16;

                // Set rotation for this car
                int dsAngle = -(car->angle512 << 6);
                oamRotateScale(&oamMain, i, dsAngle, (1 << 8), (1 << 8));

                // Check if on screen
                bool onScreen = (carScreenX >= -32 && carScreenX < SCREEN_WIDTH &&
                                 carScreenY >= -32 && carScreenY < SCREEN_HEIGHT);

                if (onScreen) {
                    oamSet(&oamMain, oamSlot, carScreenX, carScreenY, 0, 0,
                           SpriteSize_32x32, SpriteColorFormat_16Color, car->gfx, i,
                           true, false, false, false, false);
                } else {
                    // Off-screen cars
                    oamSet(&oamMain, oamSlot, -64, -64, 0, 0, SpriteSize_32x32,
                           SpriteColorFormat_16Color, car->gfx, i, true, false, false,
                           false, false);
                }
            }
        }

        oamUpdate(&oamMain);
        return;
    }

    if (!countdownCleared) {
        clearCountdownDisplay();
        countdownCleared = true;
    }

    if (Race_CheckFinishLineCross(player)) {
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
            // NEW: Hide item sprite when race finishes
            oamSet(&oamSub, 0, 0, 192, 0, 0, SpriteSize_32x32,
                   SpriteColorFormat_16Color, itemDisplayGfx_Sub, -1, true, false,
                   false, false, false);
            oamUpdate(&oamSub);
#endif
        }
    }

    int carCenterX = FixedToInt(player->position.x) + CAR_SPRITE_CENTER_OFFSET;
    int carCenterY = FixedToInt(player->position.y) + CAR_SPRITE_CENTER_OFFSET;

    scrollX = carCenterX - (SCREEN_WIDTH / 2);
    scrollY = carCenterY - (SCREEN_HEIGHT / 2);

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
        Race_SetLoadedQuadrant(newQuadrant);
    }

    int col = currentQuadrant % 3;
    int row = currentQuadrant / 3;
    BG_OFFSET[0].x = scrollX - (col * QUAD_OFFSET);
    BG_OFFSET[0].y = scrollY - (row * QUAD_OFFSET);

    //=========================================================================
    // Render cars based on game mode
    //=========================================================================
    if (state->gameMode == SinglePlayer) {
        // SINGLE PLAYER: Only render the player's car at OAM slot 41
        int carX = FixedToInt(player->position.x);
        int carY = FixedToInt(player->position.y);
        int screenX = carX - scrollX - 16;
        int screenY = carY - scrollY - 16;

        // Setup rotation for player (use affine matrix 0)
        int dsAngle = -(player->angle512 << 6);
        oamRotateScale(&oamMain, 0, dsAngle, (1 << 8), (1 << 8));

        // Render player sprite (OAM slot 41, affine matrix 0)
        // Priority 0 (highest) so kart appears above oil slicks and other items
        oamSet(&oamMain, 41, screenX, screenY, OBJPRIORITY_0, 0, SpriteSize_32x32,
               SpriteColorFormat_16Color, player->gfx, 0, true, false, false, false,
               false);
    } else {
        // MULTIPLAYER: Render all connected players
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
                // Priority 0 (highest) so karts appear above oil slicks and other items
                oamSet(&oamMain, oamSlot, carScreenX, carScreenY, OBJPRIORITY_0, 0,
                       SpriteSize_32x32, SpriteColorFormat_16Color, car->gfx, i, true,
                       false, false, false, false);
            } else {
                oamSet(&oamMain, oamSlot, -64, -64, OBJPRIORITY_0, 0, SpriteSize_32x32,
                       SpriteColorFormat_16Color, car->gfx, i, true, false, false,
                       false, false);
            }
        }
    }

    Items_Render(scrollX, scrollY);
#ifndef console_on_debug
    updateItemDisplay_Sub();
#endif
    oamUpdate(&oamMain);
}
void Gameplay_Cleanup(void) {
    freeSprites();
#ifndef console_on_debug
    if (itemDisplayGfx_Sub) {
        oamFreeGfx(&oamSub, itemDisplayGfx_Sub);
        itemDisplayGfx_Sub = NULL;
    }
#endif
}

//=============================================================================
// Display Functions
//=============================================================================

static void displayFinalTime(int min, int sec, int msec) {
#ifndef console_on_debug
    u16* map = BG_MAP_RAM_SUB(0);

    // Clear screen
    memset(map, 32, 32 * 32 * 2);

    // Display FINAL TIME at top (y = 8)
    int x = 0, y = 8;

    // Minutes
    printDigit(map, min / 10, x, y);
    x = 4;
    printDigit(map, min % 10, x, y);

    // Separator ":"
    x = 8;
    printDigit(map, 10, x, y);

    // Seconds
    x = 10;
    printDigit(map, sec / 10, x, y);
    x = 14;
    printDigit(map, sec % 10, x, y);

    // Separator "."
    x = 18;
    printDigit(map, 11, x, y);

    // Milliseconds (only first digit)
    x = 20;
    printDigit(map, msec / 100, x, y);

    // Display PERSONAL BEST below (y = 16)
    if (bestRaceMin >= 0) {
        // Show the best time
        x = 0;
        y = 16;

        // Minutes
        printDigit(map, bestRaceMin / 10, x, y);
        x = 4;
        printDigit(map, bestRaceMin % 10, x, y);

        // Separator ":"
        x = 8;
        printDigit(map, 10, x, y);

        // Seconds
        x = 10;
        printDigit(map, bestRaceSec / 10, x, y);
        x = 14;
        printDigit(map, bestRaceSec % 10, x, y);

        // Separator "."
        x = 18;
        printDigit(map, 11, x, y);

        // Milliseconds (only first digit)
        x = 20;
        printDigit(map, bestRaceMsec / 100, x, y);
    }

    // Set background color based on whether it's a new record
    if (isNewRecord) {
        changeColorDisp_Sub(DARK_GREEN);  // Green for new record!
    } else {
        changeColorDisp_Sub(BLACK);
    }
#endif
}

//=============================================================================
// Countdown Display Functions
//=============================================================================
static void renderCountdown(CountdownState state) {
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
            printDigit(map, 3, centerX, centerY);
            break;
        case COUNTDOWN_2:
            printDigit(map, 2, centerX, centerY);
            break;
        case COUNTDOWN_1:
            printDigit(map, 1, centerX, centerY);
            break;
        case COUNTDOWN_GO:
            printDigit(map, 0, centerX, centerY);
            break;
        case COUNTDOWN_FINISHED:
            break;
    }
#endif
}

static void clearCountdownDisplay(void) {
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
// Private Functions - Setup
//=============================================================================
static void configureGraphics(void) {
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

static void configureBackground(void) {
    Map selectedMap = GameContext_GetMap();
    if (selectedMap != ScorchingSands)
        return;

    // Priority 3 (lowest) so all sprites appear above the background
    BGCTRL[0] =
        BG_64x64 | BG_COLOR_256 | BG_MAP_BASE(0) | BG_TILE_BASE(1) | BG_PRIORITY(3);

#ifdef console_on_debug
    // Debug mode: Set up console
    configureConsole();
#else
    // Normal mode: Sub screen setup with numbers tileset
    BGCTRL_SUB[0] = BG_32x32 | BG_COLOR_256 | BG_MAP_BASE(0) | BG_TILE_BASE(1);
    swiCopy(numbersTiles, BG_TILE_RAM_SUB(1), numbersTilesLen);
    swiCopy(numbersPal, BG_PALETTE_SUB, numbersPalLen);
    BG_PALETTE_SUB[0] = BLACK;
    BG_PALETTE_SUB[255] = DARK_GRAY;  // neutral sub background
    memset(BG_MAP_RAM_SUB(0), 32, 32 * 32 * 2);
    updateChronoDisp_Sub(-1, -1, -1);
    loadItemDisplay_Sub();
#endif
}

static void configureSprite(void) {
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

static void freeSprites(void) {
    if (kartGfx) {
        oamFreeGfx(&oamMain, kartGfx);
        kartGfx = NULL;
    }
    Items_FreeGraphics();
}

#ifdef console_on_debug
// DEBUG Console setup (active for gameplay debug)
static void configureConsole(void) {
    // Use sub screen BG0 for console
    consoleInit(NULL, 0, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);
    printf("\x1b[2J");
    printf("=== KART DEBUG ===\n");
    printf("SELECT = exit\n\n");
}
#endif

//=============================================================================
// Sub Screen Item Display
//=============================================================================
#ifndef console_on_debug
static void loadItemDisplay_Sub(void) {
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

static void updateItemDisplay_Sub(void) {
    const Car* player = Race_GetPlayerCar();

    if (player->item == ITEM_NONE) {
        // Hide sprite when no item
        oamSet(&oamSub, 0, 0, 192, 0, 0, SpriteSize_32x32,  // CHANGED: to 32x32
               SpriteColorFormat_16Color, itemDisplayGfx_Sub, -1, true, false, false,
               false, false);
    } else {
        // Position in top-right corner
        int itemX = 220;  // Right side (256 - 16 - some margin)
        int itemY = 8;    // Top area

        // Determine which graphics to display and which palette to use
        const unsigned int* itemTiles = NULL;
        int paletteNum = 0;
        SpriteSize spriteSize = SpriteSize_16x16;  // NEW: Variable sprite size

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
                spriteSize = SpriteSize_16x32;  // CHANGED: Missile is 16x32
                break;
            case ITEM_OIL:
                itemTiles = oil_slickTiles;
                paletteNum = 7;
                spriteSize = SpriteSize_32x32;  // CHANGED: Oil is 32x32
                itemX = 208;  // Adjust position for larger sprite (256 - 32 - margin)
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
            // Copy the item graphics to sub screen sprite memory
            dmaCopy(itemTiles, itemDisplayGfx_Sub,
                    (player->item == ITEM_MISSILE) ? missileTilesLen
                    : (player->item == ITEM_OIL)   ? oil_slickTilesLen
                                                   : bananaTilesLen);

            // Display the sprite with appropriate size
            oamSet(&oamSub, 0, itemX, itemY, 0, paletteNum,
                   spriteSize,  // CHANGED: Use spriteSize variable
                   SpriteColorFormat_16Color, itemDisplayGfx_Sub, -1, false, false,
                   false, false, false);
        }
    }

    oamUpdate(&oamSub);
}
#endif

//=============================================================================
// Private Functions - Quadrant Management
//=============================================================================
static void loadQuadrant(QuadrantID quad) {
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

static QuadrantID determineQuadrant(int x, int y) {
    int col = (x < QUAD_OFFSET) ? 0 : (x < 2 * QUAD_OFFSET) ? 1 : 2;
    int row = (y < QUAD_OFFSET) ? 0 : (y < 2 * QUAD_OFFSET) ? 1 : 2;
    return (QuadrantID)(row * QUADRANT_GRID_SIZE + col);
}

//=============================================================================
// Sub-Screen Display Functions
//=============================================================================
void printDigit(u16* map, int number, int x, int y) {
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

void updateChronoDisp(u16* map, int min, int sec, int msec) {
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
    printDigit(map, number, x, y);
    x = 4;
    y = 8;
    if (min >= 0)
        number = min % 10;
    printDigit(map, number, x, y);

    // Separator ":"
    x = 8;
    y = 8;
    number = 10;
    printDigit(map, number, x, y);

    // Seconds
    number = sec;
    if (sec > 59)
        sec = number = -1;
    x = 10;
    y = 8;
    if (sec >= 0)
        number = sec / 10;
    printDigit(map, number, x, y);
    x = 14;
    y = 8;
    if (sec >= 0)
        number = sec % 10;
    printDigit(map, number, x, y);

    // Separator "."
    x = 18;
    y = 8;
    number = 11;
    printDigit(map, number, x, y);

    // Milliseconds
    number = msec;
    if (msec > 999)
        msec = number = -1;
    x = 20;
    y = 8;
    if (msec >= 0)
        number = msec / 100;
    printDigit(map, number, x, y);
    x = 24;
    y = 8;
    if (msec >= 0)
        number = (msec % 100) / 10;
    printDigit(map, number, x, y);
    x = 28;
    y = 8;
    if (msec >= 0)
        number = (msec % 10) % 10;
    printDigit(map, number, x, y);
}

void updateChronoDisp_Sub(int min, int sec, int msec) {
    updateChronoDisp(BG_MAP_RAM_SUB(0), min, sec, msec);
}

void changeColorDisp_Sub(uint16 c) {
    BG_PALETTE_SUB[0] = c;
}

void updateLapDisp_Sub(int currentLap, int totalLaps) {
    int x, y;

    // Current lap
    x = 0;
    y = 0;
    if (currentLap >= 0 && currentLap <= 9) {
        printDigit(BG_MAP_RAM_SUB(0), currentLap, x, y);
    }

    // Separator ":"
    x = 4;
    y = 0;
    printDigit(BG_MAP_RAM_SUB(0), 10, x, y);

    // Total laps
    x = 6;
    y = 0;
    if (totalLaps >= 0 && totalLaps <= 9) {
        printDigit(BG_MAP_RAM_SUB(0), totalLaps, x, y);
    } else if (totalLaps >= 10) {
        printDigit(BG_MAP_RAM_SUB(0), totalLaps / 10, x, y);
        x = 10;
        y = 0;
        printDigit(BG_MAP_RAM_SUB(0), totalLaps % 10, x, y);
    }
}
