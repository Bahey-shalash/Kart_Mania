#!/usr/bin/env python3
"""
Swap Red and Blue channels (R<->B) for a PNG.

Produces TWO outputs:
  1) <base>_swap.png  : swapped, alpha preserved (RGBA)
  2) <base>_grit.png  : swapped, alpha removed, transparent pixels filled with FF00FF (RGB)
                        -> intended ONLY as input to grit with -gTFF00FF

Usage:
  python swap_rb.py input.png
"""

import os
import sys
from PIL import Image

KEY_RGB = (255, 0, 255)  # FF00FF magenta key

def swap_rb_rgba(img: Image.Image) -> Image.Image:
    """Return RGBA image with R and B swapped, alpha preserved."""
    rgba = img.convert("RGBA")
    r, g, b, a = rgba.split()
    return Image.merge("RGBA", (b, g, r, a))

def make_grit_rgb(swapped_rgba: Image.Image, key_rgb=KEY_RGB) -> Image.Image:
    """
    Flatten swapped RGBA onto a solid key color background and drop alpha,
    yielding an RGB image suitable for DS color-key transparency via grit -gT.
    """
    # Solid magenta background (opaque)
    bg = Image.new("RGBA", swapped_rgba.size, key_rgb + (255,))
    # Alpha composite places sprite over magenta wherever alpha>0
    composited = Image.alpha_composite(bg, swapped_rgba)
    # Drop alpha entirely (DS/grit color-key workflow)
    return composited.convert("RGB")

def main():
    if len(sys.argv) != 2:
        print("Usage: python swap_rb.py input.png", file=sys.stderr)
        sys.exit(2)

    in_path = sys.argv[1]
    base, _ = os.path.splitext(in_path)
    out_swap = base + "_swap.png"
    out_grit = base + "_grit.png"

    with Image.open(in_path) as img:
        swapped = swap_rb_rgba(img)
        grit_ready = make_grit_rgb(swapped)

        swapped.save(out_swap, format="PNG")
        grit_ready.save(out_grit, format="PNG")

    print("Wrote:")
    print(" ", out_swap, "(swapped, alpha preserved)")
    print(" ", out_grit, "(swapped, alpha removed, magenta key for grit)")

if __name__ == "__main__":
    main()