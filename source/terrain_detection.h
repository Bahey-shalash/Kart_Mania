//=============================================================================
// terrain_detection
//=============================================================================
#ifndef TERRAIN_DETECTION_H
#define TERRAIN_DETECTION_H
// HUGO------
#include <stdbool.h>

#include "game_types.h"

// Check if position is on sand (off-track)
bool Terrain_IsOnSand(int x, int y, QuadrantID quad);

#endif  // TERRAIN_DETECTION_H
