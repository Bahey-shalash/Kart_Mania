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

static void updateProjectile(TrackItem* item, const Car* cars, int carCount);
static void updateHoming(TrackItem* item, const Car* cars, int carCount);
static void checkProjectileCollision(TrackItem* item, Car* cars, int carCount);
static void checkHazardCollision(TrackItem* item, Car* cars, int carCount);
static void explodeBomb(Vec2 position, Car* cars, int carCount);
static bool checkItemCarCollision(Vec2 itemPos, Vec2 carPos, int itemHitbox);
static bool checkItemBoxPickup(const Car* car, ItemBoxSpawn* box);
static QuadrantID getQuadrantFromPos(Vec2 pos);
static void checkItemBoxCollisions(Car* cars, int carCount);
static void checkAllProjectileCollisions(Car* cars, int carCount, int scrollX,
                                         int scrollY);
static void checkAllHazardCollisions(Car* cars, int carCount, int scrollX,
                                     int scrollY);
static bool isItemNearScreen(Vec2 itemPos, int scrollX, int scrollY);
static void applyShellHitEffect(Car* car);
static void applyBananaHitEffect(Car* car);
static void applyOilHitEffect(Car* car, int carIndex);
static void handleItemBoxPickup(Car* car, ItemBoxSpawn* box, int carIndex,
                                int boxIndex);

//=============================================================================
// Lifecycle
//=============================================================================

void Items_Update(void) {
    const RaceState* raceState = Race_GetState();

    // In multiplayer, receive item placements from other players
    if (raceState->gameMode == MultiPlayer) {
        ItemPlacementData itemData;
        while ((itemData = Multiplayer_ReceiveItemPlacements()).valid) {
            if (itemData.speed > 0) {
                fireProjectileInternal(itemData.itemType, itemData.position,
                                       itemData.angle512, itemData.speed,
                                       INVALID_CAR_INDEX, false,
                                       itemData.shooterCarIndex);
            } else {
                placeHazardInternal(itemData.itemType, itemData.position, false);
            }
        }

        int boxIndex;
        while ((boxIndex = Multiplayer_ReceiveItemBoxPickup()) >= 0) {
            Items_DeactivateBox(boxIndex);
        }
    }

    // Update active track items
    for (int i = 0; i < MAX_TRACK_ITEMS; i++) {
        if (!activeItems[i].active)
            continue;

        TrackItem* item = &activeItems[i];

        // Update lifetime
        if (item->lifetime_ticks > 0) {
            item->lifetime_ticks--;
            if (item->lifetime_ticks <= 0) {
                item->active = false;
                continue;
            }
        }

        // Update immunity timer
        if (item->immunityTimer != 0) {

            if (item->immunityTimer > 0) {
                // MULTIPLAYER MODE: Time-based immunity
                item->immunityTimer--;

                // Check distance - if far enough from shooter, remove immunity early
                if (item->shooterCarIndex >= 0 &&
                    item->shooterCarIndex < raceState->carCount) {
                    const Car* shooter = &raceState->cars[item->shooterCarIndex];
                    Q16_8 distFromShooter =
                        Vec2_Distance(item->position, shooter->position);

                    if (distFromShooter >= IMMUNITY_MIN_DISTANCE) {
                        item->immunityTimer = 0;
                    }
                }
            } else if (item->immunityTimer == -1) {
                // SINGLE PLAYER MODE: Lap-based immunity

                // Check if projectile has completed a lap
                if (!item->hasCompletedLap && item->waypointsVisited > 0) {
                    // Calculate waypoint distance (accounting for wraparound)
                    int waypointDiff =
                        abs(item->currentWaypoint - item->startingWaypoint);

                    // Check if we're back near the starting waypoint after traveling
                    // far
                    if (waypointDiff <= WAYPOINT_LAP_THRESHOLD &&
                        item->waypointsVisited >
                            100) {  // Must have traveled significant distance

                        item->hasCompletedLap = true;
                        item->immunityTimer = 0;  // Remove immunity
                    }
                }
            }
        }

        // Update projectiles
        if (item->type == ITEM_GREEN_SHELL || item->type == ITEM_RED_SHELL ||
            item->type == ITEM_MISSILE) {
            updateProjectile(item, raceState->cars, raceState->carCount);
        }

        // Update homing behavior
        if (item->type == ITEM_RED_SHELL || item->type == ITEM_MISSILE) {
            updateHoming(item, raceState->cars, raceState->carCount);
        }
    }

    // Update item box respawn timers
    for (int i = 0; i < itemBoxCount; i++) {
        if (!itemBoxSpawns[i].active && itemBoxSpawns[i].respawnTimer > 0) {
            itemBoxSpawns[i].respawnTimer--;
            if (itemBoxSpawns[i].respawnTimer <= 0) {
                itemBoxSpawns[i].active = true;
            }
        }
    }
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

static void updateProjectile(TrackItem* item, const Car* cars, int carCount) {
    (void)cars;
    (void)carCount;

    // Move projectile
    Vec2 velocity = Vec2_FromAngle(item->angle512);
    velocity = Vec2_Scale(velocity, item->speed);
    item->position = Vec2_Add(item->position, velocity);

    // Check wall collision
    int x = FixedToInt(item->position.x);
    int y = FixedToInt(item->position.y);
    QuadrantID quad = getQuadrantFromPos(item->position);

    if (Wall_CheckCollision(x, y, item->hitbox_width / 2, quad)) {
        item->active = false;  // Despawn on wall hit
    }
}

static void updateHoming(TrackItem* item, const Car* cars, int carCount) {
    const RaceState* state = Race_GetState();
    bool isMultiplayer = (state->gameMode == MultiPlayer);

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

            Q16_8 dist = Vec2_Distance(item->position, cars[i].position);
            if (dist <= lockOnRadius) {
                // Lock onto this car!
                item->targetCarIndex = i;
                item->usePathFollowing = false;  // Switch to direct attack
                break;
            }
        }
    }

    Vec2 targetPoint;

    // If we have a locked target, check if we should stay locked
    if (item->targetCarIndex >= 0 && item->targetCarIndex < carCount) {
        if (isMultiplayer && item->targetCarIndex == item->shooterCarIndex) {
            item->targetCarIndex = INVALID_CAR_INDEX;
            item->usePathFollowing = true;
        } else {
            const Car* target = &cars[item->targetCarIndex];
            Q16_8 distToTarget = Vec2_Distance(item->position, target->position);

            // If target is too far away, unlock and return to path following
            if (distToTarget > IntToFixed(150)) {  // 150 pixel leash
                item->targetCarIndex = INVALID_CAR_INDEX;
                item->usePathFollowing = true;
            } else {
                // Stay locked - aim directly at target
                targetPoint = target->position;
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
        if (ItemNav_IsWaypointReached(item->position, waypointPos)) {
            // Advance to next waypoint
            item->currentWaypoint =
                ItemNav_GetNextWaypoint(item->currentWaypoint, state->currentMap);
            item->waypointsVisited++;
        }

        // Aim toward current waypoint
        targetPoint = waypointPos;
    }

    // Smooth turn toward target point
    Vec2 toTarget = Vec2_Sub(targetPoint, item->position);
    int targetAngle = Vec2_ToAngle(toTarget);

    int angleDiff = (targetAngle - item->angle512) & ANGLE_MASK;
    if (angleDiff > ANGLE_HALF)
        angleDiff -= ANGLE_FULL;

    if (angleDiff > HOMING_TURN_RATE)
        angleDiff = HOMING_TURN_RATE;
    if (angleDiff < -HOMING_TURN_RATE)
        angleDiff = -HOMING_TURN_RATE;

    item->angle512 = (item->angle512 + angleDiff) & ANGLE_MASK;
}

static void checkProjectileCollision(TrackItem* item, Car* cars, int carCount) {
    // Get race state to check if we're in multiplayer mode
    const RaceState* state = Race_GetState();
    bool isMultiplayer = (state->gameMode == MultiPlayer);

    for (int i = 0; i < carCount; i++) {
        // In multiplayer, only check collision for connected players
        if (isMultiplayer && !Multiplayer_IsPlayerConnected(i)) {
            continue;
        }

        // In multiplayer, never collide with the shooter
        if (isMultiplayer && i == item->shooterCarIndex) {
            continue;
        }

        // immunityTimer > 0: multiplayer time-based immunity
        // immunityTimer == -1 AND !hasCompletedLap: single player lap-based immunity
        bool hasImmunity = (item->immunityTimer > 0) ||
                           (item->immunityTimer == -1 && !item->hasCompletedLap);

        // In single player, keep the old immunity-based shooter skip
        if (!isMultiplayer && hasImmunity && i == item->shooterCarIndex) {
            continue;  // Can't hit the shooter yet
        }

        if (checkItemCarCollision(item->position, cars[i].position,
                                  item->hitbox_width)) {
            // Apply effect based on projectile type
            if (item->type == ITEM_GREEN_SHELL || item->type == ITEM_RED_SHELL) {
                applyShellHitEffect(&cars[i]);
            } else if (item->type == ITEM_MISSILE) {
                cars[i].speed = 0;
            }

            item->active = false;  // Despawn projectile
            break;
        }
    }
}

static void checkHazardCollision(TrackItem* item, Car* cars, int carCount) {
    // Get race state to check if we're in multiplayer mode
    const RaceState* state = Race_GetState();
    bool isMultiplayer = (state->gameMode == MultiPlayer);

    for (int i = 0; i < carCount; i++) {
        // In multiplayer, only check collision for connected players
        if (isMultiplayer && !Multiplayer_IsPlayerConnected(i)) {
            continue;
        }

        if (checkItemCarCollision(item->position, cars[i].position,
                                  item->hitbox_width)) {
            // Apply hazard effect based on item type
            switch (item->type) {
                case ITEM_BANANA:
                    applyBananaHitEffect(&cars[i]);
                    item->active = false;
                    break;

                case ITEM_OIL:
                    applyOilHitEffect(&cars[i], i);
                    // Oil persists
                    break;

                case ITEM_BOMB:
                    explodeBomb(item->position, cars, carCount);
                    item->active = false;
                    break;

                default:
                    break;
            }

            if (!item->active)
                break;
        }
    }

    // Bomb auto-explodes when timer runs out
    if (item->type == ITEM_BOMB && item->lifetime_ticks <= 0) {
        explodeBomb(item->position, cars, carCount);
        item->active = false;
    }
}

static void explodeBomb(Vec2 position, Car* cars, int carCount) {
    // Get race state to check if we're in multiplayer mode
    const RaceState* state = Race_GetState();
    bool isMultiplayer = (state->gameMode == MultiPlayer);

    for (int i = 0; i < carCount; i++) {
        // In multiplayer, only check collision for connected players
        if (isMultiplayer && !Multiplayer_IsPlayerConnected(i)) {
            continue;
        }

        Q16_8 dist = Vec2_Distance(position, cars[i].position);

        if (dist <= BOMB_EXPLOSION_RADIUS) {
            // Stop car completely
            cars[i].speed = 0;

            // Knockback away from bomb
            Vec2 knockbackDir = Vec2_Sub(cars[i].position, position);
            if (!Vec2_IsZero(knockbackDir)) {
                knockbackDir = Vec2_Normalize(knockbackDir);
                Vec2 knockback =
                    Vec2_Scale(knockbackDir, IntToFixed(BOMB_KNOCKBACK_DISTANCE));
                cars[i].position = Vec2_Add(cars[i].position, knockback);
            }
        }
    }
}

static bool checkItemBoxPickup(const Car* car, ItemBoxSpawn* box) {
    Q16_8 dist = Vec2_Distance(car->position, box->position);
    int pickupRadius = (CAR_RADIUS + ITEM_BOX_HITBOX);
    return (dist <= IntToFixed(pickupRadius));
}

static bool checkItemCarCollision(Vec2 itemPos, Vec2 carPos, int itemHitbox) {
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

        if (item->type == ITEM_GREEN_SHELL || item->type == ITEM_RED_SHELL ||
            item->type == ITEM_MISSILE) {
            // Only check collision if item is near the screen
            if (isItemNearScreen(item->position, scrollX, scrollY)) {
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

        if (item->type == ITEM_BANANA || item->type == ITEM_OIL ||
            item->type == ITEM_BOMB) {
            // Only check collision if item is near the screen
            if (isItemNearScreen(item->position, scrollX, scrollY)) {
                checkHazardCollision(item, cars, carCount);
            }
        }
    }
}

static bool isItemNearScreen(Vec2 itemPos, int scrollX, int scrollY) {
    int itemX = FixedToInt(itemPos.x);
    int itemY = FixedToInt(itemPos.y);

    int screenLeft = scrollX - COLLISION_BUFFER_ZONE;
    int screenRight = scrollX + SCREEN_WIDTH + COLLISION_BUFFER_ZONE;
    int screenTop = scrollY - COLLISION_BUFFER_ZONE;
    int screenBottom = scrollY + SCREEN_HEIGHT + COLLISION_BUFFER_ZONE;

    return (itemX >= screenLeft && itemX <= screenRight && itemY >= screenTop &&
            itemY <= screenBottom);
}

static QuadrantID getQuadrantFromPos(Vec2 pos) {
    int x = FixedToInt(pos.x);
    int y = FixedToInt(pos.y);

    int col = (x < QUAD_BOUNDARY_LOW) ? 0 : (x < QUAD_BOUNDARY_HIGH) ? 1 : 2;
    int row = (y < QUAD_BOUNDARY_LOW) ? 0 : (y < QUAD_BOUNDARY_HIGH) ? 1 : 2;

    return (QuadrantID)(row * QUADRANT_GRID_SIZE + col);
}
