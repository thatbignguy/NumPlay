#!/usr/bin/env python3
"""Makes src/upgrades_data.c and src/upgrades_data.h, the table of building
upgrades, building modes, city upgrades (research) and policies, from the
original's buildingUpgradesInfo.json, cityUpgradesInfo.json, policiesInfo.json,
their classes' code (split/buildingUpgrades.*, cityUpgrades.*, policies.*) and
lang_en.csv: costs, unlock flags, kind, sprite, its layer and frames, the
quality bonus, texts, and which classes offer each (get_possibleUpgrades and
friends, inherited from the class's ancestors).

Run it after tools/gen_classes.py (it reads src/classes.h and src/data.h):

    python3 -I tools/gen_upgrades.py [REFERENCE_DIR]
"""
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
GAME = os.path.dirname(HERE)
REF = sys.argv[1] if len(sys.argv) > 1 else os.environ.get("FE2_REF", os.path.expanduser("~/Documents/Assets/gamesrc/finalearth2/web14"))
SPLIT = os.path.join(REF, "split")

MATS = ["food", "wood", "stone", "machineParts", "refinedMetal", "computerChips", "cacao", "chocolate", "graphene",
        "rocketFuel", "knowledge"]
M_ENUM = ["M_FOOD", "M_WOOD", "M_STONE", "M_MACHINEPARTS", "M_REFINEDMETAL", "M_COMPUTERCHIPS", "M_CACAO",
          "M_CHOCOLATE", "M_GRAPHENE", "M_ROCKETFUEL", "M_KNOWLEDGE"]
# out of scope: holidays, the paid versions' buildings and rockets, info entries without a class
EXCLUDE = {"Pumpkin", "MiniChristmasTree", "RocketFuelFactoryEfficiency", "GrapheneFactoryImprovement",
           "FactoryUsingFood", "FactoryUsingWood", "FactoryDisabled", "LiteratureReviews", "BoardGameCollection"}

lang = {}
for line in open(os.path.join(REF, "lang_en.csv"), encoding="utf-8"):
    k, _, v = line.rstrip("\n").partition("|")
    lang[k] = v


def cstr(s):
    if s is None:
        return "NULL"
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def read(name):
    p = os.path.join(SPLIT, name + ".js")
    return open(p, encoding="utf-8").read() if os.path.exists(p) else ""


# sprites known to the packer
spr_w = {}
for m in re.finditer(r'#define (SPR_\w+)_W (\d+)', open(os.path.join(GAME, "src", "data.h")).read()):
    spr_w[m.group(1)] = int(m.group(2))


def spr_id(name):
    return "SPR_" + re.sub(r'[^A-Z0-9]', '_', name.replace("spr_", "", 1).upper())


# the classes, in classes.h's order
classes = re.findall(r'CLS_(\w+) = \d+,', open(os.path.join(GAME, "src", "classes.h")).read())


def class_src(name):
    for prefix in ("buildings", "worldResources"):
        s = read(f"{prefix}.{name}")
        if s:
            return s
    return read(name)


def parent(name):
    m = re.search(r'__super__ = (\w+);', class_src(name))
    if not m:
        return None
    s = m.group(1)
    return s.split("_", 1)[1] if s.startswith(("buildings_", "worldResources_")) else s


def possible(name, getter):
    """the list a class's getter returns, from the nearest ancestor that has one"""
    n = name
    while n:
        m = re.search(r'get_%s: function\(\) \{\n\t\treturn \[([^\]]*)\];' % getter, class_src(n))
        if m:
            return [x.strip().split("_", 1)[1] for x in m.group(1).split(",") if x.strip()]
        n = parent(n)
    return []


per_class = {c: {k: possible(c, g) for k, g in (("up", "possibleUpgrades"), ("mode", "possibleBuildingModes"),
                                                 ("city", "possibleCityUpgrades"), ("pol", "possiblePolicies"))}
             for c in classes}
offered_modes = {m for c in classes for m in per_class[c]["mode"]}


def upgrade_class_info(name):
    """layer, texture, frames lost without a secret code, quality bonus, sprite not cached"""
    layer, tex, locked, bonus, nocache = None, "", 0, 0, False
    n = name
    seen_layer = False
    while n and n != "BuildingUpgrade":
        src = read("buildingUpgrades." + n)
        if not seen_layer:
            m = re.search(r'BuildingUpgrade\.call\(this,(\w+),building\)', src)
            if m:
                layer = {"bgStage": "LAYER_BG", "midStage": "LAYER_MID", "stage": "LAYER_MAIN"}.get(m.group(1))
                seen_layer = True
        if not tex:
            m = re.search(r'get_textureName: function\(\) \{\n\t\treturn "([^"]*)";', src)
            if m:
                tex = m.group(1)
        m = re.search(r'get_bonusAttractiveness: function\(\) \{\n\t\treturn (-?\d+);', src)
        if m and not bonus:
            bonus = int(m.group(1))
        m = re.search(r'get_availableTextures: function\(\) \{\n\t\treturn this\.textures\.length - \(Settings\.hasSecretCode\("orchid"\) \? 0 : (\d+)\)', src)
        if m:
            locked = int(m.group(1))
        if re.search(r'get_canCacheSprite: function\(\) \{\n\t\treturn false;', src):
            nocache = True
        m = re.search(r'buildingUpgrades_(\w+)\.call\(this,stage,midStage,bgStage,building\)', src)
        n = m.group(1) if m else None
    return layer, tex, locked, bonus, nocache


rows = []   # dicts
binfo = [b for b in json.load(open(os.path.join(REF, "buildingUpgradesInfo.json"))) if b["className"] not in EXCLUDE]
cinfo = [b for b in json.load(open(os.path.join(REF, "cityUpgradesInfo.json")))
         if b["className"] not in EXCLUDE and read("cityUpgrades." + b["className"])]
pinfo = [b for b in json.load(open(os.path.join(REF, "policiesInfo.json")))
         if b["className"] not in EXCLUDE and read("policies." + b["className"])]

for b in binfo:
    n = b["className"]
    if not read("buildingUpgrades." + n):
        continue
    layer, tex, locked, bonus, nocache = upgrade_class_info(n)
    spr = spr_id(tex) if tex else None
    if spr and spr not in spr_w:
        print("warning: no sprite", tex, "for", n)
        spr = None
    frames = (spr_w[spr] + 2) // 22 if spr else 0
    rows.append({"name": n, "kind": "UK_BUILDING_MODE" if n in offered_modes else "UK_BUILDING_UPGRADE", "info": b,
                 "layer": layer if spr else None, "sprite": spr, "frames": frames, "locked": locked, "bonus": bonus,
                 "nocache": nocache, "title": lang.get(f"buildingUpgradesInfo.json/{n}.name", n),
                 "text": lang.get(f"buildingUpgradesInfo.json/{n}.description", "")})
for b in cinfo:
    n = b["className"]
    rows.append({"name": n, "kind": "UK_CITY_UPGRADE", "info": b, "layer": None, "sprite": None, "frames": 0,
                 "locked": 0, "bonus": 0, "nocache": False,
                 "title": lang.get(f"cityUpgradesInfo.json/{n}.name", n),
                 "text": lang.get(f"cityUpgradesInfo.json/{n}.description", ""),
                 "titleDone": b.get("nameIfNotDone"), "textDone": b.get("descriptionIfNotDone"),
                 "next": b.get("nextUpgrade")})
for b in pinfo:
    n = b["className"]
    rows.append({"name": n, "kind": "UK_POLICY", "info": b, "layer": None, "sprite": None, "frames": 0, "locked": 0,
                 "bonus": 0, "nocache": False, "title": lang.get(f"policiesInfo.json/{n}.name", n),
                 "text": lang.get(f"policiesInfo.json/{n}.description", "")})
index = {r["name"]: i for i, r in enumerate(rows)}

h = ["/* The Final Earth 2: upgrade numbers and the classes' lists (made by tools/gen_upgrades.py). */",
     "#ifndef FE_UPGRADES_DATA_H", "#define FE_UPGRADES_DATA_H", "enum {"]
for i, r in enumerate(rows):
    h.append(f"  U_{r['name']} = {i},")
h += [f"  U_COUNT = {len(rows)}", "};", "",
      "/* a class's upgrade's bit in Perm.upgrades, and its modes' numbers (Perm.mode) */"]
for c in classes:
    for i, u in enumerate(per_class[c]["up"]):
        if u in index:
            h.append(f"#define UB_{c}_{u} {i}")
    for i, u in enumerate(per_class[c]["mode"]):
        if u in index:
            h.append(f"#define MODE_{c}_{u} {i}")
h += ["#endif"]
open(os.path.join(GAME, "src", "upgrades_data.h"), "w").write("\n".join(h) + "\n")

out = ["/* The Final Earth 2: building upgrades and modes, city upgrades and policies, made by tools/gen_upgrades.py",
       " * from the original's buildingUpgradesInfo.json, cityUpgradesInfo.json, policiesInfo.json, classes and",
       " * lang_en.csv. Don't edit: edit the generator. */",
       '#include "upgrades.h"', '#include "classes.h"', '#include "data.h"', '#include "pcls.h"', "",
       "const UpgradeInfo upgrade_info[] = {"]
for r in rows:
    b = r["info"]
    cost = ", ".join(f"[{M_ENUM[i]}] = {b.get(m, 0):g}" for i, m in enumerate(MATS) if b.get(m))
    uf = []
    if b.get("unlockedByDefault", True):
        uf.append("UF_DEFAULT")
    if b.get("notUnlockedWithAll"):
        uf.append("UF_NOT_WITH_ALL")
    f = [f'.name = "{r["name"]}"', f".kind = {r['kind']}", f".unlockFlags = {' | '.join(uf) or '0'}",
         f".title = {cstr(r['title'])}", f".text = {cstr(r['text'])}"]
    if r.get("titleDone"):
        f.append(f".titleDone = {cstr(r['titleDone'])}")
    if r.get("textDone"):
        f.append(f".textDone = {cstr(r['textDone'])}")
    if cost:
        f.append(f".cost = {{{cost}}}")
    f.append(f".layer = {r['layer'] or -1}")
    if r["sprite"]:
        f.append(f".sprite = {r['sprite']}")
        f.append(f".frames = {r['frames']}")
    if r["locked"]:
        f.append(f".lockedFrames = {r['locked']}")
    if r["bonus"]:
        f.append(f".bonusAttractiveness = {r['bonus']}")
    if r["nocache"]:
        f.append(".noCache = 1")
    f.append(f".next = {index[r['next']] if r.get('next') in index else 0xFF}")
    out.append("  {" + ", ".join(f) + "},")
out.append("};")
out.append("const int nupgrades = sizeof upgrade_info / sizeof upgrade_info[0];")
out.append("")
out.append("/* what each class offers in its window: upgrade_info indexes + 1 (0: none) */")
out.append("const ClassUpgrades class_upgrades[CLS_COUNT] = {")
for c in classes:
    pc = per_class[c]
    if not any(u in index for v in pc.values() for u in v):
        continue
    parts = []
    for k, n in (("up", 3), ("mode", 4), ("city", 6), ("pol", 3)):
        lst = [f"U_{u} + 1" for u in pc[k] if u in index]
        if len(lst) > n:
            raise SystemExit(f"{c}: too many {k}")
        if lst:
            parts.append(f".{k} = {{{', '.join(lst)}}}")
    out.append(f"  [CLS_{c}] = {{{', '.join(parts)}}},")
out.append("};")
out.append("")
out.append("int upgrade_find(const char *name) {\n  for (int i = 0; i < nupgrades; i++)\n"
           "    if (!strcmp(upgrade_info[i].name, name)) return i;\n  return -1;\n}")
open(os.path.join(GAME, "src", "upgrades_data.c"), "w").write("\n".join(out) + "\n")
print(f"{len(rows)} upgrades ({sum(1 for r in rows if r['kind'] == 'UK_BUILDING_MODE')} modes, "
      f"{sum(1 for r in rows if r['kind'] == 'UK_CITY_UPGRADE')} city upgrades, {len(pinfo)} policies)")
