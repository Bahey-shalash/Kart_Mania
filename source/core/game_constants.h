// HUGO------

#ifndef GAME_CONSTANTS_H
#define GAME_CONSTANTS_H

#include "../math/fixedmath.h"

/*
 * game_constants.h
 *
 * Central location for all game-tunable constants and magic numbers.
 * This header defines physics values, collision thresholds, durations,
 * rendering constants, and other numeric values used throughout the codebase.
 */

//=============================================================================
// Rendering & Display Constants
//=============================================================================

// Sprite constants
#define CAR_SPRITE_SIZE 32
#define CAR_SPRITE_CENTER_OFFSET 16  // Half of sprite size for centering
#define CAR_RADIUS \
    12  // Collision radius (adjust if needed - currently undefined in your code)

//=============================================================================
// Physics & Movement Constants
//=============================================================================

// Speed thresholds
#define MIN_SPEED_THRESHOLD 5  // Minimum speed before snapping to zero in Q16.8
#define MIN_MOVING_SPEED 25    // Minimum speed to be considered "moving" in Q16.8

// Speed divisors for terrain/item effects
#define SAND_SPEED_DIVISOR 2    // Speed reduced by /2 on sand
#define BANANA_SPEED_DIVISOR 3  // Speed reduced by /3 when hit by banana
#define OIL_SPEED_DIVISOR 2     // Speed reduced by /2 when hit by oil slick

//=============================================================================
// Race Physics (Q16.8 - see fixedmath.h for format)
//=============================================================================

#define TURN_STEP_50CC 3             // Steering delta per input for 50cc
#define SPEED_50CC (FIXED_ONE * 3)   // Max speed: 3.0 px/frame in Q16.8
#define ACCEL_50CC IntToFixed(1)     // Acceleration: 1.0 px/frame^2 in Q16.8
#define FRICTION_50CC 240            // Base friction (240/256 = 0.9375)
#define COLLISION_LOCKOUT_FRAMES 60  // Frames to disable accel after wall hit

// Terrain modifiers (Q16.8 where noted)
#define SAND_FRICTION 150  // Sand friction (150/256 = 0.586)
#define SAND_MAX_SPEED (SPEED_50CC / SAND_SPEED_DIVISOR)  // Cap on sand, Q16.8 units

//=============================================================================
// Angle Helpers (binary angle, 0-511)
//=============================================================================

#define ANGLE_RIGHT 0                                    // 0°
#define ANGLE_DOWN ANGLE_QUARTER                         // 90°
#define ANGLE_LEFT ANGLE_HALF                            // 180°
#define ANGLE_UP (ANGLE_HALF + ANGLE_QUARTER)            // 270°
#define ANGLE_DOWN_RIGHT (ANGLE_QUARTER / 2)             // 45°
#define ANGLE_DOWN_LEFT (ANGLE_HALF - ANGLE_DOWN_RIGHT)  // 135°
#define ANGLE_UP_LEFT (ANGLE_HALF + ANGLE_DOWN_RIGHT)    // 225°
#define ANGLE_UP_RIGHT (ANGLE_FULL - ANGLE_DOWN_RIGHT)   // 315°

//=============================================================================
// Race Layout (pixel coordinates unless noted)
//=============================================================================

// 904 Left Column  || 32px spacing || 936 Right Column
#define START_LINE_X 904
// 580 First Place || 24 px spacing per space || 48px per column
#define START_LINE_Y 580

#define START_FACING_ANGLE ANGLE_UP

#define FINISH_LINE_Y 540

#define CHECKPOINT_DIVIDE_X 512
#define CHECKPOINT_DIVIDE_Y 512

// Lap counts per map
#define LAPS_NONE 0
#define LAPS_SCORCHING_SANDS 2
#define LAPS_ALPIN_RUSH 10
#define LAPS_NEON_CIRCUIT 10

//=============================================================================
// Item Spawn & Placement Constants
//=============================================================================

// Projectile spawn offsets (in pixels)
#define PROJECTILE_SPAWN_OFFSET 30  // Distance ahead of car to spawn projectiles
#define HAZARD_DROP_OFFSET 40       // Distance behind car to drop hazards

// Collision detection
#define CAR_COLLISION_SIZE 32     // Car hitbox size for item collision
#define COLLISION_BUFFER_ZONE 64  // Buffer zone around screen for collision detection
#define ITEM_PICKUP_THRESHOLD 14  // Debug threshold for item box pickup distance
#define ITEM_PICKUP_DEBUG_DISTANCE \
    50  // Distance to start debug logging for item boxes

//=============================================================================
// Item Lifetime Constants
//=============================================================================

// Projectile & hazard lifetimes (in seconds)
#define PROJECTILE_LIFETIME_SECONDS \
    20                             // Max lifetime for green/red shells and missiles
#define BOMB_LIFETIME_SECONDS 5    // Time until bomb auto-explodes
#define ITEM_LIFETIME_INFINITE -1  // Marker for items that never expire

//=============================================================================
// Item Effect Constants
//=============================================================================

// Bomb explosion
#define BOMB_KNOCKBACK_DISTANCE 40  // Knockback distance in pixels from bomb explosion

// Shell hit effects
#define SHELL_SPIN_ANGLE_POS 64   // +45 degrees spin when hit by shell
#define SHELL_SPIN_ANGLE_NEG -64  // -45 degrees spin when hit by shell

// Projectile homing
#define HOMING_TURN_RATE 5  // Max turn rate per frame for homing projectiles (degrees)

// Projectile homing behavior
#define ATTACK_RADIUS IntToFixed(150)  // 150 pixels - switch to direct homing
#define MAX_WAYPOINT_VISITS 150        // Safety limit to prevent infinite loops

// Projectile immunity
#define SHOOTER_IMMUNITY_FRAMES 120           // 2 seconds at 60fps (multiplayer)
#define IMMUNITY_MIN_DISTANCE IntToFixed(50)  // Must be 50px away to lose immunity

// NEW: Lap detection for projectiles (single player debug feature)
#define WAYPOINT_LAP_THRESHOLD \
    10  // Must pass within 10 waypoints of start to count as lap

//=============================================================================
// Rendering & Display Constants
//=============================================================================

// VRAM & Memory
#define VRAM_BANK_SIZE (128 * 1024)  // DS VRAM bank size in bytes
#define PALETTE_SIZE 512             // Palette memory size

// Tile & Map Constants
#define TILE_SIZE 8                 // Tile size in pixels
#define TILES_PER_SCREEN_WIDTH 32   // Screen width in tiles (256/8)
#define TILES_PER_SCREEN_HEIGHT 24  // Screen height in tiles (192/8)
#define QUAD_SIZE 256               // Quadrant size in pixels
#define QUAD_SIZE_DOUBLE 512        // Double quadrant size (for bounds checking)
#define TILE_INDEX_MASK 0x3FF       // Mask for extracting tile index from map entry
#define TILE_DATA_SIZE 64           // Size of tile data in bytes
#define TILE_WIDTH_PIXELS 8         // Width of a tile in pixels
#define SCREEN_SIZE_TILES 32        // Screen size in tiles (32x32)

// Quadrant grid
#define QUADRANT_GRID_SIZE 3    // 3x3 quadrant grid
#define QUAD_BOUNDARY_LOW 256   // First quadrant boundary
#define QUAD_BOUNDARY_HIGH 512  // Second quadrant boundary

//=============================================================================
// Color & Graphics Constants
//=============================================================================

// Color bit manipulation
#define COLOR_5BIT_MASK 0x1F  // Mask for 5-bit color channel
#define COLOR_RED_SHIFT 0     // Bit shift for red channel
#define COLOR_GREEN_SHIFT 5   // Bit shift for green channel
#define COLOR_BLUE_SHIFT 10   // Bit shift for blue channel

//=============================================================================
// Audio Constants
//=============================================================================

#define VOLUME_MAX 1024  // Maximum volume (100%)
#define VOLUME_MUTE 0    // Mute volume (0%)

//=============================================================================
// Time & Frequency Constants
//=============================================================================

#define MS_PER_SECOND 1000     // Milliseconds per second
#define SECONDS_PER_MINUTE 60  // Seconds per minute
#define CHRONO_FREQ_HZ 1000    // Chronometer frequency in Hz

//=============================================================================
// Special Values & Sentinels
//=============================================================================

#define INVALID_CAR_INDEX -1   // Sentinel value for invalid car index
#define MAX_DISTANCE_INF 9999  // "Infinite" distance for initialization

//=============================================================================
// Car Physics Constants
//=============================================================================

#define CAR_NAME_MAX_LENGTH \
    31  // Maximum length for car name (without null terminator)

#endif  // GAME_CONSTANTS_H
