#include "storage_pb.h"

#include <stdio.h>
#include <string.h>

#include "../core/game_constants.h"

//=============================================================================
// Private Helper Functions
//=============================================================================

/** Convert map enum to string representation */
static const char* mapToString(Map map) {
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

/** Compare two times (returns true if time1 is faster than time2) */
static bool isTimeFaster(int min1, int sec1, int msec1, int min2, int sec2, int msec2) {
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

/** Check if a file exists at the given path */
static bool fileExists(const char* path) {
    FILE* file = fopen(path, "r");
    if (file) {
        fclose(file);
        return true;
    }
    return false;
}

//=============================================================================
// Public API
//=============================================================================

/** Initialize personal best times storage (creates file if needed) */
bool StoragePB_Init(void) {
    if (!fileExists(BEST_TIMES_FILE)) {
        FILE* file = fopen(BEST_TIMES_FILE, "w+");
        if (file == NULL) {
            return false;
        }
        fclose(file);
    }
    return true;
}

/** Load personal best time for a specific map */
bool StoragePB_LoadBestTime(Map map, int* min, int* sec, int* msec) {
    FILE* file = fopen(BEST_TIMES_FILE, "r");
    if (file == NULL) {
        return false;
    }

    const char* mapName = mapToString(map);
    char line[64];
    bool found = false;

    while (fgets(line, sizeof(line), file) != NULL) {
        char mapStr[32];
        int m, s, ms;

        // Parse format: MapName=MM:SS.mmm
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

/** Save a new personal best time (only if faster than existing record) */
bool StoragePB_SaveBestTime(Map map, int min, int sec, int msec) {
    // Check if there's an existing time
    int oldMin, oldSec, oldMsec;
    bool hadPreviousTime = StoragePB_LoadBestTime(map, &oldMin, &oldSec, &oldMsec);

    // If we had a previous time and the new time isn't faster, don't save
    if (hadPreviousTime) {
        if (!isTimeFaster(min, sec, msec, oldMin, oldSec, oldMsec)) {
            return false;
        }
    }

    // Read all existing times into memory
    char lines[STORAGE_MAX_MAP_RECORDS][64];
    int lineCount = 0;
    bool updatedExisting = false;

    FILE* file = fopen(BEST_TIMES_FILE, "r");
    if (file != NULL) {
        char line[64];
        while (fgets(line, sizeof(line), file) != NULL && lineCount < STORAGE_MAX_MAP_RECORDS) {
            char mapStr[32];
            if (sscanf(line, "%[^=]=", mapStr) == 1) {
                if (strcmp(mapStr, mapToString(map)) == 0) {
                    // Update this line with new time
                    snprintf(lines[lineCount], sizeof(lines[lineCount]),
                             "%s=%02d:%02d.%03d\n", mapToString(map), min, sec, msec);
                    updatedExisting = true;
                } else {
                    // Keep existing line
                    strncpy(lines[lineCount], line, sizeof(lines[lineCount]) - 1);
                    lines[lineCount][sizeof(lines[lineCount]) - 1] = '\0';
                }
                lineCount++;
            }
        }
        fclose(file);
    }

    // If we didn't update an existing entry, add a new one
    if (!updatedExisting && lineCount < STORAGE_MAX_MAP_RECORDS) {
        snprintf(lines[lineCount], sizeof(lines[lineCount]), "%s=%02d:%02d.%03d\n",
                 mapToString(map), min, sec, msec);
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
    return true;
}
