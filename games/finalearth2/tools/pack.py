#!/usr/bin/env python3
"""Packs The Final Earth 2's sprites and fonts for the calculator.

    python3 -I tools/pack.py [REFERENCE]

REFERENCE is the original's web version (default below). Writes src/data.bin
and src/data.h next to this folder and prints what each part takes.

Sprites: every frame of sprites.json (a frame is often a strip of frames the
game cuts at run time), cut from sprites.png, except the ones in DROPPED.
Each sprite has its own palette (RGB565 and the original's 8-bit alpha) and
its rows are coded as tokens, 4 bits a pixel when it has at most 16 colours,
8 bits otherwise:

    0x00-0x3F  skip n + 1 clear pixels
    0x40-0x7F  n + 1 pixels follow (4-bit: two a byte, low nibble first)
    0x80-0xEF  4-bit: (t >> 4) - 7 pixels of colour t & 15
    0xF0-0xFF  4-bit: the next byte + 8 pixels of colour t & 15
    0x80-0xFF  8-bit: (t & 127) + 1 pixels of the colour in the next byte

A row ends where the next one starts (clear pixels at its end are left out).
Sprites with more than 256 colours (only the colour picker's circle) keep
RGB565 pixels instead of indexes ("direct"):

    0x00-0x3F skip, 0x40-0x7F n + 1 opaque pixels (u16 each),
    0x80-0xBF n + 1 pixels of one u16 colour, 0xC0-0xFF n + 1 pixels of u16 + alpha byte

Fonts: the five BMFont fonts, 1 bit a pixel, for the characters the game's
English texts and citizens' names use (ASCII and a few accented letters),
with their offsets, advances and kerning.

data.bin (little-endian):
    0   'FE2D'
    4   u16 sprites, u16 fonts
    8   u32 offset of the sprite table: per sprite u32 offset of its data, u16 w, u16 h
    12  u32 offset of the font table: per font u32 offset of its data
  sprite data (4-aligned): u8 colours, u8 mode (0 4-bit, 1 8-bit, 2 direct; +4: u32 row offsets),
    u8 slabs, u8 0, u16 RGB565[colours], u8 alpha[colours], pad to 4, then per slab (columns x0 to
    x1 - 1, coded on their own): u16 x0, u16 x1, u32 offset of its rows (from the sprite's data);
    a slab's rows (4-aligned): u16 or u32 row offsets[h + 1] (from the end of the table), row data
  font data (4-aligned): u8 line height, u8 glyphs, u16 kerning pairs, u32 offset of the bits
    (from the font's start), glyphs (u16 first bit, i8 x offset, i8 y offset, u8 w, u8 h,
    u8 advance, u8 0), kerning pairs (u8 first, u8 second, i8 amount, u8 0) sorted by second
    then first, bits (each glyph's rows, w bits a row, one after the other, low bit first)
"""
import json
import os
import re
import struct
import sys

from PIL import Image

REF = os.environ.get("FE2_REF", os.path.expanduser("~/Documents/Assets/gamesrc/finalearth2/web14"))
HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, '..', 'src')

# Sprites left out. Each is used only by content the port leaves out, or by
# nothing at all in this version of the game.
DROPPED = {
    # premium buildings and city elements (paid versions only; bridges need
    # bridgesInfo.json, which the free version doesn't have)
    'spr_alienfarm', 'spr_alienfarm_buttonback', 'spr_alienfarm_crops',
    'spr_aliennightclub', 'spr_aliennightclub_alienthing', 'spr_lightbeam_aliennightclub',
    'spr_aquarium', 'spr_aquarium_buttonback',           # Aquarium, UnderwaterHouse
    'spr_underwaterhouse', 'spr_underwaterhouse_buttonback',
    'spr_graphenefactory', 'spr_graphenefactory_frames', 'spr_graphenefactory_idle',
    'spr_boostrange_onright', 'spr_stoneteleporter_graphenefactory_connection',   # GrapheneFactory
    'spr_grapheneresearchcenter',                      # GrapheneLab
    'spr_futuristichome', 'spr_futuristichome_interiors',
    'spr_landingpad_tunnel', 'spr_landingpadtunnel_bg', 'spr_landingpadtunnel_buttonback',
    'spr_dome_treeplantation', 'spr_dome_treeplantation_buttonback',
    'spr_treeplantation_dome_trees', 'spr_treeplantation_dome_trees_grow',
    'spr_rocketfuelfactory', 'spr_rocketfuelfactory_frames', 'spr_rocketfuelfactory_idle',
    'spr_otherworld_plants', 'spr_otherworldlyZoo_buttonback',
    'spr_otherworldcreature_1', 'spr_otherworldcreature_2', 'spr_otherworldcreature_3',
    'spr_otherworldcreature_4',                        # the zoo's animals
    'spr_oldesttreeintheuniverse',
    'spr_techmuseum', 'spr_techmuseum_buttonback', 'spr_techmuseum_majordisplay_1',
    'spr_techmuseum_majordisplay_2', 'spr_techmuseum_minordisplay', 'spr_techmuseum_minordisplay_ext',
    'spr_wizardhatcafe',
    'spr_trainstation_buttonbg',
    'spr_bridge', 'spr_bridge_potential', 'spr_woodenbridge', 'spr_woodenbridge_potential',
    'spr_reinforcedbridge', 'spr_reinforcedbridge_potential',
    'spr_botanicalbridge', 'spr_botanicalbridge_potential',
    # holiday content: Christmas, Halloween, snow and winter
    'spr_christmasmarket', 'spr_christmastree', 'spr_christmastree_bg', 'spr_upgrade_christmastree',
    'spr_horrorgamehall', 'spr_horrorgamehall_lightup', 'spr_horrorgamehall_lightup2',
    'spr_indoorpark_dark', 'spr_spiderweb', 'spr_ghostlygarden_buttonbg',   # GhostlyPark
    'spr_hauntedlaboratory', 'spr_hauntedlaboratory_failed',
    'spr_upgrade_pumpkin', 'spr_halloween',
    'spr_santahat', 'spr_snow', 'spr_snowman', 'spr_lanterns', 'spr_forest_snow', 'spr_forest_snow_grow',
    # ads, stores, upsells and links to other sites
    'app_store_badge', 'discord_badge', 'google_play_badge', 'steam_badge', 'steam_badge2',
    'steam_badge_coolmath', 'spr_badge_premium', 'spr_button_upsell', 'spr_button_upsell_ignore',
    'spr_twitter', 'spr_mailinglist', 'spr_discord',
    # the music and sound buttons (no sound on the calculator)
    'spr_music', 'spr_sound',
    # used by no code of this version
    'Naamloos', 'elephant', 'src/potential_hacker_symbol', 'spr_apple', 'spr_aquarium_plants',
    'spr_bloomrestaurant_alt', 'spr_boardgames', 'spr_divingsuit', 'spr_festival_podium_large',
    'spr_floatingplatform_bottom', 'spr_floatingplatform_top', 'spr_herbgarden_altplants',
    'spr_highlightedbuilding', 'spr_icon_followcitizen_small', 'spr_icon_play', 'spr_icon_stats',
    'spr_icon_traveltime', 'spr_icon_triplespeed', 'spr_inlinebutton', 'spr_inlinebutton_2',
    'spr_movingplatform', 'spr_movingplatform_bottom', 'spr_rail', 'spr_railwaybridge', 'spr_rock_2',
    'spr_rocket', 'spr_rocket_part1', 'spr_rocket_part1_down', 'spr_rocket_part2', 'spr_rocket_part3',
    'spr_rocket_part4', 'spr_rocketlaunchplatform', 'spr_rocketlaunchplatform_whilebuilding',
    'spr_rockettrail', 'spr_rockettrail_small', 'spr_statueofmorehappiness', 'spr_swimmingpool',
    'spr_thievesden', 'spr_train', 'spr_trainrail', 'spr_trainselectorbutton', 'spr_trainstation',
    'spr_trainstation_decoration', 'spr_trainstation_l', 'spr_trainstation_r',
    'spr_transparentbutton_mini', 'spr_vr_headset',
}

FONTS = [('FONT_STANDARD', 'standard'), ('FONT_10', 'standard10'), ('FONT_15', 'standard15'),
         ('FONT_16', 'standard16'), ('FONT_18', 'standard18')]
# characters packed: ASCII, then the others the English texts and the names use
EXTRA_CHARS = [0xDF, 0xE9, 0xEB, 0xFC]   # ß é ë ü
CHARS = list(range(32, 127)) + EXTRA_CHARS
SLAB = 48


def c565(r, g, b):
    """the nearest RGB565 colour"""
    return ((r * 31 + 127) // 255) << 11 | ((g * 63 + 127) // 255) << 5 | ((b * 31 + 127) // 255)


def enum_name(name):
    n = name[4:] if name.startswith('spr_') else name
    return 'SPR_' + re.sub(r'[^A-Z0-9]', '_', n.upper())


def encode_row(row, mode):
    """the tokens of one row (indexes, None for clear; direct: (rgb565, alpha) or None)"""
    out = bytearray()
    end = len(row)
    while end and row[end - 1] is None:
        end -= 1
    x = 0
    if mode == 2:
        while x < end:
            s = x
            if row[x] is None:
                while x < end and row[x] is None and x - s < 64:
                    x += 1
                out.append(x - s - 1)
                continue
            while x < end and row[x] == row[s] and x - s < 64:
                x += 1
            if x - s >= 3:
                out.append(0x80 | (x - s - 1))
                out += struct.pack('<H', row[s][0])
                if row[s][1] != 255:      # a run of a see-through colour: as pixels with alpha
                    del out[-3:]
                    out.append(0xC0 | (x - s - 1))
                    for _ in range(x - s):
                        out += struct.pack('<HB', row[s][0], row[s][1])
                continue
            x = s
            semi = row[x][1] != 255
            while x < end and row[x] is not None and (row[x][1] != 255) == semi and x - s < 64:
                x += 1
            out.append((0xC0 if semi else 0x40) | (x - s - 1))
            for p in row[s:x]:
                out += struct.pack('<HB', *p) if semi else struct.pack('<H', p[0])
        return bytes(out)
    four = mode == 0
    fill_min = 4 if four else 3
    fill_max = 263 if four else 128
    while x < end:
        s = x
        if row[x] is None:
            while x < end and row[x] is None and x - s < 64:
                x += 1
            out.append(x - s - 1)
            continue
        while x < end and row[x] == row[s] and x - s < fill_max:
            x += 1
        if x - s >= fill_min:
            if four and x - s < 8:
                out.append((x - s + 7) << 4 | row[s])
            elif four:
                out += bytes([0xF0 | row[s], x - s - 8])
            else:
                out += bytes([0x80 | (x - s - 1), row[s]])
            continue
        # literal pixels, up to the next clear one or a run worth breaking it for
        x = s + 1
        while x < end and row[x] is not None and x - s < 64:
            y = x
            while y < end and row[y] == row[x]:
                y += 1
            if y - x > fill_min:
                break
            x += 1
        out.append(0x40 | (x - s - 1))
        lit = row[s:x]
        if four:
            for i in range(0, len(lit), 2):
                out.append(lit[i] | (lit[i + 1] << 4 if i + 1 < len(lit) else 0))
        else:
            out += bytes(lit)
    return bytes(out)


def pack_sprite(px, f):
    """the data of one sprite (frame rectangle f of the atlas)"""
    w, h = f['w'], f['h']
    keys = [[None] * w for _ in range(h)]
    pal = {}
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[f['x'] + x, f['y'] + y]
            if a:
                k = (c565(r, g, b), a)
                keys[y][x] = k
                if k not in pal:
                    pal[k] = len(pal)
    if len(pal) > 256:
        # alpha 252 and more blends like 255 (the renderer has 33 levels)
        rows = [[None if k is None else (k[0], 255 if k[1] >= 252 else k[1]) for k in r] for r in keys]
        mode, colours = 2, []
    else:
        mode = 0 if len(pal) <= 16 else 1
        rows = [[None if k is None else pal[k] for k in r] for r in keys]
        colours = sorted(pal, key=pal.get)
    # wide strips are cut into slabs (at clear columns, at least SLAB pixels wide; every frame
    # for the small ones such as citizens), each with its own rows, so that a frame far right
    # is found without walking all the row
    slabs = [(0, w)]
    if w > 64:
        opaque = [any(rows[y][x] is not None for y in range(h)) for x in range(w)]
        segs, x = [], 0
        while x < w:
            if not opaque[x]:
                x += 1
                continue
            s0 = x
            while x < w and opaque[x]:
                x += 1
            if segs and s0 - segs[-1][0] < (SLAB if h > 8 else 1):
                segs[-1][1] = x
            else:
                segs.append([s0, x])
        if segs:
            slabs = [(a, b) for a, b in segs]
        if h <= 8 and len(slabs) == 1:
            # small strips without clear columns between frames (citizens: frames 2 wide, 4 apart)
            slabs = [(a, min(a + 4, w)) for a in range(0, w, 4)]
    assert len(slabs) < 256, 'too many slabs'
    coded = [[encode_row(r[a:b], mode) for r in rows] for a, b in slabs]
    big = any(sum(len(c) for c in sl) > 0xFFFF for sl in coded)
    head = struct.pack('<BBBB', len(colours), mode | (4 if big else 0), len(slabs), 0)
    head += b''.join(struct.pack('<H', c[0]) for c in colours)
    head += bytes(c[1] for c in colours)
    while len(head) & 3:
        head += b'\0'
    at = len(head) + 8 * len(slabs)
    table, body = b'', b''
    st = {'palette': len(head), 'rows': 8 * len(slabs), 'pixels': 0, 'mode': mode, 'slabs': len(slabs)}
    for (a, b), sl in zip(slabs, coded):
        while (at + len(body)) & 3:
            body += b'\0'
        table += struct.pack('<HHI', a, b, at + len(body))
        offs = [0]
        for c in sl:
            offs.append(offs[-1] + len(c))
        body += b''.join(struct.pack('<I' if big else '<H', o) for o in offs) + b''.join(sl)
        st['rows'] += (4 if big else 2) * len(offs)
        st['pixels'] += offs[-1]
    return head + table + body, st


def parse_fnt(path):
    text = open(path, encoding='utf-8').read()
    def attrs(tag):
        return {k: v for k, v in re.findall(r'(\w+)="([^"]*)"', tag)}
    common = attrs(re.search(r'<common [^>]*>', text).group(0))
    page = attrs(re.search(r'<page [^>]*>', text).group(0))['file']
    chars = {}
    for m in re.finditer(r'<char [^>]*>', text):
        a = attrs(m.group(0))
        chars[int(a['id'])] = {k: int(a[k]) for k in ('x', 'y', 'width', 'height', 'xoffset', 'yoffset', 'xadvance')}
    kern = []
    for m in re.finditer(r'<kerning [^>]*>', text):
        a = attrs(m.group(0))
        kern.append((int(a['first']), int(a['second']), int(a['amount'])))
    return int(common['lineHeight']), page, chars, kern


def pack_font(ref, base):
    line_h, page, chars, kern = parse_fnt(os.path.join(ref, 'fonts', base + '.fnt'))
    im = Image.open(os.path.join(ref, 'fonts', page)).convert('RGBA')
    px = im.load()
    glyphs = bytearray()
    bits = []
    for code in CHARS:
        c = chars.get(code)
        if c is None:      # missing: nothing drawn, no advance (BitmapText skips it)
            glyphs += struct.pack('<HbbBBBB', 0, 0, 0, 0, 0, 0, 0)
            continue
        first = len(bits)
        for y in range(c['height']):
            for x in range(c['width']):
                bits.append(1 if px[c['x'] + x, c['y'] + y][3] >= 128 else 0)
        glyphs += struct.pack('<HbbBBBB', first, c['xoffset'], c['yoffset'], c['width'], c['height'],
                              c['xadvance'], 0)
    index = {code: i for i, code in enumerate(CHARS)}
    pairs = sorted((index[b], index[a], n) for a, b, n in kern if a in index and b in index and n)
    kerning = b''.join(struct.pack('<BBbB', a, b, n, 0) for b, a, n in pairs)
    data = bytearray(len(bits) + 7 >> 3)
    for i, v in enumerate(bits):
        if v:
            data[i >> 3] |= 1 << (i & 7)
    head_len = 8
    bits_off = head_len + len(glyphs) + len(kerning)
    head = struct.pack('<BBHI', line_h, len(CHARS), len(pairs), bits_off)
    return head + bytes(glyphs) + kerning + bytes(data), len(pairs)


def main():
    ref = sys.argv[1] if len(sys.argv) > 1 else REF
    atlas = json.load(open(os.path.join(ref, 'sprites.json')))
    im = Image.open(os.path.join(ref, 'sprites.png')).convert('RGBA')
    px = im.load()
    names = sorted(k[:-4] for k in atlas['frames'] if k[:-4] not in DROPPED)
    missing = DROPPED - set(k[:-4] for k in atlas['frames'])
    if missing:
        sys.exit('pack.py: dropped sprites not in the atlas: ' + ', '.join(sorted(missing)))
    enums = [enum_name(n) for n in names]
    if len(set(enums)) != len(enums):
        sys.exit('pack.py: two sprites with the same SPR_ name')

    out = bytearray(16)
    def align(n=4):
        while len(out) % n:
            out.append(0)
    spr_table = len(out)
    out += bytes(8 * len(names))
    stats = {'palette': 0, 'rows': 0, 'pixels': 0, 'table': 8 * len(names), 'slabs': 0}
    modes = [0, 0, 0]
    sizes = []
    for i, n in enumerate(names):
        f = atlas['frames'][n + '.png']['frame']
        align()
        blob, st = pack_sprite(px, f)
        struct.pack_into('<IHH', out, spr_table + 8 * i, len(out), f['w'], f['h'])
        out += blob
        for k in ('palette', 'rows', 'pixels', 'slabs'):
            stats[k] += st[k]
        modes[st['mode']] += 1
        sizes.append((f['w'], f['h']))
    sprite_end = len(out)
    align()
    font_table = len(out)
    out += bytes(4 * len(FONTS))
    font_sizes = []
    for i, (_, base) in enumerate(FONTS):
        align()
        blob, npairs = pack_font(ref, base)
        struct.pack_into('<I', out, font_table + 4 * i, len(out))
        out += blob
        font_sizes.append((base, len(blob), npairs))
    align()
    struct.pack_into('<4sHHII', out, 0, b'FE2D', len(names), len(FONTS), spr_table, font_table)
    open(os.path.join(SRC, 'data.bin'), 'wb').write(out)

    h = ['/* The Final Earth 2: what tools/pack.py packed in data.bin (made by it: do not edit). */',
         '#ifndef FE_DATA_H', '#define FE_DATA_H', '#include <stdint.h>', '',
         'extern const uint8_t fe_data[];   /* data.bin (data.S) */', '',
         '/* sprites: each frame of the original\'s atlas (often a strip of frames, 2 pixels apart) */',
         'enum {']
    for e, n, (w, hh) in zip(enums, names, sizes):
        h.append('  %s,%s/* %s.png, %d x %d */' % (e, ' ' * max(1, 40 - len(e)), n, w, hh))
    h += ['  SPR_COUNT', '};', '', '/* each sprite\'s width and height */']
    for e, (w, hh) in zip(enums, sizes):
        h.append('#define %s_W %d' % (e, w))
        h.append('#define %s_H %d' % (e, hh))
    h += ['', '/* fonts (the original\'s BMFont fonts) */', 'enum { ' + ', '.join(f[0] for f in FONTS) + ', FONT_COUNT };',
          '', '#endif', '']
    open(os.path.join(SRC, 'data.h'), 'w').write('\n'.join(h))

    print('data.bin: %d bytes' % len(out))
    print('  sprites: %d bytes for %d sprites (%d 4-bit, %d 8-bit, %d direct), %d dropped' %
          (sprite_end - spr_table, len(names), modes[0], modes[1], modes[2], len(DROPPED)))
    print('    table %d, palettes and headers %d, slabs (%d) and row offsets %d, pixels %d, padding %d' %
          (stats['table'], stats['palette'], stats['slabs'], stats['rows'], stats['pixels'],
           sprite_end - spr_table - stats['table'] - stats['palette'] - stats['rows'] - stats['pixels']))
    print('  fonts: %d bytes (%s)' % (len(out) - sprite_end,
                                     ', '.join('%s %d B, %d kerning pairs' % f for f in font_sizes)))


if __name__ == '__main__':
    main()
