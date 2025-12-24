#ifndef CAR_H
#define CAR_H

#include <string.h>

#include "vect2.h"

#define NUMBER_OF_CARS 1

typedef enum { ITEM_NONE = 0, ITEM_OIL, ITEM_BOMB } Item;

typedef struct {
    Vec2 position;
    Vec2 velocity;

    Q16_8 maxSpeed;
    Q16_8 accelRate;
    Q16_8 friction;
    int angle512;
    int Lap;
    int rank;  // raceposotion 1st 2nd etc
    int lastCheckpoint;

    Item item;
    char carname[32];
} Car;

// constructor

static inline Car CarCreate(Vec2 pos, Vec2 speed, Q16_8 SpeedMax, Q16_8 accel_rate,
                            Q16_8 frictionn, Item init_item, const char* name) {
    Car car = {
        .position = pos,
        .velocity = speed,
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
        .velocity = Vec2_Zero(),
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

// Lifecycle
void Car_Init(Car* car, Vec2 pos, const char* name, Q16_8 maxSpeed, Q16_8 accelRate,
              Q16_8 friction);

void Car_Reset(Car* car, Vec2 spawnPos);

// Physics
void Car_Accelerate(Car* car);
void Car_Brake(Car* car);
void Car_Steer(Car* car, int deltaAngle512);
void Car_Update(Car* car);

// Utility
void Car_SetPosition(Car* car, Vec2 pos);
int GetCarAngle(const Car* car);
void Car_LapComplete(Car* car);

#endif  // CAR_H
