#ifndef MULTIPLAYER_LOBBY_H
#define MULTIPLAYER_LOBBY_H

#include "game_types.h"
// BAHEY------

/**
 * Initialize multiplayer lobby
 * - Sets up console on sub-screen
 * - Calls Multiplayer_JoinLobby() to broadcast presence
 * - Resets countdown state
 */
void MultiplayerLobby_Init(void);

/**
 * Update lobby state (call every frame)
 * - Handles SELECT button (toggle ready)
 * - Handles B button (cancel and return to home)
 * - Updates and displays player list
 * - Shows countdown when all players ready
 * 
 * Returns: Next game state (MULTIPLAYER_LOBBY or GAMEPLAY or HOME_PAGE)
 */
GameState MultiplayerLobby_Update(void);

#endif // MULTIPLAYER_LOBBY_H
