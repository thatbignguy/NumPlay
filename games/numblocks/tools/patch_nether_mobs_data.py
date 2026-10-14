#!/usr/bin/env python3
"""Patch src/data.c/data.h for the nether mobs' 2 new spawn eggs + 2 skins,
without a full pack (which would shift every sprite index and lose SP_EYE).

New items (N_ITEMS 140 -> 142), all appended so no existing id moves:
  396 PIGLIN_SPAWN_EGG, 397 ZOMBIFIED_PIGLIN_SPAWN_EGG (after QUARTZ 395, so the
  eight overworld eggs 387..394 and QUARTZ keep theirs; eggs non-contiguous)
New skins (9 -> 11), appended:
  9 piglin/piglin (SKIN_PIGLIN), 10 piglin/zombified_piglin (SKIN_ZPIGLIN)

Sprites: 2 new egg icons appended at 505,506 (old has 505 sprites 0..504,
SP_EYE 443 kept). it_icon for the new eggs points there (not first_gui-based).
Tabs/recipes recomputed via tools/items.py (Spawn Eggs + Search gain the eggs).
"""
import sys, os, re
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import items
from pack import item_icon, pack_sprite, rgb565, skin_sheet
from modern_map import ModernSource

EGGS = '/tmp/opencode/spawn-eggs-pack/textures/item'
jar = ModernSource(os.path.join(ROOT, '..', '..', 'minecraft-textures'),
                   os.path.join(ROOT, 'client.jar'), extra=(EGGS,))

# --- 2 egg icons (16x16, like pack.py's item_icon) ---
new_items = [it for it in items.I if it['name'] in ('PIGLIN_SPAWN_EGG', 'ZOMBIFIED_PIGLIN_SPAWN_EGG')]
assert len(new_items) == 2, [it['name'] for it in new_items]
print("new items:", [(it['name'], it['tex']) for it in new_items])
icon_data = []
for it in new_items:
    im = item_icon(jar, it)
    assert im.size == (16, 16), (it['name'], im.size)
    bs, pal, alpha = pack_sprite(im)
    assert len(bs) == 128, (it['name'], len(bs))
    icon_data.append((bs, pal, 1 if alpha else 0))
    print(f"{it['name']}: bytes {len(bs)} alpha {alpha}")

# --- 2 skins (64x32, like pack.py's skin_sheet) ---
new_skins = ['piglin/piglin', 'piglin/zombified_piglin']
skin_data = []
for n in new_skins:
    im = skin_sheet(jar, n)
    assert im.size == (64, 32), (n, im.size)
    bs, pal, alpha = pack_sprite(im)
    assert len(bs) == 1024, (n, len(bs))
    skin_data.append((bs, pal))
    print(f"skin {n}: bytes {len(bs)}")

with open(os.path.join(ROOT, 'src', 'data.c')) as f:
    dc = f.read()

def get_array_ints(name, dim=None):
    # "const ... name[N] = { ... };" -> list of ints, old stmt
    if dim:
        pat = r'const [^;]*?\b' + re.escape(name) + r'\[' + re.escape(dim) + r'\][^=]*=\s*\{(.*?)\};\s*\n'
    else:
        pat = r'const [^;]*?\b' + re.escape(name) + r'\b[^=]*=\s*\{(.*?)\};\s*\n'
    m = re.search(pat, dc, re.DOTALL)
    assert m, name
    body = re.sub(r'/\*.*?\*/', '', m.group(1), flags=re.DOTALL)
    vals = []
    for tok in body.split(','):
        tok = tok.strip()
        if not tok:
            continue
        vals.append(int(tok, 16) if tok.startswith(('0x', '0X')) else int(tok))
    return vals, m.group(0)

# --- sprites: append 2 (spr_px, spr_off, spr_w/h/alpha, spr_pal) ---
m = re.search(r'const uint8_t spr_px\[(\d+)\] = \{(.*?)\};\n', dc, re.DOTALL)
assert m
old_px_n = int(m.group(1))
old_px_vals = [int(t.strip()) for t in m.group(2).split(',') if t.strip() != '']
assert len(old_px_vals) == old_px_n, (len(old_px_vals), old_px_n)
print(f"spr_px old n {old_px_n}")
new_px_vals = old_px_vals + icon_data[0][0] + icon_data[1][0]
new_px_n = len(new_px_vals)
new_px_stmt = f'const uint8_t spr_px[{new_px_n}] = {{{",".join(str(v) for v in new_px_vals)}}};\n'
dc = dc[:m.start()] + new_px_stmt + dc[m.end():]
print(f"patched spr_px {old_px_n} -> {new_px_n}")

vals, old_stmt = get_array_ints('spr_off')
print(f"spr_off len {len(vals)} last {vals[-1]}")
assert len(vals) == 505, len(vals)
new_off = vals + [old_px_n, old_px_n + len(icon_data[0][0])]
assert len(new_off) == 507
header = old_stmt.split('=', 1)[0]
header = re.sub(r'\[\d+\]', '[507]', header)
new_stmt = header + '= {' + ','.join(str(v) for v in new_off) + '};\n'
dc = re.sub(r'const [^;]*?\bspr_off\[\d+\][^=]*=.*?;\n', lambda _: new_stmt, dc, count=1, flags=re.DOTALL)
print("patched spr_off 505 -> 507")

def patch_spr_u8(name, new_vals):
    global dc
    vals, old_stmt = get_array_ints(name)
    new_all = vals + new_vals
    header = old_stmt.split('=', 1)[0]
    header = re.sub(r'\[\d+\]', f'[{len(new_all)}]', header)
    new_stmt = header + '= {' + ','.join(str(v) for v in new_all) + '};\n'
    # spr_w/h/alpha are "const uint8_t NAME[N] = {...};"
    pat = r'const [^;]*?\b' + re.escape(name) + r'\[\d+\][^=]*=.*?;\n'
    dc = re.sub(pat, lambda _: new_stmt, dc, count=1, flags=re.DOTALL)
    print(f"patched {name} {len(vals)} -> {len(new_all)}")

patch_spr_u8('spr_w', [16, 16])
patch_spr_u8('spr_h', [16, 16])
patch_spr_u8('spr_alpha', [icon_data[0][2], icon_data[1][2]])

mfull = re.search(r'const uint16_t spr_pal\[(\d+)\]\[16\] = \{(.*?)\};\n', dc, re.DOTALL)
assert mfull
old_pal_n = int(mfull.group(1))
pal_rows = re.findall(r'\{([^}]*)\}', mfull.group(2))
assert len(pal_rows) == old_pal_n, (len(pal_rows), old_pal_n)
def pal565(pal):
    return [f'0x{rgb565(*c):04X}' for c in pal]
new_pal_rows = pal_rows + [','.join(pal565(icon_data[0][1])), ','.join(pal565(icon_data[1][1]))]
new_pal_stmt = f'const uint16_t spr_pal[{old_pal_n+2}][16] = {{' + ','.join('{'+r+'}' for r in new_pal_rows) + '};\n'
dc = dc[:mfull.start()] + new_pal_stmt + dc[mfull.end():]
print(f"patched spr_pal {old_pal_n} -> {old_pal_n+2}")

# --- skins: 9 -> 11 (skin_px[9][1024] -> [11][1024], skin_pal[9][16] -> [11][16]) ---
mfull = re.search(r'const uint8_t skin_px\[(\d+)\]\[1024\] = \{(.*?)\};\n', dc, re.DOTALL)
assert mfull
old_skin_n = int(mfull.group(1))
print(f"skin_px old n {old_skin_n}")
assert old_skin_n == 9
skin_rows = re.findall(r'\{([^}]*)\}', mfull.group(2))
assert len(skin_rows) == 9
new_skin_rows = skin_rows + [','.join(str(v) for v in skin_data[0][0]), ','.join(str(v) for v in skin_data[1][0])]
new_skin_stmt = f'const uint8_t skin_px[{old_skin_n+2}][1024] = {{' + ','.join('{'+r+'}' for r in new_skin_rows) + '};\n'
dc = dc[:mfull.start()] + new_skin_stmt + dc[mfull.end():]
print("patched skin_px 9 -> 11")

mfull = re.search(r'const uint16_t skin_pal\[(\d+)\]\[16\] = \{(.*?)\};\n', dc, re.DOTALL)
assert mfull
old_spal_n = int(mfull.group(1))
assert old_spal_n == 9
spal_rows = re.findall(r'\{([^}]*)\}', mfull.group(2))
assert len(spal_rows) == 9
new_spal_rows = spal_rows + [','.join(pal565(skin_data[0][1])), ','.join(pal565(skin_data[1][1]))]
new_spal_stmt = f'const uint16_t skin_pal[{old_spal_n+2}][16] = {{' + ','.join('{'+r+'}' for r in new_spal_rows) + '};\n'
dc = dc[:mfull.start()] + new_spal_stmt + dc[mfull.end():]
print("patched skin_pal 9 -> 11")

# --- it_* tables: a row an id's slot (256..256+ITEM_SLOTS-1) ---
# The existing 140 rows keep their places (the first 140 slots); the rows between them
# and the eggs' (the block states 396..446 are ids of their own, with no item there)
# are the tables' empty slots, and the two eggs' rows sit at their ids' slots (447, 448
# -> rows 191, 192).
SPAN = items.ITEM_SLOTS
print(f"ITEM_SLOTS {SPAN} ({len(items.I)} items)")


def patch_it(name, fill, new_by_slot, isstr=False):
    global dc
    if isstr:
        pat = r'const char \*const ' + re.escape(name) + r'\[N_ITEMS\] = \{(.*?)\};\n'
        m = re.search(pat, dc, re.DOTALL)
        assert m, name
        old = list(re.findall(r'"[^"]*"', m.group(1)))
        assert len(old) == 140, (name, len(old))
        allv = old + ['0'] * (SPAN - 140)
        for slot, v in new_by_slot.items():
            allv[slot] = '"' + v + '"'
        new_stmt = f'const char *const {name}[ITEM_SLOTS] = {{{",".join(allv)}}};\n'
        dc = re.sub(pat, lambda _: new_stmt, dc, count=1, flags=re.DOTALL)
    else:
        old, stmt = get_array_ints(name)
        assert len(old) == 140, (name, len(old))
        allv = old + [fill] * (SPAN - 140)
        for slot, v in new_by_slot.items():
            allv[slot] = v
        # keep the type of the declaration, at ITEM_SLOTS
        header = re.sub(r'\[N_ITEMS\]', '[ITEM_SLOTS]', stmt.split('=', 1)[0]) + '= {'
        new_stmt = header + ','.join(str(v) for v in allv) + '};\n'
        pat = r'const [^;]*?\b' + re.escape(name) + r'\[N_ITEMS\][^=]*=.*?;\n'
        dc = re.sub(pat, lambda _: new_stmt, dc, count=1, flags=re.DOTALL)
    print(f"patched {name} 140 -> {SPAN}")


I = items.I
new_slots = {it['iid'] - 256: it for it in new_items}
print("new item ids:", sorted(it['iid'] for it in new_items))
assert sorted(it['iid'] for it in new_items) == [447, 448]

patch_it('it_label', '0', {s: it['label'] for s, it in new_slots.items()}, isstr=True)
# it_icon: the two appended sprites (505, 506), not first_gui-based
patch_it('it_icon', 0xFFFF, {s: [505, 506][i] for i, s in enumerate(sorted(new_slots))})
patch_it('it_kind', 0, {s: items.KINDS.index(it['kind']) for s, it in new_slots.items()})
patch_it('it_stack', 0, {s: it['stack'] for s, it in new_slots.items()})
patch_it('it_dur', 0, {s: it['dur'] for s, it in new_slots.items()})
patch_it('it_tier', 0, {s: it['tier'] for s, it in new_slots.items()})
patch_it('it_a', 0, {s: it['a'] for s, it in new_slots.items()})
patch_it('it_b', 0, {s: it['b'] for s, it in new_slots.items()})
patch_it('it_place', 255, {s: 255 for s in new_slots})   # (spawn eggs place nothing)
patch_it('it_fuel', 0, {s: it['fuel'] for s, it in new_slots.items()})

# --- tabs/recipes recomputed via items.py (like temple patch) ---
tab_lists = [[] for _ in items.TABS]
BNAMES = [s['name'] for s in __import__('blocks').S]
for iid in range(len(__import__('blocks').S)):
    tabs = items.tab_of(iid, True)
    if tabs:
        for t in tabs:
            tab_lists[t].append(iid)
        tab_lists[items.T_SEARCH].append(iid)
for it in items.I:
    iid = it['iid']
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
print(f"tab_items new len {len(flat)}")
mfull = re.search(r'const uint16_t tab_items\[(\d+)\] = \{.*?\};\n', dc, re.DOTALL)
assert mfull
print(f"tab_items old n {mfull.group(1)}")
new_tab_stmt = f'const uint16_t tab_items[{len(flat)}] = {{{",".join(str(v) for v in flat)}}};\n'
dc = dc[:mfull.start()] + new_tab_stmt + dc[mfull.end():]
mfull = re.search(r'const uint16_t tab_start\[(\d+)\] = \{.*?\};\n', dc, re.DOTALL)
assert mfull
new_start_stmt = f'const uint16_t tab_start[{len(starts)}] = {{{",".join(str(v) for v in starts)}}};\n'
dc = dc[:mfull.start()] + new_start_stmt + dc[mfull.end():]
print("patched tabs")

with open(os.path.join(ROOT, 'src', 'data.c'), 'w') as f:
    f.write(dc)
print("wrote data.c")

# --- data.h: the it_ tables' size and the SKIN enum (SP_EYE and every other row stay) ---
with open(os.path.join(ROOT, 'src', 'data.h')) as f:
    dh = f.read()
for old, new in [
    ('*const it_label[N_ITEMS];', '*const it_label[ITEM_SLOTS];'),
    ('extern const uint16_t it_icon[N_ITEMS], it_dur[N_ITEMS], it_fuel[N_ITEMS];',
     'extern const uint16_t it_icon[ITEM_SLOTS], it_dur[ITEM_SLOTS], it_fuel[ITEM_SLOTS];'),
    ('extern const uint8_t it_kind[N_ITEMS], it_stack[N_ITEMS], it_tier[N_ITEMS], it_a[N_ITEMS], it_b[N_ITEMS];',
     'extern const uint8_t it_kind[ITEM_SLOTS], it_stack[ITEM_SLOTS], it_tier[ITEM_SLOTS], it_a[ITEM_SLOTS],'
     ' it_b[ITEM_SLOTS];'),
    ('extern const uint8_t it_place[N_ITEMS];', 'extern const uint8_t it_place[ITEM_SLOTS];'),
]:
    assert old in dh, old
    dh = dh.replace(old, new)
old_enum = ('enum { SKIN_ZOMBIE, SKIN_SKELETON, SKIN_CREEPER, SKIN_SPIDER, SKIN_PIG, SKIN_COW, SKIN_SHEEP, SKIN_SHEEP_FUR,'
            ' SKIN_CHICKEN };')
new_enum = ('enum { SKIN_ZOMBIE, SKIN_SKELETON, SKIN_CREEPER, SKIN_SPIDER, SKIN_PIG, SKIN_COW, SKIN_SHEEP, SKIN_SHEEP_FUR,'
            ' SKIN_CHICKEN, SKIN_PIGLIN, SKIN_ZPIGLIN };')
assert old_enum in dh, "SKIN enum not found"
dh = dh.replace(old_enum, new_enum)
with open(os.path.join(ROOT, 'src', 'data.h'), 'w') as f:
    f.write(dh)
print("patched data.h (it_ tables at ITEM_SLOTS, SKIN enum, SP_EYE kept)")
