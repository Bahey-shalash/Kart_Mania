/**
 * File: terrain_detection.h
 * -------------------------
 * Description: Terrain type detection for gameplay physics. Determines surface
 *              type (track vs sand) at specific world coordinates by sampling
 *              the background tilemap and analyzing pixel colors. Used to apply
 *              terrain-specific physics effects (speed reduction on sand).
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 05.01.2026
 */

#ifndef TERRAIN_DETECTION_H
#define TERRAIN_DETECTION_H

#include <stdbool.h>

#include "../core/game_types.h"

//=============================================================================
// PUBLIC API
//=============================================================================

/**
 * Function: Terrain_IsOnSand
 * --------------------------
 * Determines if a world position is on sand terrain (off-track).
 *
 * Algorithm:
 *   1. Converts world coordinates to quadrant-local coordinates
 *   2. Maps local coordinates to tilemap position
 *   3. Reads tile index from appropriate screen map base
 *   4. Samples pixel color from tile data
 *   5. Compares pixel color against known track/sand colors
 *
 * Parameters:
 *   x    - World X coordinate in pixels
 *   y    - World Y coordinate in pixels
 *   quad - Quadrant ID (identifies which 256×256 section of map)
 *
 * Returns:
 *   true  - Position is on sand (off-track, applies speed penalty)
 *   false - Position is on track or out of bounds
 *
 * Color Detection:
 *   - Gray track colors (12,12,12) and (14,14,14) in 5-bit RGB → NOT sand
 *   - Sand colors (20,18,12) and (22,20,14) in 5-bit RGB → IS sand
 *   - Uses 1-unit tolerance in 5-bit space for each channel
 *
 * Performance: Single palette lookup per call (~O(1))
 */
bool Terrain_IsOnSand(int x, int y, QuadrantID quad);

#endif  // TERRAIN_DETECTION_H
