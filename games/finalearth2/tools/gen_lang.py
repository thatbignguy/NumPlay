#!/usr/bin/env python3
"""Makes src/lang_data.c and src/lang_data.h: the texts the user interface
shows (the original's lang_en.csv) and its tables of the build menu (the
buildings' names, descriptions, unlock hints and button art, the categories,
the decorations and the world resources one can build).

Only the texts the game shows go in: every `L_<key>` in src/ui*.c and
src/bld*.c is the text of lang_en.csv's <key>. Texts written for a mouse or a touch screen get
the keypad's words (KEYPAD below).

    python3 -I tools/gen_lang.py [REFERENCE_DIR]
"""
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
GAME = os.path.dirname(HERE)
SRC = os.path.join(GAME, "src")
REF = sys.argv[1] if len(sys.argv) > 1 else os.environ.get("FE2_REF", os.path.expanduser("~/Documents/Assets/gamesrc/finalearth2/web14"))

lang = {}
for line in open(os.path.join(REF, "lang_en.csv"), encoding="utf-8"):
    line = line.rstrip("\n")
    if "|" not in line:
        continue
    k, v = line.split("|", 1)
    # (a key given twice: the later one, as the original's LocalizeLoader keeps it)
    lang[k] = v.replace("\\n", "\n")

# texts that speak of clicking, tapping or keys of a computer: the calculator's words instead
KEYPAD = {
    "tap_for_age_distribution": "Press OK for the age distribution.",
    "tap_for_age_distribution_or_pop_limit": "Press OK for the age distribution or to change the population limit.",
    "change_game_speed_mobile": "Press OK to change the game speed.",
    "change_game_speed_pc": "Press OK to change the game speed.\nPress shift and OK to select an advanced speed.",
    "to_pause": "to pause the game.",
    "to_unpause": "to unpause the game.",
    "destroy_tip": "",
    "ctrl_to_highlight": "",
    "follow_hint": "",
    "click_here_to_play": "Press OK to Play",
    "hint_hint": "Tip! If you ever forget what to do, just press OK on the ? here (press var to reach it).",
    "tap_near_citizen_to_follow": "Press OK near a citizen to follow them.",
    "build_mode_multi_explanation": "Hold OK and move the cursor to quickly build multiple buildings of the same type.",
    "build_mode_multi_replace_explanation": "Hold OK and move the cursor to quickly build or replace buildings.",
}
# the start of the pause button's tooltip: "Click (or press Space)" / "Tap"
EXTRA = {
    "ok_or_alpha": "Press OK (or alpha)",
    "keypad_credits_1": "Inspired by The Final Earth 2 by Florian van Strien.",
    "keypad_credits_2": "Not affiliated with him.",
    "keypad_credits_3": "The sprites, fonts and texts are his, from the original game.",
    "keypad_credits_4": "Made by Mason Chen as part of NumPlay.",
    "keypad_save_failed": "The city could not be saved: there is not enough room on the calculator.",
    "keypad_save_failed_title": "Not Saved",
    "keypad_saved": "The city was saved.",
    "keypad_home_quit": "Press Home again to quit without saving.",
    "keypad_hold": "Press OK again to confirm.",
    "save": "Save",
    "priority_text": " priority.",
    "keypad_load_failed": "The saved city could not be loaded.",
    "keypad_toolbox_hint": "Press toolbox to open the build menu.",
    "keypad_typing_hint": "Type with alpha and the keys' letters (shift, alpha: capitals), or pick them here:",
    "keypad_caps": "Caps",
    "keypad_space": "Space",
    "keypad_delete": "Delete",
    "keypad_var_hint": "Press var to reach these buttons.",
    "keypad_builder_hint": "Arrows move, OK builds here, Back picks another building.",
    "keypad_menu_hint": "Left and right: buildings. Up and down: categories. OK: pick.",
}


def adapt(key, text):
    if key in KEYPAD:
        return KEYPAD[key]
    text = text.replace("[Tap]", "Press OK").replace("[tap]", "press OK")
    return text


def cname(key):
    return "L_" + re.sub(r"[^A-Za-z0-9_]", "_", key)


def cstr(s):
    if s is None:
        return "NULL"
    out = []
    for ch in s:
        if ch == "\\":
            out.append("\\\\")
        elif ch == '"':
            out.append('\\"')
        elif ch == "\n":
            out.append("\\n")
        elif ord(ch) < 128:
            out.append(ch)
        else:
            out.append("".join("\\%03o" % b for b in ch.encode("utf-8")))
    return '"' + "".join(out) + '"'


# the keys the interface uses
used = set()
for f in sorted(os.listdir(SRC)):
    if (f.startswith("ui") or f.startswith("bld")) and f.endswith(".c"):
        used |= set(re.findall(r"\bL_([A-Za-z0-9_]+)\b", open(os.path.join(SRC, f), encoding="utf-8").read()))

# the sprites packed (data.h) and the classes (classes.h)
sprites = set(re.findall(r"^\s+SPR_([A-Z0-9_]+),", open(os.path.join(SRC, "data.h"), encoding="utf-8").read(), re.M))
classes = re.findall(r"CLS_(\w+) = (\d+),", open(os.path.join(SRC, "classes.h"), encoding="utf-8").read())
cls_index = {n: int(i) for n, i in classes}
ncls = len(classes)


def spr(name):
    if not name or name == "none":
        return "0"
    n = name[4:].upper() if name.startswith("spr_") else name.upper()
    n = n.split("@")[0]
    if n not in sprites:
        raise SystemExit(f"gen_lang.py: no sprite {name}")
    return "SPR_" + n


info = json.load(open(os.path.join(REF, "buildinginfo.json"), encoding="utf-8"))
by_class = {b["className"]: b for b in info}
res_info = json.load(open(os.path.join(REF, "buildableWorldResourcesInfo.json"), encoding="utf-8"))
deco_info = json.load(open(os.path.join(REF, "decorationsInfo.json"), encoding="utf-8"))
cats = json.load(open(os.path.join(REF, "buildingCategoriesInfo.json"), encoding="utf-8"))
SUBS = ["coloredBlocks", "slantedroofs", "slantedrooftops", "roofbottom", "domeroofs"]
SUB_TEXT = {"coloredBlocks": ("colored_blocks", "spr_coloredblock"), "slantedroofs": ("slanted_roofs", "spr_slantedroof"),
            "slantedrooftops": ("slanted_rooftops", "spr_slantedrooftop"), "roofbottom": ("roof_bottoms", "spr_roofbottom"),
            "domeroofs": ("domeroofs", "spr_domeroof")}
CAT_NAMES = [c["name"] for c in cats]

h = ["/* The Final Earth 2: the user interface's texts and build menu tables (made by tools/gen_lang.py",
     " * from the original's lang_en.csv and data). Don't edit: edit the generator. */",
     "#ifndef FE_LANG_DATA_H", "#define FE_LANG_DATA_H", "#include <stdint.h>", ""]
c = ["/* The Final Earth 2: the user interface's texts and build menu tables (made by tools/gen_lang.py",
     " * from the original's lang_en.csv and data). Don't edit: edit the generator. */",
     '#include "lang_data.h"', '#include "classes.h"', '#include "data.h"', ""]

missing = []
for k in sorted(used):
    if k in EXTRA:
        text = EXTRA[k]
    elif k in lang:
        text = adapt(k, lang[k])
    else:
        missing.append(k)
        continue
    h.append(f"extern const char {cname(k)}[];")
    c.append(f"const char {cname(k)}[] = {cstr(text)};")
if missing:
    raise SystemExit("gen_lang.py: no text for " + ", ".join(missing))

# ---------------------------------------------------------------- the classes
h += ["", "/* a class's texts and look in the build menu */",
      "typedef struct {",
      "  const char *name, *desc, *hint;   /* hint: showUnlockHint, NULL none */",
      "  uint16_t back;                    /* its button's background (buttonBack; BACK_OWN: its sprite's at 44,0), 0 none */",
      "  uint16_t on_build;                /* onBuildSprite, 0: the class's sprite */",
      "  uint16_t tip_icon;                /* tooltipBottomIconInfo, 0 none */",
      "  uint8_t flags;                    /* CT_* */",
      "  uint8_t sub;                      /* its decoration subcategory + 1 (SUB_*), 0 none */",
      "  uint16_t limited;                 /* limitedToCitizens: one per that many citizens, 0 none */",
      "} ClsText;",
      "#define BACK_OWN 0xFFFF",
      "enum { CT_MULTI_DECOR = 1, CT_ANYCOLOR = 2, CT_MIRRORABLE = 4, CT_MULTI_SPEC = 8, CT_NOT_UNIQUE_GL = 16,",
      "       CT_UNIQUE = 32, CT_LIMITED = 64 };",
      "extern const ClsText cls_text[];",
      "/* the buildings in the original's order (buildinginfo.json), for the build menu */",
      "extern const uint8_t build_order[];",
      "extern const int nbuild_order;"]
c.append("const ClsText cls_text[] = {")


def resource_name(name):
    """A world resource's get_name (its window's title), following its superclasses"""
    while name:
        p = os.path.join(REF, "split", f"worldResources.{name}.js")
        if not os.path.exists(p):
            return None
        js = open(p, encoding="utf-8").read()
        m = re.search(r'get_name: function\(\) \{\s*return (?:common_Localize\.lo\("([^"]+)"\)|"([^"]*)")', js)
        if m:
            return lang.get(m.group(1)) if m.group(1) else m.group(2)
        m = re.search(r'__super__ = worldResources_(\w+);', js)
        name = m.group(1) if m else None
    return None


rows = []
for name, i in sorted(cls_index.items(), key=lambda x: x[1]):
    b = by_class.get(name)
    key = f"buildinginfo.json/{name}."
    rkey = f"buildableWorldResourcesInfo.json/{name}."
    nm = lang.get(key + "name") or resource_name(name) or lang.get(rkey + "name") or (b or {}).get("name") or name
    desc = lang.get(key + "description") or lang.get(rkey + "description") or (b or {}).get("description") or ""
    hint = lang.get(key + "showUnlockHint")
    si = (b or {}).get("specialInfo", [])
    flags = []
    if "as_multi_decor" in si: flags.append("CT_MULTI_DECOR")
    if "as_multi_decor_anycolor" in si: flags.append("CT_ANYCOLOR")
    if "mirrorable" in si: flags.append("CT_MIRRORABLE")
    if "has_multi_decor_spec" in si: flags.append("CT_MULTI_SPEC")
    if "notUniqueIfGalacticLibrary" in si: flags.append("CT_NOT_UNIQUE_GL")
    if "unique" in si: flags.append("CT_UNIQUE")
    if "limited" in si: flags.append("CT_LIMITED")
    limited = 0
    for s in si:
        if s.startswith("limitedToCitizens:"):
            limited = int(s.split(":")[1])
    back = "0"
    if b is not None:
        bb = b.get("buttonBack")
        if bb == "none":
            back = "0"
        elif bb:
            back = spr(bb)
        else:
            back = "BACK_OWN"   # the class's own sprite at 44,0
    tips = (b or {}).get("tooltipBottomIconInfo") or []
    tip = spr(tips[0]["texture"]) if tips else "0"
    cat = (b or {}).get("category", "")
    sub = 0
    if "_" in cat:
        sub = SUBS.index(cat.split("_", 1)[1]) + 1
    rows.append(f"  {{{cstr(nm)}, {cstr(desc)}, {cstr(hint)}, {back}, {spr((b or {}).get('onBuildSprite'))}, {tip}, "
                f"{' | '.join(flags) or '0'}, {sub}, {limited}}}")
c.append(",\n".join(rows) + "};")
order = [cls_index[b["className"]] for b in info if b["className"] in cls_index]
c.append("const uint8_t build_order[] = {" + ", ".join(str(x) for x in order) + "};")
c.append(f"const int nbuild_order = {len(order)};")

# ---------------------------------------------------------------- categories
h += ["", "/* buildingCategoriesInfo.json, and the decorations' subcategories */",
      "typedef struct { const char *name, *desc; uint16_t icon; } CatText;",
      f"#define NCATEGORIES {len(cats)}",
      "extern const CatText cat_text[NCATEGORIES];",
      f"#define NSUBCATEGORIES {len(SUBS)}",
      "extern const CatText sub_text[NSUBCATEGORIES];"]
c.append("const CatText cat_text[NCATEGORIES] = {" + ", ".join(
    f"{{{cstr(lang.get('categories/' + x['name'] + '.name', x['name']))}, "
    f"{cstr(lang.get('categories/' + x['name'] + '.description', x['description']))}, {spr(x['image'])}}}" for x in cats) + "};")
c.append("const CatText sub_text[NSUBCATEGORIES] = {" + ", ".join(
    f"{{{cstr(lang[SUB_TEXT[s][0]])}, NULL, {spr(SUB_TEXT[s][1])}}}" for s in SUBS) + "};")

# ---------------------------------------------------------------- decorations and world resources
h += ["", "/* decorationsInfo.json (snow left out): what one places on a world's ground */",
      "typedef struct { uint16_t spr; uint8_t flags; uint8_t food, wood, stone; const char *name, *desc; } DecoText;",
      "enum { DT_GENE_EDITING = 1, DT_REMOVE = 2 };",
      "extern const DecoText deco_text[];",
      "extern const int ndeco_text;",
      "/* buildableWorldResourcesInfo.json: forests and rocks one can place */",
      "typedef struct { uint8_t cls; int16_t grow, resources; float wood, stone; const char *name, *desc; } ResText;",
      "extern const ResText res_text[];",
      "extern const int nres_text;"]
drows = []
for d in deco_info:
    si = d.get("specialInfo") or []
    if "snow" in si:
        continue
    fl = []
    if "geneEditingReq" in si: fl.append("DT_GENE_EDITING")
    if d.get("isRemoveDecoration"): fl.append("DT_REMOVE")
    key = f"decorationsInfo.json/{d['textureName']}."
    drows.append(f"  {{{spr(d['textureName'])}, {' | '.join(fl) or '0'}, {d['food']}, {d['wood']}, {d['stone']}, "
                 f"{cstr(lang.get(key + 'name', d['textureName']))}, {cstr(lang.get(key + 'description'))}}}")
c.append("const DecoText deco_text[] = {\n" + ",\n".join(drows) + "};")
c.append(f"const int ndeco_text = {len(drows)};")
rrows = []
for r in res_info:
    if r["className"] not in cls_index:
        continue
    key = f"buildableWorldResourcesInfo.json/{r['className']}."
    rrows.append(f"  {{CLS_{r['className']}, {r.get('initialGrow', -1)}, {r.get('initialResources', -1)}, {r['wood']}, "
                 f"{r['stone']}, {cstr(lang.get(key + 'name', r['className']))}, {cstr(lang.get(key + 'description'))}}}")
c.append("const ResText res_text[] = {\n" + ",\n".join(rrows) + "};")
c.append(f"const int nres_text = {len(rrows)};")

# ---------------------------------------------------------------- citizens' names (Resources.citizenNames)
names = open(os.path.join(REF, "citizenNames.txt"), encoding="utf-8").read().replace("\r\n", "\n").split("\n")
names = names[names.index("START") + 1:]
# front-coded: each name is the length it shares with the one before, then the rest and a 0; every 32nd
# shares nothing and has its offset in the index
blob = bytearray()
index = []
prev = b""
for i, n in enumerate(names):
    b = n.encode("utf-8")
    if i % 32 == 0:
        index.append(len(blob))
        shared = 0
    else:
        shared = 0
        while shared < min(len(prev), len(b), 255) and prev[shared] == b[shared]:
            shared += 1
    blob += bytes([shared]) + b[shared:] + b"\0"
    prev = b
h += ["", "/* the citizens' names (citizenNames.txt): name i, front-coded */",
      "extern const int ncitizen_names;",
      "const char *citizen_name(int i, char *buf, int size);"]
c.append(f"const int ncitizen_names = {len(names)};")
c.append("static const uint8_t names_blob[] = {" + ",".join(str(x) for x in blob) + "};")
c.append("static const uint32_t names_index[] = {" + ",".join(str(x) for x in index) + "};")
c.append("""const char *citizen_name(int i, char *buf, int size) {
  buf[0] = 0;
  if (i < 0 || i >= ncitizen_names || size < 2) return buf;
  const uint8_t *p = names_blob + names_index[i / 32];
  int len = 0;
  for (int k = i - i % 32; k <= i; k++) {
    int shared = *p++;
    if (shared > len) shared = len;
    len = shared;
    while (*p) {
      if (len < size - 1) buf[len++] = (char)*p;
      p++;
    }
    p++;
    buf[len] = 0;
  }
  return buf;
}""")
h += ["#endif", ""]
open(os.path.join(SRC, "lang_data.h"), "w", encoding="utf-8").write("\n".join(h))
open(os.path.join(SRC, "lang_data.c"), "w", encoding="utf-8").write("\n".join(c) + "\n")
print(f"gen_lang.py: {len(used)} texts, {ncls} classes, {len(drows)} decorations, {len(rrows)} world resources")
