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
#define CAR_RADIUS 16  // Collision radius for 32x32 sprite

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

// Screen dimensions
#define SCREEN_WIDTH 256   // Screen width in pixels
#define SCREEN_HEIGHT 192  // Screen height in pixels

// Map dimensions
#define MAP_SIZE 1024              // Map size in pixels (1024x1024)
#define MAX_SCROLL_X (MAP_SIZE - SCREEN_WIDTH)   // Maximum scroll X position
#define MAX_SCROLL_Y (MAP_SIZE - SCREEN_HEIGHT)  // Maximum scroll Y position

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

//=============================================================================
// UI Constants - Play Again Screen
//=============================================================================

// Palette indices for selection highlighting
#define PA_SELECTION_PAL_BASE 240  // Base palette index for selection tiles

// YES button touch hitbox (pixels)
#define PA_YES_TOUCH_X_MIN 50
#define PA_YES_TOUCH_X_MAX 120
#define PA_YES_TOUCH_Y_MIN 85
#define PA_YES_TOUCH_Y_MAX 175

// NO button touch hitbox (pixels)
#define PA_NO_TOUCH_X_MIN 136
#define PA_NO_TOUCH_X_MAX 206
#define PA_NO_TOUCH_Y_MIN 85
#define PA_NO_TOUCH_Y_MAX 175

// Selection rectangle tile coordinates
#define PA_YES_RECT_X_START 6
#define PA_YES_RECT_Y_START 10
#define PA_YES_RECT_X_END 16
#define PA_YES_RECT_Y_END 20

#define PA_NO_RECT_X_START 17
#define PA_NO_RECT_Y_START 10
#define PA_NO_RECT_X_END 27
#define PA_NO_RECT_Y_END 20

//=============================================================================
// UI Constants - Settings Screen
//=============================================================================

// Palette indices for selection highlighting
#define SETTINGS_SELECTION_PAL_BASE 244

// Toggle rectangle parameters (for WiFi/Music/Sound FX pills)
#define SETTINGS_TOGGLE_START_X 21
#define SETTINGS_TOGGLE_WIDTH 9

// WiFi toggle Y coordinates
#define SETTINGS_WIFI_TOGGLE_Y_START 1
#define SETTINGS_WIFI_TOGGLE_Y_END 5

// Music toggle Y coordinates
#define SETTINGS_MUSIC_TOGGLE_Y_START 5
#define SETTINGS_MUSIC_TOGGLE_Y_END 9

// Sound FX toggle Y coordinates
#define SETTINGS_SOUNDFX_TOGGLE_Y_START 9
#define SETTINGS_SOUNDFX_TOGGLE_Y_END 13

// WiFi selection rectangle tile coordinates
#define SETTINGS_WIFI_RECT_X_START 2
#define SETTINGS_WIFI_RECT_Y_START 1
#define SETTINGS_WIFI_RECT_X_END 7
#define SETTINGS_WIFI_RECT_Y_END 4

// Music selection rectangle tile coordinates
#define SETTINGS_MUSIC_RECT_X_START 2
#define SETTINGS_MUSIC_RECT_Y_START 5
#define SETTINGS_MUSIC_RECT_X_END 9
#define SETTINGS_MUSIC_RECT_Y_END 8

// Sound FX selection rectangle tile coordinates
#define SETTINGS_SOUNDFX_RECT_X_START 2
#define SETTINGS_SOUNDFX_RECT_Y_START 9
#define SETTINGS_SOUNDFX_RECT_X_END 13
#define SETTINGS_SOUNDFX_RECT_Y_END 12

// Save button selection rectangle tile coordinates
#define SETTINGS_SAVE_RECT_X_START 4
#define SETTINGS_SAVE_RECT_Y_START 15
#define SETTINGS_SAVE_RECT_X_END 14
#define SETTINGS_SAVE_RECT_Y_END 23

// Back button selection rectangle tile coordinates
#define SETTINGS_BACK_RECT_X_START 12
#define SETTINGS_BACK_RECT_Y_START 15
#define SETTINGS_BACK_RECT_X_END 20
#define SETTINGS_BACK_RECT_Y_END 23

// Home button selection rectangle tile coordinates
#define SETTINGS_HOME_RECT_X_START 20
#define SETTINGS_HOME_RECT_Y_START 15
#define SETTINGS_HOME_RECT_X_END 28
#define SETTINGS_HOME_RECT_Y_END 23

// WiFi text touch hitbox (pixels)
#define SETTINGS_WIFI_TEXT_X_MIN 23
#define SETTINGS_WIFI_TEXT_X_MAX 53
#define SETTINGS_WIFI_TEXT_Y_MIN 10
#define SETTINGS_WIFI_TEXT_Y_MAX 25

// WiFi pill touch hitbox (pixels)
#define SETTINGS_WIFI_PILL_X_MIN 175
#define SETTINGS_WIFI_PILL_X_MAX 240
#define SETTINGS_WIFI_PILL_Y_MIN 10
#define SETTINGS_WIFI_PILL_Y_MAX 37

// Music text touch hitbox (pixels)
#define SETTINGS_MUSIC_TEXT_X_MIN 24
#define SETTINGS_MUSIC_TEXT_X_MAX 69
#define SETTINGS_MUSIC_TEXT_Y_MIN 40
#define SETTINGS_MUSIC_TEXT_Y_MAX 55

// Music pill touch hitbox (pixels)
#define SETTINGS_MUSIC_PILL_X_MIN 175
#define SETTINGS_MUSIC_PILL_X_MAX 240
#define SETTINGS_MUSIC_PILL_Y_MIN 40
#define SETTINGS_MUSIC_PILL_Y_MAX 67

// Sound FX text touch hitbox (pixels)
#define SETTINGS_SOUNDFX_TEXT_X_MIN 23
#define SETTINGS_SOUNDFX_TEXT_X_MAX 99
#define SETTINGS_SOUNDFX_TEXT_Y_MIN 70
#define SETTINGS_SOUNDFX_TEXT_Y_MAX 85

// Sound FX pill touch hitbox (pixels)
#define SETTINGS_SOUNDFX_PILL_X_MIN 175
#define SETTINGS_SOUNDFX_PILL_X_MAX 240
#define SETTINGS_SOUNDFX_PILL_Y_MIN 70
#define SETTINGS_SOUNDFX_PILL_Y_MAX 97

// Save button touch hitbox (circle: center=64,152 diameter=48)
#define SETTINGS_SAVE_TOUCH_X_MIN 40
#define SETTINGS_SAVE_TOUCH_X_MAX 88
#define SETTINGS_SAVE_TOUCH_Y_MIN 128
#define SETTINGS_SAVE_TOUCH_Y_MAX 176

// Back button touch hitbox (circle: center=128,152 diameter=48)
#define SETTINGS_BACK_TOUCH_X_MIN 104
#define SETTINGS_BACK_TOUCH_X_MAX 152
#define SETTINGS_BACK_TOUCH_Y_MIN 128
#define SETTINGS_BACK_TOUCH_Y_MAX 176

// Home button touch hitbox (circle: center=192,152 diameter=48)
#define SETTINGS_HOME_TOUCH_X_MIN 168
#define SETTINGS_HOME_TOUCH_X_MAX 216
#define SETTINGS_HOME_TOUCH_Y_MIN 128
#define SETTINGS_HOME_TOUCH_Y_MAX 176

//=============================================================================
// UI Constants - Home Page Screen
//=============================================================================

// Palette indices for selection highlighting
#define HOME_HL_PAL_BASE 251

// Menu layout parameters
#define HOME_MENU_X 32
#define HOME_MENU_WIDTH 192
#define HOME_MENU_HEIGHT 40
#define HOME_MENU_SPACING 54
#define HOME_MENU_Y_START 24

// Highlight tile parameters
#define HOME_HIGHLIGHT_TILE_X 6
#define HOME_HIGHLIGHT_TILE_WIDTH 20
#define HOME_HIGHLIGHT_TILE_HEIGHT 3

// Kart sprite parameters
#define HOME_KART_INITIAL_X -64
#define HOME_KART_Y 120
#define HOME_KART_OFFSCREEN_X 256

//=============================================================================
// Storage Constants
//=============================================================================

// Personal best times storage limits
#define STORAGE_MAX_MAP_RECORDS 10

//=============================================================================
// Countdown Constants
//=============================================================================

#define COUNTDOWN_NUMBER_DURATION 60  // Duration for each countdown number (3, 2, 1)
#define COUNTDOWN_GO_DURATION 60      // Duration for "GO!" display
#define FINISH_DELAY_FRAMES (5 * 60)  // Frames to wait before showing finish menu (5 seconds)

//=============================================================================
// Terrain Detection Constants
//=============================================================================

// Track colors (5-bit RGB)
#define TERRAIN_GRAY_MAIN_R5 12
#define TERRAIN_GRAY_MAIN_G5 12
#define TERRAIN_GRAY_MAIN_B5 12
#define TERRAIN_GRAY_LIGHT_R5 14
#define TERRAIN_GRAY_LIGHT_G5 14
#define TERRAIN_GRAY_LIGHT_B5 14

// Sand colors (5-bit RGB)
#define TERRAIN_SAND_PRIMARY_R5 20
#define TERRAIN_SAND_PRIMARY_G5 18
#define TERRAIN_SAND_PRIMARY_B5 12
#define TERRAIN_SAND_SECONDARY_R5 22
#define TERRAIN_SAND_SECONDARY_G5 20
#define TERRAIN_SAND_SECONDARY_B5 14

// Color matching tolerance
#define TERRAIN_COLOR_TOLERANCE_5BIT 1

#endif  // GAME_CONSTANTS_H
