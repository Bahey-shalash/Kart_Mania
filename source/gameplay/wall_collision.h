//=============================================================================
// wall_collision.h
//=============================================================================
#ifndef WALL_COLLISION_H
#define WALL_COLLISION_H

#include <stdbool.h>

#include "../core/game_types.h"
#include "../core/game_constants.h"

//=============================================================================
// Public API
//=============================================================================

// Check if car collides with walls in current quadrant
bool Wall_CheckCollision(int carX, int carY, int carRadius, QuadrantID quad);

// Get collision normal for bouncing
void Wall_GetCollisionNormal(int carX, int carY, QuadrantID quad, int* nx, int* ny);

#endif  // WALL_COLLISION_H