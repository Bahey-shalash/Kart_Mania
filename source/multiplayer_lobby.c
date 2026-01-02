#include "multiplayer_lobby.h"

#include <nds.h>
#include <dswifi9.h>  // to remove after wifi simulator debug
#include <stdio.h>

#include "context.h"
#include "game_types.h"
#include "multiplayer.h"

//=============================================================================
// Lobby State
//=============================================================================

static int countdownTimer = 0;
static bool countdownActive = false;

//=============================================================================
// Public API - Lobby Initialization
//=============================================================================

void MultiplayerLobby_Init(void) {
    // Initialize console on sub-screen
    consoleDemoInit();
    consoleClear();

    printf("\x1b[2J");  // Clear screen
    printf("=== MULTIPLAYER LOBBY ===\n\n");
    printf("Connecting...\n");

    // Join the lobby (broadcasts presence to other players)
    Multiplayer_JoinLobby();

    // Set default map for multiplayer (ScorchingSands for now)
    // TODO: Add map selection for multiplayer later
    GameContext_SetMap(ScorchingSands);

    countdownTimer = 0;
    countdownActive = false;
}

//=============================================================================
// Public API - Lobby Update (call every frame)
//=============================================================================

GameState MultiplayerLobby_Update(void) {
    scanKeys();
    int keys = keysDown();

    //=========================================================================
    // Handle SELECT button to toggle ready state
    //=========================================================================
    if (keys & KEY_SELECT && !countdownActive) {
        int myID = Multiplayer_GetMyPlayerID();
        bool currentReady = Multiplayer_IsPlayerReady(myID);
        Multiplayer_SetReady(!currentReady);  // Toggle ready state
    }

    //=========================================================================
    // Handle B button to cancel and return to home page
    //=========================================================================
    if (keys & KEY_B && !countdownActive) {
        // Multiplayer_Cleanup();
        return HOME_PAGE;
    }

    //=========================================================================
    // Update lobby state (receive packets, check timeouts)
    //=========================================================================
    bool allReady = Multiplayer_UpdateLobby();

    //=========================================================================
    // Display lobby status on console
    //=========================================================================
    consoleClear();
    printf("=== MULTIPLAYER LOBBY ===\n\n");

    //==============
    // Show our IP so external simulators can target us
    unsigned long ip = Wifi_GetIP();
    printf("Your IP: %lu.%lu.%lu.%lu\n\n", (ip >> 0) & 0xFF, (ip >> 8) & 0xFF,
           (ip >> 16) & 0xFF, (ip >> 24) & 0xFF);

    //=============
    int myID = Multiplayer_GetMyPlayerID();
    int connectedCount = 0;
    int readyCount = 0;

    // Display all connected players
    for (int i = 0; i < MAX_MULTIPLAYER_PLAYERS; i++) {
        if (Multiplayer_IsPlayerConnected(i)) {
            connectedCount++;
            bool ready = Multiplayer_IsPlayerReady(i);
            if (ready)
                readyCount++;

            printf("Player %d: %s%s\n", i + 1, ready ? "[READY]   " : "[WAITING] ",
                   (i == myID) ? "(YOU)" : "");
        }
    }

    printf("\n(%d/%d ready)\n\n", readyCount, connectedCount);

    //=========================================================================
    // Handle countdown or show instructions
    //=========================================================================
    if (!countdownActive) {
        // Not in countdown - show instructions
        printf("Press SELECT when ready\n");
        printf("Press B to cancel\n");

        // Start countdown if all players are ready
        if (allReady && connectedCount >= 2) {
            countdownActive = true;
            countdownTimer = 180;  // 3 seconds at 60Hz (3 * 60 = 180 frames)
        }
    } else {
        // Countdown active - show countdown
        int secondsLeft = (countdownTimer / 60) + 1;  // Round up

        printf("\nStarting in %d...\n", secondsLeft);

        countdownTimer--;
        if (countdownTimer <= 0) {
            // Countdown finished - start race!
            GameContext_SetMap(ScorchingSands);  // not sure about this
            return GAMEPLAY;
        }
    }

    return MULTIPLAYER_LOBBY;
}
