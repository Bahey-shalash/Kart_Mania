//=============================================================================
// wall_collision.h
//=============================================================================
#ifndef WALL_COLLISION_H
#define WALL_COLLISION_H
// HUGO------
#include <stdbool.h>

#include "../core/game_types.h"

#define CAR_RADIUS 16  // Collision radius for 32x32 sprite

// Wall segment types
typedef enum {
    WALL_HORIZONTAL,  // Constant Y, X range
    WALL_VERTICAL     // Constant X, Y range
} WallType;

typedef struct {
    WallType type;
    int fixed_coord;  // X for vertical, Y for horizontal
    int min_range;    // Min X/Y
    int max_range;    // Max X/Y
} WallSegment;

typedef struct {
    const WallSegment* segments;
    int count;
} QuadrantWalls;

// Check if car collides with walls in current quadrant
bool Wall_CheckCollision(int carX, int carY, int carRadius, QuadrantID quad);

// Get collision normal for bouncing
void Wall_GetCollisionNormal(int carX, int carY, QuadrantID quad, int* nx, int* ny);

#endif  // WALL_COLLISION_H