#include "gameplay_logic.h"

#include <nds.h>

#include "Items.h"
#include "terrain_detection.h"
#include "timer.h"
#include "wall_collision.h"

//=============================================================================
// Constants
//=============================================================================
#define TURN_STEP_50CC 3

// Physics tuning (50cc class, Q16.8 format, 60Hz)

#define SPEED_50CC (FIXED_ONE * 3)  // 3.0 px/frame
#define ACCEL_50CC IntToFixed(1)    // 1.0 px/frame
#define FRICTION_50CC 240           // 240/256 = 0.9375

// Sand Physics - SEVERE PENALTIES
#define SAND_FRICTION 150  // Much higher friction (150/256 = 0.586 - very draggy!)
#define SAND_MAX_SPEED (SPEED_50CC / 2)  // Only 50% max speed on sand (was 75%)

// Collision state
#define COLLISION_LOCKOUT_FRAMES 60  // Frames to disable acceleration after wall hit

// World directions (0-511 binary angle)
#define ANGLE_RIGHT 0        // 0°
#define ANGLE_DOWN 128       // 90°
#define ANGLE_LEFT 256       // 180°
#define ANGLE_UP 384         // 270°
#define ANGLE_DOWN_RIGHT 64  // 45°
#define ANGLE_DOWN_LEFT 192  // 135°
#define ANGLE_UP_LEFT 320    // 225°
#define ANGLE_UP_RIGHT 448   // 315°

// Start line position (BR quadrant)
#define START_LINE_X 920
#define START_LINE_Y 595
#define CAR_SPACING 40
#define START_FACING_ANGLE ANGLE_UP

// Finish line detection
#define FINISH_LINE_Y 580
#define FINISH_LINE_X_MIN 900
#define FINISH_LINE_X_MAX 960

// Checkpoint system
#define CHECKPOINT_DIVIDE_X 512
#define CHECKPOINT_DIVIDE_Y 512

typedef enum {
    CP_STATE_START = 0,
    CP_STATE_NEED_LEFT,
    CP_STATE_NEED_DOWN,
    CP_STATE_NEED_RIGHT,
    CP_STATE_READY_FOR_LAP
} CheckpointProgressState;

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
static bool wasAboveFinishLine[MAX_CARS] = {false};
static bool hasCompletedFirstCrossing[MAX_CARS] = {false};
static CheckpointProgressState cpState[MAX_CARS] = {CP_STATE_START};
static bool wasOnLeftSide[MAX_CARS] = {false};
static bool wasOnTopSide[MAX_CARS] = {false};
static bool isPaused = false;
static bool itemButtonHeldLast = false;  // Track L-button edge ourselves

static int collisionLockoutTimer[MAX_CARS] = {0};
static QuadrantID loadedQuadrant = QUAD_BR;

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
    return checkFinishLineCross(car, 0);
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
    KartMania.playerIndex = 0;
    KartMania.raceStarted = true;
    KartMania.raceFinished = false;
    KartMania.carCount = (mode == SinglePlayer) ? MAX_CARS : 1;
    KartMania.checkpointCount = 0;
    itemButtonHeldLast = false;

    for (int i = 0; i < KartMania.carCount; i++) {
        initCarAtSpawn(&KartMania.cars[i], i);
        collisionLockoutTimer[i] = 0;
    }

    // Initialize items system
    Items_Init(map);

    RaceTick_TimerInit();
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

    for (int i = 0; i < KartMania.carCount; i++) {
        initCarAtSpawn(&KartMania.cars[i], i);
        collisionLockoutTimer[i] = 0;
    }

    RaceTick_TimerInit();
}

void Race_Stop(void) {
    KartMania.raceStarted = false;
    RaceTick_TimerStop();
}

//=============================================================================
// Public API - Game Loop
//=============================================================================

// Modify Race_Tick to include terrain effects
void Race_Tick(void) {
    if (!Race_IsActive()) {
        return;
    }

    Car* player = &KartMania.cars[KartMania.playerIndex];

    handlePlayerInput(player, KartMania.playerIndex);
    applyTerrainEffects(player);

    Items_Update();
    Items_CheckCollisions(KartMania.cars, KartMania.carCount);
    Items_UpdatePlayerEffects(player, Items_GetPlayerEffects());

    Car_Update(player);
    clampToMapBounds(player, KartMania.playerIndex);
    checkCheckpointProgression(player, KartMania.playerIndex);

    if (collisionLockoutTimer[KartMania.playerIndex] > 0) {
        collisionLockoutTimer[KartMania.playerIndex]--;
    }
}

//=============================================================================
// Checkpoint System
//=============================================================================
static void checkCheckpointProgression(const Car* car, int carIndex) {
    int carX = FixedToInt(car->position.x);
    int carY = FixedToInt(car->position.y);

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

//=============================================================================
// Finish Line Detection
//=============================================================================
static bool checkFinishLineCross(const Car* car, int carIndex) {
    int carX = FixedToInt(car->position.x);
    int carY = FixedToInt(car->position.y);

    bool inXRange = (carX >= FINISH_LINE_X_MIN && carX <= FINISH_LINE_X_MAX);

    if (!inXRange) {
        wasAboveFinishLine[carIndex] = (carY < FINISH_LINE_Y);
        return false;
    }

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
    int carX = FixedToInt(car->position.x);
    int carY = FixedToInt(car->position.y);
    if (Terrain_IsOnSand(carX, carY, loadedQuadrant)) {
        car->friction = SAND_FRICTION;
        if (car->speed > SAND_MAX_SPEED) {
            Q16_8 excessSpeed = car->speed - SAND_MAX_SPEED;
            car->speed -= (excessSpeed / 2);
        }
    } else {
        car->friction = FRICTION_50CC;
    }
}
//=============================================================================
// Private Implementation
//=============================================================================
static void initCarAtSpawn(Car* car, int index) {
    Vec2 spawnPos = Vec2_FromInt(START_LINE_X, START_LINE_Y + (index * CAR_SPACING));

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
    scanKeys();
    uint32 held = keysHeld();
    // when i had keysDown game was super responsive(like youd have to wait  a second
    // before being able to place it) which is normal bevause it checks buttons that
    // went from up -> down this frame

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
        bool fireForward = !pressingDown;  // Default forward unless DOWN held
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
        Car_Accelerate(player);  // Forward acceleration (blocked during lockout)
    } else if (pressingB && player->speed > 0) {
        Car_Brake(player);
    }
}

static void clampToMapBounds(Car* car, int carIndex) {
    int carX = FixedToInt(car->position.x);
    int carY = FixedToInt(car->position.y);

    QuadrantID quad = determineCarQuadrant(carX, carY);

    if (Wall_CheckCollision(carX, carY, CAR_RADIUS, quad)) {
        int nx, ny;
        Wall_GetCollisionNormal(carX, carY, quad, &nx, &ny);

        if (nx != 0 || ny != 0) {
            // Push car out of wall
            int pushDistance = CAR_RADIUS + 2;
            car->position.x += IntToFixed(nx * pushDistance);
            car->position.y += IntToFixed(ny * pushDistance);

            // FULL STOP on collision
            car->speed = 0;

            // Enable collision lockout to prevent immediate re-acceleration
            collisionLockoutTimer[carIndex] = COLLISION_LOCKOUT_FRAMES;
        }
    }

    // Map boundary clamping
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
}

static QuadrantID determineCarQuadrant(int x, int y) {
    int col = (x < QUAD_OFFSET) ? 0 : (x < 2 * QUAD_OFFSET) ? 1 : 2;
    int row = (y < QUAD_OFFSET) ? 0 : (y < 2 * QUAD_OFFSET) ? 1 : 2;
    return (QuadrantID)(row * 3 + col);
}

//=============================================================================
// Pause Handling
//=============================================================================
void init_pause_interrupt(void) {
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
