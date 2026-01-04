#include "../gameplay/terrain_detection.h"

#include <nds.h>
#include <stdlib.h>

#include "../core/game_constants.h"

//=============================================================================
// Private Helper Functions
//=============================================================================

/** Check if 5-bit RGB color matches target within tolerance */
static inline bool colorMatches5bit(int r5, int g5, int b5, int targetR5, int targetG5,
                                    int targetB5, int tolerance) {
    return (abs(r5 - targetR5) <= tolerance && abs(g5 - targetG5) <= tolerance &&
            abs(b5 - targetB5) <= tolerance);
}

/** Check if 5-bit color is gray track (main or light gray) */
static inline bool isGrayTrack5bit(int r5, int g5, int b5) {
    // Check for main gray (12,12,12)
    if (colorMatches5bit(r5, g5, b5, TERRAIN_GRAY_MAIN_R5, TERRAIN_GRAY_MAIN_G5, TERRAIN_GRAY_MAIN_B5,
                         TERRAIN_COLOR_TOLERANCE_5BIT)) {
        return true;
    }
    // Check for light gray (14,14,14)
    if (colorMatches5bit(r5, g5, b5, TERRAIN_GRAY_LIGHT_R5, TERRAIN_GRAY_LIGHT_G5, TERRAIN_GRAY_LIGHT_B5,
                         TERRAIN_COLOR_TOLERANCE_5BIT)) {
        return true;
    }
    return false;
}

//=============================================================================
// Public API
//=============================================================================

bool Terrain_IsOnSand(int x, int y, QuadrantID quad) {
    int col = quad % QUADRANT_GRID_SIZE;
    int row = quad / QUADRANT_GRID_SIZE;

    int quadStartX = col * QUAD_SIZE;
    int quadStartY = row * QUAD_SIZE;

    int localX = x - quadStartX;
    int localY = y - quadStartY;

    if (localX < 0 || localX >= QUAD_SIZE_DOUBLE || localY < 0 ||
        localY >= QUAD_SIZE_DOUBLE) {
        return false;
    }

    int tileX = localX / TILE_SIZE;
    int tileY = localY / TILE_SIZE;

    int screenX = tileX / SCREEN_SIZE_TILES;
    int screenY = tileY / SCREEN_SIZE_TILES;
    int localTileX = tileX % SCREEN_SIZE_TILES;
    int localTileY = tileY % SCREEN_SIZE_TILES;

    int screenBase = screenY * 2 + screenX;

    u16* mapBase = (u16*)BG_MAP_RAM(screenBase);
    u16 tileEntry = mapBase[localTileY * SCREEN_SIZE_TILES + localTileX];
    u16 tileIndex = tileEntry & TILE_INDEX_MASK;

    int pixelX = localX % TILE_WIDTH_PIXELS;
    int pixelY = localY % TILE_WIDTH_PIXELS;

    u8* tileData = (u8*)BG_TILE_RAM(1);
    u8 paletteIndex =
        tileData[tileIndex * TILE_DATA_SIZE + pixelY * TILE_WIDTH_PIXELS + pixelX];

    // Get 5-bit RGB directly from palette
    u16 paletteColor = BG_PALETTE[paletteIndex];
    int r5 = (paletteColor >> COLOR_RED_SHIFT) & COLOR_5BIT_MASK;
    int g5 = (paletteColor >> COLOR_GREEN_SHIFT) & COLOR_5BIT_MASK;
    int b5 = (paletteColor >> COLOR_BLUE_SHIFT) & COLOR_5BIT_MASK;

    // FIRST: Check if it's gray track - if so, definitely NOT sand
    if (isGrayTrack5bit(r5, g5, b5)) {
        return false;
    }

    // SECOND: Check if color matches sand shades (5-bit exact match)
    return colorMatches5bit(r5, g5, b5, TERRAIN_SAND_PRIMARY_R5, TERRAIN_SAND_PRIMARY_G5,
                            TERRAIN_SAND_PRIMARY_B5, TERRAIN_COLOR_TOLERANCE_5BIT) ||
           colorMatches5bit(r5, g5, b5, TERRAIN_SAND_SECONDARY_R5, TERRAIN_SAND_SECONDARY_G5,
                            TERRAIN_SAND_SECONDARY_B5, TERRAIN_COLOR_TOLERANCE_5BIT);
}