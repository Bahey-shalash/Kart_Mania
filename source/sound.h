#ifndef SOUND_HH
#define SOUND_HH

#include <maxmod9.h>
#include <nds.h>

#include "soundbank.h"
#include "soundbank_bin.h"

//=============================================================================
// SOUND EFFECTS
//=============================================================================
void initSoundLibrary(void);

void LoadClickSoundFX(void);
void UnloadClickSoundFX(void);

void PlayCLICKSFX(void);

void LoadDingSoundFX(void);
void UnloadDingSoundFX(void);
void PlayDingSFX(void);

void cleanSound_home_page(void);
void cleanSound_settings(void);
void cleanSound_singleplayer(void);

void LoadALLSoundFX(void);
void UnloadALLSoundFX(void);

void SOUNDFX_ON(void);
void SOUNDFX_OFF(void);


//=============================================================================
// MUSIC
//=============================================================================

#endif  // SOUND_HH