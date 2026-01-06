/**
 * File: items_effects.c
 * ---------------------
 * Description: Player status effect management for the items system. Handles
 *              confusion (swapped controls), speed boosts, and oil slows with
 *              timer-based and distance-based duration tracking.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 06.01.2026
 */

#include "items_internal.h"
#include "items_api.h"

#include "../Car.h"
#include "../../core/game_constants.h"

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
