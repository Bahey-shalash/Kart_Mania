/**
 * File: items_spawning.c
 * ----------------------
 * Description: Item spawning logic for projectiles and hazards. Manages track
 *              item creation, initialization, network synchronization, and
 *              shooter immunity for multiplayer safety.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 06.01.2026
 */

#include "items_internal.h"
#include "items_api.h"
#include "item_navigation.h"

#include "../gameplay_logic.h"
#include "../../core/game_constants.h"
#include "../../network/multiplayer.h"

//=============================================================================
// Item Spawning
//=============================================================================

static int findInactiveItemSlot(void);

void fireProjectileInternal(Item type, Vec2 pos, int angle512, Q16_8 speed,
                            int targetCarIndex, bool sendNetwork,
                            int shooterCarIndex) {
    const RaceState* state = Race_GetState();

    // In multiplayer, broadcast item placement to other players
    if (sendNetwork && state->gameMode == MultiPlayer) {
        Multiplayer_SendItemPlacement(type, pos, angle512, speed, state->playerIndex);
    }

    int slot = findInactiveItemSlot();
    if (slot < 0) {
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

    int resolvedShooter = shooterCarIndex;
    if (resolvedShooter < 0 || resolvedShooter >= state->carCount) {
        resolvedShooter = -1;
    }

    // Initialize shooter immunity and navigation
    if (type == ITEM_RED_SHELL || type == ITEM_MISSILE) {
        item->shooterCarIndex = resolvedShooter;

        // Use lap-based immunity for both single player and multiplayer
        // This ensures shells complete ~1 lap before they can hit the shooter
        item->immunityTimer = -1;  // Special value: lap-based immunity

        // Initialize path following
        item->usePathFollowing = true;
        item->currentWaypoint = ItemNav_FindNearestWaypoint(pos, state->currentMap);
        item->waypointsVisited = 0;

        // Track starting waypoint for lap detection
        item->startingWaypoint = item->currentWaypoint;
        item->hasCompletedLap = false;

    } else {
        // Green shells have NO immunity
        item->shooterCarIndex = -1;
        item->immunityTimer = 0;
        item->usePathFollowing = false;
        item->currentWaypoint = 0;
        item->waypointsVisited = 0;
        item->startingWaypoint = -1;
        item->hasCompletedLap = false;
    }

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
    const RaceState* state = Race_GetState();
    fireProjectileInternal(type, pos, angle512, speed, targetCarIndex, true,
                           state->playerIndex);
}

void placeHazardInternal(Item type, Vec2 pos, bool sendNetwork) {
    // In multiplayer, broadcast item placement to other players
    if (sendNetwork) {
        const RaceState* state = Race_GetState();
        if (state->gameMode == MultiPlayer) {
            Multiplayer_SendItemPlacement(type, pos, 0, 0,
                                          state->playerIndex);  // Hazards don't move
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
        item->lifetime_ticks = BANANA_LIFETIME_SECONDS * RACE_TICK_FREQ;
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
// Internal helpers
//=============================================================================

static int findInactiveItemSlot(void) {
    for (int i = 0; i < MAX_TRACK_ITEMS; i++) {
        if (!activeItems[i].active) {
            return i;
        }
    }
    return INVALID_CAR_INDEX;
}
