#!/usr/bin/env python3
"""Rounds the corners of the README's pictures, in place: a PNG gets smooth transparent corners, a GIF transparent
corners on every frame (a GIF's transparency is all or nothing, so its edge is crisp). GitHub drops CSS from READMEs,
so the corners have to be in the files. The radius is a twentieth of the width, so every picture looks alike at the
size it is shown. A picture already rounder there stays as it is.

Usage: round_corners.py FILE.png|FILE.gif...  (needs gifsicle for GIFs)"""
import os
import shutil
import subprocess
import sys
import tempfile

from PIL import Image, ImageChops, ImageDraw, ImageSequence

SS = 4   # supersampling for the PNGs' smooth edge


def radius(w):
    return max(4, round(w / 20))


def png_mask(w, h):
    r = radius(w) * SS
    m = Image.new("L", (w * SS, h * SS), 0)
    ImageDraw.Draw(m).rounded_rectangle([0, 0, w * SS - 1, h * SS - 1], radius=r, fill=255)
    return m.resize((w, h), Image.LANCZOS)


def outside(w, h):
    """The pixels outside the rounded rectangle (by their centres), for a GIF."""
    r = radius(w)
    out = []
    for y in list(range(r)) + list(range(h - r, h)):
        cy = r if y < r else h - r
        for x in list(range(r)) + list(range(w - r, w)):
            cx = r if x < r else w - r
            dx, dy = x + 0.5 - cx, y + 0.5 - cy
            if dx * dx + dy * dy > r * r:
                out.append((x, y))
    return out


def round_png(path):
    im = Image.open(path)
    rgba = im.convert("RGBA")
    rgba.putalpha(ImageChops.darker(rgba.getchannel("A"), png_mask(*im.size)))
    rgba.save(path, optimize=True)


def round_gif(path):
    src = Image.open(path)
    w, h = src.size
    frames, durations = [], []
    for f in ImageSequence.Iterator(src):
        durations.append(f.info.get("duration", 100))
        frames.append(f.convert("RGB"))
    # one palette for all frames (255 colours; the last index is the transparent corner)
    step = max(1, len(frames) // 48)
    sample = Image.new("RGB", (w, h * len(frames[::step])))
    for i, f in enumerate(frames[::step]):
        sample.paste(f, (0, i * h))
    pal = sample.quantize(colors=255, method=Image.Quantize.MEDIANCUT)
    corner = outside(w, h)
    with tempfile.TemporaryDirectory() as d:
        args = ["gifsicle", "-O3", "--loopcount=0"]
        for i, (f, ms) in enumerate(zip(frames, durations)):
            q = f.quantize(palette=pal, dither=Image.Dither.NONE)
            pal_bytes = q.getpalette()[:765] + [0, 0, 0]
            q.putpalette(pal_bytes)
            px = q.load()
            for x, y in corner:
                px[x, y] = 255
            fp = os.path.join(d, f"{i:05d}.gif")
            q.save(fp, transparency=255)
            args += ["-d", str(max(2, round(ms / 10))), fp]
        out = os.path.join(d, "out.gif")
        subprocess.run(args + ["-o", out], check=True)
        shutil.copyfile(out, path)


for p in sys.argv[1:]:
    before = os.path.getsize(p)
    (round_gif if p.lower().endswith(".gif") else round_png)(p)
    print(f"rounded {p}: {before // 1024} KB -> {os.path.getsize(p) // 1024} KB")
