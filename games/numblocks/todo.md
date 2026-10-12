# NumBlocks — todo

Work list for `games/numblocks`. Each item: what it is, where the code lives,
what "done" means. Order matters only where noted.

- [ ] **Make skeletons look at you when they are shooting at you**
- [ ] **Fix biome: unkown glitch**
- [ ] **Fix generation/world bugs** — sand and gravel hang in the air after
      generation until a block update makes them fall; water and lava sit still
      until a block update makes them flow; and any other terrain/generation
      issues found along the way.
- [ ] **[15] Nether mobs** — the nether has no life of its own: add the piglin
      and the zombified piglin as its mobs (spawning, models, skins, drops,
      spawn eggs, `/summon`); the enderman is the stretch past them.
- [ ] **[16] Deepslate** — extend the world down to -64: the bedrock floor
      moves to -64, deepslate runs from y=0 down to it, stone and everything
      above stay; the cache, the generator, the edits and every height guard
      move with it.
- [ ] **[17] Concrete** — the concrete pair, all sixteen colours: concrete
      powder that falls and turns solid the moment water touches it, and the
      concrete it becomes.
- [ ] **[18] Modern water and lava textures** — swap the modern (1.17+) water
      and lava files in from `minecraft-textures/`; the lava as it is now looks
      horrible.

---

## Fix generation/world bugs

**What.** Three related problems, all stemming from the same root cause: the
world generator places blocks but never triggers the reactions that should
follow.

1. **Sand and gravel hang in the air.** After a chunk is generated, sand and
   gravel blocks float unsupported until something else changes a nearby block
   (a block break, a placement, etc.). Only then does `neighbours_changed()`
   notice and make them fall. This is most visible on beaches, in caves, and
   under overhangs.

2. **Water and lava do not flow.** Liquids placed during generation (in
   caves, ravines, lakes, springs) sit frozen until a block update triggers
   `fluid_schedule()` via `neighbours_changed()`. They should start flowing
   on their own once the chunk is generated.

3. **Other terrain/generation issues.** Any additional bugs found while
   investigating the above — e.g. incorrect block placement during
   generation, missing decorators, or lighting issues.

**Where.**

- `src/player.c:293-309` — `neighbours_changed()` is the function that
  triggers both sand falling and liquid scheduling. It checks the 6
  neighbours of a changed block: if a liquid neighbour exists, it calls
  `fluid_schedule()`; if a sand/gravel neighbour is unsupported, it drops it.

- `src/tick.c:24-32` — `fluid_schedule()` adds a liquid block to the
  `pend[]` array (max 160 entries). The liquid updates on its scheduled tick
  via `fluid_update()`.

- `src/player.c:300-307` — sand/gravel falling logic inside
  `neighbours_changed()`: checks if the block below is `replaceable()`, then
  drops the sand straight down to the lowest available spot.

- `src/world.c:153-323` — chunk generation via `fill_chunk()`. This is where
  terrain is placed. After generation, no `neighbours_changed()` call is
  made, so sand and liquids never get their initial update.

- `src/gen.c` — the generator itself. Springs are placed at line 5056
  (`spring()`), which places water/lava source blocks and flowing blocks
  but does not schedule them.

**Notes / the work.**

- **Sand falling.** The fix is to call `neighbours_changed()` (or a lighter
  version) on sand/gravel blocks after they are placed during generation.
  However, doing this for every sand block in a chunk would be expensive.
  Options:
  - After generating a chunk, scan it once for unsupported sand/gravel and
    drop them in a single pass. This is O(n) per chunk but only runs once
    at generation time.
  - Alternatively, defer: when a chunk is first loaded/rendered, run a
    single pass to fix any floating sand. This spreads the cost across
    frames.

- **Liquid flowing.** Same approach: after generating a chunk, call
  `fluid_schedule()` on every liquid block (or at least every source block
  and every block with air below it). The `pend[]` array already caps at
  160, so excess liquids will simply be skipped until their neighbours
  change — acceptable.

- **Keeping RAM/CPU low.** The key insight from the user's note is that
  caves should get their block update "when the player renders it in."
  This suggests a lazy approach: rather than fixing everything at
  generation time, mark chunks as "needs update" and process them when
  they enter the render distance. This avoids the cost of scanning every
  chunk at generation time and only pays for chunks the player actually
  sees.

  Implementation idea: when a chunk is generated, set a flag. When the
  chunk is first rendered (in `fill_chunk()` or the render loop), run a
  single pass: for each sand/gravel with air below, drop it; for each
  liquid with air below or beside it, call `fluid_schedule()`. Clear the
  flag. This is O(n) but only once per chunk, and only for visible chunks.

- **Other issues.** While investigating, check:
  - Are there blocks that should be decorators (flowers, grass, trees)
    that are not being placed?
  - Are springs creating infinite water sources correctly?
  - Do cave liquids interact correctly with each other (lava + water =
    obsidian/cobblestone)?
  - Are there any blocks that should fall but are not sand/gravel?

**Done when.**

- Sand and gravel do not float after generation — they either fall during
  generation or when the chunk is first rendered, without a visible pause.
- Water and lava flow naturally after generation without waiting for a
  separate block update.
- The fix does not cause noticeable frame drops (amortized or lazy).
- `make test` passes, `make build && make check` still fits in budget.
- `./build/play --frames …` frames show sand fallen and liquids flowing
  in newly generated chunks without player interaction.

---

## [15] Nether mobs

**What.** The nether generates, renders and is walked in, but it is empty: the
spawn pass is gated off there (`entity.c:393`) and every mob is one of the
eight overworld ones. Give it its own: **piglins and zombified piglins** are
the item, **the enderman is the stretch** (the "possibly" in the request) and
can be cut without losing the point.

- **Spawning.** In the nether only, on solid ground of the wastes, around the
  player, in small groups, with the mob cap (`count()`) in charge. No sky light
  exists there, so the overworld's `light <= rnd(8)` gate means nothing: use
  the nether's own rule (vanilla nether mobs ignore light level) and write it
  down.
- **Behaviour.** The zombified piglin is the neutral one: it wanders, and when
  hit it fights back and its neighbours join (the 1.8 zombie pigman's one good
  trick). The piglin is hostile: the zombie's chase and bite. Bartering, the
  golden-armour truce and any piglin "anger" beyond that are out of scope
  unless asked — say in the commit what was left out.
- **Enderman (if in).** A tall, thin biped, neutral until hit. Its teleport is
  what makes it an enderman: a blink when hit and when it loses sight of the
  player (the 3-block `teleportRandomly` kind). Its drop is an ender pearl, and
  **that item does not exist yet** (`items.h`), so either add it or drop
  nothing but XP — decide and say. (1.8 also spawns endermen in the overworld
  at night; if in at all, decide whether that comes too.)
- **No burning.** The daylight burn is hard-coded to the zombie and skeleton
  (`mob.c:919`); the new ones stay outside it.

**Where.**
- `src/entity.c:390-397` — the every-second spawn pass and its
  `world_dim != DIM_NETHER` gate; the nether gets a branch of its own (or
  `mobs_spawn` learns dimensions).
- `src/mob.c:395-431` — `mobs_spawn()`: the overworld's light/below-block
  rules; the nether's version is a smaller roll (solid ground, a group, the
  cap). `count(true)` caps the hostiles at 8, `count(false)` the passives
  (`mob.c:202-207`).
- `src/mob.c:14-27` — `info[]` (size, speed, health) and `hostile()`.
- `src/mob.c:49-175` — `mob_parts()`, the models. The zombie and skeleton share
  a biped case (`mob.c:57`); the zombified piglin is that body with a snout,
  the piglin its own, the enderman a tall thin one. `MAX_PARTS` is 12
  (`mob.h:18`) — the boxes fit, count them anyway.
- `src/mob.c:217-226` — `mob_summon`, `mob_of_egg`, `egg_mob[8]`, `mob_name[8]`.
- `src/mob.c:437-` — `loot()`: zombified piglin rotten flesh and gold nuggets
  (`I_GOLD_NUGGET` is there), the piglin's own, the enderman's pearl only if
  that item is added.
- `src/mob.c:889-` — `mob_tick`, the AI tail. The chase is the hostile branch
  (`mob.c:969`), the melee bite is written for the zombie and spider only
  (`mob.c:1116`), and neutral-on-hit is `hurt()` / `panic`; the new mobs have
  to fit each.
- `src/nb.h:226` — the `E_*` enum. The mobs are contiguous
  (`E_ZOMBIE`..`E_CHICKEN`) and seventeen range checks shortcut on that
  (`grep -n 'E_ZOMBIE && .*E_CHICKEN' src/*.c src/*.h`: mob.c, entity.c,
  render.c, player.c, tick.c, command.c). Appending keeps old spawner records
  readable (`mob.c:388-389`), but every check must be audited — or a single
  `is_mob(t)` helper replaces them; the item does the audit.
- `src/nb.h:272`, `src/command.c:490,717` — `mob_name[8]`, `/summon` and its
  name completion.
- `src/items.h:139-148` — the eight `I_*_SPAWN_EGG` ids, then `I_QUARTZ`,
  `I_END`. New eggs must go last (item ids are what saves keep), so the eggs
  stop being contiguous: `mob_of_egg`'s `id - I_ZOMBIE_SPAWN_EGG` arithmetic
  (`mob.c:220-222`) needs a real lookup (today it would run to `egg_mob[8]` for
  `I_QUARTZ`; only quartz never being asked as an egg hides it), and the
  spawner's egg is the same arithmetic the other way round — `d->egg = set -
  E_ZOMBIE` (`render.c:2119`), `I_ZOMBIE_SPAWN_EGG + sd->egg`
  (`render.c:2139`) — which no longer points at the right item once the types
  grow.
- `tools/items.py:145-149` — the `MOBS` tuple (label, egg picture, the numeric
  id) and the `TABS` (a `spawn_egg` kind goes to the Spawn Eggs tab,
  `items.py:431-472`); `tools/pack.py:979` — the `SKINS` list
  (`entity/piglin/piglin`, `entity/zombified_piglin/zombified_piglin`,
  `entity/enderman/enderman`), 64 x 32 folded from the modern sheets as the
  others are (`pack.py:638-658`); `src/data.h:336-338` — `skin_px`/`skin_pal`/
  `SKIN_*` (each new skin is 1 KB — 64 x 32 at 4 bits — plus a 32 B palette of
  flash); `tools/names.py` — the `/give` names.
- `src/mob.c:278` — `dungeon_mob[4]` (what dungeons make);
  `src/mob.c:325-358` — `spawners_spawn()` (the nether skips spawners today,
  `entity.c:395`; decide whether that stays).
- Tests that enumerate the mobs: `tests/unit.c:1097-1104` (the eggs),
  `tests/play.c:256-267` (`--mobs` spawns `E_ZOMBIE + k`, k < 8),
  `tests/play.c:335` (the spawner's kit).

**Notes / the work.**
- Costs: each new skin is ~1 KB of flash, on top of its model case and AI; the
  entity pool (`N_ENT`) is fixed. `make build && make check` (`.nwa` + BSS
  <= 153676 B) must stay inside budget.
- Pick the spawn group, the nether's cap and the spawn column: the wastes run
  down to the lava sea, so a spawn must not drop them in lava (or they have to
  swim — the existing `wet` handling decides); write down which.
- The `--nether` harness already lands the player at y=40 by the sea
  (`tests/play.c:129-155`), a natural place to watch the spawns; it may want a
  way to set the difficulty and put the mobs in front of the camera, the way
  `--mobs`/`--animals` do for the overworld.
- The piglins are 1.16 mobs on a 1.8-shaped game: their skins come from
  `minecraft-textures/` (where every other mob's does), on this game's own box
  models.
- One 1.5.x entry in `versions.md`.

**Done when.** Standing in the wastes, zombified piglins and piglins spawn
around the player and behave: wander and look about; the piglin gives chase in
survival; hitting a zombified piglin turns it and its neighbours on the player;
each has a working spawn egg (in the Spawn Eggs tab, and set into a spawner)
and `/summon piglin` works and completes; their drops land; the enderman (if
in) teleports as it should; no overworld-only rule (daylight burn, despawn)
misfires on them;
`make test` passes; `make build && make check` fits; and scripted
`./build/play --nether --frames … --shots …` frames show them — **not**
`build/desktop`, which is not the test rig.

---

## [16] Deepslate

**What.** The world is y 0..127 and its floor is bedrock at y=0. Move the floor
down and put the modern deep layer under the surface world: the world becomes
**y -64..127** (192 tall), **bedrock at -64**, **deepslate from y=0 down**,
stone and everything above it unchanged. A clean boundary at 0 (no 0..8 mix) is
the spec — "deepslate starts at 0". The top stays 127; extending upward too
(modern's 320) is not in the request.

What moves with it:
- **The generator.** Its columns and density grid span 0..127
  (`chunk_terrain`'s `col`, gen.c:2361-2449; the 33-value grid, gen.c:1711);
  the bedrock floor — `build_default`'s random draws at `y <= 4`
  (gen.c:2104-2114) and the `y == 0` bedrock in the mesa and summary paths
  (gen.c:2204, 2541-2552, 3282) — moves to -64, and stone below 0 becomes
  deepslate. Ores replace stone only (`gen.c:3845`) over absolute ranges
  (gen.c:3852-3866): below 0 there is no rule yet — decide (plain ores,
  deepslate ores, or none) and say what was chosen.
- **The world and the cache.** `WORLD_H 128` becomes a floor and a height
  (`WORLD_MIN_Y -64`, height 192), and every `y < 0` / `y >= WORLD_H` guard
  turns floor-relative: the follow clamp (`world.c:471-472`), the `world_set`
  bound (`world.c:508`), the loading unstick loop (`main.c:56`), the commands'
  checks (`command.c:482,522`), the portal's top checks
  (`portal.c:55,159,187,192`), the player's (`player.c:491,737,768,775,927`),
  the ticks' (`tick.c:63,333,535-536,570` — `below()`'s `y > 0` is the old
  bedrock floor), and the rain sheets (`render.c:1756`). Watch the flight
  ceiling in particular: `player.c:927` is `WORLD_H + 24` = 151 today and must
  not silently become 216. `VOID_Y` (`nb.h:104`) becomes -128 — "64 below the
  bottom of the world" — in `player.c:892`, `entity.c:388`, `mob.c:1132`. The
  cache window stays 24 tall (`VCY`): only `vc_y0` now reaches -64, and the sky
  light's "below the world reads 15" rule keys on `vc_y0 == 0`
  (`world.c:116`) — it must key on the new floor.
- **The edits.** A record entry packs the y in 7 bits (`edits.c:103-112`;
  `KEY` is the low 19 bits): -64..127 needs 8, so the key becomes 20 bits and
  the block's 9 sit above it (29 of 32 — fine). Old records get the one-time
  re-encode on load, the way the 9-bit conversion did (nether.md [13]): the
  `read_head` ladder (`save.c:88-119`) is where the record's shape is decided.
- **Flat worlds.** Their layers are absolute (`flat_block`, gen.c:6214-6217).
  Leave the presets at their y=0 (and say so) or move the default down with the
  floor — either way the default flat world must not float 64 blocks above its
  own void.
- **The nether.** It is its own 0..127 column (bedrock floor 0, ceiling
  123..127). Keep it exactly that: below 0 in the nether is air (modern's
  nether has no deepslate) — recommended, and say so in the commit.
- **The block.** `B_DEEPSLATE` (+ `TX_DEEPSLATE`) appended after the last
  state: `src/blocks.h`, `tools/blocks.py`, the pictures from
  `minecraft-textures/` (`deepslate.png`, `deepslate_top.png`, so the top face
  can differ), `tools/items.py` (label, tab), `tools/names.py`/`src/names.c`
  (`/give deepslate`). Modern's drop is cobbled deepslate; with the cobbled
  family out of scope it drops itself (say so). Cobbled deepslate and the
  bricks/polished family are not asked for — leave them out unless wanted.
- **The tests.** The `tests/gen` tools are built around the 0..128 column
  (`gen_slab(cx, cz, 0, 128, …)` in t_slabs, t_smoke, t_sum, genview) and the
  reference comparison (`t_refcmp`, `ref_terrain.h`: 1.8's terrain, whose
  bedrock shares the floor at 0) no longer describes y 0..4 and knows nothing
  below 0. Keep it honest — compare from y=5 up with deepslate mapped to stone,
  or patch the reference — and say which in the tool's output.

**Notes / the work.**
- **Cost.** A chunk's column is 1.5x the rows and the density and sign grids
  grow to reach -64 (33 values a chunk column become 41: ~+1 KB of the
  generator's statics). Measure with `tests/gen/arm/bench.py` before and after
  and quote the numbers. The fill is one chunk a frame, so if the growth hurts,
  the deep layer can be made cheaper (all-solid deepslate is not what the
  octaves are for) — but measure, do not assume.
- **RAM / flash.** One more state (~35 B of `.rodata` rows plus the pictures)
  and the grown grid. The standing rule is no BSS growth; this item grows the
  generator's statics, so it is the documented exception (numbers in the
  commit), as nether [7] was.
- **Old worlds.** Terrain is regenerated, not stored, so a world opened in the
  new build gets the new floor — the old bedrock at y=0 is gone from
  ungenerated ground — while the player's edits survive the record re-encode.
  Say it in the version entry.
- **`--void`** (`tests/play.c:284-294`) digs through the bedrock under the
  player: with the floor at -64 the shaft and the hang are 64 blocks longer —
  extend it, and it doubles as the deepslate view.

**Done when.** A dug shaft shows stone to y=0, deepslate below it and bedrock
at -64; the cache follows the player down and back up; an edit placed at -40
survives a save and reload; worlds from before load and re-save losslessly;
`make test` passes; `make build && make check` fits (numbers in the commit);
and `./build/play --frames … --shots …` frames (scripted keys, **not**
`build/desktop`) show the boundary, the deep and the floor.

---

## [17] Concrete

**What.** The modern concrete pair, in all sixteen colours: **concrete powder**,
which falls like sand and gravel, and **concrete**, which is what a powder
block becomes the moment water touches it (a source or a flow). Powder without
water stays powder; the water does the setting; no other mechanic.

**Where.**
- `src/blocks.h` / `tools/blocks.py` — 32 new states (16 powder + 16 concrete),
  appended after the last state; `tools/blocks.py` carries names, models,
  hardness, tool and texture (powder: like sand and gravel — shovel, drops
  itself; concrete: like stone — pickaxe). The quartz family (1.5.3) is the
  recent example of appending a family; the wool and stained clay loops
  (`blocks.py:103`) are the pattern for sixteen of a kind.
- `tools/pack.py` + `minecraft-textures/` — the pictures are already in the
  modern dump (`textures/block/<colour>_concrete.png` and
  `<colour>_concrete_powder.png`, all 32 there).
- `tools/items.py` — the item names, labels, icons, the creative tabs and the
  recipe: modern's concrete powder is shapeless (4 sand + 4 gravel + 1 dye →
  8 powder; `shapeless()` and `DYES` are there, `items.py:326-330` is the
  dyed-family pattern). Concrete is not crafted.
- `tools/names.py` — the `/give` names.
- **The fall**: `src/player.c:303-310`, `neighbours_changed()`'s sand and
  gravel drop (straight down, at once) — the powder joins that list.
- **The water contact**: the same `neighbours_changed()`, which runs on every
  set and break and on every liquid move (`tick.c:65-68`'s `set()`), so the
  powder finds a water neighbour there and becomes its concrete; that covers
  water placed by the player, water flowing in (`flow_into`, `tick.c:84`) and
  powder falling into or beside water. The floor guard (`tick.c:63`) is the
  world's floor, which [16]'s world may be moving.

**Notes / the work.**
- 32 states of the ~110 free (B_COUNT 402 of 512) and 32 pictures (128 B + a
  palette each of flash): `make build && make check` (`.nwa` + BSS
  <= 153676 B) must still fit.
- Keep the powder's floor guard consistent with the sand one, so it never
  falls out of the world (and so it agrees with [16] if deepslate lands first).
- `versions.md` gets its next entry.

**Done when.** All sixteen colours: the powder places, falls when the block
under it goes, and sets into its concrete when water reaches it (placed *or*
flowing); concrete sits still; the powder crafts from sand, gravel and dye; the
pieces show in the creative tabs and by `/give`; `make test` passes;
`make build && make check` fits; and `./build/play` frames (scripted keys,
**not** `build/desktop`) show a powder column falling and a water stream
setting one.

---

## [18] Modern water and lava textures

**What.** Swap the water and lava pictures for the modern ones in
`minecraft-textures/`, because the lava texture as it is now looks horrible.
The four files: `textures/block/water_still.png`, `water_flow.png`,
`lava_still.png`, `lava_flow.png`.

**Where.** The pictures live in `src/data.c` (`tex_px`/`tex_pal`; the
`TX_WATER_STILL`, `TX_WATER_FLOW`, `TX_LAVA_STILL`, `TX_LAVA_FLOW` rows in
`data.h` name them). `tools/pack.py --modern` (`make data-modern`) rewrites the
whole file from the dump; `tools/data_add.py` patches pieces into it. Water is
grey and takes `biome_water`'s tint (`pack.py:270-318`); lava is untinted.
Swap the four and nothing else.

**Done when.** `./build/play` frames show the modern water and the modern lava
(a pool, and the nether's lava sea); `make test` passes; `make build &&
make check` fits.

---

## Checking any of it

```bash
cd games/numblocks
make data-modern                     # regenerate textures/blocks from the modern client
make test                            # unit tests
make host && ./build/play --frames 120 --mobs --torches --rain --shots 30,60,90,119 --out /tmp/new
make desktop                         # build/desktop (interactive, saves in build/desktop-saves)
python3 tests/render/arm/bench_render.py --fancy --rain --mobs /tmp/after
make build && make check             # device .nwa + BSS <= 153676 B
make -C ../numblocks-interest build check
```

Test with `./build/play` (scriptable keys and shots), not `build/desktop`.

Rules that apply to every item: no new `-Wall -Wextra` warnings, no BSS growth,
keep `git status` clean, and ship both the calculator `.nwa` and the desktop
binary.
