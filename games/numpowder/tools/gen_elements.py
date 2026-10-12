#!/usr/bin/env python3
"""NumPowder: the element table, from The Powder Toy's element files.

Each of The Powder Toy's src/simulation/elements/XXXX.cpp sets an element's
properties in its Element::Element_XXXX() function; their numbers come from
the list in elements/meson.build. This writes:

- src/elements.h: the elements' numbers (PT_*) and the Element struct;
- src/elements_data.c: the table, each property as the original writes it
  (the C compiler works out expressions such as 0.04f * CFDS), and the
  element's functions, declared weak: one not ported yet is NULL.

Usage: gen_elements.py [the Powder Toy's source folder]
"""
import os
import re
import sys

TPT = sys.argv[1] if len(sys.argv) > 1 else os.environ.get(
    "TPT_SRC", os.path.expanduser("~/Documents/Assets/gamesrc/powdertoy"))
ELEM = os.path.join(TPT, "src", "simulation", "elements")
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src")

names = re.findall(r"'([A-Z0-9_]+)'", open(os.path.join(ELEM, "meson.build")).read().split("]")[0])

# the properties, their C type in the table and their default (Element::Element())
PROPS = [
    ("Identifier", "const char *", '""'), ("Name", "const char *", '""'), ("Colour", "uint32_t", "0xFFFFFF"),
    ("MenuVisible", "uint8_t", "0"), ("MenuSection", "uint8_t", "SC_SPECIAL"), ("Enabled", "uint8_t", "0"),
    ("Advection", "float", "0"), ("AirDrag", "float", "0"), ("AirLoss", "float", "1"), ("Loss", "float", "1"),
    ("Collision", "float", "0"), ("Gravity", "float", "0"), ("NewtonianGravity", "float", "1"),
    ("Diffusion", "float", "0"), ("HotAir", "float", "0"), ("Falldown", "int8_t", "0"),
    ("Flammable", "int16_t", "0"), ("Explosive", "int16_t", "0"), ("Meltable", "int16_t", "0"),
    ("Hardness", "int16_t", "30"), ("PhotonReflectWavelengths", "uint32_t", "0x3FFFFFFF"),
    ("Weight", "int16_t", "50"), ("HeatConduct", "uint8_t", "128"), ("HeatCapacity", "float", "1"),
    ("Description", "const char *", '""'), ("Properties", "uint32_t", "0"), ("CarriesTypeIn", "uint32_t", "0"),
    ("LowPressure", "float", "IPL"), ("LowPressureTransition", "int16_t", "NT"),
    ("HighPressure", "float", "IPH"), ("HighPressureTransition", "int16_t", "NT"),
    ("LowTemperature", "float", "ITL"), ("LowTemperatureTransition", "int16_t", "NT"),
    ("HighTemperature", "float", "ITH"), ("HighTemperatureTransition", "int16_t", "NT"),
]
DEFAULTS = [("temp", "float", "R_TEMP + 273.15f"), ("life", "int", "0"), ("ctype", "int", "0"), ("tmp", "int", "0"),
            ("tmp2", "int", "0"), ("tmp3", "int", "0"), ("tmp4", "int", "0")]
FUNCS = {"Update": "update", "Graphics": "graphics", "Create": "create", "CreateAllowed": "create_allowed",
         "ChangeType": "change_type", "CtypeDraw": "ctype_draw"}
PROTOS = {"Update": "int %s(UPDATE_FUNC_ARGS)", "Graphics": "int %s(GRAPHICS_FUNC_ARGS)",
          "Create": "void %s(ELEMENT_CREATE_FUNC_ARGS)", "CreateAllowed": "bool %s(ELEMENT_CREATE_ALLOWED_FUNC_ARGS)",
          "ChangeType": "void %s(ELEMENT_CHANGETYPE_FUNC_ARGS)", "CtypeDraw": "bool %s(CTYPEDRAW_FUNC_ARGS)"}


def c_expr(v):
    v = v.strip()
    v = re.sub(r"(0x[0-9A-Fa-f]+)_rgb", r"\1", v)
    v = v.replace("UINT32_C(", "(uint32_t)(")
    v = re.sub(r"\bString\(", "(", v)
    v = re.sub(r"(\d)U\b", r"\1u", v)
    return v


def body_of(name, text):
    m = re.search(r"void\s+Element::Element_%s\s*\(\s*\)\s*\{" % name, text)
    if not m:
        raise SystemExit("no Element_%s()" % name)
    i, depth = m.end(), 1
    while depth:
        depth += {"{": 1, "}": -1}.get(text[i], 0)
        i += 1
    return text[m.end():i - 1]


def strip_comments(s):
    s = re.sub(r"/\*.*?\*/", "", s, flags=re.S)
    return re.sub(r"//[^\n]*", "", s)


elements, used_funcs = [], {}
for n in names:
    text = open(os.path.join(ELEM, n + ".cpp"), encoding="utf-8").read()
    body = strip_comments(body_of(n, text))
    props, defaults, funcs = {}, {}, {}
    for stmt in body.split(";"):
        stmt = " ".join(stmt.split())
        m = re.match(r"^([A-Za-z]+(?:\.[a-z0-9]+)?) = (.*)$", stmt)
        if not m:
            continue
        key, val = m.groups()
        if key.startswith("DefaultProperties."):
            defaults[key.split(".")[1]] = c_expr(val)
        elif key in FUNCS:
            f = val.lstrip("&").strip()
            if f in ("nullptr", "NULL"):
                continue
            if f.startswith("Element::"):
                cname = f.split("::")[1]                       # shared: Element::ctypeDrawVInTmp
            elif f.startswith("Element_"):
                cname = f                                      # another element's: Element_FIRE_update
            else:
                cname = "el_%s_%s" % (n, f)                   # the file's own: update -> el_SAND_update
            funcs[key] = cname
            used_funcs[cname] = key
        elif key == "IconGenerator":
            continue
        else:
            props[key] = c_expr(val)
    elements.append((n, props, defaults, funcs))

# --------------------------------------------------------------------------- elements.h
h = ["/* NumPowder: The Powder Toy's elements, their numbers and properties (generated by tools/gen_elements.py",
     " * from the original's element files: do not edit). */",
     "#ifndef NP_ELEMENTS_H", "#define NP_ELEMENTS_H", "#include \"simdefs.h\"", "",
     "enum {"]
for i, (n, _, _, _) in enumerate(elements):
    h.append("  PT_%s = %d," % (n, i))
h += ["  PT_COUNT = %d" % len(elements), "};", "",
      "typedef struct {"]
for k, t, _ in PROPS:
    h.append("  %s %s;" % (t, k))
h.append("  struct { float temp; int life, ctype, tmp, tmp2, tmp3, tmp4; } DefaultProperties;")
for k in FUNCS:
    h.append("  " + PROTOS[k].replace("%s", "(*%s)") % k + ";")
h += ["} Element;", "extern const Element elements[PT_NUM];", "", "#endif", ""]

# --------------------------------------------------------------------------- elements_data.c
c = ["/* NumPowder: The Powder Toy's element table (generated by tools/gen_elements.py: do not edit). Each element's",
     " * functions are weak: one not ported yet is NULL. */",
     "#include \"sim.h\"", ""]
for f, k in sorted(used_funcs.items()):
    c.append("%s __attribute__((weak));" % (PROTOS[k] % f))
c += ["", "const Element elements[PT_NUM] = {"]
for n, props, defaults, funcs in elements:
    c.append("  [PT_%s] = {" % n)
    for k, t, d in PROPS:
        v = props.get(k, d)
        c.append("    .%s = %s," % (k, v))
    dv = ", ".join(".%s = %s" % (k, defaults.get(k, d)) for k, _, d in DEFAULTS)
    c.append("    .DefaultProperties = {%s}," % dv)
    for k in FUNCS:
        if k in funcs:
            c.append("    .%s = %s," % (k, funcs[k]))
    c.append("  },")
c += ["};", ""]

# --------------------------------------------------------------------------- canmove_data.c
# SimulationData::init_can_move, worked out here from the elements' Properties and Weight (read as data: a list
# of flags joined by |, an integer)
FLAGS = {"TYPE_PART": 0x1, "TYPE_LIQUID": 0x2, "TYPE_SOLID": 0x4, "TYPE_GAS": 0x8, "TYPE_ENERGY": 0x10,
         "PROP_CONDUCTS": 0x20, "PROP_PHOTPASS": 0x40, "PROP_NEUTPENETRATE": 0x80, "PROP_NEUTABSORB": 0x100,
         "PROP_NEUTPASS": 0x200, "PROP_DEADLY": 0x400, "PROP_HOT_GLOW": 0x800, "PROP_LIFE": 0x1000,
         "PROP_RADIOACTIVE": 0x2000, "PROP_LIFE_DEC": 0x4000, "PROP_LIFE_KILL": 0x8000,
         "PROP_LIFE_KILL_DEC": 0x10000, "PROP_SPARKSETTLE": 0x20000, "PROP_NOAMBHEAT": 0x40000,
         "PROP_NOCTYPEDRAW": 0x100000}


def flags_of(v):
    r = 0
    for tok in v.replace("(", " ").replace(")", " ").split("|"):
        tok = tok.strip()
        if tok in FLAGS:
            r |= FLAGS[tok]
        elif re.fullmatch(r"(0x[0-9A-Fa-f]+|\d+)u?", tok):
            r |= int(tok.rstrip("u"), 0)
        else:
            raise SystemExit("can't read Properties %r" % v)
    return r


def int_of(v):
    v = v.strip()
    if not re.fullmatch(r"-?\d+", v):
        raise SystemExit("can't read Weight %r" % v)
    return int(v)


N = len(elements)
num = {n: i for i, (n, _, _, _) in enumerate(elements)}
props = [flags_of(p.get("Properties", "TYPE_SOLID")) for _, p, _, _ in elements]
weight = [int_of(p.get("Weight", "50")) for _, p, _, _ in elements]
cm = [[1] * N for _ in range(N)]
for d in range(N):
    cm[0][d] = 0
PHOT, NEUT = num["PHOT"], num["NEUT"]
for d in range(1, N):
    cm[PHOT][d] = 2
for m in range(1, N):
    for d in range(1, N):
        if weight[m] <= weight[d] or d == num["GEL"]:
            cm[m][d] = 0
        if m == NEUT and props[d] & FLAGS["PROP_NEUTPASS"]:
            cm[m][d] = 2
        if m == NEUT and props[d] & FLAGS["PROP_NEUTABSORB"]:
            cm[m][d] = 1
        if m == NEUT and props[d] & FLAGS["PROP_NEUTPENETRATE"]:
            cm[m][d] = 1
        if d == NEUT and props[m] & FLAGS["PROP_NEUTPENETRATE"]:
            cm[m][d] = 0
        if props[m] & FLAGS["TYPE_ENERGY"] and props[d] & FLAGS["TYPE_ENERGY"]:
            cm[m][d] = 2
for d in range(N):
    stkm_move = 0
    if props[d] & (FLAGS["TYPE_LIQUID"] | FLAGS["TYPE_GAS"]):
        stkm_move = 2
    if not d or d in (num["PRTO"], num["SPAWN"], num["SPAWN2"]):
        stkm_move = 2
    for s_ in ("STKM", "STKM2", "FIGH"):
        cm[num[s_]][d] = stkm_move
    cm[num["SPRK"]][d] = 0
for m in range(1, N):
    for d_ in ("BHOL", "NBHL"):
        cm[m][num[d_]] = 1
    for d_ in ("STKM", "STKM2", "FIGH"):
        cm[m][num[d_]] = 0
    cm[m][num["INVIS"]] = 3
    cm[m][num["CNCT"]] = 0
    cm[m][num["PVOD"]] = 3
    cm[m][num["VOID"]] = 3
    cm[m][num["EMBR"]] = 0
    cm[num["EMBR"]][m] = 0
    if props[m] & FLAGS["TYPE_ENERGY"]:
        cm[m][num["VIBR"]] = 1
        cm[m][num["BVBR"]] = 1
    if props[m] & FLAGS["TYPE_PART"]:
        cm[m][num["SAWD"]] = 0
for d in range(N):
    if props[d] & FLAGS["PROP_PHOTPASS"]:
        cm[PHOT][d] = 2
    if d not in [num[k] for k in ("DMND", "INSL", "VOID", "PVOD", "VIBR", "BVBR", "PRTI", "PRTO")]:
        cm[num["PROT"]][d] = 2
        cm[num["GRVT"]][d] = 2
for a, b, v in [("DEST", "DMND", 0), ("DEST", "CLNE", 0), ("DEST", "PCLN", 0), ("DEST", "BCLN", 0), ("DEST", "PBCN", 0),
                ("DEST", "ROCK", 0), ("NEUT", "INVIS", 2), ("ELEC", "LCRY", 2), ("ELEC", "EXOT", 2), ("ELEC", "GLOW", 2),
                ("PHOT", "LCRY", 3), ("PHOT", "GPMP", 3), ("PHOT", "BIZR", 2), ("ELEC", "BIZR", 2), ("PHOT", "BIZRG", 2),
                ("ELEC", "BIZRG", 2), ("PHOT", "BIZRS", 2), ("ELEC", "BIZRS", 2), ("BIZR", "FILT", 2), ("BIZRG", "FILT", 2),
                ("ANAR", "WHOL", 1), ("ANAR", "NWHL", 1), ("ELEC", "DEUT", 1), ("THDR", "THDR", 2), ("EMBR", "EMBR", 2),
                ("TRON", "SWCH", 3), ("ELEC", "RSST", 2), ("ELEC", "RSSS", 2)]:
    cm[num[a]][num[b]] = v
stride = (N + 3) // 4
m_ = ["/* NumPowder: The Powder Toy's can_move table (SimulationData::init_can_move), 2 bits a pair (generated by",
      " * tools/gen_elements.py: do not edit). can_move[moving][destination]: 0 bounce, 1 swap, 2 same place, 3 varies */",
      "#include \"simpriv.h\"", "",
      "const uint8_t can_move_table[PT_COUNT][CANMOVE_STRIDE] = {"]
for m in range(N):
    row = []
    for k in range(stride):
        b = 0
        for q in range(4):
            d = k * 4 + q
            if d < N:
                b |= cm[m][d] << (2 * q)
        row.append("0x%02X" % b)
    m_.append("  {" + ",".join(row) + "},")
m_ += ["};", ""]
open(os.path.join(OUT, "canmove_data.c"), "w").write("\n".join(m_))

open(os.path.join(OUT, "elements.h"), "w").write("\n".join(h))
open(os.path.join(OUT, "elements_data.c"), "w").write("\n".join(c))
print("gen_elements.py: %d elements, %d functions to port" % (len(elements), len(used_funcs)))
