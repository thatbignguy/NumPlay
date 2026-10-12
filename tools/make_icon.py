#!/usr/bin/env python3
"""Draws NumPlay's home screen icon (55x56, like the calculator's own apps):
a white game pad on a deep teal to sky blue gradient with a fine grain.

Also the corner every app icon shares (corner_mask): a rounded square whose
radius is 12 pixels at 55 wide, anti-aliased. tools/round_icons.py applies it
to the games' icons.

Usage: make_icon.py out.png [big.png WIDTH]  (the second file: a bigger copy
with transparent corners, for the README)"""
import random
import sys

from PIL import Image, ImageDraw

SS = 8                     # supersampling
RADIUS = 12                # the corner's radius at 55 pixels wide
# the gradient, from the top left to the bottom right
STOPS = ((0.0, (19, 58, 50)), (0.35, (27, 108, 100)), (0.7, (92, 152, 176)), (1.0, (66, 150, 208)))
INK = (24, 78, 66)         # the d-pad: the gradient's deep green
SAGE = (126, 146, 112)     # a button: sage green
BLUE = (58, 134, 196)      # the other: sky blue
GRAIN = 9                  # the grain's strength (levels of 255 either way)


def corner_mask(w, h):
    """The shape every app icon shares, as an L mask of w x h (anti-aliased)."""
    r = RADIUS * w / 55 * SS
    m = Image.new("L", (w * SS, h * SS), 0)
    ImageDraw.Draw(m).rounded_rectangle([0, 0, w * SS - 1, h * SS - 1], radius=r, fill=255)
    return m.resize((w, h), Image.LANCZOS)


def gradient_at(t):
    for (t0, c0), (t1, c1) in zip(STOPS, STOPS[1:]):
        if t <= t1:
            k = (t - t0) / (t1 - t0)
            k = k * k * (3 - 2 * k)   # smooth between the stops
            return tuple(a + (b - a) * k for a, b in zip(c0, c1))
    return STOPS[-1][1]


def icon(w=55, h=56):
    """Drawn for 55x56 and scaled for other sizes; RGBA with transparent corners."""
    S = SS * w / 55
    W, H = w * SS, h * SS
    # the gradient, at the icon's own size (it is smooth), with its grain
    rnd = random.Random(363)
    grad = Image.new("RGB", (w, h))
    px = grad.load()
    for y in range(h):
        for x in range(w):
            t = (x * 0.45 + y * 0.55) / ((w - 1) * 0.45 + (h - 1) * 0.55)
            n = rnd.uniform(-GRAIN, GRAIN)
            px[x, y] = tuple(max(0, min(255, int(c + n + 0.5))) for c in gradient_at(t))
    # the pad, supersampled
    pad = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(pad)
    cx, cy = W / 2, H / 2
    pw, ph = 42 * S, 26 * S

    def body(dx, dy, fill):
        d.rounded_rectangle([cx - pw / 2 + dx, cy - ph / 2 + dy, cx + pw / 2 + dx, cy + ph / 2 + dy], radius=ph / 2,
                            fill=fill)

    body(0, 2 * S, (8, 40, 34, 90))   # its soft shadow
    body(0, 0, (255, 255, 255, 255))
    # d-pad
    dx, dy, a, b = cx - 10.5 * S, cy, 5.5 * S, 2 * S
    d.rounded_rectangle([dx - a, dy - b, dx + a, dy + b], radius=S, fill=INK + (255,))
    d.rounded_rectangle([dx - b, dy - a, dx + b, dy + a], radius=S, fill=INK + (255,))
    # buttons
    for bx, by, col in ((cx + 8 * S, cy + 2.5 * S, SAGE), (cx + 13 * S, cy - 2.5 * S, BLUE)):
        r = 3 * S
        d.ellipse([bx - r, by - r, bx + r, by + r], fill=col + (255,))
    out = grad.convert("RGBA")
    out.alpha_composite(pad.resize((w, h), Image.LANCZOS))
    out.putalpha(corner_mask(w, h))
    return out


if __name__ == "__main__":
    im = icon()
    # the calculator ignores transparency: round the corners on the home screen's white
    flat = Image.new("RGB", im.size, (255, 255, 255))
    flat.paste(im, (0, 0), im)
    flat.save(sys.argv[1])
    if len(sys.argv) > 3:
        bw = int(sys.argv[3])
        icon(bw, round(bw * 56 / 55)).save(sys.argv[2])
