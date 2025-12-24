#include "gameplay_logic.h"

#include <nds.h>

#include "timer.h"

//=============================================================================
// Constants
//=============================================================================

#define TURN_STEP_50CC 3  // units per tick @ 60Hz (~127 deg/sec)

// Physics tuning (50cc class, Q16.8 format, 60Hz)
#define SPEED_50CC ((FIXED_ONE * 3) / 4)  // 0.75 px/frame = 192
#define ACCEL_50CC IntToFixed(1)          // 1.0 px/frame
#define FRICTION_50CC 240                 // 240/256 = 0.9375

// World directions (0-511 binary angle)
#define ANGLE_RIGHT 0        // 0°
#define ANGLE_DOWN 128       // 90°
#define ANGLE_LEFT 256       // 180°
#define ANGLE_UP 384         // 270°
#define ANGLE_DOWN_RIGHT 64  // 45°
#define ANGLE_DOWN_LEFT 192  // 135°
#define ANGLE_UP_LEFT 320    // 225°
#define ANGLE_UP_RIGHT 448   // 315°

// Map configuration
static const int MapLaps[] = {
    [NONEMAP] = 0,
    [ScorchingSands] = 10,
    [AlpinRush] = 10,
    [NeonCircuit] = 10,
};

//=============================================================================
// Module State
//=============================================================================

static RaceState KartMania;

//=============================================================================
// Private Prototypes
//=============================================================================

static inline int AngleDiff512(int target, int current);
static int getTargetAngleFromDpad(uint32 held, int currentAngle, bool* hasDpadInput);
static void initCarAtSpawn(Car* car, int index);
static void handlePlayerInput(Car* player);
static void clampToMapBounds(Car* car);

//=============================================================================
// Public API - Accessors
//=============================================================================

RaceState* Race_GetState(void) {
    return &KartMania;
}

Car* Race_GetPlayerCar(void) {
    return &KartMania.cars[KartMania.playerIndex];
}

//=============================================================================
// Public API - Lifecycle
//=============================================================================

void Race_Init(Map map, GameMode mode) {
    if (map == NONEMAP || map > NeonCircuit) {
        return;
    }

    // Configure race
    KartMania.currentMap = map;
    KartMania.totalLaps = MapLaps[map];
    KartMania.gameMode = mode;
    KartMania.playerIndex = 0;
    KartMania.raceStarted = true;
    KartMania.raceFinished = false;

    // Set car count based on mode
    KartMania.carCount = (mode == SinglePlayer) ? MAX_CARS : 1;

    // Initialize all cars
    for (int i = 0; i < KartMania.carCount; i++) {
        initCarAtSpawn(&KartMania.cars[i], i);
    }

    // TODO: Load checkpoints for this map
    KartMania.checkpointCount = 0;

    RaceTick_TimerInit();
}

void Race_Reset(void) {
    if (KartMania.currentMap == NONEMAP) {
        return;
    }

    RaceTick_TimerStop();

    KartMania.raceStarted = true;
    KartMania.raceFinished = false;

    for (int i = 0; i < KartMania.carCount; i++) {
        initCarAtSpawn(&KartMania.cars[i], i);
    }

    RaceTick_TimerInit();
}

void stop_Race(void) {
    KartMania.raceStarted = false;
    RaceTick_TimerStop();
}

//=============================================================================
// Public API - Game Loop
//=============================================================================

void Race_Tick(void) {
    if (!KartMania.raceStarted || KartMania.raceFinished) {
        return;
    }

    Car* player = &KartMania.cars[KartMania.playerIndex];

    handlePlayerInput(player);
    Car_Update(player);
    clampToMapBounds(player);

    // TODO: Update AI cars
    // TODO: Check checkpoint crossings
    // TODO: Update rankings
    // TODO: Check win condition
}

//=============================================================================
// Private Implementation - Helpers
//=============================================================================

static inline int AngleDiff512(int target, int current) {
    int d = (target - current) & ANGLE_MASK;
    if (d > ANGLE_HALF) {
        d -= ANGLE_FULL;
    }
    return d;
}

static void initCarAtSpawn(Car* car, int index) {
    // TODO: Get spawn position from map data
    Vec2 spawnPos = Vec2_FromInt(100 + (index * 40), 100);

    car->position = spawnPos;
    car->velocity = Vec2_Zero();
    car->angle512 = 0;
    car->Lap = 0;
    car->lastCheckpoint = 0;
    car->rank = index + 1;
    car->item = ITEM_NONE;

    car->maxSpeed = SPEED_50CC;
    car->accelRate = ACCEL_50CC;
    car->friction = FRICTION_50CC;
}

//=============================================================================
// Private Implementation - Input
//=============================================================================

static int getTargetAngleFromDpad(uint32 held, int currentAngle, bool* hasDpadInput) {
    bool up = held & KEY_UP;
    bool down = held & KEY_DOWN;
    bool left = held & KEY_LEFT;
    bool right = held & KEY_RIGHT;

    *hasDpadInput = false;

    if (right && !left) {
        *hasDpadInput = true;
        if (up)
            return ANGLE_UP_RIGHT;
        if (down)
            return ANGLE_DOWN_RIGHT;
        return ANGLE_RIGHT;
    }
    if (left && !right) {
        *hasDpadInput = true;
        if (up)
            return ANGLE_UP_LEFT;
        if (down)
            return ANGLE_DOWN_LEFT;
        return ANGLE_LEFT;
    }
    if (up && !down) {
        *hasDpadInput = true;
        return ANGLE_UP;
    }
    if (down && !up) {
        *hasDpadInput = true;
        return ANGLE_DOWN;
    }

    return currentAngle;
}

static void handlePlayerInput(Car* player) {
    uint32 held = keysHeld();

    bool pressingA = held & KEY_A;
    bool pressingB = held & KEY_B;
    bool hasDpadInput = false;

    int targetAngle = getTargetAngleFromDpad(held, player->angle512, &hasDpadInput);

    // Steering: only turn while accelerating AND pressing D-pad
    if (pressingA && hasDpadInput) {
        int diff = AngleDiff512(targetAngle, player->angle512);

        if (diff > TURN_STEP_50CC)
            diff = TURN_STEP_50CC;
        if (diff < -TURN_STEP_50CC)
            diff = -TURN_STEP_50CC;

        player->angle512 = (player->angle512 + diff) & ANGLE_MASK;
    }

    // Acceleration (A) and Braking (B)
    if (pressingA && !pressingB) {
        Car_Accelerate(player);
    } else if (pressingB) {
        Car_Brake(player);
    }
}

//=============================================================================
// Private Implementation - Physics
//=============================================================================

static void clampToMapBounds(Car* car) {
    Q16_8 minPos = IntToFixed(0);
    Q16_8 maxPosX = IntToFixed(1024 - 32);
    Q16_8 maxPosY = IntToFixed(1024 - 32);

    if (car->position.x < minPos)
        car->position.x = minPos;
    if (car->position.y < minPos)
        car->position.y = minPos;
    if (car->position.x > maxPosX)
        car->position.x = maxPosX;
    if (car->position.y > maxPosY)
        car->position.y = maxPosY;

    //todo add circuit walls 
}