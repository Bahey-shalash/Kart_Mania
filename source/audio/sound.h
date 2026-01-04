/**
 * File: sound.h
 * --------------
 * Description: Sound system interface for the kart racing game. Provides
 *              functionality for loading, playing, and managing sound effects
 *              and background music. Supports enabling/disabling sound effects
 *              and music independently, as well as cleanup functions for
 *              different game states.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 04.01.2026
 */

#ifndef SOUND_HH
#define SOUND_HH

#include <stdbool.h>

#define MUSIC_VOLUME 256  // Default music volume (range 0...1024)

/**
 * Function: initSoundLibrary
 * --------------------------
 * Initializes the sound library system. Must be called before any other
 * sound functions are used.
 */
void initSoundLibrary(void);

//=============================================================================
// SOUND EFFECTS
//=============================================================================

/**
 * Function: LoadClickSoundFX
 * --------------------------
 * Loads the click sound effect into memory. Used for UI button clicks
 * and menu interactions.
 */
void LoadClickSoundFX(void);

/**
 * Function: UnloadClickSoundFX
 * ----------------------------
 * Unloads the click sound effect from memory to free resources.
 */
void UnloadClickSoundFX(void);

/**
 * Function: PlayCLICKSFX
 * ----------------------
 * Plays the click sound effect. The sound must be loaded first using
 * LoadClickSoundFX().
 */
void PlayCLICKSFX(void);

/**
 * Function: LoadDingSoundFX
 * -------------------------
 * Loads the ding sound effect into memory. Typically used for notifications
 * or success indicators.
 */
void LoadDingSoundFX(void);

/**
 * Function: UnloadDingSoundFX
 * ---------------------------
 * Unloads the ding sound effect from memory to free resources.
 */
void UnloadDingSoundFX(void);

/**
 * Function: PlayDingSFX
 * ---------------------
 * Plays the ding sound effect. The sound must be loaded first using
 * LoadDingSoundFX().
 */
void PlayDingSFX(void);

/**
 * Function: LoadBoxSoundFx
 * ------------------------
 * Loads the box sound effect into memory. Used when players interact
 * with item boxes in the game.
 */
void LoadBoxSoundFx(void);

/**
 * Function: UnloadBoxSoundFx
 * --------------------------
 * Unloads the box sound effect from memory to free resources.
 */
void UnloadBoxSoundFx(void);

/**
 * Function: PlayBoxSFX
 * --------------------
 * Plays the box sound effect. The sound must be loaded first using
 * LoadBoxSoundFx().
 */
void PlayBoxSFX(void);

/**
 * Function: cleanSound_home_page
 * ------------------------------
 * Cleans up and unloads sound effects specific to the home page screen.
 * Should be called when leaving the home page to free resources.
 */
void cleanSound_home_page(void);

/**
 * Function: cleanSound_settings
 * -----------------------------
 * Cleans up and unloads sound effects specific to the settings screen.
 * Should be called when leaving the settings screen to free resources.
 */
void cleanSound_settings(void);

/**
 * Function: cleanSound_MapSelection
 * ---------------------------------
 * Cleans up and unloads sound effects specific to the map selection screen.
 * Should be called when leaving the map selection screen to free resources.
 */
void cleanSound_MapSelection(void);

/**
 * Function: cleanSound_gamePlay
 * -----------------------------
 * Cleans up and unloads sound effects specific to the gameplay screen.
 * Should be called when leaving gameplay to free resources.
 */
void cleanSound_gamePlay(void);

/**
 * Function: LoadALLSoundFX
 * ------------------------
 * Loads all sound effects into memory at once. This is a convenience function
 * that loads all available sound effects (click, ding, box, etc.).
 */
void LoadALLSoundFX(void);

/**
 * Function: UnloadALLSoundFX
 * --------------------------
 * Unloads all sound effects from memory to free resources. This is a
 * convenience function that unloads all available sound effects at once.
 */
void UnloadALLSoundFX(void);

/**
 * Function: SOUNDFX_ON
 * --------------------
 * Enables sound effects playback. After calling this, sound effects will
 * play when their respective Play functions are called.
 */
void SOUNDFX_ON(void);

/**
 * Function: SOUNDFX_OFF
 * ---------------------
 * Disables sound effects playback. After calling this, sound effects will
 * not play even if their respective Play functions are called.
 */
void SOUNDFX_OFF(void);

//=============================================================================
// MUSIC
//=============================================================================

/**
 * Function: loadMUSIC
 * -------------------
 * Loads the background music into memory. Should be called during
 * initialization or when entering game states that require music.
 */
void loadMUSIC(void);

/**
 * Function: MusicSetEnabled
 * -------------------------
 * Enables or disables background music playback.
 *
 * Parameters:
 *   enabled - true to enable music playback, false to disable it
 */
void MusicSetEnabled(bool enabled);

#endif  // SOUND_HH
