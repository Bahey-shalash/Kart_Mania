/**
 * File: multiplayer.h
 * -------------------
 * Description: Peer-to-peer multiplayer racing system for Nintendo DS. Implements
 *              UDP broadcast-based networking for 2-8 players on local WiFi, with
 *              lobby synchronization, car state updates, and item synchronization.
 *              Uses Selective Repeat ARQ for reliable lobby messaging.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 04.01.2026
 *
 * Architecture Overview:
 *   - Peer-to-peer (no dedicated server)
 *   - Each DS runs full game independently
 *   - Each player controls ONE car (identified by player ID)
 *   - UDP broadcast on 255.255.255.255:8888
 *   - Non-blocking I/O (doesn't freeze game loop)
 *
 * Network Protocol:
 *   - Packet size: 32 bytes fixed (efficient for DS WiFi)
 *   - Lobby messages: Reliable (Selective Repeat ARQ with ACKs)
 *   - Car updates: Unreliable (15Hz, loss is acceptable)
 *   - Item events: Unreliable broadcast (best effort)
 *   - Player ID: Assigned by MAC address (hardware-unique, deterministic)
 *
 * Synchronization Strategy:
 *   - Lobby: Wait for all players to press "ready" before race start
 *   - Race: Each DS broadcasts car state every 4 frames (15Hz at 60 FPS)
 *   - Items: Broadcast placement/pickup, each DS creates items locally
 *   - Timeout: 3 seconds without packets = player disconnected
 *
 * Usage Flow:
 *   1. Home Page:   Call Multiplayer_Init() → Connects WiFi, assigns player ID
 *   2. Lobby:       Call Multiplayer_JoinLobby() → Discovers other players
 *                   Call Multiplayer_UpdateLobby() every frame
 *                   Call Multiplayer_SetReady(true) when player presses SELECT
 *   3. Race Start:  Call Multiplayer_StartRace() → Clears lobby ACK queues
 *   4. Race:        Call Multiplayer_SendCarState() every 4 frames
 *                   Call Multiplayer_ReceiveCarStates() every 4 frames
 *                   Call item sync functions when placing/picking up items
 *   5. Race End:    Call Multiplayer_Cleanup() → Disconnects, frees resources
 */

#ifndef MULTIPLAYER_H
#define MULTIPLAYER_H

#include <stdbool.h>

#include "../gameplay/Car.h"
#include "../core/game_types.h"

//=============================================================================
// MULTIPLAYER SYSTEM OVERVIEW
//=============================================================================
// Peer-to-peer multiplayer racing for 2-8 players over local WiFi.
//
// Network Architecture:
//   - Protocol: UDP broadcast (255.255.255.255:8888)
//   - Player ID: MAC address-based (0-7, hardware-unique, deterministic)
//   - Car updates: 15Hz unreliable broadcast (position, speed, angle, lap, item)
//   - Lobby: Reliable Selective Repeat ARQ (join, ready, heartbeat, ACK)
//   - Items: Best-effort broadcast (placement, box pickup)
//
// Synchronization:
//   - Each DS runs full game independently (no authoritative server)
//   - Player 0-7 controls car 0-7 (1:1 mapping)
//   - Lobby waits for all players to be "ready" before starting race
//   - Timeout: 3s without packets = player disconnected
//   - Race start synchronized when all lobby players mark ready
//
// Reliability Strategy:
//   - Lobby messages: Selective Repeat ARQ (500ms timeout, 5 retries, ACK-based)
//   - Car updates: Unreliable (15Hz is fast enough to tolerate packet loss)
//   - Items: Unreliable (visual only, loss is acceptable)
//
// Key Design Decisions:
//   - MAC-based player ID (not IP) prevents collisions from sequential DHCP
//   - 32-byte fixed packet size (efficient for DS WiFi hardware)
//   - Lobby uses ARQ, race doesn't (tradeoff: reliability vs. overhead)
//   - Item sync via broadcast (simpler than client-server authoritative model)
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
 * Prepare for race start
 * - Clears pending lobby ACK queues
 * - Call this once when transitioning from lobby to race
 */
void Multiplayer_StartRace(void);

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
void Multiplayer_SendItemPlacement(Item itemType, const Vec2* position, int angle512,
                                   Q16_8 speed, int shooterCarIndex);

/**
 * Receive item placements from other players
 * - Call every frame during race (or when processing network packets)
 * - Returns item placement data if available, NULL otherwise
 * - Caller should create the item on their local track
 */
typedef struct {
    bool valid;           // True if data is available
    uint8_t playerID;     // Which player placed the item
    Item itemType;        // What item was placed
    Vec2 position;        // Where it was placed
    int angle512;         // Direction (for projectiles)
    Q16_8 speed;          // Initial speed (for projectiles)
    int shooterCarIndex;  // Who fired this projectile
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


/**
 * Nuclear option: Completely reset all multiplayer/WiFi state
 * Use this when returning to home page or when things are stuck
 */
void Multiplayer_NukeConnectivity(void);

/**
 * Get debug statistics (for troubleshooting)
 * @param sentCount - Output: total packets sent
 * @param receivedCount - Output: total packets received
 */
void Multiplayer_GetDebugStats(int* sentCount, int* receivedCount);

#endif  // MULTIPLAYER_H
