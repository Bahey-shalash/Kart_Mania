/**
 * File: items_update.c
 * --------------------
 * Description: Core update and collision logic for the items system. Handles
 *              projectile movement, homing behavior, collision detection with
 *              cars and walls, item box pickups, and multiplayer synchronization.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 06.01.2026
 */

#include "items_internal.h"
#include "items_api.h"
#include "item_navigation.h"

#include <stdlib.h>

#include "../Car.h"
#include "../gameplay_logic.h"
#include "../wall_collision.h"
#include "../../core/game_constants.h"
#include "../../audio/sound.h"
#include "../../network/multiplayer.h"

//=============================================================================
// Internal Helper Prototypes
//=============================================================================
static void updateProjectile(TrackItem* item);
static void updateHoming(TrackItem* item, const Car* cars, int carCount);
static void updateHomingTargetLock(TrackItem* item, const Car* cars, int carCount,
                                   bool isMultiplayer);
static void updateHomingTargetPoint(TrackItem* item, const Car* cars, int carCount,
                                    bool isMultiplayer, const RaceState* state,
                                    Vec2* targetPoint);
static void applyHomingTurn(TrackItem* item, const Vec2* targetPoint);
static bool shouldCheckProjectileCar(const TrackItem* item, int carIndex,
                                     bool isMultiplayer);
static void applyProjectileHit(TrackItem* item, Car* car);
static bool isHazardHit(const TrackItem* item, const Car* car);
static void applyHazardHit(TrackItem* item, Car* car, int carIndex, Car* cars,
                           int carCount);
static void checkProjectileCollision(TrackItem* item, Car* cars, int carCount);
static void checkHazardCollision(TrackItem* item, Car* cars, int carCount);
static void explodeBomb(const Vec2* position, Car* cars, int carCount);
static bool checkItemCarCollision(const Vec2* itemPos, const Vec2* carPos,
                                  int itemHitbox);
static bool checkItemBoxPickup(const Car* car, ItemBoxSpawn* box);
static QuadrantID getQuadrantFromPos(const Vec2* pos);
static void checkItemBoxCollisions(Car* cars, int carCount);
static void checkAllProjectileCollisions(Car* cars, int carCount, int scrollX,
                                         int scrollY);
static void checkAllHazardCollisions(Car* cars, int carCount, int scrollX,
                                     int scrollY);
static bool isItemNearScreen(const Vec2* itemPos, int scrollX, int scrollY);
static void applyShellHitEffect(Car* car);
static void applyBananaHitEffect(Car* car);
static void applyOilHitEffect(Car* car, int carIndex);
static void handleItemBoxPickup(Car* car, ItemBoxSpawn* box, int carIndex,
                                int boxIndex);
static void Items_ReceiveMultiplayerUpdates(RaceState* raceState);
static void Items_UpdateTrackItems(RaceState* raceState);
static void Items_UpdateItemBoxRespawns(void);
static bool Items_TickItemLifetime(TrackItem* item, RaceState* raceState);
static void Items_TickItemImmunity(TrackItem* item, const RaceState* raceState);
static bool Item_IsProjectile(Item type);
static bool Item_IsHoming(Item type);
static bool Item_IsHazard(Item type);

//=============================================================================
// Lifecycle
//=============================================================================

void Items_Update(void) {
    RaceState* raceState = Race_GetState();

    Items_ReceiveMultiplayerUpdates(raceState);
    Items_UpdateTrackItems(raceState);
    Items_UpdateItemBoxRespawns();
}

void Items_CheckCollisions(Car* cars, int carCount, int scrollX, int scrollY) {
    checkItemBoxCollisions(cars, carCount);
    checkAllProjectileCollisions(cars, carCount, scrollX, scrollY);
    checkAllHazardCollisions(cars, carCount, scrollX, scrollY);
}

void Items_DeactivateBox(int boxIndex) {
    if (boxIndex < 0 || boxIndex >= itemBoxCount) {
        return;
    }

    itemBoxSpawns[boxIndex].active = false;
    itemBoxSpawns[boxIndex].respawnTimer = ITEM_BOX_RESPAWN_TICKS;
}

//=============================================================================
// Private Implementation
//=============================================================================

static void Items_ReceiveMultiplayerUpdates(RaceState* raceState) {
    if (raceState->gameMode != MultiPlayer) {
        return;
    }

    ItemPlacementData itemData;
    while ((itemData = Multiplayer_ReceiveItemPlacements()).valid) {
        if (itemData.speed > 0) {
            fireProjectileInternal(itemData.itemType, &itemData.position,
                                   itemData.angle512, itemData.speed,
                                   INVALID_CAR_INDEX, false,
                                   itemData.shooterCarIndex);
        } else {
            placeHazardInternal(itemData.itemType, &itemData.position, false);
        }
    }

    int boxIndex;
    while ((boxIndex = Multiplayer_ReceiveItemBoxPickup()) >= 0) {
        Items_DeactivateBox(boxIndex);
    }
}

static void Items_UpdateTrackItems(RaceState* raceState) {
    for (int i = 0; i < MAX_TRACK_ITEMS; i++) {
        if (!activeItems[i].active) {
            continue;
        }

        TrackItem* item = &activeItems[i];

        if (!Items_TickItemLifetime(item, raceState)) {
            continue;
        }

        Items_TickItemImmunity(item, raceState);

        if (Item_IsProjectile(item->type)) {
            updateProjectile(item);
        }

        if (Item_IsHoming(item->type)) {
            updateHoming(item, raceState->cars, raceState->carCount);
        }
    }
}

static void Items_UpdateItemBoxRespawns(void) {
    for (int i = 0; i < itemBoxCount; i++) {
        if (!itemBoxSpawns[i].active && itemBoxSpawns[i].respawnTimer > 0) {
            itemBoxSpawns[i].respawnTimer--;
            if (itemBoxSpawns[i].respawnTimer <= 0) {
                itemBoxSpawns[i].active = true;
            }
        }
    }
}

static bool Items_TickItemLifetime(TrackItem* item, RaceState* raceState) {
    if (item->lifetime_ticks > 0) {
        item->lifetime_ticks--;
        if (item->lifetime_ticks <= 0) {
            if (item->type == ITEM_BOMB) {
                explodeBomb(&item->position, raceState->cars, raceState->carCount);
            }
            item->active = false;
            return false;
        }
    }

    return true;
}

static void Items_TickItemImmunity(TrackItem* item, const RaceState* raceState) {
    if (item->immunityTimer == 0) {
        return;
    }

    if (item->immunityTimer > 0) {
        item->immunityTimer--;

        if (item->shooterCarIndex >= 0 &&
            item->shooterCarIndex < raceState->carCount) {
            const Car* shooter = &raceState->cars[item->shooterCarIndex];
            Q16_8 distFromShooter =
                Vec2_Distance(&item->position, &shooter->position);

            if (distFromShooter >= IMMUNITY_MIN_DISTANCE) {
                item->immunityTimer = 0;
            }
        }
    } else if (item->immunityTimer == -1) {
        if (!item->hasCompletedLap && item->waypointsVisited > 0) {
            int waypointDiff =
                abs(item->currentWaypoint - item->startingWaypoint);

            if (waypointDiff <= WAYPOINT_LAP_THRESHOLD &&
                item->waypointsVisited > 100) {
                item->hasCompletedLap = true;
                item->immunityTimer = 0;
            }
        }
    }
}

static void updateProjectile(TrackItem* item) {
    // Move projectile
    Vec2 velocity = Vec2_FromAngle(item->angle512);
    velocity = Vec2_Scale(velocity, item->speed);
    item->position = Vec2_Add(item->position, velocity);

    // Check wall collision
    int x = FixedToInt(item->position.x);
    int y = FixedToInt(item->position.y);
    QuadrantID quad = getQuadrantFromPos(&item->position);

    if (Wall_CheckCollision(x, y, item->hitbox_width / 2, quad)) {
        item->active = false;  // Despawn on wall hit
    }
}

static void updateHoming(TrackItem* item, const Car* cars, int carCount) {
    const RaceState* state = Race_GetState();
    bool isMultiplayer = (state->gameMode == MultiPlayer);

    updateHomingTargetLock(item, cars, carCount, isMultiplayer);

    Vec2 targetPoint = item->position;
    updateHomingTargetPoint(item, cars, carCount, isMultiplayer, state,
                            &targetPoint);
    applyHomingTurn(item, &targetPoint);
}

static void updateHomingTargetLock(TrackItem* item, const Car* cars, int carCount,
                                   bool isMultiplayer) {
    // Never keep the shooter as a target in multiplayer
    if (isMultiplayer && item->targetCarIndex == item->shooterCarIndex) {
        item->targetCarIndex = INVALID_CAR_INDEX;
    }

    // If no target is locked, scan for nearby cars to attack
    if (item->targetCarIndex == INVALID_CAR_INDEX) {
        Q16_8 lockOnRadius = IntToFixed(100);  // 100 pixels detection range

        for (int i = 0; i < carCount; i++) {
            if (isMultiplayer && i == item->shooterCarIndex) {
                continue;  // Never lock onto the shooter in multiplayer
            }

            // Single player keeps the old immunity-based skip
            if (!isMultiplayer && item->immunityTimer != 0 &&
                i == item->shooterCarIndex) {
                continue;
            }

            Q16_8 dist = Vec2_Distance(&item->position, &cars[i].position);
            if (dist <= lockOnRadius) {
                // Lock onto this car!
                item->targetCarIndex = i;
                item->usePathFollowing = false;  // Switch to direct attack
                break;
            }
        }
    }
}

static void updateHomingTargetPoint(TrackItem* item, const Car* cars, int carCount,
                                    bool isMultiplayer, const RaceState* state,
                                    Vec2* targetPoint) {
    *targetPoint = item->position;

    // If we have a locked target, check if we should stay locked
    if (item->targetCarIndex >= 0 && item->targetCarIndex < carCount) {
        if (isMultiplayer && item->targetCarIndex == item->shooterCarIndex) {
            item->targetCarIndex = INVALID_CAR_INDEX;
            item->usePathFollowing = true;
        } else {
            const Car* target = &cars[item->targetCarIndex];
            Q16_8 distToTarget = Vec2_Distance(&item->position, &target->position);

            // If target is too far away, unlock and return to path following
            if (distToTarget > IntToFixed(150)) {  // 150 pixel leash
                item->targetCarIndex = INVALID_CAR_INDEX;
                item->usePathFollowing = true;
            } else {
                // Stay locked - aim directly at target
                *targetPoint = target->position;
                item->usePathFollowing = false;
            }
        }
    }

    // If no target or using path following, follow waypoints
    if (item->usePathFollowing || item->targetCarIndex == INVALID_CAR_INDEX) {
        // Get current waypoint position
        Vec2 waypointPos =
            ItemNav_GetWaypointPosition(item->currentWaypoint, state->currentMap);

        // Check if we've reached this waypoint
        if (ItemNav_IsWaypointReached(&item->position, &waypointPos)) {
            // Advance to next waypoint
            item->currentWaypoint =
                ItemNav_GetNextWaypoint(item->currentWaypoint, state->currentMap);
            item->waypointsVisited++;
        }

        // Aim toward current waypoint
        *targetPoint = waypointPos;
    }
}

static void applyHomingTurn(TrackItem* item, const Vec2* targetPoint) {
    // Smooth turn toward target point
    Vec2 toTarget = Vec2_Sub(*targetPoint, item->position);
    int targetAngle = Vec2_ToAngle(&toTarget);

    int angleDiff = (targetAngle - item->angle512) & ANGLE_MASK;
    if (angleDiff > ANGLE_HALF)
        angleDiff -= ANGLE_FULL;

    if (angleDiff > HOMING_TURN_RATE)
        angleDiff = HOMING_TURN_RATE;
    if (angleDiff < -HOMING_TURN_RATE)
        angleDiff = -HOMING_TURN_RATE;

    item->angle512 = (item->angle512 + angleDiff) & ANGLE_MASK;
}

static bool shouldCheckProjectileCar(const TrackItem* item, int carIndex,
                                     bool isMultiplayer) {
    // In multiplayer, only check collision for connected players
    if (isMultiplayer && !Multiplayer_IsPlayerConnected(carIndex)) {
        return false;
    }

    // In multiplayer, never collide with the shooter
    if (isMultiplayer && carIndex == item->shooterCarIndex) {
        return false;
    }

    // immunityTimer > 0: multiplayer time-based immunity
    // immunityTimer == -1 AND !hasCompletedLap: single player lap-based immunity
    bool hasImmunity = (item->immunityTimer > 0) ||
                       (item->immunityTimer == -1 && !item->hasCompletedLap);

    // In single player, keep the old immunity-based shooter skip
    if (!isMultiplayer && hasImmunity && carIndex == item->shooterCarIndex) {
        return false;  // Can't hit the shooter yet
    }

    return true;
}

static void applyProjectileHit(TrackItem* item, Car* car) {
    // Apply effect based on projectile type
    if (item->type == ITEM_GREEN_SHELL || item->type == ITEM_RED_SHELL) {
        applyShellHitEffect(car);
    } else if (item->type == ITEM_MISSILE) {
        car->speed = 0;
    }

    item->active = false;  // Despawn projectile
}

static bool isHazardHit(const TrackItem* item, const Car* car) {
    return checkItemCarCollision(&item->position, &car->position,
                                 item->hitbox_width);
}

static void applyHazardHit(TrackItem* item, Car* car, int carIndex, Car* cars,
                           int carCount) {
    // Apply hazard effect based on item type
    switch (item->type) {
        case ITEM_BANANA:
            applyBananaHitEffect(car);
            item->active = false;
            break;

        case ITEM_OIL:
            applyOilHitEffect(car, carIndex);
            // Oil persists
            break;

        case ITEM_BOMB:
            explodeBomb(&item->position, cars, carCount);
            item->active = false;
            break;

        default:
            break;
    }
}

static void checkProjectileCollision(TrackItem* item, Car* cars, int carCount) {
    // Get race state to check if we're in multiplayer mode
    const RaceState* state = Race_GetState();
    bool isMultiplayer = (state->gameMode == MultiPlayer);

    for (int i = 0; i < carCount; i++) {
        if (!shouldCheckProjectileCar(item, i, isMultiplayer)) {
            continue;
        }

        if (checkItemCarCollision(&item->position, &cars[i].position,
                                  item->hitbox_width)) {
            applyProjectileHit(item, &cars[i]);
            break;
        }
    }
}

static void checkHazardCollision(TrackItem* item, Car* cars, int carCount) {
    for (int i = 0; i < carCount; i++) {
        if (isHazardHit(item, &cars[i])) {
            applyHazardHit(item, &cars[i], i, cars, carCount);

            if (!item->active)
                break;
        }
    }

    // Bomb auto-explodes when timer runs out
    if (item->type == ITEM_BOMB && item->lifetime_ticks <= 0) {
        explodeBomb(&item->position, cars, carCount);
        item->active = false;
    }
}

static void explodeBomb(const Vec2* position, Car* cars, int carCount) {
    for (int i = 0; i < carCount; i++) {
        Q16_8 dist = Vec2_Distance(position, &cars[i].position);

        if (dist <= BOMB_EXPLOSION_RADIUS) {
            // Stop car completely
            cars[i].speed = 0;
            cars[i].angle512 = (cars[i].angle512 + ANGLE_HALF) & ANGLE_MASK;  // 180° flip

            // Knockback away from bomb
            Vec2 knockbackDir = Vec2_Sub(cars[i].position, *position);
            if (!Vec2_IsZero(knockbackDir)) {
                knockbackDir = Vec2_Normalize(&knockbackDir);
                Vec2 knockback =
                    Vec2_Scale(knockbackDir, IntToFixed(BOMB_KNOCKBACK_DISTANCE));
                cars[i].position = Vec2_Add(cars[i].position, knockback);
            }
        }
    }
}

static bool checkItemBoxPickup(const Car* car, ItemBoxSpawn* box) {
    Q16_8 dist = Vec2_Distance(&car->position, &box->position);
    int pickupRadius = (CAR_RADIUS + ITEM_BOX_HITBOX);
    return (dist <= IntToFixed(pickupRadius));
}

static bool checkItemCarCollision(const Vec2* itemPos, const Vec2* carPos,
                                  int itemHitbox) {
    Q16_8 dist = Vec2_Distance(itemPos, carPos);
    int hitRadius = (itemHitbox + CAR_COLLISION_SIZE) / 2;
    return (dist <= IntToFixed(hitRadius));
}

static void applyShellHitEffect(Car* car) {
    // Stop car and spin it 45° in random direction
    car->speed = 0;
    int spinDirection =
        (rand() % 2 == 0) ? SHELL_SPIN_ANGLE_POS : SHELL_SPIN_ANGLE_NEG;
    car->angle512 = (car->angle512 + spinDirection) & ANGLE_MASK;
}

static void applyBananaHitEffect(Car* car) {
    // Spin car 180° and keep speed reduction
    car->speed = car->speed / BANANA_SPEED_DIVISOR;
    car->angle512 = (car->angle512 + ANGLE_HALF) & ANGLE_MASK;  // 180° turn
}

static void applyOilHitEffect(Car* car, int carIndex) {
    // Get race state to determine player index
    const RaceState* state = Race_GetState();
    int playerIndex = state->playerIndex;

    // Apply oil slow to player only
    if (carIndex == playerIndex) {
        Items_ApplyOilSlow(car, &playerEffects);
    } else {
        car->speed = car->speed / OIL_SPEED_DIVISOR;
    }
}

static void handleItemBoxPickup(Car* car, ItemBoxSpawn* box, int carIndex,
                                int boxIndex) {
    // Get race state to determine player index
    const RaceState* state = Race_GetState();
    int playerIndex = state->playerIndex;

    // Give item to the local player only (not AI or remote players)
    if (carIndex == playerIndex) {
        // PLAY SOUND - only for the local player who picked up the box
        PlayBoxSFX();

        if (car->item == ITEM_NONE) {
            Item receivedItem = Items_GetRandomItem(car->rank);
            car->item = receivedItem;
        }

        // In multiplayer, broadcast the pickup to other players
        if (state->gameMode == MultiPlayer) {
            Multiplayer_SendItemBoxPickup(boxIndex);
        }
    }
    // Deactivate box and start respawn timer
    box->active = false;
    box->respawnTimer = ITEM_BOX_RESPAWN_TICKS;
}

static void checkItemBoxCollisions(Car* cars, int carCount) {
    // DEBUG: Track if we're checking item boxes
    static bool debugItemBoxes = true;
    (void)debugItemBoxes;

    // Get race state to check if we're in multiplayer mode
    const RaceState* state = Race_GetState();
    bool isMultiplayer = (state->gameMode == MultiPlayer);

    for (int i = 0; i < itemBoxCount; i++) {
        if (!itemBoxSpawns[i].active)
            continue;

        for (int c = 0; c < carCount; c++) {
            // In multiplayer, only check collision for connected players
            if (isMultiplayer && !Multiplayer_IsPlayerConnected(c)) {
                continue;
            }

            if (checkItemBoxPickup(&cars[c], &itemBoxSpawns[i])) {
                handleItemBoxPickup(&cars[c], &itemBoxSpawns[i], c, i);
                break;
            }
        }
    }
}

static void checkAllProjectileCollisions(Car* cars, int carCount, int scrollX,
                                         int scrollY) {
    for (int i = 0; i < MAX_TRACK_ITEMS; i++) {
        if (!activeItems[i].active)
            continue;

        TrackItem* item = &activeItems[i];

        if (Item_IsProjectile(item->type)) {
            // Only check collision if item is near the screen
            if (isItemNearScreen(&item->position, scrollX, scrollY)) {
                checkProjectileCollision(item, cars, carCount);
            }
        }
    }
}

static void checkAllHazardCollisions(Car* cars, int carCount, int scrollX,
                                     int scrollY) {
    for (int i = 0; i < MAX_TRACK_ITEMS; i++) {
        if (!activeItems[i].active)
            continue;

        TrackItem* item = &activeItems[i];

        if (Item_IsHazard(item->type)) {
            // Only check collision if item is near the screen
            if (isItemNearScreen(&item->position, scrollX, scrollY)) {
                checkHazardCollision(item, cars, carCount);
            }
        }
    }
}

static bool isItemNearScreen(const Vec2* itemPos, int scrollX, int scrollY) {
    int itemX = FixedToInt(itemPos->x);
    int itemY = FixedToInt(itemPos->y);

    int screenLeft = scrollX - COLLISION_BUFFER_ZONE;
    int screenRight = scrollX + SCREEN_WIDTH + COLLISION_BUFFER_ZONE;
    int screenTop = scrollY - COLLISION_BUFFER_ZONE;
    int screenBottom = scrollY + SCREEN_HEIGHT + COLLISION_BUFFER_ZONE;

    return (itemX >= screenLeft && itemX <= screenRight && itemY >= screenTop &&
            itemY <= screenBottom);
}

static QuadrantID getQuadrantFromPos(const Vec2* pos) {
    int x = FixedToInt(pos->x);
    int y = FixedToInt(pos->y);

    int col = (x < QUAD_BOUNDARY_LOW) ? 0 : (x < QUAD_BOUNDARY_HIGH) ? 1 : 2;
    int row = (y < QUAD_BOUNDARY_LOW) ? 0 : (y < QUAD_BOUNDARY_HIGH) ? 1 : 2;

    return (QuadrantID)(row * QUADRANT_GRID_SIZE + col);
}

static bool Item_IsProjectile(Item type) {
    return (type == ITEM_GREEN_SHELL || type == ITEM_RED_SHELL ||
            type == ITEM_MISSILE);
}

static bool Item_IsHoming(Item type) {
    return (type == ITEM_RED_SHELL || type == ITEM_MISSILE);
}

static bool Item_IsHazard(Item type) {
    return (type == ITEM_BANANA || type == ITEM_OIL || type == ITEM_BOMB);
}
