#include "Items.h"

#include <stdlib.h>
#include <string.h>

#include "gameplay_logic.h"
#include "wall_collision.h"

// Sprite graphics includes
#include <stdio.h>

#include "Car.h"
#include "banana.h"
#include "bomb.h"
#include "green_shell.h"
#include "item_box.h"
#include "missile.h"
#include "oil_slick.h"
#include "red_shell.h"
#include "sound.h"

//=============================================================================
// Module State
//=============================================================================

static TrackItem activeItems[MAX_TRACK_ITEMS];
static ItemBoxSpawn itemBoxSpawns[MAX_ITEM_BOX_SPAWNS];
static int itemBoxCount = 0;
static PlayerItemEffects playerEffects;

// Sprite graphics pointers (allocated during Items_LoadGraphics)
static u16* itemBoxGfx = NULL;
static u16* bananaGfx = NULL;
static u16* bombGfx = NULL;
static u16* greenShellGfx = NULL;
static u16* redShellGfx = NULL;
static u16* missileGfx = NULL;
static u16* oilSlickGfx = NULL;

//=============================================================================
// Private Prototypes
//=============================================================================

static void initItemBoxSpawns(Map map);
static void clearActiveItems(void);
static void updateProjectile(TrackItem* item, const Car* cars, int carCount);
static void updateHoming(TrackItem* item, const Car* cars, int carCount);
static void checkProjectileCollision(TrackItem* item, Car* cars, int carCount);
static void checkHazardCollision(TrackItem* item, Car* cars, int carCount);
static void explodeBomb(Vec2 position, Car* cars, int carCount);
static bool checkItemBoxPickup(const Car* car, ItemBoxSpawn* box);
static int findInactiveItemSlot(void);
static int findCarAhead(int currentRank, int carCount);
static QuadrantID getQuadrantFromPos(Vec2 pos);

//=============================================================================
// Lifecycle
//=============================================================================

void Items_Init(Map map) {
    clearActiveItems();
    initItemBoxSpawns(map);

    // Initialize player effects
    memset(&playerEffects, 0, sizeof(PlayerItemEffects));

    // Graphics are loaded once in configureSprite()
}

void Items_Reset(void) {
    clearActiveItems();

    // Reset all item boxes to active
    for (int i = 0; i < itemBoxCount; i++) {
        itemBoxSpawns[i].active = true;
        itemBoxSpawns[i].respawnTimer = 0;
    }

    // Reset player effects
    memset(&playerEffects, 0, sizeof(PlayerItemEffects));
}

void Items_Update(void) {
    const RaceState* raceState = Race_GetState();

    // Update active track items
    for (int i = 0; i < MAX_TRACK_ITEMS; i++) {
        if (!activeItems[i].active)
            continue;

        TrackItem* item = &activeItems[i];

        // Update lifetime
        item->lifetime_ticks--;
        if (item->lifetime_ticks <= 0) {
            item->active = false;
            continue;
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

void Items_CheckCollisions(Car* cars, int carCount) {
    // DEBUG: Track if we're checking item boxes
    static bool debugItemBoxes = true;

    for (int i = 0; i < itemBoxCount; i++) {
        if (!itemBoxSpawns[i].active)
            continue;

        for (int c = 0; c < carCount; c++) {
            // DEBUG: Print distance to item box for player (car 0)
            if (c == 0 && debugItemBoxes) {
                Q16_8 dist =
                    Vec2_Distance(cars[c].position, itemBoxSpawns[i].position);
                int distInt = FixedToInt(dist);

                // Only print for closest box (avoid spam)
                if (distInt < 50) {
                    printf("Box %d: dist=%d (threshold=14)\n", i, distInt);
                    printf("  Car: %d,%d\n", FixedToInt(cars[c].position.x),
                           FixedToInt(cars[c].position.y));
                    printf("  Box: %d,%d\n", FixedToInt(itemBoxSpawns[i].position.x),
                           FixedToInt(itemBoxSpawns[i].position.y));
                }
            }

            if (checkItemBoxPickup(&cars[c], &itemBoxSpawns[i])) {
                // DEBUG: Announce pickup
                if (c == 0) {
                    printf("*** ITEM BOX PICKUP! ***\n");
                }

                // PLAY SOUND - no throttling, every pickup should ding!
                PlayBoxSFX();

                // Give player random item
                if (c == 0) {  // Only player car (index 0)
                    if (cars[c].item == ITEM_NONE) {
                        Item receivedItem = Items_GetRandomItem(cars[c].rank);
                        cars[c].item = receivedItem;

                        // DEBUG: Print which item was received
                        printf("  Received item: %d\n", receivedItem);
                    }
                }
                // Deactivate box and start respawn timer
                itemBoxSpawns[i].active = false;
                itemBoxSpawns[i].respawnTimer = ITEM_BOX_RESPAWN_TICKS;
                break;
            }
        }
    }

    for (int i = 0; i < MAX_TRACK_ITEMS; i++) {
        if (!activeItems[i].active)
            continue;

        TrackItem* item = &activeItems[i];

        if (item->type == ITEM_GREEN_SHELL || item->type == ITEM_RED_SHELL ||
            item->type == ITEM_MISSILE) {
            checkProjectileCollision(item, cars, carCount);
        }
    }

    for (int i = 0; i < MAX_TRACK_ITEMS; i++) {
        if (!activeItems[i].active)
            continue;

        TrackItem* item = &activeItems[i];

        if (item->type == ITEM_BANANA || item->type == ITEM_OIL ||
            item->type == ITEM_BOMB) {
            checkHazardCollision(item, cars, carCount);
        }
    }
}

void Items_UsePlayerItem(Car* player, bool fireForward) {
    if (player->item == ITEM_NONE)
        return;

    Item itemType = player->item;
    player->item = ITEM_NONE;  // Clear inventory

    switch (itemType) {
        case ITEM_BANANA:
        case ITEM_BOMB:
        case ITEM_OIL: {
            // Calculate backward direction (180° from facing direction)
            int backwardAngle = (player->angle512 + ANGLE_HALF) & ANGLE_MASK;  // +180°
            Vec2 backward = Vec2_FromAngle(backwardAngle);
            Vec2 offset = Vec2_Scale(backward, IntToFixed(40));  // 40 pixels back
            Vec2 dropPos = Vec2_Add(player->position, offset);
            Items_PlaceHazard(itemType, dropPos);
            break;
        }

        case ITEM_GREEN_SHELL: {
            // Fire in specified direction
            int fireAngle = fireForward
                                ? player->angle512
                                : ((player->angle512 + ANGLE_HALF) & ANGLE_MASK);
            Q16_8 shellSpeed = FixedMul(player->maxSpeed, GREEN_SHELL_SPEED_MULT);
            Items_FireProjectile(ITEM_GREEN_SHELL, player->position, fireAngle,
                                 shellSpeed, -1);
            break;
        }

        case ITEM_RED_SHELL: {
            // Fire at car ahead
            int targetIndex = findCarAhead(player->rank, MAX_CARS);
            int fireAngle = fireForward
                                ? player->angle512
                                : ((player->angle512 + ANGLE_HALF) & ANGLE_MASK);
            Q16_8 shellSpeed = FixedMul(player->maxSpeed, RED_SHELL_SPEED_MULT);
            Items_FireProjectile(ITEM_RED_SHELL, player->position, fireAngle,
                                 shellSpeed, targetIndex);
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

    const ItemProbability* prob = &ITEM_PROBABILITIES[rankIndex];

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
// Item Spawning
//=============================================================================

void Items_FireProjectile(Item type, Vec2 pos, int angle512, Q16_8 speed,
                          int targetCarIndex) {
    int slot = findInactiveItemSlot();
    if (slot < 0)
        return;  // No free slots

    TrackItem* item = &activeItems[slot];
    item->type = type;
    item->position = pos;
    item->speed = speed;
    item->angle512 = angle512;
    item->targetCarIndex = targetCarIndex;
    item->active = true;
    item->lifetime_ticks = 10 * RACE_TICK_FREQ;  // 10 seconds max

    // Set hitbox and graphics based on type
    if (type == ITEM_MISSILE) {
        item->hitbox_width = MISSILE_HITBOX_W;
        item->hitbox_height = MISSILE_HITBOX_H;
        item->gfx = missileGfx;
    } else {
        item->hitbox_width = SHELL_HITBOX;
        item->hitbox_height = SHELL_HITBOX;
        item->gfx = (type == ITEM_GREEN_SHELL) ? greenShellGfx : redShellGfx;
    }
}

void Items_PlaceHazard(Item type, Vec2 pos) {
    int slot = findInactiveItemSlot();
    if (slot < 0)
        return;

    TrackItem* item = &activeItems[slot];
    item->type = type;
    item->position = pos;
    item->startPosition = pos;
    item->speed = 0;
    item->angle512 = 0;
    item->active = true;

    // DEBUG: Print where item was placed
    printf("Placed %d at %d,%d\n", type, FixedToInt(pos.x), FixedToInt(pos.y));

    // Set lifetime and graphics
    switch (type) {
        case ITEM_OIL:
            item->lifetime_ticks = OIL_LIFETIME_TICKS;
            item->hitbox_width = OIL_SLICK_HITBOX;
            item->hitbox_height = OIL_SLICK_HITBOX;
            item->gfx = oilSlickGfx;
            break;

        case ITEM_BOMB:
            item->lifetime_ticks = 5 * RACE_TICK_FREQ;  // 5 seconds until explosion
            item->hitbox_width = BOMB_HITBOX;
            item->hitbox_height = BOMB_HITBOX;
            item->gfx = bombGfx;
            break;

        case ITEM_BANANA:
            item->lifetime_ticks = 15 * RACE_TICK_FREQ;  // 15 seconds
            item->hitbox_width = BANANA_HITBOX;
            item->hitbox_height = BANANA_HITBOX;
            item->gfx = bananaGfx;
            break;

        default:
            item->active = false;
            break;
    }
}

//=============================================================================
// Player Effects
//=============================================================================

void Items_UpdatePlayerEffects(Car* player, PlayerItemEffects* effects) {
    // Update confusion timer
    if (effects->confusionActive) {
        effects->confusionTimer--;
        if (effects->confusionTimer <= 0) {
            effects->confusionActive = false;
        }
    }

    // Update speed boost timer
    if (effects->speedBoostActive) {
        effects->speedBoostTimer--;
        if (effects->speedBoostTimer <= 0) {
            // Restore original max speed
            player->maxSpeed = effects->originalMaxSpeed;
            effects->speedBoostActive = false;
        }
    }

    // Update oil slow effect (distance-based)
    if (effects->oilSlowActive) {
        Q16_8 distTraveled = Vec2_Distance(player->position, effects->oilSlowStart);
        if (distTraveled >= OIL_SLOW_DISTANCE) {
            effects->oilSlowActive = false;
            // Restore normal friction/accel (if modified)
        }
    }
}

PlayerItemEffects* Items_GetPlayerEffects(void) {
    return &playerEffects;
}

void Items_ApplyConfusion(PlayerItemEffects* effects) {
    effects->confusionActive = true;
    effects->confusionTimer = MUSHROOM_CONFUSION_DURATION;
}

void Items_ApplySpeedBoost(Car* player, PlayerItemEffects* effects) {
    if (!effects->speedBoostActive) {
        effects->originalMaxSpeed = player->maxSpeed;
    }
    player->maxSpeed = FixedMul(effects->originalMaxSpeed, SPEED_BOOST_MULT);
    effects->speedBoostActive = true;
    effects->speedBoostTimer = SPEED_BOOST_DURATION;
}

void Items_ApplyOilSlow(Car* player, PlayerItemEffects* effects) {
    // Instant speed reduction
    player->speed = player->speed / 2;

    // Mark oil slow as active for distance tracking
    effects->oilSlowActive = true;
    effects->oilSlowStart = player->position;
}

//=============================================================================
// Rendering
//=============================================================================
void Items_Render(int scrollX, int scrollY) {
    // =========================================================================
    // ITEM BOXES
    // =========================================================================
    for (int i = 0; i < itemBoxCount; i++) {
        int oamSlot = ITEM_BOX_OAM_START + i;

        if (!itemBoxSpawns[i].active) {
            // Hide inactive item boxes
            oamSet(&oamMain, oamSlot, 0, 192, 0, 1, SpriteSize_8x8,
                   SpriteColorFormat_16Color, itemBoxSpawns[i].gfx, -1, true, false,
                   false, false, false);
            continue;
        }

        // Calculate screen position (centered on hitbox)
        int screenX =
            FixedToInt(itemBoxSpawns[i].position.x) - scrollX - (ITEM_BOX_HITBOX / 2);
        int screenY =
            FixedToInt(itemBoxSpawns[i].position.y) - scrollY - (ITEM_BOX_HITBOX / 2);

        // Render if on-screen, otherwise hide
        if (screenX >= -16 && screenX < 256 && screenY >= -16 && screenY < 192) {
            oamSet(&oamMain, oamSlot, screenX, screenY, 0, 1, SpriteSize_8x8,
                   SpriteColorFormat_16Color, itemBoxSpawns[i].gfx, -1, false, false,
                   false, false, false);
        } else {
            // Hide offscreen boxes
            oamSet(&oamMain, oamSlot, 0, 192, 0, 1, SpriteSize_8x8,
                   SpriteColorFormat_16Color, itemBoxSpawns[i].gfx, -1, true, false,
                   false, false, false);
        }
    }

    // =========================================================================
    // TRACK ITEMS (Bananas, Shells, etc.)
    // =========================================================================

    // STEP 1: Clear/hide all track item OAM slots
    for (int i = 0; i < MAX_TRACK_ITEMS; i++) {
        int oamSlot = TRACK_ITEM_OAM_START + i;
        oamSet(&oamMain, oamSlot, 0, 192, 0, 0, SpriteSize_16x16,
               SpriteColorFormat_16Color, NULL, -1, true, false, false, false, false);
    }

    for (int i = 0; i < MAX_TRACK_ITEMS; i++) {
        if (!activeItems[i].active)
            continue;

        TrackItem* item = &activeItems[i];
        int oamSlot = TRACK_ITEM_OAM_START + i;  // STABLE MAPPING

        // DEBUG: Print item position each frame
        if (item->type == ITEM_BANANA) {
            printf("Banana pos: %d,%d\n", FixedToInt(item->position.x),
                   FixedToInt(item->position.y));
        }

        // Calculate screen position
        int screenX =
            FixedToInt(item->position.x) - scrollX - (item->hitbox_width / 2);
        int screenY =
            FixedToInt(item->position.y) - scrollY - (item->hitbox_height / 2);

        // Skip if offscreen (slot already hidden in step 1)
        if (screenX < -32 || screenX > 256 || screenY < -32 || screenY > 192)
            continue;

        // Determine sprite size and palette based on item type
        SpriteSize spriteSize;
        int paletteNum;

        switch (item->type) {
            case ITEM_MISSILE:
                spriteSize = SpriteSize_16x32;
                paletteNum = 6;
                break;
            case ITEM_OIL:
                spriteSize = SpriteSize_32x32;
                paletteNum = 7;
                break;
            case ITEM_BANANA:
                spriteSize = SpriteSize_16x16;
                paletteNum = 2;
                break;
            case ITEM_BOMB:
                spriteSize = SpriteSize_16x16;
                paletteNum = 3;
                break;
            case ITEM_GREEN_SHELL:
                spriteSize = SpriteSize_16x16;
                paletteNum = 4;
                break;
            case ITEM_RED_SHELL:
                spriteSize = SpriteSize_16x16;
                paletteNum = 5;
                break;
            default:
                spriteSize = SpriteSize_16x16;
                paletteNum = 0;
                break;
        }

        // Determine if sprite needs rotation (projectiles)
        bool useRotation =
            (item->type == ITEM_GREEN_SHELL || item->type == ITEM_RED_SHELL ||
             item->type == ITEM_MISSILE);

        if (useRotation) {
            // Use separate affine matrix slots for each rotating item
            // DS has 32 affine matrices, we'll use slots 1-4 rotating
            int affineSlot = 1 + (i % 4);
            int rotation = -(item->angle512 << 6);  // Convert to DS angle

            oamRotateScale(&oamMain, affineSlot, rotation, (1 << 8), (1 << 8));
            oamSet(&oamMain, oamSlot, screenX, screenY, 0, paletteNum, spriteSize,
                   SpriteColorFormat_16Color, item->gfx, affineSlot, false, false,
                   false, false, false);
        } else {
            // No rotation - static sprites
            oamSet(&oamMain, oamSlot, screenX, screenY, 0, paletteNum, spriteSize,
                   SpriteColorFormat_16Color, item->gfx, -1, false, false, false,
                   false, false);
        }
    }
}

void Items_LoadGraphics(void) {
    // Allocate sprite graphics - NOTE: 16-color format!
    itemBoxGfx = oamAllocateGfx(&oamMain, SpriteSize_8x8, SpriteColorFormat_16Color);
    bananaGfx = oamAllocateGfx(&oamMain, SpriteSize_16x16, SpriteColorFormat_16Color);
    bombGfx = oamAllocateGfx(&oamMain, SpriteSize_16x16, SpriteColorFormat_16Color);
    greenShellGfx =
        oamAllocateGfx(&oamMain, SpriteSize_16x16, SpriteColorFormat_16Color);
    redShellGfx =
        oamAllocateGfx(&oamMain, SpriteSize_16x16, SpriteColorFormat_16Color);
    missileGfx = oamAllocateGfx(&oamMain, SpriteSize_16x32, SpriteColorFormat_16Color);
    oilSlickGfx =
        oamAllocateGfx(&oamMain, SpriteSize_32x32, SpriteColorFormat_16Color);

    // Copy tile data (note: for 16-color, still divide by 2)
    dmaCopy(item_boxTiles, itemBoxGfx, item_boxTilesLen);
    dmaCopy(bananaTiles, bananaGfx, bananaTilesLen);
    dmaCopy(bombTiles, bombGfx, bombTilesLen);
    dmaCopy(green_shellTiles, greenShellGfx, green_shellTilesLen);
    dmaCopy(red_shellTiles, redShellGfx, red_shellTilesLen);
    dmaCopy(missileTiles, missileGfx, missileTilesLen);
    dmaCopy(oil_slickTiles, oilSlickGfx, oil_slickTilesLen);

    // Copy palettes to separate palette slots (like the example)
    // Start at palette slot 1 (slot 0 is for the kart)
    dmaCopy(item_boxPal, &SPRITE_PALETTE[16], item_boxPalLen);
    dmaCopy(bananaPal, &SPRITE_PALETTE[32], bananaPalLen);
    dmaCopy(bombPal, &SPRITE_PALETTE[48], bombPalLen);
    dmaCopy(green_shellPal, &SPRITE_PALETTE[64], green_shellPalLen);
    dmaCopy(red_shellPal, &SPRITE_PALETTE[80], red_shellPalLen);
    dmaCopy(missilePal, &SPRITE_PALETTE[96], missilePalLen);
    dmaCopy(oil_slickPal, &SPRITE_PALETTE[112], oil_slickPalLen);
}

//=============================================================================
// Debug API
//=============================================================================

const ItemBoxSpawn* Items_GetBoxSpawns(int* count) {
    *count = itemBoxCount;
    return itemBoxSpawns;
}

const TrackItem* Items_GetActiveItems(int* count) {
    int activeCount = 0;
    for (int i = 0; i < MAX_TRACK_ITEMS; i++) {
        if (activeItems[i].active)
            activeCount++;
    }
    *count = activeCount;
    return activeItems;
}

//=============================================================================
// Private Implementation
//=============================================================================

static void initItemBoxSpawns(Map map) {
    // Hardcoded spawn locations for Scorching Sands
    // TODO: Add spawn locations for other maps

    if (map != ScorchingSands) {
        itemBoxCount = 0;
        return;
    }

    const Vec2 spawnLocations[] = {Vec2_FromInt(908, 469), Vec2_FromInt(967, 466),
                                   Vec2_FromInt(474, 211), Vec2_FromInt(493, 167),
                                   Vec2_FromInt(47, 483),  Vec2_FromInt(117, 483)};

    itemBoxCount = 6;  // can add more

    for (int i = 0; i < itemBoxCount; i++) {
        itemBoxSpawns[i].position = spawnLocations[i];
        itemBoxSpawns[i].active = true;
        itemBoxSpawns[i].respawnTimer = 0;
        itemBoxSpawns[i].gfx = itemBoxGfx;
    }
}

static void clearActiveItems(void) {
    for (int i = 0; i < MAX_TRACK_ITEMS; i++) {
        activeItems[i].active = false;
    }
}

static void updateProjectile(TrackItem* item, const Car* cars, int carCount) {
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
    if (item->targetCarIndex < 0 || item->targetCarIndex >= carCount)
        return;

    const Car* target = &cars[item->targetCarIndex];

    // Calculate vector to target
    Vec2 toTarget = Vec2_Sub(target->position, item->position);

    // Get angle to target
    int targetAngle = Vec2_ToAngle(toTarget);

    // Smoothly rotate toward target (max 5 degrees per frame)
    int angleDiff = (targetAngle - item->angle512) & ANGLE_MASK;
    if (angleDiff > ANGLE_HALF)
        angleDiff -= ANGLE_FULL;

    int turnRate = 5;  // Max turn per frame
    if (angleDiff > turnRate)
        angleDiff = turnRate;
    if (angleDiff < -turnRate)
        angleDiff = -turnRate;

    item->angle512 = (item->angle512 + angleDiff) & ANGLE_MASK;
}

static void checkProjectileCollision(TrackItem* item, Car* cars, int carCount) {
    for (int i = 0; i < carCount; i++) {
        Q16_8 dist = Vec2_Distance(item->position, cars[i].position);
        int hitRadius = (item->hitbox_width + 32) / 2;  // 32 = car size

        if (dist <= IntToFixed(hitRadius)) {
            // Hit car - apply effect based on projectile type
            if (item->type == ITEM_GREEN_SHELL || item->type == ITEM_RED_SHELL) {
                // Spin out - reduce speed significantly
                cars[i].speed = cars[i].speed / 3;
            } else if (item->type == ITEM_MISSILE) {
                // Missile hit - bigger impact
                cars[i].speed = 0;
            }

            item->active = false;  // Despawn projectile
            break;
        }
    }
}

static void checkHazardCollision(TrackItem* item, Car* cars, int carCount) {
    for (int i = 0; i < carCount; i++) {
        Q16_8 dist = Vec2_Distance(item->position, cars[i].position);
        int hitRadius = (item->hitbox_width + 32) / 2;

        if (dist <= IntToFixed(hitRadius)) {
            // Apply hazard effect
            switch (item->type) {
                case ITEM_BANANA:
                    cars[i].speed = cars[i].speed / 3;
                    item->active = false;  // Despawn on hit
                    break;

                case ITEM_OIL:
                    // Apply oil slow to player only
                    if (i == 0) {
                        Items_ApplyOilSlow(&cars[i], &playerEffects);
                    } else {
                        cars[i].speed = cars[i].speed / 2;
                    }
                    // Oil persists
                    break;

                case ITEM_BOMB:
                    // Explode on contact or timer expiration
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
    for (int i = 0; i < carCount; i++) {
        Q16_8 dist = Vec2_Distance(position, cars[i].position);

        if (dist <= BOMB_EXPLOSION_RADIUS) {
            // Calculate knockback direction (away from bomb)
            Vec2 knockbackDir = Vec2_Sub(cars[i].position, position);
            if (!Vec2_IsZero(knockbackDir)) {
                knockbackDir = Vec2_Normalize(knockbackDir);
                Vec2 impulse = Vec2_Scale(knockbackDir, BOMB_KNOCKBACK_IMPULSE);
                Car_ApplyImpulse(&cars[i], impulse);
            }
        }
    }
}

static bool checkItemBoxPickup(const Car* car, ItemBoxSpawn* box) {
    Q16_8 dist = Vec2_Distance(car->position, box->position);
    int pickupRadius = (CAR_RADIUS + ITEM_BOX_HITBOX);
    return (dist <= IntToFixed(pickupRadius));
}

static int findInactiveItemSlot(void) {
    for (int i = 0; i < MAX_TRACK_ITEMS; i++) {
        if (!activeItems[i].active) {
            return i;
        }
    }
    return -1;  // No free slots
}

static int findCarAhead(int currentRank, int carCount) {
    const RaceState* state = Race_GetState();

    // Find car with rank one better than current
    int targetRank = currentRank - 1;
    if (targetRank < 1)
        targetRank = 1;

    for (int i = 0; i < carCount; i++) {
        if (state->cars[i].rank == targetRank) {
            return i;
        }
    }

    return 0;  // Default to first car
}

static QuadrantID getQuadrantFromPos(Vec2 pos) {
    int x = FixedToInt(pos.x);
    int y = FixedToInt(pos.y);

    int col = (x < 256) ? 0 : (x < 512) ? 1 : 2;
    int row = (y < 256) ? 0 : (y < 512) ? 1 : 2;

    return (QuadrantID)(row * 3 + col);
}
