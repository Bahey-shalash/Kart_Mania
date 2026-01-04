// item_navigation.h
#ifndef ITEM_NAVIGATION_H
#define ITEM_NAVIGATION_H
// BAHEY------
#include "fixedmath2d.h"
#include "game_types.h"

// Waypoint for projectile path following
typedef struct {
    Vec2 pos;       // Position (Q16.8 fixed-point)
    int next;       // Index of next waypoint
} Waypoint;

// Find closest waypoint to a position
int ItemNav_FindNearestWaypoint(Vec2 position, Map map);

// Get target waypoint for projectile to aim at
Vec2 ItemNav_GetWaypointPosition(int waypointIndex, Map map);

// Get next waypoint in sequence
int ItemNav_GetNextWaypoint(int currentWaypoint, Map map);

// Check if waypoint has been reached (within distance threshold)
bool ItemNav_IsWaypointReached(Vec2 itemPos, Vec2 waypointPos);

#endif