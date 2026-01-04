/**
 * File: sound.c
 * --------------
 * Implementation of the sound system using the MaxMod9 audio library.
 * Manages sound effects (.wav files) and background music (.xm files)
 * located in the audio folder.
 *
 * Authors: Bahey Shalash, Hugo Svolgaard
 * Version: 1.0
 * Date: 04.01.2026
 */

#include "sound.h"

#include <maxmod9.h>

#include "../core/game_constants.h"
#include "soundbank.h"
#include "soundbank_bin.h"

void initSoundLibrary(void) {
    mmInitDefaultMem((mm_addr)soundbank_bin);
}

//=============================================================================
// SOUND EFFECTS
//=============================================================================

void LoadClickSoundFX(void) {
    mmLoadEffect(SFX_CLICK);
}

void UnloadClickSoundFX(void) {
    mmUnloadEffect(SFX_CLICK);
}

void PlayCLICKSFX(void) {
    mmEffect(SFX_CLICK);
}

void LoadDingSoundFX(void) {
    mmLoadEffect(SFX_DING);
}

void UnloadDingSoundFX(void) {
    mmUnloadEffect(SFX_DING);
}

void PlayDingSFX(void) {
    mmEffect(SFX_DING);
}

void LoadBoxSoundFx(void) {
    mmLoadEffect(SFX_BOX);
}

void UnloadBoxSoundFx(void) {
    mmUnloadEffect(SFX_BOX);
}

void PlayBoxSFX(void) {
    mmEffect(SFX_BOX);
}

// Screen-specific cleanup functions
void cleanSound_home_page(void) {
    UnloadClickSoundFX();  // Home page only uses click sound for menu navigation
}

void cleanSound_settings(void) {
    UnloadClickSoundFX();  // Click for back/home buttons
    UnloadDingSoundFX();   // Ding for toggling settings (wifi, music, sound fx)
}

void cleanSound_MapSelection(void) {
    UnloadClickSoundFX();  // Map selection only uses click sound
}

void LoadALLSoundFX(void) {
    LoadClickSoundFX();
    LoadDingSoundFX();
    LoadBoxSoundFx();
}

void UnloadALLSoundFX(void) {
    UnloadClickSoundFX();
    UnloadDingSoundFX();
    UnloadBoxSoundFx();
}

void SOUNDFX_ON(void) {
    mmSetEffectsVolume(VOLUME_MAX);
}

void SOUNDFX_OFF(void) {
    mmSetEffectsVolume(VOLUME_MUTE);
}

void cleanSound_gamePlay(void) {
    UnloadBoxSoundFx();  // Gameplay uses box sound for item pickups
}

//=============================================================================
// MUSIC
//=============================================================================

void loadMUSIC(void) {
    mmLoad(MOD_TROPICAL);
}

void MusicSetEnabled(bool enabled) {
    if (enabled) {
        mmStart(MOD_TROPICAL, MM_PLAY_LOOP);  // Start looping tropical theme
        mmSetModuleVolume(MUSIC_VOLUME);
    } else {
        mmStop();
    }
}
