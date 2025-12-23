#include "Car.h"

#include <string.h>
#define NULL ((void*)0)

// ---------------------------------------------
// Helpers
// ---------------------------------------------
static void copy_name(char destination[32], const char* name) {
    destination[0] = '\0';
    if (name == NULL) {
        return;
    }
    strncpy(destination, name, 31);
    destination[31] = '\0';
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

// ---------------------------------------------
// Lifecycle
// ---------------------------------------------
void Car_Init(Car* car, Vec2 pos, const char* name, Q16_8 maxSpeed, Q16_8 accelRate,
              Q16_8 friction) {
    if (car == NULL) {
        return;
    }

    car->position = pos;
    car->velocity = Vec2_Zero();
    car->maxSpeed = maxSpeed;
    car->accelRate = accelRate;
    car->friction = clamp_friction(friction);
    car->item = ITEM_NONE;
    copy_name(car->carname, name);
}

void Car_Reset(Car* car, Vec2 spawnPos) {
    car->position = spawnPos;
    car->velocity = Vec2_Zero();
    car->item = ITEM_NONE;
    // keeps name, maxSpeed, accelRate, friction
}
// ---------------------------------------------
// Physics
// ---------------------------------------------

// Accelerate "forward".
// Without a stored heading, we infer forward from current velocity.
// If stopped, we accelerate along +X (angle = 0).
void Car_Accelerate(Car* car) {
    if (car == NULL) {
        return;
    }

    Vec2 forward;
    if (!Vec2_IsZero(car->velocity)) {
        // Forward = normalized velocity direction
        forward = Vec2_Normalize(car->velocity);
    } else {
        // Standing still: choose a default forward direction
        forward = Vec2_FromAngle(0);  // (1, 0) in Q16.8
    }

    // dv = forward * accelRate
    Vec2 dv = Vec2_Scale(forward, car->accelRate);
    car->velocity = Vec2_Add(car->velocity, dv);

    // Cap speed
    if (car->maxSpeed > 0) {
        car->velocity = Vec2_ClampLen(car->velocity, car->maxSpeed);
    }
}

// Brake reduces speed along the opposite of current velocity.
// If accelRate is "brake strength" too, this is symmetric and simple.
void Car_Brake(Car* car) {
    if (car == NULL) {
        return;
    }
    if (Vec2_IsZero(car->velocity)) {
        return;
    }

    Q16_8 speed = Vec2_Len(car->velocity);
    if (speed <= 0) {
        car->velocity = Vec2_Zero();
        return;
    }

    // If braking step would overshoot, just stop.
    if (car->accelRate >= speed) {
        car->velocity = Vec2_Zero();
        return;
    }

    Vec2 dir = Vec2_Normalize(car->velocity);   // direction of travel
    Vec2 dv = Vec2_Scale(dir, car->accelRate);  // amount to remove
    car->velocity = Vec2_Sub(car->velocity, dv);
}

// Steering rotates the velocity vector by deltaAngle512.
// If velocity is zero, nothing happens (no heading stored).
void Car_Steer(Car* car, int deltaAngle512) {
    if (car == NULL) {
        return;
    }
    if (Vec2_IsZero(car->velocity)) {
        return;
    }

    car->velocity = Vec2_Rotate(car->velocity, deltaAngle512);
}

// Update integrates velocity into position and applies friction + speed cap.
void Car_Update(Car* car) {
    if (car == NULL) {
        return;
    }

    // Apply friction (treat friction as multiplier in Q16.8; e.g., 250 ~= 0.9766)
    car->friction = clamp_friction(car->friction);
    car->velocity = Vec2_Scale(car->velocity, car->friction);

    // Snap tiny velocities to 0 (optional, prevents endless drifting due to
    // quantization) Threshold = 1/256 px/frame (one LSB in Q16.8)
    if (FixedAbs(car->velocity.x) <= 1) {
        car->velocity.x = 0;
    }
    if (FixedAbs(car->velocity.y) <= 1) {
        car->velocity.y = 0;
    }

    // Cap speed
    if (car->maxSpeed > 0) {
        car->velocity = Vec2_ClampLen(car->velocity, car->maxSpeed);
    }

    // Integrate
    car->position = Vec2_Add(car->position, car->velocity);
}

// ---------------------------------------------
// Utility
// ---------------------------------------------
void Car_SetPosition(Car* car, Vec2 pos) {
    if (car == NULL) {
        return;
    }
    car->position = pos;
}
