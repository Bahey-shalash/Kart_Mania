# Initialization System

## Overview

`InitGame()` performs all one-time startup work before the main loop begins. It brings up storage, loads user settings into the global context, primes audio, initializes the WiFi stack exactly once, and boots the initial game state. Called from `main()` before any frame processing ([main.c:20-22](../source/core/main.c#L20-L22)).

- Interface: [init.h:15-31](../source/core/init.h#L15-L31)
- Implementation: [init.c:85-98](../source/core/init.c#L85-L98)

## Initialization Order (and Why)

1. **Storage + Context** — `init_storage_and_context()` mounts FAT via `Storage_Init()`, seeds defaults with `GameContext_InitDefaults()`, then reloads saved preferences when storage is available ([init.c:32-42](../source/core/init.c#L32-L42)). Defaults live in [context.c:24-32](../source/core/context.c#L24-L32). Doing this first ensures later subsystems honor the player’s saved WiFi/music/SFX preferences.
2. **Audio System** — `init_audio_system()` brings up MaxMod (`initSoundLibrary()`), loads all SFX/music (`LoadALLSoundFX()`, `loadMUSIC()`), and applies saved toggles via `GameContext_SetMusicEnabled()` / `SOUNDFX_OFF()` ([init.c:51-65](../source/core/init.c#L51-L65)). See [sound.md](sound.md) for the audio pipeline.
3. **WiFi Stack (one-time!)** — `init_wifi_stack()` calls `Wifi_InitDefault(false)` exactly once ([init.c:68-79](../source/core/init.c#L68-L79)). The WiFi firmware must stay alive for the entire run; never reinitialize or disable it elsewhere (see `old/MULTIPLAYER_RECONNECTION_FIX.md` for the failure mode details).
4. **Initial Game State** — After settings are applied, the starting `GameState` from the context (default `HOME_PAGE`) is booted via `StateMachine_Init()` ([init.c:95-97](../source/core/init.c#L95-L97)). See [state_machine.md](state_machine.md) for lifecycle hooks.

## Usage Notes

- `InitGame()` is invoked once from `main()`; returning to `main` transfers control to the frame loop described in [main.md](main.md).
- WiFi enable/disable in settings only flips the user preference; the stack itself stays running so multiplayer can reconnect instantly (see [context.md](context.md) and `old/MULTIPLAYER_RECONNECTION_FIX.md`).
