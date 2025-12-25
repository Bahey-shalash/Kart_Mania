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
// Module State (PRIVATE - do not expose)
//=============================================================================

static RaceState KartMania;

//=============================================================================
// Private Prototypes
//=============================================================================

static inline int AngleDiff512(int target, int current);
static void initCarAtSpawn(Car* car, int index);
static void handlePlayerInput(Car* player);
static void clampToMapBounds(Car* car);

//=============================================================================
// Public API - State Queries (Read-Only)
//=============================================================================

const RaceState* Race_GetState(void) {
    return &KartMania;
}

const Car* Race_GetPlayerCar(void) {
    return &KartMania.cars[KartMania.playerIndex];
}

bool Race_IsActive(void) {
    return KartMania.raceStarted && !KartMania.raceFinished;
}

int Race_GetLapCount(void) {
    return KartMania.totalLaps;
}

//=============================================================================
// Public API - State Modification (Controlled)
//=============================================================================

void Race_Init(Map map, GameMode mode) {
    init_pause_interupt();
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

void Race_Stop(void) {
    KartMania.raceStarted = false;
    RaceTick_TimerStop();
}

//=============================================================================
// Public API - Game Loop (Physics Tick)
//=============================================================================

void Race_Tick(void) {
    if (!Race_IsActive()) {
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
    car->speed = 0;
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

static void handlePlayerInput(Car* player) {
    uint32 held = keysHeld();

    bool pressingA = held & KEY_A;
    bool pressingB = held & KEY_B;
    bool pressingLeft = held & KEY_LEFT;
    bool pressingRight = held & KEY_RIGHT;

    // Steering: turn left/right relative to current facing
    // Only allow steering while accelerating (like Mario Kart)
    if (pressingA) {
        if (pressingLeft && !pressingRight) {
            // Turn left (CCW) - use Car_Steer to keep speed aligned with facing
            Car_Steer(player, -TURN_STEP_50CC);
        } else if (pressingRight && !pressingLeft) {
            // Turn right (CW) - use Car_Steer to keep speed aligned with facing
            Car_Steer(player, TURN_STEP_50CC);
        }
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

    // TODO: Add circuit walls
}
static bool isPaused = false;

void init_pause_interupt(void) {
    REG_KEYCNT = BIT(14) | KEY_START;
    irqSet(IRQ_KEYS, PauseISR);
    irqEnable(IRQ_KEYS);
}

void PauseISR(void) {
    isPaused = !isPaused;

    if (isPaused) {
        RaceTick_TimerPause();
    } else {
        RaceTick_TimerEnable();
    }
}