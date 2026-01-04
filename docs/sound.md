# Sound System Documentation

## Overview

The sound system for Kart Mania uses the **MaxMod** audio library to handle audio playback on the Nintendo DS. The implementation is split across two files:
- `source/audio/sound.h` - Interface definitions
- `source/audio/sound.c` - Implementation using MaxMod API

Audio assets (`.wav` for sound effects, `.xm` for music) are stored in the `audio/` folder and compiled into a soundbank during the build process.

---

## Build Process

### Audio Compilation with mmutil

The Makefile uses **mmutil** (MaxMod utility) to generate soundbank files from audio assets:

```makefile
soundbank.bin : $(AUDIOFILES)
	@mmutil $^ -osoundbank.bin -hsoundbank.h -d
```

**What happens:**
1. `mmutil` scans all files in the `audio/` folder
2. Generates `soundbank.bin` (binary audio data)
3. Generates `soundbank.h` (sound effect and music IDs as `#define` constants)
4. Generates `soundbank_bin.h` (binary data accessor)

**Generated Files** (in `build/`):
- `soundbank.h` - Contains `SFX_*` and `MOD_*` constants for referencing sounds
- `soundbank.bin` - Compiled audio data
- `soundbank_bin.h` - Provides access to the binary data in memory

**Example** from `soundbank.h`:
```c
#define SFX_BOX     0
#define SFX_CLICK   1
#define SFX_DING    2
#define MOD_TROPICAL 0
```

---

## Library: MaxMod

MaxMod is a module/sound effect player library for the Nintendo DS. Key features:
- Plays tracker music formats (`.xm`, `.mod`, `.it`, `.s3m`)
- Plays sound effects (`.wav`)
- Hardware mixing and streaming
- Volume control for music and effects independently

**Library file:** `libmm9` (linked via `-lmm9` in Makefile)

**Official Documentation:** See MaxMod library docs in devkitPro for API details.

---

## Architecture

### Initialization

```c
void initSoundLibrary(void)
```
**Purpose:** Initializes MaxMod with the compiled soundbank.

**Implementation:** Calls `mmInitDefaultMem()` with the soundbank binary data.

**When to call:** Once at application startup, before any audio functions.

**See:** [sound.c:21-23](../source/audio/sound.c#L21-L23)

---

## Sound Effects

Sound effects are loaded into memory on-demand and played using MaxMod's effect system.

### Current Sound Effects

| Sound Effect | File | Usage |
|--------------|------|-------|
| `SFX_CLICK` | `CLICK.wav` | UI button clicks, menu navigation |
| `SFX_DING` | `DING.wav` | Settings toggles|
| `SFX_BOX` | `BOX.wav` | Item box pickups during gameplay |

### Load/Unload Pattern

Each sound effect follows this pattern:

```c
void Load<Name>SoundFX(void)    // Loads into memory
void Unload<Name>SoundFX(void)  // Frees from memory
void Play<Name>SFX(void)        // Plays the sound
```

**Why load/unload?** Nintendo DS has limited memory (~4MB). Loading only needed sounds per screen optimizes memory usage.

### Function Reference

#### LoadClickSoundFX / UnloadClickSoundFX / PlayCLICKSFX
```c
void LoadClickSoundFX(void);
void UnloadClickSoundFX(void);
void PlayCLICKSFX(void);
```
**Description:** Manages the click sound effect for UI interactions.

**Implementation:** Calls `mmLoadEffect(SFX_CLICK)`, `mmUnloadEffect(SFX_CLICK)`, `mmEffect(SFX_CLICK)`.

**See:** [sound.c:29-39](../source/audio/sound.c#L29-L39)

---

#### LoadDingSoundFX / UnloadDingSoundFX / PlayDingSFX
```c
void LoadDingSoundFX(void);
void UnloadDingSoundFX(void);
void PlayDingSFX(void);
```
**Description:** Manages the ding sound effect for toggles and confirmations.

**See:** [sound.c:41-51](../source/audio/sound.c#L41-L51)

---

#### LoadBoxSoundFx / UnloadBoxSoundFx / PlayBoxSFX
```c
void LoadBoxSoundFx(void);
void UnloadBoxSoundFx(void);
void PlayBoxSFX(void);
```
**Description:** Manages the box sound effect for item pickups during races.

**See:** [sound.c:53-63](../source/audio/sound.c#L53-L63)

---

### Batch Operations

#### LoadALLSoundFX / UnloadALLSoundFX
```c
void LoadALLSoundFX(void);
void UnloadALLSoundFX(void);
```
**Description:** Convenience functions to load/unload all sound effects at once.

**Implementation:** Calls individual Load/Unload functions for click, ding, and box sounds.

**Use case:** Loading all sounds upfront or cleanup on application exit.

**See:** [sound.c:79-89](../source/audio/sound.c#L79-L89)

---

### Screen-Specific Cleanup

These functions unload sounds when transitioning between game screens to free memory.

#### cleanSound_home_page
```c
void cleanSound_home_page(void);
```
**Unloads:** `SFX_CLICK`

**Reason:** Home page only uses click sound for menu navigation (singleplayer, multiplayer, settings buttons).

**See:** [sound.c:66-68](../source/audio/sound.c#L66-L68)

---

#### cleanSound_settings
```c
void cleanSound_settings(void);
```
**Unloads:** `SFX_CLICK`, `SFX_DING`

**Reason:** Settings screen uses click (back/home buttons) and ding (toggling wifi, music, sound fx options).

**See:** [sound.c:70-73](../source/audio/sound.c#L70-L73)

---

#### cleanSound_MapSelection
```c
void cleanSound_MapSelection(void);
```
**Unloads:** `SFX_CLICK`

**Reason:** Map selection only uses click sound for choosing maps.

**See:** [sound.c:75-77](../source/audio/sound.c#L75-L77)

---

#### cleanSound_gamePlay
```c
void cleanSound_gamePlay(void);
```
**Unloads:** `SFX_BOX`

**Reason:** Gameplay uses box sound for item pickups.

**See:** [sound.c:99-101](../source/audio/sound.c#L99-L101)

---

### Volume Control

#### SOUNDFX_ON / SOUNDFX_OFF
```c
void SOUNDFX_ON(void);
void SOUNDFX_OFF(void);
```
**Description:** Globally enable or disable sound effects playback.

**Implementation:**
- `SOUNDFX_ON` sets volume to `VOLUME_MAX` via `mmSetEffectsVolume()`
- `SOUNDFX_OFF` sets volume to `VOLUME_MUTE` via `mmSetEffectsVolume()`

**Note:** Sounds must still be loaded; this only controls volume, not loading state.

**See:** [sound.c:91-97](../source/audio/sound.c#L91-L97)

---

## Background Music

### Current Music Tracks

| Track | File | Usage |
|-------|------|-------|
| `MOD_TROPICAL` | `tropical.xm` | Main background music |

### Music Functions

#### loadMUSIC
```c
void loadMUSIC(void);
```
**Description:** Loads the tropical theme module into memory.

**Implementation:** Calls `mmLoad(MOD_TROPICAL)`.

**When to call:** During initialization or when entering a game state that requires music.

**See:** [sound.c:92-94](../source/audio/sound.c#L92-L94)

---

#### MusicSetEnabled
```c
void MusicSetEnabled(bool enabled);
```
**Description:** Starts or stops background music playback.

**Parameters:**
- `enabled` - `true` to start music, `false` to stop

**Implementation:**
- If enabled: Calls `mmStart(MOD_TROPICAL, MM_PLAY_LOOP)` to loop the track, then sets volume to `MUSIC_VOLUME` (256/1024)
- If disabled: Calls `mmStop()` to halt playback

**Note:** Music must be loaded with `loadMUSIC()` first.

**See:** [sound.c:96-103](../source/audio/sound.c#L96-L103)

---

## Constants

### MUSIC_VOLUME
```c
#define MUSIC_VOLUME 256  // Range 0...1024
```
**Description:** Default music volume level (25% of maximum).

**See:** [sound.h:20](../source/audio/sound.h#L20)

---

## Usage Patterns

### Example: Home Page Screen
```c
// On screen enter
LoadClickSoundFX();

// User clicks button
PlayCLICKSFX();

// On screen exit
cleanSound_home_page();
```

### Example: Settings Screen
```c
// On screen enter
LoadClickSoundFX();
LoadDingSoundFX();

// User toggles setting
PlayDingSFX();

// User clicks back
PlayCLICKSFX();

// On screen exit
cleanSound_settings();
```

### Example: Gameplay
```c
// On game start
LoadBoxSoundFx();
loadMUSIC();
MusicSetEnabled(true);

// Player hits item box
PlayBoxSFX();

// On game end
cleanSound_gamePlay();
MusicSetEnabled(false);
```

---

## Adding New Sounds

To add a new sound effect:

1. **Add audio file** to `audio/` folder (`.wav` for SFX, `.xm` for music)
2. **Rebuild** - Makefile will regenerate `soundbank.h` with new `#define`
3. **Add functions** to `sound.h` and `sound.c`:
   ```c
   void LoadNewSoundFX(void);
   void UnloadNewSoundFX(void);
   void PlayNewSFX(void);
   ```
4. **Implement** using `mmLoadEffect(SFX_NEW)`, `mmUnloadEffect(SFX_NEW)`, `mmEffect(SFX_NEW)`
5. **Update cleanup functions** for screens that use the new sound

---

## MaxMod API Reference

Key functions used from MaxMod:

| Function | Purpose |
|----------|---------|
| `mmInitDefaultMem(addr)` | Initialize MaxMod with soundbank data |
| `mmLoadEffect(id)` | Load sound effect into memory |
| `mmUnloadEffect(id)` | Unload sound effect from memory |
| `mmEffect(id)` | Play a loaded sound effect |
| `mmLoad(id)` | Load music module |
| `mmStart(id, mode)` | Start playing music (with loop mode) |
| `mmStop()` | Stop music playback |
| `mmSetEffectsVolume(vol)` | Set sound effects volume (0-1024) |
| `mmSetModuleVolume(vol)` | Set music volume (0-1024) |

For complete MaxMod documentation, see devkitPro's MaxMod9 library reference.
