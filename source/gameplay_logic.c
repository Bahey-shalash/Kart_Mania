#include "gameplay_logic.h"

#include <nds.h>

#include "Items.h"
#include "game_constants.h"
#include "multiplayer.h"
#include "terrain_detection.h"
#include "timer.h"
#include "wall_collision.h"

//=============================================================================
// Constants
//=============================================================================
#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 192
#define MAP_SIZE 1024
#define COUNTDOWN_NUMBER_DURATION 60
#define COUNTDOWN_GO_DURATION 60
#define MAX_SCROLL_X (MAP_SIZE - SCREEN_WIDTH)
#define MAX_SCROLL_Y (MAP_SIZE - SCREEN_HEIGHT)

typedef enum {
    CP_STATE_START = 0,
    CP_STATE_NEED_LEFT,
    CP_STATE_NEED_DOWN,
    CP_STATE_NEED_RIGHT,
    CP_STATE_READY_FOR_LAP
} CheckpointProgressState;

static const int MapLaps[] = {
    [NONEMAP] = LAPS_NONE,
    [ScorchingSands] = LAPS_SCORCHING_SANDS,
    [AlpinRush] = LAPS_ALPIN_RUSH,
    [NeonCircuit] = LAPS_NEON_CIRCUIT,
};

//=============================================================================
// Module State
//=============================================================================
static RaceState KartMania;
static bool wasAboveFinishLine[MAX_CARS] = {false};
static bool hasCompletedFirstCrossing[MAX_CARS] = {false};
static CheckpointProgressState cpState[MAX_CARS] = {CP_STATE_START};
static bool wasOnLeftSide[MAX_CARS] = {false};
static bool wasOnTopSide[MAX_CARS] = {false};
//static bool isPaused = false;
static bool itemButtonHeldLast = false;

static int collisionLockoutTimer[MAX_CARS] = {0};
static QuadrantID loadedQuadrant = QUAD_BR;
static int networkUpdateCounter = 0;
static bool isMultiplayerRace = false;
// Countdown state
static CountdownState countdownState = COUNTDOWN_3;
static int countdownTimer = 0;
static bool raceCanStart = false;

//=============================================================================
// Private Prototypes
//=============================================================================
static void initCarAtSpawn(Car* car, int index);
static void handlePlayerInput(Car* player, int carIndex);
static void clampToMapBounds(Car* car, int carIndex);
static QuadrantID determineCarQuadrant(int x, int y);
static void checkCheckpointProgression(const Car* car, int carIndex);
static bool checkFinishLineCross(const Car* car, int carIndex);
static void applyTerrainEffects(Car* car);
static void updateCountdown(void);

//=============================================================================
// Public API - State Queries
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

bool Race_CheckFinishLineCross(const Car* car) {
    // In multiplayer, use the actual player index instead of hardcoded 0
    return checkFinishLineCross(car, KartMania.playerIndex);
}

void Race_SetLoadedQuadrant(QuadrantID quad) {
    loadedQuadrant = quad;
}

void Race_SetCarGfx(int index, u16* gfx) {
    if (index < 0 || index >= KartMania.carCount) {
        return;
    }
    KartMania.cars[index].gfx = gfx;
}

bool Race_IsCompleted(void) {
    return KartMania.raceFinished;
}

void Race_GetFinalTime(int* min, int* sec, int* msec) {
    *min = KartMania.finalTimeMin;
    *sec = KartMania.finalTimeSec;
    *msec = KartMania.finalTimeMsec;
}

// Countdown getters
CountdownState Race_GetCountdownState(void) {
    return countdownState;
}

bool Race_IsCountdownActive(void) {
    return countdownState != COUNTDOWN_FINISHED;
}

bool Race_CanRaceStart(void) {
    return raceCanStart;
}

void Race_UpdateCountdown(void) {
    updateCountdown();
}

//=============================================================================
// Public API - Lifecycle
//=============================================================================
void Race_Init(Map map, GameMode mode) {
    init_pause_interrupt();
    if (map == NONEMAP || map > NeonCircuit) {
        return;
    }

    KartMania.currentMap = map;
    KartMania.totalLaps = MapLaps[map];
    KartMania.gameMode = mode;
    KartMania.raceStarted = true;
    KartMania.raceFinished = false;
    itemButtonHeldLast = false;
    
    // Initialize finish tracking
    KartMania.finishDelayTimer = 0;
    KartMania.finalTimeMin = 0;
    KartMania.finalTimeSec = 0;
    KartMania.finalTimeMsec = 0;

    // Initialize countdown
    countdownState = COUNTDOWN_3;
    countdownTimer = 0;
    raceCanStart = false;

    // NEW: Check if this is a multiplayer race
    isMultiplayerRace = (mode == MultiPlayer);

    if (isMultiplayerRace) {
        // Multiplayer mode
        KartMania.playerIndex = Multiplayer_GetMyPlayerID();
        // In multiplayer, we need all 8 car slots because player IDs are 0-7
        // (a sparse array where only connected players' cars are used)
        KartMania.carCount = MAX_CARS;
    } else {
        // Single player mode
        KartMania.playerIndex = 0;
        KartMania.carCount = 1;  // Only 1 car in single player
    }

    KartMania.checkpointCount = 0;

    for (int i = 0; i < KartMania.carCount; i++) {
        initCarAtSpawn(&KartMania.cars[i], i);
        collisionLockoutTimer[i] = 0;
    }

    // Initialize items system
    Items_Init(map);
}

void Race_Reset(void) {
    if (KartMania.currentMap == NONEMAP) {
        return;
    }

    RaceTick_TimerStop();

    // Reset items
    Items_Reset();

    KartMania.raceStarted = true;
    KartMania.raceFinished = false;
    itemButtonHeldLast = false;
    
    // Reset finish tracking
    KartMania.finishDelayTimer = 0;
    KartMania.finalTimeMin = 0;
    KartMania.finalTimeSec = 0;
    KartMania.finalTimeMsec = 0;

    // Reset countdown
    countdownState = COUNTDOWN_3;
    countdownTimer = 0;
    raceCanStart = false;

    for (int i = 0; i < KartMania.carCount; i++) {
        initCarAtSpawn(&KartMania.cars[i], i);
        collisionLockoutTimer[i] = 0;
    }
}

void Race_Stop(void) {
    KartMania.raceStarted = false;
    cleanup_pause_interrupt();  // NEW: Clean up pause interrupt
    RaceTick_TimerStop();
}

void Race_MarkAsCompleted(int min, int sec, int msec) {
    KartMania.raceFinished = true;
    KartMania.finishDelayTimer = FINISH_DELAY_FRAMES;
    KartMania.finalTimeMin = min;
    KartMania.finalTimeSec = sec;
    KartMania.finalTimeMsec = msec;
    
    // Stop race timer
    irqDisable(IRQ_TIMER1);
    irqClear(IRQ_TIMER1);
}

//=============================================================================
// Public API - Game Loop
//=============================================================================
void Race_Tick(void) {
    // Special handling: if race is finished, ONLY count down the delay timer
    if (KartMania.raceFinished) {
        if (KartMania.finishDelayTimer > 0) {
            KartMania.finishDelayTimer--;
        }
        return;  // Don't process any game logic after completion
    }
    
    // Normal race active check
    if (!Race_IsActive()) {
        return;
    }

    Car* player = &KartMania.cars[KartMania.playerIndex];
    handlePlayerInput(player, KartMania.playerIndex);
    applyTerrainEffects(player);

    Items_Update();

    // Calculate scroll position based on car's CENTER (not top-left corner)
    int carCenterX = FixedToInt(player->position.x) + CAR_SPRITE_CENTER_OFFSET;
    int carCenterY = FixedToInt(player->position.y) + CAR_SPRITE_CENTER_OFFSET;
    
    int scrollX = carCenterX - (SCREEN_WIDTH / 2);
    int scrollY = carCenterY - (SCREEN_HEIGHT / 2);
    
    if (scrollX < 0)
        scrollX = 0;
    if (scrollY < 0)
        scrollY = 0;
    if (scrollX > MAX_SCROLL_X)
        scrollX = MAX_SCROLL_X;
    if (scrollY > MAX_SCROLL_Y)
        scrollY = MAX_SCROLL_Y;

    Items_CheckCollisions(KartMania.cars, KartMania.carCount, scrollX, scrollY);
    Items_UpdatePlayerEffects(player, Items_GetPlayerEffects());

    Car_Update(player);
    clampToMapBounds(player, KartMania.playerIndex);  // This handles wall collision
    checkCheckpointProgression(player, KartMania.playerIndex);

    if (collisionLockoutTimer[KartMania.playerIndex] > 0) {
        collisionLockoutTimer[KartMania.playerIndex]--;
    }
    
    // NEW: Network synchronization for multiplayer
    if (isMultiplayerRace) {
        networkUpdateCounter++;
        if (networkUpdateCounter >= 4) {  // Every 4 frames = 15Hz
            // Send my car state
            Multiplayer_SendCarState(player);

            // Receive others' car states
            Multiplayer_ReceiveCarStates(KartMania.cars, KartMania.carCount);

            networkUpdateCounter = 0;
        }
    }
}


//=============================================================================
// Countdown Tick - Only handles network sync, no game logic
//=============================================================================
void Race_CountdownTick(void) {
    // Only run during countdown in multiplayer
    if (!Race_IsCountdownActive() || !isMultiplayerRace) {
        return;
    }
    
    // Network sync only - share spawn positions
    networkUpdateCounter++;
    if (networkUpdateCounter >= 4) {  // Every 4 frames = 15Hz
        Car* player = &KartMania.cars[KartMania.playerIndex];
        
        // Send my car's spawn position
        Multiplayer_SendCarState(player);

        // Receive others' spawn positions
        Multiplayer_ReceiveCarStates(KartMania.cars, KartMania.carCount);

        networkUpdateCounter = 0;
    }
}
//=============================================================================
// Countdown System
//=============================================================================
static void updateCountdown(void) {
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
                // Start the race timer now
                RaceTick_TimerInit();
            }
            break;

        case COUNTDOWN_FINISHED:
            // Do nothing - race is running
            break;
    }
}

//=============================================================================
// Checkpoint System
//=============================================================================
static void checkCheckpointProgression(const Car* car, int carIndex) {
    int carX = FixedToInt(car->position.x) + CAR_SPRITE_CENTER_OFFSET;
    int carY = FixedToInt(car->position.y) + CAR_SPRITE_CENTER_OFFSET;

    bool isOnLeftSide = (carX < CHECKPOINT_DIVIDE_X);
    bool isOnTopSide = (carY < CHECKPOINT_DIVIDE_Y);

    // ... rest of function stays the same
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

//=============================================================================
// Finish Line Detection
//=============================================================================
static bool checkFinishLineCross(const Car* car, int carIndex) {
    int carX = FixedToInt(car->position.x) + CAR_SPRITE_CENTER_OFFSET;
    int carY = FixedToInt(car->position.y) + CAR_SPRITE_CENTER_OFFSET;
    
    bool isNowAbove = (carY < FINISH_LINE_Y);
    bool crossedLine = !wasAboveFinishLine[carIndex] && isNowAbove;
    wasAboveFinishLine[carIndex] = isNowAbove;
    
    if (crossedLine && !hasCompletedFirstCrossing[carIndex]) {
        hasCompletedFirstCrossing[carIndex] = true;
        return false;
    }
    
    if (crossedLine && cpState[carIndex] == CP_STATE_READY_FOR_LAP) {
        cpState[carIndex] = CP_STATE_START;
        return true;
    }
    
    return false;
}
//=============================================================================
// Terrain Applications
//=============================================================================
static void applyTerrainEffects(Car* car) {
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

//=============================================================================
// Private Implementation
//=============================================================================
static void initCarAtSpawn(Car* car, int index) {
    int column = (index % 2);  // 0 = left (even), 1 = right (odd) 
    int x = START_LINE_X + (column * 32);  // Left column at 904, right at 936
    int y = START_LINE_Y + (index * 24);     
    
    Vec2 spawnPos = Vec2_FromInt(x, y);
    car->position = spawnPos;
    car->speed = 0;
    car->angle512 = START_FACING_ANGLE;
    car->Lap = 0;
    car->lastCheckpoint = 0;
    car->rank = index + 1;
    car->item = ITEM_NONE;
    car->maxSpeed = SPEED_50CC;
    car->accelRate = ACCEL_50CC;
    car->friction = FRICTION_50CC;
    wasAboveFinishLine[index] = false;
    hasCompletedFirstCrossing[index] = false;
    cpState[index] = CP_STATE_START;
    wasOnLeftSide[index] = false;
    wasOnTopSide[index] = false;
}

static void handlePlayerInput(Car* player, int carIndex) {
    // CRITICAL: Block all input if race is finished
    if (KartMania.raceFinished) {
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
    bool itemPressed = pressingL && !itemButtonHeldLast;
    itemButtonHeldLast = pressingL;

    // Item usage
    if (itemPressed) {
        bool fireForward = !pressingDown;
        Items_UsePlayerItem(player, fireForward);
    }

    // Steering (potentially inverted by mushroom confusion)
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

    bool isLockedOut = (collisionLockoutTimer[carIndex] > 0);

    if (pressingA && !pressingB && !isLockedOut) {
        Car_Accelerate(player);
    } else if (pressingB && player->speed > 0) {
        Car_Brake(player);
    }
}

static void clampToMapBounds(Car* car, int carIndex) {
    // Get the visual center of the car (where it actually appears on screen)
    int carX = FixedToInt(car->position.x) + CAR_SPRITE_CENTER_OFFSET;
    int carY = FixedToInt(car->position.y) + CAR_SPRITE_CENTER_OFFSET;

    QuadrantID quad = determineCarQuadrant(carX, carY);

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

    // Map bounds should account for the sprite center offset
    Q16_8 minPos = IntToFixed(-CAR_SPRITE_CENTER_OFFSET);
    Q16_8 maxPosX = IntToFixed(1024 - CAR_SPRITE_CENTER_OFFSET);
    Q16_8 maxPosY = IntToFixed(1024 - CAR_SPRITE_CENTER_OFFSET);

    if (car->position.x < minPos)
        car->position.x = minPos;
    if (car->position.y < minPos)
        car->position.y = minPos;
    if (car->position.x > maxPosX)
        car->position.x = maxPosX;
    if (car->position.y > maxPosY)
        car->position.y = maxPosY;
}

static QuadrantID determineCarQuadrant(int x, int y) {
    int col = (x < QUAD_OFFSET) ? 0 : (x < 2 * QUAD_OFFSET) ? 1 : 2;
    int row = (y < QUAD_OFFSET) ? 0 : (y < 2 * QUAD_OFFSET) ? 1 : 2;
    return (QuadrantID)(row * QUADRANT_GRID_SIZE + col);
}

//=============================================================================
// Pause System with Key Interrupt
//=============================================================================
static volatile bool isPaused = false;
static volatile int debounceFrames = 0;

#define DEBOUNCE_DELAY 15  // ~250ms at 60Hz

void init_pause_interrupt(void) {
    // BIT(14) = enable key interrupt
    // Actually, we need to handle this differently - let's use IRQ on press
    REG_KEYCNT = BIT(14) | KEY_START;  // Enable interrupt for START key
    irqSet(IRQ_KEYS, PauseISR);
    irqEnable(IRQ_KEYS);
}

void PauseISR(void) {
    // Check if we're in debounce period
    if (debounceFrames > 0) {
        return;  // Ignore bounces
    }
    
    // Check if START is actually pressed (not released)
    scanKeys();
    if (!(keysHeld() & KEY_START)) {
        return;  // Button was released, ignore
    }
    
    // Toggle pause state
    isPaused = !isPaused;
    debounceFrames = DEBOUNCE_DELAY;
    
    if (isPaused) {
        RaceTick_TimerPause();
    } else {
        RaceTick_TimerEnable();
    }
}

// Add this function to update debounce counter
void UpdatePauseDebounce(void) {
    if (debounceFrames > 0) {
        debounceFrames--;
    }
}

bool IsPaused(void) {
    return isPaused;
}

void cleanup_pause_interrupt(void) {
    irqDisable(IRQ_KEYS);
    irqClear(IRQ_KEYS);
    isPaused = false;
    debounceFrames = 0;
}