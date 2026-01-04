#ifndef GAMEPLAY_LOGIC_H
#define GAMEPLAY_LOGIC_H
// HUGO------
#include <stdbool.h>

#include "../math/Car.h"
#include "../gameplay/items/Items.h"
#include "../core/game_types.h"
#include "../gameplay/wall_collision.h"

//=============================================================================
// Constants
//=============================================================================

#define MAX_CARS 8
#define MAX_CHECKPOINTS 16
#define QUAD_OFFSET 256
#define FINISH_DELAY_FRAMES (5 * 60)
//=============================================================================
// Enums
//=============================================================================

typedef enum { SinglePlayer, MultiPlayer } GameMode;


typedef enum {
    COUNTDOWN_3 = 0,
    COUNTDOWN_2,
    COUNTDOWN_1,
    COUNTDOWN_GO,
    COUNTDOWN_FINISHED
} CountdownState;

//=============================================================================
// Structs
//=============================================================================

typedef struct {
    Vec2 topLeft;
    Vec2 bottomRight;
} CheckpointBox;

typedef struct {
    bool raceStarted;


    GameMode gameMode;
    Map currentMap;

    int carCount;
    int playerIndex;  // The player on this DS
    Car cars[MAX_CARS];

    int totalLaps;

    int checkpointCount;
    CheckpointBox checkpoints[MAX_CHECKPOINTS];

    bool raceFinished;
    int finishDelayTimer;      // NEW: Countdown before showing menu
    int finalTimeMin;          // NEW: Store final time
    int finalTimeSec;
    int finalTimeMsec;
} RaceState;

//=============================================================================
// Lifecycle
//=============================================================================

void Race_Init(Map map, GameMode mode);
void Race_Tick(void);
void Race_Reset(void);
void Race_Stop(void);  // Renamed from stop_Race


void Race_UpdateCountdown(void);
bool Race_IsCountdownActive(void);
CountdownState Race_GetCountdownState(void);

bool Race_IsCompleted(void);
void Race_GetFinalTime(int* min, int* sec, int* msec);
void Race_MarkAsCompleted(int min, int sec, int msec);
void cleanup_pause_interrupt(void);
//=============================================================================
// State Queries (Read-Only Access)
//=============================================================================

const Car* Race_GetPlayerCar(void);    // Read-only access for rendering
const RaceState* Race_GetState(void);  // Read-only access for UI/rendering
bool Race_IsActive(void);              // Check if race is running
int Race_GetLapCount(void);            // Get total laps for current map
bool Race_CheckFinishLineCross(const Car* car);
void init_pause_interrupt(void);
void PauseISR(void);
void Race_SetCarGfx(int index, u16* gfx);
void Race_SetLoadedQuadrant(QuadrantID quad);
void Race_SetPlayerIndex(int playerIndex);
void UpdatePauseDebounce(void);
void Race_CountdownTick(void);

#endif  // GAMEPLAY_LOGIC_H
