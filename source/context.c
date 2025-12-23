#include "context.h"

#include "sound.h"

static GameContext gGameContext;  // actual game context

GameContext* GameContext_Get(void) {
    return &gGameContext;
}
// should make const to stop things like GameContext_Get()->userSettings.musicEnabled =
// false

void GameContext_InitDefaults(void) {
    gGameContext.userSettings.wifiEnabled = true;
    gGameContext.userSettings.musicEnabled = true;
    gGameContext.userSettings.soundFxEnabled = true;

    gGameContext.currentGameState = HOME_PAGE;
}

void GameContext_SetMusicEnabled(bool enabled) {
    gGameContext.userSettings.musicEnabled = enabled;
    MusicSetEnabled(enabled);  // side-effect lives here
}

void GameContext_SetSoundFxEnabled(bool enabled) {
    gGameContext.userSettings.soundFxEnabled = enabled;
    if (enabled)
        SOUNDFX_ON();
    else
        SOUNDFX_OFF();
}

void GameContext_SetWifiEnabled(bool enabled) {
    gGameContext.userSettings.wifiEnabled = enabled;
    // later: actually enable/disable wifi here
}