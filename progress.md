# Progress

## Bahey -------------------------

## Home Menu Image

- Made the image for the home menu  
  **Path:** `/figures/homepage_second_screen/ds_menu.png`

### GRIT macOS Workaround

There is a problem with GRIT on macOS, so I made a Python file to fix it:

**Path:** `R_B_IMAGE_CONVERTER/convert.py`

**Usage:**
```bash
python3 convert.py input.png output.png
```

The script swaps the red and blue channels, allowing the image to be used normally in `data/ds_menu.png` and converted with GRIT.
Transparent areas may appear magenta, which is expected and handled correctly by GRIT when generating the `.grit` assets.
---

## Project Structure

Used the template project structure.

**Added:**
- `source/home_page.h`
- `source/home_page.c`

which will include all of the related stuff for the homescreen

- `source/settings.h`
- `source/settings.c`

which will include all of the related controlls for the settings.



You can see in them there is only a config for the sub engine.

---

## Sub Engine Configuration for homescreen

I used for the home menu:

- SUB engine in mode 5 with background 2 active
- VRAM C
- `BG_BMP_BASE(0)` (VRAM C)
- `dmaCopy` rather than `swiCopy` because it's faster

In the `.grit` I used a config to generate the bitmap and the corresponding palette (8-bit pixels).

## Main Engine Configuration for homescreen

- Main engine in mode 5 with background 2 active
- VRAM A
- `BG_BMP_BASE(0)` (VRAM A)
- `dmaCopy` rather than `swiCopy` because it's faster


## sprite configuration for homescreen Kart

- Vram B 
- Sprite engine uses 1D mapping (32-byte boundaries)
- Sprite size: 64×64
- Color format: 256-color (8-bit indexed)

sprite logic in `home_page.h/home_page.c` and the new types are in `game_types.h`


## ----------------------------------------------------------------------------

## Hugo ----------------------------------------------------------------------
## Button Sprite System

### Graphics Layout

- Button size: `64 × 32` pixels  
- Frames per button: `3`  
- Sprite sheet size: `64 × 96` pixels (stacked vertically)

**Frame order:**
- Frame 0 (0–31px): Normal
- Frame 1 (32–63px): Highlighted
- Frame 2 (64–95px): Pressed

Each frame uses:  
`64 × 32 × 8bpp = 2048 bytes`

---

### VRAM Management

```c
u16 *single_player_gfx[3];
u16 *multi_player_gfx[3];
u16 *settings_gfx[3];
```

- One VRAM allocation per frame  
- Memory allocated using `oamAllocateGfx()`  
- Graphics copied using `swiCopy()` in 2048-byte blocks  
---

### Button Size Rationale

- DS resolution: 256 × 192  
- 64px width allows three centered buttons horizontally  
- 32px height allows three buttons vertically with spacing  
- Matches supported DS sprite sizes  
- Large enough for touch input  

---

## Input System

### Unified Input Handler

```c
void HomePage_handleInput(void)
{
    // Reset pressed states
    // Handle D-pad input
    // Handle touch input
}
```

**Design choices:**
- One input function called per frame  
- Touch and D-pad modify the same state  
- Pressed state only active while input is held  
- Keeps the main loop simple  

---

### Touch Detection

- Button positions are fixed  
- Touch bounds are hardcoded  
- X range checked first  
- Y range checked only if X is valid  
- This avoids unnecessary calculations  

---

## Code Organization

### Initialization Structure

- `configGraphics_Sub()` – Display and VRAM setup  
- `configBackground_Sub()` – Background loading  
- `configSprites_Sub()` – Sprite allocation and graphics loading  

**Reason:**
- Separation of concerns  
- Easier debugging  
- Clear initialization order  

---

## Button Sprite Generator Tool

A web-based tool was created to generate button sprites.

**Features:**
- Fixed `64×96` output  
- Bitmap font rendering  
- Automatic generation of three frames  
- Magenta (`#FF00FF`) background for GRIT transparency  

---

## Background Generator Tool

A separate web tool generates backgrounds.

**Details:**
- Resolution: `256 × 192`  
- Styles: gradient, checkered, racing stripes, grid  
- Single PNG output  

**Reason:**
- Different requirements than buttons  
- No animation frames needed  

---

## GRIT Conversion

### Button Sprites

```bash
grit button.png -g -gB8 -gTFF00FF -gt
```
- 8-bit graphics  
- Magenta transparency  

### Backgrounds

```bash
grit ds_menu.png -g -gB8 -gb -p
```
- bitmap and palette
---

## Main Loop

```c
HomePage_initialize();

while (true) {
    scanKeys();
    HomePage_handleInput();
    HomePage_updateMenu();
    swiWaitForVBlank();
}
```

**Execution order:**
1. Read input  
2. Process input  
3. Update sprites  
4. Wait for VBlank  

---

## Resource Cleanup

```c
void HomePage_cleanup(void)
{
    for (int i = 0; i < 3; i++) {
        oamFreeGfx(&oamSub, single_player_gfx[i]);
        // free other button graphics
    }

    oamClear(&oamSub, 0, 128);
    oamUpdate(&oamSub);
}
```

**Reason:**
- Prevents VRAM leaks  
- DS sprite VRAM is limited  
- Required when changing screens  

## Files

### New Files
- `source/home_page.h`  
- `source/home_page.c`  
- `data/button_single_player.png`  
- `data/button_multi_player.png`  
- `data/button_settings.png`  
- Button Sprite Generator (HTML/JS)  
- Background Generator (HTML/JS)  

### Modified Files
- `source/main.c`
- `source/homepage.c`
- `source/homepage.h`

## Code audit (current implementation)

### Runtime loop
- `main.c` initializes `GameContext` (wifi/music/sfx default to true, state starts at `HOME_PAGE`), sets up maxmod (`initSoundLibrary`, `LoadALLSoundFX`, `loadMUSIC`), enables music per settings, then runs `init_state` for the current state.
- Per-frame loop calls `update_state`; on a state change it updates `ctx->currentGameState`, calls `video_nuke` to clear VRAM/palettes/OAM/regs, then re-runs `init_state` for the new screen, with a `swiWaitForVBlank` each frame.
- `update_state` dispatches to `HomePage_update`, `Settings_update`, or `Singleplayer_update`; `MULTIPLAYER` currently routes to `HomePage_update` as a placeholder.

### Shared context & audio
- `GameContext` holds user toggles and the active `GameState`; setters immediately trigger side effects (`MusicSetEnabled`, SFX volume on/off, wifi stub).
- SFX: click/ding loaded via `LoadALLSoundFX` and played with `PlayCLICKSFX`/`PlayDingSFX`; volume toggled with `SOUNDFX_ON/OFF`.
- Music: module `tropical.xm` loads via `loadMUSIC`; `MusicSetEnabled` starts/stops looping playback at `MUSIC_VOLUME` (256).

### Home page
- Top screen: MODE_5_2D, BG2 bitmap from `data/home_top.png` in VRAM A. A 64×64 256-color kart sprite on VRAM B uses `kart_home` tiles/palette; starts at x = -64, y = 120 and moves +1 px per VBlank (wraps to -64 after x > 256) via a VBlank IRQ (`initTimer`/`timerISRVblank`).
- Bottom screen: MODE_0_2D on VRAM C with BG0 (`ds_menu` tiles/map/palette at map base 0, tile base 1) and BG1 highlight layer (map base 1, tile base 2). Highlight palette slots 251–253 start black; rectangles drawn at x = 6..25 and y rows 4/10/17 using tiles 1–3.
- Menu hitboxes (all three rows): x = 32, width = 192, height = 40; y = 24 + 54·row. D-pad cycles selection (mod 3); touch selects by hitbox. On release (A or touch), plays click; returns `SINGLEPLAYER`, `MULTIPLAYER` (placeholder), or `SETTINGS` per selection.

### Settings page
- Top screen: MODE_5_2D BG2 bitmap from `data/settings_top.png` on VRAM A.
- Bottom screen: MODE_0_2D on VRAM C. BG0 uses `nds_settings` tiles/map/palette (map base 0, tile base 1). BG1 overlay at map base 1, tile base 2 holds toggle pills and selection tiles. Palette indices 254/255 are red/green; selection highlight palette indices 244–249 start black.
- Toggle pills drawn at cols 21–29; wifi rows 1–4, music 5–8, sound fx 9–12 using tile 3 (red) or 4 (green) based on `GameContext`.
- Selection rectangles: wifi (x 2–7, y 1–4), music (x 2–9, y 5–8), sound fx (x 2–13, y 9–12), save (x 4–14, y 15–23), back (x 12–20, y 15–23), home (x 20–28, y 15–23); palette slot = 244 + button index when active.
- Touch bounds: wifi text (px 24–52, py 11–24) or pill (px 176–239, py 11–36); music text (px 25–68, py 41–54) or pill (px 176–239, py 41–66); sound fx text (px 24–98, py 71–84) or pill (px 176–239, py 71–96); save/back/home circles roughly px 41–87/105–151/169–215 with py 129–175.
- D-pad up/down cycles; left/right wraps bottom buttons (save/back/home). On release (A or touch): wifi/music/sfx toggle context + redraw pill (+ ding, click before muting for sfx), save calls stub `onSavePressed` + ding, back/home play click and return `HOME_PAGE`.

### Singleplayer map select
- Top screen: MODE_5_2D BG2 bitmap from `data/map_top.png` on VRAM A.
- Bottom screen: MODE_0_2D on VRAM C with BG0 (`map_bottom` tiles/map/palette at map base 0, tile base 1) and BG1 highlights (map base 1, tile base 3). Palette indices 240–243 are used for selection tint (start black); tiles loaded at indices 0–3.
- Selection rectangles: Map1 x 2–11, y 9–20; Map2 x 11–20, y 9–20; Map3 x 20–29, y 9–20; Home x 28–31, y 20–23. Active selection sets palette slot 240 + button to `SP_SELECT_COLOR`, otherwise black.
- Touch bounds: Map1 px 20–80, py 70–165; Map2 98–158, 70–165; Map3 176–236, 70–165; Home 224–251, 161–188. D-pad up/down cycles; left/right steps through map1→map2→map3→home (with wrap). On release (A or touch), Map buttons play click (map loading TODO), Home plays click and returns `HOME_PAGE`.

### Graphics reset
- `video_nuke` blanks both displays, clears OAM, palettes, and VRAM banks A/B/C, resets BG control/offsets/affine registers to known defaults before entering a new state.

### Assets/build notes
- PNGs in `data/` have matching `.grit` configs; Makefile only builds PNGs with `.grit` siblings. Special-case palette tweak for `ds_menu` in the grit rule (`sed -i '' 's/\.hword 0x7C1F/\.hword 0x7BBD/' ds_menu.s`).
- Audio assets live in `audio/` (`CLICK.wav`, `DING.wav`, `tropical.xm`); `mmutil` builds `soundbank`/`soundbank_bin` during the make.
