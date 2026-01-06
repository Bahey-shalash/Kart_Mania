#include "items_internal.h"
#include "items_api.h"

#include <string.h>

//=============================================================================
// Module State
//=============================================================================

TrackItem activeItems[MAX_TRACK_ITEMS];
ItemBoxSpawn itemBoxSpawns[MAX_ITEM_BOX_SPAWNS];
int itemBoxCount = 0;
PlayerItemEffects playerEffects;

// Sprite graphics pointers (allocated during Items_LoadGraphics)
u16* itemBoxGfx = NULL;
u16* bananaGfx = NULL;
u16* bombGfx = NULL;
u16* greenShellGfx = NULL;
u16* redShellGfx = NULL;
u16* missileGfx = NULL;
u16* oilSlickGfx = NULL;

static void initItemBoxSpawns(Map map);
static void clearActiveItems(void);

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

//=============================================================================
// Internal helpers
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

// Declared but unused in the current implementation
void Items_SpawnBoxes(void) {}
