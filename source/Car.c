// BAHEY------
#include "Car.h"

#include <stddef.h>
#include <string.h>

#include "game_constants.h"

//=============================================================================
// Private Helpers
//=============================================================================

static void copy_name(char destination[32], const char* name) {
    destination[0] = '\0';
    if (name == NULL) {
        return;
    }
    strncpy(destination, name, CAR_NAME_MAX_LENGTH);
    destination[CAR_NAME_MAX_LENGTH] = '\0';
}

// Clamp friction to [0, FIXED_ONE]
static Q16_8 clamp_friction(Q16_8 friction) {
    if (friction < 0) {
        return 0;
    }
    if (friction > FIXED_ONE) {
        return FIXED_ONE;
    }
    return friction;
}

// Build a velocity vector from the car's current facing + speed magnitude
static Vec2 build_velocity(const Car* car) {
    if (car == NULL || car->speed == 0) {
        return Vec2_Zero();
    }
    Vec2 forward = Vec2_FromAngle(car->angle512);
    return Vec2_Scale(forward, car->speed);
}

// Convert a velocity vector into internal speed/angle representation
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
// Lifecycle
//=============================================================================

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
    // Note: We intentionally do NOT reset maxSpeed, accelRate, friction, or carname
    // These are car properties that should persist across resets
}

//=============================================================================
// Physics Control
//=============================================================================

// Accelerate in the direction the car is facing (angle512).
// This allows acceleration from standstill in any direction.
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

// Brake reduces speed along the current facing direction.
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

// Steering rotates the car's facing angle.
// Movement direction always follows the facing since speed is scalar.
void Car_Steer(Car* car, int deltaAngle512) {
    if (car == NULL) {
        return;
    }

    // Update facing angle (always works, even when stopped)
    car->angle512 = (car->angle512 + deltaAngle512) & ANGLE_MASK;
}

// Update integrates speed/angle into position and applies friction + speed cap.
// Call this once per physics tick (e.g., 60Hz).
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
// Read-Only Queries
//=============================================================================

// Returns the car's facing angle (not velocity direction)
// Use this for sprite rotation
int Car_GetAngle(const Car* car) {
    if (car == NULL) {
        return 0;
    }
    return car->angle512;
}

// Get the car's current movement direction (for debugging/display)
// With scalar speed, this matches angle512 whenever the car is moving.
int Car_GetVelocityAngle(const Car* car) {
    if (car == NULL) {
        return 0;
    }
    return car->angle512;
}

// Check if car is moving (speed above threshold)
bool Car_IsMoving(const Car* car) {
    if (car == NULL) {
        return false;
    }
    return car->speed > MIN_MOVING_SPEED;
}

// Get current speed (magnitude of velocity)
Q16_8 Car_GetSpeed(const Car* car) {
    if (car == NULL) {
        return 0;
    }
    return car->speed;
}

//=============================================================================
// Special Operations
//=============================================================================

void Car_SetPosition(Car* car, Vec2 pos) {
    if (car == NULL) {
        return;
    }
    car->position = pos;
}

// Set speed/angle directly from a velocity vector (use with caution - prefer
// Accelerate/Brake/Steer). Useful for external forces like boosts, collisions,
// or hazards.
void Car_SetVelocity(Car* car, Vec2 velocity) {
    if (car == NULL) {
        return;
    }
    apply_velocity(car, velocity);
}

// Apply an impulse (instant velocity change)
// Useful for collisions, item effects, etc.
void Car_ApplyImpulse(Car* car, Vec2 impulse) {
    if (car == NULL) {
        return;
    }
    Vec2 currentVelocity = build_velocity(car);
    Vec2 newVelocity = Vec2_Add(currentVelocity, impulse);
    apply_velocity(car, newVelocity);
}

// Set the car's facing angle directly. Movement direction will follow this
// angle on the next update since speed is scalar.
void Car_SetAngle(Car* car, int angle512) {
    if (car == NULL) {
        return;
    }
    car->angle512 = angle512 & ANGLE_MASK;
}

//=============================================================================
// Game Events
//=============================================================================

void Car_LapComplete(Car* car) {
    if (car == NULL) {
        return;
    }
    car->Lap++;
}
