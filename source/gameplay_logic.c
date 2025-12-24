#include "gameplay_logic.h"

#include <nds.h>

#include "timer.h"

//=============================================================================
// Constants - scale with tick rate for consistent feel
//=============================================================================

#define PLAYER_KEYS (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_A | KEY_B)

#define TURN_STEP_50CC 3  // units of 0..511 per tick @ 60Hz  (~127 deg/sec)

//=============================================================================
// Module State
//=============================================================================
static RaceState KartMania;

static volatile u16 g_keysHeld = 0;

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
    DisableInput();
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

        car->maxSpeed = (FIXED_ONE * 75) / 100;

        // Minimum accel must clear Car_Update's snap-to-zero threshold (<= 1).
        // Use 1.0 in Q16.8 so a single press produces visible movement even with friction.
        car->accelRate = IntToFixed(1);

        car->friction = 250;
    }

    // TODO: Load checkpoints for this map
    KartMania.checkpointCount = 0;

    // Start input and physics timer
    PlayerInput_Init();
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
    }

    // Restart
    PlayerInput_Init();
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

    // Read current key state
    u16 keys = g_keysHeld;

    int targetAngle = player->angle512;  // default: keep heading if no D-pad

    // Determine targetAngle from D-pad (same logic you already have)
    if ((keys & KEY_UP) && (keys & KEY_RIGHT)) {
        targetAngle = ANGLE_UP_RIGHT;
    } else if ((keys & KEY_UP) && (keys & KEY_LEFT)) {
        targetAngle = ANGLE_UP_LEFT;
    } else if ((keys & KEY_DOWN) && (keys & KEY_RIGHT)) {
        targetAngle = ANGLE_DOWN_RIGHT;
    } else if ((keys & KEY_DOWN) && (keys & KEY_LEFT)) {
        targetAngle = ANGLE_DOWN_LEFT;
    } else if (keys & KEY_UP) {
        targetAngle = ANGLE_UP;
    } else if (keys & KEY_DOWN) {
        targetAngle = ANGLE_DOWN;
    } else if (keys & KEY_LEFT) {
        targetAngle = ANGLE_LEFT;
    } else if (keys & KEY_RIGHT) {
        targetAngle = ANGLE_RIGHT;
    }

    // Steer gradually toward targetAngle
    int diff = AngleDiff512(targetAngle, player->angle512);
    if (diff > TURN_STEP_50CC)
        diff = TURN_STEP_50CC;
    if (diff < -TURN_STEP_50CC)
        diff = -TURN_STEP_50CC;

    player->angle512 = (player->angle512 + diff) & ANGLE_MASK;

    // A = accelerate in current facing direction
    if (keys & KEY_A) {
        Car_Accelerate(player);
    }

    // B = brake
    if (keys & KEY_B) {
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

//=============================================================================
// Input Handling
//=============================================================================
void PlayerInput_Init(void) {
    g_keysHeld = 0;

    // Configure keys to trigger interrupt
    REG_KEYCNT = BIT(14) | PLAYER_KEYS;

    irqSet(IRQ_KEYS, &PlayerInput_Apply_ISR);
    irqEnable(IRQ_KEYS);
}

void PlayerInput_Apply_ISR(void) {
    if (!KartMania.raceStarted || KartMania.raceFinished) {
        return;
    }

    // Capture key state (KEYINPUT is active-low)
    g_keysHeld = (u16)(~REG_KEYINPUT) & PLAYER_KEYS;
}

void DisableInput(void) {
    irqDisable(IRQ_KEYS);
    irqClear(IRQ_KEYS);
    g_keysHeld = 0;
}
