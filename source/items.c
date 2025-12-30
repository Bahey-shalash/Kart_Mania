#include "Items.h"

#include <stdlib.h>
#include <string.h>

#include "game_constants.h"
#include "gameplay_logic.h"
#include "multiplayer.h"
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

// TODO: seperate this into multiple files
/*
item_spawning.c for creation/placement
item_physics.c for  projectile movement/homing
item_effects.c for player status effects
item_rendering.c for OAM updates
because the file is getting a bit out of hand
*/

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

// Helper functions for item effects
static void applyShellHitEffect(Car* car);
static void applyBananaHitEffect(Car* car);
static void applyOilHitEffect(Car* car, int carIndex);
static bool checkItemCarCollision(Vec2 itemPos, Vec2 carPos, int itemHitbox);
static void handleItemBoxPickup(Car* car, ItemBoxSpawn* box, int carIndex,
                                int boxIndex);
static void checkItemBoxCollisions(Car* cars, int carCount);
static void checkAllProjectileCollisions(Car* cars, int carCount, int scrollX,
                                         int scrollY);
static void checkAllHazardCollisions(Car* cars, int carCount, int scrollX,
                                     int scrollY);
static bool isItemNearScreen(Vec2 itemPos, int scrollX, int scrollY);

// Internal item placement functions (for network synchronization)
static void fireProjectileInternal(Item type, Vec2 pos, int angle512, Q16_8 speed,
                                   int targetCarIndex, bool sendNetwork);
static void placeHazardInternal(Item type, Vec2 pos, bool sendNetwork);

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

    // In multiplayer, receive item placements from other players
    if (raceState->gameMode == MultiPlayer) {
        ItemPlacementData itemData;
        while ((itemData = Multiplayer_ReceiveItemPlacements()).valid) {
            // Create the item locally that was placed by another player
            // Use internal functions with sendNetwork=false to avoid re-broadcasting
            if (itemData.speed > 0) {
                // It's a projectile
                fireProjectileInternal(itemData.itemType, itemData.position,
                                       itemData.angle512, itemData.speed,
                                       INVALID_CAR_INDEX, false);
            } else {
                // It's a hazard
                placeHazardInternal(itemData.itemType, itemData.position, false);
            }
        }

        // Receive item box pickups from other players
        int boxIndex;
        while ((boxIndex = Multiplayer_ReceiveItemBoxPickup()) >= 0) {
            // Deactivate the box that was picked up by another player
            Items_DeactivateBox(boxIndex);
        }
    }

    // Update active track items
    for (int i = 0; i < MAX_TRACK_ITEMS; i++) {
        if (!activeItems[i].active)
            continue;

        TrackItem* item = &activeItems[i];

        // Update lifetime (skip items with lifetime_ticks = -1, which means infinite)
        if (item->lifetime_ticks > 0) {
            item->lifetime_ticks--;
            if (item->lifetime_ticks <= 0) {
                item->active = false;
                continue;
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
            // Fire at car ahead
            int targetIndex = findCarAhead(player->rank, MAX_CARS);
            int fireAngle = fireForward
                                ? player->angle512
                                : ((player->angle512 + ANGLE_HALF) & ANGLE_MASK);

            // Spawn shell slightly ahead of player to avoid immediate collision
            Vec2 forward = Vec2_FromAngle(fireAngle);
            Vec2 offset = Vec2_Scale(forward, IntToFixed(PROJECTILE_SPAWN_OFFSET));
            Vec2 spawnPos = Vec2_Add(player->position, offset);

            Q16_8 shellSpeed = FixedMul(player->maxSpeed, RED_SHELL_SPEED_MULT);
            Items_FireProjectile(ITEM_RED_SHELL, spawnPos, fireAngle, shellSpeed,
                                 targetIndex);
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

    // Safety: prevent division by zero
    if (total == 0)
        total = 1;

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

static void fireProjectileInternal(Item type, Vec2 pos, int angle512, Q16_8 speed,
                                   int targetCarIndex, bool sendNetwork) {
    // In multiplayer, broadcast item placement to other players
    if (sendNetwork) {
        const RaceState* state = Race_GetState();
        if (state->gameMode == MultiPlayer) {
            Multiplayer_SendItemPlacement(type, pos, angle512, speed);
        }
    }

    int slot = findInactiveItemSlot();
    if (slot < 0) {
        // printf("WARNING: No free slots for projectile!\n");
        return;  // No free slots
    }

    TrackItem* item = &activeItems[slot];
    item->type = type;
    item->position = pos;
    item->speed = speed;
    item->angle512 = angle512;
    item->targetCarIndex = targetCarIndex;
    item->active = true;
    item->lifetime_ticks = PROJECTILE_LIFETIME_SECONDS * RACE_TICK_FREQ;

    // DEBUG: Print projectile firing info
    /*
    printf("Fired %s at %d,%d angle=%d speed=%d\n",
           type == ITEM_GREEN_SHELL ? "GREEN_SHELL"
           : type == ITEM_RED_SHELL ? "RED_SHELL"
                                    : "MISSILE",
           FixedToInt(pos.x), FixedToInt(pos.y), angle512, FixedToInt(speed));
    */

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

void Items_FireProjectile(Item type, Vec2 pos, int angle512, Q16_8 speed,
                          int targetCarIndex) {
    fireProjectileInternal(type, pos, angle512, speed, targetCarIndex, true);
}

static void placeHazardInternal(Item type, Vec2 pos, bool sendNetwork) {
    // In multiplayer, broadcast item placement to other players
    if (sendNetwork) {
        const RaceState* state = Race_GetState();
        if (state->gameMode == MultiPlayer) {
            Multiplayer_SendItemPlacement(type, pos, 0, 0);  // Hazards don't move
        }
    }

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

    // Set lifetime and graphics based on item type
    if (type == ITEM_BOMB) {
        item->lifetime_ticks = BOMB_LIFETIME_SECONDS * RACE_TICK_FREQ;
        item->hitbox_width = BOMB_HITBOX;
        item->hitbox_height = BOMB_HITBOX;
        item->gfx = bombGfx;
    } else if (type == ITEM_BANANA) {
        item->lifetime_ticks = ITEM_LIFETIME_INFINITE;
        item->hitbox_width = BANANA_HITBOX;
        item->hitbox_height = BANANA_HITBOX;
        item->gfx = bananaGfx;
    } else if (type == ITEM_OIL) {
        item->lifetime_ticks = OIL_LIFETIME_TICKS;
        item->hitbox_width = OIL_SLICK_HITBOX;
        item->hitbox_height = OIL_SLICK_HITBOX;
        item->gfx = oilSlickGfx;
    }
}

void Items_PlaceHazard(Item type, Vec2 pos) {
    placeHazardInternal(type, pos, true);
}

//=============================================================================
// OLD FUNCTION TO REMOVE
//=============================================================================
// This old function below should be deleted - it's been replaced by
// placeHazardInternal
/*
static void placeHazardOld_DELETE_ME(Item type, Vec2 pos) {
    // In multiplayer, broadcast item placement to other players
    const RaceState* state = Race_GetState();
    if (state->gameMode == MultiPlayer) {
        Multiplayer_SendItemPlacement(type, pos, 0, 0);  // Hazards don't move, so
angle/speed = 0
    }

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
    //printf("Placed %d at %d,%d\n", type, FixedToInt(pos.x), FixedToInt(pos.y));

    // Set lifetime and graphics
    switch (type) {
        case ITEM_OIL:
            item->lifetime_ticks = OIL_LIFETIME_TICKS;
            item->hitbox_width = OIL_SLICK_HITBOX;
            item->hitbox_height = OIL_SLICK_HITBOX;
            item->gfx = oilSlickGfx;
            break;

        case ITEM_BOMB:
            item->lifetime_ticks = BOMB_LIFETIME_SECONDS * RACE_TICK_FREQ;
            item->hitbox_width = BOMB_HITBOX;
            item->hitbox_height = BOMB_HITBOX;
            item->gfx = bombGfx;
            break;

        case ITEM_BANANA:
            item->lifetime_ticks = ITEM_LIFETIME_INFINITE;
            item->hitbox_width = BANANA_HITBOX;
            item->hitbox_height = BANANA_HITBOX;
            item->gfx = bananaGfx;
            break;

        default:
            item->active = false;
            break;
    }
}

*/
// End of old duplicate function that should be removed

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
            // Immediately cap current speed to prevent lingering boost
            if (player->speed > player->maxSpeed) {
                player->speed = player->maxSpeed;
            }
            effects->speedBoostActive = false;
        }
    }

    // Update oil slow effect (distance-based)
    if (effects->oilSlowActive) {
        Q16_8 distTraveled = Vec2_Distance(player->position, effects->oilSlowStart);
        if (distTraveled >= OIL_SLOW_DISTANCE) {
            effects->oilSlowActive = false;
            // Note: Friction/accel recovery is handled automatically by
            // applyTerrainEffects()
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
    player->speed = player->speed / OIL_SPEED_DIVISOR;

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
                   SpriteColorFormat_16Color, itemBoxSpawns[i].gfx, 1, true, false,
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
                   SpriteColorFormat_16Color, itemBoxSpawns[i].gfx, 1, false, false,
                   false, false, false);
        } else {
            // Hide offscreen boxes
            oamSet(&oamMain, oamSlot, 0, 192, 0, 1, SpriteSize_8x8,
                   SpriteColorFormat_16Color, itemBoxSpawns[i].gfx, 1, true, false,
                   false, false, false);
        }
    }

    // =========================================================================
    // TRACK ITEMS (Bananas, Shells, etc.)
    // =========================================================================

    // Clear/hide all track item OAM slots
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

        // DEBUG: Print item position each frame for projectiles
        if (item->type == ITEM_GREEN_SHELL || item->type == ITEM_RED_SHELL) {
            // printf("%s pos: %d,%d active=%d\n",
            //        item->type == ITEM_GREEN_SHELL ? "Green" : "Red",
            //        FixedToInt(item->position.x), FixedToInt(item->position.y),
            //        item->active);
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

    // CRITICAL FIX: Update item box spawns with the newly allocated graphics pointer
    // This is necessary because initItemBoxSpawns() is called BEFORE
    // Items_LoadGraphics() in the initialization sequence, so the spawns were
    // initially assigned NULL
    for (int i = 0; i < itemBoxCount; i++) {
        itemBoxSpawns[i].gfx = itemBoxGfx;
    }
}

void Items_FreeGraphics(void) {
    if (itemBoxGfx) {
        oamFreeGfx(&oamMain, itemBoxGfx);
        itemBoxGfx = NULL;
    }
    if (bananaGfx) {
        oamFreeGfx(&oamMain, bananaGfx);
        bananaGfx = NULL;
    }
    if (bombGfx) {
        oamFreeGfx(&oamMain, bombGfx);
        bombGfx = NULL;
    }
    if (greenShellGfx) {
        oamFreeGfx(&oamMain, greenShellGfx);
        greenShellGfx = NULL;
    }
    if (redShellGfx) {
        oamFreeGfx(&oamMain, redShellGfx);
        redShellGfx = NULL;
    }
    if (missileGfx) {
        oamFreeGfx(&oamMain, missileGfx);
        missileGfx = NULL;
    }
    if (oilSlickGfx) {
        oamFreeGfx(&oamMain, oilSlickGfx);
        oilSlickGfx = NULL;
    }
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
        // printf("Projectile hit wall at %d,%d - despawning\n", x, y);
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

    // Smoothly rotate toward target
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

static int findInactiveItemSlot(void) {
    for (int i = 0; i < MAX_TRACK_ITEMS; i++) {
        if (!activeItems[i].active) {
            return i;
        }
    }
    return INVALID_CAR_INDEX;
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

    int col = (x < QUAD_BOUNDARY_LOW) ? 0 : (x < QUAD_BOUNDARY_HIGH) ? 1 : 2;
    int row = (y < QUAD_BOUNDARY_LOW) ? 0 : (y < QUAD_BOUNDARY_HIGH) ? 1 : 2;

    return (QuadrantID)(row * QUADRANT_GRID_SIZE + col);
}

//=============================================================================
// Helper Functions for Item Effects
//=============================================================================

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

            // DEBUG: Print which item was received
            // printf("  Received item: %d\n", receivedItem);
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

            // DEBUG: Print distance to item box for player (car 0)
            if (c == 0 && debugItemBoxes) {
                Q16_8 dist =
                    Vec2_Distance(cars[c].position, itemBoxSpawns[i].position);
                int distInt = FixedToInt(dist);

                // Only print for closest box (avoid spam)
                /*
                if (distInt < ITEM_PICKUP_DEBUG_DISTANCE) {
                    printf("Box %d: dist=%d (threshold=%d)\n", i, distInt,
                           ITEM_PICKUP_THRESHOLD);
                    printf("  Car: %d,%d\n", FixedToInt(cars[c].position.x),
                           FixedToInt(cars[c].position.y));
                    printf("  Box: %d,%d\n", FixedToInt(itemBoxSpawns[i].position.x),
                           FixedToInt(itemBoxSpawns[i].position.y));
                }
                */
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

void Items_DeactivateBox(int boxIndex) {
    if (boxIndex < 0 || boxIndex >= itemBoxCount) {
        return;
    }

    itemBoxSpawns[boxIndex].active = false;
    itemBoxSpawns[boxIndex].respawnTimer = ITEM_BOX_RESPAWN_TICKS;
}
