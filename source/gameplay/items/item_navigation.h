/**
 * File: item_navigation.h
 * -----------------------
 * Description: Waypoint-based navigation system for homing projectiles.
 *              Provides racing line waypoints for each map that red shells
 *              and missiles follow until they lock onto a target.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 06.01.2026
 */

#ifndef ITEM_NAVIGATION_H
#define ITEM_NAVIGATION_H

#include "../../math/fixedmath.h"
#include "../../core/game_types.h"

//=============================================================================
// Waypoint System
//=============================================================================

/**
 * Struct: Waypoint
 * ----------------
 * Represents a single waypoint on the racing line for projectile navigation.
 */
typedef struct {
    Vec2 pos;  // Position (Q16.8 fixed-point)
    int next;  // Index of next waypoint
} Waypoint;

//=============================================================================
// Navigation Functions
//=============================================================================

/**
 * Function: ItemNav_FindNearestWaypoint
 * --------------------------------------
 * Finds the closest waypoint to a given position on the specified map.
 *
 * Parameters:
 *   position - World position to search from
 *   map      - Map to search waypoints for
 *
 * Returns:
 *   Index of nearest waypoint
 */
int ItemNav_FindNearestWaypoint(const Vec2* position, Map map);

/**
 * Function: ItemNav_GetWaypointPosition
 * --------------------------------------
 * Returns the world position of a specific waypoint.
 *
 * Parameters:
 *   waypointIndex - Index of waypoint to query
 *   map           - Map the waypoint belongs to
 *
 * Returns:
 *   World position of the waypoint (Q16.8 fixed-point)
 */
Vec2 ItemNav_GetWaypointPosition(int waypointIndex, Map map);

/**
 * Function: ItemNav_GetNextWaypoint
 * ----------------------------------
 * Returns the index of the next waypoint in the racing line sequence.
 *
 * Parameters:
 *   currentWaypoint - Current waypoint index
 *   map             - Map the waypoint belongs to
 *
 * Returns:
 *   Index of next waypoint (loops back to start at end of track)
 */
int ItemNav_GetNextWaypoint(int currentWaypoint, Map map);

/**
 * Function: ItemNav_IsWaypointReached
 * ------------------------------------
 * Checks if a projectile is close enough to a waypoint to consider it reached.
 *
 * Parameters:
 *   itemPos      - Current projectile position
 *   waypointPos  - Target waypoint position
 *
 * Returns:
 *   true if waypoint is reached, false otherwise
 */
bool ItemNav_IsWaypointReached(const Vec2* itemPos, const Vec2* waypointPos);

#endif
