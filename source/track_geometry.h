#ifndef TRACK_GEOMETRY_H
#define TRACK_GEOMETRY_H

#include "fixedmath2d.h"
#include "game_types.h"
#include <stdbool.h>

//=============================================================================
// Track Geometry System
// 
// Fast geometric queries for track boundaries
// Replaces tile-based terrain detection for bots (works off-screen!)
//=============================================================================

/**
 * Initialize track geometry for a map
 * Call this when loading a new track
 */
void TrackGeometry_Init(Map map);

/**
 * Check if position is on track (geometric, works anywhere on map)
 * @param pos - Position to check (world coordinates)
 * @return true if on track, false if off-track (sand)
 */
bool TrackGeometry_IsOnTrack(Vec2 pos);

/**
 * Get perpendicular distance from position to nearest track edge
 * @param pos - Position to check
 * @return Distance to edge (positive = on track, negative = off track)
 */
Q16_8 TrackGeometry_GetDistanceToEdge(Vec2 pos);

/**
 * Get track width at given position
 * @param pos - Position to query
 * @return Width of track at this location
 */
Q16_8 TrackGeometry_GetTrackWidth(Vec2 pos);

#endif // TRACK_GEOMETRY_H