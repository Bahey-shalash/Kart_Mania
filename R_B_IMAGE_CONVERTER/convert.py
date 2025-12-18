#!/usr/bin/env python3
"""
Swap Red and Blue channels (R<->B) in a PNG while keeping:
- same dimensions
- same mode (RGB/RGBA/etc.)
- alpha unchanged (if present)
- PNG output

Usage:
  python swap_rb.py input.png output.png
"""

import sys
from PIL import Image

def swap_rb(img: Image.Image) -> Image.Image:
    # Ensure we can access channels reliably while preserving alpha if present
    mode = img.mode

    if mode == "RGB":
        r, g, b = img.split()
        return Image.merge("RGB", (b, g, r))

    if mode == "RGBA":
        r, g, b, a = img.split()
        return Image.merge("RGBA", (b, g, r, a))

    # Palette or other modes: convert to RGBA, swap, then convert back if possible
    # (This may not preserve palette indexing, but preserves visual output.)
    if mode in ("P", "LA", "L", "1", "CMYK", "YCbCr", "I", "F"):
        tmp = img.convert("RGBA")
        r, g, b, a = tmp.split()
        swapped = Image.merge("RGBA", (b, g, r, a))
        # Try to return to original mode if it makes sense
        try:
            return swapped.convert(mode)
        except Exception:
            return swapped

    # Fallback: try via RGBA
    tmp = img.convert("RGBA")
    r, g, b, a = tmp.split()
    return Image.merge("RGBA", (b, g, r, a))

def main():
    if len(sys.argv) != 3:
        print("Usage: python swap_rb.py input.png output.png", file=sys.stderr)
        sys.exit(2)

    in_path, out_path = sys.argv[1], sys.argv[2]

    with Image.open(in_path) as img:
        swapped = swap_rb(img)
        # Save as PNG; size/dimensions stay the same. Alpha preserved if present.
        swapped.save(out_path, format="PNG")

if __name__ == "__main__":
    main()