//=============================================================================
// wall_collision.c
//=============================================================================
#include "../gameplay/wall_collision.h"
// HUGO------
// TL Quadrant (offset: 0, 0) - walls already in correct global coords
static const WallSegment walls_TL[] = {
    {WALL_VERTICAL, 8, 0, 512},        // good
    {WALL_HORIZONTAL, 8, 0, 512},      // good
    {WALL_VERTICAL, 167, 160, 512},    // good
    {WALL_HORIZONTAL, 160, 167, 273},  // good
    {WALL_VERTICAL, 273, 162, 375},    // good
    {WALL_HORIZONTAL, 375, 273, 512}   // good
};

// TC Quadrant (offset: 256, 0) - add 256 to all X coords
static const WallSegment walls_TC[] = {
    {WALL_HORIZONTAL, 8, 256, 734},    // Y=10, X: 0-478 → 256-734 good
    {WALL_VERTICAL, 734, 8, 160},      // X=478 → 734, Y: 8-160 good
    {WALL_HORIZONTAL, 160, 734, 768},  // Y=162, X: 480-512 → 736-768 good
    {WALL_HORIZONTAL, 160, 256, 272},  // Y=160, X: 0-16 → 256-272 good
    {WALL_VERTICAL, 272, 160, 376},    // X=16 → 274, Y: 160-376 good
    {WALL_HORIZONTAL, 376, 272, 688},  // Y=376, X: 16-432 → 272-688 good
    {WALL_HORIZONTAL, 416, 688, 768},  // Y=418, X: 434-512 → 690-768 good
    {WALL_VERTICAL, 688, 376, 416}     // X=434 → 690, Y: 377-418 good
};

// TR Quadrant (offset: 512, 0) - add 512 to all X coords
static const WallSegment walls_TR[] = {
    {WALL_HORIZONTAL, 8, 512, 735},     // Y=8, X: 0-223 → 512-735 good
    {WALL_VERTICAL, 735, 8, 160},       // X=223 → 734, Y: 8-160 good
    {WALL_HORIZONTAL, 160, 735, 1016},  // Y=160, X: 223-504 → 734-1016 good
    {WALL_VERTICAL, 1016, 160, 512},    // X=504 → 1016, Y: 162-512 good
    {WALL_HORIZONTAL, 376, 512, 687},   // Y=376, X: 0-175 → 512-687 good
    {WALL_VERTICAL, 687, 376, 416},     // X=175 → 687, Y: 376-416 good
    {WALL_HORIZONTAL, 416, 687, 815},   // Y=416, X: 175-303 → 687-815 good
    {WALL_VERTICAL, 815, 416, 512}      // X=308 → 820, Y: 418-512 good
};

// ML Quadrant (offset: 0, 256) - add 256 to all Y coords
static const WallSegment walls_ML[] = {
    {WALL_VERTICAL, 8, 256, 768},      // X=8, Y: 0-512 → 256-768 good
    {WALL_VERTICAL, 168, 256, 552},    // X=168, Y: 0-296 → 256-552 good
    {WALL_HORIZONTAL, 552, 136, 168},  // Y=296 → 552, X: 136-168 good
    {WALL_VERTICAL, 136, 552, 664},    // X=136, Y: 296-408 → 552-664 good
    {WALL_HORIZONTAL, 664, 136, 176},  // Y=408 → 664, X: 136-176 good
    {WALL_VERTICAL, 176, 594, 664},    // X=176, Y: 336-408 → 592-664 good
    {WALL_HORIZONTAL, 594, 176, 271},  // Y=336 → 592, X: 176-271 good
    {WALL_VERTICAL, 271, 496, 594},    // X=271, Y: 240-336 → 496-592 good
    {WALL_HORIZONTAL, 496, 271, 512}   // Y=240 → 496, X: 271-512 good
};

// MC Quadrant (offset: 256, 256) - add 256 to both X and Y
static const WallSegment walls_MC[] = {
    {WALL_VERTICAL, 735, 496, 768},    // X=479 → 735, Y: 240-512 → 496-768 good
    {WALL_HORIZONTAL, 496, 272, 735},  // Y=240 → 496, X: 16-479 → 272-735 good
    {WALL_VERTICAL, 272, 496, 594},    // X=16 → 272, Y: 240-336 → 496-592 good
    {WALL_HORIZONTAL, 594, 256, 272},  // Y=336 → 592, X: 0-16 → 256-272 good
    {WALL_VERTICAL, 272, 256, 376},    // X=16 → 272, Y: 0-120 → 256-376 good
    {WALL_HORIZONTAL, 376, 272, 688},  // Y=120 → 376, X:16-432 -→ 256-688 good
    {WALL_VERTICAL, 688, 376, 416},    // X=432 → 688, Y: 120-160 → 376-416 good
    {WALL_HORIZONTAL, 416, 688, 768},  // Y=160 → 416, X:432-512 -→ 688-768 good
};

// MR Quadrant (offset: 512, 256) - add 512 to X, 256 to Y
static const WallSegment walls_MR[] = {
    {WALL_VERTICAL, 815, 416, 768},    // X=303 → 815, Y: 160-512 → 416-768 good
    {WALL_VERTICAL, 1016, 256, 768},   // X=504 → 1016, Y: 0-512 → 256-768 good
    {WALL_HORIZONTAL, 416, 688, 815},  // Y=160 → 416, X: 176-303 → 688-815 good
    {WALL_VERTICAL, 688, 376, 416},    // X=176 → 688, Y: 120-160 → 376-416 good
    {WALL_HORIZONTAL, 376, 512, 688},  // Y=120 → 376, X: 0-176 → 512-688 good
    {WALL_HORIZONTAL, 495, 512, 735},  // Y=239 → 495, X: 0-223 → 512-735 good
    {WALL_VERTICAL, 735, 495, 768}     // X=223 → 735, Y: 239-512 → 495-768 good
};

// BL Quadrant (offset: 0, 512) - add 512 to all Y coords
static const WallSegment walls_BL[] = {
    {WALL_HORIZONTAL, 1016, 480, 512},  // Y=504 → 1016, X: 480-512 good
    {WALL_HORIZONTAL, 872, 0, 479},     // Y=360 → 872, X: 0-479 good
    {WALL_VERTICAL, 479, 872, 1016},    // X=479, Y: 360-504 → 872-1016 good
    {WALL_VERTICAL, 8, 512, 872},       // X=8, Y: 0-360 → 512-872 good
    {WALL_HORIZONTAL, 592, 176, 271},   // Y=80 → 592, X: 176-271 good
    {WALL_VERTICAL, 176, 592, 663},     // X=176, Y: 80-151 → 592-663 good
    {WALL_HORIZONTAL, 663, 136, 176},   // Y=151 → 667, X: 136-176 good
    {WALL_VERTICAL, 136, 552, 663},     // X=136, Y: 40-151 → 552-663 good
    {WALL_HORIZONTAL, 552, 136, 168},   // Y=40 → 552, X: 136-168 good
    {WALL_VERTICAL, 168, 512, 552},     // X=168, Y: 0-40 → 512-552 good
    {WALL_VERTICAL, 271, 512, 592},     // X=271, Y: 0-80 → 512-592 good
};

// BC Quadrant (offset: 256, 512) - add 256 to X, 512 to Y
static const WallSegment walls_BC[] = {
    {WALL_VERTICAL, 736, 512, 815},     // X=480 → 736, Y: 0-303 → 512-815 good
    {WALL_HORIZONTAL, 815, 736, 768},   // Y=303 → 815, X: 480-512 → 736-768 good
    {WALL_HORIZONTAL, 1016, 479, 768},  // Y=504 → 1016, X: 223-512 → 479-768 good
    {WALL_VERTICAL, 479, 872, 1016},    // X=223 → 479, Y: 360-504 → 875-1016 good
    {WALL_HORIZONTAL, 872, 256, 479},   // Y=363 → 875, X: 0-223 → 256-479 good
    {WALL_HORIZONTAL, 592, 256, 272},   // Y=80 → 592, X: 0-16 → 256-272 good
    {WALL_VERTICAL, 272, 512, 592}      // X=16 → 272, Y: 0-80 → 512-592 good
};

// BR Quadrant (offset: 512, 512) - add 512 to both X and Y
static const WallSegment walls_BR[] = {
    {WALL_HORIZONTAL, 1008, 512, 1008},  // Y=504 → 1016, X: 0-504 → 512-1016 good
    {WALL_VERTICAL, 1008, 512, 1008},    // X=504 → 1016, Y: 0-512 → 512-1016 good
    {WALL_HORIZONTAL, 815, 736, 815},    // Y=303 → 815, X: 224-303 → 736-815 good
    {WALL_VERTICAL, 736, 512, 815},      // X=224 → 736, Y: 0-303 → 512-815 good
    {WALL_VERTICAL, 815, 512, 815}       // X=303 → 813, Y: 0-303 → 512-815 good
};

// Quadrant wall lookup table
static const QuadrantWalls quadrantWalls[9] = {
    {walls_TL, sizeof(walls_TL) / sizeof(WallSegment)},
    {walls_TC, sizeof(walls_TC) / sizeof(WallSegment)},
    {walls_TR, sizeof(walls_TR) / sizeof(WallSegment)},
    {walls_ML, sizeof(walls_ML) / sizeof(WallSegment)},
    {walls_MC, sizeof(walls_MC) / sizeof(WallSegment)},
    {walls_MR, sizeof(walls_MR) / sizeof(WallSegment)},
    {walls_BL, sizeof(walls_BL) / sizeof(WallSegment)},
    {walls_BC, sizeof(walls_BC) / sizeof(WallSegment)},
    {walls_BR, sizeof(walls_BR) / sizeof(WallSegment)}};

//=============================================================================
// Collision Detection
//=============================================================================

static inline bool segmentCollision(const WallSegment* wall, int carX, int carY,
                                    int radius) {
    if (wall->type == WALL_HORIZONTAL) {
        int distY = (carY > wall->fixed_coord) ? (carY - wall->fixed_coord)
                                               : (wall->fixed_coord - carY);
        if (distY > radius)
            return false;
        return (carX + radius >= wall->min_range && carX - radius <= wall->max_range);
    } else {
        int distX = (carX > wall->fixed_coord) ? (carX - wall->fixed_coord)
                                               : (wall->fixed_coord - carX);
        if (distX > radius)
            return false;
        return (carY + radius >= wall->min_range && carY - radius <= wall->max_range);
    }
}

bool Wall_CheckCollision(int carX, int carY, int carRadius, QuadrantID quad) {
    if (quad < QUAD_TL || quad > QUAD_BR)
        return false;

    const QuadrantWalls* walls = &quadrantWalls[quad];

    for (int i = 0; i < walls->count; i++) {
        if (segmentCollision(&walls->segments[i], carX, carY, carRadius)) {
            return true;
        }
    }

    return false;
}

void Wall_GetCollisionNormal(int carX, int carY, QuadrantID quad, int* nx, int* ny) {
    if (quad < QUAD_TL || quad > QUAD_BR) {
        *nx = 0;
        *ny = 0;
        return;
    }

    const QuadrantWalls* walls = &quadrantWalls[quad];
    int minDist = 9999;
    int bestNx = 0, bestNy = 0;

    for (int i = 0; i < walls->count; i++) {
        const WallSegment* wall = &walls->segments[i];

        if (wall->type == WALL_HORIZONTAL) {
            int dist = (carY > wall->fixed_coord) ? (carY - wall->fixed_coord)
                                                  : (wall->fixed_coord - carY);
            if (dist < minDist && carX >= wall->min_range && carX <= wall->max_range) {
                minDist = dist;
                bestNy = (carY > wall->fixed_coord) ? 1 : -1;
                bestNx = 0;
            }
        } else {
            int dist = (carX > wall->fixed_coord) ? (carX - wall->fixed_coord)
                                                  : (wall->fixed_coord - carX);
            if (dist < minDist && carY >= wall->min_range && carY <= wall->max_range) {
                minDist = dist;
                bestNx = (carX > wall->fixed_coord) ? 1 : -1;
                bestNy = 0;
            }
        }
    }

    *nx = bestNx;
    *ny = bestNy;
}