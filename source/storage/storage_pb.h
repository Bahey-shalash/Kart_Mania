/**
 * storage_pb.h
 * Personal best race times management
 */

#ifndef STORAGE_PB_H
#define STORAGE_PB_H

#include <stdbool.h>

#include "../core/game_types.h"

//=============================================================================
// Constants
//=============================================================================

#define BEST_TIMES_FILE "/kart-mania/best_times.txt"

//=============================================================================
// Public API
//=============================================================================

/** Initialize best times file (creates if it doesn't exist) */
bool StoragePB_Init(void);

/** Load best time for a specific map (returns true if time exists) */
bool StoragePB_LoadBestTime(Map map, int* min, int* sec, int* msec);

/** Save best time for a map (only if faster than existing record) */
bool StoragePB_SaveBestTime(Map map, int min, int sec, int msec);

#endif  // STORAGE_PB_H