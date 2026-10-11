#!/usr/bin/env python3
"""Makes src/classes.c and src/classes.h, the table of permanents' classes,
from the original's data (buildinginfo.json, buildableWorldResourcesInfo.json)
and its classes' code (the split/ folder of game_js.js): each class's base
classes, the numbers its constructor sets and the constants its getters
return, inherited from its ancestors.

A class's behaviour is a PermOps `ops_<Class>` defined in a src/bld_*.c file;
a class without one uses its nearest ancestor's (ops_Work, ops_House...).

    python3 -I tools/gen_classes.py [REFERENCE_DIR]
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
# out of scope: paid versions, holidays, snow
EXCLUDE_FLAGS = {"premium", "halloween"}
EXCLUDE = {"ChristmasTree", "ChristmasMarket", "UnknownBuilding", "ForestSnow",
           "CacaoFarm", "ChocolateFactory", "BoardGameStore", "ShadyHome", "TheContraption"}   # scrapped in the original
CATEGORIES = ["Houses", "Production", "Services", "Entertainment", "Transportation", "Unique Buildings",
              "Decoration & Nature", "Hidden", "None"]

static = open(os.path.join(SPLIT, "_static_init.js"), encoding="utf-8").read()
lang = open(os.path.join(REF, "lang_en.csv"), encoding="utf-8").read()
HINTS = set(re.findall(r'buildinginfo\.json/(\w+)\.showUnlockHint', lang))


def read_class(prefix, name):
    p = os.path.join(SPLIT, f"{prefix}.{name}.js")
    return open(p, encoding="utf-8").read() if os.path.exists(p) else ""


def class_file(name):
    for prefix in ("buildings", "worldResources", ""):
        p = os.path.join(SPLIT, f"{prefix}.{name}.js" if prefix else f"{name}.js")
        if os.path.exists(p):
            return open(p, encoding="utf-8").read()
    return ""


def parse(name):
    """super class, interfaces, constructor numbers, constant getters"""
    src = class_file(name)
    info = {"super": None, "ifaces": [], "ctor": {}, "get": {}}
    m = re.search(r'__super__ = (\w+);', src)
    if m:
        sup = m.group(1)
        info["super"] = sup.split("_", 1)[1] if sup.startswith(("buildings_", "worldResources_")) else sup
    m = re.search(r'__interfaces__ = \[([^\]]*)\]', src)
    if m:
        info["ifaces"] = [i.strip().split("_")[-1] for i in m.group(1).split(",") if i.strip()]
    ctor = src.split("$hxClasses", 1)[0]
    for k, v in re.findall(r'this\.(\w+) = (-?[\d.]+|true|false);', ctor):
        info["ctor"].setdefault(k, v)
    for k, v in re.findall(r'get_(\w+): function\(\) \{\n\t\treturn (-?[\d.]+|true|false|[\w.]+);\n\t\}', src):
        info["get"].setdefault(k, v)
    return info


def chain(prefix, name):
    """the class and its ancestors, nearest first"""
    out = []
    while name and name != "Permanent" and len(out) < 12:
        info = parse(name)
        out.append((name, info))
        name = info["super"]
    return out


def inherited(ch, kind, key, default=None):
    for _, info in ch:
        if key in info[kind]:
            return info[kind][key]
    return default


def flit(v):
    """a C float literal"""
    t = f"{v:g}"
    if "." not in t and "e" not in t:
        t += ".0"
    return t + "f"


def num(v, default=0):
    try:
        return float(v)
    except (TypeError, ValueError):
        return default


ops_defined = set()
for f in os.listdir(os.path.join(GAME, "src")):
    if f.startswith("bld") and f.endswith(".c"):
        ops_defined |= set(re.findall(r'^const PermOps ops_(\w+)\b', open(os.path.join(GAME, "src", f)).read(), re.M))

spr_known = set()
data_h = os.path.join(GAME, "src", "data.h")
if os.path.exists(data_h):
    spr_known = set(re.findall(r'\b(SPR_\w+)\b', open(data_h).read()))


def spr_id(name):
    if not name:
        return "0"
    s = "SPR_" + re.sub(r'[^A-Z0-9]', '_', name.replace("spr_", "", 1).upper())
    return s if (not spr_known or s in spr_known) else "0"


rows = []
binfo = json.load(open(os.path.join(REF, "buildinginfo.json")))
winfo = json.load(open(os.path.join(REF, "buildableWorldResourcesInfo.json")))
for r in ["Forest", "PineForest", "BranchyForest", "RedForest", "PurpleForest", "BirchForest", "Rock", "Boulder",
          "AlienRuins", "ComputerAlienRuins"]:
    info = next((w for w in winfo if w["className"] == r), {"className": r})
    ch = chain("worldResources", r)
    names = [n for n, _ in ch]
    base = []
    if any("Forest" in n for n in names):
        base.append("B_FOREST")
    if "Rock" in names or "Boulder" in names:
        base.append("B_ROCK")
    if "AlienRuins" in names:
        base.append("B_RUINS")
    rows.append({"name": r, "kind": "KIND_RESOURCE", "base": base, "info": info, "chain": ch, "category": "Decoration & Nature",
                 "sprite": (info.get("textureName") or "").split("@")[0]})

for b in binfo:
    n = b["className"]
    flags = set(b.get("specialInfo", []))
    if n in EXCLUDE or flags & EXCLUDE_FLAGS:
        continue
    ch = chain("buildings", n)
    names = [c for c, _ in ch]
    ifaces = [i for _, info in ch for i in info["ifaces"]]
    base = []
    if "House" in names or "IHousing" in ifaces:
        base.append("B_HOUSE")
    if "Work" in names:
        base.append("B_WORK")
    if "IEntertainmentBuilding" in ifaces or inherited(ch, "ctor", "isEntertainment") == "true":
        base.append("B_ENTERTAINMENT")
    if "IMedicalBuilding" in ifaces or inherited(ch, "ctor", "isMedical") == "true":
        base.append("B_MEDICAL")
    if "School" in names:
        base.append("B_SCHOOL")
    if "Teleporter" in names:
        base.append("B_TELEPORTER")
    if "LandingSite" in names:
        base.append("B_LANDINGSITE")
    if "HyperElevator" in names:
        base.append("B_ELEVATOR")
    if "unique" in flags:
        base.append("B_UNIQUE")
    if "rooftop" in flags:
        base.append("B_ROOFTOP")
    if "disableInsertReplaceOrBuildOnTop" in flags:
        base += ["B_NO_INSERT", "B_NO_BUILD_ON_TOP"]
    if "buildOnGround" in flags:
        base.append("B_BUILD_ON_GROUND")
    if "cityTop" in flags:
        base.append("B_CITY_TOP")
    if inherited(ch, "get", "walkThroughCanViewSelfInThisBuilding") == "false":
        base.append("B_HIDE_WALKERS")
    m = re.search(r'buildings_%s\.spriteName = "([^"]*)"' % n, static)
    rows.append({"name": n, "kind": "KIND_BUILDING", "base": base, "info": b, "chain": ch,
                 "category": "Decoration & Nature" if b["category"].startswith("Decoration & Nature") else b["category"],
                 "sprite": m.group(1) if m else ""})

out = []
out.append("/* The Final Earth 2: the classes of permanents, made by tools/gen_classes.py from the original's\n"
           " * buildinginfo.json, buildableWorldResourcesInfo.json and classes. Don't edit: edit the generator. */")
out.append('#include "classes.h"')
out.append('#include "data.h"' if spr_known else "")
ops_used = set()
for r in rows:
    for c, _ in r["chain"]:
        if c in ops_defined:
            ops_used.add(c)
            break
for o in sorted(ops_used):
    out.append(f"extern const PermOps ops_{o};")
out.append("const PermClass pcls[] = {")
for r in rows:
    b, ch = r["info"], r["chain"]
    cost = ", ".join(f"[{M_ENUM[i]}] = {num(b.get(m)):g}" for i, m in enumerate(MATS) if num(b.get(m)) or
                     (m == "refinedMetal" and num(b.get("refinedMetals"))))
    if num(b.get("refinedMetals")) and not num(b.get("refinedMetal")):
        cost = cost.replace(f"[M_REFINEDMETAL] = 0", f"[M_REFINEDMETAL] = {num(b.get('refinedMetals')):g}")
    fields = [f'.name = "{r["name"]}"', f'.kind = {r["kind"]}', f'.base = {" | ".join(r["base"]) or "0"}',
              f'.category = {CATEGORIES.index(r["category"]) if r["category"] in CATEGORIES else 8}']
    if cost:
        fields.append(f".cost = {{{cost}}}")
    if b.get("residents"):
        fields.append(f".residents = {int(b['residents'])}")
    if b.get("jobs"):
        fields.append(f".jobs = {int(b['jobs'])}")
    if b.get("quality") is not None:
        fields.append(f".quality = {int(b['quality'])}")
    fields.append(f".startTime = {num(inherited(ch, 'ctor', 'startTime', 6), 6):g}")
    fields.append(f".endTime = {num(inherited(ch, 'ctor', 'endTime', 19), 19):g}")
    fields.append(f".doorX = {int(num(inherited(ch, 'ctor', 'doorX', 13), 13))}")
    fields.append(f".workTimePreferenceMod = {num(inherited(ch, 'ctor', 'workTimePreferenceMod', 1), 1):g}")
    et = inherited(ch, "get", "entertainmentType")
    fields.append(f".entType = {int(num(et, -1)) if et is not None else -1}")
    for k, f in (("minimumNormalTimeToSpend", "entMinTime"), ("maximumNormalTimeToSpend", "entMaxTime"),
                 ("minimumEntertainmentGroupSatisfy", "entMinSatisfy"), ("maximumEntertainmentGroupSatisfy", "entMaxSatisfy")):
        v = inherited(ch, "get", k)
        if v is not None and re.match(r'^-?[\d.]+$', v):
            fields.append(f".{f} = {flit(num(v))}")
    v = inherited(ch, "get", "entertainmentQuality")
    if v is not None and re.match(r'^-?[\d.]+$', v):
        fields.append(f".entQuality = {int(num(v))}")
    v = inherited(ch, "get", "medicalTypeID")
    fields.append(f".medicalTypeID = {int(num(v, 0)) if v is not None and re.match(r'^-?[0-9]+$', v) else 0}")
    v = inherited(ch, "get", "medicalTypeLimit")
    if v is not None and re.match(r'^-?[\d.]+$', v):
        fields.append(f".medicalTypeLimit = {flit(num(v))}")
    if b.get("teleporterOperatingCost"):
        fields.append(f".teleporterOperatingCost = {flit(num(b['teleporterOperatingCost']))}")
    for s in b.get("specialInfo", []):
        m = re.match(r'limitedToCitizens:(\d+)', s)
        if m:
            fields.append(f".limitedPerCitizens = {int(m.group(1))}")
    uf = []
    if b.get("unlockedByDefault", r["kind"] == "KIND_RESOURCE"):
        uf.append("UF_DEFAULT")
    if r["name"] in HINTS:
        uf.append("UF_HINT")
    if b.get("notUnlockedWithAll"):
        uf.append("UF_NOT_WITH_ALL")
    if uf:
        fields.append(f".unlockFlags = {' | '.join(uf)}")
    names_in_table = [x["name"] for x in rows]
    parent = next((names_in_table.index(c) for c, _ in ch[1:] if c in names_in_table), -1)
    fields.append(f".parent = {parent}")
    fields.append(f".sprite = {spr_id(r['sprite'])}")
    for c, _ in ch:
        if c in ops_defined:
            fields.append(f".ops = &ops_{c}")
            break
    out.append("  {" + ", ".join(fields) + "},")
out.append("};")
out.append("const int npcls = sizeof pcls / sizeof pcls[0];")
out.append("int pcls_find(const char *name) {\n  for (int i = 0; i < npcls; i++)\n    if (!strcmp(pcls[i].name, name)) return i;\n  return -1;\n}")
open(os.path.join(GAME, "src", "classes.c"), "w").write("\n".join(l for l in out if l is not None) + "\n")

h = ["/* The Final Earth 2: class numbers (made by tools/gen_classes.py). */", "#ifndef FE_CLASSES_H", "#define FE_CLASSES_H",
     '#include "pcls.h"', "enum {"]
for i, r in enumerate(rows):
    h.append(f"  CLS_{r['name']} = {i},")
h += [f"  CLS_COUNT = {len(rows)}", "};", "#endif"]
open(os.path.join(GAME, "src", "classes.h"), "w").write("\n".join(h) + "\n")
print(f"{len(rows)} classes ({sum(1 for r in rows if r['kind'] == 'KIND_BUILDING')} buildings), ops for {len(ops_used)}")
