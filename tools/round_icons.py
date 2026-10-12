#!/usr/bin/env python3
"""Gives every app icon the same rounded corners: make_icon.corner_mask (a radius of 12 pixels at 55 wide, the
corner of NumPlay's own icon). Each icon keeps its picture; outside the corner it becomes transparent, which the
calculator draws as the home screen's white and GitHub as its page. An icon already rounder there stays as it is.
The colour under the transparency is the icon on white, for tools that ignore it (Tetris' Rust build).

Usage: round_icons.py ICON.png...  (rewritten in place; 55x56 app icons and their bigger copies)
Run it again after redrawing an icon with a game's own script (balatro, chess, crossyroad, numblocks and portal have
one), as those draw square or differently rounded corners."""
import os
import sys

from PIL import Image, ImageChops

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from make_icon import corner_mask  # noqa: E402

for path in sys.argv[1:]:
    im = Image.open(path).convert("RGBA")
    alpha = ImageChops.darker(im.getchannel("A"), corner_mask(*im.size))
    flat = Image.new("RGB", im.size, (255, 255, 255))
    flat.paste(im.convert("RGB"), (0, 0), alpha)
    flat.putalpha(alpha)
    flat.save(path, optimize=True)
    print("rounded", path, im.size)
