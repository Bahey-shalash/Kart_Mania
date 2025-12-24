#ifndef GAMEPLAY_LOGIC_H
#define GAMEPLAY_LOGIC_H

#include <stdbool.h>

#include "Car.h"
#include "game_types.h"

//=============================================================================
// Constants
//=============================================================================
#define MAX_CARS 8
#define MAX_CHECKPOINTS 16

static const int MapLaps[] = {
    [NONEMAP] = 0,
    [ScorchingSands] = 10,
    [AlpinRush] = 10,
    [NeonCircuit] = 10,
};

//=============================================================================
// Enums
//=============================================================================
typedef enum { SinglePlayer, MultiPlayer } GameMode;

//=============================================================================
// Structs
//=============================================================================
typedef struct {
    Vec2 topLeft;
    Vec2 bottomRight;
} CheckpointBox;

typedef struct {
    bool raceStarted;
    bool raceFinished;

    GameMode gameMode;
    Map currentMap;

    int carCount;
    int playerIndex;  // the guy playing on his own DS
    Car cars[MAX_CARS];

    int totalLaps;

    int checkpointCount;
    CheckpointBox checkpoints[MAX_CHECKPOINTS];
} RaceState;

//=============================================================================
// Lifecycle
//=============================================================================
void Race_Init(Map map, GameMode mode);
void Race_Tick(void);
void Race_Reset(void);
void stop_Race(void);

//=============================================================================
// Input
//=============================================================================
void PlayerInput_Init(void);
void PlayerInput_Apply_ISR(void);
void DisableInput(void);

//=============================================================================
// Accessors
//=============================================================================
Car* Race_GetPlayerCar(void);
RaceState* Race_GetState(void);

#endif  // GAMEPLAY_LOGIC_H