/**
 * File: Car.h
 * -----------
 * Description: Car physics and state management for kart racing gameplay.
 *              Defines the Car structure and provides functions for acceleration,
 *              braking, steering, collision handling, and position/velocity
 *              manipulation. Uses a scalar speed + angle representation for
 *              simplified physics and tight control.
 *
 * Ownership: Cars are owned by RaceState in gameplay_logic.c
 * Access Rules:
 *   - Read: Direct member access (car->position.x, car->speed, etc.)
 *   - Modify: Use Car_* functions to maintain invariants
 * Concurrency: Single-threaded only (DS hardware limitation)
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 06.01.2026
 */

#ifndef CAR_H
#define CAR_H

#include <stdbool.h>
#include <string.h>

#include "items/items_types.h"
#include "../math/fixedmath.h"

//=============================================================================
// Car Structure
//=============================================================================

/**
 * Struct: Car
 * -----------
 * Represents a racing kart with physics state, race progress, and inventory.
 *
 * Physics Model:
 *   - Uses scalar speed (magnitude) + angle512 (direction) representation
 *   - Movement direction always follows facing angle when speed > 0
 *   - Acceleration/braking affect speed magnitude
 *   - Steering rotates the facing angle
 *
 * Members:
 *   position       - World position (Q16.8 fixed-point)
 *   speed          - Current speed magnitude (Q16.8)
 *   maxSpeed       - Maximum allowed speed (Q16.8)
 *   accelRate      - Acceleration/braking step (Q16.8)
 *   friction       - Speed multiplier per frame (0-256, where 256 = 100%)
 *   angle512       - Facing direction (512-based angle system, 0-511)
 *   Lap            - Current lap number
 *   rank           - Race position (1st, 2nd, 3rd, etc.)
 *   lastCheckpoint - Last checkpoint crossed (-1 = none)
 *   item           - Currently held item
 *   carname        - Car name string (max 31 chars + null terminator)
 *   gfx            - Sprite graphics pointer
 */
typedef struct Car {
    Vec2 position;
    Q16_8 speed;
    Q16_8 maxSpeed;
    Q16_8 accelRate;
    Q16_8 friction;
    int angle512;
    int Lap;
    int rank;
    int lastCheckpoint;
    Item item;
    char carname[32];
    u16* gfx;
} Car;
//=============================================================================
// Constructors (Inline Helpers)
//=============================================================================

/**
 * Function: CarCreate
 * -------------------
 * Creates a new car with specified physics parameters and initial state.
 *
 * Parameters:
 *   pos        - Initial position (Q16.8 fixed-point)
 *   speed      - Initial speed magnitude (Q16.8)
 *   SpeedMax   - Maximum speed (Q16.8)
 *   accel_rate - Acceleration/braking rate (Q16.8)
 *   frictionn  - Friction multiplier (0-256)
 *   init_item  - Initial item in inventory
 *   name       - Car name string (null-terminated)
 *
 * Returns:
 *   Initialized Car structure
 */
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

/**
 * Function: emptyCar
 * ------------------
 * Creates a car with all physics values initialized to zero.
 *
 * Parameters:
 *   name - Car name string (null-terminated)
 *
 * Returns:
 *   Car structure with zeroed physics state
 */
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
// Lifecycle Management
//=============================================================================

/**
 * Function: Car_Init
 * ------------------
 * Initializes a car with starting position, name, and physics parameters.
 *
 * Parameters:
 *   car       - Pointer to car to initialize
 *   pos       - Starting position (Q16.8)
 *   name      - Car name (null-terminated string)
 *   maxSpeed  - Maximum speed (Q16.8)
 *   accelRate - Acceleration rate (Q16.8)
 *   friction  - Friction multiplier (0-256)
 */
void Car_Init(Car* car, const Vec2* pos, const char* name, Q16_8 maxSpeed,
              Q16_8 accelRate,
              Q16_8 friction);

/**
 * Function: Car_Reset
 * -------------------
 * Resets car to spawn position with zeroed race state. Preserves physics
 * parameters (maxSpeed, accelRate, friction) and car name.
 *
 * Parameters:
 *   car      - Pointer to car to reset
 *   spawnPos - Respawn position (Q16.8)
 */
void Car_Reset(Car* car, const Vec2* spawnPos);

//=============================================================================
// Physics Control
//=============================================================================

/**
 * Function: Car_Accelerate
 * -------------------------
 * Increases car speed by accelRate in the current facing direction.
 * Speed is capped at maxSpeed.
 */
void Car_Accelerate(Car* car);

/**
 * Function: Car_Brake
 * -------------------
 * Decreases car speed by accelRate. Speed cannot go negative.
 */
void Car_Brake(Car* car);

/**
 * Function: Car_Steer
 * -------------------
 * Rotates the car's facing angle. ALWAYS use this for normal steering!
 *
 * Parameters:
 *   car           - Pointer to car to steer
 *   deltaAngle512 - Angle change in 512-based system (positive = clockwise)
 */
void Car_Steer(Car* car, int deltaAngle512);

/**
 * Function: Car_Update
 * --------------------
 * Updates car physics for one frame. Applies friction, snaps low speeds to
 * zero, and integrates velocity into position. Call once per physics tick (60Hz).
 */
void Car_Update(Car* car);
//=============================================================================
// Read-Only Queries
//=============================================================================

/**
 * Function: Car_GetAngle
 * ----------------------
 * Returns the car's facing angle (for sprite rotation).
 *
 * Returns:
 *   Facing angle in 512-based system (0-511)
 */
int Car_GetAngle(const Car* car);

/**
 * Function: Car_GetVelocityAngle
 * -------------------------------
 * Returns the car's movement direction. With scalar speed, this always
 * matches the facing angle when moving.
 *
 * Returns:
 *   Movement angle in 512-based system (0-511)
 */
int Car_GetVelocityAngle(const Car* car);

/**
 * Function: Car_IsMoving
 * ----------------------
 * Checks if car speed is above the movement threshold.
 *
 * Returns:
 *   true if car is moving, false otherwise
 */
bool Car_IsMoving(const Car* car);

/**
 * Function: Car_GetSpeed
 * ----------------------
 * Returns the car's current speed magnitude.
 *
 * Returns:
 *   Speed in Q16.8 fixed-point
 */
Q16_8 Car_GetSpeed(const Car* car);

//=============================================================================
// Special Operations
//=============================================================================

/**
 * Function: Car_SetPosition
 * -------------------------
 * Directly sets car position. Use for respawn/teleport effects.
 *
 * Parameters:
 *   car - Pointer to car
 *   pos - New position (Q16.8)
 */
void Car_SetPosition(Car* car, const Vec2* pos);

/**
 * Function: Car_SetVelocity
 * -------------------------
 * Sets car speed and direction from a velocity vector. Use for boosts,
 * collisions, and hazard effects. Speed is capped to maxSpeed.
 *
 * Parameters:
 *   car      - Pointer to car
 *   velocity - New velocity vector (Q16.8)
 */
void Car_SetVelocity(Car* car, const Vec2* velocity);

/**
 * Function: Car_ApplyImpulse
 * --------------------------
 * Applies an instant velocity change to the car. Useful for collision
 * responses and item effects. Speed is capped to maxSpeed.
 *
 * Parameters:
 *   car     - Pointer to car
 *   impulse - Velocity impulse to add (Q16.8)
 */
void Car_ApplyImpulse(Car* car, const Vec2* impulse);

/**
 * Function: Car_SetAngle
 * ----------------------
 * Directly sets car facing angle. Movement direction will follow this angle
 * on the next update. Use for spawning and respawn orientation.
 *
 * WARNING: For normal steering, ALWAYS use Car_Steer() instead!
 *
 * Parameters:
 *   car       - Pointer to car
 *   angle512  - New facing angle (512-based system)
 */
void Car_SetAngle(Car* car, int angle512);

//=============================================================================
// Game Events
//=============================================================================

/**
 * Function: Car_LapComplete
 * -------------------------
 * Increments the car's lap counter when a lap is completed.
 *
 * Parameters:
 *   car - Pointer to car
 */
void Car_LapComplete(Car* car);

#endif  // CAR_H
