#ifndef SOUND_HH
#define SOUND_HH

#include <maxmod9.h>
#include <nds.h>
#include <stdbool.h>

#include "soundbank.h"
#include "soundbank_bin.h"

void initSoundLibrary(void);

//=============================================================================
// SOUND EFFECTS
//=============================================================================
void LoadClickSoundFX(void);
void UnloadClickSoundFX(void);
void PlayCLICKSFX(void);

void LoadDingSoundFX(void);
void UnloadDingSoundFX(void);
void PlayDingSFX(void);

void LoadBoxSoundFx(void);
void UnloadBoxSoundFx(void);
void PlayBoxSFX(void);

void cleanSound_home_page(void);
void cleanSound_settings(void);
void cleanSound_MapSelection(void);
void cleanSound_gamePlay(void);

void LoadALLSoundFX(void);
void UnloadALLSoundFX(void);

void SOUNDFX_ON(void);
void SOUNDFX_OFF(void);

//=============================================================================
// MUSIC
//=============================================================================
#define MUSIC_VOLUME 256  //(range 0...1024)
void loadMUSIC(void);
void MusicSetEnabled(bool enabled);

#endif  // SOUND_HH