
// BAHEY------
#ifndef CAR_H
#define CAR_H

#include <stdbool.h>
#include <string.h>

#include "items/Items.h"
#include "../math/fixedmath.h"

//=============================================================================
// Car - Physics & State
//=============================================================================
// OWNERSHIP: Cars are owned by RaceState in gameplay_logic.c
// ACCESS RULES:
//   - Read: Direct member access (car->position.x, car->speed, etc.)
//   - Modify: Use Car_* functions to maintain invariants
// CONCURRENCY: Single-threaded only (DS hardware limitation)
//=============================================================================

typedef struct Car {
    Vec2 position;
    Q16_8 speed;  // Magnitude of movement; direction comes from angle512

    Q16_8 maxSpeed;
    Q16_8 accelRate;
    Q16_8 friction;
    int angle512;  // Facing direction (for rendering)
    int Lap;
    int rank;  // Race position: 1st, 2nd, etc.
    int lastCheckpoint;

    Item item;
    char carname[32];

    u16* gfx;  // Sprite graphics pointer

} Car;
//=============================================================================
// Constructors (inline helpers)
//=============================================================================

static inline Car CarCreate(Vec2 pos, Q16_8 speed, Q16_8 SpeedMax, Q16_8 accel_rate,
                            Q16_8 frictionn, Item init_item, const char* name) {
    Car car = {
        .position = pos,
        .speed = speed,
        .maxSpeed = SpeedMax,
        .accelRate = accel_rate,
        .friction = frictionn,
        .angle512 = 0,
        .Lap = 0,
        .rank = 0,
        .lastCheckpoint = -1,
        .item = init_item,
        .carname = {0},
    };
    if (name) {
        strncpy(car.carname, name, sizeof(car.carname) - 1);
        car.carname[sizeof(car.carname) - 1] = '\0';
    }
    return car;
}

static inline Car emptyCar(const char* name) {
    Car car = {
        .position = Vec2_Zero(),
        .speed = 0,
        .maxSpeed = 0,
        .angle512 = 0,
        .accelRate = 0,
        .friction = 0,
        .Lap = 0,
        .rank = 0,
        .lastCheckpoint = -1,
        .item = ITEM_NONE,
        .carname = {0},
    };
    if (name) {
        strncpy(car.carname, name, sizeof(car.carname) - 1);
        car.carname[sizeof(car.carname) - 1] = '\0';
    }
    return car;
}

//=============================================================================
// Lifecycle
//=============================================================================

void Car_Init(Car* car, Vec2 pos, const char* name, Q16_8 maxSpeed, Q16_8 accelRate,
              Q16_8 friction);
void Car_Reset(Car* car, Vec2 spawnPos);

//=============================================================================
// Physics Control - Use these for gameplay logic
//=============================================================================
void Car_Accelerate(Car* car);
void Car_Brake(Car* car);
void Car_Steer(Car* car, int deltaAngle512);  // ← ALWAYS use this for steering!
void Car_Update(Car* car);                    // Call once per physics tick (60Hz)
//=============================================================================
// Read-Only Queries - Safe to call from anywhere
//=============================================================================

int Car_GetAngle(const Car* car);          // Facing direction (for sprite rotation)
int Car_GetVelocityAngle(const Car* car);  // Actual movement direction
bool Car_IsMoving(const Car* car);         // Speed above threshold?
Q16_8 Car_GetSpeed(const Car* car);        // Speed magnitude

//=============================================================================
// Special Operations - Use with caution!
//=============================================================================
// Direct manipulation for special effects (boosts, collisions, hazards)

void Car_SetPosition(Car* car, Vec2 pos);  // For respawn/teleport
void Car_SetVelocity(Car* car,
                     Vec2 velocity);  // For boosts/hazards (caps to maxSpeed)
void Car_ApplyImpulse(Car* car,
                      Vec2 impulse);  // For collisions/items (caps to maxSpeed)

// WARNING: Sets facing instantly (and therefore movement direction on next tick).
// Examples: spawning at specific angle, respawn orientation
// For normal steering, ALWAYS use Car_Steer() instead!
void Car_SetAngle(Car* car, int angle512);

//=============================================================================
// Game Events
//=============================================================================

void Car_LapComplete(Car* car);

#endif  // CAR_H
