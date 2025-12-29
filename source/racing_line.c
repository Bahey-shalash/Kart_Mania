#include "racing_line.h"

#include <stdlib.h>
#include <string.h>

#include "game_constants.h"
#include "track_scorching_sands_data.h"

//=============================================================================
// Module State
//=============================================================================

static RacingLine currentRacingLine;
static const Vec2* currentInnerBoundary = NULL;
static const Vec2* currentOuterBoundary = NULL;
static int innerBoundaryCount = 0;
static int outerBoundaryCount = 0;

//=============================================================================
// Private Helpers
//=============================================================================

// Calculate racing line point between inner and outer boundaries
// apexBias: 0.0 = inner edge, 0.5 = center, 1.0 = outer edge
static Vec2 interpolateBoundaries(Vec2 inner, Vec2 outer, Q16_8 apexBias) {
    Vec2 diff = Vec2_Sub(outer, inner);
    return Vec2_Add(inner, Vec2_Scale(diff, apexBias));
}

// Calculate curvature at a point (0 = straight, higher = sharper turn)
static int calculateCurvature(Vec2 prev, Vec2 current, Vec2 next) {
    Vec2 v1 = Vec2_Sub(current, prev);
    Vec2 v2 = Vec2_Sub(next, current);

    if (Vec2_IsZero(v1) || Vec2_IsZero(v2))
        return 0;

    int angle1 = Vec2_ToAngle(v1);
    int angle2 = Vec2_ToAngle(v2);

    int angleDiff = abs(angle2 - angle1);
    if (angleDiff > ANGLE_HALF)
        angleDiff = ANGLE_FULL - angleDiff;

    // Return 0-100 scale (0 = straight, 100 = hairpin)
    return (angleDiff * 100) / ANGLE_QUARTER;  // Map 0-128 to 0-100
}

// Calculate recommended speed based on corner sharpness
static Q16_8 calculateTargetSpeed(int cornerSharpness) {
    if (cornerSharpness < 20) {
        return SPEED_50CC;  // Straight or gentle
    } else if (cornerSharpness < 50) {
        return FixedMul(SPEED_50CC, IntToFixed(80) / 100);  // Medium (80%)
    } else if (cornerSharpness < 75) {
        return FixedMul(SPEED_50CC, IntToFixed(65) / 100);  // Sharp (65%)
    } else {
        return FixedMul(SPEED_50CC, IntToFixed(50) / 100);  // Hairpin (50%)
    }
}

// Downsample boundaries to target point count
static void generateRacingLinePoints(int targetCount) {
    // Use whichever boundary has fewer points as reference
    int refCount = (innerBoundaryCount < outerBoundaryCount) ? innerBoundaryCount
                                                             : outerBoundaryCount;

    int step = refCount / targetCount;
    if (step < 1)
        step = 1;

    currentRacingLine.count = 0;

    for (int i = 0; i < refCount && currentRacingLine.count < MAX_RACING_LINE_POINTS;
         i += step) {
        int innerIdx = (i * innerBoundaryCount) / refCount;
        int outerIdx = (i * outerBoundaryCount) / refCount;

        Vec2 inner = currentInnerBoundary[innerIdx];
        Vec2 outer = currentOuterBoundary[outerIdx];

        RacingLinePoint* point = &currentRacingLine.points[currentRacingLine.count];

        // Store boundaries
        point->leftBound = inner;
        point->rightBound = outer;

        // Calculate track width
        point->trackWidth = Vec2_Distance(inner, outer);

        // Calculate racing line position (50% = center of track for safety)
        Q16_8 apexBias = IntToFixed(50) / 100;  // 0.5
        point->position = interpolateBoundaries(inner, outer, apexBias);

        currentRacingLine.count++;
    }

    // Second pass: Calculate curvature and speeds
    for (int i = 0; i < currentRacingLine.count; i++) {
        RacingLinePoint* point = &currentRacingLine.points[i];

        int prevIdx = (i - 1 + currentRacingLine.count) % currentRacingLine.count;
        int nextIdx = (i + 1) % currentRacingLine.count;

        Vec2 prev = currentRacingLine.points[prevIdx].position;
        Vec2 next = currentRacingLine.points[nextIdx].position;

        point->cornerSharpness = calculateCurvature(prev, point->position, next);
        point->targetSpeed = calculateTargetSpeed(point->cornerSharpness);

        // Calculate tangent angle
        Vec2 tangent = Vec2_Sub(next, point->position);
        point->tangentAngle512 = Vec2_ToAngle(tangent);
    }
}

//=============================================================================
// Public API Implementation
//=============================================================================

void RacingLine_Generate(Map map) {
    if (map == ScorchingSands) {
        currentInnerBoundary = INNER_BOUNDARY_SCORCHING_SANDS;
        currentOuterBoundary = OUTER_BOUNDARY_SCORCHING_SANDS;
        innerBoundaryCount = INNER_BOUNDARY_COUNT_SS;
        outerBoundaryCount = OUTER_BOUNDARY_COUNT_SS;

        generateRacingLinePoints(64);  // 64 waypoints (MAX_RACING_LINE_POINTS)
    }
    // Add other maps here as needed
}

const RacingLine* RacingLine_Get(void) {
    return &currentRacingLine;
}

RacingLinePoint RacingLine_GetNearestPoint(Vec2 pos, int* outIndex) {
    if (currentRacingLine.count == 0) {
        RacingLinePoint empty = {0};
        if (outIndex)
            *outIndex = -1;
        return empty;
    }

    Q16_8 nearestDistSq = IntToFixed(999999);
    int nearestIdx = 0;

    for (int i = 0; i < currentRacingLine.count; i++) {
        Q16_8 distSq = Vec2_DistanceSquared(pos, currentRacingLine.points[i].position);
        if (distSq < nearestDistSq) {
            nearestDistSq = distSq;
            nearestIdx = i;
        }
    }

    if (outIndex)
        *outIndex = nearestIdx;
    return currentRacingLine.points[nearestIdx];
}

bool RacingLine_IsOnTrack(Vec2 pos) {
    // Find nearest racing line point
    int nearestIdx = 0;
    RacingLinePoint nearest = RacingLine_GetNearestPoint(pos, &nearestIdx);

    if (Vec2_IsZero(nearest.position))
        return false;

    // Check if position is between left and right bounds
    // Using simple distance check (more accurate would use line segment projection)
    Q16_8 distToInner = Vec2_Distance(pos, nearest.leftBound);
    Q16_8 distToOuter = Vec2_Distance(pos, nearest.rightBound);
    Q16_8 trackWidth = nearest.trackWidth;

    // If sum of distances to both edges is close to track width, we're on track
    Q16_8 sumDist = distToInner + distToOuter;
    Q16_8 tolerance = trackWidth / 4;  // 25% tolerance

    return (sumDist <= trackWidth + tolerance);
}

Q16_8 RacingLine_GetDistanceToEdge(Vec2 pos) {
    int nearestIdx = 0;
    RacingLinePoint nearest = RacingLine_GetNearestPoint(pos, &nearestIdx);

    if (Vec2_IsZero(nearest.position))
        return 0;

    Q16_8 distToInner = Vec2_Distance(pos, nearest.leftBound);
    Q16_8 distToOuter = Vec2_Distance(pos, nearest.rightBound);

    // Return distance to nearest edge (positive if on track)
    Q16_8 minEdgeDist = (distToInner < distToOuter) ? distToInner : distToOuter;
    Q16_8 halfWidth = nearest.trackWidth / 2;

    return halfWidth - minEdgeDist;  // Positive = on track, negative = off track
}