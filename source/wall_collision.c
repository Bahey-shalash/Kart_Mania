//=============================================================================
// wall_collision.c - CORRECTED FOR QUAD_OFFSET = 256
//=============================================================================
#include "wall_collision.h"

// TL Quadrant (offset: 0, 0) - walls already in correct global coords
static const WallSegment walls_TL[] = {
    {WALL_VERTICAL, 8, 0, 512},
    {WALL_HORIZONTAL, 8, 0, 512},
    {WALL_VERTICAL, 168, 162, 512},
    {WALL_HORIZONTAL, 162, 168, 272},
    {WALL_VERTICAL, 272, 162, 378},
    {WALL_HORIZONTAL, 378, 272, 512}
};

// TC Quadrant (offset: 256, 0) - add 256 to all X coords
static const WallSegment walls_TC[] = {
    {WALL_HORIZONTAL, 10, 256, 736},      // Y=10, X: 0-480 → 256-736
    {WALL_VERTICAL, 736, 10, 162},        // X=480 → 736, Y: 10-162
    {WALL_HORIZONTAL, 162, 736, 768},     // Y=162, X: 480-512 → 736-768
    {WALL_HORIZONTAL, 162, 256, 274},     // Y=162, X: 0-18 → 256-274
    {WALL_VERTICAL, 274, 162, 377},       // X=18 → 274, Y: 162-377
    {WALL_HORIZONTAL, 377, 274, 690},     // Y=377, X: 18-434 → 274-690
    {WALL_HORIZONTAL, 418, 690, 768},     // Y=418, X: 434-512 → 690-768
    {WALL_VERTICAL, 690, 377, 418}        // X=434 → 690, Y: 377-418
};

// TR Quadrant (offset: 512, 0) - add 512 to all X coords
static const WallSegment walls_TR[] = {
    {WALL_HORIZONTAL, 10, 512, 734},      // Y=10, X: 0-222 → 512-734
    {WALL_VERTICAL, 734, 10, 162},        // X=222 → 734, Y: 10-162
    {WALL_HORIZONTAL, 162, 734, 1016},    // Y=162, X: 222-504 → 734-1016
    {WALL_VERTICAL, 1016, 162, 512},      // X=504 → 1016, Y: 162-512
    {WALL_HORIZONTAL, 378, 512, 688},     // Y=378, X: 0-176 → 512-688
    {WALL_VERTICAL, 688, 378, 418},       // X=176 → 688, Y: 378-418
    {WALL_HORIZONTAL, 418, 688, 818},     // Y=418, X: 176-306 → 688-818
    {WALL_VERTICAL, 820, 418, 512}        // X=308 → 820, Y: 418-512
};

// ML Quadrant (offset: 0, 256) - add 256 to all Y coords
static const WallSegment walls_ML[] = {
    {WALL_VERTICAL, 8, 256, 768},         // X=8, Y: 0-512 → 256-768
    {WALL_VERTICAL, 168, 256, 554},       // X=168, Y: 0-298 → 256-554
    {WALL_HORIZONTAL, 554, 135, 168},     // Y=298 → 554, X: 135-168
    {WALL_VERTICAL, 135, 554, 668},       // X=135, Y: 298-412 → 554-668
    {WALL_HORIZONTAL, 668, 135, 178},     // Y=412 → 668, X: 135-178
    {WALL_VERTICAL, 178, 596, 668},       // X=178, Y: 340-412 → 596-668
    {WALL_HORIZONTAL, 596, 178, 275},     // Y=340 → 596, X: 178-275
    {WALL_VERTICAL, 275, 496, 596},       // X=275, Y: 240-340 → 496-596
    {WALL_HORIZONTAL, 496, 275, 512}      // Y=240 → 496, X: 275-512
};

// MC Quadrant (offset: 256, 256) - add 256 to both X and Y
static const WallSegment walls_MC[] = {
    {WALL_VERTICAL, 738, 500, 768},       // X=482 → 738, Y: 244-512 → 500-768
    {WALL_HORIZONTAL, 500, 274, 738},     // Y=244 → 500, X: 18-482 → 274-738
    {WALL_VERTICAL, 274, 500, 594},       // X=18 → 274, Y: 244-338 → 500-594
    {WALL_HORIZONTAL, 594, 256, 274}      // Y=338 → 594, X: 0-18 → 256-274
};

// MR Quadrant (offset: 512, 256) - add 512 to X, 256 to Y
static const WallSegment walls_MR[] = {
    {WALL_VERTICAL, 734, 499, 768},       // X=222 → 734, Y: 243-512 → 499-768
    {WALL_VERTICAL, 818, 417, 768},       // X=306 → 818, Y: 161-512 → 417-768
    {WALL_VERTICAL, 1016, 256, 768},      // X=504 → 1016, Y: 0-512 → 256-768
    {WALL_HORIZONTAL, 417, 689, 818},     // Y=161 → 417, X: 177-306 → 689-818
    {WALL_VERTICAL, 689, 378, 417},       // X=177 → 689, Y: 122-161 → 378-417
    {WALL_HORIZONTAL, 378, 512, 689}      // Y=122 → 378, X: 0-177 → 512-689
};

// BL Quadrant (offset: 0, 512) - add 512 to all Y coords
static const WallSegment walls_BL[] = {
    {WALL_HORIZONTAL, 1018, 480, 512},    // Y=506 → 1018, X: 480-512
    {WALL_HORIZONTAL, 875, 0, 480},       // Y=363 → 875, X: 0-480
    {WALL_VERTICAL, 480, 875, 1024},      // X=480, Y: 363-512 → 875-1024
    {WALL_VERTICAL, 8, 512, 875},         // X=8, Y: 0-363 → 512-875
    {WALL_HORIZONTAL, 595, 178, 274},     // Y=83 → 595, X: 178-274
    {WALL_VERTICAL, 178, 595, 667},       // X=178, Y: 83-155 → 595-667
    {WALL_HORIZONTAL, 667, 136, 178},     // Y=155 → 667, X: 136-178
    {WALL_VERTICAL, 136, 556, 667},       // X=136, Y: 44-155 → 556-667
    {WALL_HORIZONTAL, 556, 136, 166},     // Y=44 → 556, X: 136-166
    {WALL_VERTICAL, 166, 512, 556}        // X=166, Y: 0-44 → 512-556
};

// BC Quadrant (offset: 256, 512) - add 256 to X, 512 to Y
static const WallSegment walls_BC[] = {
    {WALL_VERTICAL, 739, 512, 819},       // X=483 → 739, Y: 0-307 → 512-819
    {WALL_HORIZONTAL, 819, 739, 768},     // Y=307 → 819, X: 483-512 → 739-768
    {WALL_HORIZONTAL, 1018, 479, 768},    // Y=506 → 1018, X: 223-512 → 479-768
    {WALL_VERTICAL, 479, 875, 1024},      // X=223 → 479, Y: 363-512 → 875-1024
    {WALL_HORIZONTAL, 875, 256, 489},     // Y=363 → 875, X: 0-233 → 256-489
    {WALL_HORIZONTAL, 594, 256, 272},     // Y=82 → 594, X: 0-16 → 256-272
    {WALL_VERTICAL, 272, 512, 594}        // X=16 → 272, Y: 0-82 → 512-594
};

// BR Quadrant (offset: 512, 512) - add 512 to both X and Y
static const WallSegment walls_BR[] = {
    {WALL_HORIZONTAL, 1018, 512, 1024},   // Y=506 → 1018, X: 0-512 → 512-1024
    {WALL_VERTICAL, 1016, 512, 1024},     // X=504 → 1016, Y: 0-512 → 512-1024
    {WALL_HORIZONTAL, 818, 734, 818},     // Y=306 → 818, X: 222-306 → 734-818
    {WALL_VERTICAL, 734, 512, 818},       // X=222 → 734, Y: 0-306 → 512-818
    {WALL_VERTICAL, 818, 512, 818}        // X=306 → 818, Y: 0-306 → 512-818
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
    {walls_BR, sizeof(walls_BR) / sizeof(WallSegment)}
};

//=============================================================================
// Collision Detection
//=============================================================================

static inline bool segmentCollision(const WallSegment* wall, int carX, int carY, int radius) {
    if (wall->type == WALL_HORIZONTAL) {
        int distY = (carY > wall->fixed_coord) ? (carY - wall->fixed_coord) : (wall->fixed_coord - carY);
        if (distY > radius) return false;
        return (carX + radius >= wall->min_range && carX - radius <= wall->max_range);
    } else {
        int distX = (carX > wall->fixed_coord) ? (carX - wall->fixed_coord) : (wall->fixed_coord - carX);
        if (distX > radius) return false;
        return (carY + radius >= wall->min_range && carY - radius <= wall->max_range);
    }
}

bool Wall_CheckCollision(int carX, int carY, int carRadius, QuadrantID quad) {
    if (quad < QUAD_TL || quad > QUAD_BR) return false;
    
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
            int dist = (carY > wall->fixed_coord) ? (carY - wall->fixed_coord) : (wall->fixed_coord - carY);
            if (dist < minDist && carX >= wall->min_range && carX <= wall->max_range) {
                minDist = dist;
                bestNy = (carY > wall->fixed_coord) ? 1 : -1;
                bestNx = 0;
            }
        } else {
            int dist = (carX > wall->fixed_coord) ? (carX - wall->fixed_coord) : (wall->fixed_coord - carX);
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