#include "track_geometry.h"
#include "track_scorching_sands_data.h"
#include <stdlib.h>

//=============================================================================
// Module State
//=============================================================================

static const Vec2* innerBoundary = NULL;
static const Vec2* outerBoundary = NULL;
static int innerCount = 0;
static int outerCount = 0;

//=============================================================================
// Private Helpers
//=============================================================================

// Find nearest boundary segment to a position
static void findNearestBoundaryPoints(Vec2 pos, Vec2* nearestInner, Vec2* nearestOuter) {
    Q16_8 minInnerDistSq = IntToFixed(999999);
    Q16_8 minOuterDistSq = IntToFixed(999999);
    
    *nearestInner = Vec2_Zero();
    *nearestOuter = Vec2_Zero();
    
    // Find nearest inner boundary point
    for (int i = 0; i < innerCount; i++) {
        Q16_8 distSq = Vec2_DistanceSquared(pos, innerBoundary[i]);
        if (distSq < minInnerDistSq) {
            minInnerDistSq = distSq;
            *nearestInner = innerBoundary[i];
        }
    }
    
    // Find nearest outer boundary point
    for (int i = 0; i < outerCount; i++) {
        Q16_8 distSq = Vec2_DistanceSquared(pos, outerBoundary[i]);
        if (distSq < minOuterDistSq) {
            minOuterDistSq = distSq;
            *nearestOuter = outerBoundary[i];
        }
    }
}

//=============================================================================
// Public API Implementation
//=============================================================================

void TrackGeometry_Init(Map map) {
    if (map == ScorchingSands) {
        innerBoundary = INNER_BOUNDARY_SCORCHING_SANDS;
        outerBoundary = OUTER_BOUNDARY_SCORCHING_SANDS;
        innerCount = INNER_BOUNDARY_COUNT_SS;
        outerCount = OUTER_BOUNDARY_COUNT_SS;
    }
    // Add other maps here as needed
}

bool TrackGeometry_IsOnTrack(Vec2 pos) {
    if (innerBoundary == NULL || outerBoundary == NULL) {
        return false;
    }
    
    Vec2 nearestInner, nearestOuter;
    findNearestBoundaryPoints(pos, &nearestInner, &nearestOuter);
    
    if (Vec2_IsZero(nearestInner) || Vec2_IsZero(nearestOuter)) {
        return false;
    }
    
    // Calculate distances to boundaries
    Q16_8 distToInner = Vec2_Distance(pos, nearestInner);
    Q16_8 distToOuter = Vec2_Distance(pos, nearestOuter);
    Q16_8 trackWidth = Vec2_Distance(nearestInner, nearestOuter);
    
    // Position is on track if sum of distances ≈ track width
    Q16_8 sumDist = distToInner + distToOuter;
    Q16_8 tolerance = trackWidth / 3; // 33% tolerance for curved sections
    
    return (sumDist <= trackWidth + tolerance);
}

Q16_8 TrackGeometry_GetDistanceToEdge(Vec2 pos) {
    if (innerBoundary == NULL || outerBoundary == NULL) {
        return 0;
    }
    
    Vec2 nearestInner, nearestOuter;
    findNearestBoundaryPoints(pos, &nearestInner, &nearestOuter);
    
    Q16_8 distToInner = Vec2_Distance(pos, nearestInner);
    Q16_8 distToOuter = Vec2_Distance(pos, nearestOuter);
    
    // Return distance to nearest edge
    Q16_8 minEdgeDist = (distToInner < distToOuter) ? distToInner : distToOuter;
    Q16_8 trackWidth = Vec2_Distance(nearestInner, nearestOuter);
    Q16_8 halfWidth = trackWidth / 2;
    
    // Positive = on track, negative = off track
    return halfWidth - minEdgeDist;
}

Q16_8 TrackGeometry_GetTrackWidth(Vec2 pos) {
    if (innerBoundary == NULL || outerBoundary == NULL) {
        return IntToFixed(80); // Default width
    }
    
    Vec2 nearestInner, nearestOuter;
    findNearestBoundaryPoints(pos, &nearestInner, &nearestOuter);
    
    return Vec2_Distance(nearestInner, nearestOuter);
}