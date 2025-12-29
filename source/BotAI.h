#ifndef BOTAI_H
#define BOTAI_H

#include <stdbool.h>

#include "Car.h"
#include "Items.h"
#include "fixedmath2d.h"
#include "gameplay_logic.h"

//=============================================================================
// Constants
//=============================================================================

#define MAX_WAYPOINTS 96         // Maximum waypoints per track (full lap coverage)
#define LOOKAHEAD_DISTANCE IntToFixed(80)   // Distance to look ahead for steering
#define WAYPOINT_REACH_THRESHOLD IntToFixed(30)  // When to advance to next waypoint
#define ITEM_SEARCH_RADIUS IntToFixed(150)  // Range to detect item boxes
#define HAZARD_DETECT_RANGE IntToFixed(100) // Range to detect hazards ahead
#define OVERTAKE_DISTANCE IntToFixed(50)    // Distance to trigger overtake behavior

// Mistake parameters
#define MISTAKE_INTERVAL_BASE 200   // Base frames between mistakes
#define MISTAKE_DURATION_MIN 15     // Minimum frames per mistake
#define MISTAKE_DURATION_MAX 45     // Maximum frames per mistake
#define MISTAKE_OFFSET_MIN 20       // Minimum path deviation (pixels)
#define MISTAKE_OFFSET_MAX 60       // Maximum path deviation (pixels)

// Hazard avoidance
#define HAZARD_AVOIDANCE_DURATION 30  // Frames of avoidance maneuver

// Stuck detection / anti-wall-bounce
#define STUCK_MOVE_THRESHOLD IntToFixed(12)  // Considered stationary if moving < 12px
#define STUCK_BOUNCE_FRAMES 90               // Frames of bouncing before teleport
#define WALL_BOUNCE_LIMIT 3                  // Teleport after this many wall hits
#define WALL_BOUNCE_COOLDOWN 12              // Frames between counting wall hits

// Rubber-banding
#define RUBBERBAND_BOOST_THRESHOLD IntToFixed(300)  // Distance to trigger catch-up
#define RUBBERBAND_SLOW_THRESHOLD IntToFixed(400)   // Distance to slow leaders
#define RUBBERBAND_BOOST_MULT (IntToFixed(110) / 100) // 1.10x speed boost
#define RUBBERBAND_SLOW_MULT (IntToFixed(90) / 100)   // 0.90x speed reduction

// Skill level presets (affects mistake frequency and reaction time)
typedef enum {
    SKILL_EASY = 0,   // Frequent mistakes, slow reactions
    SKILL_MEDIUM = 1, // Moderate mistakes, average reactions
    SKILL_HARD = 2    // Rare mistakes, fast reactions
} BotSkillLevel;

//=============================================================================
// Bot Personality Traits (Q16.8 fixed-point percentages)
//=============================================================================

typedef struct {
    Q16_8 aggression;       // 0-256: How likely to use offensive items (0% - 100%)
    Q16_8 consistency;      // 0-256: How often they make mistakes (higher = fewer mistakes)
    Q16_8 itemPriority;     // 0-256: How much they prioritize collecting items vs racing line
    int reactionDelay;      // Frames of delay before reacting to hazards/items
    BotSkillLevel skillLevel; // Overall skill tier
} BotPersonality;

//=============================================================================
// Bot Runtime State
//=============================================================================

typedef struct {
    // Navigation state
    int targetWaypoint;    // Current waypoint index being chased
    int nextWaypoint;      // Next waypoint for lookahead
    Vec2 targetPosition;   // Exact position bot is steering toward

    // Behavior state
    int mistakeTimer;      // Countdown until next intentional mistake
    int correctionTimer;   // How long current mistake lasts
    int stuckTimer;        // Frames bot has been stuck/slow
    bool isOvertaking;     // Currently deviating from racing line to pass
    Vec2 overtakeTarget;   // Position to steer toward when overtaking

    // Item/hazard tracking
    Vec2 nearestHazardPos;      // Position of closest hazard to avoid
    int hazardAvoidanceTimer;   // Frames remaining in avoidance maneuver
    Vec2 targetItemBoxPos;      // Position of item box to collect
    bool seekingItemBox;        // Currently deviating to collect item

    // Decision making
    int itemUsageTimer;    // Cooldown between item uses (prevents spam)
    int stuckStillFrames;  // Frames spent in roughly same spot
    Vec2 lastPos;          // Last recorded post-physics position
    bool lastPosInitialized;
    int wallBounceCount;   // Number of recent wall impacts
    int wallBounceCooldown;// Cooldown to avoid double-counting same impact

    // Personality
    BotPersonality personality;      // Current working personality
    BotPersonality basePersonality;  // Baseline personality (never mutated)
} BotState;

//=============================================================================
// Waypoint System
//=============================================================================

typedef struct {
    Vec2 position;         // Waypoint location in world space
    Q16_8 targetSpeed;     // Recommended speed at this waypoint (for corners)
    int cornerAngle512;    // Expected angle through this waypoint
    bool isCheckpoint;     // Whether this is a mandatory checkpoint
} Waypoint;

typedef struct {
    Waypoint waypoints[MAX_WAYPOINTS];
    int count;
} WaypointPath;

//=============================================================================
// Public API
//=============================================================================

/**
 * Initialize bot AI system for the current map
 * Loads waypoint data, initializes bot states
 *
 * @param map - Current track (ScorchingSands, AlpinRush, etc.)
 */
void BotAI_Init(Map map);

/**
 * Main update function - call once per frame for each bot
 * Handles navigation, item usage, hazard avoidance, and all AI decisions
 *
 * @param car - The bot's car to control
 * @param botIndex - Bot index (0-7, playerIndex should be skipped by caller)
 * @param raceState - Current race state (for accessing other cars, positions, etc.)
 */
void BotAI_Update(Car* car, int botIndex, const RaceState* raceState);

/**
 * Post-physics update for bots (call after Car_Update / collision resolution)
 * Handles stuck detection and warp-to-checkpoint recovery.
 */
void BotAI_PostPhysicsUpdate(Car* car, int botIndex);

/**
 * Reset bot state (for race restart)
 * Clears timers, resets waypoint tracking
 *
 * @param botIndex - Which bot to reset
 */
void BotAI_Reset(int botIndex);

/**
 * Assign personality to a bot (call after Race_Init)
 *
 * @param botIndex - Which bot to configure
 * @param personality - Personality traits to assign
 */
void BotAI_SetPersonality(int botIndex, BotPersonality personality);

/**
 * Generate a random personality for a skill level
 * Useful for quick bot setup with varied behavior
 *
 * @param skillLevel - Desired skill tier
 * @return Randomized personality within skill constraints
 */
BotPersonality BotAI_GeneratePersonality(BotSkillLevel skillLevel);

#endif  // BOTAI_H
