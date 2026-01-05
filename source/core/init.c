/**
 * File: init.c
 * ------------
 * Description: Implementation of application initialization. Sets up all subsystems
 *              in the correct order with proper error handling and WiFi management.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 04.01.2026
 */

#include "init.h"

#include <nds.h>
#include <dswifi9.h>

#include "../audio/sound.h"
#include "../storage/storage.h"
#include "context.h"
#include "state_machine.h"

//=============================================================================
// PRIVATE INITIALIZATION HELPERS
//=============================================================================

/**
 * Function: init_storage_and_context
 * -----------------------------------
 * Initializes storage system and game context. Loads saved settings from
 * storage if available, otherwise uses hardcoded defaults.
 */
static void init_storage_and_context(void) {
    // Initialize FAT filesystem first (required for loading saved settings)
    bool storageAvailable = Storage_Init();

    // Initialize context with hardcoded defaults (WiFi on, music on, etc.)
    GameContext_InitDefaults();

    // If SD card available, load saved settings to overwrite defaults
    if (storageAvailable) {
        Storage_LoadSettings();
    }
}

/**
 * Function: init_audio_system
 * ---------------------------
 * Initializes audio library and applies saved audio settings from context.
 * Loads soundbank and music, then applies user preferences for music/SFX.
 */
static void init_audio_system(void) {
    const GameContext* ctx = GameContext_Get();

    initSoundLibrary();  // Initialize MaxMod with soundbank
    LoadALLSoundFX();    // Load all sound effects into memory
    loadMUSIC();         // Load background music module

    // Apply music setting (starts/stops playback based on saved preference)
    GameContext_SetMusicEnabled(ctx->userSettings.musicEnabled);

    // Apply sound effects setting (mute if disabled in settings)
    if (!ctx->userSettings.soundFxEnabled) {
        SOUNDFX_OFF();
    }
}

/**
 * Function: init_wifi_stack
 * -------------------------
 * Initializes WiFi stack ONCE at program start. This is critical for
 * multiplayer reconnection - the stack must never be re-initialized or
 * disabled during the program lifetime.
 *
 * IMPORTANT: DO NOT call Wifi_InitDefault() anywhere else in the code!
 * See wifi.md for details on why re-initialization breaks multiplayer.
 */
static void init_wifi_stack(void) {
    Wifi_InitDefault(false);
}

//=============================================================================
// PUBLIC INITIALIZATION
//=============================================================================

void InitGame(void) {
    // 1. Initialize storage and load settings
    init_storage_and_context();

    // 2. Initialize audio system
    init_audio_system();

    // 3. Initialize WiFi stack (CRITICAL - only once!)
    init_wifi_stack();

    // 4. Initialize starting game state (HOME_PAGE)
    GameContext* ctx = GameContext_Get();
    StateMachine_Init(ctx->currentGameState);
}
