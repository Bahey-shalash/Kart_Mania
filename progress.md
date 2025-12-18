# Progress


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