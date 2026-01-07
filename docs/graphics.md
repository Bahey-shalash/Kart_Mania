# Graphics Utilities

## Overview

Graphics helpers centralize “clean slate” setup between screens and common palette colors. Everything lives in:
- `source/graphics/graphics.c` / `graphics.h` — `video_nuke()` hard-resets displays, OAM, VRAM, palettes, and BG registers to avoid artifacts between states.
- `source/graphics/color.h` — shared ARGB15 color constants for UI highlights, toggles, and menu accents.

Use these helpers during state transitions and when keeping color usage consistent across UI/gameplay code.

## video_nuke()

**Signature:** `void video_nuke(void)`  
**Defined in:** [graphics.c:1-67](../source/graphics/graphics.c#L1-L67)  
**Purpose:** Defensive reset before a new screen/state initializes.

What it clears:
- Disables both displays (`REG_DISPCNT`, `REG_DISPCNT_SUB`) to prevent visible garbage during swaps.
- Clears sprites and resets OAM allocators on main/sub to avoid leaks.
- Zeroes BG and sprite palettes for both engines.
- Maps VRAM banks (A: main BG, B: main SPR, C: sub BG, D: sub SPR) and wipes them to a known state.
- Resets BG control registers, scroll offsets, and affine transforms (main + sub) to identity/zero.

Usage:
- Called from the main loop during state transitions right after cleanup and before initializing the next state (see `StateMachine_Cleanup()` and `StateMachine_Init()` usage in [main.md](main.md)).
- Keeps state-specific graphics code simple by guaranteeing a clean baseline.

## Color Constants

**Defined in:** [color.h](../source/graphics/color.h)  
**Format:** `RGB15` values for Nintendo DS palettes.

Key constants:
- `BLACK`, `WHITE`, `RED`, `GREEN`, `BLUE`, `YELLOW`
- UI highlights: `MENU_BUTTON_HIGHLIGHT_COLOR`, `MENU_HIGHLIGHT_OFF_COLOR`
- Toggles: `TOGGLE_ON_COLOR`, `TOGGLE_OFF_COLOR`
- Selection tints: `SETTINGS_SELECT_COLOR`, `SP_SELECT_COLOR`
- Utility shades: `DARK_GREEN` (new record), `DARK_GRAY` (neutral background), `TEAL` (blue-ish highlight)

Usage patterns:
- Prefer these names instead of raw `RGB15`/`ARGB16` literals in UI/gameplay to keep palettes consistent and readable (see `gameplay.c` and `play_again.c` after cleanup).
- Add new colors here when introducing new UI themes to keep palette choices centralized.



---

## Navigation

- [← Back to Wiki](wiki.md)
- [← Back to README](../README.md)