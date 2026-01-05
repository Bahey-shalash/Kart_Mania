/**
 * gameplay_logic.c
 * Core race logic, physics updates, and state management
 */

#include "../gameplay/gameplay_logic.h"

#include <nds.h>

#include "../core/game_constants.h"
#include "../core/timer.h"
#include "items/Items.h"
#include "terrain_detection.h"
#include "wall_collision.h"
#include "../network/multiplayer.h"

//=============================================================================
// Private Types
//=============================================================================

typedef enum {
    CP_STATE_START = 0,
    CP_STATE_NEED_LEFT,
    CP_STATE_NEED_DOWN,
    CP_STATE_NEED_RIGHT,
    CP_STATE_READY_FOR_LAP
} CheckpointProgressState;

//=============================================================================
// Private Constants
//=============================================================================

#define DEBOUNCE_DELAY 15  // ~250ms at 60Hz for pause button
#define NETWORK_UPDATE_INTERVAL 4  // Send updates every 4 frames (15Hz)
#define MULTIPLAYER_LAPS_SCORCHING_SANDS 5  // Special lap count for multiplayer

static const int MapLaps[] = {
    [NONEMAP] = LAPS_NONE,
    [ScorchingSands] = LAPS_SCORCHING_SANDS,
    [AlpinRush] = LAPS_ALPIN_RUSH,
    [NeonCircuit] = LAPS_NEON_CIRCUIT,
};

//=============================================================================
// Private State - Race
//=============================================================================

static RaceState raceState;
static QuadrantID loadedQuadrant = QUAD_BR;
static bool isMultiplayerRace = false;

//=============================================================================
// Private State - Input
//=============================================================================

static bool itemButtonHeldLast = false;

//=============================================================================
// Private State - Physics
//=============================================================================

static int collisionLockoutTimer[MAX_CARS] = {0};

//=============================================================================
// Private State - Lap Tracking
//=============================================================================

static bool wasAboveFinishLine[MAX_CARS] = {false};
static bool hasCompletedFirstCrossing[MAX_CARS] = {false};
static CheckpointProgressState cpState[MAX_CARS] = {CP_STATE_START};
static bool wasOnLeftSide[MAX_CARS] = {false};
static bool wasOnTopSide[MAX_CARS] = {false};

//=============================================================================
// Private State - Countdown
//=============================================================================

static CountdownState countdownState = COUNTDOWN_3;
static int countdownTimer = 0;
static bool raceCanStart = false;

//=============================================================================
// Private State - Pause
//=============================================================================

static volatile bool isPaused = false;
static volatile int debounceFrames = 0;

//=============================================================================
// Private State - Network
//=============================================================================

static int networkUpdateCounter = 0;

//=============================================================================
// Private Function Prototypes
//=============================================================================

// Initialization
static void Race_InitCarAtSpawn(Car* car, int spawnPosition);
static void Race_InitMultiplayerCars(void);
static void Race_InitSinglePlayerCars(void);
static void Race_ResetLapTracking(int carIndex);

// Update loops
static void Race_UpdateCountdownState(void);
static void Race_UpdatePhysics(void);
static void Race_UpdateNetworkSync(void);

// Input handling
static void Race_HandlePlayerInput(Car* player, int carIndex);

// Physics
static void Race_ApplyTerrainEffects(Car* car);
static void Race_ClampToMapBounds(Car* car, int carIndex);

// Lap tracking
static void Race_CheckCheckpointProgression(const Car* car, int carIndex);
static bool Race_CheckFinishLineCrossInternal(const Car* car, int carIndex);

// Utilities
static QuadrantID Race_DetermineCarQuadrant(int x, int y);

//=============================================================================
// Public API - Lifecycle
//=============================================================================

/** Initialize a new race */
void Race_Init(Map map, GameMode mode) {
    Race_InitPauseInterrupt();
    
    if (map == NONEMAP || map > NeonCircuit) {
        return;
    }

    // Initialize race state
    raceState.currentMap = map;
    raceState.gameMode = mode;
    raceState.raceStarted = true;
    raceState.raceFinished = false;
    
    raceState.finishDelayTimer = 0;
    raceState.finalTimeMin = 0;
    raceState.finalTimeSec = 0;
    raceState.finalTimeMsec = 0;

    // Initialize countdown
    countdownState = COUNTDOWN_3;
    countdownTimer = 0;
    raceCanStart = false;

    // Determine race mode
    isMultiplayerRace = (mode == MultiPlayer);

    // Set lap count
    if (isMultiplayerRace && map == ScorchingSands) {
        raceState.totalLaps = MULTIPLAYER_LAPS_SCORCHING_SANDS;
    } else {
        raceState.totalLaps = MapLaps[map];
    }

    // Initialize cars based on mode
    if (isMultiplayerRace) {
        Race_InitMultiplayerCars();
    } else {
        Race_InitSinglePlayerCars();
    }

    raceState.checkpointCount = 0;
    itemButtonHeldLast = false;

    // Initialize items system
    Items_Init(map);
}

/** Update race logic for one frame */
void Race_Tick(void) {
    // Handle race completion display delay
    if (raceState.raceFinished) {
        if (raceState.finishDelayTimer > 0) {
            raceState.finishDelayTimer--;
        }
        return;
    }

    // Normal race tick
    if (!Race_IsActive()) {
        return;
    }

    Race_UpdatePhysics();
    Race_UpdateNetworkSync();
}

/** Update countdown only (network sync during countdown) */
void Race_CountdownTick(void) {
    if (!Race_IsCountdownActive() || !isMultiplayerRace) {
        return;
    }

    Race_UpdateNetworkSync();
}

/** Reset the race to initial state */
void Race_Reset(void) {
    if (raceState.currentMap == NONEMAP) {
        return;
    }

    RaceTick_TimerStop();
    Items_Reset();

    raceState.raceStarted = true;
    raceState.raceFinished = false;
    itemButtonHeldLast = false;

    raceState.finishDelayTimer = 0;
    raceState.finalTimeMin = 0;
    raceState.finalTimeSec = 0;
    raceState.finalTimeMsec = 0;

    countdownState = COUNTDOWN_3;
    countdownTimer = 0;
    raceCanStart = false;

    for (int i = 0; i < raceState.carCount; i++) {
        Race_InitCarAtSpawn(&raceState.cars[i], i);
        collisionLockoutTimer[i] = 0;
    }
}

/** Stop the race and cleanup resources */
void Race_Stop(void) {
    raceState.raceStarted = false;
    Race_CleanupPauseInterrupt();
    RaceTick_TimerStop();
}

/** Mark race as completed with final time */
void Race_MarkAsCompleted(int min, int sec, int msec) {
    raceState.raceFinished = true;
    raceState.finishDelayTimer = FINISH_DELAY_FRAMES;
    raceState.finalTimeMin = min;
    raceState.finalTimeSec = sec;
    raceState.finalTimeMsec = msec;

    // Stop race timer
    irqDisable(IRQ_TIMER1);
    irqClear(IRQ_TIMER1);
}

//=============================================================================
// Public API - State Queries
//=============================================================================

const RaceState* Race_GetState(void) {
    return &raceState;
}

const Car* Race_GetPlayerCar(void) {
    return &raceState.cars[raceState.playerIndex];
}

bool Race_IsActive(void) {
    return raceState.raceStarted && !raceState.raceFinished;
}

bool Race_IsCompleted(void) {
    return raceState.raceFinished;
}

int Race_GetLapCount(void) {
    return raceState.totalLaps;
}

bool Race_CheckFinishLineCross(const Car* car) {
    return Race_CheckFinishLineCrossInternal(car, raceState.playerIndex);
}

void Race_GetFinalTime(int* min, int* sec, int* msec) {
    *min = raceState.finalTimeMin;
    *sec = raceState.finalTimeSec;
    *msec = raceState.finalTimeMsec;
}

//=============================================================================
// Public API - Countdown System
//=============================================================================

void Race_UpdateCountdown(void) {
    Race_UpdateCountdownState();
}

bool Race_IsCountdownActive(void) {
    return countdownState != COUNTDOWN_FINISHED;
}

CountdownState Race_GetCountdownState(void) {
    return countdownState;
}

//=============================================================================
// Public API - Configuration
//=============================================================================

void Race_SetCarGfx(int index, u16* gfx) {
    if (index < 0 || index >= raceState.carCount) {
        return;
    }
    raceState.cars[index].gfx = gfx;
}

void Race_SetLoadedQuadrant(QuadrantID quad) {
    loadedQuadrant = quad;
}

//=============================================================================
// Public API - Pause System
//=============================================================================

void Race_InitPauseInterrupt(void) {
    REG_KEYCNT = BIT(14) | KEY_START;
    irqSet(IRQ_KEYS, Race_PauseISR);
    irqEnable(IRQ_KEYS);
}

void Race_CleanupPauseInterrupt(void) {
    irqDisable(IRQ_KEYS);
    irqClear(IRQ_KEYS);
    isPaused = false;
    debounceFrames = 0;
}

void Race_UpdatePauseDebounce(void) {
    if (debounceFrames > 0) {
        debounceFrames--;
    }
}

void Race_PauseISR(void) {
    // Check debounce period
    if (debounceFrames > 0) {
        return;
    }

    // Verify START is pressed
    scanKeys();
    if (!(keysHeld() & KEY_START)) {
        return;
    }

    // Toggle pause
    isPaused = !isPaused;
    debounceFrames = DEBOUNCE_DELAY;

    if (isPaused) {
        RaceTick_TimerPause();
    } else {
        RaceTick_TimerEnable();
    }
}

//=============================================================================
// Initialization - Cars
//=============================================================================

/** Initialize car at spawn position */
static void Race_InitCarAtSpawn(Car* car, int spawnPosition) {
    // Handle disconnected players
    if (spawnPosition < 0) {
        Vec2 offMapPos = Vec2_FromInt(-1000, -1000);
        car->position = offMapPos;
        car->speed = 0;
        car->angle512 = START_FACING_ANGLE;
        car->Lap = 0;
        car->lastCheckpoint = 0;
        car->rank = 99;
        car->item = ITEM_NONE;
        car->maxSpeed = SPEED_50CC;
        car->accelRate = ACCEL_50CC;
        car->friction = FRICTION_50CC;
        return;
    }

    // Calculate spawn position
    int column = (spawnPosition % 2);
    int x = START_LINE_X + (column * 32);
    int y = START_LINE_Y + (spawnPosition * 24);

    Vec2 spawnPos = Vec2_FromInt(x, y);
    car->position = spawnPos;
    car->speed = 0;
    car->angle512 = START_FACING_ANGLE;
    car->Lap = 0;
    car->lastCheckpoint = 0;
    car->rank = spawnPosition + 1;
    car->item = ITEM_NONE;
    car->maxSpeed = SPEED_50CC;
    car->accelRate = ACCEL_50CC;
    car->friction = FRICTION_50CC;

    Race_ResetLapTracking(spawnPosition);
}

/** Initialize multiplayer cars */
static void Race_InitMultiplayerCars(void) {
    raceState.playerIndex = Multiplayer_GetMyPlayerID();
    raceState.carCount = MAX_CARS;

    // Build list of connected players
    int connectedIndices[MAX_CARS];
    int connectedCount = 0;

    for (int i = 0; i < MAX_CARS; i++) {
        if (Multiplayer_IsPlayerConnected(i)) {
            connectedIndices[connectedCount++] = i;
        }
    }

    // Initialize all cars
    for (int i = 0; i < MAX_CARS; i++) {
        if (Multiplayer_IsPlayerConnected(i)) {
            // Find spawn position among connected players
            int spawnPosition = 0;
            for (int j = 0; j < connectedCount; j++) {
                if (connectedIndices[j] == i) {
                    spawnPosition = j;
                    break;
                }
            }
            Race_InitCarAtSpawn(&raceState.cars[i], spawnPosition);
        } else {
            // Initialize disconnected players off-map
            Race_InitCarAtSpawn(&raceState.cars[i], -1);
        }
        collisionLockoutTimer[i] = 0;
    }
}

/** Initialize single-player cars */
static void Race_InitSinglePlayerCars(void) {
    raceState.playerIndex = 0;
    raceState.carCount = 1;

    for (int i = 0; i < raceState.carCount; i++) {
        Race_InitCarAtSpawn(&raceState.cars[i], i);
        collisionLockoutTimer[i] = 0;
    }
}

/** Reset lap tracking state for a car */
static void Race_ResetLapTracking(int carIndex) {
    wasAboveFinishLine[carIndex] = false;
    hasCompletedFirstCrossing[carIndex] = false;
    cpState[carIndex] = CP_STATE_START;
    wasOnLeftSide[carIndex] = false;
    wasOnTopSide[carIndex] = false;
}

//=============================================================================
// Update Loops
//=============================================================================

/** Update countdown state */
static void Race_UpdateCountdownState(void) {
    countdownTimer++;

    switch (countdownState) {
        case COUNTDOWN_3:
            if (countdownTimer >= COUNTDOWN_NUMBER_DURATION) {
                countdownState = COUNTDOWN_2;
                countdownTimer = 0;
            }
            break;

        case COUNTDOWN_2:
            if (countdownTimer >= COUNTDOWN_NUMBER_DURATION) {
                countdownState = COUNTDOWN_1;
                countdownTimer = 0;
            }
            break;

        case COUNTDOWN_1:
            if (countdownTimer >= COUNTDOWN_NUMBER_DURATION) {
                countdownState = COUNTDOWN_GO;
                countdownTimer = 0;
            }
            break;

        case COUNTDOWN_GO:
            if (countdownTimer >= COUNTDOWN_GO_DURATION) {
                countdownState = COUNTDOWN_FINISHED;
                countdownTimer = 0;
                raceCanStart = true;
                RaceTick_TimerInit();
            }
            break;

        case COUNTDOWN_FINISHED:
            break;
    }
}

/** Update race physics */
static void Race_UpdatePhysics(void) {
    Car* player = &raceState.cars[raceState.playerIndex];
    
    Race_HandlePlayerInput(player, raceState.playerIndex);
    Race_ApplyTerrainEffects(player);
    Items_Update();

    // Calculate scroll position for item collision detection
    int carCenterX = FixedToInt(player->position.x) + CAR_SPRITE_CENTER_OFFSET;
    int carCenterY = FixedToInt(player->position.y) + CAR_SPRITE_CENTER_OFFSET;

    int scrollX = carCenterX - (SCREEN_WIDTH / 2);
    int scrollY = carCenterY - (SCREEN_HEIGHT / 2);

    if (scrollX < 0) scrollX = 0;
    if (scrollY < 0) scrollY = 0;
    if (scrollX > MAX_SCROLL_X) scrollX = MAX_SCROLL_X;
    if (scrollY > MAX_SCROLL_Y) scrollY = MAX_SCROLL_Y;

    Items_CheckCollisions(raceState.cars, raceState.carCount, scrollX, scrollY);
    Items_UpdatePlayerEffects(player, Items_GetPlayerEffects());

    Car_Update(player);
    Race_ClampToMapBounds(player, raceState.playerIndex);
    Race_CheckCheckpointProgression(player, raceState.playerIndex);

    if (collisionLockoutTimer[raceState.playerIndex] > 0) {
        collisionLockoutTimer[raceState.playerIndex]--;
    }
}

/** Update network synchronization */
static void Race_UpdateNetworkSync(void) {
    if (!isMultiplayerRace) {
        return;
    }

    networkUpdateCounter++;
    if (networkUpdateCounter >= NETWORK_UPDATE_INTERVAL) {
        Car* player = &raceState.cars[raceState.playerIndex];
        
        Multiplayer_SendCarState(player);
        Multiplayer_ReceiveCarStates(raceState.cars, raceState.carCount);

        networkUpdateCounter = 0;
    }
}

//=============================================================================
// Input Handling
//=============================================================================

/** Handle player input */
static void Race_HandlePlayerInput(Car* player, int carIndex) {
    if (raceState.raceFinished) {
        return;
    }

    scanKeys();
    uint32 held = keysHeld();

    bool pressingA = held & KEY_A;
    bool pressingB = held & KEY_B;
    bool pressingLeft = held & KEY_LEFT;
    bool pressingRight = held & KEY_RIGHT;
    bool pressingDown = held & KEY_DOWN;
    bool pressingL = held & KEY_L;

    // Item usage
    bool itemPressed = pressingL && !itemButtonHeldLast;
    itemButtonHeldLast = pressingL;

    if (itemPressed) {
        bool fireForward = !pressingDown;
        Items_UsePlayerItem(player, fireForward);
    }

    // Steering control inversion check
    const PlayerItemEffects* effects = Items_GetPlayerEffects();
    bool invertControls = effects->confusionActive;

    // Steering (only when accelerating forward)
    if (pressingA && player->speed >= 0) {
        if (pressingLeft && !pressingRight) {
            int turnAmount = invertControls ? TURN_STEP_50CC : -TURN_STEP_50CC;
            Car_Steer(player, turnAmount);
        } else if (pressingRight && !pressingLeft) {
            int turnAmount = invertControls ? -TURN_STEP_50CC : TURN_STEP_50CC;
            Car_Steer(player, turnAmount);
        }
    }

    // Acceleration/braking
    bool isLockedOut = (collisionLockoutTimer[carIndex] > 0);

    if (pressingA && !pressingB && !isLockedOut) {
        Car_Accelerate(player);
    } else if (pressingB && player->speed > 0) {
        Car_Brake(player);
    }
}

//=============================================================================
// Physics
//=============================================================================

/** Apply terrain effects to car */
static void Race_ApplyTerrainEffects(Car* car) {
    int carX = FixedToInt(car->position.x) + CAR_SPRITE_CENTER_OFFSET;
    int carY = FixedToInt(car->position.y) + CAR_SPRITE_CENTER_OFFSET;

    if (Terrain_IsOnSand(carX, carY, loadedQuadrant)) {
        car->friction = SAND_FRICTION;
        if (car->speed > SAND_MAX_SPEED) {
            Q16_8 excessSpeed = car->speed - SAND_MAX_SPEED;
            car->speed -= (excessSpeed / SAND_SPEED_DIVISOR);
        }
    } else {
        car->friction = FRICTION_50CC;
    }
}

/** Clamp car to map bounds with wall collision */
static void Race_ClampToMapBounds(Car* car, int carIndex) {
    int carX = FixedToInt(car->position.x) + CAR_SPRITE_CENTER_OFFSET;
    int carY = FixedToInt(car->position.y) + CAR_SPRITE_CENTER_OFFSET;

    QuadrantID quad = Race_DetermineCarQuadrant(carX, carY);

    if (Wall_CheckCollision(carX, carY, CAR_RADIUS, quad)) {
        int nx, ny;
        Wall_GetCollisionNormal(carX, carY, quad, &nx, &ny);

        if (nx != 0 || ny != 0) {
            int pushDistance = CAR_RADIUS;
            car->position.x += IntToFixed(nx * pushDistance);
            car->position.y += IntToFixed(ny * pushDistance);

            car->speed = 0;
            collisionLockoutTimer[carIndex] = COLLISION_LOCKOUT_FRAMES;
        }
    }

    // Map boundary clamping
    Q16_8 minPos = IntToFixed(-CAR_SPRITE_CENTER_OFFSET);
    Q16_8 maxPosX = IntToFixed(MAP_SIZE - CAR_SPRITE_CENTER_OFFSET);
    Q16_8 maxPosY = IntToFixed(MAP_SIZE - CAR_SPRITE_CENTER_OFFSET);

    if (car->position.x < minPos) car->position.x = minPos;
    if (car->position.y < minPos) car->position.y = minPos;
    if (car->position.x > maxPosX) car->position.x = maxPosX;
    if (car->position.y > maxPosY) car->position.y = maxPosY;
}

//=============================================================================
// Lap Tracking
//=============================================================================

/** Check checkpoint progression for lap validation */
static void Race_CheckCheckpointProgression(const Car* car, int carIndex) {
    int carX = FixedToInt(car->position.x) + CAR_SPRITE_CENTER_OFFSET;
    int carY = FixedToInt(car->position.y) + CAR_SPRITE_CENTER_OFFSET;

    bool isOnLeftSide = (carX < CHECKPOINT_DIVIDE_X);
    bool isOnTopSide = (carY < CHECKPOINT_DIVIDE_Y);

    switch (cpState[carIndex]) {
        case CP_STATE_START:
            if (!wasOnTopSide[carIndex] && isOnTopSide) {
                cpState[carIndex] = CP_STATE_NEED_LEFT;
            }
            break;

        case CP_STATE_NEED_LEFT:
            if (!wasOnLeftSide[carIndex] && isOnLeftSide) {
                cpState[carIndex] = CP_STATE_NEED_DOWN;
            }
            break;

        case CP_STATE_NEED_DOWN:
            if (wasOnTopSide[carIndex] && !isOnTopSide) {
                cpState[carIndex] = CP_STATE_NEED_RIGHT;
            }
            break;

        case CP_STATE_NEED_RIGHT:
            if (wasOnLeftSide[carIndex] && !isOnLeftSide) {
                cpState[carIndex] = CP_STATE_READY_FOR_LAP;
            }
            break;

        case CP_STATE_READY_FOR_LAP:
            break;
    }

    wasOnLeftSide[carIndex] = isOnLeftSide;
    wasOnTopSide[carIndex] = isOnTopSide;
}

/** Check if car crossed finish line */
static bool Race_CheckFinishLineCrossInternal(const Car* car, int carIndex) {
    int carX = FixedToInt(car->position.x) + CAR_SPRITE_CENTER_OFFSET;
    int carY = FixedToInt(car->position.y) + CAR_SPRITE_CENTER_OFFSET;

    bool isNowAbove = (carY < FINISH_LINE_Y);
    bool crossedLine = !wasAboveFinishLine[carIndex] && isNowAbove;
    wasAboveFinishLine[carIndex] = isNowAbove;

    // Ignore first crossing (spawn position)
    if (crossedLine && !hasCompletedFirstCrossing[carIndex]) {
        hasCompletedFirstCrossing[carIndex] = true;
        return false;
    }

    // Valid lap completion
    if (crossedLine && cpState[carIndex] == CP_STATE_READY_FOR_LAP) {
        cpState[carIndex] = CP_STATE_START;
        return true;
    }

    return false;
}

//=============================================================================
// Utilities
//=============================================================================

/** Determine which quadrant contains the given position */
static QuadrantID Race_DetermineCarQuadrant(int x, int y) {
    int col = (x < QUAD_SIZE) ? 0 : (x < 2 * QUAD_SIZE) ? 1 : 2;
    int row = (y < QUAD_SIZE) ? 0 : (y < 2 * QUAD_SIZE) ? 1 : 2;
    return (QuadrantID)(row * QUADRANT_GRID_SIZE + col);
}