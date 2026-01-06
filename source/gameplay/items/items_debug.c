/**
 * File: items_debug.c
 * -------------------
 * Description: Debug and testing API for the items system. Provides access
 *              to internal state for debugging, visualization, and testing.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 06.01.2026
 */

#include "items_internal.h"
#include "items_api.h"

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
