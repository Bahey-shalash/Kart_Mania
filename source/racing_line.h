#ifndef RACING_LINE_H
#define RACING_LINE_H

#include "fixedmath2d.h"
#include "game_types.h"
#include <stdbool.h>

//=============================================================================
// Racing Line System
// 
// Generates optimal racing line from track boundaries
// Provides fast geometric queries for AI navigation
//=============================================================================

#define MAX_RACING_LINE_POINTS 64

typedef struct {
    Vec2 position;          // Point on racing line
    Vec2 leftBound;         // Inner track boundary at this point
    Vec2 rightBound;        // Outer track boundary at this point
    Q16_8 trackWidth;       // Width of track at this point
    Q16_8 targetSpeed;      // Recommended speed (based on curvature)
    int tangentAngle512;    // Direction of racing line
    int cornerSharpness;    // 0-100: how sharp the corner is
} RacingLinePoint;

typedef struct {
    RacingLinePoint points[MAX_RACING_LINE_POINTS];
    int count;
} RacingLine;

//=============================================================================
// Public API
//=============================================================================

/**
 * Generate racing line for a map
 * @param map - Which track to generate for
 */
void RacingLine_Generate(Map map);

/**
 * Get the racing line for current map
 */
const RacingLine* RacingLine_Get(void);

/**
 * Find nearest point on racing line to given position
 * @param pos - Position to query
 * @param outIndex - (optional) Index of nearest point
 * @return Nearest racing line point
 */
RacingLinePoint RacingLine_GetNearestPoint(Vec2 pos, int* outIndex);

/**
 * Check if position is on track (between boundaries)
 * @param pos - Position to check
 * @return true if on track, false if in sand/off-track
 */
bool RacingLine_IsOnTrack(Vec2 pos);

/**
 * Get perpendicular distance from position to track edge
 * Positive = on track, Negative = off track
 * @param pos - Position to check
 * @return Distance to nearest edge (Q16.8)
 */
Q16_8 RacingLine_GetDistanceToEdge(Vec2 pos);

#endif // RACING_LINE_H