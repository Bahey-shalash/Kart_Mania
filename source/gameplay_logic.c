#include "gameplay_logic.h"

#include <nds.h>

#include "timer.h"

//=============================================================================
// Constants - scale with tick rate for consistent feel
//=============================================================================

#define TURN_STEP_50CC 3  // units of 0..511 per tick @ 60Hz  (~127 deg/sec)

//=============================================================================
// Module State
//=============================================================================
static RaceState KartMania;

static inline int AngleDiff512(int target, int current) {
    int d = (target - current) & ANGLE_MASK;  // 0..511
    if (d > ANGLE_HALF)
        d -= ANGLE_FULL;  // -256..+255
    return d;
}

//=============================================================================
// Accessors
//=============================================================================
RaceState* Race_GetState(void) {
    return &KartMania;
}

Car* Race_GetPlayerCar(void) {
    return &KartMania.cars[KartMania.playerIndex];
}

//============================================================
// Race Lifecycle
//============================================================
void stop_Race(void) {
    KartMania.raceStarted = false;
    RaceTick_TimerStop();
}

void Race_Init(Map map, GameMode mode) {
    // Validate map
    if (map == NONEMAP || map > NeonCircuit) {
        return;
    }

    // Set map and laps
    KartMania.currentMap = map;
    KartMania.totalLaps = MapLaps[map];

    // Set mode
    KartMania.gameMode = mode;

    // Set car count based on mode
    switch (mode) {
        case SinglePlayer:
            KartMania.carCount = MAX_CARS;
            break;
        case MultiPlayer:
            KartMania.carCount = 1;  // placeholder
            break;
    }

    // Player is always car 0
    KartMania.playerIndex = 0;

    // Race state
    KartMania.raceStarted = true;
    KartMania.raceFinished = false;

    // Initialize all cars
    for (int i = 0; i < KartMania.carCount; i++) {
        Car* car = &KartMania.cars[i];

        // TODO: Get spawn position from map data
        Vec2 spawnPos = Vec2_FromInt(100 + (i * 40), 100);  // placeholder

        car->position = spawnPos;
        car->velocity = Vec2_Zero();
        car->angle512 = 0;  // facing right
        car->Lap = 0;
        car->lastCheckpoint = 0;
        car->rank = i + 1;
        car->item = ITEM_NONE;

        // =============================================================================
        // Physics tuned for 50cc (Q16.8, 60Hz)
        // =============================================================================

        // Max speed: 0.75 pixels/frame in Q16.8 = 192
        car->maxSpeed = (FIXED_ONE * 3) / 4;  // 192

        // Acceleration: 1.0 px/frame in Q16.8 (clamped by maxSpeed)
        // Using a full fixed-point unit keeps velocity above the snap-to-zero guard.
        car->accelRate = IntToFixed(1);

        // Friction: multiplier per frame. Lower = more drag.
        // 240/256 = 0.9375 -> loses 6.25% speed per frame
        // At 60Hz, coasting from max speed to stop takes ~40 frames (~0.7 sec)
        car->friction = 240;
    }

    // TODO: Load checkpoints for this map
    KartMania.checkpointCount = 0;

    // Start physics timer
    RaceTick_TimerInit();
}

void Race_Reset(void) {
    if (KartMania.currentMap == NONEMAP) {
        return;
    }

    // Stop timer first
    RaceTick_TimerStop();

    // Reset race state
    KartMania.raceStarted = true;
    KartMania.raceFinished = false;

    // Reset all cars
    for (int i = 0; i < KartMania.carCount; i++) {
        Car* car = &KartMania.cars[i];

        Vec2 spawnPos = Vec2_FromInt(100 + (i * 40), 100);

        car->position = spawnPos;
        car->velocity = Vec2_Zero();
        car->angle512 = 0;
        car->Lap = 0;
        car->lastCheckpoint = 0;
        car->rank = i + 1;
        car->item = ITEM_NONE;

        car->maxSpeed = (FIXED_ONE * 75) / 100;
        car->accelRate = IntToFixed(1);
        car->friction = 240;
    }

    // Restart timer
    RaceTick_TimerInit();
}

//=============================================================================
// Race Tick - called by TIMER0 ISR at RACE_TICK_FREQ Hz
//=============================================================================

// Angle constants for world directions (0-511 binary angle)
#define ANGLE_RIGHT 0   // 0° - facing east
#define ANGLE_DOWN 128  // 90° - facing south
#define ANGLE_LEFT 256  // 180° - facing west
#define ANGLE_UP 384    // 270° - facing north

// Diagonal angles
#define ANGLE_UP_RIGHT 448   // 315° - NE
#define ANGLE_UP_LEFT 320    // 225° - NW
#define ANGLE_DOWN_RIGHT 64  // 45° - SE
#define ANGLE_DOWN_LEFT 192  // 135° - SW

void Race_Tick(void) {
    if (!KartMania.raceStarted || KartMania.raceFinished) {
        return;
    }

    Car* player = &KartMania.cars[KartMania.playerIndex];

    // Read current input state
    uint32 held = keysHeld();

    bool pressingA = held & KEY_A;
    bool pressingB = held & KEY_B;

    // Determine target angle from D-pad (8 directions)
    int targetAngle = player->angle512;  // default: keep current heading
    bool hasDpadInput = false;

    bool up = held & KEY_UP;
    bool down = held & KEY_DOWN;
    bool left = held & KEY_LEFT;
    bool right = held & KEY_RIGHT;

    if (right && !left) {
        hasDpadInput = true;
        if (up)
            targetAngle = ANGLE_UP_RIGHT;
        else if (down)
            targetAngle = ANGLE_DOWN_RIGHT;
        else
            targetAngle = ANGLE_RIGHT;
    } else if (left && !right) {
        hasDpadInput = true;
        if (up)
            targetAngle = ANGLE_UP_LEFT;
        else if (down)
            targetAngle = ANGLE_DOWN_LEFT;
        else
            targetAngle = ANGLE_LEFT;
    } else if (up && !down) {
        hasDpadInput = true;
        targetAngle = ANGLE_UP;
    } else if (down && !up) {
        hasDpadInput = true;
        targetAngle = ANGLE_DOWN;
    }

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

    Car_Update(player);

    // Clamp to map bounds
    Q16_8 minPos = IntToFixed(0);
    Q16_8 maxPosX = IntToFixed(1024 - 32);
    Q16_8 maxPosY = IntToFixed(1024 - 32);

    if (player->position.x < minPos)
        player->position.x = minPos;
    if (player->position.y < minPos)
        player->position.y = minPos;
    if (player->position.x > maxPosX)
        player->position.x = maxPosX;
    if (player->position.y > maxPosY)
        player->position.y = maxPosY;

    // TODO: Update AI cars
    // TODO: Check checkpoint crossings
    // TODO: Update rankings
    // TODO: Check win condition
}
