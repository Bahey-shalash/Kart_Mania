/**
 * File: multiplayer_lobby.c
 * --------------------------
 * Description: Implementation of multiplayer lobby UI for Kart Mania. Manages
 *              player ready states, countdown timer, and debug display for
 *              2-8 player local WiFi racing.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 06.01.2026
 *
 * Implementation Details:
 *
 * COUNTDOWN MECHANISM:
 *   - Triggered when all connected players mark ready (minimum 2 players)
 *   - Runs for 180 frames at 60 FPS = 3 seconds
 *   - Automatically cancels if:
 *     * Any player unreadies (toggles SELECT again)
 *     * Any player disconnects (timeout or explicit disconnect)
 *     * Player count drops below 2
 *   - SELECT button disabled during countdown (prevents toggling)
 *   - B button works at any time (even during countdown)
 *
 * DISPLAY LAYOUT:
 *   - Top: Lobby title and player list
 *   - Middle: Ready count and countdown (if active)
 *   - Bottom: Debug statistics (packets, IP, MAC)
 *
 * DEBUG INFORMATION:
 *   - MyID: Local player ID (0-7, MAC address-based)
 *   - Connected: Number of players in lobby
 *   - AllReady: Whether all players have pressed SELECT
 *   - Countdown: Whether countdown is active
 *   - Packets: Sent/Received counts (from multiplayer.c)
 *   - Socket: Raw recvfrom() stats (from WiFi_minilib.c)
 *   - IP: Local IPv4 address from DHCP
 *   - MAC: Hardware WiFi MAC address
 *
 * STATE MANAGEMENT:
 *   - countdownTimer: Frames remaining in countdown (180 → 0)
 *   - countdownActive: Whether countdown is running
 *   - Reset on lobby entry, preserved across updates until race start
 */

#include "multiplayer_lobby.h"

#include <nds.h>
#include <stdio.h>

#include "../core/context.h"
#include "../core/game_types.h"
#include "../network/multiplayer.h"
#include "../network/WiFi_minilib.h"

//=============================================================================
// Module State
//=============================================================================

/** Countdown timer in frames (180 frames = 3 seconds at 60 FPS) */
static int countdownTimer = 0;

/** Whether the countdown is currently running */
static bool countdownActive = false;

//=============================================================================
// Public API - Lobby Initialization
//=============================================================================

/**
 * Function: MultiplayerLobby_Init
 * --------------------------------
 * Initializes the multiplayer lobby screen and joins the network lobby.
 */
void MultiplayerLobby_Init(void) {
    // Initialize console on sub-screen for lobby UI
    consoleDemoInit();
    consoleClear();

    // Clear screen using ANSI escape code
    printf("\x1b[2J");
    printf("=== MULTIPLAYER LOBBY ===\n\n");
    printf("Connecting...\n");

    // Join the lobby - broadcasts MSG_LOBBY_JOIN to discover other players
    // This sends our presence to all devices on the network
    Multiplayer_JoinLobby();

    // Set default map for multiplayer (ScorchingSands for now)
    // TODO: Add map selection screen for multiplayer mode
    GameContext_SetMap(ScorchingSands);

    // Reset countdown state
    countdownTimer = 0;
    countdownActive = false;
}

//=============================================================================
// Public API - Lobby Update
//=============================================================================

/**
 * Function: MultiplayerLobby_Update
 * ----------------------------------
 * Updates lobby state and handles user input. Call once per frame at 60Hz.
 */
GameState MultiplayerLobby_Update(void) {
    // Read button inputs
    scanKeys();
    int keys = keysDown();

    //=========================================================================
    // Input Handling
    //=========================================================================

    // Toggle ready state when SELECT pressed (disabled during countdown)
    if (keys & KEY_SELECT && !countdownActive) {
        int myID = Multiplayer_GetMyPlayerID();
        bool currentReady = Multiplayer_IsPlayerReady(myID);
        Multiplayer_SetReady(!currentReady);  // Toggle ready state
    }

    // Cancel and return to home page when B pressed (works any time)
    if (keys & KEY_B) {
        Multiplayer_Cleanup();  // Disconnect WiFi, close socket
        GameContext_SetMultiplayerMode(false);
        return HOME_PAGE;
    }

    //=========================================================================
    // Network Update
    //=========================================================================

    // Update lobby state - receives packets, checks timeouts, updates player info
    // Returns true if all connected players are ready (minimum 2 players required)
    bool allReady = Multiplayer_UpdateLobby();

    //=========================================================================
    // Display Lobby Status
    //=========================================================================

    // Clear console and redraw lobby UI
    consoleClear();
    printf("=== MULTIPLAYER LOBBY ===\n\n");

    // Get local player info
    int myID = Multiplayer_GetMyPlayerID();
    int connectedCount = 0;
    int readyCount = 0;

    // Display all connected players with ready status
    for (int i = 0; i < MAX_MULTIPLAYER_PLAYERS; i++) {
        if (Multiplayer_IsPlayerConnected(i)) {
            connectedCount++;
            bool ready = Multiplayer_IsPlayerReady(i);
            if (ready) {
                readyCount++;
            }

            // Show player number, ready status, and "(YOU)" marker for local player
            printf("Player %d: %s%s\n", i + 1, ready ? "[READY]   " : "[WAITING] ",
                   (i == myID) ? "(YOU)" : "");
        }
    }

    printf("\n(%d/%d ready)\n\n", readyCount, connectedCount);

    //=========================================================================
    // Debug Information Display
    //=========================================================================

    // Get high-level multiplayer stats (from multiplayer.c)
    int sent, received;
    Multiplayer_GetDebugStats(&sent, &received);

    // Get low-level socket stats (from WiFi_minilib.c)
    int recvCalls, recvSuccess, recvFiltered;
    getReceiveDebugStats(&recvCalls, &recvSuccess, &recvFiltered);

    // Display debug information
    printf("--------------------------------\n");
    printf("DEBUG: MyID=%d Connected=%d\n", myID, connectedCount);
    printf("AllReady=%d Countdown=%d\n", allReady ? 1 : 0, countdownActive ? 1 : 0);
    printf("Packets: Sent=%d Recv=%d\n", sent, received);
    printf("Socket: Calls=%d OK=%d Filt=%d\n", recvCalls, recvSuccess, recvFiltered);

    // Display network addressing
    unsigned long myIP = Wifi_GetIP();
    unsigned char macAddr[6];
    Wifi_GetData(WIFIGETDATA_MACADDRESS, 6, macAddr);
    printf("IP: %lu.%lu.%lu.%lu\n", (myIP & 0xFF), ((myIP >> 8) & 0xFF),
           ((myIP >> 16) & 0xFF), ((myIP >> 24) & 0xFF));
    printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", macAddr[0], macAddr[1], macAddr[2],
           macAddr[3], macAddr[4], macAddr[5]);

    //=========================================================================
    // Countdown Management
    //=========================================================================

    // Cancel countdown if conditions no longer met
    // (player unreadied, disconnected, or count dropped below 2)
    if (countdownActive && (!allReady || connectedCount < 2)) {
        countdownActive = false;
        countdownTimer = 0;
    }

    // Handle countdown state
    if (!countdownActive) {
        // Not in countdown - show instructions to user
        printf("Press SELECT when ready\n");
        printf("Press B to cancel\n");

        // Start countdown if all players are ready (minimum 2 players)
        if (allReady && connectedCount >= 2) {
            countdownActive = true;
            countdownTimer = 180;  // 3 seconds at 60Hz (3 * 60 = 180 frames)
        }
    } else {
        // Countdown active - show time remaining
        int secondsLeft = (countdownTimer / 60) + 1;  // Round up to nearest second

        printf("\nStarting in %d...\n", secondsLeft);

        // Decrement countdown timer
        countdownTimer--;

        // Countdown finished - transition to gameplay
        if (countdownTimer <= 0) {
            // Clear pending lobby ACK queues (prevents retransmits during race)
            Multiplayer_StartRace();

            // Set map (already set in Init, but ensure it's correct)
            GameContext_SetMap(ScorchingSands);

            return GAMEPLAY;
        }
    }

    // Stay in lobby
    return MULTIPLAYER_LOBBY;
}
