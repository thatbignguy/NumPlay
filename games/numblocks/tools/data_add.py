#!/usr/bin/env python3
"""Adds block states, textures and sprites to src/data.c and src/data.h.

`make data` / `make data-modern` rewrite data.c from scratch, and for that they need
the 1.8.8 client.jar as the fallback for the pictures a modern client has no
equivalent of (the GUI, the HUD, the bed block...). This tool does the same job for
what is new, without touching what data.c already has: the states tools/blocks.py
has added since, their textures and icons and any new item - packed with
tools/pack.py's own functions, so a later full pack writes the same bytes.

    python3 tools/data_add.py [MODERN_TEX_DIR]     (default: ../../minecraft-textures)

Idempotent: run it again and it finds nothing to add."""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import blocks  # noqa: E402
import items  # noqa: E402
import pack  # noqa: E402
import modern_map  # noqa: E402

DATA = os.path.join(ROOT, 'src', 'data.c')
DATAH = os.path.join(ROOT, 'src', 'data.h')
MODERN = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, '..', '..', 'minecraft-textures')


def parse(body):
    """a C initializer's body -> nested lists (strings stay strings)"""
    body = re.sub(r'/\*.*?\*/', '', body, flags=re.S)   # the rows' comments
    toks = re.findall(r'\{|\}|,|"[^"]*"|0[xX][0-9a-fA-F]+|-?\d+', body)
    pos = [0]

    def val():
        t = toks[pos[0]]
        if t == '{':
            pos[0] += 1
            out = []
            while toks[pos[0]] != '}':
                if toks[pos[0]] == ',':
                    pos[0] += 1
                    continue
                out.append(val())
            pos[0] += 1
            return out
        pos[0] += 1
        if t.startswith('"'):
            return t[1:-1]
        return int(t[2:], 16) if t[:2] in ('0x', '0X') else int(t)

    r = []
    while pos[0] < len(toks):
        if toks[pos[0]] == ',':
            pos[0] += 1
            continue
        r.append(val())
    return r


class Table:
    """one array of data.c: where its declaration and body are, and its rows"""

    def __init__(self, name, decl_start, decl_end, body_start, body_end, rows):
        self.name = name
        # the declaration's text before `{`, and the body between `{` and `};`
        self.decl_start, self.declend = decl_start, decl_end
        self.body_start, self.bodyend = body_start, body_end
        self.rows = rows
        self.changed = False

    def body_str(self):
        """the rows as one initializer body: one line, or a line a row (what pack.py writes)"""
        def one(r):
            if isinstance(r, list):
                return '{' + ','.join(one(v) for v in r) + '}'
            if isinstance(r, str):
                return '"' + r + '"'
            return str(r)
        # (a Recipe's nine ingredients are an array of their own: {out,n,w,h,{nine}})
        if self.rows and isinstance(self.rows[0], list):
            return ',\n  '.join(one(r) for r in self.rows)
        return ','.join(one(v) for v in self.rows)

    def emit(self):
        """the table again: its declaration (with this many rows) and its body"""
        decl = DECL[self.name]
        # the first bracketed size is this table's own; the rest is the rows' shape
        # a numeric first bracketed size is this table's own; a macro's the generated
        # headers keep - except B_COUNT, which the per-state tables no longer use
        decl = re.sub(r'(\b%s)(\[\d+\])' % self.name, lambda m: m.group(1) + '[%d]' % len(self.rows), decl, count=1)
        decl = re.sub(r'(\b%s)\[B_COUNT\]' % self.name, r'\1[NSTATES]', decl, count=1)
        return decl + '{' + self.body_str() + '};'

    def add(self, row):
        self.rows.append(row)
        self.changed = True


DECL = {}


def tables(src):
    out = {}
    for m in re.finditer(r'(const [^\n=]*?\b(\w+)((?:\[[^\]]*\])+) = )\{([^;]*)\};', src):
        name, body = m.group(2), m.group(4)
        DECL[name] = m.group(1)
        out[name] = Table(name, m.start(1), m.end(1), m.start(4), m.end(4), parse(body))
    return out


def add_sprite(T, bs, pal, alpha):
    """a sprite at the end of spr_px, as pack.py builds them"""
    T['spr_px'].rows += bs
    T['spr_off'].rows.append(len(T['spr_px'].rows) - len(bs))
    T['spr_w'].rows.append(16)
    T['spr_h'].rows.append(16)
    T['spr_alpha'].rows.append(1 if alpha else 0)
    T['spr_pal'].rows.append([pack.rgb565(*c) for c in (pal + [(0, 0, 0)] * (16 - len(pal)))])
    for name in ('spr_px', 'spr_off', 'spr_w', 'spr_h', 'spr_alpha', 'spr_pal'):
        T[name].changed = True
    return len(T['spr_off'].rows) - 1


def main():
    src = open(DATA).read()
    T = tables(src)
    jar = modern_map.ModernSource(MODERN)
    BCOUNT = len(blocks.S)
    # the per-state tables are padded to NSTATES (the 9-bit packing): the states data.c
    # defines end where its nonzero Minecraft ids do (every defined state but air and the
    # reserved id-0 states has one; the padding past them is all zero). Cut that padding so
    # the new states overwrite it instead of appending past the tables' declared size.
    idrows = T['blk_id'].rows
    nstates = max([i for i, v in enumerate(idrows) if v] or [0]) + 1
    assert BCOUNT >= nstates, 'data.c has more states than blocks.py'
    assert all(v == 0 for v in idrows[nstates:]), 'blk_id padding is not zero'
    for name in ('blk_tex', 'blk_model', 'blk_light', 'blk_nbox', 'blk_box', 'blk_front', 'blk_flags',
                 'blk_tool', 'blk_level', 'blk_hard', 'blk_id', 'blk_meta', 'blk_icon', 'blk_item',
                 'blk_fuel', 'blk_label'):
        T[name].rows = T[name].rows[:nstates]
    ntex = len(T['tex_px'].rows)
    nsprites0 = len(T['spr_off'].rows)
    nitems0 = len(T['it_label'].rows)

    # ---- the texture names, in pack_textures' order: data.h's TX_ says what is there,
    # and what the new states need comes after it
    by_index = {}
    for m in re.finditer(r'#define TX_(\w+) (\d+)', open(DATAH).read()):
        by_index[int(m.group(2))] = m.group(1).lower()
    assert sorted(by_index) == list(range(ntex)), 'data.h lost a texture name'
    new_names = []
    for s in blocks.S[nstates:]:
        t = s['tex']
        if t is None:
            continue
        for n in ([t] if isinstance(t, str) else list(t.values())):
            if n and n.lower() not in by_index.values() and n.lower() not in new_names:
                new_names.append(n)

    # ---- the new textures
    ntex0 = ntex
    for n in new_names:
        idx, pal, alpha = pack.quantize(pack.tex_image(jar, n), 16)
        while len(pal) < 16:
            pal.append((0, 0, 0))
        T['tex_px'].add([idx[i] | (idx[i + 1] << 4) for i in range(0, 256, 2)])
        T['tex_pal'].add([pack.rgb565(*c) for c in pal])
        T['tex_flags'].add(16 | (0x20 if alpha else 0))
        by_index[ntex] = n.lower()
        ntex += 1

    # ---- the new block states (pack.faces takes the names by index)
    name_list = [by_index[i] for i in range(ntex)]
    for i in range(nstates, BCOUNT):
        s = blocks.S[i]
        T['blk_tex'].add(pack.faces(s, name_list))
        T['blk_model'].add(blocks.MODELS.index(s['model']))
        T['blk_light'].add(s['light'])
        b = pack.boxes_of(s)
        T['blk_nbox'].add(len(b))
        b = (list(b) + [[0] * 6] * 2)[:2]
        T['blk_box'].add([list(x) for x in b])
        t = s['tex']
        T['blk_front'].add(name_list.index(t['front'].lower()) if isinstance(t, dict) and 'front' in t else 0)
        f = pack.TINT[s['tint']]
        if s['model'] in ('cube',):
            f |= pack.BF_OPAQUE
        if s['model'] in ('cube', 'leaves', 'glass', 'slab', 'cactus', 'fence', 'pane', 'door', 'layer', 'stairs',
                          'chest', 'bed'):
            f |= pack.BF_SOLID
        T['blk_flags'].add(f)
        T['blk_tool'].add(blocks.TOOLS.index(s['tool']))
        T['blk_level'].add(s['level'])
        T['blk_hard'].add(255 if s['hard'] < 0 or s['hard'] >= 12.7 else int(round(s['hard'] * 20)))
        T['blk_id'].add(s['id'])
        T['blk_meta'].add(s['meta'])
        T['blk_icon'].add(0)   # set below, with the sprites
        bi = items.block_item(s['name'])
        T['blk_item'].add(0xFFFF if bi is None else bi)
        T['blk_fuel'].add(items.fuel_of(i))
        T['blk_label'].add(items.LABELS[s['name']] if s['name'] in items.LABELS else '0')

    # ---- the icons: a sprite for every new state that has one, then every new item.
    # New sprites go at the end (past the GUI sprites), so the old sprite indices
    # (blk_icon, it_icon, SP_*) all stay valid: nothing already packed moves.
    for i in range(nstates, BCOUNT):
        s = blocks.S[i]
        if s['tex'] is None:
            continue
        bs, pal, alpha = pack.pack_sprite(pack.iso_icon(jar, s, None))
        T['blk_icon'].rows[i] = add_sprite(T, bs, pal, alpha)
    T['blk_icon'].changed = True
    for k in range(nitems0, len(items.I)):
        it = items.I[k]
        bs, pal, alpha = pack.pack_sprite(pack.item_icon(jar, it))
        T['it_label'].add(it['label'])
        T['it_icon'].add(add_sprite(T, bs, pal, alpha))
        T['it_kind'].add(items.KINDS.index(it['kind']))
        T['it_stack'].add(it['stack'])
        T['it_dur'].add(it['dur'])
        T['it_tier'].add(it['tier'])
        T['it_a'].add(it['a'])
        T['it_b'].add(it['b'])
        T['it_place'].add(items.BNAMES.index(it['place']) if it['place'] else 255)
        T['it_fuel'].add(it['fuel'])

    # ---- the recipes
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
        rec.append([items.resolve(o), n, w, hh, ins])
    T['recipes'].rows = rec
    T['recipes'].changed = True

    # ---- smelting: rebuilt like the recipes (pack.py's smelting table)
    T['smelting'].rows = [[items.resolve(a), items.resolve(b)] for a, b in items.SMELT]
    T['smelting'].changed = True

    # ---- the creative tabs: each tab's ids, in items.py's order (blocks as blocks,
    # items as items: the two share the ids from 256 up)
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
    T['tab_items'].rows = flat
    T['tab_items'].changed = True
    T['tab_start'].rows = starts
    T['tab_start'].changed = True

    # ---- the unused states, and every per-state table's size
    pad = blocks.NSTATES - BCOUNT
    T['blk_tex'].rows += [[0] * 6] * pad
    T['blk_tex'].changed = True
    T['blk_box'].rows += [[[0] * 6, [0] * 6]] * pad
    T['blk_box'].changed = True
    for name in ('blk_model', 'blk_light', 'blk_nbox', 'blk_front', 'blk_flags', 'blk_tool', 'blk_level', 'blk_hard',
                 'blk_id', 'blk_meta', 'blk_fuel'):
        T[name].rows += [0] * pad
        T[name].changed = True
    T['blk_icon'].rows += [0xFFFF] * pad
    T['blk_icon'].changed = True
    T['blk_item'].rows += [0xFFFF] * pad
    T['blk_item'].changed = True
    T['blk_label'].rows += [0] * pad
    T['blk_label'].changed = True

    # ---- data.c again: what changed says how many rows it has; the rest is untouched
    out, at = [], 0
    for t in T.values():
        if not t.changed:
            continue
        out.append(src[at:t.decl_start])
        out.append(t.emit())
        at = t.bodyend + len('};')
    out.append(src[at:])
    open(DATA, 'w').write(''.join(out))

    # ---- data.h: NTEX, the new textures' TX_, the tables' sizes, the recipe count
    h = open(DATAH).read()
    h = re.sub(r'#define NTEX \d+', '#define NTEX %d' % ntex, h)
    lines = h.split('\n')
    last_tx = max(i for i, l in enumerate(lines) if l.startswith('#define TX_'))
    for k, n in enumerate(new_names):
        lines.insert(last_tx + 1 + k, '#define TX_%s %d' % (n.upper(), ntex0 + k))
    h = '\n'.join(lines)
    for name in ('blk_tex', 'blk_model', 'blk_light', 'blk_flags', 'blk_tool', 'blk_nbox', 'blk_box', 'blk_front',
                 'blk_level', 'blk_hard', 'blk_id', 'blk_meta', 'blk_icon', 'blk_item', 'blk_fuel', 'blk_label'):
        h = h.replace('%s[B_COUNT]' % name, '%s[NSTATES]' % name)
    h = re.sub(r'#define N_RECIPES \d+', '#define N_RECIPES %d' % len(items.R), h)
    h = re.sub(r'#define N_SMELTING \d+', '#define N_SMELTING %d' % len(items.SMELT), h)
    open(DATAH, 'w').write(h)
    print(f'{BCOUNT - nstates} block states added (B_COUNT {BCOUNT}, {blocks.NSTATES} ids), {len(new_names)} '
          f'textures (to {ntex}), {len(items.I)} items, {len(items.R)} recipes')


if __name__ == '__main__':
    main()
