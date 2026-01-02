#ifndef MULTIPLAYER_H
#define MULTIPLAYER_H

#include <stdbool.h>

#include "Car.h"
#include "game_types.h"

//=============================================================================
// Multiplayer System
//=============================================================================
// Peer-to-peer multiplayer racing for 2-8 players.
//
// Architecture:
//   - Each DS runs the full game independently
//   - Each DS controls ONE car (the player car)
//   - Car states are broadcast at 15Hz (every 4 physics frames)
//   - Player ID is auto-assigned based on MAC address
//
// Flow:
//   1. Home page: Call Multiplayer_Init()
//   2. Lobby: Call Multiplayer_JoinLobby(), wait for all ready
//   3. Race: Call Multiplayer_SendCarState() and ReceiveCarStates() every 4 frames
//   4. End: Call Multiplayer_Cleanup()
//=============================================================================

//=============================================================================
// Constants
//=============================================================================

#define MAX_MULTIPLAYER_PLAYERS 8  // Maximum players (matches MAX_CARS)

//=============================================================================
// Initialization & Cleanup
//=============================================================================

/**
 * Initialize multiplayer system
 * - Calls initWiFi() and openSocket() internally
 * - Auto-assigns player ID based on IP address (deterministic)
 * - Shows connection status on console (sub-screen)
 *
 * Returns: Assigned player ID (0-7), or -1 on error
 *
 * Error conditions:
 *   - WiFi disabled/unavailable
 *   - "MES-NDS" AP not found (5 second timeout)
 *   - Connection failed (10 second timeout)
 *   - Socket creation failed
 */
int Multiplayer_Init(void);

/**
 * Cleanup multiplayer system
 * - Broadcasts disconnect message
 * - Calls closeSocket() and disconnectFromWiFi()
 * - Safe to call multiple times
 */
void Multiplayer_Cleanup(void);

//=============================================================================
// Player Info
//=============================================================================

/**
 * Get my assigned player ID
 * Returns: 0-7, or -1 if not initialized
 */
int Multiplayer_GetMyPlayerID(void);

/**
 * Get number of currently connected players
 * Returns: 1-8 (includes self)
 */
int Multiplayer_GetConnectedCount(void);

/**
 * Check if a specific player is connected
 * Returns: true if player is in the game
 */
bool Multiplayer_IsPlayerConnected(int playerID);

/**
 * Check if a specific player is ready (in lobby)
 * Returns: true if player has pressed SELECT
 */
bool Multiplayer_IsPlayerReady(int playerID);

//=============================================================================
// Lobby Functions
//=============================================================================

/**
 * Join multiplayer lobby
 * - Broadcasts presence to other players
 * - Call this once when entering MULTIPLAYER_LOBBY state
 */
void Multiplayer_JoinLobby(void);

/**
 * Update lobby state (call every frame in lobby)
 * - Receives lobby packets from other players
 * - Updates player connection/ready status
 * - Checks for timeouts (3 seconds no packets = disconnected)
 *
 * Returns: true if all connected players are ready and race should start
 *          (requires at least 2 players)
 */
bool Multiplayer_UpdateLobby(void);

/**
 * Mark local player as ready/not ready
 * - Broadcasts ready state to all players
 * - Call when player presses SELECT button
 */
void Multiplayer_SetReady(bool ready);

//=============================================================================
// Race Functions
//=============================================================================

/**
 * Send my car state to all players
 * - Call every 4 frames (15Hz) during race
 * - Sends: position, speed, angle, lap, item
 * - Packet size: ~25 bytes
 */
void Multiplayer_SendCarState(const Car* car);

/**
 * Receive and update other players' car states
 * - Call every 4 frames (15Hz) during race
 * - Directly updates cars array with received network data
 * - Skips own car (myPlayerID)
 * - Marks players as connected when packets received
 *
 * @param cars - Array of cars to update (typically KartMania.cars)
 * @param carCount - Number of cars in array (typically MAX_CARS = 8)
 */
void Multiplayer_ReceiveCarStates(Car* cars, int carCount);

/**
 * Broadcast that an item was placed/thrown on the track
 * - Call when player uses an item (banana, shell, etc.)
 * - Sends: item type, position, angle, speed
 * - Other players will create the same item on their screens
 *
 * @param itemType - What item was placed (ITEM_BANANA, ITEM_GREEN_SHELL, etc.)
 * @param position - Where the item was placed (world coordinates)
 * @param angle512 - Direction (for projectiles like shells)
 * @param speed - Initial speed (for projectiles)
 */
void Multiplayer_SendItemPlacement(Item itemType, Vec2 position, int angle512, Q16_8 speed);

/**
 * Receive item placements from other players
 * - Call every frame during race (or when processing network packets)
 * - Returns item placement data if available, NULL otherwise
 * - Caller should create the item on their local track
 */
typedef struct {
    bool valid;         // True if data is available
    uint8_t playerID;   // Which player placed the item
    Item itemType;      // What item was placed
    Vec2 position;      // Where it was placed
    int angle512;       // Direction (for projectiles)
    Q16_8 speed;        // Initial speed (for projectiles)
} ItemPlacementData;

ItemPlacementData Multiplayer_ReceiveItemPlacements(void);

/**
 * Broadcast that an item box was picked up
 * - Call when any player picks up an item box
 * - Other players will deactivate the same item box on their screens
 *
 * @param boxIndex - Index of the item box that was picked up
 */
void Multiplayer_SendItemBoxPickup(int boxIndex);

/**
 * Receive item box pickups from other players
 * - Call every frame during race (or when processing network packets)
 * - Returns box index if available, -1 otherwise
 */
int Multiplayer_ReceiveItemBoxPickup(void);

#endif  // MULTIPLAYER_H