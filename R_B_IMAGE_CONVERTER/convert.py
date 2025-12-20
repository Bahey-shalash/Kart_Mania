#!/usr/bin/env python3
import os, sys
from PIL import Image

KEY_RGB = (255, 0, 255)

# Pixels with alpha >= KEEP_ALPHA are considered "real sprite"
KEEP_ALPHA = 1          # 1 keeps any nonzero alpha as sprite
# Optional: erode the sprite mask by N pixels to remove fringes
ERODE = 0               # try 1 if you see magenta edge noise
# Optional: dilate the sprite mask by N pixels to avoid cutting edges
DILATE = 0              # usually 0

def swap_rb_rgba(img: Image.Image) -> Image.Image:
    rgba = img.convert("RGBA")
    r, g, b, a = rgba.split()
    return Image.merge("RGBA", (b, g, r, a))

def erode_mask(mask: Image.Image, n: int) -> Image.Image:
    # mask is mode "1" or "L" with 0/255 values
    if n <= 0: return mask
    m = mask.convert("L")
    w, h = m.size
    src = m.load()
    out = Image.new("L", (w, h), 0)
    dst = out.load()
    for y in range(h):
        for x in range(w):
            ok = 255
            for dy in range(-n, n+1):
                yy = y + dy
                if yy < 0 or yy >= h: ok = 0; break
                for dx in range(-n, n+1):
                    xx = x + dx
                    if xx < 0 or xx >= w or src[xx, yy] == 0:
                        ok = 0
                        break
                if ok == 0: break
            dst[x, y] = ok
    return out

def dilate_mask(mask: Image.Image, n: int) -> Image.Image:
    if n <= 0: return mask
    m = mask.convert("L")
    w, h = m.size
    src = m.load()
    out = Image.new("L", (w, h), 0)
    dst = out.load()
    for y in range(h):
        for x in range(w):
            v = 0
            for dy in range(-n, n+1):
                yy = y + dy
                if yy < 0 or yy >= h: continue
                for dx in range(-n, n+1):
                    xx = x + dx
                    if xx < 0 or xx >= w: continue
                    if src[xx, yy] != 0:
                        v = 255
                        break
                if v: break
            dst[x, y] = v
    return out

def to_grit_key(img_rgba: Image.Image) -> Image.Image:
    rgba = img_rgba.convert("RGBA")
    r, g, b, a = rgba.split()

    # Build sprite mask from alpha: keep where alpha >= KEEP_ALPHA
    mask = a.point(lambda v: 255 if v >= KEEP_ALPHA else 0).convert("L")

    # Optional morphology to kill fringes
    mask = erode_mask(mask, ERODE)
    mask = dilate_mask(mask, DILATE)

    w, h = rgba.size
    src = rgba.load()

    out = Image.new("RGB", (w, h), KEY_RGB)
    dst = out.load()
    mpx = mask.load()

    for y in range(h):
        for x in range(w):
            if mpx[x, y] != 0:
                rr, gg, bb, _ = src[x, y]
                dst[x, y] = (rr, gg, bb)
            else:
                dst[x, y] = KEY_RGB  # force EXACT FF00FF outside sprite
    return out

def main():
    if len(sys.argv) != 2:
        print("Usage: python swap_rb.py input.png", file=sys.stderr)
        sys.exit(2)

    in_path = sys.argv[1]
    base, _ = os.path.splitext(in_path)
    out_path = base + "_grit.png"

    with Image.open(in_path) as img:
        swapped = swap_rb_rgba(img)
        grit_img = to_grit_key(swapped)
        grit_img.save(out_path, "PNG")

    print("Wrote:", out_path)

if __name__ == "__main__":
    main()