#ifndef GAMEPLAY_H
#define GAMEPLAY_H
#include "game_types.h"

//=============================================================================
// Public Constants
//=============================================================================
#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 192
#define MAP_SIZE 1024
#define QUADRANT_SIZE 512
#define QUAD_OFFSET 256
#define MAX_SCROLL_X (MAP_SIZE - SCREEN_WIDTH)
#define MAX_SCROLL_Y (MAP_SIZE - SCREEN_HEIGHT)

//=============================================================================
// Public API
//=============================================================================
void Graphical_Gameplay_initialize(void);
GameState Gameplay_update(void);
void Gameplay_OnVBlank(void);

// Timer getters
int Gameplay_GetRaceMin(void);
int Gameplay_GetRaceSec(void);
int Gameplay_GetRaceMsec(void);
int Gameplay_GetCurrentLap(void);
void Gameplay_IncrementTimer(void);

// Sub-screen display updates
void updateLapDisp_Sub(int currentLap, int totalLaps);
void updateChronoDisp_Sub(int min, int sec, int msec);
void changeColorDisp_Sub(uint16 c);

// Play Again screen getters 
bool Gameplay_IsPlayAgainActive(void);


// Item rendering
void Gameplay_RenderItems(int scrollX, int scrollY);

#endif  // GAMEPLAY_H