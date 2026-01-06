#include "items_internal.h"
#include "items_api.h"

#include <stdlib.h>

#include "../Car.h"
#include "../gameplay_logic.h"
#include "../../core/game_constants.h"

static int findCarAhead(int currentRank, int carCount);
static int findCarInDirection(Vec2 fromPosition, int direction512, int playerIndex,
                              const Car* cars, int carCount);

void Items_UsePlayerItem(Car* player, bool fireForward) {
    if (player->item == ITEM_NONE)
        return;

    Item itemType = player->item;
    player->item = ITEM_NONE;  // Clear inventory

    switch (itemType) {
        case ITEM_BANANA: {
            // Calculate backward direction (180° from facing direction)
            int backwardAngle = (player->angle512 + ANGLE_HALF) & ANGLE_MASK;  // +180°
            Vec2 backward = Vec2_FromAngle(backwardAngle);
            Vec2 offset = Vec2_Scale(backward, IntToFixed(BANANA_DROP_OFFSET));
            Vec2 dropPos = Vec2_Add(player->position, offset);
            Items_PlaceHazard(itemType, dropPos);
            break;
        }
        case ITEM_BOMB: {
            // Calculate backward direction (180° from facing direction)
            int backwardAngle = (player->angle512 + ANGLE_HALF) & ANGLE_MASK;  // +180°
            Vec2 backward = Vec2_FromAngle(backwardAngle);
            Vec2 offset = Vec2_Scale(backward, IntToFixed(BOMB_DROP_OFFSET));
            Vec2 dropPos = Vec2_Add(player->position, offset);
            Items_PlaceHazard(itemType, dropPos);
            break;
        }
        case ITEM_OIL: {
            // Calculate backward direction (180° from facing direction)
            int backwardAngle = (player->angle512 + ANGLE_HALF) & ANGLE_MASK;  // +180°
            Vec2 backward = Vec2_FromAngle(backwardAngle);
            Vec2 offset = Vec2_Scale(backward, IntToFixed(HAZARD_DROP_OFFSET));
            Vec2 dropPos = Vec2_Add(player->position, offset);
            Items_PlaceHazard(itemType, dropPos);
            break;
        }

        case ITEM_GREEN_SHELL: {
            // Fire in specified direction
            int fireAngle = fireForward
                                ? player->angle512
                                : ((player->angle512 + ANGLE_HALF) & ANGLE_MASK);

            // Spawn shell slightly ahead of player to avoid immediate collision
            Vec2 forward = Vec2_FromAngle(fireAngle);
            Vec2 offset = Vec2_Scale(forward, IntToFixed(PROJECTILE_SPAWN_OFFSET));
            Vec2 spawnPos = Vec2_Add(player->position, offset);

            Q16_8 shellSpeed = FixedMul(player->maxSpeed, GREEN_SHELL_SPEED_MULT);
            Items_FireProjectile(ITEM_GREEN_SHELL, spawnPos, fireAngle, shellSpeed,
                                 INVALID_CAR_INDEX);
            break;
        }

        case ITEM_RED_SHELL: {
            // Fire using car's current angle (forward or backward)
            int fireAngle = fireForward
                                ? player->angle512
                                : ((player->angle512 + ANGLE_HALF) & ANGLE_MASK);

            // Spawn shell slightly ahead of player to avoid immediate collision
            Vec2 forward = Vec2_FromAngle(fireAngle);
            Vec2 offset = Vec2_Scale(forward, IntToFixed(PROJECTILE_SPAWN_OFFSET));
            Vec2 spawnPos = Vec2_Add(player->position, offset);

            Q16_8 shellSpeed = FixedMul(player->maxSpeed, RED_SHELL_SPEED_MULT);

            // Red shell follows the racing line and locks onto any nearby car
            // targetCarIndex = INVALID_CAR_INDEX means "attack first car you get close
            // to"
            Items_FireProjectile(ITEM_RED_SHELL, spawnPos, fireAngle, shellSpeed,
                                 INVALID_CAR_INDEX);
            break;
        }

        case ITEM_MISSILE: {
            // Fire at 1st place
            int targetIndex = findCarAhead(1, MAX_CARS);  // Find 1st place car
            Q16_8 missileSpeed = FixedMul(player->maxSpeed, MISSILE_SPEED_MULT);
            Items_FireProjectile(ITEM_MISSILE, player->position, player->angle512,
                                 missileSpeed, targetIndex);
            break;
        }

        case ITEM_MUSHROOM: {
            // Apply confusion to player
            Items_ApplyConfusion(&playerEffects);
            break;
        }

        case ITEM_SPEEDBOOST: {
            // Apply speed boost to player
            Items_ApplySpeedBoost(player, &playerEffects);
            break;
        }

        default:
            break;
    }
}

Item Items_GetRandomItem(int playerRank) {
    // Clamp rank to valid range (1-8+)
    int rankIndex = playerRank - 1;  // Convert to 0-indexed
    if (rankIndex < 0)
        rankIndex = 0;
    if (rankIndex >= 8)
        rankIndex = 7;

    const RaceState* state = Race_GetState();
    const ItemProbability* table =
        (state && state->gameMode == MultiPlayer) ? ITEM_PROBABILITIES_MP
                                                  : ITEM_PROBABILITIES_SP;
    const ItemProbability* prob = &table[rankIndex];

    // Calculate total probability
    int total = prob->banana + prob->oil + prob->bomb + prob->greenShell +
                prob->redShell + prob->missile + prob->mushroom + prob->speedBoost;

    // Generate random number
    int roll = rand() % total;

    // Determine which item based on probability ranges
    int cumulative = 0;

    cumulative += prob->banana;
    if (roll < cumulative)
        return ITEM_BANANA;

    cumulative += prob->oil;
    if (roll < cumulative)
        return ITEM_OIL;

    cumulative += prob->bomb;
    if (roll < cumulative)
        return ITEM_BOMB;

    cumulative += prob->greenShell;
    if (roll < cumulative)
        return ITEM_GREEN_SHELL;

    cumulative += prob->redShell;
    if (roll < cumulative)
        return ITEM_RED_SHELL;

    cumulative += prob->missile;
    if (roll < cumulative)
        return ITEM_MISSILE;

    cumulative += prob->mushroom;
    if (roll < cumulative)
        return ITEM_MUSHROOM;

    return ITEM_SPEEDBOOST;
}

//=============================================================================
// Internal helpers
//=============================================================================

static int findCarAhead(int currentRank, int carCount) {
    const RaceState* state = Race_GetState();
    int playerIndex = state->playerIndex;
    const Car* player = &state->cars[playerIndex];

    // Use the player's current facing direction to find targets ahead
    return findCarInDirection(player->position, player->angle512, playerIndex,
                              state->cars, carCount);
}

// Find the car that is most ahead in the given direction
// direction512: The angle to search in (typically player's facing angle)
static int findCarInDirection(Vec2 fromPosition, int direction512, int playerIndex,
                              const Car* cars, int carCount) {
    // If there's only one car (player), return invalid
    if (carCount <= 1) {
        return INVALID_CAR_INDEX;
    }

    // Direction vector
    Vec2 directionVec = Vec2_FromAngle(direction512);

    int bestTarget = INVALID_CAR_INDEX;
    Q16_8 bestScore = IntToFixed(-1);  // Best score = furthest ahead in direction

    for (int i = 0; i < carCount; i++) {
        // Skip the player themselves
        if (i == playerIndex) {
            continue;
        }

        const Car* otherCar = &cars[i];

        // Vector from player to other car
        Vec2 toOther = Vec2_Sub(otherCar->position, fromPosition);

        // Skip if other car is at the same position
        if (Vec2_IsZero(toOther)) {
            continue;
        }

        // Calculate dot product: how much is this car in our facing direction?
        // Positive = ahead, Negative = behind
        Q16_8 dotProduct = Vec2_Dot(toOther, directionVec);

        // Only consider cars that are ahead (positive dot product)
        if (dotProduct <= 0) {
            continue;
        }

        // Get angle between our direction and the car
        int angleToOther = Vec2_ToAngle(toOther);
        int angleDiff = (angleToOther - direction512) & ANGLE_MASK;
        if (angleDiff > ANGLE_HALF) {
            angleDiff = ANGLE_FULL - angleDiff;
        }

        // Only target cars within a 90-degree cone ahead (45 degrees either side)
        if (angleDiff > ANGLE_QUARTER) {
            continue;
        }

        // Score = distance in the direction (dot product)
        // Higher score = further ahead in our direction
        if (dotProduct > bestScore) {
            bestScore = dotProduct;
            bestTarget = i;
        }
    }

    return bestTarget;
}
