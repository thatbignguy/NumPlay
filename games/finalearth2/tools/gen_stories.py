#!/usr/bin/env python3
"""Makes src/stories_data.c, the scenarios and free play maps, from the
original's stories.json, stories/*.json and lang_en.csv (their texts).

    python3 -I tools/gen_stories.py [REFERENCE_DIR]
"""
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
GAME = os.path.dirname(HERE)
REF = sys.argv[1] if len(sys.argv) > 1 else os.environ.get("FE2_REF", os.path.expanduser("~/Documents/Assets/gamesrc/finalearth2/web14"))

STORIES = ["theLostShip", "multipleWorlds", "hackersandaliens", "cityofthekey", "FreePlayMegaWorld",
           "FreePlayManyWorldsRandom", "FreePlayEmptyCanvas", "FreePlayDivided", "FreePlayRandomMiniWorlds",
           "FreePlayManyWorlds"]
MATS = ["food", "wood", "stone", "machineParts", "refinedMetal", "computerChips", "cacao", "chocolate", "graphene",
        "rocketFuel", "knowledge"]
M_ENUM = ["M_FOOD", "M_WOOD", "M_STONE", "M_MACHINEPARTS", "M_REFINEDMETAL", "M_COMPUTERCHIPS", "M_CACAO",
          "M_CHOCOLATE", "M_GRAPHENE", "M_ROCKETFUEL", "M_KNOWLEDGE"]
SUBGOALS = ["BuildPermanents", "GatherMaterials", "ProductionGoal", "PerformBuildingUpgrades", "PerformCityUpgrade",
            "MinimumHappiness", "MinimumCitizens", "HousingCapacity", "Wait", "InvitePeople", "ExploreAlienRuins",
            "ExploredWithResearchSpacecraft", "CompleteHackerMissions", "WaitForDecryption", "GloryGoal"]
FLAGS = ["disableDying", "disableDestroy", "hiddenBoost", "setBuildableAliens"]
APPEARANCES = {None: 0, "": 0, "key": 1, "key2": 2}

lang = {}
for line in open(os.path.join(REF, "lang_en.csv"), encoding="utf-8"):
    if "|" in line:
        k, v = line.rstrip("\n").split("|", 1)
        lang[k] = v.replace("\\n", "\n")
classes = re.findall(r'CLS_(\w+) = ', open(os.path.join(GAME, "src", "classes.h")).read())


def cstr(s):
    if s is None:
        return "NULL"
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n") + '"'


def mats(d):
    if not d:
        return "{0}"
    return "{" + ", ".join(f"[{M_ENUM[MATS.index(k)]}] = {v}" for k, v in d.items() if k in MATS) + "}"


def cls(name):
    name = name.split(".")[-1]
    if name not in classes:
        raise SystemExit(f"unknown class {name}")
    return f"CLS_{name}"


out = ['/* The Final Earth 2: the scenarios and free play maps, made by tools/gen_stories.py from the',
       " * original's stories and texts. Don't edit: edit the generator. */",
       '#include "story.h"', '#include "classes.h"', '#include "data.h"', ""]
table = []
for si, name in enumerate(STORIES):
    d = json.load(open(os.path.join(REF, "stories", name + ".json")))
    p = f"s{si}"
    free = bool(d.get("isFreePlay"))
    tkey = "FreePlay" if free else name + ".json"
    # worlds
    wrows = []
    for wi, w in enumerate(d.get("worlds", [])):
        q = f"{p}w{wi}"
        res = w.get("worldResources", [])
        if res:
            out.append(f"static const StoryRes {q}r[] = {{" + ", ".join(f"{{{r['position']}, {cls(r['className'])}}}" for r in res) + "};")
        stacks = w.get("buildingStacks", [])
        for k, s in enumerate(stacks):
            items = []
            for c in s["classNames"]:
                if c == "NULL":
                    items.append("STACK_NULL")
                    continue
                m = re.match(r'^(\w+)\*(true|false)\.(\d+)$', c)
                if m:
                    items.append(f"{cls(m.group(1))} | STACK_DECOR({int(m.group(3))})" + (" | STACK_MIRROR" if m.group(2) == "true" else ""))
                elif c.endswith("_mirrored"):
                    items.append(f"{cls(c[:-9])} | STACK_MIRROR")
                else:
                    items.append(cls(c))
            out.append(f"static const uint16_t {q}s{k}[] = {{{', '.join(items)}}};")
        if stacks:
            out.append(f"static const StoryStack {q}s[] = {{" + ", ".join(
                f"{{{s['position']}, {len(s['classNames'])}, {q}s{k}}}" for k, s in enumerate(stacks)) + "};")
        cits = w.get("citizens", [])
        if cits:
            out.append(f"static const StoryCitizens {q}c[] = {{" + ", ".join(
                f"{{{c['amount']}, {c['ageRangeMin']}, {c['ageRangeMax']}, {c.get('minX', -1)}, {c.get('maxX', -1)}}}" for c in cits) + "};")
        decos = w.get("decorations", [])
        if decos:
            out.append(f"static const StoryDeco {q}d[] = {{" + ", ".join(
                f"{{{x['position']}, SPR_{x['spriteName'][4:].upper()}}}" for x in decos) + "};")
        flags = []
        if w.get("unbuildableAliens"):
            flags.append("WF_UNBUILDABLE_ALIENS")
        if w.get("protectedKey"):
            flags.append("WF_PROTECTED_KEY")
        r = w["rect"]
        deco = w.get("decoration")
        wrows.append("{" + ", ".join([str(r["x"]), str(r["y"]), str(r["width"]), str(r["height"]), str(w.get("seed", 0)) + "u",
                                      str(APPEARANCES[w.get("appearance")]), " | ".join(flags) or "0",
                                      f"SPR_{deco[4:].upper()}" if deco else "0",
                                      str(len(res)), f"{q}r" if res else "NULL", str(len(stacks)), f"{q}s" if stacks else "NULL",
                                      str(len(cits)), f"{q}c" if cits else "NULL", str(len(decos)), f"{q}d" if decos else "NULL"]) + "}")
    out.append(f"static const StoryWorld {p}w[] = {{\n  " + ",\n  ".join(wrows) + "};")
    # goals
    grows = []
    for gi, g in enumerate(d["goals"]):
        q = f"{p}g{gi}"
        subs = []
        for s in g.get("subGoals", []):
            t = s["type"]
            if t not in SUBGOALS:
                raise SystemExit(f"{name}: unknown subgoal {t}")
            f = [f".type = SG_{t.upper()}"]
            if t == "BuildPermanents":
                f.append(f".cls = {cls(s['permanentToBuildClass'])}")
                f.append(f".amount = {s['amount']}")
                f.append(f".world = {s.get('onWorld', -1)}")
                f.append(f".notWorld = {s.get('notOnWorld', -1)}")
            elif t in ("GatherMaterials", "ProductionGoal"):
                f.append(f".mats = {mats(s['materials'])}")
            elif t in ("PerformBuildingUpgrades", "PerformCityUpgrade"):
                f.append(f'.name = "{s["upgradeClass"]}"')
                f.append(f".amount = {s.get('amount', 1)}")
            elif t == "MinimumHappiness":
                f.append(f".value = {s['happiness']}")
            elif t == "MinimumCitizens":
                f.append(f".amount = {s['number']}")
            elif t in ("HousingCapacity", "CompleteHackerMissions"):
                f.append(f".amount = {s['amount']}")
            elif t == "Wait":
                f.append(f".value = {s['waitTime']}")
            subs.append("{" + ", ".join(f) + "}")
        if subs:
            out.append(f"static const SubGoal {q}sub[] = {{" + ", ".join(subs) + "};")
        for key in ("unlocks", "researches"):
            if g.get(key):
                out.append(f"static const char *const {q}{key[0]}[] = {{" + ", ".join(cstr(x) for x in g[key]) + "};")
        plans = []
        for e in g.get("planning", []):
            t = e["type"]
            if t == "SpawnCitizensFlyingSaucer":
                plans.append(f"{{.type = EV_SAUCER, .time = {e['time']}, .amount = {e['amount']}, .ageMin = {e['ageRangeMin']}, "
                             f".ageMax = {e['ageRangeMax']}, .world = {e['world']}}}")
            elif t == "ScriptedStoryPart" and e["className"] == "SpawnCitizensRegularly":
                after = e.get("afterDoneMessage")
                if after is not None:
                    after = lang.get(f"{tkey}.FreePlayStart.planning.afterDoneMessage" if free else "", after)
                plans.append(f"{{.type = EV_REGULARLY, .time = {e['time']}, .amount = {e['spawnNumber']}, .ageMin = {e['ageRangeMin']}, "
                             f".ageMax = {e['ageRangeMax']}, .world = {e['world']}, .groupMin = {e['groupSizeMin']}, "
                             f".groupMax = {e['groupSizeMax']}, .betweenMin = {e['timeBetweenMin']}, .betweenMax = {e['timeBetweenMax']}, "
                             f".afterDone = {cstr(after)}}}")
            else:
                raise SystemExit(f"{name}: unknown planning {t}")
        if plans:
            out.append(f"static const StoryEvent {q}e[] = {{" + ", ".join(plans) + "};")
        fl = g.get("flags", {})
        fset = [f"GF_{k.upper()}" for k in FLAGS if k in fl]
        fval = [f"GF_{k.upper()}" for k in FLAGS if fl.get(k)]
        gname = g["name"]
        # (lang_en.csv's keys: "theLostShip.json/Start.text", and "FreePlay.FreePlayStart.text")
        tsep = "." if free else "/"
        text = lang.get(f"{tkey}{tsep}{gname}.text", g.get("text"))
        quick = lang.get(f"{tkey}{tsep}{gname}.quickText", g.get("quickText") or "")
        title = lang.get(f"{tkey}{tsep}{gname}.title", g.get("title"))
        nxt = g.get("nextGoal", "")
        nxt_i = next((i for i, x in enumerate(d["goals"]) if x["name"] == nxt), -1) if nxt else -1
        nstory = STORIES.index(g["nextStory"]) if g.get("nextStory") in STORIES else -1
        grows.append("{" + ", ".join([cstr(gname), str(nxt_i), str(nstory), str(len(subs)), f"{q}sub" if subs else "NULL",
                                      str(len(g.get("unlocks", []))), f"{q}u" if g.get("unlocks") else "NULL",
                                      str(len(g.get("researches", []))), f"{q}r" if g.get("researches") else "NULL",
                                      str(len(plans)), f"{q}e" if plans else "NULL", " | ".join(fset) or "0", " | ".join(fval) or "0",
                                      str(g.get("showTutorial", 0)), cstr(text), cstr(quick), cstr(title)]) + "}")
    out.append(f"static const StoryGoal {p}g[] = {{\n  " + ",\n  ".join(grows) + "};")
    gens = []
    for gi, gs in enumerate(d.get("generatorScripts", [])):
        a = gs["args"]
        items = [(x["className"], x["numberMin"], x["numberMax"], 1) for x in a.get("initialBuildings", [])] + \
                [(x["className"], x["numberMin"], x["numberMax"], 0) for x in a.get("initialWorldResources", [])]
        out.append(f"static const GenItem {p}x{gi}[] = {{" + ", ".join(f"{{{cls(c)}, {lo}, {hi}, {b}}}" for c, lo, hi, b in items) + "};")
        gens.append("{" + ", ".join(str(a[k]) for k in ("numberOfWorldsMin", "numberOfWorldsMax", "minX", "minY", "maxX", "maxY",
                                                          "minWidth", "maxWidth", "minHeight", "maxHeight", "heightVariation")) +
                    f", {len(items)}, {p}x{gi}, " + ", ".join("true" if a.get(k) else "false" for k in (
                        "spreadBuildingsEvenlyBetweenWorlds", "spreadWorldResourcesEvenlyBetweenWorlds",
                        "rememberWorldSpreadBetweenBuildingsAndWorldResources", "avoidGeneratingOverfullWorlds")) + "}")
    if gens:
        out.append(f"static const WorldGenerator {p}x[] = {{" + ", ".join(gens) + "};")
    bgs = d.get("backgroundSprites", [])
    if bgs:
        out.append(f"static const StoryBackground {p}b[] = {{" + ", ".join(
            f"{{SPR_{b['sprite'][4:].upper()}, {b['width']}, {b['image']}, {b['x']}, {b['y']}, {float(b['velocity'])!r}f}}" for b in bgs) + "};")
    start = next(i for i, x in enumerate(d["goals"]) if x["name"] == d["startGoal"])
    sname = lang.get(f"stories.json/{name}.name", name)
    sdesc = lang.get(f"stories.json/{name}.description", "")
    flags = [f for f, k in (("ST_FREEPLAY", "isFreePlay"), ("ST_LOCK_ALL", "lockAllPermanents"), ("ST_INVITE", "useInviteCitizens"),
                            ("ST_SPEED_UP_NIGHTS", "speedUpStartNights"), ("ST_NO_EXTRA_RESOURCES", "extraResourcesDisabled")) if d.get(k)]
    locks = d.get("lockPermanents", [])
    if locks:
        out.append(f"static const char *const {p}l[] = {{" + ", ".join(cstr(x) for x in locks) + "};")
    table.append("{" + ", ".join([cstr(name), cstr(sname), cstr(sdesc), " | ".join(flags) or "0", mats(d.get("initialMaterials")),
                                  str(len(d.get("worlds", []))), f"{p}w", str(len(d["goals"])), f"{p}g", str(start),
                                  str(len(gens)), f"{p}x" if gens else "NULL", str(len(locks)), f"{p}l" if locks else "NULL",
                                  str(d.get("viewYFromBottom", 0)), str(d.get("viewStartX", d.get("mobileViewStartX", 0))),
                                  str(len(bgs)), f"{p}b" if bgs else "NULL"]) + "}")
    out.append("")
out.append("const Story stories[] = {\n  " + ",\n  ".join(table) + "};")
out.append(f"const int nstories = {len(STORIES)};")
open(os.path.join(GAME, "src", "stories_data.c"), "w").write("\n".join(out) + "\n")
print(f"{len(STORIES)} stories")
