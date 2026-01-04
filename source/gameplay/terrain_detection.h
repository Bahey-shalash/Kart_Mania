//=============================================================================
// terrain_detection.h
//=============================================================================
#ifndef TERRAIN_DETECTION_H
#define TERRAIN_DETECTION_H

#include <stdbool.h>

#include "../core/game_types.h"

//=============================================================================
// Public API
//=============================================================================

/** Check if position is on sand (off-track) */
bool Terrain_IsOnSand(int x, int y, QuadrantID quad);

#endif  // TERRAIN_DETECTION_H
