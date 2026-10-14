#!/usr/bin/env python3
"""Patch src/data.c/data.h for the desert pyramid's 2 new block kinds (5 states),
without a full pack (which would shift every sprite index and lose SP_EYE).

New states (B_COUNT 442 -> 447), all appended so no existing id moves:
  442 STONE_PRESSURE_PLATE 70:0 flat stone
  443 SANDSTONE_STAIRS     128:0 stairs sandstone
  444 SANDSTONE_STAIRS_W   128:1 (inventory: SANDSTONE_STAIRS)
  445 SANDSTONE_STAIRS_S   128:2 (inventory: SANDSTONE_STAIRS)
  446 SANDSTONE_STAIRS_N   128:3 (inventory: SANDSTONE_STAIRS)
CUT_SANDSTONE reuses SANDSTONE_SMOOTH (24:2, modern cut_sandstone texture);
orange/blue terracotta reuse STAINED_CLAY_ORANGE/BLUE.

Sprites: 2 new icons (plate, stairs) appended at 503,504 (old has 503 sprites
0..502, SP_EYE 443 kept). Stairs variants share the stairs icon (identical
pictures, saves 3 sprites of flash).
Tabs/recipes recomputed via tools/items.py (plate to Functional, stairs to
Building Blocks, both to Search; 2 new recipes).
"""
import sys, os, re
HERE = os.path.dirname(os.path.abspath(__file__))  # tools/
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import blocks, items
from pack import iso_icon, pack_sprite, rgb565
from modern_map import ModernSource

# --- texture indices from old data.h (no new textures) ---
with open(os.path.join(ROOT, 'src', 'data.h')) as f:
    dh = f.read()
def tx(name):
    m = re.search(r'#define TX_' + name + r' (\d+)', dh)
    assert m, name
    return int(m.group(1))
TX_STONE = tx('STONE')
TX_TOP = tx('SANDSTONE_TOP')
TX_SIDE = tx('SANDSTONE_NORMAL')
TX_BOT = tx('SANDSTONE_BOTTOM')
print(f"TX stone {TX_STONE} top {TX_TOP} side {TX_SIDE} bot {TX_BOT}")

# --- per-state values for 442..446 ---
# faces order -Y +Y -Z +Z -X +X
new_tex = [
    [TX_STONE]*6,
    [TX_BOT, TX_TOP, TX_SIDE, TX_SIDE, TX_SIDE, TX_SIDE],
    [TX_BOT, TX_TOP, TX_SIDE, TX_SIDE, TX_SIDE, TX_SIDE],
    [TX_BOT, TX_TOP, TX_SIDE, TX_SIDE, TX_SIDE, TX_SIDE],
    [TX_BOT, TX_TOP, TX_SIDE, TX_SIDE, TX_SIDE, TX_SIDE],
]
MODELS = blocks.MODELS
new_model = [MODELS.index('flat')] + [MODELS.index('stairs')]*4
new_light = [0]*5
# nbox/box
STAIR_TOP = [(8,8,0,16,16,16),(0,8,0,8,16,16),(0,8,8,16,16,16),(0,8,0,16,16,8)]
new_nbox = [1,2,2,2,2]
new_box = [
    [(0,0,0,16,1,16),(0,0,0,0,0,0)],
    [(0,0,0,16,8,16),STAIR_TOP[0]],
    [(0,0,0,16,8,16),STAIR_TOP[1]],
    [(0,0,0,16,8,16),STAIR_TOP[2]],
    [(0,0,0,16,8,16),STAIR_TOP[3]],
]
new_front = [0]*5
# flags: TINT 0 | BF_OPAQUE(8) for cube only | BF_SOLID(16) for stairs etc.
# pack.py: cube->opaque, (cube,leaves,glass,slab,cactus,fence,pane,door,layer,stairs,chest,bed)->solid
new_flags = [0,16,16,16,16]
TOOLS = blocks.TOOLS
new_tool = [TOOLS.index('pickaxe')]*5
new_level = [0]*5
new_hard = [10,16,16,16,16]  # 0.5*20, 0.8*20
new_id = [70,128,128,128,128]
new_meta = [0,0,1,2,3]
# blk_item: plate itself, stairs base, variants -> base
new_item = [442,443,443,443,443]
new_fuel = [0]*5
new_label = ['"Stone Pressure Plate"','"Sandstone Stairs"','0','0','0']
# blk_icon: 503 plate, 504 stairs (shared)
new_icon = [503,504,504,504,504]

# --- generate 2 icons ---
jar = ModernSource(os.path.join(ROOT, '..', '..', 'minecraft-textures'), os.path.join(ROOT, 'client.jar'))
plate_state = [s for s in blocks.S if s['name']=='STONE_PRESSURE_PLATE'][0]
stairs_state = [s for s in blocks.S if s['name']=='SANDSTONE_STAIRS'][0]
plate_im = iso_icon(jar, plate_state, None)
stairs_im = iso_icon(jar, stairs_state, None)
plate_bs, plate_pal, plate_alpha = pack_sprite(plate_im)
stairs_bs, stairs_pal, stairs_alpha = pack_sprite(stairs_im)
print(f"plate icon bytes {len(plate_bs)} alpha {plate_alpha}, stairs bytes {len(stairs_bs)} alpha {stairs_alpha}")
assert len(plate_bs)==128 and len(stairs_bs)==128
def pal565(pal):
    return [f'0x{rgb565(*c):04X}' for c in pal]

with open(os.path.join(ROOT, 'src', 'data.c')) as f:
    dc = f.read()

# --- helper to replace a table ---
def replace_table(name, new_decl, new_body):
    # match "const ... name[...] = ...;" non-greedy up to "};"
    pat = r'const [^;]*?\b' + re.escape(name) + r'\b[^=]*=.*?;\n'
    m = re.search(pat, dc, re.DOTALL)
    assert m, name
    return dc[m.start():m.end()], new_decl + new_body

# We will build the new data.c by string replacements. To keep it robust,
# parse each needed table with regex and rebuild.

def get_array_ints(name):
    # returns list of ints from "const ... name[...] = { ... };"
    pat = r'const [^;]*?\b' + re.escape(name) + r'\b[^=]*=\s*\{(.*?)\};\s*\n'
    m = re.search(pat, dc, re.DOTALL)
    assert m, name
    body = m.group(1)
    # remove comments /* ... */
    body = re.sub(r'/\*.*?\*/', '', body, flags=re.DOTALL)
    # split by comma, ignore empty
    vals = []
    for tok in body.split(','):
        tok=tok.strip()
        if not tok: continue
        # handle hex? all decimal or 0xFFFF?
        if tok.startswith('0x') or tok.startswith('0X'):
            vals.append(int(tok,16))
        else:
            # may contain newlines/spaces
            vals.append(int(tok))
    return vals, m.group(0)

# --- blk_tex: multi-line, 512 rows of 6 ---
# parse specially: find all {a,b,c,d,e,f} rows
m = re.search(r'const uint16_t blk_tex\[NSTATES\]\[6\] = \{(.*?)\};\n', dc, re.DOTALL)
assert m
tex_body = m.group(1)
rows = re.findall(r'\{([^}]*)\}', tex_body)
print(f"blk_tex rows {len(rows)}")
assert len(rows)==512
# replace rows 442..446
for i, nt in enumerate(new_tex):
    rows[442+i] = ','.join(str(v) for v in nt)
new_tex_body = '{' + '},\n  {'.join(rows) + '},\n'
# keep original header? original starts "{{0,0,...},\n  {.."
# our rebuild: "const uint16_t blk_tex[NSTATES][6] = {{...},\n  {...},\n};\n"
# need to match original formatting loosely (C cares not). Use original decl prefix.
old_tex_decl = dc[m.start():m.start()+40]
# find full old statement to replace
old_tex_stmt = dc[m.start():m.end()+2]  # includes "};\n"? m ends before "};\n"? Actually pat includes "};\n"? Our regex ends with "\};\n", group(1) is inside. m.group(0) is full? We used group(1) inside, need full.
# redo: get full statement
mfull = re.search(r'const uint16_t blk_tex\[NSTATES\]\[6\] = \{.*?\};\n', dc, re.DOTALL)
assert mfull
new_tex_stmt = 'const uint16_t blk_tex[NSTATES][6] = {{' + '},\n  {'.join(rows) + '},\n};\n'
dc = dc[:mfull.start()] + new_tex_stmt + dc[mfull.end():]
print("patched blk_tex")

def patch_simple_u8(name, new_vals_5, start=442):
    global dc
    vals, old_stmt = get_array_ints(name)
    assert len(vals)==512, (name, len(vals))
    for i,v in enumerate(new_vals_5):
        vals[start+i]=v
    # rebuild as single-line like original? original is single-line for these.
    # Use same style: "const uint8_t NAME[NSTATES] = {a,b,...};\n"
    # Find decl type: need to preserve type. Get type from old_stmt header.
    mtype = re.match(r'(const \S+ \S+\[NSTATES\][^\=]*=)', old_stmt)
    # simpler: reuse old header up to '='
    header = old_stmt.split('=',1)[0] + '= {'
    new_stmt = header + ','.join(str(v) for v in vals) + '};\n'
    # replace
    pat = r'const [^;]*?\b' + re.escape(name) + r'\[NSTATES\][^=]*=.*?;\n'
    dc = re.sub(pat, lambda _: new_stmt, dc, count=1, flags=re.DOTALL)
    print(f"patched {name}")

patch_simple_u8('blk_model', new_model)
patch_simple_u8('blk_light', new_light)
patch_simple_u8('blk_nbox', new_nbox)
patch_simple_u8('blk_front', new_front)
patch_simple_u8('blk_flags', new_flags)
patch_simple_u8('blk_tool', new_tool)
patch_simple_u8('blk_level', new_level)
patch_simple_u8('blk_hard', new_hard)
patch_simple_u8('blk_id', new_id)
patch_simple_u8('blk_meta', new_meta)
patch_simple_u8('blk_fuel', new_fuel)

# blk_box: special [NSTATES][2][6]
mfull = re.search(r'const int8_t blk_box\[NSTATES\]\[2\]\[6\] = \{.*?\};\n', dc, re.DOTALL)
assert mfull
box_body = mfull.group(0)
# find all {{a,b,c,d,e,f},{g,h,i,j,k,l}} entries (inside only numbers, so the
# outer array's brace cannot be mistaken for an entry's)
entries = re.findall(r'\{\{([0-9,\- ]*)\},\{([0-9,\- ]*)\}\}', box_body)
print(f"blk_box entries {len(entries)}")
assert len(entries)==512
for i, nb in enumerate(new_box):
    entries[442+i] = (','.join(str(v) for v in nb[0]), ','.join(str(v) for v in nb[1]))
new_box_stmt = 'const int8_t blk_box[NSTATES][2][6] = {' + ','.join('{{'+a+'},{'+b+'}}' for a,b in entries) + '};\n'
dc = dc[:mfull.start()] + new_box_stmt + dc[mfull.end():]
print("patched blk_box")

# blk_icon, blk_item: uint16, may contain 0xFFFF (65535)
def patch_simple_u16(name, new_vals_5, start=442):
    global dc
    vals, old_stmt = get_array_ints(name)
    assert len(vals)==512, (name, len(vals))
    for i,v in enumerate(new_vals_5):
        vals[start+i]=v
    header = old_stmt.split('=',1)[0] + '= {'
    new_stmt = header + ','.join(str(v) for v in vals) + '};\n'
    pat = r'const [^;]*?\b' + re.escape(name) + r'\[NSTATES\][^=]*=.*?;\n'
    dc = re.sub(pat, lambda _: new_stmt, dc, count=1, flags=re.DOTALL)
    print(f"patched {name}")

patch_simple_u16('blk_icon', new_icon)
patch_simple_u16('blk_item', new_item)

# blk_label: strings + 0
mfull = re.search(r'const char \*const blk_label\[NSTATES\] = \{.*?\};\n', dc, re.DOTALL)
assert mfull
# split by comma, but strings contain commas? No, labels have no commas. Safe.
label_body = mfull.group(0)
# find all entries: "..." or 0
labels = re.findall(r'"[^"]*"|0', label_body)
print(f"blk_label entries {len(labels)}")
assert len(labels)==512
labels[442]='"Stone Pressure Plate"'
labels[443]='"Sandstone Stairs"'
labels[444]='0'; labels[445]='0'; labels[446]='0'
new_label_stmt = 'const char *const blk_label[NSTATES] = {' + ','.join(labels) + '};\n'
dc = dc[:mfull.start()] + new_label_stmt + dc[mfull.end():]
print("patched blk_label")

# --- sprites: append 2 ---
# spr_px: const uint8_t spr_px[78378] = {...};
m = re.search(r'const uint8_t spr_px\[(\d+)\] = \{(.*?)\};\n', dc, re.DOTALL)
assert m
old_px_n = int(m.group(1))
old_px_body = m.group(2)
old_px_vals = [int(t.strip()) for t in old_px_body.split(',') if t.strip()!='']
print(f"spr_px old n {old_px_n} vals {len(old_px_vals)}")
assert len(old_px_vals)==old_px_n
new_px_vals = old_px_vals + plate_bs + stairs_bs
new_px_n = len(new_px_vals)
new_px_stmt = f'const uint8_t spr_px[{new_px_n}] = {{{",".join(str(v) for v in new_px_vals)}}};\n'
dc = dc[:m.start()] + new_px_stmt + dc[m.end():]
print(f"patched spr_px {old_px_n} -> {new_px_n}")

# spr_off[503]: offsets into spr_px
vals, old_stmt = get_array_ints('spr_off')
print(f"spr_off len {len(vals)} last {vals[-1]}")
assert len(vals)==503
new_off = vals + [vals[-1]+len(plate_bs), vals[-1]+len(plate_bs)+len(stairs_bs)]
# Wait spr_off length should be n_sprites (503 old, 505 new). Offsets: spr_off[i] = byte offset of sprite i in spr_px.
# Old spr_off has 503 entries. New should have 505: old 503 + 2 new.
# New offsets: old offsets + [old_total, old_total+128]? Actually spr_px total old_n = 78378. spr_off[-1] is offset of last sprite (502). Its length = old_n - spr_off[-1].
# New sprite 503 at old_n, 504 at old_n+128.
assert old_px_n == 78378
new_off_vals = vals + [old_px_n, old_px_n+len(plate_bs)]
assert len(new_off_vals)==505
header = old_stmt.split('=',1)[0]
# header contains "[503]", need to update to [505]
header = re.sub(r'\[\d+\]', '[505]', header)
new_stmt = header + '= {' + ','.join(str(v) for v in new_off_vals) + '};\n'
pat = r'const [^;]*?\bspr_off\[\d+\][^=]*=.*?;\n'
dc = re.sub(pat, lambda _: new_stmt, dc, count=1, flags=re.DOTALL)
print("patched spr_off 503 -> 505")

def patch_spr_u8(name, new_vals):
    global dc
    vals, old_stmt = get_array_ints(name)
    print(f"{name} old len {len(vals)}")
    new_all = vals + new_vals
    header = old_stmt.split('=',1)[0]
    header = re.sub(r'\[\d+\]', f'[{len(new_all)}]', header)
    new_stmt = header + '= {' + ','.join(str(v) for v in new_all) + '};\n'
    pat = r'const [^;]*?\b' + re.escape(name) + r'\[\d+\][^=]*=.*?;\n'
    dc = re.sub(pat, lambda _: new_stmt, dc, count=1, flags=re.DOTALL)
    print(f"patched {name} {len(vals)} -> {len(new_all)}")

patch_spr_u8('spr_w', [16,16])
patch_spr_u8('spr_h', [16,16])
patch_spr_u8('spr_alpha', [1 if plate_alpha else 0, 1 if stairs_alpha else 0])

# spr_pal[503][16]: need special (2D)
mfull = re.search(r'const uint16_t spr_pal\[(\d+)\]\[16\] = \{(.*?)\};\n', dc, re.DOTALL)
assert mfull
old_pal_n = int(mfull.group(1))
print(f"spr_pal old n {old_pal_n}")
pal_rows = re.findall(r'\{([^}]*)\}', mfull.group(2))
print(f"spr_pal rows {len(pal_rows)}")
assert len(pal_rows)==old_pal_n
new_pal_rows = pal_rows + [','.join(pal565(plate_pal)), ','.join(pal565(stairs_pal))]
new_pal_stmt = f'const uint16_t spr_pal[{old_pal_n+2}][16] = {{' + ','.join('{'+r+'}' for r in new_pal_rows) + '};\n'
dc = dc[:mfull.start()] + new_pal_stmt + dc[mfull.end():]
print("patched spr_pal")

# --- tab_items/tab_start: recompute via items.py ---
tab_lists = [[] for _ in items.TABS]
BNAMES = [s['name'] for s in blocks.S]
for iid in range(len(blocks.S)):
    tabs = items.tab_of(iid, True)
    if tabs:
        for t in tabs:
            tab_lists[t].append(iid)
        tab_lists[items.T_SEARCH].append(iid)
for k in range(len(items.I)):
    iid = 256+k
    tabs = items.tab_of(iid, False)
    if tabs:
        for t in tabs:
            tab_lists[t].append(iid)
        if tab_lists[items.T_SEARCH].count(iid)==0:
            tab_lists[items.T_SEARCH].append(iid)
for t in range(len(items.TABS)):
    tab_lists[t] = items.tab_order(t, tab_lists[t])
flat, starts = [], []
for L in tab_lists:
    starts.append(len(flat))
    flat += L
starts.append(len(flat))
print(f"tab_items new len {len(flat)} starts {starts}")
# replace
mfull = re.search(r'const uint16_t tab_items\[(\d+)\] = \{.*?\};\n', dc, re.DOTALL)
assert mfull
old_tab_n = int(mfull.group(1))
print(f"tab_items old n {old_tab_n}")
new_tab_stmt = f'const uint16_t tab_items[{len(flat)}] = {{{",".join(str(v) for v in flat)}}};\n'
dc = dc[:mfull.start()] + new_tab_stmt + dc[mfull.end():]
mfull = re.search(r'const uint16_t tab_start\[(\d+)\] = \{.*?\};\n', dc, re.DOTALL)
assert mfull
new_start_stmt = f'const uint16_t tab_start[{len(starts)}] = {{{",".join(str(v) for v in starts)}}};\n'
dc = dc[:mfull.start()] + new_start_stmt + dc[mfull.end():]
print("patched tabs")

# --- recipes: recompute via items.py ---
rec = []
for o,n,rows,keys in items.R:
    if rows:
        w,hh = max(len(r) for r in rows), len(rows)
        ins=[]
        for r in rows:
            for c in r.ljust(w):
                ins.append(0 if c==' ' else items.resolve(keys[c]))
    else:
        w,hh = 0, len(keys)
        ins=[items.resolve(k) for k in keys]
    ins += [0]*(9-len(ins))
    rec.append('{%d,%d,%d,%d,{%s}}' % (items.resolve(o),n,w,hh,','.join(str(v) for v in ins)))
print(f"recipes new {len(rec)}")
mfull = re.search(r'const Recipe recipes\[(\d+)\] = \{.*?\};\n', dc, re.DOTALL)
assert mfull
old_rec_n = int(mfull.group(1))
print(f"recipes old {old_rec_n}")
new_rec_stmt = f'const Recipe recipes[{len(rec)}] = {{{",".join(rec)}}};\n'
dc = dc[:mfull.start()] + new_rec_stmt + dc[mfull.end():]
print("patched recipes")

with open(os.path.join(ROOT, 'src', 'data.c'), 'w') as f:
    f.write(dc)
print("wrote data.c")

# --- data.h: N_RECIPES 208 -> 210 ---
with open(os.path.join(ROOT, 'src', 'data.h')) as f:
    dh = f.read()
dh = dh.replace('#define N_RECIPES 208', f'#define N_RECIPES {len(rec)}')
with open(os.path.join(ROOT, 'src', 'data.h'), 'w') as f:
    f.write(dh)
print("patched data.h N_RECIPES")
