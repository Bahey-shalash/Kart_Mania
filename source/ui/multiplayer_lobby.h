/**
 * File: multiplayer_lobby.h
 * --------------------------
 * Description: Multiplayer lobby screen UI for Kart Mania. Displays connected
 *              players, ready status, and countdown timer before race start.
 *              Provides interface for players to ready up and cancel lobby.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 06.01.2026
 *
 * UI Flow:
 *   1. Players join lobby (Multiplayer_JoinLobby broadcasts presence)
 *   2. Each player presses SELECT to mark ready
 *   3. When all players ready (2+ players), 3-second countdown starts
 *   4. Countdown completes → transition to GAMEPLAY state
 *   5. Press B at any time to cancel and return to HOME_PAGE
 *
 * Display Format:
 *   === MULTIPLAYER LOBBY ===
 *
 *   Player 1: [READY]    (YOU)
 *   Player 3: [WAITING]
 *
 *   (2/2 ready)
 *
 *   Starting in 3...
 *
 *   DEBUG: MyID=0 Connected=2
 *   AllReady=1 Countdown=1
 *   Packets: Sent=42 Recv=38
 *   Socket: Calls=150 OK=38 Filt=12
 *   IP: 192.168.1.100
 *   MAC: 00:09:BF:12:34:AB
 */

#ifndef MULTIPLAYER_LOBBY_H
#define MULTIPLAYER_LOBBY_H

#include "../core/game_types.h"

//=============================================================================
// Lobby Management
//=============================================================================

/**
 * Function: MultiplayerLobby_Init
 * --------------------------------
 * Initializes the multiplayer lobby screen and joins the network lobby.
 *
 * Actions:
 *   - Clears console on sub-screen
 *   - Calls Multiplayer_JoinLobby() to broadcast presence to other players
 *   - Sets default map to ScorchingSands (TODO: add map selection for multiplayer)
 *   - Resets countdown timer and active flag
 *
 * Prerequisites:
 *   - Multiplayer_Init() must have been called successfully
 *   - WiFi connection must be active
 */
void MultiplayerLobby_Init(void);

/**
 * Function: MultiplayerLobby_Update
 * ----------------------------------
 * Updates lobby state and handles user input. Call once per frame at 60Hz.
 *
 * Input Handling:
 *   - SELECT: Toggles local player ready state (disabled during countdown)
 *   - B: Cancels lobby, cleans up multiplayer, returns to HOME_PAGE
 *
 * Behavior:
 *   - Receives network packets and updates player connection/ready status
 *   - Displays player list with ready indicators
 *   - Shows debug statistics (packet counts, IP, MAC)
 *   - Starts 3-second countdown when all players ready (minimum 2 players)
 *   - Cancels countdown if any player unreadies or disconnects
 *   - Transitions to GAMEPLAY when countdown completes
 *
 * Returns:
 *   - MULTIPLAYER_LOBBY: Stay in lobby
 *   - GAMEPLAY: Countdown finished, start race
 *   - HOME_PAGE: Player pressed B to cancel
 *
 * Notes:
 *   - Countdown runs at 60 FPS (180 frames = 3 seconds)
 *   - Automatically cancels countdown if players < 2 or not all ready
 *   - Debug info displayed at bottom of screen for troubleshooting
 */
GameState MultiplayerLobby_Update(void);

#endif  // MULTIPLAYER_LOBBY_H
