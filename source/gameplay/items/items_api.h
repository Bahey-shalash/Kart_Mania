#ifndef ITEMS_API_H
#define ITEMS_API_H

/*
 * Item behaviors (player inventory effects and track interactions):
 *
 * ITEM_NONE: Car has no item in inventory
 * ITEM_BOX: Gives a random item when hit (probabilities depend on race position).
 * ITEM_OIL: Dropped behind the car; slows cars that run over it; despawns after 10s.
 * ITEM_BOMB: Dropped behind the car; explodes after a delay; hits all cars in radius.
 * ITEM_BANANA: Dropped behind; slows and spins cars on hit; despawns on hit.
 * ITEM_GREEN_SHELL: Projectile fired in facing direction; despawns on wall/car hit.
 * ITEM_RED_SHELL: Homing projectile targeting the car ahead; despawns on collision.
 * ITEM_MISSILE: Targets 1st place directly; despawns on hit.
 * ITEM_MUSHROOM: Applies confusion (swapped controls) for several seconds.
 * ITEM_SPEEDBOOST: Temporary speed increase for several seconds.
 */

#include "items_types.h"
#include "items_constants.h"

// Forward declaration to avoid circular include with Car.h
typedef struct Car Car;

// Public API - Lifecycle
void Items_Init(Map map);
void Items_Reset(void);
void Items_Update(void);
void Items_CheckCollisions(Car* cars, int carCount, int scrollX, int scrollY);

// Public API - Item Spawning
void Items_FireProjectile(Item type, Vec2 pos, int angle512, Q16_8 speed,
                          int targetCarIndex);
void Items_PlaceHazard(Item type, Vec2 pos);

// Public API - Player Items and Effects
void Items_UsePlayerItem(Car* player, bool fireForward);
Item Items_GetRandomItem(int playerRank);
void Items_UpdatePlayerEffects(Car* player, PlayerItemEffects* effects);
PlayerItemEffects* Items_GetPlayerEffects(void);
void Items_ApplyConfusion(PlayerItemEffects* effects);
void Items_ApplySpeedBoost(Car* player, PlayerItemEffects* effects);
void Items_ApplyOilSlow(Car* player, PlayerItemEffects* effects);

// Public API - Rendering
void Items_Render(int scrollX, int scrollY);
void Items_LoadGraphics(void);
void Items_FreeGraphics(void);

// Debug/Testing API
const ItemBoxSpawn* Items_GetBoxSpawns(int* count);
const TrackItem* Items_GetActiveItems(int* count);
void Items_DeactivateBox(int boxIndex);

// Declared but unused in the current implementation
void Items_SpawnBoxes(void);

#endif  // ITEMS_API_H
