
#include "../../gameplay/items/item_navigation.h"
#include <stdlib.h>
// BAHEY------
#define WAYPOINT_REACHED_DIST IntToFixed(25)  // 25 pixels = close enough

// Scorching Sands racing line waypoints (extracted from track coordinates)
static const Waypoint scorchingSands_racingLine[] = {
    // Start/finish straight (right side)
    {.pos = {IntToFixed(940), IntToFixed(553)}, .next = 1},
    {.pos = {IntToFixed(940), IntToFixed(533)}, .next = 2},
    {.pos = {IntToFixed(944), IntToFixed(501)}, .next = 3},
    {.pos = {IntToFixed(944), IntToFixed(479)}, .next = 4},
    {.pos = {IntToFixed(944), IntToFixed(459)}, .next = 5},
    {.pos = {IntToFixed(940), IntToFixed(452)}, .next = 6},
    {.pos = {IntToFixed(938), IntToFixed(430)}, .next = 7},

    // Top-right corner (curving left)
    {.pos = {IntToFixed(904), IntToFixed(413)}, .next = 8},
    {.pos = {IntToFixed(865), IntToFixed(395)}, .next = 9},
    {.pos = {IntToFixed(840), IntToFixed(373)}, .next = 10},
    {.pos = {IntToFixed(816), IntToFixed(354)}, .next = 11},
    {.pos = {IntToFixed(790), IntToFixed(339)}, .next = 12},
    {.pos = {IntToFixed(759), IntToFixed(322)}, .next = 13},
    {.pos = {IntToFixed(736), IntToFixed(305)}, .next = 14},
    {.pos = {IntToFixed(710), IntToFixed(293)}, .next = 15},
    {.pos = {IntToFixed(676), IntToFixed(283)}, .next = 16},
    {.pos = {IntToFixed(639), IntToFixed(269)}, .next = 17},
    {.pos = {IntToFixed(620), IntToFixed(260)}, .next = 18},
    {.pos = {IntToFixed(602), IntToFixed(253)}, .next = 19},
    {.pos = {IntToFixed(590), IntToFixed(246)}, .next = 20},
    {.pos = {IntToFixed(568), IntToFixed(232)}, .next = 21},
    {.pos = {IntToFixed(550), IntToFixed(222)}, .next = 22},
    {.pos = {IntToFixed(534), IntToFixed(219)}, .next = 23},
    {.pos = {IntToFixed(514), IntToFixed(204)}, .next = 24},
    {.pos = {IntToFixed(487), IntToFixed(192)}, .next = 25},
    {.pos = {IntToFixed(462), IntToFixed(177)}, .next = 26},
    {.pos = {IntToFixed(431), IntToFixed(165)}, .next = 27},
    {.pos = {IntToFixed(413), IntToFixed(154)}, .next = 28},
    {.pos = {IntToFixed(398), IntToFixed(145)}, .next = 29},
    {.pos = {IntToFixed(380), IntToFixed(132)}, .next = 30},
    {.pos = {IntToFixed(356), IntToFixed(117)}, .next = 31},

    // Top section (heading left)
    {.pos = {IntToFixed(313), IntToFixed(100)}, .next = 32},
    {.pos = {IntToFixed(282), IntToFixed(82)}, .next = 33},
    {.pos = {IntToFixed(240), IntToFixed(71)}, .next = 34},
    {.pos = {IntToFixed(207), IntToFixed(71)}, .next = 35},
    {.pos = {IntToFixed(178), IntToFixed(71)}, .next = 36},
    {.pos = {IntToFixed(157), IntToFixed(77)}, .next = 37},
    {.pos = {IntToFixed(140), IntToFixed(84)}, .next = 38},

    // Top-left corner (curving down)
    {.pos = {IntToFixed(119), IntToFixed(105)}, .next = 39},
    {.pos = {IntToFixed(103), IntToFixed(116)}, .next = 40},
    {.pos = {IntToFixed(86), IntToFixed(142)}, .next = 41},
    {.pos = {IntToFixed(81), IntToFixed(160)}, .next = 42},
    {.pos = {IntToFixed(79), IntToFixed(181)}, .next = 43},
    {.pos = {IntToFixed(77), IntToFixed(214)}, .next = 44},
    {.pos = {IntToFixed(72), IntToFixed(237)}, .next = 45},

    // Left side (going down)
    {.pos = {IntToFixed(68), IntToFixed(308)}, .next = 46},
    {.pos = {IntToFixed(68), IntToFixed(332)}, .next = 47},
    {.pos = {IntToFixed(68), IntToFixed(379)}, .next = 48},
    {.pos = {IntToFixed(68), IntToFixed(418)}, .next = 49},
    {.pos = {IntToFixed(68), IntToFixed(455)}, .next = 50},
    {.pos = {IntToFixed(68), IntToFixed(492)}, .next = 51},
    {.pos = {IntToFixed(68), IntToFixed(535)}, .next = 52},
    {.pos = {IntToFixed(68), IntToFixed(555)}, .next = 53},
    {.pos = {IntToFixed(68), IntToFixed(595)}, .next = 54},
    {.pos = {IntToFixed(68), IntToFixed(639)}, .next = 55},

    // Bottom-left corner (curving right)
    {.pos = {IntToFixed(70), IntToFixed(668)}, .next = 56},
    {.pos = {IntToFixed(71), IntToFixed(692)}, .next = 57},
    {.pos = {IntToFixed(82), IntToFixed(704)}, .next = 58},
    {.pos = {IntToFixed(92), IntToFixed(715)}, .next = 59},
    {.pos = {IntToFixed(100), IntToFixed(718)}, .next = 60},
    {.pos = {IntToFixed(115), IntToFixed(725)}, .next = 61},
    {.pos = {IntToFixed(135), IntToFixed(731)}, .next = 62},
    {.pos = {IntToFixed(149), IntToFixed(731)}, .next = 63},
    {.pos = {IntToFixed(175), IntToFixed(731)}, .next = 64},
    {.pos = {IntToFixed(196), IntToFixed(726)}, .next = 65},
    {.pos = {IntToFixed(208), IntToFixed(724)}, .next = 66},
    {.pos = {IntToFixed(218), IntToFixed(717)}, .next = 67},
    {.pos = {IntToFixed(237), IntToFixed(702)}, .next = 68},
    {.pos = {IntToFixed(264), IntToFixed(697)}, .next = 69},
    {.pos = {IntToFixed(280), IntToFixed(684)}, .next = 70},
    {.pos = {IntToFixed(288), IntToFixed(681)}, .next = 71},
    {.pos = {IntToFixed(305), IntToFixed(668)}, .next = 72},
    {.pos = {IntToFixed(326), IntToFixed(666)}, .next = 73},
    {.pos = {IntToFixed(342), IntToFixed(645)}, .next = 74},
    {.pos = {IntToFixed(362), IntToFixed(639)}, .next = 75},
    {.pos = {IntToFixed(391), IntToFixed(634)}, .next = 76},
    {.pos = {IntToFixed(423), IntToFixed(613)}, .next = 77},
    {.pos = {IntToFixed(446), IntToFixed(600)}, .next = 78},
    {.pos = {IntToFixed(480), IntToFixed(588)}, .next = 79},
    {.pos = {IntToFixed(500), IntToFixed(587)}, .next = 80},
    {.pos = {IntToFixed(513), IntToFixed(587)}, .next = 81},

    // Bottom section (heading right)
    {.pos = {IntToFixed(546), IntToFixed(596)}, .next = 82},
    {.pos = {IntToFixed(557), IntToFixed(614)}, .next = 83},
    {.pos = {IntToFixed(570), IntToFixed(631)}, .next = 84},
    {.pos = {IntToFixed(574), IntToFixed(643)}, .next = 85},
    {.pos = {IntToFixed(585), IntToFixed(660)}, .next = 86},
    {.pos = {IntToFixed(592), IntToFixed(677)}, .next = 87},
    {.pos = {IntToFixed(622), IntToFixed(728)}, .next = 88},
    {.pos = {IntToFixed(629), IntToFixed(747)}, .next = 89},
    {.pos = {IntToFixed(636), IntToFixed(760)}, .next = 90},
    {.pos = {IntToFixed(651), IntToFixed(801)}, .next = 91},
    {.pos = {IntToFixed(674), IntToFixed(846)}, .next = 92},
    {.pos = {IntToFixed(694), IntToFixed(871)}, .next = 93},
    {.pos = {IntToFixed(711), IntToFixed(883)}, .next = 94},
    {.pos = {IntToFixed(723), IntToFixed(887)}, .next = 95},
    {.pos = {IntToFixed(735), IntToFixed(897)}, .next = 96},
    {.pos = {IntToFixed(759), IntToFixed(911)}, .next = 97},
    {.pos = {IntToFixed(776), IntToFixed(918)}, .next = 98},
    {.pos = {IntToFixed(798), IntToFixed(923)}, .next = 99},
    {.pos = {IntToFixed(826), IntToFixed(923)}, .next = 100},
    {.pos = {IntToFixed(840), IntToFixed(923)}, .next = 101},
    {.pos = {IntToFixed(881), IntToFixed(925)}, .next = 102},
    {.pos = {IntToFixed(898), IntToFixed(918)}, .next = 103},

    // Bottom-right corner (curving up)
    {.pos = {IntToFixed(910), IntToFixed(908)}, .next = 104},
    {.pos = {IntToFixed(927), IntToFixed(893)}, .next = 105},
    {.pos = {IntToFixed(930), IntToFixed(883)}, .next = 106},
    {.pos = {IntToFixed(938), IntToFixed(857)}, .next = 107},
    {.pos = {IntToFixed(940), IntToFixed(837)}, .next = 108},
    {.pos = {IntToFixed(940), IntToFixed(814)}, .next = 109},
    {.pos = {IntToFixed(942), IntToFixed(778)}, .next = 110},
    {.pos = {IntToFixed(942), IntToFixed(756)}, .next = 111},
    {.pos = {IntToFixed(944), IntToFixed(732)}, .next = 112},
    {.pos = {IntToFixed(948), IntToFixed(686)}, .next = 113},
    {.pos = {IntToFixed(949), IntToFixed(657)}, .next = 114},
    {.pos = {IntToFixed(948), IntToFixed(624)}, .next = 115},
    {.pos = {IntToFixed(946), IntToFixed(609)}, .next = 116},
    {.pos = {IntToFixed(945), IntToFixed(582)}, .next = 117},
    {.pos = {IntToFixed(945), IntToFixed(557)}, .next = 118},

    // Back to start (loop)
    {.pos = {IntToFixed(940), IntToFixed(553)},
     .next = 0},  // Loops back to waypoint 0
};

static const int scorchingSands_waypointCount = 119;

// Get waypoint data for a specific map
static const Waypoint* getWaypointsForMap(Map map, int* count) {
    switch (map) {
        case ScorchingSands:
            *count = scorchingSands_waypointCount;
            return scorchingSands_racingLine;

        case AlpinRush:
        case NeonCircuit:
        case NONEMAP:
        default:
            *count = 0;
            return NULL;  // TODO: Add other maps when they're ready
    }
}

int ItemNav_FindNearestWaypoint(Vec2 position, Map map) {
    int count;
    const Waypoint* waypoints = getWaypointsForMap(map, &count);

    if (waypoints == NULL || count == 0) {
        return 0;
    }

    int nearestIndex = 0;
    Q16_8 minDist = 0x7FFFFFFF;  // Max value

    for (int i = 0; i < count; i++) {
        Q16_8 dist = Vec2_Distance(position, waypoints[i].pos);
        if (dist < minDist) {
            minDist = dist;
            nearestIndex = i;
        }
    }

    return nearestIndex;
}

Vec2 ItemNav_GetWaypointPosition(int waypointIndex, Map map) {
    int count;
    const Waypoint* waypoints = getWaypointsForMap(map, &count);

    if (waypoints == NULL || waypointIndex < 0 || waypointIndex >= count) {
        return Vec2_Zero();
    }

    return waypoints[waypointIndex].pos;
}

int ItemNav_GetNextWaypoint(int currentWaypoint, Map map) {
    int count;
    const Waypoint* waypoints = getWaypointsForMap(map, &count);

    if (waypoints == NULL || currentWaypoint < 0 || currentWaypoint >= count) {
        return 0;
    }

    return waypoints[currentWaypoint].next;
}

bool ItemNav_IsWaypointReached(Vec2 itemPos, Vec2 waypointPos) {
    Q16_8 dist = Vec2_Distance(itemPos, waypointPos);
    return (dist <= WAYPOINT_REACHED_DIST);
}