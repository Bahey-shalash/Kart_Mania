/**
 * File: storage_pb.c
 * ------------------
 * Description: Implementation of personal best lap time storage. Reads and
 *              writes per-map racing records to SD card. Uses text file
 *              format (MapName=MM:SS.mmm). Implements time comparison and
 *              automatic record detection.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 06.01.2026
 */

#include "storage_pb.h"

#include <stdio.h>
#include <string.h>

//=============================================================================
// PRIVATE HELPER FUNCTIONS
//=============================================================================

/**
 * Converts map enum to string representation for file storage.
 *
 * Parameters:
 *   map - Map enum value
 *
 * Returns: String name of map, or "Unknown" for invalid values
 */
static const char* StoragePB_MapToString(Map map) {
    switch (map) {
        case ScorchingSands:
            return "ScorchingSands";
        case AlpinRush:
            return "AlpinRush";
        case NeonCircuit:
            return "NeonCircuit";
        default:
            return "Unknown";
    }
}

/**
 * Compares two lap times to determine which is faster.
 *
 * Compares minutes, then seconds, then milliseconds in order.
 *
 * Parameters:
 *   min1, sec1, msec1  - First time
 *   min2, sec2, msec2  - Second time
 *
 * Returns: true if first time is faster (less than) second time
 */
static bool StoragePB_IsTimeFaster(int min1, int sec1, int msec1, int min2, int sec2,
                                   int msec2) {
    if (min1 < min2)
        return true;
    if (min1 > min2)
        return false;
    if (sec1 < sec2)
        return true;
    if (sec1 > sec2)
        return false;
    return msec1 < msec2;
}

/**
 * Checks if a file exists on the filesystem.
 *
 * Parameters:
 *   path - File path to check
 *
 * Returns: true if file exists and is accessible, false otherwise
 */
static bool StoragePB_FileExists(const char* path) {
    FILE* file = fopen(path, "r");
    if (file) {
        fclose(file);
        return true;
    }
    return false;
}

//=============================================================================
// PUBLIC API
//=============================================================================

bool StoragePB_Init(void) {
    // Create best_times.txt if it doesn't exist (empty file is fine)
    if (!StoragePB_FileExists(BEST_TIMES_FILE)) {
        FILE* file = fopen(BEST_TIMES_FILE, "w+");
        if (file == NULL) {
            return false;
        }
        fclose(file);
    }
    return true;
}

bool StoragePB_LoadBestTime(Map map, int* min, int* sec, int* msec) {
    FILE* file = fopen(BEST_TIMES_FILE, "r");
    if (file == NULL) {
        return false;  // No best times file exists yet
    }

    const char* mapName = StoragePB_MapToString(map);
    char line[64];
    bool found = false;

    while (fgets(line, sizeof(line), file) != NULL) {
        char mapStr[32];
        int m, s, ms;
        // Format: MapName=MM:SS.mmm
        if (sscanf(line, "%[^=]=%d:%d.%d", mapStr, &m, &s, &ms) == 4) {
            if (strcmp(mapStr, mapName) == 0) {
                *min = m;
                *sec = s;
                *msec = ms;
                found = true;
                break;
            }
        }
    }

    fclose(file);
    return found;
}

bool StoragePB_SaveBestTime(Map map, int min, int sec, int msec) {
    // Check if there's an existing time
    int oldMin, oldSec, oldMsec;
    bool hadPreviousTime = StoragePB_LoadBestTime(map, &oldMin, &oldSec, &oldMsec);

    // If we had a previous time and the new time isn't faster, don't save
    if (hadPreviousTime) {
        if (!StoragePB_IsTimeFaster(min, sec, msec, oldMin, oldSec, oldMsec)) {
            return false;  // Not a new record
        }
    }

    // Read all existing times into memory
    char lines[10][64];  // Support up to 10 map records
    int lineCount = 0;
    bool updatedExisting = false;

    FILE* file = fopen(BEST_TIMES_FILE, "r");
    if (file != NULL) {
        char line[64];
        while (fgets(line, sizeof(line), file) != NULL && lineCount < 10) {
            char mapStr[32];
            if (sscanf(line, "%[^=]=", mapStr) == 1) {
                if (strcmp(mapStr, StoragePB_MapToString(map)) == 0) {
                    // Update this line with new time
                    snprintf(lines[lineCount], sizeof(lines[lineCount]),
                             "%s=%02d:%02d.%03d\n", StoragePB_MapToString(map), min,
                             sec, msec);
                    updatedExisting = true;
                } else {
                    // Keep existing line
                    snprintf(lines[lineCount], sizeof(lines[lineCount]), "%s", line);
                }
                lineCount++;
            }
        }
        fclose(file);
    }

    // If we didn't update an existing entry, add a new one
    if (!updatedExisting && lineCount < 10) {
        snprintf(lines[lineCount], sizeof(lines[lineCount]), "%s=%02d:%02d.%03d\n",
                 StoragePB_MapToString(map), min, sec, msec);
        lineCount++;
    }

    // Write all lines back to file
    file = fopen(BEST_TIMES_FILE, "w+");
    if (file == NULL) {
        return false;
    }

    for (int i = 0; i < lineCount; i++) {
        fputs(lines[i], file);
    }

    fclose(file);
    return true;  // New record!
}
