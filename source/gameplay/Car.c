/**
 * File: Car.c
 * -----------
 * Description: Implementation of car physics and state management. Provides
 *              acceleration, braking, steering, friction, position updates,
 *              and velocity manipulation for kart racing gameplay.
 *
 * Physics Model:
 *   - Scalar speed + angle representation for simplified control
 *   - Movement direction always follows facing angle
 *   - Friction applied as multiplicative decay per frame
 *   - Speed capped to maxSpeed on all operations
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 06.01.2026
 */

#include "Car.h"

#include <stddef.h>
#include <string.h>

#include "../core/game_constants.h"

//=============================================================================
// Private Function Prototypes
//=============================================================================

static void copy_name(char destination[32], const char* name);
static Q16_8 clamp_friction(Q16_8 friction);
static Vec2 build_velocity(const Car* car);
static void apply_velocity(Car* car, Vec2 velocity);

//=============================================================================
// Private Helper Implementations
//=============================================================================

/**
 * Function: copy_name
 * -------------------
 * Safely copies a car name string with bounds checking.
 */
static void copy_name(char destination[32], const char* name) {
    destination[0] = '\0';
    if (name == NULL) {
        return;
    }
    strncpy(destination, name, CAR_NAME_MAX_LENGTH);
    destination[CAR_NAME_MAX_LENGTH] = '\0';
}

/**
 * Function: clamp_friction
 * -------------------------
 * Clamps friction value to valid range [0, FIXED_ONE].
 */
static Q16_8 clamp_friction(Q16_8 friction) {
    if (friction < 0) {
        return 0;
    }
    if (friction > FIXED_ONE) {
        return FIXED_ONE;
    }
    return friction;
}

/**
 * Function: build_velocity
 * -------------------------
 * Builds a velocity vector from car's current facing angle and speed magnitude.
 */
static Vec2 build_velocity(const Car* car) {
    if (car == NULL || car->speed == 0) {
        return Vec2_Zero();
    }
    Vec2 forward = Vec2_FromAngle(car->angle512);
    return Vec2_Scale(forward, car->speed);
}

/**
 * Function: apply_velocity
 * -------------------------
 * Converts a velocity vector into the car's internal speed/angle representation.
 * Speed is capped to maxSpeed.
 */
static void apply_velocity(Car* car, Vec2 velocity) {
    if (car == NULL) {
        return;
    }

    if (Vec2_IsZero(velocity)) {
        car->speed = 0;
        return;
    }

    car->speed = Vec2_Len(velocity);
    car->angle512 = Vec2_ToAngle(velocity);

    if (car->maxSpeed > 0 && car->speed > car->maxSpeed) {
        car->speed = car->maxSpeed;
    }
}

//=============================================================================
// Public API - Lifecycle Management
//=============================================================================

/**
 * Function: Car_Init
 * ------------------
 * Initializes a car with starting position, name, and physics parameters.
 */
void Car_Init(Car* car, Vec2 pos, const char* name, Q16_8 maxSpeed, Q16_8 accelRate,
              Q16_8 friction) {
    if (car == NULL) {
        return;
    }

    car->position = pos;
    car->speed = 0;
    car->maxSpeed = maxSpeed;
    car->accelRate = accelRate;
    car->friction = clamp_friction(friction);
    car->angle512 = 0;  // facing right (east)
    car->Lap = 0;
    car->rank = 0;
    car->lastCheckpoint = -1;
    car->item = ITEM_NONE;
    copy_name(car->carname, name);
}

/**
 * Function: Car_Reset
 * -------------------
 * Resets car to spawn position with zeroed race state. Preserves physics
 * parameters and car name.
 */
void Car_Reset(Car* car, Vec2 spawnPos) {
    if (car == NULL) {
        return;
    }

    car->position = spawnPos;
    car->speed = 0;
    car->angle512 = 0;  // reset facing direction
    car->Lap = 0;
    car->rank = 0;
    car->lastCheckpoint = -1;
    car->item = ITEM_NONE;
    // Note: maxSpeed, accelRate, friction, and carname persist across resets
}

//=============================================================================
// Public API - Physics Control
//=============================================================================

/**
 * Function: Car_Accelerate
 * -------------------------
 * Increases car speed by accelRate in the current facing direction.
 */
void Car_Accelerate(Car* car) {
    if (car == NULL) {
        return;
    }

    car->speed += car->accelRate;

    // Cap speed
    if (car->maxSpeed > 0 && car->speed > car->maxSpeed) {
        car->speed = car->maxSpeed;
    }
}

/**
 * Function: Car_Brake
 * -------------------
 * Decreases car speed by accelRate. Speed cannot go negative.
 */
void Car_Brake(Car* car) {
    if (car == NULL) {
        return;
    }
    if (car->speed <= 0) {
        return;
    }

    // If braking step would overshoot, just stop.
    if (car->accelRate >= car->speed) {
        car->speed = 0;
        return;
    }

    car->speed -= car->accelRate;
}

/**
 * Function: Car_Steer
 * -------------------
 * Rotates the car's facing angle. Movement direction follows the facing
 * angle since speed is scalar.
 */
void Car_Steer(Car* car, int deltaAngle512) {
    if (car == NULL) {
        return;
    }

    // Update facing angle (always works, even when stopped)
    car->angle512 = (car->angle512 + deltaAngle512) & ANGLE_MASK;
}

/**
 * Function: Car_Update
 * --------------------
 * Updates car physics for one frame. Applies friction, snaps low speeds to
 * zero, and integrates velocity into position. Call once per physics tick (60Hz).
 */
void Car_Update(Car* car) {
    if (car == NULL) {
        return;
    }

    // Apply friction (treat friction as multiplier in Q16.8; e.g., 250 ~= 0.9766)
    car->friction = clamp_friction(car->friction);
    car->speed = FixedMul(car->speed, car->friction);

    // Snap tiny speeds to 0 (prevents endless drifting)
    if (car->speed <= MIN_SPEED_THRESHOLD) {
        car->speed = 0;
    }

    // Cap speed (safety net)
    if (car->maxSpeed > 0 && car->speed > car->maxSpeed) {
        car->speed = car->maxSpeed;
    }

    // Integrate position
    Vec2 velocity = build_velocity(car);
    car->position = Vec2_Add(car->position, velocity);
}

//=============================================================================
// Public API - Read-Only Queries
//=============================================================================

/**
 * Function: Car_GetAngle
 * ----------------------
 * Returns the car's facing angle for sprite rotation.
 */
int Car_GetAngle(const Car* car) {
    if (car == NULL) {
        return 0;
    }
    return car->angle512;
}

/**
 * Function: Car_GetVelocityAngle
 * -------------------------------
 * Returns the car's movement direction. With scalar speed, this matches
 * the facing angle when moving.
 */
int Car_GetVelocityAngle(const Car* car) {
    if (car == NULL) {
        return 0;
    }
    return car->angle512;
}

/**
 * Function: Car_IsMoving
 * ----------------------
 * Checks if car speed is above the movement threshold.
 */
bool Car_IsMoving(const Car* car) {
    if (car == NULL) {
        return false;
    }
    return car->speed > MIN_MOVING_SPEED;
}

/**
 * Function: Car_GetSpeed
 * ----------------------
 * Returns the car's current speed magnitude.
 */
Q16_8 Car_GetSpeed(const Car* car) {
    if (car == NULL) {
        return 0;
    }
    return car->speed;
}

//=============================================================================
// Public API - Special Operations
//=============================================================================

/**
 * Function: Car_SetPosition
 * -------------------------
 * Directly sets car position for respawn/teleport effects.
 */
void Car_SetPosition(Car* car, Vec2 pos) {
    if (car == NULL) {
        return;
    }
    car->position = pos;
}

/**
 * Function: Car_SetVelocity
 * -------------------------
 * Sets car speed and direction from a velocity vector. Use for boosts,
 * collisions, and hazard effects. Speed is capped to maxSpeed.
 */
void Car_SetVelocity(Car* car, Vec2 velocity) {
    if (car == NULL) {
        return;
    }
    apply_velocity(car, velocity);
}

/**
 * Function: Car_ApplyImpulse
 * --------------------------
 * Applies an instant velocity change to the car. Useful for collision
 * responses and item effects. Speed is capped to maxSpeed.
 */
void Car_ApplyImpulse(Car* car, Vec2 impulse) {
    if (car == NULL) {
        return;
    }
    Vec2 currentVelocity = build_velocity(car);
    Vec2 newVelocity = Vec2_Add(currentVelocity, impulse);
    apply_velocity(car, newVelocity);
}

/**
 * Function: Car_SetAngle
 * ----------------------
 * Directly sets car facing angle. Movement direction will follow this angle
 * on the next update. Use for spawning and respawn orientation.
 */
void Car_SetAngle(Car* car, int angle512) {
    if (car == NULL) {
        return;
    }
    car->angle512 = angle512 & ANGLE_MASK;
}

//=============================================================================
// Public API - Game Events
//=============================================================================

/**
 * Function: Car_LapComplete
 * -------------------------
 * Increments the car's lap counter when a lap is completed.
 */
void Car_LapComplete(Car* car) {
    if (car == NULL) {
        return;
    }
    car->Lap++;
}
