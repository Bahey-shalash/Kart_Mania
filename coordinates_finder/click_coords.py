#!/usr/bin/env python3
from pathlib import Path
from PIL import Image
import matplotlib.pyplot as plt

def click_to_get_xy(png_path: str):
    path = Path(png_path)
    if not path.exists():
        raise FileNotFoundError(f"File not found: {path}")

    img = Image.open(path).convert("RGBA")
    w, h = img.size

    if (w, h) != (256, 192):
        raise ValueError(f"Expected 256x192 image, got {w}x{h}")

    fig, ax = plt.subplots()
    ax.imshow(img, origin="upper", interpolation="nearest")  # (0,0) is top-left
    ax.set_title("Click a pixel to print (x, y). Close window to quit.")
    ax.set_xlim(-0.5, w - 0.5)
    ax.set_ylim(h - 0.5, -0.5)

    def on_click(event):
        # Only accept clicks inside the image axes
        if event.inaxes != ax or event.xdata is None or event.ydata is None:
            return

        x = int(round(event.xdata))
        y = int(round(event.ydata))

        # Clamp to valid pixel range
        x = max(0, min(w - 1, x))
        y = max(0, min(h - 1, y))

        r, g, b, a = img.getpixel((x, y))
        print(f"(x, y) = ({x}, {y})   RGBA=({r},{g},{b},{a})")

    fig.canvas.mpl_connect("button_press_event", on_click)
    plt.show()

if __name__ == "__main__":
    # Example usage:
    # python click_coords.py your_image.png
    import sys
    if len(sys.argv) != 2:
        print("Usage: python click_coords.py <image_256x192.png>")
        raise SystemExit(2)
    click_to_get_xy(sys.argv[1])