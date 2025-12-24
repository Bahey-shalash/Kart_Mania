#ifndef GAMEPLAY_LOGIC_H
#define GAMEPLAY_LOGIC_H

#include <stdbool.h>
#include <stdint.h>

#include "Car.h"
#include "game_types.h"
#include "vect2.h"

#define MAX_CARS 8

typedef enum { GAME_MODE_SINGLEPLAYER = 0, GAME_MODE_MULTIPLAYER = 1 } GameMode;

typedef struct {
    bool accel;
    bool brake;
    int16_t steerDelta;  // in your angle512 units (+/-), per tick
} CarInput;

/* Read-only track definition (per map) */
typedef struct {
    const Vec2* checkpoints;  // pointer to checkpoint array
    int checkpointCount;

    Vec2 spawnPos[MAX_CARS];  // optional: predefined spawns
    int spawnCount;           // <= MAX_CARS

    int totalLaps;
} TrackDef;

/* Runtime race state */
typedef struct {
    bool raceStarted;
    bool raceFinished;

    GameMode gameMode;

    TrackDef track;

    int carCount;    // total cars in race (humans + AI)
    int humanCount;  // number of human-controlled cars

    Car cars[MAX_CARS];

    // Progress tracking (one per car)
    int currentLap[MAX_CARS];
    int nextCheckpoint[MAX_CARS];

    // Finish order
    int finishedOrder[MAX_CARS];  // store indices (0..carCount-1)
    int finishedCount;

    // Inputs (set each tick)
    CarInput input[MAX_CARS];

} RaceState;

// Global access (or you can pass RaceState* around instead)
RaceState* Race_Get(void);

// Lifecycle
void Race_Init(Map map, int lapCount, GameMode gamemode);
void Race_Reset(void);

// Simulation
void Race_SetInput(int carIndex, CarInput in);
void Race_Tick(void);  // call once per frame (VBlank) or fixed timestep

// Queries
const Car* Race_GetCar(int index);
int Race_GetCarCount(void);
bool Race_IsFinished(void);

// Ranking: position 0 = 1st, 1 = 2nd, ...
// Returns car index, or -1 if not available yet.
int Race_GetRankingIndex(int position);

#endif  // GAMEPLAY_LOGIC_H