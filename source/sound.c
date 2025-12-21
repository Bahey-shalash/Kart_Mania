#include "sound.h"

void initSoundLibrary(void) {
    // Init the sound library
    mmInitDefaultMem((mm_addr)soundbank_bin);
}

void LoadClickSoundFX(void) {
    // Load the click sound effect
    mmLoadEffect(SFX_CLICK);
}

void UnloadClickSoundFX(void) {
    // Unload the click sound effect
    mmUnloadEffect(SFX_CLICK);
}

void PlayCLICKSFX(void) {
    // Play the click sound
    mmEffect(SFX_CLICK);
}

void LoadDingSoundFX(void) {
    // Load the ding sound effect
    mmLoadEffect(SFX_DING);
}
void UnloadDingSoundFX(void) {
    // Unload the ding sound effect
    mmUnloadEffect(SFX_DING);
}

void PlayDingSFX(void) {
    // Play the ding sound
    mmEffect(SFX_DING);
}
void cleanSound_home_page(void) {
    UnloadClickSoundFX();
}
void cleanSound_settings(void) {
    UnloadClickSoundFX();
    UnloadDingSoundFX();
}

void cleanSound_singleplayer(void) {
    UnloadClickSoundFX();
}

void LoadALLSoundFX(void) {
    LoadClickSoundFX();
    LoadDingSoundFX();
}

void UnloadALLSoundFX(void) {
    UnloadClickSoundFX();
    UnloadDingSoundFX();
}