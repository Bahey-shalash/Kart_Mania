#ifndef ITEMS_INTERNAL_H
#define ITEMS_INTERNAL_H

#include "items_types.h"
#include "items_constants.h"

// Shared module state (internal visibility only)
extern TrackItem activeItems[MAX_TRACK_ITEMS];
extern ItemBoxSpawn itemBoxSpawns[MAX_ITEM_BOX_SPAWNS];
extern int itemBoxCount;
extern PlayerItemEffects playerEffects;

extern u16* itemBoxGfx;
extern u16* bananaGfx;
extern u16* bombGfx;
extern u16* greenShellGfx;
extern u16* redShellGfx;
extern u16* missileGfx;
extern u16* oilSlickGfx;

// Internal helpers used across compilation units (not part of the public API)
void fireProjectileInternal(Item type, Vec2 pos, int angle512, Q16_8 speed,
                            int targetCarIndex, bool sendNetwork,
                            int shooterCarIndex);
void placeHazardInternal(Item type, Vec2 pos, bool sendNetwork);

#endif  // ITEMS_INTERNAL_H
