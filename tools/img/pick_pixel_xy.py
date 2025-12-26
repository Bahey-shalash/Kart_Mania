from pathlib import Path
from PIL import Image
import matplotlib.pyplot as plt


def click_to_get_xy(png_path: str):
    path = Path(png_path)
    if not path.exists():
        raise FileNotFoundError(f"File not found: {path}")

    img = Image.open(path).convert("RGBA")
    w, h = img.size

    print(f"Loaded image {path} with size {w}x{h}")

    fig, ax = plt.subplots()
    ax.imshow(img, origin="upper", interpolation="nearest")  # (0,0) is top-left
    ax.set_title(
        f"Click a pixel to print (x, y) — image {w}x{h}. Close window to quit."
    )
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
    # python pick_pixel_xy.py your_image.png
    import sys

    if len(sys.argv) != 2:
        print("Usage: python pick_pixel_xy.py <image.png>")
        raise SystemExit(2)

    click_to_get_xy(sys.argv[1])
