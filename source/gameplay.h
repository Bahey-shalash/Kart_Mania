#ifndef GAMEPLAY_H
#define GAMEPLAY_H
#include "game_types.h"

//=============================================================================
// Public constants
//=============================================================================
#define SCROLL_SPEED 2
#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 192
#define MAP_SIZE 1024
#define QUADRANT_SIZE 512
#define QUAD_OFFSET 256

#define MAX_SCROLL_X (MAP_SIZE - SCREEN_WIDTH)
#define MAX_SCROLL_Y (MAP_SIZE - SCREEN_HEIGHT)

void Graphical_Gameplay_initialize(void);
GameState Gameplay_update(void);
#endif  // GAMEPLAY_H