#ifndef STORAGE_PB_H
#define STORAGE_PB_H
// HUGO------
#include <stdbool.h>
#include "../core/game_types.h"  // For Map enum

#define BEST_TIMES_FILE "/kart-mania/best_times.txt"

// Initialize best times file if it doesn't exist
// Returns true on success
bool StoragePB_Init(void);

// Load best time for a specific map. Returns true if a time exists.
// If no time exists, min/sec/msec are unchanged and function returns false
bool StoragePB_LoadBestTime(Map map, int* min, int* sec, int* msec);

// Save best time for a specific map (only if it's better than existing)
// Returns true if the new time was a record (better than previous or first time)
// Returns false if the time was not better than existing record
bool StoragePB_SaveBestTime(Map map, int min, int sec, int msec);

#endif  // STORAGE_PB_H