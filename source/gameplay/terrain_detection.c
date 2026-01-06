/**
 * File: terrain_detection.c
 * -------------------------
 * Description: Implementation of terrain type detection for gameplay physics.
 *              Samples background tilemap pixels to determine surface type
 *              (track vs sand) at specific world coordinates. Uses color-based
 *              classification with 5-bit RGB values and tolerance matching.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 05.01.2026
 */

#include "terrain_detection.h"

#include <nds.h>
#include <stdlib.h>

#include "../core/game_constants.h"

//=============================================================================
// PRIVATE CONSTANTS
//=============================================================================

// Note: QUAD_OFFSET, terrain color constants (GRAY_*, SAND_*), and
//       COLOR_TOLERANCE_5BIT moved to game_constants.h

//=============================================================================
// PRIVATE HELPER FUNCTIONS
//=============================================================================

/**
 * Checks if a 5-bit RGB color matches a target color within tolerance.
 *
 * Parameters:
 *   r5, g5, b5           - Color to test (5-bit values, 0-31)
 *   targetR5, targetG5, targetB5 - Target color (5-bit values, 0-31)
 *   tolerance            - Maximum difference per channel
 *
 * Returns: true if all channels within tolerance, false otherwise
 */
static inline bool Terrain_ColorMatches5bit(int r5, int g5, int b5, int targetR5,
                                             int targetG5, int targetB5, int tolerance) {
    return (abs(r5 - targetR5) <= tolerance && abs(g5 - targetG5) <= tolerance &&
            abs(b5 - targetB5) <= tolerance);
}

/**
 * Checks if a 5-bit RGB color represents gray track surface.
 *
 * Checks against two known gray shades:
 *   - Main gray: (12, 12, 12)
 *   - Light gray: (14, 14, 14)
 *
 * Parameters:
 *   r5, g5, b5 - Color to test (5-bit values, 0-31)
 *
 * Returns: true if color matches gray track, false otherwise
 */
static inline bool Terrain_IsGrayTrack5bit(int r5, int g5, int b5) {
    // Check for main gray (12,12,12)
    if (Terrain_ColorMatches5bit(r5, g5, b5, GRAY_MAIN_R5, GRAY_MAIN_G5, GRAY_MAIN_B5,
                                  COLOR_TOLERANCE_5BIT))
        return true;

    // Check for light gray (14,14,14)
    if (Terrain_ColorMatches5bit(r5, g5, b5, GRAY_LIGHT_R5, GRAY_LIGHT_G5, GRAY_LIGHT_B5,
                                  COLOR_TOLERANCE_5BIT))
        return true;

    return false;
}

//=============================================================================
// PUBLIC API
//=============================================================================

bool Terrain_IsOnSand(int x, int y, QuadrantID quad) {
    // Convert quadrant ID to grid coordinates
    int col = quad % QUADRANT_GRID_SIZE;
    int row = quad / QUADRANT_GRID_SIZE;

    // Calculate quadrant's world offset
    int quadStartX = col * QUAD_OFFSET;
    int quadStartY = row * QUAD_OFFSET;

    // Convert to quadrant-local coordinates
    int localX = x - quadStartX;
    int localY = y - quadStartY;

    // Bounds check: ensure position is within quadrant
    if (localX < 0 || localX >= QUAD_SIZE_DOUBLE || localY < 0 ||
        localY >= QUAD_SIZE_DOUBLE)
        return false;

    // Convert local pixel coordinates to tile coordinates
    int tileX = localX / TILE_SIZE;
    int tileY = localY / TILE_SIZE;

    // Determine which screen base the tile is in (2×2 screen layout)
    int screenX = tileX / SCREEN_SIZE_TILES;
    int screenY = tileY / SCREEN_SIZE_TILES;
    int localTileX = tileX % SCREEN_SIZE_TILES;
    int localTileY = tileY % SCREEN_SIZE_TILES;

    // Calculate screen base index (0-3 for 2×2 layout)
    int screenBase = screenY * 2 + screenX;

    // Read tile index from map
    u16* mapBase = (u16*)BG_MAP_RAM(screenBase);
    u16 tileEntry = mapBase[localTileY * SCREEN_SIZE_TILES + localTileX];
    u16 tileIndex = tileEntry & TILE_INDEX_MASK;

    // Calculate pixel position within tile
    int pixelX = localX % TILE_WIDTH_PIXELS;
    int pixelY = localY % TILE_WIDTH_PIXELS;

    // Read palette index from tile data
    u8* tileData = (u8*)BG_TILE_RAM(1);
    u8 paletteIndex =
        tileData[tileIndex * TILE_DATA_SIZE + pixelY * TILE_WIDTH_PIXELS + pixelX];

    // Extract 5-bit RGB from palette
    u16 paletteColor = BG_PALETTE[paletteIndex];
    int r5 = (paletteColor >> COLOR_RED_SHIFT) & COLOR_5BIT_MASK;
    int g5 = (paletteColor >> COLOR_GREEN_SHIFT) & COLOR_5BIT_MASK;
    int b5 = (paletteColor >> COLOR_BLUE_SHIFT) & COLOR_5BIT_MASK;

    // Check 1: If gray track color, definitely NOT sand
    if (Terrain_IsGrayTrack5bit(r5, g5, b5))
        return false;

    // Check 2: If matches sand colors, IS sand
    return Terrain_ColorMatches5bit(r5, g5, b5, SAND_PRIMARY_R5, SAND_PRIMARY_G5,
                                     SAND_PRIMARY_B5, COLOR_TOLERANCE_5BIT) ||
           Terrain_ColorMatches5bit(r5, g5, b5, SAND_SECONDARY_R5, SAND_SECONDARY_G5,
                                     SAND_SECONDARY_B5, COLOR_TOLERANCE_5BIT);
}
