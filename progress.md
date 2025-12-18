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


