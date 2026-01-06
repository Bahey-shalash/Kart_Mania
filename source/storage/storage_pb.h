/**
 * File: storage_pb.h
 * ------------------
 * Description: Persistent storage for personal best lap times. Manages
 *              per-map racing records saved to SD card. Provides loading,
 *              saving, and automatic record detection (only saves if time
 *              is better than existing record).
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 06.01.2026
 */

#ifndef STORAGE_PB_H
#define STORAGE_PB_H

#include <stdbool.h>

#include "../core/game_types.h"

//=============================================================================
// PUBLIC CONSTANTS
//=============================================================================

// Best times file path on SD card
#define BEST_TIMES_FILE "/kart-mania/best_times.txt"

//=============================================================================
// PUBLIC API
//=============================================================================

/**
 * Function: StoragePB_Init
 * ------------------------
 * Initializes best times file if it doesn't exist.
 *
 * Creates an empty /kart-mania/best_times.txt file. File format:
 *   MapName=MM:SS.mmm
 *
 * Example:
 *   ScorchingSands=01:23.456
 *   AlpinRush=02:15.789
 *
 * Returns:
 *   true  - File initialized or already exists
 *   false - File creation failed (SD card error)
 */
bool StoragePB_Init(void);

/**
 * Function: StoragePB_LoadBestTime
 * ---------------------------------
 * Loads best lap time for a specific map from file.
 *
 * Parameters:
 *   map  - Map enum to load time for (ScorchingSands, AlpinRush, NeonCircuit)
 *   min  - Output: Minutes component of time
 *   sec  - Output: Seconds component of time (0-59)
 *   msec - Output: Milliseconds component of time (0-999)
 *
 * Returns:
 *   true  - Record found, output parameters populated
 *   false - No record exists for this map, output parameters unchanged
 */
bool StoragePB_LoadBestTime(Map map, int* min, int* sec, int* msec);

/**
 * Function: StoragePB_SaveBestTime
 * ---------------------------------
 * Saves best lap time for a specific map (only if it's a new record).
 *
 * Compares new time against existing record. Only writes to file if:
 *   1. No previous record exists, OR
 *   2. New time is faster than existing record
 *
 * Parameters:
 *   map  - Map enum to save time for
 *   min  - Minutes component of time
 *   sec  - Seconds component of time (0-59)
 *   msec - Milliseconds component of time (0-999)
 *
 * Returns:
 *   true  - New record! Time was saved (faster than previous or first time)
 *   false - Not a record, existing time is faster (nothing saved)
 */
bool StoragePB_SaveBestTime(Map map, int min, int sec, int msec);

#endif  // STORAGE_PB_H