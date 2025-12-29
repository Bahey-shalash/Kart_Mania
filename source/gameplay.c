#include "gameplay.h"

#include <nds.h>
#include <stdio.h>
#include <string.h>

#include "Items.h"
#include "context.h"
#include "game_constants.h"
#include "game_types.h"
#include "gameplay_logic.h"
#include "kart_sprite.h"
#include "numbers.h"
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
// Play Again screen state
static bool playAgainScreenActive = false;
static bool playAgainYesSelected = true;  // Default to YES
static bool countdownCleared = false; 

static int totalRaceMin = 0;
static int totalRaceSec = 0;
static int totalRaceMsec = 0;
//=============================================================================
// Quadrant Data
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
// Private Prototypes
//=============================================================================
static void configureGraphics(void);
static void configureBackground(void);
static void configureSprite(void);
static void loadQuadrant(QuadrantID quad);
static QuadrantID determineQuadrant(int x, int y);
static void renderCountdown(CountdownState state);
static void clearCountdownDisplay(void);

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
// Play Again Screen Functions
//=============================================================================

// Getter functions for Play Again state
bool Gameplay_IsPlayAgainActive(void) {
    return playAgainScreenActive;
}

bool Gameplay_IsYesSelected(void) {
    return playAgainYesSelected;
}

// Make these non-static so they can be called from timer.c
void renderPlayAgainScreen(int min, int sec, int msec, bool yesSelected) {
    u16* map = BG_MAP_RAM_SUB(0);
    
    // Clear entire screen
    memset(map, 32, 32 * 32 * 2);
    
    // Display final time at y=6 (centered)
    // Format: MM:SS.mmm
    int x, y;
    
    // Minutes
    x = 8;
    y = 6;
    printDigit(map, min / 10, x, y);
    x = 12;
    printDigit(map, min % 10, x, y);
    
    // Separator ":"
    x = 16;
    printDigit(map, 10, x, y);
    
    // Seconds
    x = 18;
    printDigit(map, sec / 10, x, y);
    x = 22;
    printDigit(map, sec % 10, x, y);
    
    // Separator "."
    x = 26;
    printDigit(map, 11, x, y);
    
    // Milliseconds
    x = 28;
    printDigit(map, msec / 100, x, y);
    
    // Display "YES" and "NO" options at bottom
    // YES at position (x=6, y=18)
    x = 6;
    y = 18;
    printDigit(map, 1, x, y);  // Placeholder for YES
    
    // NO at position (x=22, y=18)
    x = 22;
    y = 18;
    printDigit(map, 0, x, y);  // Placeholder for NO
    
    // Highlight selected option by changing background color
    if (yesSelected) {
        changeColorDisp_Sub(ARGB16(1, 0, 31, 0));  // Green background
    } else {
        changeColorDisp_Sub(ARGB16(1, 31, 0, 0));  // Red background
    }
}

void clearPlayAgainScreen(void) {
    u16* map = BG_MAP_RAM_SUB(0);
    memset(map, 32, 32 * 32 * 2);
    changeColorDisp_Sub(ARGB16(1, 31, 31, 0));  // Reset to yellow
}

//=============================================================================
// Public API - Initialization
//=============================================================================
void Graphical_Gameplay_initialize(void) {
    configureGraphics();
    configureBackground();
    configureSprite();
    raceMin = 0;
    raceSec = 0;
    raceMsec = 0;
    currentLap = 1;
    playAgainScreenActive = false;
    playAgainYesSelected = true;  
    countdownCleared = false;  

        // Clear any leftover display from previous race
    u16* map = BG_MAP_RAM_SUB(0);
    memset(map, 32, 32 * 32 * 2);
    changeColorDisp_Sub(ARGB16(1, 31, 31, 0));  // Reset to yellow

    Map selectedMap = GameContext_GetMap();
    Race_Init(selectedMap, SinglePlayer);

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
        clearPlayAgainScreen();
        return HOME_PAGE;
    }

    const RaceState* state = Race_GetState();
    
    // Check if race finished and delay expired
    if (state->raceFinished && state->finishDelayTimer == 0) {
        playAgainScreenActive = true;
        
        // Handle Play Again input
        if (keysdown & KEY_LEFT) {
            playAgainYesSelected = true;
        } else if (keysdown & KEY_RIGHT) {
            playAgainYesSelected = false;
        }
        
        // Confirm selection with A button
        if (keysdown & KEY_A) {
            if (playAgainYesSelected) {
                // Play Again - Reset race
                clearPlayAgainScreen();
                playAgainScreenActive = false;
                playAgainYesSelected = true;
                raceMin = 0;
                raceSec = 0;
                raceMsec = 0;
                totalRaceMin = 0;      // ADD THIS
                totalRaceSec = 0;       // ADD THIS
                totalRaceMsec = 0;      // ADD THIS
                currentLap = 1;
                Race_Reset();
                
                // Reinitialize graphics
                const Car* player = Race_GetPlayerCar();
                scrollX = FixedToInt(player->position.x) - (SCREEN_WIDTH / 2);
                scrollY = FixedToInt(player->position.y) - (SCREEN_HEIGHT / 2);
                
                if (scrollX < 0) scrollX = 0;
                if (scrollY < 0) scrollY = 0;
                if (scrollX > MAX_SCROLL_X) scrollX = MAX_SCROLL_X;
                if (scrollY > MAX_SCROLL_Y) scrollY = MAX_SCROLL_Y;
                
                currentQuadrant = determineQuadrant(scrollX, scrollY);
                loadQuadrant(currentQuadrant);
                
                return GAMEPLAY;
            } else {
                // Go to Home Page
                Race_Stop();
                clearPlayAgainScreen();
                return HOME_PAGE;
            }
        }
    }

    return GAMEPLAY;
}
//=============================================================================
// Public API - VBlank (Graphics Update)
//=============================================================================
void Gameplay_OnVBlank(void) {
    const Car* player = Race_GetPlayerCar();
    
    // Check if countdown is active
    if (Race_IsCountdownActive()) {
        Race_UpdateCountdown();
        renderCountdown(Race_GetCountdownState());
        
        // Still update camera position during countdown
        int carX = FixedToInt(player->position.x);
        int carY = FixedToInt(player->position.y);
        
        scrollX = carX - (SCREEN_WIDTH / 2);
        scrollY = carY - (SCREEN_HEIGHT / 2);
        
        if (scrollX < 0) scrollX = 0;
        if (scrollY < 0) scrollY = 0;
        if (scrollX > MAX_SCROLL_X) scrollX = MAX_SCROLL_X;
        if (scrollY > MAX_SCROLL_Y) scrollY = MAX_SCROLL_Y;
        
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
        
        // Render car sprite even during countdown
        int dsAngle = -(player->angle512 << 6);
        oamRotateScale(&oamMain, 0, dsAngle, (1 << 8), (1 << 8));
        int screenX = carX - scrollX - 32;
        int screenY = carY - scrollY - 32;
        
        oamSet(&oamMain, 0, screenX, screenY, 0, 0, SpriteSize_32x32,
               SpriteColorFormat_16Color, player->gfx, 0, true, false, false, false, false);
        
        oamUpdate(&oamMain);
        return;
    }
    
    if (!countdownCleared) {
        clearCountdownDisplay();
        countdownCleared = true;
    }

    if (Race_CheckFinishLineCross(player)) {
        const RaceState* state = Race_GetState();
        
        if (currentLap < state->totalLaps) {
            // Normal lap completion - reset LAP timer (but total keeps running)
            currentLap++;
            raceMin = 0;
            raceSec = 0;
            raceMsec = 0;
        } else {
            // RACE COMPLETED! (finished final lap)
            // Pass TOTAL race time, not lap time
            Race_MarkAsCompleted(totalRaceMin, totalRaceSec, totalRaceMsec);
        }
    }

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

    int dsAngle = -(player->angle512 << 6);
    oamRotateScale(&oamMain, 0, dsAngle, (1 << 8), (1 << 8));
    int screenX = carX - scrollX - 32;
    int screenY = carY - scrollY - 32;

    oamSet(&oamMain, 0, screenX, screenY, 0, 0, SpriteSize_32x32,
           SpriteColorFormat_16Color, player->gfx, 0, true, false, false, false,
           false);
    Items_Render(scrollX, scrollY);
    oamUpdate(&oamMain);
}

//=============================================================================
// Countdown Display Functions
//=============================================================================
static void renderCountdown(CountdownState state) {
    u16* map = BG_MAP_RAM_SUB(0);
    
    // Clear previous countdown display
    for (int i = 16; i < 24; i++) {
        for (int j = 12; j < 20; j++) {
            map[i * 32 + j] = 32;  // Empty tile
        }
    }
    
    // Center position for large countdown numbers
    int centerX = 14;  // Centered horizontally (32 tiles / 2 - 2 for number width)
    int centerY = 10;  // Lower on screen
    
    switch (state) {
        case COUNTDOWN_3:
            // Display large "3"
            printDigit(map, 3, centerX, centerY);
            break;
            
        case COUNTDOWN_2:
            // Display large "2"
            printDigit(map, 2, centerX, centerY);
            break;
            
        case COUNTDOWN_1:
            // Display large "1"
            printDigit(map, 1, centerX, centerY);
            break;
    
        case COUNTDOWN_GO:
            // Display "GO!" using custom tiles
            printDigit(map, 0, centerX - 2, centerY);
            // You could add more tiles here for a proper "GO" display
            break;
            
        case COUNTDOWN_FINISHED:
            // Don't display anything
            break;
    }
}

static void clearCountdownDisplay(void) {
    u16* map = BG_MAP_RAM_SUB(0);
    
    // Clear the countdown area
    for (int i = 16; i < 24; i++) {
        for (int j = 12; j < 20; j++) {
            map[i * 32 + j] = 32;  // Empty tile
        }
    }
}

//=============================================================================
// Private Functions - Setup
//=============================================================================
static void configureGraphics(void) {
    REG_DISPCNT = MODE_0_2D | DISPLAY_BG0_ACTIVE | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D;
    VRAM_A_CR = VRAM_ENABLE | VRAM_A_MAIN_BG;
    VRAM_B_CR = VRAM_ENABLE | VRAM_B_MAIN_SPRITE;

    REG_DISPCNT_SUB = MODE_0_2D | DISPLAY_BG0_ACTIVE;
    VRAM_C_CR = VRAM_ENABLE | VRAM_C_SUB_BG;
}

static void configureBackground(void) {
    Map selectedMap = GameContext_GetMap();
    if (selectedMap != ScorchingSands)
        return;

    BGCTRL[0] = BG_64x64 | BG_COLOR_256 | BG_MAP_BASE(0) | BG_TILE_BASE(1);
    dmaCopy(scorching_sands_TLPal, BG_PALETTE, scorching_sands_TLPalLen);

    // Sub screen setup with numbers tileset
    BGCTRL_SUB[0] = BG_32x32 | BG_COLOR_256 | BG_MAP_BASE(0) | BG_TILE_BASE(1);
    swiCopy(numbersTiles, BG_TILE_RAM_SUB(1), numbersTilesLen);
    swiCopy(numbersPal, BG_PALETTE_SUB, numbersPalLen);
    BG_PALETTE_SUB[0] = ARGB16(1, 31, 31, 0);
    BG_PALETTE_SUB[1] = ARGB16(1, 0, 0, 0);
    memset(BG_MAP_RAM_SUB(0), 32, 32 * 32 * 2);
}

static void configureSprite(void) {
    oamInit(&oamMain, SpriteMapping_1D_32, false);

    // Allocate sprite graphics for player kart (16-color)
    u16* kartGfx =
        oamAllocateGfx(&oamMain, SpriteSize_32x32, SpriteColorFormat_16Color);

    // Load kart palette to slot 0
    dmaCopy(kart_spritePal, SPRITE_PALETTE, kart_spritePalLen);
    dmaCopy(kart_spriteTiles, kartGfx, kart_spriteTilesLen);

    // Set the graphics pointer on the player car
    Race_SetCarGfx(0, kartGfx);

    // Load item graphics (palettes 1-7)
    Items_LoadGraphics();
}

//=============================================================================
// Private Functions - Quadrant Management
//=============================================================================
static void loadQuadrant(QuadrantID quad) {
    const QuadrantData* data = &quadrantData[quad];

    dmaCopy(data->tiles, BG_TILE_RAM(1), data->tilesLen);

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

    x = 0;
    y = 0;
    if (currentLap >= 0 && currentLap <= 9) {
        printDigit(BG_MAP_RAM_SUB(0), currentLap, x, y);
    }

    x = 4;
    y = 0;

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

#ifdef DEBUG
    DEBUG: Movement telemetry
    int facingAngle = player->angle512;
    int velocityAngle = Car_GetVelocityAngle(player);
    Q16_8 speed = Car_GetSpeed(player);
    bool moving = Car_IsMoving(player);
    Vec2 velocityVec = Vec2_Scale(Vec2_FromAngle(player->angle512), speed);

    consoleClear();
    printf("=== CAR DEBUG ===\n");
    printf("Facing:   %3d\n", facingAngle);
    printf("Velocity: %3d\n", velocityAngle);
    printf("Speed:    %d.%02d\n", (int)(speed >> 8), (int)(((speed & 0xFF) * 100) >>
    8)); printf("Moving:   %s\n", moving ? "YES" : "NO");

    if (moving) {
        int angleDiff = (velocityAngle - facingAngle) & ANGLE_MASK;
        if (angleDiff > 256) angleDiff -= 512;
        printf("Diff:     %+4d", angleDiff);
        if (angleDiff > 32 || angleDiff < -32) {
            iprintf(" <!>");
        }
        printf("\n");
    }

    printf("\nPos: %d,%d\n", carX, carY);
    printf("Vel: %d,%d\n", FixedToInt(velocityVec.x), FixedToInt(velocityVec.y));

    // Item debug
    printf("\nItem: ");
    switch (player->item) {
        case ITEM_NONE:
            printf("NONE");
            break;
        case ITEM_BANANA:
            printf("BANANA");
            break;
        case ITEM_OIL:
            printf("OIL");
            break;
        case ITEM_BOMB:
            printf("BOMB");
            break;
        case ITEM_GREEN_SHELL:
            printf("GREEN_SHELL");
            break;
        case ITEM_RED_SHELL:
            printf("RED_SHELL");
            break;
        case ITEM_MISSILE:
            printf("MISSILE");
            break;
        case ITEM_MUSHROOM:
            printf("MUSHROOM");
            break;
        case ITEM_SPEEDBOOST:
            printf("SPEEDBOOST");
            break;
        default:
            printf("???");
            break;
    }
    printf("\n");
    #endif




#ifdef DEBUG
    //Reserve BG1 on sub for console (4bpp, separate tile/map blocks)
    BGCTRL_SUB[1] = BG_32x32 | BG_COLOR_16 | BG_MAP_BASE(31) | BG_TILE_BASE(2);
#endif

#ifdef DEBUG
/*
// DEBUG Console setup (active for gameplay debug)
static void configureConsole(void) {
    // Use sub screen BG1 so we don't clobber gameplay BGs/palettes
    consoleInit(NULL, 1, BgType_Text4bpp, BgSize_T_256x256, 31, 2, false, true);
    printf("\x1b[2J");
    printf("=== KART DEBUG ===\n");
    printf("SELECT = exit\n\n");
}
*/
#endif

