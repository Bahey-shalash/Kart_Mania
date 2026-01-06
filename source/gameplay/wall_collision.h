/**
 * File: wall_collision.h
 * ----------------------
 * Description: Wall collision detection for racing track boundaries. Detects
 *              collisions between circular kart hitboxes and axis-aligned wall
 *              segments. Provides collision normals for bounce physics. Each
 *              quadrant has pre-defined wall geometry in global coordinates.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 06.01.2026
 */

#ifndef WALL_COLLISION_H
#define WALL_COLLISION_H

#include <stdbool.h>

#include "../core/game_types.h"

//=============================================================================
// PUBLIC TYPES
//=============================================================================

/**
 * Wall segment orientation types for axis-aligned boundaries.
 */
typedef enum {
    WALL_HORIZONTAL,  // Constant Y coordinate, X range [min, max]
    WALL_VERTICAL     // Constant X coordinate, Y range [min, max]
} WallType;

/**
 * Axis-aligned wall segment with fixed coordinate and range.
 *
 * For WALL_HORIZONTAL:
 *   - fixed_coord is the Y coordinate
 *   - min_range/max_range define X extent
 *
 * For WALL_VERTICAL:
 *   - fixed_coord is the X coordinate
 *   - min_range/max_range define Y extent
 */
typedef struct {
    WallType type;        // Orientation: horizontal or vertical
    int fixed_coord;      // Fixed axis coordinate (X for vertical, Y for horizontal)
    int min_range;        // Start of variable axis range
    int max_range;        // End of variable axis range
} WallSegment;

/**
 * Collection of wall segments for a single quadrant.
 */
typedef struct {
    const WallSegment* segments;  // Array of wall segments
    int count;                     // Number of segments
} QuadrantWalls;

//=============================================================================
// PUBLIC API
//=============================================================================

/**
 * Function: Wall_CheckCollision
 * -----------------------------
 * Checks if a circular kart hitbox collides with any walls in the quadrant.
 *
 * Parameters:
 *   carX      - Kart center X coordinate in world space
 *   carY      - Kart center Y coordinate in world space
 *   carRadius - Kart collision radius in pixels
 *   quad      - Current quadrant ID
 *
 * Returns:
 *   true  - Collision detected with at least one wall
 *   false - No collision or invalid quadrant
 */
bool Wall_CheckCollision(int carX, int carY, int carRadius, QuadrantID quad);

/**
 * Function: Wall_GetCollisionNormal
 * ----------------------------------
 * Determines the collision normal vector for the nearest wall.
 *
 * Finds the closest wall to the kart position and returns a unit normal
 * vector pointing away from the wall. Used for bounce physics.
 *
 * Parameters:
 *   carX - Kart center X coordinate in world space
 *   carY - Kart center Y coordinate in world space
 *   quad - Current quadrant ID
 *   nx   - Output: X component of normal vector (-1, 0, or 1)
 *   ny   - Output: Y component of normal vector (-1, 0, or 1)
 *
 * Normal Directions:
 *   - Horizontal walls: ny = ±1, nx = 0 (points up or down)
 *   - Vertical walls:   nx = ±1, ny = 0 (points left or right)
 */
void Wall_GetCollisionNormal(int carX, int carY, QuadrantID quad, int* nx, int* ny);

#endif  // WALL_COLLISION_H