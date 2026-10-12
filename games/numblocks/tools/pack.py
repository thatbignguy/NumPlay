#!/usr/bin/env python3
"""Packs Minecraft textures for NumBlocks into src/data.c and src/data.h.

    python3 tools/pack.py CLIENT_JAR
    python3 tools/pack.py --modern MODERN_TEX_DIR CLIENT_JAR
    python3 tools/pack.py [--modern MODERN_TEX_DIR] CLIENT_JAR [--eggs EXTRA_TEX_DIR]

CLIENT_JAR is the official 1.8.8 client (client.jar, from Mojang's launcher
servers); only its pictures are read. With --modern, block/item/entity/font/
environment/colormap pictures come from an extracted modern client instead
(tools/modern_map.py names them), and CLIENT_JAR is only the fallback for
what has no modern equivalent (bed block textures, most GUI/HUD sprites).
With --eggs, an extra modern root is looked in for what the modern set has
none of neither: the per-mob spawn eggs (1.21.5's <mob>_spawn_egg.png, which
a 1.20.1 client has none of). Nothing passes it by default (no such root is
kept in this repository): run `make data` with EGG_TEX set to a directory
holding the per-mob eggs to pack those pictures.
Every texture is 16 x 16 with its own
palette of up to 16 colours (4 bits a texel): tinted ones (grass, leaves,
water) keep their grey levels, and the game multiplies them by the biome's
colour. Index 0 is transparent in textures that have see-through texels.

Also written: each block state's six faces, model, light, flags, hardness and
tool (from tools/blocks.py)."""
import io
import os
import sys
import zipfile

from PIL import Image


def pixels(im):
    return list(im.get_flattened_data() if hasattr(im, 'get_flattened_data') else im.getdata())

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import blocks  # noqa: E402

TEX = 'assets/minecraft/textures/'


def rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def extra_image(roots, name):
    """a picture from one of the extra modern texture roots (the 1.21.5 per-mob spawn
    eggs, which 1.8.8's jar has none of), as RGBA, or None if no root has it."""
    if not roots:
        return None
    import modern_map
    return modern_map.extra_image(roots, name)


class Jar:
    def __init__(self, path, extra=()):
        self.z = zipfile.ZipFile(path)
        self.extra = tuple(extra)   # modern roots for what 1.8.8 has none of (the spawn eggs)

    def image(self, name):
        im = extra_image(self.extra, name) if self.extra else None
        if im is None:
            im = Image.open(io.BytesIO(self.z.read(TEX + name + '.png'))).convert('RGBA')
        return im


def quantize(im, colors, keep_grey=False):
    """(indices 16x16, palette [(r,g,b)], transparent?): index 0 is transparent if any texel is."""
    px = pixels(im)
    alpha = any(a < 128 for _, _, _, a in px)
    opaque = [(r, g, b) for r, g, b, a in px if a >= 128]
    n = colors - (1 if alpha else 0)
    src = Image.new('RGB', (len(opaque), 1))
    src.putdata(opaque)
    q = src.quantize(n, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    pal = q.getpalette()[:3 * n]
    pal = [tuple(pal[i:i + 3]) for i in range(0, len(pal), 3)]
    idx = pixels(q)
    out, k = [], 0
    for r, g, b, a in px:
        if a < 128:
            out.append(0)
        else:
            out.append(idx[k] + (1 if alpha else 0))
            k += 1
    if alpha:
        pal = [(0, 0, 0)] + pal
    return out, pal, alpha


def first_frame(im):
    w, h = im.size
    return im.crop((0, 0, w, w)).resize((16, 16), Image.NEAREST) if w != 16 or h != 16 else im


def custom_texture(jar, n):
    """textures made from an entity's: the chest's faces (ModelChest: a 14 x 14 x 14 box)

    1.15 changed the chest texture's layout (MC-161338): the front moved to
    the far right (x 42) and the lid's top to x 28, the sides flipped
    vertically and the top/bottom rotated 180. Detect the layout from the
    pixels (modern wood at (35, 7), old dark there) so both 1.8.8's client.jar
    and a modern extracted client give a closed chest, not an open-looking
    dark hole."""
    from PIL import ImageOps
    ch = jar.image('entity/chest/normal')
    out = Image.new('RGBA', (16, 16), (0, 0, 0, 0))
    # modern: wood at the middle of (28, 0, 42, 14); 1.8.8: dark there (the inside)
    try:
        r, g, b, a = ch.getpixel((35, 7))
        modern = r > 100
    except Exception:
        modern = False
    if n == '@chest_top':
        box = (28, 0, 42, 14) if modern else (14, 0, 28, 14)
        top = ch.crop(box)
        if modern:
            top = top.rotate(180)
        out.paste(top, (1, 1))
    elif n in ('@chest_front', '@chest_side'):
        if modern:
            lid_box = (42, 14, 56, 19) if n == '@chest_front' else (0, 14, 14, 19)
            base_box = (42, 34, 56, 43) if n == '@chest_front' else (0, 34, 14, 43)
            lid = ImageOps.flip(ch.crop(lid_box))
            base = ImageOps.flip(ch.crop(base_box))
            # flip back the latch notch: the flip moves it to the wrong end for the
            # composite (lid 5 + base 9); flipping the whole 14-high face keeps the
            # junction aligned, so flip each part then paste (wood is horizontal anyway)
        else:
            lid = ch.crop((14, 14, 28, 19) if n == '@chest_front' else (0, 14, 14, 19))
            base = ch.crop((14, 34, 28, 43) if n == '@chest_front' else (0, 34, 14, 43))
        out.paste(lid, (1, 2))
        out.paste(base, (1, 7))
        if n == '@chest_front':
            out.paste(ch.crop((1, 1, 3, 5)), (7, 5))   # the latch
    return out


def tex_image(jar, n):
    # bed block faces: use the old blocks/bed_* (1.8.8) – the modern entity is
    # 3D and too much for NumBlocks' low 9-high box (user asked for the old one)
    return custom_texture(jar, n) if n.startswith('@') else first_frame(jar.image('blocks/' + n))


def pack_textures(jar):
    """Textures in the order blocks.py first names them."""
    names = []
    for s in blocks.S:
        t = s['tex']
        if t is None:
            continue
        for n in ([t] if isinstance(t, str) else list(t.values())):
            if n and n not in names:
                names.append(n)
    tinted = set()
    for s in blocks.S:
        if s['tint'] and s['tex']:
            t = s['tex']
            if isinstance(t, str):
                tinted.add(t)
            else:
                tinted.add(t['top'] if s['name'].startswith('GRASS') else t.get('side'))
                if 'overlay' in t:
                    tinted.add(t['overlay'])
    tinted.discard('grass_side')   # its fringe comes from the overlay
    tinted.discard('grass_side_snowed')
    px, pals, flags = [], [], []
    for n in names:
        im = tex_image(jar, n)
        if n == 'grass_side':
            # the dirt side keeps its colours; the fringe (the overlay's texels)
            # becomes grey levels tinted by the biome: palette indices from 8
            over = first_frame(jar.image('blocks/grass_side_overlay'))
            base = im.copy()
            ov = [a >= 128 for _, _, _, a in pixels(over)]
            dirt = Image.new('RGBA', (16, 16))
            dirt.putdata([p if not o else (0, 0, 0, 0) for p, o in zip(pixels(base), ov)])
            grey = Image.new('RGBA', (16, 16))
            grey.putdata([p if o else (0, 0, 0, 0) for p, o in zip(pixels(over), ov)])
            di, dp, _ = quantize(dirt, 9)
            gi, gp, _ = quantize(grey, 9)
            idx = [(gi[i] + 7) if ov[i] else di[i] for i in range(256)]
            pal = dp[:8] + gp[1:9]
            while len(pal) < 16:
                pal.append((0, 0, 0))
            px.append(idx)
            pals.append(pal)
            flags.append(8)   # tinted from index 8
            continue
        idx, pal, alpha = quantize(im, 16)
        while len(pal) < 16:
            pal.append((0, 0, 0))
        px.append(idx)
        pals.append(pal)
        tint_from = (1 if alpha else 0) if n in tinted else 16
        flags.append(tint_from | (0x20 if alpha else 0))
    return names, px, pals, flags


FACE = ['bottom', 'top', 'north', 'south', 'west', 'east']   # -Y +Y -Z +Z -X +X


def faces(s, names):
    t = s['tex']
    if t is None:
        return [0] * 6
    if isinstance(t, str):
        i = names.index(t)
        return [i] * 6
    side = t.get('side')
    out = []
    for f in FACE:
        n = t.get(f) or (t.get('top') if f == 'bottom' and 'bottom' not in t else side)
        if f in ('west', 'east') and 'x' in t:
            n = t['x']
        if f in ('north', 'south') and 'z' in t:
            n = t['z']
        out.append(names.index(n))
    return out


# shapes in sixteenths (x0 y0 z0 x1 y1 z1), up to two boxes; models not listed are whole cubes or
# drawn their own way (cross, liquid); ladders, vines, fences and panes are shaped by their neighbours
BOX = {'slab': [(0, 0, 0, 16, 8, 16)], 'layer': [(0, 0, 0, 16, 2, 16)], 'torch': [(7, 0, 7, 9, 10, 9)],
       'cactus': [(1, 0, 1, 15, 16, 15)], 'flat': [(0, 0, 0, 16, 1, 16)], 'fence': [(6, 0, 6, 10, 16, 10)],
       'pane': [(7, 0, 7, 9, 16, 9)], 'ladder': [(0, 0, 14, 16, 16, 16)], 'vine': [(0, 0, 15, 16, 16, 16)],
       'bed': [(0, 0, 0, 16, 9, 16)], 'chest': [(1, 0, 1, 15, 14, 15)], 'liquid': [(0, 0, 0, 16, 14, 16)]}
STAIR_TOP = [(8, 8, 0, 16, 16, 16), (0, 8, 0, 8, 16, 16), (0, 8, 8, 16, 16, 16), (0, 8, 0, 16, 16, 8)]
# BlockDoor's bounds: facing east, south, west, north; closed, then open
DOOR = [(0, 0, 0, 3, 16, 16), (0, 0, 0, 16, 16, 3), (13, 0, 0, 16, 16, 16), (0, 0, 13, 16, 16, 16),
        (0, 0, 13, 16, 16, 16), (0, 0, 0, 3, 16, 16), (0, 0, 0, 16, 16, 3), (13, 0, 0, 16, 16, 16)]
WALL_TORCH = {'TORCH_E': (1, 3, 7, 3, 14, 9), 'TORCH_W': (13, 3, 7, 15, 14, 9), 'TORCH_S': (7, 3, 1, 9, 14, 3),
              'TORCH_N': (7, 3, 13, 9, 14, 15)}


def boxes_of(s):
    m = s['model']
    if m == 'portal':
        # the nether portal's thin plane: NS runs east-west (thin in Z), EW runs
        # north-south (thin in X), two texels thick in the middle like a pane
        if s['name'].endswith('_EW'):
            return [(7, 0, 0, 9, 16, 16)]
        return [(0, 0, 7, 16, 16, 9)]
    if m == 'stairs':
        return [(0, 0, 0, 16, 8, 16), STAIR_TOP[s['meta'] & 3]]
    if m == 'door':
        return [DOOR[s['meta'] & 7]] if s['meta'] < 8 else [DOOR[0]]
    if s['name'] in WALL_TORCH:
        return [WALL_TORCH[s['name']]]
    return BOX.get(m, [])


TINT = {None: 0, 'grass': 1, 'foliage': 2, 'spruce': 3, 'birch': 4, 'water': 5, 'lily': 6}
OPAQUE = {'cube', 'slab'}   # (slabs and layers only hide what is under them)


# biome id: (temperature, rainfall) as BiomeBase sets them (1.8.8); mutated biomes (id + 128) share them
BIOMES = {0: (0.5, 0.5), 1: (0.8, 0.4), 2: (2.0, 0.0), 3: (0.2, 0.3), 4: (0.7, 0.8), 5: (0.25, 0.8), 6: (0.8, 0.9),
          7: (0.5, 0.5), 8: (2.0, 0.0), 9: (0.5, 0.5), 10: (0.0, 0.5), 11: (0.0, 0.5), 12: (0.0, 0.5), 13: (0.0, 0.5),
          14: (0.9, 1.0), 15: (0.9, 1.0), 16: (0.8, 0.4), 17: (2.0, 0.0), 18: (0.7, 0.8), 19: (0.25, 0.8),
          20: (0.2, 0.3), 21: (0.95, 0.9), 22: (0.95, 0.9), 23: (0.95, 0.8), 24: (0.5, 0.5), 25: (0.2, 0.3),
          26: (0.05, 0.3), 27: (0.6, 0.6), 28: (0.6, 0.6), 29: (0.7, 0.8), 30: (-0.5, 0.4), 31: (-0.5, 0.4),
          32: (0.3, 0.8), 33: (0.3, 0.8), 34: (0.2, 0.3), 35: (1.2, 0.0), 36: (1.0, 0.0), 37: (2.0, 0.0), 38: (2.0, 0.0),
          39: (2.0, 0.0)}
# (forest 4 is 0.7/0.8; birch forests 0.6/0.6; plains 0.8/0.4; mega spruce taiga mutated 160: 0.25/0.8)


def biome_colors(jar):
    grass = jar.image('colormap/grass').convert('RGB')
    foliage = jar.image('colormap/foliage').convert('RGB')
    # 1.8.8 water textures are already blue (white tint keeps them);
    # modern (1.13+) ones are grey and need the biome's water colour.
    try:
        _wr, _wg, _wb, _wa = first_frame(jar.image('blocks/water_still')).getpixel((8, 8))
        modern_water = max(abs(_wr - _wg), abs(_wg - _wb), abs(_wr - _wb)) < 12
    except Exception:
        modern_water = True

    def look(img, t, r):
        t = min(max(t, 0.0), 1.0)
        r = min(max(r, 0.0), 1.0) * t
        return img.getpixel((int((1 - t) * 255), int((1 - r) * 255)))

    g, f, w = [], [], []
    for i in range(256):
        base = i & 127 if i >= 128 else i
        t, r = BIOMES.get(base, (0.5, 0.5))
        if i == 160 or i == 161:
            t, r = 0.25, 0.8
        gc, fc = look(grass, t, r), look(foliage, t, r)
        if base == 6:   # swamp: fixed colours
            gc, fc = (0x6A, 0x70, 0x39), (0x6A, 0x70, 0x39)
        if base in (37, 38, 39):   # mesa
            gc, fc = (0x90, 0x81, 0x4D), (0x9E, 0x81, 0x4D)
        if base == 29:   # roofed forest: grass halfway to a dark green
            c = (gc[0] << 16) | (gc[1] << 8) | gc[2]
            c = ((c & 0xFEFEFE) + 0x28340A) >> 1
            gc = ((c >> 16) & 255, (c >> 8) & 255, c & 255)
        # modern (1.13+) water textures are grey; the biome's water colour
        # provides the blue (Java water_color): default #3F76E4, swamp #617B64,
        # frozen ocean/river #3938C9, cold water (snowy beach, snowy taiga) #3D57D6.
        # 1.8.8 water is already blue: white tint (swamp #E0FFAE) keeps it.
        if not modern_water:
            wc = (0xE0, 0xFF, 0xAE) if base == 6 else (0xFF, 0xFF, 0xFF)
        elif base == 6:   # swamp
            wc = (0x61, 0x7B, 0x64)
        elif base in (10, 11):   # frozen ocean, frozen river
            wc = (0x39, 0x38, 0xC9)
        elif base in (26, 30, 31):   # snowy beach, snowy taiga (+ hills)
            wc = (0x3D, 0x57, 0xD6)
        else:
            wc = (0x3F, 0x76, 0xE4)
        g.append(gc)
        f.append(fc)
        w.append(wc)
    return g, f, w


# GUI pieces: name, file, x, y, w, h
SPRITES = [
    ('hotbar', 'gui/widgets', 0, 0, 182, 22),
    ('hotbar_sel', 'gui/widgets', 0, 22, 24, 24),
    ('crosshair', 'gui/icons', 0, 0, 16, 16),
    ('heart_bg', 'gui/icons', 16, 0, 9, 9),
    ('heart', 'gui/icons', 52, 0, 9, 9),
    ('heart_half', 'gui/icons', 61, 0, 9, 9),
    ('food_bg', 'gui/icons', 16, 27, 9, 9),
    ('food', 'gui/icons', 52, 27, 9, 9),
    ('food_half', 'gui/icons', 61, 27, 9, 9),
    ('armor_bg', 'gui/icons', 16, 9, 9, 9),
    ('xp_bg', 'gui/icons', 0, 64, 182, 5),
    ('xp', 'gui/icons', 0, 69, 182, 5),
    ('bubble', 'gui/icons', 16, 18, 9, 9),
    ('heart_poison', 'gui/icons', 88, 0, 9, 9),
    ('heart_poison_half', 'gui/icons', 97, 0, 9, 9),
    ('food_hunger_bg', 'gui/icons', 133, 27, 9, 9),
    ('food_hunger', 'gui/icons', 88, 27, 9, 9),
    ('food_hunger_half', 'gui/icons', 97, 27, 9, 9),
    ('bobber', 'particle/particles', 8, 16, 8, 8),
    ('fishing_rod_cast', 'items/fishing_rod_cast', 0, 0, 16, 16),
    # the sun (its glow) and the moon's eight phases (their middles; drawn adding light)
    ('sun', 'environment/sun', 2, 2, 28, 28),
] + [(f'moon_{k}', 'environment/moon_phases', (k % 4) * 32 + 8, (k // 4) * 32 + 8, 16, 16) for k in range(8)]


# what the inventory tints its icons with: plains grass and foliage, and the fixed colours
# (ColorizerFoliage.getFoliageColorPine and getFoliageColorBirch, BlockLilyPad,
# and the default water colour: modern water textures are grey)
ICON_TINT = {'grass': (0x91, 0xBD, 0x59), 'foliage': (0x91, 0xBD, 0x59), 'spruce': (0x61, 0x99, 0x61),
             'birch': (0x80, 0xA7, 0x55), 'lily': (0x20, 0x80, 0x30), 'water': (0x3F, 0x76, 0xE4)}


def iso_icon(jar, s, names_px):
    """A block as the inventory shows it: three faces, the top lit, the left 0.8, the right 0.6 (16 x 16)."""
    t = s['tex']
    k = ICON_TINT.get(s['tint'])
    if s['model'] in ('cross', 'flat', 'torch', 'vine', 'ladder', 'door', 'pane', 'portal'):
        n = t if isinstance(t, str) else t.get('side')
        im = tex_image(jar, n)
        if k:
            im = im.convert('RGBA').copy()
            im.putdata([(r * k[0] // 255, g * k[1] // 255, b * k[2] // 255, a) for r, g, b, a in pixels(im)])
        return im
    # stairs and fences have their own shapes, not a cube (below)
    if s['model'] == 'stairs':
        return iso_stairs(jar, s, k)
    if s['model'] == 'fence':
        return iso_fence(jar, s, k)
    top = t if isinstance(t, str) else t['top']
    left = t if isinstance(t, str) else t.get('front', t['side']) if s['meta'] in (0,) else t['side']
    right = t if isinstance(t, str) else t['side']
    out = Image.new('RGBA', (16, 16), (0, 0, 0, 0))
    timg = tex_image(jar, top)
    if s['tint'] == 'water':
        # 1.8.8 water is already blue; only modern grey water needs the tint
        try:
            _wr, _wg, _wb, _wa = timg.getpixel((8, 8))
            if max(abs(_wr - _wg), abs(_wg - _wb), abs(_wr - _wb)) >= 12:
                k = None
        except Exception:
            pass
    limg = tex_image(jar, (t.get('front', t['side']) if isinstance(t, dict) and 'front' in t else left))
    rimg = tex_image(jar, right)
    tint = s['tint']
    def tinted(im, shade, tint_on):
        px = []
        for r, g, b, a in pixels(im):
            if tint_on:
                r, g, b = r * k[0] // 255, g * k[1] // 255, b * k[2] // 255
            px.append((int(r * shade), int(g * shade), int(b * shade), a))
        o = Image.new('RGBA', im.size)
        o.putdata(px)
        return o
    timg = tinted(timg, 1.0, k is not None)
    limg = tinted(limg, 0.8, k is not None and tint != 'grass')
    rimg = tinted(rimg, 0.6, k is not None and tint != 'grass')
    # chest and cactus faces keep a 1-texel see-through border (the box is
    # smaller: chest 14 x 14 x 14, cactus 14 x 16 x 14): sample the opaque
    # middle so the icon is solid (no splits/holes), drawn full-size.
    # Their outer columns are dark edge trim: sample asymmetrically so the
    # trim stays on the icon's outer silhouette and plain wood meets at the
    # middle seam (trim-against-trim there reads as a black split/hole).
    h = 0.5 if s['model'] == 'slab' else 0.5625 if s['model'] == 'bed' else (14 / 16) if s['model'] == 'chest' else 1.0
    inset_tex = s['model'] in ('chest', 'cactus')
    is_chest = s['model'] == 'chest'
    for y in range(16):
        for x in range(16):
            px, py = x + 0.5, y + 0.5
            # top face: (8,0) (16,4) (8,8) (0,4); u along (8,0)->(16,4), v along (8,0)->(0,4)
            oy = (1 - h) * 8
            a = ((px - 8) / 8 + (py - oy) / 4) / 2
            b = (-(px - 8) / 8 + (py - oy) / 4) / 2
            if 0 <= a < 1 and 0 <= b < 1:
                if inset_tex:
                    tu, tv = 1 + min(13, int(a * 14)), 1 + min(13, int(b * 14))
                    c = timg.getpixel((tu, tv))
                else:
                    c = timg.getpixel((min(15, int(a * 16)), min(15, int(b * 16))))
                if c[3] >= 128:
                    out.putpixel((x, y), c)
                continue
            # left face: (0,4) (8,8) down 8: u = x/8, v = (y - 4 - x/2) / 8
            if px < 8:
                u = px / 8
                v = (py - 4 - oy - px / 2) / (8 * h)
                if 0 <= u < 1 and 0 <= v < 1:
                    if inset_tex:
                        # trim on the outer edge (tu=1), plain wood at the seam
                        tu = 1 + min(12, int(u * 13))
                        if is_chest:
                            tv = 2 + min(13, int(((1 - h) + v * h) * 14))
                        else:
                            tv = 1 + min(13, int(((1 - h) + v * h) * 14))
                        # v maps 0..1 to 2..15 (chest 14 high: lid 5 + base 9)
                        # or 1..14 (cactus 14 opaque rows)
                        c = limg.getpixel((tu, tv))
                    else:
                        c = limg.getpixel((min(15, int(u * 16)), min(15, int(((1 - h) + v * h) * 16))))
                    if c[3] >= 128:
                        out.putpixel((x, y), c)
            else:
                u = (px - 8) / 8
                v = (py - 8 - oy + (px - 8) / 2) / (8 * h)
                if 0 <= u < 1 and 0 <= v < 1:
                    if inset_tex:
                        # plain wood at the seam (tu=2), trim on the outer edge
                        tu = 2 + min(12, int(u * 13))
                        if is_chest:
                            tv = 2 + min(13, int(((1 - h) + v * h) * 14))
                        else:
                            tv = 1 + min(13, int(((1 - h) + v * h) * 14))
                        c = rimg.getpixel((tu, tv))
                    else:
                        c = rimg.getpixel((min(15, int(u * 16)), min(15, int(((1 - h) + v * h) * 16))))
                    if c[3] >= 128:
                        out.putpixel((x, y), c)
    return out


def iso_stairs(jar, s, k):
    """Stairs as the inventory shows them: the bottom slab plus the raised step."""
    import blocks as _blocks
    t = s['tex']
    tex = t if isinstance(t, str) else t.get('side', t.get('top'))
    base_img = tex_image(jar, tex)
    top_img = tex_image(jar, t['top'] if isinstance(t, dict) else t)

    def tinted(im, shade):
        if k is None:
            px = [(int(r * shade), int(g * shade), int(b * shade), a) for r, g, b, a in pixels(im)]
        else:
            px = [(int(r * k[0] // 255 * shade), int(g * k[1] // 255 * shade), int(b * k[2] // 255 * shade), a)
                  for r, g, b, a in pixels(im)]
        o = Image.new('RGBA', im.size)
        o.putdata(px)
        return o

    # bottom slab (0,0,0,16,8,16) and the east step (8,8,0,16,16,16): reuse the
    # cube renderer by faking two slab/bed-like states (avoids duplicating it)
    out = Image.new('RGBA', (16, 16), (0, 0, 0, 0))
    # draw the full bottom half first, then the step on top of it
    for part, y0, y1, x0, x1 in (('base', 0, 8, 0, 16), ('step', 8, 16, 8, 16)):
        img = base_img if part == 'base' else top_img
        # shade like the cube faces: top 1.0, left 0.8, right 0.6
        for shade, getter in ((1.0, lambda a, b: (a, b)), (0.8, None), (0.6, None)):
            pass
        # simplest: draw each part as its own small iso cube via the generic path
        # by temporarily shifting it: bottom slab h=0.5 at y0, step h=0.5 at y=8
        # with x range x0..x1. Reuse the math inline (kept small on purpose).
        h = 0.5
        oy_base = (1 - h) * 8 if part == 'base' else 0  # step sits on the slab
        # step's top is at full height (y=16), its diamond is the full one cut to x 8..16
        for y in range(16):
            for x in range(16):
                px, py = x + 0.5, y + 0.5
                if part == 'base':
                    oy = 4.0
                    a = ((px - 8) / 8 + (py - oy) / 4) / 2
                    b = (-(px - 8) / 8 + (py - oy) / 4) / 2
                    if 0 <= a < 1 and 0 <= b < 1:
                        c = tinted(img, 1.0).getpixel((min(15, int(a * 16)), min(15, int(b * 16))))
                        if c[3] >= 128:
                            # only where the step does not cover (handled by order)
                            if out.getpixel((x, y))[3] < 128:
                                out.putpixel((x, y), c)
                        continue
                    if px < 8:
                        u, v = px / 8, (py - 4 - oy - px / 2) / (8 * h)
                        if 0 <= u < 1 and 0 <= v < 1:
                            c = tinted(img, 0.8).getpixel((min(15, int(u * 16)), min(15, int(v * 16 + 8))))
                            if c[3] >= 128 and out.getpixel((x, y))[3] < 128:
                                out.putpixel((x, y), c)
                    else:
                        u, v = (px - 8) / 8, (py - 8 - oy + (px - 8) / 2) / (8 * h)
                        if 0 <= u < 1 and 0 <= v < 1:
                            c = tinted(img, 0.6).getpixel((min(15, int(u * 16)), min(15, int(v * 16 + 8))))
                            if c[3] >= 128 and out.getpixel((x, y))[3] < 128:
                                out.putpixel((x, y), c)
                else:
                    # step: top diamond cut to its x half (east half: a in [0.5, 1))
                    oy = 0.0
                    a = ((px - 8) / 8 + (py - oy) / 4) / 2
                    b = (-(px - 8) / 8 + (py - oy) / 4) / 2
                    if 0.5 <= a < 1 and 0 <= b < 1:
                        c = tinted(img, 1.0).getpixel((min(15, int(a * 16)), min(15, int(b * 16))))
                        if c[3] >= 128:
                            out.putpixel((x, y), c)
                        continue
                    # step sides: left face only where x in the step's half
                    if px < 8:
                        # step's west face (at x=8) shows as a small square mid-icon;
                        # the south/north faces show on the left/right halves
                        u, v = px / 8, (py - 4 - oy - px / 2) / 4
                        # only the east half's south face is visible on the left side
                        if 0.5 <= u < 1 and 0 <= v < 1:
                            c = tinted(img, 0.8).getpixel((min(15, int(u * 16)), min(15, int(v * 8))))
                            if c[3] >= 128:
                                out.putpixel((x, y), c)
                    else:
                        u, v = (px - 8) / 8, (py - 8 - oy + (px - 8) / 2) / 4
                        if 0 <= u < 1 and 0 <= v < 1:
                            # step's east face (full) on the right, plus its top edge
                            c = tinted(img, 0.6).getpixel((min(15, int(u * 16)), min(15, int(8 + v * 8))))
                            if c[3] >= 128:
                                # draw over the base (the step is in front/on top)
                                out.putpixel((x, y), c)
    return out


def iso_boxes(jar, tex, boxes, k):
    """Iso icon of arbitrary boxes (x0,y0,z0,x1,y1,z1 in sixteenths), e.g. the
    fence inventory model (two posts plus two full-length bars). One texture
    for every face (full-face UVs, like the other iso icons); faces shaded
    top 1.0, south/left 0.8, east/right 0.6. Per-pixel nearest-face wins, so
    interpenetrating boxes (bars through posts) come out connected."""
    img = tex_image(jar, tex)

    def tinted(im, shade):
        if k is None:
            px = [(int(r * shade), int(g * shade), int(b * shade), a) for r, g, b, a in pixels(im)]
        else:
            px = [(int(r * k[0] // 255 * shade), int(g * k[1] // 255 * shade), int(b * k[2] // 255 * shade), a)
                  for r, g, b, a in pixels(im)]
        o = Image.new('RGBA', im.size)
        o.putdata(px)
        return o

    top, left, right = tinted(img, 1.0), tinted(img, 0.8), tinted(img, 0.6)
    out = Image.new('RGBA', (16, 16), (0, 0, 0, 0))
    for y in range(16):
        for x in range(16):
            px, py = x + 0.5, y + 0.5
            best, bc = -1e9, None
            for (x0, y0, z0, x1, y1, z1) in boxes:
                oy = (16 - y1) / 2
                # top face (y=y1): a along +x, b along +z
                a = ((px - 8) / 8 + (py - oy) / 4) / 2
                b = (-(px - 8) / 8 + (py - oy) / 4) / 2
                if x0 / 16 <= a < x1 / 16 and z0 / 16 <= b < z1 / 16:
                    wx, wz = a * 16, b * 16
                    c = top.getpixel((min(15, int((a - x0 / 16) / ((x1 - x0) / 16) * 16)),
                                          min(15, int((b - z0 / 16) / ((z1 - z0) / 16) * 16))))
                    if c[3] >= 128 and wx + y1 + wz > best:
                        best, bc = wx + y1 + wz, c
                # south face (z=z1): u along +x, v down from the top (y1)
                if y0 < y1:
                    # px = x/2 + 8 - z1/2  ->  x = 2*(px-8) + z1
                    sx = 2 * (px - 8) + z1
                    # py = 4 + oy + px/2 + (y1-y)/2  ->  y = y1 - 2*(py-4-oy-px/2)
                    sy = y1 - 2 * (py - 4 - oy - px / 2)
                    if x0 <= sx < x1 and y0 <= sy < y1:
                        c = left.getpixel((min(15, int((sx - x0) / (x1 - x0) * 16)),
                                                min(15, int((y1 - sy) / (y1 - y0) * 16))))
                        if c[3] >= 128 and sx + sy + z1 > best:
                            best, bc = sx + sy + z1, c
                # east face (x=x1): u mirrored along -z, v down from the top
                if y0 < y1:
                    # px = 8 + (x1-z)/2  ->  z = x1 - 2*(px-8)
                    sz = x1 - 2 * (px - 8)
                    sy = y1 - 2 * (py - 8 - oy + (px - 8) / 2)
                    if z0 <= sz < z1 and y0 <= sy < y1:
                        c = right.getpixel((min(15, int((1 - (sz - z0) / (z1 - z0)) * 16)),
                                            min(15, int((y1 - sy) / (y1 - y0) * 16))))
                        if c[3] >= 128 and x1 + sy + sz > best:
                            best, bc = x1 + sy + sz, c
            if bc is not None:
                out.putpixel((x, y), bc)
    return out


def iso_fence(jar, s, k):
    """A fence as the inventory shows it: vanilla's fence_inventory model (two
    full-height posts plus a top and a lower full-length bar), not a cube."""
    t = s['tex']
    tex = t if isinstance(t, str) else t.get('side', t.get('top'))
    return iso_boxes(jar, tex, [(6, 0, 0, 10, 16, 4), (6, 0, 12, 10, 16, 16),
                                (7, 12, 0, 9, 15, 16), (7, 6, 0, 9, 9, 16)], k)


def pack_sprite(im):
    idx, pal, alpha = quantize(im, 16)
    if not alpha:   # keep index 0 free: a sprite is always drawn with 0 as see-through
        idx, pal, _ = quantize(im, 15)
        idx = [i + 1 for i in idx]
        pal = [(0, 0, 0)] + pal
    while len(pal) < 16:
        pal.append((0, 0, 0))
    w, h = im.size
    bs = []
    for y in range(h):
        row = idx[y * w:(y + 1) * w] + [0]
        bs += [row[i] | (row[i + 1] << 4) for i in range(0, w, 2)]
    return bs, pal, alpha


def skin_sheet(jar, name):
    """A mob's picture folded into the 64 x 32 the renderer indexes: mob.h's
    Part uv is 1.8's — the head at (0,0), the body at (16,16), the right arm at
    (40,16), the right leg at (0,16) — and the left arm and leg are boxes of
    their own with mirror set, drawn off the right limb's box instead of boxes
    of their own. Modern clients draw these mobs the same way, and their sheets
    keep 1.8's uv, so a 64 x 32 sheet folds straight across.

    A sheet taller than 32 rows is the zombie's (entity/zombie/zombie.png,
    64 x 64). Its rows past 32 are where a 64 x 64 player-style sheet would
    put the left arm and the left leg, and a modern zombie's model does not
    read them (it has no outer layer and mirrors its right limbs, as mob.c
    does): the top half is the whole mob, so that is all the fold keeps.

    A mob may no longer be drawn with 1.8's pictures, so a skin that fell back
    to the jar is an error rather than a silent mix of old and new art.
    """
    im = jar.image('entity/' + name)
    if 'entity/' + name in getattr(jar, 'used_fallback', ()):
        sys.exit(f'entity/{name} has no modern texture and fell back to the 1.8.8 jar')
    return im.crop((0, 0, 64, 32))


import items  # noqa: E402


def rle_image(im):
    """A picture as runs: each byte is a palette index (high nibble) and a run
    length - 1 (low nibble, 0-14); 15 means 16 + the next byte. Rows start at
    the offsets given. Index 0 is see-through if the picture has see-through
    pixels."""
    idx, pal, alpha = quantize(im, 16)
    if not alpha:
        idx, pal, _ = quantize(im, 15)
        idx = [i + 1 for i in idx]
        pal = [(0, 0, 0)] + pal
    while len(pal) < 16:
        pal.append((0, 0, 0))
    w, h = im.size
    data, rows = [], []
    for y in range(h):
        rows.append(len(data))
        row = idx[y * w:(y + 1) * w]
        x = 0
        while x < w:
            n = 1
            while x + n < w and n < 271 and row[x + n] == row[x]:
                n += 1
            if n < 16:
                data.append(row[x] << 4 | (n - 1))
            else:
                data += [row[x] << 4 | 15, n - 16]
            x += n
    return data, rows, pal


def leather(jar, part):
    """leather armour: the grey base dyed Minecraft's default leather colour, the overlay on top"""
    base = first_frame(jar.image('items/leather_' + part))
    over = first_frame(jar.image('items/leather_' + part + '_overlay'))
    out = Image.new('RGBA', (16, 16))
    px = []
    for (r, g, b, a), (r2, g2, b2, a2) in zip(pixels(base), pixels(over)):
        if a2 >= 128:
            px.append((r2, g2, b2, a2))
        else:
            px.append((r * 0xA0 // 255, g * 0x65 // 255, b * 0x40 // 255, a))
    out.putdata(px)
    return out


def item_icon(jar, it):
    t = it['tex']
    for part in ('helmet', 'chestplate', 'leggings', 'boots'):
        if t == 'leather_' + part:
            return leather(jar, part)
    # bed stays flat (items/bed.png, 1.8.8 fallback when modern has none) –
    # the 3D entity is too much for an item icon (user asked for the old one)
    return first_frame(jar.image('items/' + t))


def steve_front(jar):
    """the player as the inventory shows it, from the front (skin pixels scaled to 27 x 54)"""
    sk = jar.image('entity/steve')
    im = Image.new('RGBA', (16, 32), (0, 0, 0, 0))
    def put(sx, sy, w, h, dx, dy, over=False):
        part = sk.crop((sx, sy, sx + w, sy + h))
        if over:
            im.alpha_composite(part, (dx, dy))
        else:
            im.paste(part, (dx, dy))
    put(8, 8, 8, 8, 4, 0)            # head
    put(40, 8, 8, 8, 4, 0, True)     # hat
    put(20, 20, 8, 12, 4, 8)         # body
    put(44, 20, 4, 12, 0, 8)         # right arm (on the left)
    put(36, 52, 4, 12, 12, 8)        # left arm
    put(4, 20, 4, 12, 4, 20)         # right leg
    put(20, 52, 4, 12, 8, 20)        # left leg
    return im.resize((27, 54), Image.NEAREST)


def font_bits(jar):
    """ascii.png: 8 x 8 glyphs, a byte a row (bit 0 the left column); widths as FontRenderer measures them"""
    f = jar.image('font/ascii')
    bits, widths = [], []
    for c in range(32, 128):
        gx, gy = (c % 16) * 8, (c // 16) * 8
        rows = []
        right = -1
        for y in range(8):
            v = 0
            for x in range(8):
                if f.getpixel((gx + x, gy + y))[3] >= 128:
                    v |= 1 << x
                    right = max(right, x)
            rows.append(v)
        bits += rows
        widths.append(4 if c == 32 else right + 2)
    return bits, widths


# big GUI pictures, kept as runs: name, picture
def gui_images(jar):
    inv = jar.image('gui/container/inventory').crop((0, 0, 176, 166))
    craft = jar.image('gui/container/crafting_table').crop((0, 0, 176, 166))
    furn = jar.image('gui/container/furnace').crop((0, 0, 176, 166))
    g54 = jar.image('gui/container/generic_54')
    chest = Image.new('RGBA', (176, 71 + 96))
    chest.paste(g54.crop((0, 0, 176, 71)), (0, 0))
    chest.paste(g54.crop((0, 126, 176, 222)), (0, 71))
    w = jar.image('gui/widgets')
    tab = jar.image('gui/container/creative_inventory/tab_items').crop((0, 0, 195, 136))
    surv = jar.image('gui/container/creative_inventory/tab_inventory').crop((0, 0, 195, 136))
    return [('inventory', inv), ('crafting_table', craft), ('furnace', furn), ('chest', chest), ('creative', tab),
            ('tab_inventory', surv)]


GUI_SPRITES = [
    ('flame', 'gui/container/furnace', 176, 0, 14, 14),
    ('arrow', 'gui/container/furnace', 176, 14, 24, 17),
    ('armor_full', 'gui/icons', 34, 9, 9, 9),
    ('armor_half', 'gui/icons', 25, 9, 9, 9),
    ('bubble_pop', 'gui/icons', 25, 18, 9, 9),
    ('heart_hit', 'gui/icons', 25, 0, 9, 9),
    ('tab_top', 'gui/container/creative_inventory/tabs', 0, 0, 28, 32),
    ('tab_top_sel', 'gui/container/creative_inventory/tabs', 0, 32, 28, 32),
    ('tab_bottom', 'gui/container/creative_inventory/tabs', 0, 64, 28, 32),
    ('tab_bottom_sel', 'gui/container/creative_inventory/tabs', 0, 96, 28, 32),
    ('scroller', 'gui/container/creative_inventory/tabs', 232, 0, 12, 15),
    ('button', 'gui/widgets', 0, 66, 200, 20),
    ('button_hover', 'gui/widgets', 0, 86, 200, 20),
    ('button_off', 'gui/widgets', 0, 46, 200, 20),
    # the game mode switcher (sin + cos, as Minecraft's F3 + F4): its panel, the
    # slot a mode sits in, and the outline of the one selected. From the modern
    # client, as modern_rel() lets gui/sprites and this container through.
    ('gm_bg', 'gui/container/gamemode_switcher', 0, 0, 125, 75),
    ('gm_slot', 'gui/sprites/gamemode_switcher/slot', 0, 0, 26, 26),
    ('gm_sel', 'gui/sprites/gamemode_switcher/selection', 0, 0, 26, 26),
]


def main():
    args = sys.argv[1:]
    extra = []
    if '--eggs' in args:
        i = args.index('--eggs')
        if i + 1 >= len(args):
            sys.exit('--eggs needs a directory')
        extra.append(args[i + 1])
        args = args[:i] + args[i + 2:]
    if args[:1] == ['--modern']:
        import modern_map
        root, fb = args[1], args[2] if len(args) > 2 else None
        jar = modern_map.ModernSource(root, fb, extra=extra)
    else:
        jar = Jar(args[0], extra=extra)
    names, px, pals, flags = pack_textures(jar)
    out = ['/* Generated by tools/pack.py from Minecraft 1.8.8\'s client.jar: textures and block tables. */',
           '#include "data.h"', '']
    out.append(f'const uint8_t tex_px[{len(names)}][128] = {{')
    for p in px:
        bs = [p[i] | (p[i + 1] << 4) for i in range(0, 256, 2)]
        out.append('  {' + ','.join(str(v) for v in bs) + '},')
    out.append('};')
    out.append(f'const uint16_t tex_pal[{len(names)}][16] = {{')
    for pal in pals:
        out.append('  {' + ','.join(f'0x{rgb565(*c):04X}' for c in pal) + '},')
    out.append('};')
    out.append(f'const uint8_t tex_flags[{len(names)}] = {{' + ','.join(str(f) for f in flags) + '};')
    out.append('')
    NST = blocks.NSTATES   # the per-state tables: 9 bits a state (see blocks.h)
    def pad(rows, fill='0'):
        """the table's rows, padded to NSTATES (the states past B_COUNT are none)"""
        return rows + [fill] * (NST - len(rows))

    out.append(f'const uint16_t blk_tex[NSTATES][6] = {{')
    for s in blocks.S:
        out.append('  {' + ','.join(str(v) for v in faces(s, names)) + '},   /* ' + s['name'] + ' */')
    out.append('  ' + ', '.join('{0,0,0,0,0,0}' for _ in range(NST - len(blocks.S))) + '   /* unused states */')
    out.append('};')
    models = blocks.MODELS
    out.append('const uint8_t blk_model[NSTATES] = {' + ','.join(pad([str(models.index(s['model'])) for s in blocks.S])) +
               '};')
    out.append('const uint8_t blk_light[NSTATES] = {' + ','.join(pad([str(s['light']) for s in blocks.S])) + '};')
    # shapes
    nb, bx = [], []
    for st in blocks.S:
        b = boxes_of(st)
        nb.append(len(b))
        b = (b + [(0, 0, 0, 0, 0, 0)] * 2)[:2]
        bx.append('{' + ','.join('{' + ','.join(str(v) for v in box) + '}' for box in b) + '}')
    out.append('const uint8_t blk_nbox[NSTATES] = {' + ','.join(pad([str(v) for v in nb])) + '};')
    out.append('const int8_t blk_box[NSTATES][2][6] = {' + ','.join(pad(bx, '{{0,0,0,0,0,0},{0,0,0,0,0,0}}')) + '};')
    # a front (furnaces, pumpkins, chests: turned to the open side) or a bed's end
    fr = []
    for st in blocks.S:
        t = st['tex']
        fr.append(names.index(t['front']) if isinstance(t, dict) and 'front' in t else 0)
    out.append('const uint16_t blk_front[NSTATES] = {' + ','.join(pad([str(v) for v in fr])) + '};')
    fl = []
    for s in blocks.S:
        f = TINT[s['tint']]
        if s['model'] in ('cube',):
            f |= BF_OPAQUE
        if s['model'] in ('cube', 'leaves', 'glass', 'slab', 'cactus', 'fence', 'pane', 'door', 'layer', 'stairs',
                          'chest', 'bed'):
            f |= BF_SOLID
        fl.append(f)
    out.append('const uint8_t blk_flags[NSTATES] = {' + ','.join(pad([str(v) for v in fl])) + '};')
    out.append('const uint8_t blk_tool[NSTATES] = {' +
               ','.join(pad([str(blocks.TOOLS.index(s['tool'])) for s in blocks.S])) + '};')
    out.append('const uint8_t blk_level[NSTATES] = {' + ','.join(pad([str(s['level']) for s in blocks.S])) + '};')
    # hardness in twentieths (255: unbreakable)
    out.append('const uint8_t blk_hard[NSTATES] = {' +
               ','.join(pad([str(255 if s['hard'] < 0 or s['hard'] >= 12.7 else int(round(s['hard'] * 20)))
                            for s in blocks.S])) + '};')
    out.append('const uint8_t blk_id[NSTATES] = {' + ','.join(pad([str(s['id']) for s in blocks.S])) + '};')
    out.append('const uint8_t blk_meta[NSTATES] = {' + ','.join(pad([str(s['meta']) for s in blocks.S])) + '};')
    # NOTE for the superflat customizer (gen.c flat_parse_block): a preset layer may name any
    # block by these same Minecraft numeric ids, as id or id:meta (e.g. 1 stone, 2 grass, 3 dirt,
    # 7 bedrock, 12 sand, 20 glass, 44 smooth stone slab, 159:14 red terracotta, 172 terracotta).
    # Every id:meta pair listed in blocks.py (i.e. with a B_* state) is legal; anything else is
    # rejected with a message, never placed.
    # GUI sprites and block icons: 4 bits a pixel, a palette each; index 0 see-through
    sprites = [(n, jar.image(f).crop((x, y, x + w, y + h))) for n, f, x, y, w, h in SPRITES]
    # the moon's faintest halo left out: cut to its middle, it would show as a square
    for k, (n, im) in enumerate(sprites):
        if n.startswith('moon_'):
            im = im.copy()
            im.putdata([(r, g, b, 0 if r + g + b < 60 else 255) for r, g, b, a in pixels(im)])
            sprites[k] = (n, im)
    for st in blocks.S:
        if st['tex'] is not None:
            sprites.append(('icon_' + st['name'].lower(), iso_icon(jar, st, None)))
    first_item = len(sprites)
    for it in items.I:
        sprites.append(('item_' + it['name'].lower(), item_icon(jar, it)))
    first_gui = len(sprites)
    sprites += [(n, jar.image(f).crop((x, y, x + w, y + h))) for n, f, x, y, w, h in GUI_SPRITES]
    for part in ('helmet', 'chestplate', 'leggings', 'boots'):
        sprites.append(('slot_' + part, first_frame(jar.image('items/empty_armor_slot_' + part))))
    sprites.append(('steve', steve_front(jar)))
    sprites.append(('arm', jar.image('entity/steve').crop((40, 16, 56, 32))))   # the right arm's box, all six faces
    bg = first_frame(jar.image('gui/options_background'))
    bg.putdata([(r * 0x40 // 255, g * 0x40 // 255, b * 0x40 // 255, 255) for r, g, b, a in pixels(bg)])
    sprites.append(('dirt_bg', bg))
    # the ender eye: the spectator slot's icon in the game mode switcher, as Minecraft's
    # is. The eye of ender is not an item of the game (there is no End): its picture is
    # here for that one icon and nothing else
    sprites.append(('eye_ender', first_frame(jar.image('items/ender_eye'))))
    data, offs, pals, dims, flags2 = [], [], [], [], []
    for n, im in sprites:
        bs, pal, alpha = pack_sprite(im)
        offs.append(len(data))
        data += bs
        pals.append(pal)
        dims.append(im.size)
        flags2.append(1 if alpha else 0)
    out.append(f'const uint8_t spr_px[{len(data)}] = {{' + ','.join(str(v) for v in data) + '};')
    spr_bytes = len(data)
    out.append(f'const uint32_t spr_off[{len(sprites)}] = {{' + ','.join(str(v) for v in offs) + '};')
    out.append(f'const uint8_t spr_w[{len(sprites)}] = {{' + ','.join(str(w) for w, h in dims) + '};')
    out.append(f'const uint8_t spr_h[{len(sprites)}] = {{' + ','.join(str(h) for w, h in dims) + '};')
    out.append(f'const uint8_t spr_alpha[{len(sprites)}] = {{' + ','.join(str(v) for v in flags2) + '};')
    out.append(f'const uint16_t spr_pal[{len(sprites)}][16] = {{')
    for pal in pals:
        out.append('  {' + ','.join(f'0x{rgb565(*c):04X}' for c in pal) + '},')
    out.append('};')
    icon_of = []
    k = len(SPRITES)
    for st in blocks.S:
        if st['tex'] is not None:
            icon_of.append(k)
            k += 1
        else:
            icon_of.append(0xFFFF)
    out.append('const uint16_t blk_icon[NSTATES] = {' + ','.join(pad([str(v) for v in icon_of], '0xFFFF')) + '};')
    # items: the block state a block counts as, then each item's tables
    def bi(n):
        v = items.block_item(n)
        return 0xFFFF if v is None else v
    out.append('const uint16_t blk_item[NSTATES] = {' + ','.join(pad([str(bi(st['name'])) for st in blocks.S], '0xFFFF')) +
               '};')
    out.append('const uint16_t blk_fuel[NSTATES] = {' +
               ','.join(pad([str(items.fuel_of(i)) for i in range(len(blocks.S))])) + '};')
    out.append('const char *const blk_label[NSTATES] = {' +
               ','.join(pad(['"' + items.LABELS[st['name']] + '"' if st['name'] in items.LABELS else '0'
                            for st in blocks.S], '0')) + '};')
    I = items.I
    out.append('const char *const it_label[N_ITEMS] = {' + ','.join('"' + it['label'] + '"' for it in I) + '};')
    out.append('const uint16_t it_icon[N_ITEMS] = {' + ','.join(str(first_item + k) for k in range(len(I))) + '};')
    out.append('const uint8_t it_kind[N_ITEMS] = {' + ','.join(str(items.KINDS.index(it['kind'])) for it in I) + '};')
    out.append('const uint8_t it_stack[N_ITEMS] = {' + ','.join(str(it['stack']) for it in I) + '};')
    out.append('const uint16_t it_dur[N_ITEMS] = {' + ','.join(str(it['dur']) for it in I) + '};')
    out.append('const uint8_t it_tier[N_ITEMS] = {' + ','.join(str(it['tier']) for it in I) + '};')
    out.append('const uint8_t it_a[N_ITEMS] = {' + ','.join(str(it['a']) for it in I) + '};')
    out.append('const uint8_t it_b[N_ITEMS] = {' + ','.join(str(it['b']) for it in I) + '};')
    out.append('const uint8_t it_place[N_ITEMS] = {' +
               ','.join(str(items.BNAMES.index(it['place']) if it['place'] else 255) for it in I) + '};')
    out.append('const uint16_t it_fuel[N_ITEMS] = {' + ','.join(str(it['fuel']) for it in I) + '};')
    # crafting: out, count, width (0: shapeless), height (shapeless: how many), ingredients
    rec = []
    for o, n, rows, keys in items.R:
        if rows:
            w, hh = max(len(r) for r in rows), len(rows)
            ins = []
            for r in rows:
                for c in r.ljust(w):
                    ins.append(0 if c == ' ' else items.resolve(keys[c]))
        else:
            w, hh = 0, len(keys)
            ins = [items.resolve(k) for k in keys]
        ins += [0] * (9 - len(ins))
        rec.append('{%d,%d,%d,%d,{%s}}' % (items.resolve(o), n, w, hh, ','.join(str(v) for v in ins)))
    out.append(f'const Recipe recipes[{len(rec)}] = {{' + ','.join(rec) + '};')
    out.append(f'const uint16_t smelting[{len(items.SMELT)}][2] = {{' +
               ','.join('{%d,%d}' % (items.resolve(a), items.resolve(b)) for a, b in items.SMELT) + '};')
    for g, members in items.GROUP_OF.items():
        out.append(f'const uint16_t group_{g[2:].lower()}[] = {{' +
                   ','.join(str(items.BNAMES.index(m)) for m in members) + ',0xFFFF};')
    # mob skins: 64 x 32, 4 bits a texel, index 0 see-through
    SKINS = ['zombie/zombie', 'skeleton/skeleton', 'creeper/creeper', 'spider/spider', 'pig/pig', 'cow/cow',
             'sheep/sheep', 'sheep/sheep_fur', 'chicken']
    out.append(f'const uint8_t skin_px[{len(SKINS)}][1024] = {{')
    spal = []
    for n in SKINS:
        im = skin_sheet(jar, n)
        bs, pal, alpha = pack_sprite(im)
        out.append('  {' + ','.join(str(v) for v in bs) + '},')
        spal.append(pal)
    out.append('};')
    out.append(f'const uint16_t skin_pal[{len(SKINS)}][16] = {{')
    for pal in spal:
        out.append('  {' + ','.join(f'0x{rgb565(*c):04X}' for c in pal) + '},')
    out.append('};')
    # the cracks of a block being broken: 10 stages, 1 bit a texel (dark where set)
    cr = []
    for st in range(10):
        im = jar.image(f'blocks/destroy_stage_{st}')
        bits = [0] * 32
        for i, (r, g, b, a) in enumerate(pixels(im)):
            if a >= 100:
                bits[i >> 3] |= 1 << (i & 7)
        cr.append(bits)
    out.append('const uint8_t cracks[10][32] = {' + ','.join('{' + ','.join(str(v) for v in b) + '}' for b in cr) + '};')
    # the creative inventory: each tab's items, in id order (blocks, then items).
    # Block states and items share the ids from 256 up (block state 256 is item
    # id 256 too), so the two walks say what each id is: the states as states,
    # the items as items. Search Items holds everything, once.
    tab_lists = [[] for _ in items.TABS]
    for iid in range(len(blocks.S)):
        tabs = items.tab_of(iid, True)
        if tabs:
            for t in tabs:
                tab_lists[t].append(iid)
            tab_lists[items.T_SEARCH].append(iid)
    for k in range(len(items.I)):
        iid = 256 + k
        tabs = items.tab_of(iid, False)
        if tabs:
            for t in tabs:
                tab_lists[t].append(iid)
            if tab_lists[items.T_SEARCH].count(iid) == 0:
                tab_lists[items.T_SEARCH].append(iid)
    for t in range(len(items.TABS)):
        tab_lists[t] = items.tab_order(t, tab_lists[t])
    flat, starts = [], []
    for L in tab_lists:
        starts.append(len(flat))
        flat += L
    starts.append(len(flat))
    out.append(f'const uint16_t tab_items[{len(flat)}] = {{' + ','.join(str(v) for v in flat) + '};')
    out.append(f'const uint16_t tab_start[{len(starts)}] = {{' + ','.join(str(v) for v in starts) + '};')
    out.append(f'const char *const tab_name[{len(items.TABS)}] = {{' + ','.join('"' + t + '"' for t in items.TABS) + '};')
    # the font
    fb, fw = font_bits(jar)
    out.append('const uint8_t font_bits[96 * 8] = {' + ','.join(str(v) for v in fb) + '};')
    out.append('const uint8_t font_w[96] = {' + ','.join(str(v) for v in fw) + '};')
    # GUI pictures as runs
    gi = gui_images(jar)
    allrle, offs, pals, sizes, rowoffs = [], [], [], [], []
    for n, im in gi:
        data, rows, pal = rle_image(im)
        offs.append(len(allrle))
        rowoffs.append(len(allrle))
        allrle += data
        pals.append(pal)
        sizes.append((im.size, rows))
    out.append(f'const uint8_t img_rle[{len(allrle)}] = {{' + ','.join(str(v) for v in allrle) + '};')
    rowtab, rowstart = [], []
    for (wh, rows), base in zip(sizes, offs):
        rowstart.append(len(rowtab))
        rowtab += [base + r for r in rows]
    out.append(f'const uint32_t img_rows[{len(rowtab)}] = {{' + ','.join(str(v) for v in rowtab) + '};')
    out.append(f'const uint16_t img_row0[{len(gi)}] = {{' + ','.join(str(v) for v in rowstart) + '};')
    out.append(f'const uint8_t img_w[{len(gi)}] = {{' + ','.join(str(wh[0]) for wh, r in sizes) + '};')
    out.append(f'const uint8_t img_h[{len(gi)}] = {{' + ','.join(str(wh[1]) for wh, r in sizes) + '};')
    out.append(f'const uint16_t img_pal[{len(gi)}][16] = {{')
    for pal in pals:
        out.append('  {' + ','.join(f'0x{rgb565(*c):04X}' for c in pal) + '},')
    out.append('};')
    gui_names = [n for n, im in gi]
    extra = [n for n, *_ in GUI_SPRITES] + ['slot_helmet', 'slot_chestplate', 'slot_leggings', 'slot_boots', 'steve',
                                            'arm', 'dirt_bg']
    # Minecraft's clouds: 256 x 256, 1 bit a cell
    cl = jar.image('environment/clouds')
    bits = bytearray(256 * 256 // 8)
    for i, (r, g, b, a) in enumerate(pixels(cl)):
        if a >= 128:
            bits[i >> 3] |= 1 << (i & 7)
    out.append('const uint8_t clouds[8192] = {' + ','.join(str(v) for v in bits) + '};')
    # rain and snow (64 x 256): their streaks and flakes as runs down a texel column,
    # {row, column, length, alpha}, by row; texels too faint to pass 1.8's alpha test (0.1) left out
    for name in ('rain', 'snow'):
        im = jar.image('environment/' + name)
        w, h = im.size
        px = im.load()
        runs = []
        for x in range(w):
            y = 0
            while y < h:
                if px[x, y][3] > 25:
                    y0, al = y, []
                    while y < h and px[x, y][3] > 25:
                        al.append(px[x, y][3])
                        y += 1
                    runs.append((y0, x, len(al), round(sum(al) / len(al))))
                else:
                    y += 1
        runs.sort()
        idx = [next((i for i, r in enumerate(runs) if r[0] >= row), len(runs)) for row in range(h + 1)]
        out.append(f'const uint8_t {name}_run[{len(runs)}][4] = {{' + ','.join('{%d,%d,%d,%d}' % r for r in runs) + '};')
        out.append(f'const uint8_t {name}_idx[{h + 1}] = {{' + ','.join(str(v) for v in idx) + '};')
    g, f, w = biome_colors(jar)
    out.append('const uint16_t biome_grass[256] = {' + ','.join(f'0x{rgb565(*c):04X}' for c in g) + '};')
    out.append('const uint16_t biome_foliage[256] = {' + ','.join(f'0x{rgb565(*c):04X}' for c in f) + '};')
    out.append('const uint16_t biome_water[256] = {' + ','.join(f'0x{rgb565(*c):04X}' for c in w) + '};')
    temps = [BIOMES.get(i & 127 if i >= 128 else i, (0.5, 0.5))[0] for i in range(256)]
    out.append('const int8_t biome_temp[256] = {' + ','.join(str(int(round(t * 50))) for t in temps) + '};   /* x 50 */')
    # BiomeGenBase.enableRain (off in deserts, savannas, mesas) and enableSnow; mutated biomes copy them
    no_rain, snowy = {2, 8, 9, 17, 35, 36, 37, 38, 39}, {10, 11, 12, 13, 26, 30, 31}
    rain = [(0 if (i & 127) in no_rain else 1) | (2 if (i & 127) in snowy else 0) for i in range(256)]
    out.append('const uint8_t biome_rain[256] = {' + ','.join(str(v) for v in rain) + '};')
    open(os.path.join(ROOT, 'src', 'data.c'), 'w').write('\n'.join(out) + '\n')

    h = ['/* Generated by tools/pack.py: textures and block tables (see data.c). */', '#ifndef NB_DATA_H',
         '#define NB_DATA_H', '#include <stdint.h>', '#include "blocks.h"', '',
         f'#define NTEX {len(names)}',
         '/* tex_flags: bits 0-4, the first palette index tinted by the biome (16: none); 0x20: has see-through texels */',
         'extern const uint8_t tex_px[NTEX][128];', 'extern const uint16_t tex_pal[NTEX][16];',
         'extern const uint8_t tex_flags[NTEX];', '',
         '/* faces: -Y +Y -Z +Z -X +X; 16 bits: the texture list passed 256 with the concrete */',
         'extern const uint16_t blk_tex[NSTATES][6];',
         'extern const uint8_t blk_model[NSTATES], blk_light[NSTATES], blk_flags[NSTATES], blk_tool[NSTATES];',
         'extern const uint8_t blk_nbox[NSTATES];   /* how many boxes it is made of (0: a whole cube) */',
         'extern const int8_t blk_box[NSTATES][2][6];   /* the boxes, sixteenths: x0 y0 z0 x1 y1 z1 */',
         'extern const uint16_t blk_front[NSTATES];   /* the texture of its front (0: none) */',
         'extern const uint8_t blk_level[NSTATES], blk_hard[NSTATES], blk_id[NSTATES], blk_meta[NSTATES];',
         '/* blk_flags: bits 0-2 the tint (1 grass, 2 foliage, 3 spruce, 4 birch, 5 water, 6 lily pad) */',
         'extern const uint16_t biome_grass[256], biome_foliage[256], biome_water[256];',
         'extern const int8_t biome_temp[256];   /* temperature x 50 */',
         'extern const uint8_t biome_rain[256];   /* 1: it rains (or snows), 2: a snowy biome */',
         '/* rain and snow: runs down a texel column {row, column, length, alpha}, by row; idx: the first run at or below a row */',
         'extern const uint8_t rain_run[][4], snow_run[][4], rain_idx[257], snow_idx[257];',
         f'#define BF_OPAQUE {BF_OPAQUE}', f'#define BF_SOLID {BF_SOLID}', '#define BF_TINT 7', '']
    for i, n in enumerate(names):
        h.append(f'#define TX_{n.upper().replace("@", "")} {i}')
    h += ['', 'extern const uint8_t spr_px[], spr_w[], spr_h[], spr_alpha[];', 'extern const uint32_t spr_off[];',
          'extern const uint16_t spr_pal[][16];', 'extern const uint16_t blk_icon[NSTATES];   /* sprite of its icon */']
    for i, sp in enumerate(SPRITES):
        h.append(f'#define SP_{sp[0].upper()} {i}')
    for i, n in enumerate(extra):
        h.append(f'#define SP_{n.upper()} {first_gui + i}')
    h += ['', '#include "items.h"',
          'extern const uint16_t blk_item[NSTATES];   /* the item a block state counts as (0xFFFF: none) */',
          'extern const uint16_t blk_fuel[NSTATES];   /* ticks it burns in a furnace */',
          'extern const char *const blk_label[NSTATES], *const it_label[N_ITEMS];',
          'extern const uint16_t it_icon[N_ITEMS], it_dur[N_ITEMS], it_fuel[N_ITEMS];',
          '/* it_tier: the tool or armour material; it_a: attack damage, armour points or food;'
          ' it_b: saturation x 10 */',
          'extern const uint8_t it_kind[N_ITEMS], it_stack[N_ITEMS], it_tier[N_ITEMS], it_a[N_ITEMS], it_b[N_ITEMS];',
          'extern const uint8_t it_place[N_ITEMS];   /* the block it places (255: none) */',
          'typedef struct { uint16_t out; uint8_t n, w, h; uint16_t in[9]; } Recipe;   /* w 0: shapeless, h of them */',
          f'#define N_RECIPES {len(items.R)}', 'extern const Recipe recipes[N_RECIPES];',
          f'#define N_SMELTING {len(items.SMELT)}', 'extern const uint16_t smelting[N_SMELTING][2];']
    for g in items.GROUP_OF:
        h.append(f'extern const uint16_t group_{g[2:].lower()}[];   /* ends with 0xFFFF */')
    h += ['extern const uint8_t skin_px[][1024];   /* mob skins, 64 x 32 */', 'extern const uint16_t skin_pal[][16];',
          'enum { SKIN_ZOMBIE, SKIN_SKELETON, SKIN_CREEPER, SKIN_SPIDER, SKIN_PIG, SKIN_COW, SKIN_SHEEP, SKIN_SHEEP_FUR,'
          ' SKIN_CHICKEN };',
          'extern const uint8_t cracks[10][32];   /* destroy stages, 1 bit a texel */']
    h += [f'extern const uint16_t tab_items[], tab_start[{len(items.TABS) + 1}];   /* the creative tabs\' items */',
          f'extern const char *const tab_name[{len(items.TABS)}];']
    h += ['extern const uint8_t font_bits[96 * 8], font_w[96];   /* characters 32-127 */',
          'extern const uint8_t img_rle[], img_w[], img_h[];', 'extern const uint32_t img_rows[];',
          'extern const uint16_t img_row0[], img_pal[][16];']
    for i, n in enumerate(gui_names):
        h.append(f'#define IMG_{n.upper()} {i}')
    h += ['', '#endif', '']
    open(os.path.join(ROOT, 'src', 'data.h'), 'w').write('\n'.join(h))
    print(f'{len(names)} textures, {len(blocks.S)} block states, {len(items.I)} items, {len(items.R)} recipes, '
          f'{len(sprites)} sprites ({spr_bytes} bytes), GUI runs {len(allrle)} bytes')
    if hasattr(jar, 'report'):
        n, fb = jar.report()
        print(f'modern textures: {n} from the modern set, {len(fb)} kinds from the 1.8.8 fallback:')
        for f in fb:
            print(f'  fallback {f}')


BF_OPAQUE = 8
BF_SOLID = 16

if __name__ == '__main__':
    main()
