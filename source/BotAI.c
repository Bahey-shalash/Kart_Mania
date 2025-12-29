#include "BotAI.h"

#include <stdlib.h>

#include "game_constants.h"
#include "game_types.h"
#include "racing_line.h"
#include "track_geometry.h"
#include "wall_collision.h"

//=============================================================================
// Module State
//=============================================================================

static BotState botStates[MAX_CARS];
static WaypointPath currentTrackWaypoints;
static Map currentMap = NONEMAP;
static const int MAP_SIZE_PX = 1024;

// Specific rescue spot for known stuck area (outside track near bottom-left)
static const Vec2 STUCK_RESCUE_POS = {IntToFixed(265), IntToFixed(697)};
static const Vec2 STUCK_RESCUE_FACE_TARGET = {IntToFixed(272), IntToFixed(697)};
static const int STUCK_REGION_MIN_X = 120;
static const int STUCK_REGION_MAX_X = 230;
static const int STUCK_REGION_MIN_Y = 540;
static const int STUCK_REGION_MAX_Y = 700;

//=============================================================================
// Waypoint Data - Scorching Sands Track
//=============================================================================

// Define racing line waypoints around Scorching Sands track (based on actual track
// flow)
static const Waypoint WAYPOINTS_SCORCHING_SANDS[] = {
    // Start/finish area - Right side heading up
    {.position = {IntToFixed(948), IntToFixed(524)},
     .targetSpeed = SPEED_50CC,
     .cornerAngle512 = ANGLE_UP,
     .isCheckpoint = false},
    {.position = {IntToFixed(946), IntToFixed(480)},
     .targetSpeed = SPEED_50CC,
     .cornerAngle512 = ANGLE_UP,
     .isCheckpoint = false},
    {.position = {IntToFixed(945), IntToFixed(467)},
     .targetSpeed = SPEED_50CC,
     .cornerAngle512 = ANGLE_UP,
     .isCheckpoint = false},
    {.position = {IntToFixed(942), IntToFixed(441)},
     .targetSpeed = SPEED_50CC,
     .cornerAngle512 = ANGLE_UP,
     .isCheckpoint = false},
    {.position = {IntToFixed(923), IntToFixed(413)},
     .targetSpeed = SPEED_50CC,
     .cornerAngle512 = ANGLE_UP,
     .isCheckpoint = false},

    // Top-right curve - transitioning to left
    {.position = {IntToFixed(886), IntToFixed(400)},
     .targetSpeed = SPEED_50CC * 3 / 4,
     .cornerAngle512 = ANGLE_UP_LEFT,
     .isCheckpoint = false},
    {.position = {IntToFixed(829), IntToFixed(355)},
     .targetSpeed = SPEED_50CC * 3 / 4,
     .cornerAngle512 = ANGLE_UP_LEFT,
     .isCheckpoint = false},
    {.position = {IntToFixed(766), IntToFixed(339)},
     .targetSpeed = SPEED_50CC * 2 / 3,
     .cornerAngle512 = ANGLE_LEFT,
     .isCheckpoint = false},
    {.position = {IntToFixed(717), IntToFixed(312)},
     .targetSpeed = SPEED_50CC * 2 / 3,
     .cornerAngle512 = ANGLE_LEFT,
     .isCheckpoint = false},
    {.position = {IntToFixed(662), IntToFixed(275)},
     .targetSpeed = SPEED_50CC * 2 / 3,
     .cornerAngle512 = ANGLE_LEFT,
     .isCheckpoint = false},
    {.position = {IntToFixed(608), IntToFixed(249)},
     .targetSpeed = SPEED_50CC * 2 / 3,
     .cornerAngle512 = ANGLE_LEFT,
     .isCheckpoint = false},
    {.position = {IntToFixed(566), IntToFixed(231)},
     .targetSpeed = SPEED_50CC * 2 / 3,
     .cornerAngle512 = ANGLE_LEFT,
     .isCheckpoint = false},
    {.position = {IntToFixed(518), IntToFixed(206)},
     .targetSpeed = SPEED_50CC * 2 / 3,
     .cornerAngle512 = ANGLE_LEFT,
     .isCheckpoint = false},
    {.position = {IntToFixed(467), IntToFixed(177)},
     .targetSpeed = SPEED_50CC * 2 / 3,
     .cornerAngle512 = ANGLE_LEFT,
     .isCheckpoint = false},
    {.position = {IntToFixed(417), IntToFixed(145)},
     .targetSpeed = SPEED_50CC * 2 / 3,
     .cornerAngle512 = ANGLE_LEFT,
     .isCheckpoint = true},
    {.position = {IntToFixed(355), IntToFixed(128)},
     .targetSpeed = SPEED_50CC * 2 / 3,
     .cornerAngle512 = ANGLE_LEFT,
     .isCheckpoint = false},
    {.position = {IntToFixed(308), IntToFixed(102)},
     .targetSpeed = SPEED_50CC * 2 / 3,
     .cornerAngle512 = ANGLE_LEFT,
     .isCheckpoint = false},
    {.position = {IntToFixed(269), IntToFixed(81)},
     .targetSpeed = SPEED_50CC * 2 / 3,
     .cornerAngle512 = ANGLE_LEFT,
     .isCheckpoint = false},
    {.position = {IntToFixed(236), IntToFixed(75)},
     .targetSpeed = SPEED_50CC * 2 / 3,
     .cornerAngle512 = ANGLE_LEFT,
     .isCheckpoint = false},
    {.position = {IntToFixed(192), IntToFixed(60)},
     .targetSpeed = SPEED_50CC / 2,
     .cornerAngle512 = ANGLE_LEFT,
     .isCheckpoint = false},

    // Top-left curve - transitioning to down
    {.position = {IntToFixed(169), IntToFixed(62)},
     .targetSpeed = SPEED_50CC / 2,
     .cornerAngle512 = ANGLE_DOWN_LEFT,
     .isCheckpoint = false},
    {.position = {IntToFixed(119), IntToFixed(81)},
     .targetSpeed = SPEED_50CC / 2,
     .cornerAngle512 = ANGLE_DOWN,
     .isCheckpoint = false},
    {.position = {IntToFixed(95), IntToFixed(154)},
     .targetSpeed = SPEED_50CC * 3 / 4,
     .cornerAngle512 = ANGLE_DOWN,
     .isCheckpoint = false},
    {.position = {IntToFixed(88), IntToFixed(175)},
     .targetSpeed = SPEED_50CC * 3 / 4,
     .cornerAngle512 = ANGLE_DOWN,
     .isCheckpoint = false},
    {.position = {IntToFixed(88), IntToFixed(206)},
     .targetSpeed = SPEED_50CC,
     .cornerAngle512 = ANGLE_DOWN,
     .isCheckpoint = false},
    {.position = {IntToFixed(88), IntToFixed(247)},
     .targetSpeed = SPEED_50CC,
     .cornerAngle512 = ANGLE_DOWN,
     .isCheckpoint = false},
    {.position = {IntToFixed(53), IntToFixed(325)},
     .targetSpeed = SPEED_50CC,
     .cornerAngle512 = ANGLE_DOWN,
     .isCheckpoint = false},
    {.position = {IntToFixed(65), IntToFixed(373)},
     .targetSpeed = SPEED_50CC,
     .cornerAngle512 = ANGLE_DOWN,
     .isCheckpoint = false},
    {.position = {IntToFixed(65), IntToFixed(416)},
     .targetSpeed = SPEED_50CC,
     .cornerAngle512 = ANGLE_DOWN,
     .isCheckpoint = false},
    {.position = {IntToFixed(65), IntToFixed(470)},
     .targetSpeed = SPEED_50CC,
     .cornerAngle512 = ANGLE_DOWN,
     .isCheckpoint = true},
    {.position = {IntToFixed(65), IntToFixed(531)},
     .targetSpeed = SPEED_50CC,
     .cornerAngle512 = ANGLE_DOWN,
     .isCheckpoint = false},
    {.position = {IntToFixed(65), IntToFixed(582)},
     .targetSpeed = SPEED_50CC,
     .cornerAngle512 = ANGLE_DOWN,
     .isCheckpoint = false},
    {.position = {IntToFixed(70), IntToFixed(641)},
     .targetSpeed = SPEED_50CC,
     .cornerAngle512 = ANGLE_DOWN,
     .isCheckpoint = false},
    {.position = {IntToFixed(71), IntToFixed(688)},
     .targetSpeed = SPEED_50CC,
     .cornerAngle512 = ANGLE_DOWN,
     .isCheckpoint = false},
    {.position = {IntToFixed(71), IntToFixed(708)},
     .targetSpeed = SPEED_50CC * 3 / 4,
     .cornerAngle512 = ANGLE_DOWN,
     .isCheckpoint = false},

    // Bottom-left curve - transitioning to right
    {.position = {IntToFixed(121), IntToFixed(731)},
     .targetSpeed = SPEED_50CC / 2,
     .cornerAngle512 = ANGLE_DOWN_RIGHT,
     .isCheckpoint = false},
    {.position = {IntToFixed(144), IntToFixed(732)},
     .targetSpeed = SPEED_50CC / 2,
     .cornerAngle512 = ANGLE_RIGHT,
     .isCheckpoint = false},
    {.position = {IntToFixed(154), IntToFixed(732)},
     .targetSpeed = SPEED_50CC * 2 / 3,
     .cornerAngle512 = ANGLE_RIGHT,
     .isCheckpoint = false},
    {.position = {IntToFixed(187), IntToFixed(732)},
     .targetSpeed = SPEED_50CC * 2 / 3,
     .cornerAngle512 = ANGLE_RIGHT,
     .isCheckpoint = false},
    {.position = {IntToFixed(207), IntToFixed(710)},
     .targetSpeed = SPEED_50CC * 2 / 3,
     .cornerAngle512 = ANGLE_RIGHT,
     .isCheckpoint = false},
    {.position = {IntToFixed(221), IntToFixed(710)},
     .targetSpeed = SPEED_50CC * 2 / 3,
     .cornerAngle512 = ANGLE_RIGHT,
     .isCheckpoint = false},
    {.position = {IntToFixed(243), IntToFixed(704)},
     .targetSpeed = SPEED_50CC * 2 / 3,
     .cornerAngle512 = ANGLE_RIGHT,
     .isCheckpoint = false},
    {.position = {IntToFixed(275), IntToFixed(685)},
     .targetSpeed = SPEED_50CC * 2 / 3,
     .cornerAngle512 = ANGLE_RIGHT,
     .isCheckpoint = false},
    {.position = {IntToFixed(277), IntToFixed(682)},
     .targetSpeed = SPEED_50CC * 2 / 3,
     .cornerAngle512 = ANGLE_RIGHT,
     .isCheckpoint = false},
    {.position = {IntToFixed(301), IntToFixed(675)},
     .targetSpeed = SPEED_50CC * 2 / 3,
     .cornerAngle512 = ANGLE_RIGHT,
     .isCheckpoint = false},
    {.position = {IntToFixed(347), IntToFixed(656)},
     .targetSpeed = SPEED_50CC * 2 / 3,
     .cornerAngle512 = ANGLE_RIGHT,
     .isCheckpoint = false},
    {.position = {IntToFixed(402), IntToFixed(620)},
     .targetSpeed = SPEED_50CC * 2 / 3,
     .cornerAngle512 = ANGLE_RIGHT,
     .isCheckpoint = false},
    {.position = {IntToFixed(446), IntToFixed(600)},
     .targetSpeed = SPEED_50CC * 2 / 3,
     .cornerAngle512 = ANGLE_RIGHT,
     .isCheckpoint = false},
    {.position = {IntToFixed(481), IntToFixed(595)},
     .targetSpeed = SPEED_50CC * 2 / 3,
     .cornerAngle512 = ANGLE_RIGHT,
     .isCheckpoint = false},
    {.position = {IntToFixed(503), IntToFixed(594)},
     .targetSpeed = SPEED_50CC * 3 / 4,
     .cornerAngle512 = ANGLE_RIGHT,
     .isCheckpoint = true},
    {.position = {IntToFixed(527), IntToFixed(591)},
     .targetSpeed = SPEED_50CC * 3 / 4,
     .cornerAngle512 = ANGLE_RIGHT,
     .isCheckpoint = false},
    {.position = {IntToFixed(541), IntToFixed(595)},
     .targetSpeed = SPEED_50CC * 3 / 4,
     .cornerAngle512 = ANGLE_RIGHT,
     .isCheckpoint = false},
    {.position = {IntToFixed(548), IntToFixed(610)},
     .targetSpeed = SPEED_50CC * 3 / 4,
     .cornerAngle512 = ANGLE_RIGHT,
     .isCheckpoint = false},
    {.position = {IntToFixed(552), IntToFixed(623)},
     .targetSpeed = SPEED_50CC * 3 / 4,
     .cornerAngle512 = ANGLE_RIGHT,
     .isCheckpoint = false},
    {.position = {IntToFixed(560), IntToFixed(632)},
     .targetSpeed = SPEED_50CC * 3 / 4,
     .cornerAngle512 = ANGLE_DOWN_RIGHT,
     .isCheckpoint = false},
    {.position = {IntToFixed(600), IntToFixed(681)},
     .targetSpeed = SPEED_50CC / 2,
     .cornerAngle512 = ANGLE_DOWN,
     .isCheckpoint = false},
    {.position = {IntToFixed(607), IntToFixed(717)},
     .targetSpeed = SPEED_50CC / 2,
     .cornerAngle512 = ANGLE_DOWN,
     .isCheckpoint = false},
    {.position = {IntToFixed(631), IntToFixed(767)},
     .targetSpeed = SPEED_50CC / 2,
     .cornerAngle512 = ANGLE_DOWN_RIGHT,
     .isCheckpoint = false},
    {.position = {IntToFixed(693), IntToFixed(837)},
     .targetSpeed = SPEED_50CC / 2,
     .cornerAngle512 = ANGLE_DOWN_RIGHT,
     .isCheckpoint = false},
    {.position = {IntToFixed(708), IntToFixed(861)},
     .targetSpeed = SPEED_50CC / 2,
     .cornerAngle512 = ANGLE_RIGHT,
     .isCheckpoint = false},
    {.position = {IntToFixed(743), IntToFixed(872)},
     .targetSpeed = SPEED_50CC * 2 / 3,
     .cornerAngle512 = ANGLE_RIGHT,
     .isCheckpoint = false},
    {.position = {IntToFixed(769), IntToFixed(884)},
     .targetSpeed = SPEED_50CC * 2 / 3,
     .cornerAngle512 = ANGLE_RIGHT,
     .isCheckpoint = false},
    {.position = {IntToFixed(797), IntToFixed(884)},
     .targetSpeed = SPEED_50CC * 2 / 3,
     .cornerAngle512 = ANGLE_RIGHT,
     .isCheckpoint = false},
    {.position = {IntToFixed(807), IntToFixed(898)},
     .targetSpeed = SPEED_50CC * 2 / 3,
     .cornerAngle512 = ANGLE_RIGHT,
     .isCheckpoint = false},
    {.position = {IntToFixed(840), IntToFixed(907)},
     .targetSpeed = SPEED_50CC * 2 / 3,
     .cornerAngle512 = ANGLE_RIGHT,
     .isCheckpoint = false},
    {.position = {IntToFixed(892), IntToFixed(907)},
     .targetSpeed = SPEED_50CC * 3 / 4,
     .cornerAngle512 = ANGLE_RIGHT,
     .isCheckpoint = false},

    // Bottom-right curve - transitioning up to finish
    {.position = {IntToFixed(908), IntToFixed(893)},
     .targetSpeed = SPEED_50CC / 2,
     .cornerAngle512 = ANGLE_UP_RIGHT,
     .isCheckpoint = false},
    {.position = {IntToFixed(915), IntToFixed(880)},
     .targetSpeed = SPEED_50CC / 2,
     .cornerAngle512 = ANGLE_UP,
     .isCheckpoint = false},
    {.position = {IntToFixed(920), IntToFixed(859)},
     .targetSpeed = SPEED_50CC * 2 / 3,
     .cornerAngle512 = ANGLE_UP,
     .isCheckpoint = false},
    {.position = {IntToFixed(922), IntToFixed(830)},
     .targetSpeed = SPEED_50CC * 3 / 4,
     .cornerAngle512 = ANGLE_UP,
     .isCheckpoint = false},
    {.position = {IntToFixed(923), IntToFixed(790)},
     .targetSpeed = SPEED_50CC,
     .cornerAngle512 = ANGLE_UP,
     .isCheckpoint = false},
    {.position = {IntToFixed(923), IntToFixed(771)},
     .targetSpeed = SPEED_50CC,
     .cornerAngle512 = ANGLE_UP,
     .isCheckpoint = false},
    {.position = {IntToFixed(923), IntToFixed(733)},
     .targetSpeed = SPEED_50CC,
     .cornerAngle512 = ANGLE_UP,
     .isCheckpoint = false},
    {.position = {IntToFixed(935), IntToFixed(689)},
     .targetSpeed = SPEED_50CC,
     .cornerAngle512 = ANGLE_UP,
     .isCheckpoint = false},
    {.position = {IntToFixed(935), IntToFixed(648)},
     .targetSpeed = SPEED_50CC,
     .cornerAngle512 = ANGLE_UP,
     .isCheckpoint = false},
    {.position = {IntToFixed(935), IntToFixed(638)},
     .targetSpeed = SPEED_50CC,
     .cornerAngle512 = ANGLE_UP,
     .isCheckpoint = false},
    {.position = {IntToFixed(935), IntToFixed(610)},
     .targetSpeed = SPEED_50CC,
     .cornerAngle512 = ANGLE_UP,
     .isCheckpoint = false},
    {.position = {IntToFixed(935), IntToFixed(584)},
     .targetSpeed = SPEED_50CC,
     .cornerAngle512 = ANGLE_UP,
     .isCheckpoint = false},
    {.position = {IntToFixed(938), IntToFixed(560)},
     .targetSpeed = SPEED_50CC,
     .cornerAngle512 = ANGLE_UP,
     .isCheckpoint = false},
    {.position = {IntToFixed(941), IntToFixed(541)},
     .targetSpeed = SPEED_50CC,
     .cornerAngle512 = ANGLE_UP,
     .isCheckpoint = false},
};

#define WAYPOINT_COUNT_SCORCHING_SANDS \
    (sizeof(WAYPOINTS_SCORCHING_SANDS) / sizeof(Waypoint))

//=============================================================================
// Helper Function Prototypes
//=============================================================================

// Navigation
static Vec2 calculateSteeringTarget(Car* car, BotState* state,
                                    const WaypointPath* path);
static Vec2 applyNavigationMistakes(Vec2 targetPos, BotState* state,
                                    BotPersonality personality);

// Item collection
static bool findNearestItemBox(Car* car, BotState* state, const RaceState* raceState);
static bool shouldSeekItemBox(Car* car, BotState* state, BotPersonality personality);
static Vec2 calculateItemInterceptPoint(Vec2 carPos, Vec2 itemPos,
                                        Vec2 racingLineTarget);

// Hazard avoidance
static bool detectHazardsAhead(Car* car, BotState* state);
static Vec2 calculateAvoidanceVector(Car* car, Vec2 hazardPos);
static Vec2 blendAvoidanceWithRacingLine(Vec2 racingTarget, Vec2 avoidanceTarget,
                                         int avoidanceTimer);

// Item usage
static void updateItemUsage(Car* car, BotState* state, BotPersonality personality,
                            const RaceState* raceState);
static bool isOnStraightaway(BotState* state);
static bool isCarAheadInRange(Car* car, const RaceState* raceState, Q16_8 range);

// Adaptive behaviors
static void applyRubberBanding(Car* car, const RaceState* raceState);
static BotPersonality applyPositionTactics(const BotState* state, const Car* car,
                                           BotPersonality basePersonality);
static void updateOvertaking(Car* car, BotState* state, const RaceState* raceState);

// Wall avoidance
static QuadrantID determineQuadrantAtPos(int x, int y);
static bool applyWallAvoidance(Car* car, Vec2* steeringTarget);

// Stuck handling
static int findNearestCheckpointIndex(Vec2 position);
static void teleportToCheckpoint(Car* car, BotState* state, int checkpointIndex);
static bool teleportToKnownSafeSpot(Car* car, BotState* state);

// Control execution
static void executeSteeringControl(Car* car, Vec2 targetPos);
static void executeAccelerationControl(Car* car, BotState* state);

//=============================================================================
// Public API Implementation
//=============================================================================

void BotAI_Init(Map map) {
    currentMap = map;

    // Load waypoints from generated racing line
    const RacingLine* racingLine = RacingLine_Get();

    if (racingLine->count > 0) {
        // Use racing line points as waypoints
        currentTrackWaypoints.count = racingLine->count;

        for (int i = 0; i < racingLine->count && i < MAX_WAYPOINTS; i++) {
            currentTrackWaypoints.waypoints[i].position =
                racingLine->points[i].position;
            currentTrackWaypoints.waypoints[i].targetSpeed =
                racingLine->points[i].targetSpeed;
            currentTrackWaypoints.waypoints[i].cornerAngle512 =
                racingLine->points[i].tangentAngle512;
            currentTrackWaypoints.waypoints[i].isCheckpoint =
                false;  // Can mark specific ones later
        }
    } else {
        // Fallback to hardcoded waypoints if racing line failed to generate
        if (map == ScorchingSands) {
            int count = (WAYPOINT_COUNT_SCORCHING_SANDS < MAX_WAYPOINTS)
                            ? WAYPOINT_COUNT_SCORCHING_SANDS
                            : MAX_WAYPOINTS;
            for (int i = 0; i < count; i++) {
                currentTrackWaypoints.waypoints[i] = WAYPOINTS_SCORCHING_SANDS[i];
            }
            currentTrackWaypoints.count = count;
        }
    }

    // Initialize all bot states
    for (int i = 0; i < MAX_CARS; i++) {
        BotAI_Reset(i);
    }
}

void BotAI_Reset(int botIndex) {
    if (botIndex < 0 || botIndex >= MAX_CARS)
        return;

    BotState* state = &botStates[botIndex];
    state->targetWaypoint = 0;
    state->nextWaypoint = 1;
    state->targetPosition = Vec2_Zero();
    state->mistakeTimer = MISTAKE_INTERVAL_BASE;
    state->correctionTimer = 0;
    state->stuckTimer = 0;
    state->isOvertaking = false;
    state->overtakeTarget = Vec2_Zero();
    state->nearestHazardPos = Vec2_Zero();
    state->hazardAvoidanceTimer = 0;
    state->targetItemBoxPos = Vec2_Zero();
    state->seekingItemBox = false;
    state->itemUsageTimer = 0;
    state->stuckStillFrames = 0;
    state->lastPos = Vec2_Zero();
    state->lastPosInitialized = false;
    state->personality = state->basePersonality;
}

void BotAI_SetPersonality(int botIndex, BotPersonality personality) {
    if (botIndex < 0 || botIndex >= MAX_CARS)
        return;
    botStates[botIndex].basePersonality = personality;
    botStates[botIndex].personality = personality;
}

BotPersonality BotAI_GeneratePersonality(BotSkillLevel skillLevel) {
    BotPersonality p;
    p.skillLevel = skillLevel;

    switch (skillLevel) {
        case SKILL_EASY:
            // Low consistency = many mistakes
            p.consistency = IntToFixed(40 + (rand() % 40)) / 100;   // 40-80%
            p.aggression = IntToFixed(30 + (rand() % 30)) / 100;    // 30-60%
            p.itemPriority = IntToFixed(60 + (rand() % 30)) / 100;  // 60-90%
            p.reactionDelay = 15 + (rand() % 15);                   // 15-30 frames
            break;

        case SKILL_MEDIUM:
            p.consistency = IntToFixed(60 + (rand() % 30)) / 100;   // 60-90%
            p.aggression = IntToFixed(50 + (rand() % 40)) / 100;    // 50-90%
            p.itemPriority = IntToFixed(40 + (rand() % 40)) / 100;  // 40-80%
            p.reactionDelay = 8 + (rand() % 10);                    // 8-18 frames
            break;

        case SKILL_HARD:
            p.consistency = IntToFixed(80 + (rand() % 20)) / 100;   // 80-100%
            p.aggression = IntToFixed(70 + (rand() % 30)) / 100;    // 70-100%
            p.itemPriority = IntToFixed(20 + (rand() % 40)) / 100;  // 20-60%
            p.reactionDelay = 3 + (rand() % 8);                     // 3-10 frames
            break;
    }

    return p;
}

void BotAI_Update(Car* car, int botIndex, const RaceState* raceState) {
    if (car == NULL || botIndex < 0 || botIndex >= MAX_CARS)
        return;

    BotState* state = &botStates[botIndex];
    WaypointPath* path = &currentTrackWaypoints;
    state->personality = state->basePersonality;  // Reset working copy each frame

    // 1. ADAPTIVE BEHAVIOR
    applyRubberBanding(car, raceState);
    BotPersonality effectivePersonality =
        applyPositionTactics(state, car, state->basePersonality);
    state->personality = effectivePersonality;

    // 2. NAVIGATION
    Vec2 racingLineTarget = calculateSteeringTarget(car, state, path);
    racingLineTarget =
        applyNavigationMistakes(racingLineTarget, state, effectivePersonality);

    // 3. ITEM COLLECTION
    bool itemBoxFound = findNearestItemBox(car, state, raceState);
    if (itemBoxFound && shouldSeekItemBox(car, state, effectivePersonality)) {
        state->seekingItemBox = true;
        racingLineTarget = calculateItemInterceptPoint(
            car->position, state->targetItemBoxPos, racingLineTarget);
    } else {
        state->seekingItemBox = false;
    }

    // 4. HAZARD AVOIDANCE (highest priority)
    if (detectHazardsAhead(car, state)) {
        // Start avoidance maneuver
        if (state->hazardAvoidanceTimer == 0) {
            state->hazardAvoidanceTimer = HAZARD_AVOIDANCE_DURATION;
        }
    }

    if (state->hazardAvoidanceTimer > 0) {
        Vec2 avoidanceTarget = calculateAvoidanceVector(car, state->nearestHazardPos);
        racingLineTarget = blendAvoidanceWithRacingLine(
            racingLineTarget, avoidanceTarget, state->hazardAvoidanceTimer);
        state->hazardAvoidanceTimer--;
    }

    // 5. WALL AVOIDANCE (prevents grinding on outer/inner walls)
    applyWallAvoidance(car, &racingLineTarget);

    // 6. OVERTAKING
    updateOvertaking(car, state, raceState);
    if (state->isOvertaking) {
        racingLineTarget = state->overtakeTarget;
    }

    // 7. STUCK DETECTION
    if (car->speed < IntToFixed(1)) {
        state->stuckTimer++;
        if (state->stuckTimer > 60) {
            // Reverse briefly
            Car_Brake(car);
            Car_Steer(car, 128);  // Turn around
            state->stuckTimer = 0;
        }
    } else {
        state->stuckTimer = 0;
    }

    // 8. STEERING CONTROL
    executeSteeringControl(car, racingLineTarget);

    // 9. ACCELERATION CONTROL
    executeAccelerationControl(car, state);

    // 10. ITEM USAGE
    updateItemUsage(car, state, effectivePersonality, raceState);
}

void BotAI_PostPhysicsUpdate(Car* car, int botIndex) {
    if (car == NULL || botIndex < 0 || botIndex >= MAX_CARS)
        return;

    BotState* state = &botStates[botIndex];

    // Track wall impacts (banging without progress)
    int carX = FixedToInt(car->position.x);
    int carY = FixedToInt(car->position.y);
    QuadrantID quad = determineQuadrantAtPos(carX, carY);
    bool touchingWall = Wall_CheckCollision(carX, carY, CAR_RADIUS, quad);

    if (state->wallBounceCooldown > 0) {
        state->wallBounceCooldown--;
    }

    if (touchingWall && car->speed < STUCK_MOVE_THRESHOLD) {
        if (state->wallBounceCooldown == 0) {
            state->wallBounceCount++;
            state->wallBounceCooldown = WALL_BOUNCE_COOLDOWN;
        }
    } else {
        state->wallBounceCount = 0;
    }

    if (!state->lastPosInitialized) {
        state->lastPos = car->position;
        state->lastPosInitialized = true;
        return;
    }

    Q16_8 movedDist = Vec2_Distance(car->position, state->lastPos);
    if (movedDist < STUCK_MOVE_THRESHOLD) {
        state->stuckStillFrames++;
    } else {
        state->stuckStillFrames = 0;
    }

    // If bouncing in the same area or hitting wall repeatedly, warp to nearest checkpoint
    bool tooManyBounces = state->wallBounceCount >= WALL_BOUNCE_LIMIT;
    if (state->stuckStillFrames >= STUCK_BOUNCE_FRAMES || tooManyBounces) {
        if (!teleportToKnownSafeSpot(car, state)) {
            int checkpointIndex = findNearestCheckpointIndex(car->position);
            if (checkpointIndex >= 0) {
                teleportToCheckpoint(car, state, checkpointIndex);
            }
        }
        state->stuckStillFrames = 0;
        state->wallBounceCount = 0;
        state->wallBounceCooldown = 0;
    }

    state->lastPos = car->position;
}

//=============================================================================
// Navigation System
//=============================================================================

static Vec2 calculateSteeringTarget(Car* car, BotState* state,
                                    const WaypointPath* path) {
    if (path->count == 0)
        return car->position;

    Vec2 carPos = car->position;
    Q16_8 lookaheadDist = LOOKAHEAD_DISTANCE;

    // Scale lookahead by speed (faster = look further ahead), but cap it
    if (car->speed > 0) {
        Q16_8 speedScale = FixedDiv(car->speed, SPEED_50CC);
        // Clamp scale between 0.5 and 1.2
        if (speedScale < (FIXED_ONE / 2)) speedScale = FIXED_ONE / 2;
        if (speedScale > (FIXED_ONE * 120 / 100)) speedScale = FIXED_ONE * 120 / 100;
        lookaheadDist = FixedMul(lookaheadDist, speedScale);
    }

    // Advance waypoint if we're close to current target (do this first!)
    Vec2 currentWP = path->waypoints[state->targetWaypoint].position;
    Q16_8 thresholdSq = FixedMul(WAYPOINT_REACH_THRESHOLD, WAYPOINT_REACH_THRESHOLD);
    if (Vec2_DistanceSquared(carPos, currentWP) < thresholdSq) {
        state->targetWaypoint = (state->targetWaypoint + 1) % path->count;
    }

    // Find best lookahead point within lookahead distance
    Vec2 lookaheadPoint = path->waypoints[state->targetWaypoint].position;
    Q16_8 lookaheadSq = FixedMul(lookaheadDist, lookaheadDist);

    // Check next few waypoints to find best lookahead point
    for (int offset = 0; offset < 5; offset++) {
        int wpIndex = (state->targetWaypoint + offset) % path->count;
        Vec2 wpPos = path->waypoints[wpIndex].position;
        Q16_8 distSq = Vec2_DistanceSquared(carPos, wpPos);

        // Use the furthest waypoint within lookahead range
        if (distSq <= lookaheadSq) {
            lookaheadPoint = wpPos;
            state->nextWaypoint = wpIndex;
        } else {
            // Stop searching once we exceed lookahead range
            break;
        }
    }

    return lookaheadPoint;
}

static Vec2 applyNavigationMistakes(Vec2 targetPos, BotState* state,
                                    BotPersonality personality) {
    // Consistency determines mistake frequency
    if (state->mistakeTimer <= 0) {
        // Time for a new mistake
        int mistakeDuration = MISTAKE_DURATION_MIN +
                              (rand() % (MISTAKE_DURATION_MAX - MISTAKE_DURATION_MIN));
        state->correctionTimer = mistakeDuration;

        // Calculate mistake frequency based on consistency
        int avgInterval =
            MISTAKE_INTERVAL_BASE +
            FixedToInt(FixedMul(personality.consistency, IntToFixed(200)));
        state->mistakeTimer = avgInterval + (rand() % 100) - 50;
    } else {
        state->mistakeTimer--;
    }

    // Apply current mistake (oversteer/path deviation)
    if (state->correctionTimer > 0) {
        state->correctionTimer--;

        // Random lateral offset from target (reduced to prevent going off-track)
        int offsetAmount =
            MISTAKE_OFFSET_MIN / 2 + (rand() % ((MISTAKE_OFFSET_MAX - MISTAKE_OFFSET_MIN) / 2));
        int direction = (rand() % 2) ? 1 : -1;

        // Apply smaller offset (mostly lateral, less forward deviation)
        Vec2 offset =
            Vec2_FromInt(offsetAmount * direction / 2, offsetAmount * direction / 3);
        targetPos = Vec2_Add(targetPos, offset);
    }

    return targetPos;
}

//=============================================================================
// Item Collection System
//=============================================================================

static bool findNearestItemBox(Car* car, BotState* state, const RaceState* raceState) {
    // Get item box spawns from Items system
    int boxCount = 0;
    const ItemBoxSpawn* boxes = Items_GetBoxSpawns(&boxCount);

    if (boxes == NULL)
        return false;

    Vec2 carPos = car->position;
    Vec2 carForward = Vec2_FromAngle(car->angle512);
    Q16_8 searchRadiusSq = FixedMul(ITEM_SEARCH_RADIUS, ITEM_SEARCH_RADIUS);
    Q16_8 nearestDistSq = searchRadiusSq;
    bool found = false;

    for (int i = 0; i < boxCount; i++) {
        if (!boxes[i].active)
            continue;

        Vec2 boxPos = boxes[i].position;
        Q16_8 distSq = Vec2_DistanceSquared(carPos, boxPos);

        if (distSq < nearestDistSq) {
            // Check if box is roughly ahead
            Vec2 toBox = Vec2_Sub(boxPos, carPos);
            Q16_8 dotProduct = Vec2_Dot(Vec2_Normalize(toBox), carForward);

            if (dotProduct > 0) {
                state->targetItemBoxPos = boxPos;
                nearestDistSq = distSq;
                found = true;
            }
        }
    }

    return found;
}

static bool shouldSeekItemBox(Car* car, BotState* state, BotPersonality personality) {
    // Don't seek if already have item
    if (car->item != ITEM_NONE)
        return false;

    // Personality-based priority check
    int roll = rand() % 256;
    if (roll > personality.itemPriority)
        return false;

    // Position-based logic
    if (car->rank >= 5) {
        return true;  // Back of pack - always seek
    } else if (car->rank <= 2) {
        // Leaders - only if very close
        Q16_8 deviationDist = Vec2_Distance(car->position, state->targetItemBoxPos);
        return (deviationDist < IntToFixed(40));
    }

    return true;  // Mid-pack - seek moderately
}

static Vec2 calculateItemInterceptPoint(Vec2 carPos, Vec2 itemPos,
                                        Vec2 racingLineTarget) {
    // Blend between racing line and item position
    Q16_8 distToItem = Vec2_Distance(carPos, itemPos);

    // Closer to item = stronger weight toward item
    Q16_8 itemWeight = FIXED_ONE - FixedDiv(distToItem, ITEM_SEARCH_RADIUS);
    if (itemWeight < 0)
        itemWeight = 0;
    Q16_8 lineWeight = FIXED_ONE - itemWeight;

    Vec2 blended;
    blended.x =
        FixedMul(itemPos.x, itemWeight) + FixedMul(racingLineTarget.x, lineWeight);
    blended.y =
        FixedMul(itemPos.y, itemWeight) + FixedMul(racingLineTarget.y, lineWeight);

    return blended;
}

//=============================================================================
// Hazard Avoidance System
//=============================================================================

static bool detectHazardsAhead(Car* car, BotState* state) {
    int itemCount = 0;
    const TrackItem* items = Items_GetActiveItems(&itemCount);

    if (items == NULL)
        return false;

    Vec2 carPos = car->position;
    Vec2 carForward = Vec2_FromAngle(car->angle512);
    Q16_8 detectRangeSq = FixedMul(HAZARD_DETECT_RANGE, HAZARD_DETECT_RANGE);

    bool hazardFound = false;
    Q16_8 nearestDistSq = detectRangeSq;

    for (int i = 0; i < itemCount; i++) {
        const TrackItem* item = &items[i];
        if (!item->active)
            continue;

        // Only care about stationary hazards
        if (item->type != ITEM_OIL && item->type != ITEM_BANANA &&
            item->type != ITEM_BOMB) {
            continue;
        }

        Vec2 itemPos = item->position;
        Q16_8 distSq = Vec2_DistanceSquared(carPos, itemPos);

        if (distSq > detectRangeSq)
            continue;

        // Check if hazard is ahead (dot product test)
        Vec2 toHazard = Vec2_Sub(itemPos, carPos);
        Vec2 toHazardNorm = Vec2_Normalize(toHazard);
        Q16_8 dotProduct = Vec2_Dot(toHazardNorm, carForward);

        // Hazard is ahead if dot > 0.5 (~60 degree cone)
        if (dotProduct > (FIXED_ONE / 2)) {
            if (distSq < nearestDistSq) {
                state->nearestHazardPos = itemPos;
                nearestDistSq = distSq;
                hazardFound = true;
            }
        }
    }

    return hazardFound;
}

static Vec2 calculateAvoidanceVector(Car* car, Vec2 hazardPos) {
    Vec2 carPos = car->position;
    Vec2 toHazard = Vec2_Sub(hazardPos, carPos);

    // Calculate perpendicular vectors (left and right dodge)
    Vec2 dodgeLeft = Vec2_Perp(toHazard);
    Vec2 dodgeRight = Vec2_PerpCW(toHazard);

    // Choose dodge direction toward track center (512, 512)
    Vec2 leftTarget =
        Vec2_Add(carPos, Vec2_Scale(Vec2_Normalize(dodgeLeft), IntToFixed(60)));
    Vec2 rightTarget =
        Vec2_Add(carPos, Vec2_Scale(Vec2_Normalize(dodgeRight), IntToFixed(60)));

    Vec2 center = Vec2_FromInt(512, 512);
    Q16_8 leftDist = Vec2_DistanceSquared(leftTarget, center);
    Q16_8 rightDist = Vec2_DistanceSquared(rightTarget, center);

    return (leftDist < rightDist) ? leftTarget : rightTarget;
}

static Vec2 blendAvoidanceWithRacingLine(Vec2 racingTarget, Vec2 avoidanceTarget,
                                         int avoidanceTimer) {
    // Avoidance priority fades over time
    if (avoidanceTimer > 15) {
        return avoidanceTarget;  // Full avoidance
    } else if (avoidanceTimer > 0) {
        // Blend proportionally
        Q16_8 avoidWeight = FixedDiv(IntToFixed(avoidanceTimer), IntToFixed(15));
        Q16_8 raceWeight = FIXED_ONE - avoidWeight;

        Vec2 blended;
        blended.x = FixedMul(avoidanceTarget.x, avoidWeight) +
                    FixedMul(racingTarget.x, raceWeight);
        blended.y = FixedMul(avoidanceTarget.y, avoidWeight) +
                    FixedMul(racingTarget.y, raceWeight);
        return blended;
    }

    return racingTarget;
}

//=============================================================================
// Item Usage Strategy
//=============================================================================

static void updateItemUsage(Car* car, BotState* state, BotPersonality personality,
                            const RaceState* raceState) {
    // Cooldown between item uses
    if (state->itemUsageTimer > 0) {
        state->itemUsageTimer--;
        return;
    }

    if (car->item == ITEM_NONE)
        return;

    bool shouldUseItem = false;
    bool fireForward = true;

    switch (car->item) {
        case ITEM_SPEEDBOOST:
            // Use on straights, not in corners
            shouldUseItem = isOnStraightaway(state);
            break;

        case ITEM_BANANA:
        case ITEM_OIL:
        case ITEM_BOMB:
            // Defensive: Drop when in lead OR offensive: Drop in traffic
            if (car->rank <= 2) {
                shouldUseItem = (rand() % 100) < 70;  // 70% chance
            } else {
                shouldUseItem = (rand() % 100) < 30;  // 30% chance
            }
            fireForward = false;
            break;

        case ITEM_GREEN_SHELL:
            // Use if someone is ahead and close
            shouldUseItem = isCarAheadInRange(car, raceState, IntToFixed(80));
            fireForward = true;
            break;

        case ITEM_RED_SHELL:
        case ITEM_MISSILE:
            // Use based on aggression
            shouldUseItem = (rand() % 256) < personality.aggression;
            fireForward = true;
            break;

        case ITEM_MUSHROOM:
            // Use when rival is close
            shouldUseItem = isCarAheadInRange(car, raceState, IntToFixed(50));
            break;

        default:
            break;
    }

    if (shouldUseItem) {
        Items_UsePlayerItem(car, fireForward);
        state->itemUsageTimer = 60 + (rand() % 120);  // 1-3 second cooldown
    }
}

static bool isOnStraightaway(BotState* state) {
    const WaypointPath* path = &currentTrackWaypoints;
    int currentWP = state->targetWaypoint;

    if (path->count < 3)
        return true;

    int angle1 = path->waypoints[currentWP % path->count].cornerAngle512;
    int angle2 = path->waypoints[(currentWP + 1) % path->count].cornerAngle512;
    int angle3 = path->waypoints[(currentWP + 2) % path->count].cornerAngle512;

    int diff1 = abs(angle2 - angle1);
    int diff2 = abs(angle3 - angle2);

    // Small angle changes = straight
    return (diff1 < 30 && diff2 < 30);
}

static bool isCarAheadInRange(Car* car, const RaceState* raceState, Q16_8 range) {
    for (int i = 0; i < raceState->carCount; i++) {
        const Car* other = &raceState->cars[i];
        if (other == car)
            continue;

        // Check if ahead in race position
        if (other->rank < car->rank) {
            Q16_8 dist = Vec2_Distance(car->position, other->position);
            if (dist < range) {
                // Check if actually in front spatially
                Vec2 toCar = Vec2_Sub(other->position, car->position);
                Vec2 forward = Vec2_FromAngle(car->angle512);
                if (Vec2_Dot(toCar, forward) > 0) {
                    return true;
                }
            }
        }
    }
    return false;
}

//=============================================================================
// Adaptive Behaviors
//=============================================================================

static void applyRubberBanding(Car* car, const RaceState* raceState) {
    const Car* player = &raceState->cars[raceState->playerIndex];

    Q16_8 distToPlayer = Vec2_Distance(car->position, player->position);

    // If bot is far behind player, give speed boost
    if (distToPlayer > RUBBERBAND_BOOST_THRESHOLD) {
        car->maxSpeed = FixedMul(SPEED_50CC, RUBBERBAND_BOOST_MULT);
    }
    // If bot is far ahead of player, slow down slightly
    else if (distToPlayer > RUBBERBAND_SLOW_THRESHOLD && car->rank < player->rank) {
        car->maxSpeed = FixedMul(SPEED_50CC, RUBBERBAND_SLOW_MULT);
    }
    // Normal range - reset to base speed
    else {
        car->maxSpeed = SPEED_50CC;
    }
}

static BotPersonality applyPositionTactics(const BotState* state, const Car* car,
                                           BotPersonality basePersonality) {
    (void)state;
    BotPersonality adjusted = basePersonality;

    if (car->rank <= 2) {
        // LEADER TACTICS - conservative
        adjusted.itemPriority = adjusted.itemPriority / 2;
    } else if (car->rank >= 6) {
        // BACK-PACK TACTICS - aggressive
        Q16_8 doubleAggression = adjusted.aggression * 2;
        if (doubleAggression > FIXED_ONE)
            doubleAggression = FIXED_ONE;
        adjusted.aggression = doubleAggression;
        adjusted.itemPriority = FIXED_ONE;
    }

    return adjusted;
}

static void updateOvertaking(Car* car, BotState* state, const RaceState* raceState) {
    Vec2 carPos = car->position;
    Vec2 carForward = Vec2_FromAngle(car->angle512);

    bool blockingCarFound = false;
    Vec2 blockerPos = Vec2_Zero();

    for (int i = 0; i < raceState->carCount; i++) {
        const Car* other = &raceState->cars[i];
        if (other == car)
            continue;

        Vec2 toOther = Vec2_Sub(other->position, carPos);
        Q16_8 dist = Vec2_Len(toOther);

        if (dist < OVERTAKE_DISTANCE) {
            Vec2 toOtherNorm = Vec2_Normalize(toOther);
            Q16_8 dot = Vec2_Dot(toOtherNorm, carForward);

            // Other car is directly ahead (dot > 0.7)
            if (dot > (FIXED_ONE * 180 / 256)) {
                if (other->speed < car->speed) {
                    blockingCarFound = true;
                    blockerPos = other->position;
                    break;
                }
            }
        }
    }

    if (blockingCarFound) {
        state->isOvertaking = true;

        // Calculate overtake point (offset to the side)
        Vec2 perpendicular = Vec2_Perp(carForward);
        int sideOffset = (rand() % 2) ? 50 : -50;
        state->overtakeTarget =
            Vec2_Add(blockerPos, Vec2_Scale(perpendicular, IntToFixed(sideOffset)));
    } else if (state->isOvertaking) {
        // No longer blocked
        Q16_8 distToOvertakeTarget = Vec2_Distance(carPos, state->overtakeTarget);
        if (distToOvertakeTarget < IntToFixed(30)) {
            state->isOvertaking = false;
        }
    }
}

static QuadrantID determineQuadrantAtPos(int x, int y) {
    int col = (x < QUAD_OFFSET) ? 0 : (x < 2 * QUAD_OFFSET) ? 1 : 2;
    int row = (y < QUAD_OFFSET) ? 0 : (y < 2 * QUAD_OFFSET) ? 1 : 2;
    return (QuadrantID)(row * QUADRANT_GRID_SIZE + col);
}

static bool applyWallAvoidance(Car* car, Vec2* steeringTarget) {
    Vec2 forward = Vec2_FromAngle(car->angle512);
    Vec2 aheadPos = Vec2_Add(car->position, Vec2_Scale(forward, IntToFixed(50)));
    int aheadX = FixedToInt(aheadPos.x);
    int aheadY = FixedToInt(aheadPos.y);

    QuadrantID quad = determineQuadrantAtPos(aheadX, aheadY);
    bool collisionLikely = Wall_CheckCollision(aheadX, aheadY, CAR_RADIUS + 8, quad);

    // Also guard against outer bounds (matching clamp limits)
    int minBound = CAR_RADIUS + 10;
    int maxBound = MAP_SIZE_PX - 32 - 10;
    if (aheadX < minBound || aheadY < minBound || aheadX > maxBound ||
        aheadY > maxBound) {
        collisionLikely = true;
    }

    if (!collisionLikely) {
        return false;
    }

    int nx = 0, ny = 0;
    Wall_GetCollisionNormal(aheadX, aheadY, quad, &nx, &ny);

    Vec2 avoidanceDir;
    if (nx == 0 && ny == 0) {
        Vec2 toCenter = Vec2_Sub(Vec2_FromInt(512, 512), car->position);
        avoidanceDir =
            Vec2_Normalize(Vec2_IsZero(toCenter) ? Vec2_Perp(forward) : toCenter);
    } else {
        avoidanceDir = Vec2_FromInt(nx, ny);
    }

    Vec2 avoidanceTarget = Vec2_Add(
        car->position, Vec2_Scale(Vec2_Normalize(avoidanceDir), IntToFixed(80)));

    // Strong avoidance weight (70% avoidance, 30% racing line)
    steeringTarget->x = (steeringTarget->x * 3 + avoidanceTarget.x * 7) / 10;
    steeringTarget->y = (steeringTarget->y * 3 + avoidanceTarget.y * 7) / 10;

    // Brake more aggressively to prevent wall grinding
    if (car->speed > IntToFixed(2)) {
        Car_Brake(car);
    }

    return true;
}

static int findNearestCheckpointIndex(Vec2 position) {
    int bestIndex = -1;
    Q16_8 bestDistSq = IntToFixed(10000);  // Large number (in pixels^2)

    for (int i = 0; i < currentTrackWaypoints.count; i++) {
        if (!currentTrackWaypoints.waypoints[i].isCheckpoint) {
            continue;
        }
        Q16_8 distSq = Vec2_DistanceSquared(
            position, currentTrackWaypoints.waypoints[i].position);
        if (distSq < bestDistSq) {
            bestDistSq = distSq;
            bestIndex = i;
        }
    }

    return bestIndex;
}

static bool teleportToKnownSafeSpot(Car* car, BotState* state) {
    int x = FixedToInt(car->position.x);
    int y = FixedToInt(car->position.y);

    bool inStuckRegion = (x >= STUCK_REGION_MIN_X && x <= STUCK_REGION_MAX_X &&
                          y >= STUCK_REGION_MIN_Y && y <= STUCK_REGION_MAX_Y);
    if (!inStuckRegion) {
        return false;
    }

    // Teleport to predefined rescue spot and face down-track
    Car_SetPosition(car, STUCK_RESCUE_POS);
    car->speed = 0;
    Vec2 facingVec = Vec2_Sub(STUCK_RESCUE_FACE_TARGET, STUCK_RESCUE_POS);
    Car_SetAngle(car, Vec2_ToAngle(facingVec));

    // Align waypoint targets near the rescue spot
    int checkpointIndex = findNearestCheckpointIndex(STUCK_RESCUE_POS);
    if (checkpointIndex < 0) {
        checkpointIndex = 0;
    }
    state->targetWaypoint = checkpointIndex;
    state->nextWaypoint = (checkpointIndex + 1) % currentTrackWaypoints.count;
    state->targetPosition = currentTrackWaypoints.waypoints[state->targetWaypoint].position;
    state->stuckTimer = 0;
    state->stuckStillFrames = 0;
    state->hazardAvoidanceTimer = 0;

    return true;
}

static void teleportToCheckpoint(Car* car, BotState* state, int checkpointIndex) {
    if (checkpointIndex < 0 || checkpointIndex >= currentTrackWaypoints.count) {
        return;
    }

    const Waypoint* wp = &currentTrackWaypoints.waypoints[checkpointIndex];
    Car_SetPosition(car, wp->position);
    car->speed = 0;
    Car_SetAngle(car, wp->cornerAngle512);

    state->targetWaypoint = checkpointIndex;
    state->nextWaypoint = (checkpointIndex + 1) % currentTrackWaypoints.count;
    state->targetPosition = wp->position;
    state->stuckTimer = 0;
    state->stuckStillFrames = 0;
    state->hazardAvoidanceTimer = 0;
}

//=============================================================================
// Control Execution
//=============================================================================

static void executeSteeringControl(Car* car, Vec2 targetPos) {
    // Calculate desired angle to target
    Vec2 toTarget = Vec2_Sub(targetPos, car->position);
    int desiredAngle = Vec2_ToAngle(toTarget);
    int currentAngle = car->angle512;

    // Calculate angle difference (shortest rotation)
    int angleDiff = (desiredAngle - currentAngle) & ANGLE_MASK;
    if (angleDiff > ANGLE_HALF) {
        angleDiff -= ANGLE_FULL;
    }

    // Apply steering with smooth turning
    if (angleDiff > 0) {
        int turnAmount = (angleDiff > TURN_STEP_50CC) ? TURN_STEP_50CC : angleDiff;
        Car_Steer(car, turnAmount);
    } else if (angleDiff < 0) {
        int turnAmount = (angleDiff < -TURN_STEP_50CC) ? -TURN_STEP_50CC : angleDiff;
        Car_Steer(car, turnAmount);
    }
}

static void executeAccelerationControl(Car* car, BotState* state) {
    // Get target speed for current waypoint
    Q16_8 targetSpeed =
        currentTrackWaypoints.waypoints[state->targetWaypoint].targetSpeed;

    // Adjust for skill level (easy bots overshoot speed in corners)
    if (state->personality.skillLevel == SKILL_EASY) {
        targetSpeed = FixedMul(targetSpeed, IntToFixed(110) / 100);
    }

    // Accelerate or brake based on target speed
    if (car->speed < targetSpeed) {
        Car_Accelerate(car);
    } else if (car->speed > targetSpeed + IntToFixed(1)) {
        Car_Brake(car);
    }
    // else: coasting (no input)
}
