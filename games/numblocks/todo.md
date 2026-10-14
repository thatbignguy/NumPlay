# NumBlocks — todo

Work list for `games/numblocks`. Each item: what it is, where the code lives,
what "done" means. Order matters only where noted.

- [x] **Make skeletons look at you when they are shooting at you**
- [x] **Fix biome: unknown glitch**
- [x] **[15] Nether mobs** — the nether has no life of its own: add the piglin
      and the zombified piglin as its mobs (spawning, models, skins, drops,
      spawn eggs, `/summon`); the enderman is the stretch past them.
- [ ] **[16] Deepslate** — extend the world down to -64: the bedrock floor
      moves to -64, deepslate runs from y=0 down to it, stone and everything
      above stay; the cache, the generator, the edits and every height guard
      move with it.
- [x] **[22] Nether biomes** — the nether is one biome: give it the modern
      three — Crimson Forest, Warped Forest and Soul Sand Valley — and not
      Basalt Deltas; each with its own surface, its own blocks (the crimson
      and warped nyliums, stems, planks, roots and vines, soul soil and soul
      fire, bone blocks) and its own fog.
- [x] **[23] Basic structures** — the world makes terrain, dungeons and lakes
      and no structures: a placement pass, the desert pyramid first, working
      and looking exactly as modern Minecraft makes it (`26.3.jar`).
- [ ] **[24] Under-lava fog and overlay** — the eye in lava reads like the eye
      beside it: give it Java's dense orange fog and the orange screen wash,
      as `under_water` already gives water its blue (`render.c`).
- [x] **[25] Water evaporates in the nether** — a water bucket emptied in the
      nether places nothing: the fizz and a puff of smoke, and the bucket
      comes back empty, as Java's ultra-warm dimension does.
- [x] **[26] Liquids at the cache's edge** — a stream whose source leaves the
      loaded cache drains to nothing: an unloaded neighbour must read unknown,
      not air, so the liquid stands until the world that feeds it is whole.
- [x] **[27] Deepslate caves** — the deep layer is solid rock: carve it with
      caves of 1.17-1.19's kind (bigger and more open than the 1.8 tunnels), so
      there is somewhere to walk, to fall into and to light under y=0.

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

## [22] Nether biomes

**What.** The nether is one biome — Nether Wastes, `BI_HELL` (1.8's id 8,
`gen.c:539`), the constant `gen_biome` answers for every column
(`gen.c:6307`) — and the request is the three modern biomes beside it:
**Crimson Forest**, **Warped Forest** and **Soul Sand Valley**, and **no
Basalt Deltas**. Nether Wastes stays, the ground the other three sit in.
Each is its own surface, its own growth, its own blocks and its own fog:

- **Crimson Forest.** Crimson nylium where the netherrack meets air, huge
  crimson fungi to walk under (a stem with wart blocks and shroomlight about
  it), crimson fungi and crimson roots on the ground, weeping vines hanging
  from the ceiling. Fog `#330303` (the client's own).
- **Warped Forest.** Warped nylium, huge warped fungi, warped fungi, warped
  roots and nether sprouts, twisting vines growing up. Fog `#1a051a`.
- **Soul Sand Valley.** Netherrack broken by soul sand and **soul soil**,
  **soul fire** wherever fire lights on either, **bone blocks** in the
  ground (modern's fossils: whole skeletons where cheap, bunches of bone
  blocks where not), crimson roots about. Fog `#1b4745`. **Basalt pillars**
  are the valley's silhouette in modern; they need basalt and polished
  basalt and the request only rules the *biome* out — decide and say in the
  commit.

**Where.**
- **The split.** Modern chooses its nether biomes with its multi-noise source
  (temperature, humidity and the rest of its climate parameters) — machinery
  this generator does not have. A reading of the patch noise the soul
  sand/gravel patches already use (`grass_noise_div(X, Z, 24)`,
  `gen.c:5577`) or a new low-frequency field splits a column into the four.
  One rule, read by both the terrain and `gen_biome` (`gen.c:6307`), so the
  biome line and the ground agree, and the same biome comes out whichever
  slab window is asked (the glowstone lesson of 1.5.4).
- **The surface and the growth.** `nether_slab` (`gen.c:5492`) leaves every
  column netherrack; each biome gives the column's exposed top faces its own
  surface (the patch code at `gen.c:5574-5584` is the pattern — it reaches
  only the rows about the lava sea today; the biomes cover the whole
  column), and its fungi, roots, vines and shroomlight arrive as per-chunk
  features beside the quartz and the glowstone (`gen.c:5606-5613`), drawn
  from the chunk's own `JRand` so a chunk's growth is the same whichever
  window is asked.
- **The blocks.** Appended past the concrete states the way [17] did
  (`B_CONCRETE_BLACK` is 433 now, `B_COUNT` 434): crimson and warped
  **nylium** (top and side textures), crimson and warped **fungus** and
  **roots**, **nether sprouts** (cross plants, `M_CROSS`), **weeping** and
  **twisting vines** (the vine model is there), **crimson** and **warped
  stems** with their axis states (the logs' `_X`/`_Z`/bark pattern,
  `tools/blocks.py:78-84`) and the stripped pair, **crimson** and **warped
  planks**, the two **wart blocks** (1.8's nether wart block never made it
  into the nether set), **shroomlight** (light 15), **soul soil**, **soul
  fire** and the **bone block** (with its axis states). The request's own
  list is the floor, not the ceiling: trim what does not earn its flash and
  say what was left out. The pictures are all in the modern dump
  (`minecraft-textures/textures/block/`: `crimson_nylium`, `warped_stem`,
  `soul_soil`, `bone_block_side`, `shroomlight`, the vines ...), packed by
  `tools/pack.py`; labels, tabs and recipes in `tools/items.py` (planks from
  stems, the modern four a stem), the `/give` names in `tools/names.py`.
- **Soul fire.** Fire is one block everywhere (`fire_can_stay`, `tick.c:246`;
  `fire_update`, `tick.c:277`): fire lit on soul soil or soul sand becomes
  its own blue state now — the pictures are `soul_fire_0/1.png` — which
  burns out as fire does, where ordinary fire does not stay on the soul
  blocks, and soul fire stays nowhere else.
- **The fog and the sky.** The nether's red is two constants
  (`NETHER_R/G/B`, `render.c:203-205`) read by the sky's early-out
  (`render.c:209`) and the per-frame block (`render.c:2378-2395`); they
  become the colour of the biome under the camera (the wastes keep
  `#330808`), which is how modern's fog follows the camera.
- **The F3 line.** `biome_name` (`gui.c:205`) gains the three (and Nether
  Wastes for 8); `vbiome` already carries the answer per column
  (`world.c:201`, read at `gui.c:318`).

**Notes / the work.**
- Biome ids: 1.16's own 49, 50 and 51 (8 stays Nether Wastes);
  `biome_temp`/`biome_rain` (`data.c:2365-2366`, from `tools/pack.py`'s
  `BIOMES`, `pack.py:284`) take their rows; the nether has no weather, rain
  or snow, so the values are nominal — write down what they are.
- **Mobs.** The nether's piglins and zombified piglins are [15]; what each
  biome spawns (endermen in the warped forest, skeletons and ghasts in the
  valley) is [15]'s pass reading `vbiome` — note in the commit what it reads.
- **Cost.** Each new state is ~35 B of `.rodata` rows plus its picture
  (128 B and a palette each); `make build && make check` (`.nwa` + BSS
  <= 153676 B) must still fit. A new noise field grows the generator's
  statics: measure (`tests/gen/arm/bench.py`) and quote it if one is added.
- One `versions.md` entry.

**Done when.** The wastes show the forests and the valley with their own
surfaces, growth and fog; the same seed gives the same biomes and the same
growth whichever way the world is walked; soul fire burns blue on soul soil
and soul sand and nowhere else; every new block places, breaks, drops, shows
in the creative tabs and by `/give`; `make test` passes; `make build &&
make check` fits; and `./build/play --nether --frames ... --shots ...`
(scripted keys, **not** `build/desktop`) shows each biome's surface and fog.

---

## [23] Basic structures: the desert pyramid

**What.** The generator makes terrain, caves, lakes and dungeons, and no
structures at all (`gen.c:78-79`, the same TODO inside `populate`,
`gen.c:5351`). Add the structure pass and the first structure through it, the
**desert pyramid**, working and looking exactly as modern Minecraft makes it:
the same 21 x 21 step pyramid of sandstone, cut sandstone and chiselled
sandstone, the same orange and blue terracotta, the same hidden chamber with
its four chests, and the same stone pressure plate over nine TNT. The client
for 26.3 the owner placed in the folder (`26.3.jar`, the repo root) is the
reference for all of it.

**Where.**
- **The reference, in `26.3.jar`** (a modern client, names and all):
  - `net/minecraft/world/level/levelgen/structure/structures/DesertPyramidPiece.class`
    and `DesertPyramidStructure.class`: the piece's whole build — one piece,
    no NBT asset — a decompiler (or the bytecode and its constant pool)
    gives the block-by-block code and the palette it names: `SANDSTONE`,
    `CUT_SANDSTONE`, `CHISELED_SANDSTONE`, `SANDSTONE_SLAB`,
    `SANDSTONE_STAIRS`, the dyed terracotta, `TNT`, `STONE_PRESSURE_PLATE`
    and `createChest`. Read the build out of it, not out of memory: the
    modern pyramid is what is owed.
  - `data/minecraft/worldgen/structure/desert_pyramid.json` and
    `data/minecraft/worldgen/structure_set/desert_pyramids.json`: the
    placement — a random spread, **spacing 32** chunks, **separation 8**,
    **salt 14357617**, at the surface step — and
    `tags/worldgen/biome/has_structure/desert_pyramid.json`, which says
    deserts only.
  - `data/minecraft/loot_table/chests/desert_pyramid.json`: the chests'
    items (the main pool's 2-4 rolls and the bone pool's four).
- **The pass.** Ride the population pass the dungeons already ride
  (`populate`, `gen.c:5329`; `dungeon`, `gen.c:3709`): the pyramid's grid
  cell is computed from the population's chunk and the modern salt (the
  `RandomSpreadStructurePlacement` class sits in the jar beside the piece),
  so a replayed population makes the same decision and the memo
  (`pmemo_begin`, the `dungeons[]` bits, `gen.c:3540-3548`) keeps its
  outcome — the same temple whichever direction the world is walked, as the
  dungeons are. Writes go through `put_c`/`cget` (`gen.c:3445-3470`) and land
  in each chunk as it is generated, which is how the dungeons' cross-chunk
  pieces come out whole. The surface y and the biome come from the column
  summary (`ColSum`, `cs_top`/`b00`).
- **The blocks it needs.** `STONE_PRESSURE_PLATE` (1.8's id 70) and
  `SANDSTONE_STAIRS` (128, with their four facings as the oak stairs have
  them, `tools/blocks.py:235-237`) are not in the game; `CUT_SANDSTONE` may
  need its own state (the 24:0-24:2 trio maps to modern sandstone,
  chiselled sandstone and smooth sandstone today, `tools/modern_map.py:36-37`
  — audit the pyramid's palette against it); sandstone slabs, orange and
  blue terracotta, TNT and the chest are in.
- **The trap.** The plate is the trap: stood on, it lights the nine TNT
  under it (the game lights TNT from fire and flint and steel, `tick.c:274`,
  `player.c:820`). The pyramid needs no redstone anywhere; say in the commit
  that the plate is the one place a redstone block behaves.
- **The chests.** A pyramid's four chests open with the modern table's
  items, the table's own rolls and weights. The game places dungeon chests
  empty today — `dungeon_loot` (`gen.c:3697`) only replays the draws of the
  population's stream — and chests are tiles filled by whoever opens them
  (`gui.c:704-751`). Draw the loot where a chest is first opened, from the
  world seed and the chest's own place, so nothing new is stored and the
  same world always gives the same loot; the items the game has no item for
  (the horse armours, the enchanted golden apple, the armour trim template)
  are dropped, and say which. If the dungeons' chests can take their 1.8
  loot through the same path for a little more, take it; else say so.
- **The chamber floor.** Modern plants suspicious sand (archaeology) under
  the floor; there is no brushing here — leave it out and say so.

**Notes / the work.**
- **The reference comparison.** `tests/gen`'s reference comparison
  (`t_refcmp.c`, `ref_terrain.h`) is 1.8's terrain and knows no structures:
  pick a seed and an area no temple falls in, or teach the tool to skip the
  pieces' boxes — say which.
- **Determinism.** A structure spanning chunks must come out whole and the
  same from every direction (the dungeons' memo is the pattern), and it must
  not move what another population draws beyond the draws it makes itself.
- **Tests.** `tests/unit.c` for the placement grid (neighbouring chunks see
  the same temple), the piece's blocks, the trap and the chest loot; a
  `--temple` scenario in `tests/play.c` (or a seed that puts one under the
  player) with the documented `./build/play --frames ... --shots ...` run —
  scripted keys, **not** `build/desktop`.
- **The next ones.** The pass takes the next structure as its own row
  (jungle pyramid, witch hut, igloo ...); nothing of the machinery should
  need rebuilding for them.
- Cost: the piece's build is code, not tables, and the placement is a few
  dozen bytes; `make build && make check` must still fit. One `versions.md`
  entry.

**Done when.** A desert holds the pyramid; its blocks and shape match
`26.3.jar`'s piece block for block; walking to the same place from any
direction meets the same temple; its plate sets off the TNT; its four chests
hold the modern loot when opened; the documented `./build/play --frames ...
--shots ...` frames show it; `make test` passes and `make build && make
check` fits.

---

## [24] Under-lava fog and overlay

**What.** The eye in lava reads almost exactly like the eye beside it: the fog
and the sky keep the dimension's colour, and the only case the renderer knows
is water (`under_water`). Java has its own for lava — `FogRenderer`/
`getFogColor` gives the eye in lava a dense orange fog (colour **0.6, 0.1,
0.0** = `#991900`, near full within a block or two, much shorter than water's)
and `EntityRenderer` washes the whole picture with the lava overlay — so the
screen is unmistakably orange. Add the same here: `under_lava` beside
`under_water`, its own colour and distances, and the orange screen wash.

**Where.**
- `src/render.c:106` — `static int under_water;` gains `under_lava` beside it,
  set at `render.c:2556` (`under_water = is_water(cam_b)`) from `cam_b`
  (`render.c:2551`, the block at the eye): `under_lava = is_lava(cam_b)`
  (`is_lava`'s all-states form, `nb.h:75`, so a flowing cell counts too).
- **The fog.** The nether's is set at `render.c:2417-2433`, the overworld's at
  `render.c:2457-2547`, and water's override sits after both at
  `render.c:2567-2571` (`fog_r/g/b = 10,30,110; fog0 = 0; fog1 = 9`). Lava's
  goes with it: colour `#991900` and Java's reach (`fog0 = 0`, `fog1` about 2),
  applied whichever dimension branch ran.
- **The tinted sky and faces.** Every `under_water` colour wants a lava arm —
  `sky()` (`render.c:214,221`), `sky_px()` (`render.c:316`), the face path
  (`render.c:1316`) and the `plain` test (`render.c:1002`) — but tinted orange
  (Java's overlay), not the water blue.
- **The screen wash.** A translucent orange over the whole picture, so a wall
  of blocks right in front reads as lava and not only the fog does: `gui.c`'s
  `tint_rect` (`gui.c:111`) as the hurt flash uses it (`gui.c:2835-2837`), or
  `portal_overlay` (`gui.c:127`) as the pattern.
- **The frames.** `tests/play.c`'s `--lava` (`play.c:323-365`) puts the camera
  over the sea (nether) or over a fall (overworld), four blocks up; it needs a
  shot with the eyes *in* a lava cell (drop the camera into the source) for the
  recorded run.

**Notes / the work.**
- Java's numbers: `FogRenderer.setupColor`/`getFogColor` lava `(0.6F, 0.1F,
  0.0F)`; the eye's own wash is `EntityRenderer`'s lava overlay (a textured
  orange), which here is a flat `tint_rect`.
- The colour is the same in both dimensions, as Java's is; `NETHER_R/G/B`
  (`render.c:207-209`) are the nether *sky*, not this.
- No state, no RAM, no flash: one flag and a few constants. The camera's own
  cell is already read (`cam_b`), so this is the same hook water uses.
- Eyes in water and lava at once is impossible (one cell); if it mattered, lava
  wins.
- One `versions.md` entry.

**Done when.** Standing with the head in lava, the view is a dense orange haze
with the picture washed orange; standing beside lava with the eyes in air is
unchanged; `make test` passes; `make build && make check` fits; and the
recorded `./build/play --lava --nether --frames … --shots …` (**not**
`build/desktop`) shows both the surface and the inside.

---

## [25] Water evaporates in the nether

**What.** A water bucket used in the nether places water as if it were the
overworld: the block stands and the stream runs. Java's nether is an
**ultra-warm** dimension (`NetherDimension` sets `ultraWarm`), and
`ItemBucket`/`BucketItem` checks `dimension.isUltraWarm()` before it places:
a bucket emptied there places **nothing** — the water evaporates on the spot —
puffs a burst of smoke, and the player keeps the empty bucket. Give the nether
the same: the block is never placed, the fizz shows, the bucket comes back.

**Where.**
- `src/player.c:769-778` — the `IK_WATER_BUCKET`/`IK_LAVA_BUCKET` case:
  `set_block(px, py, pz, k == IK_WATER_BUCKET ? B_WATER : B_LAVA)`
  (`player.c:774`). In the nether (`world_dim == DIM_NETHER`, `nb.h:404-405`;
  `dim_nether()`, `render.c:210`) a **water** bucket skips the `set_block`,
  spawns the smoke at the target cell, and still returns the bucket
  (`h->id = I_BUCKET`, as the lava branch already does at `player.c:775`).
  Lava is untouched.
- `src/entity.c:14,44,51-58` — `SMOKE` (`entity.c:14`) and `particle_add`
  (`entity.c:44`); add a small helper beside `particles_spawner` (a
  `particles_evaporate(x, y, z)`) puffing a few grey puffs up and out of the
  cell. `N_PART` is 64 (`nb.h:268`) — a handful per use, no new storage.
- `src/player.c:754-768` — the `IK_BUCKET` branch fills from a *source* the
  eyes look at (`pick_liquid`, `player.c:519-532`); nothing there changes (the
  nether has no water to pick up).
- The nether's other water entries: rain is off (`tick.c:611`), the generator
  makes none, and the portal lands above liquid (`portal.c`), so the bucket is
  the only place water arrives. A `/setblock water` still places water — it is
  not a bucket — decide and say so in the commit.
- `tests/unit.c` — a case that a water bucket used on a nether face leaves the
  target `B_AIR` and the hand an empty bucket; `tests/play.c` — reuse
  `--nether` with a water bucket in hand (the `--showcase` rig already holds
  water and lava, `play.c:4`) and a recorded frame sequence.

**Notes / the work.**
- Java plays the fizz as a sound; with no sound here the particles are the
  tell, so make them read clearly (a short, dense burst at the placement cell).
- The bucket is consumed either way; Creative keeps it exactly as the lava
  branch does (`player.c:775`).
- `neighbours_changed` must not run for the evaporated cell, or the empty
  placement would wake its neighbours for nothing.
- Reference: Java's `ItemBucket`/`BucketItem` + `isUltraWarm`, particle types
  `SMOKE`/`LARGE_SMOKE`; read the exact step from `26.3.jar`.
- One `versions.md` entry.

**Done when.** In the nether, a water bucket in hand used on a block puffs
smoke and leaves air (no water block, no flow), the hand holds the empty
bucket, and a lava bucket still places lava; `make test` passes;
`make build && make check` fits; and the recorded `./build/play --nether
--frames … --shots …` (**not** `build/desktop`) frames show the puff where the
water would have been.

---

## [26] Liquids at the cache's edge

**What.** A liquid whose source (or supply) leaves the loaded cache drains.
Place a water or lava source high in the nether, let it run down, then go down
until the source is above the cache: the stream's cells recompute with the
source unseen, and an unloaded cell reads as **air** (`world_get`,
`world.c:527`), so `fluid_update` finds no same-liquid neighbour, takes the
level to nothing (`nl < 0`), and airs the block (`tick.c:162-166`); the next
cell down does the same, and the stream is gone. `settle_fall` already has the
right rule for its own case — out of the cache is not air, so a fall waits
until the ground is in sight (`player.c:355-373`) — and liquids need the same:
an unloaded neighbour must read **unknown**, never "nothing", so a liquid
stands until the world that feeds it is whole again.

**Where.**
- `src/tick.c:136-195` — `fluid_update`. The level comes from the four sides
  (`tick.c:145-152`), the block above (`tick.c:155-156`), then four ways'
  `open_to`/`slope_dist` (`tick.c:172-194`), every read through `world_get`, so
  every cell outside the cache is `B_AIR`. It needs a loaded-aware read: if any
  cell it depends on is unloaded (`world_loaded`, `world.c:521`) the update
  does **nothing** this run and the cell is left standing — the same "wait for
  the world to be whole" `settle_fall` uses — rather than re-levelling or
  draining from an unseen side.
- `src/tick.c:48-63` — `open_to` and `below`: the same guard, so an unloaded
  side is not "somewhere to go" and a fall is not re-levelled against air.
  (`flow_into`'s own write already refuses an unloaded target through
  `world_set`, `world.c:533`, so the tail waits there too.)
- `src/tick.c:72-87` — `fluid_wake`, the settle entry: `settle_cache`
  (`world.c:227-237`) runs it once a move's chunks are whole, which is exactly
  where a cell the update left standing gets re-asked. Make sure a flowing cell
  whose supply was out of the cache is woken when the supply is back, and that
  its level is recomputed then.
- `src/tick.c:24-32,198-213` — `fluid_schedule`/`fluids_tick`: an entry whose
  cell is outside the cache is dropped (`tick.c:208`), which is only safe
  because `settle_cache` wakes liquids again when the region returns; the fix
  leans on that, so keep it true for the waiting cell.
- `src/world.c:521-530` — `world_get`/`world_loaded`: the fix belongs in the
  fluid code, **not** in `world_get` (its air-for-unloaded is what every other
  caller expects).
- **Tests.** `tests/play.c:397-430` — `--settle` is the nearest rig (it already
  slides the cache and back); add a `--drain` (or extend it): a source at the
  top with a fall down, then move the cache down so the source is out of it and
  assert the stream's cells are still liquid, and the same with the cache moved
  back. `tests/unit.c` — the level arithmetic with an unloaded neighbour: a
  level-1 flow beside a source two blocks outside the cache keeps its level.

**Notes / the work.**
- The nether is where it shows (a source high over the sea, the fall down to
  it) but the overworld has the same edge; test both.
- One rule for water and lava (the code is shared), but the reach differs
  (water 4, lava 2 — `tick.c:181`) and the decay (1 vs 2 — `tick.c:141`); check
  both.
- No new state, no BSS growth: the guard is a few `world_loaded` calls. The
  pending queue stays `N_PENDING` 160 (`tick.c:20`) — do not grow it.
- Java has no such edge (its force-loaded chunks keep the region); this is the
  game's own cache, so the rule is the game's own. Write the deliberate
  difference down in the commit, as `below()`'s world-floor note does
  (`tick.c:60-63`).
- Watch a source the player placed: it lives in the edit records and returns
  when the cache does, so only the standing stream has to be kept alive in
  between.
- One `versions.md` entry.

**Done when.** A source placed high with its fall left behind as the cache
moves down does not drain — the stream's cells stay liquid while the source is
out of the cache, and are unchanged when it returns — while a source removed
beside a loaded stream still drains it as before (the edge does not freeze live
flow); `make test` passes; `make build && make check` fits; and the recorded
`./build/play` frames (**not** `build/desktop`) show the fall intact with the
source above the cache.

---

## [27] Deepslate caves

**What.** The deep layer (below y=0, [16]) is solid deepslate: the 1.8 carver
stops at y=0, so under it there is nothing but rock and the ores. Carve it with
caves of **1.17-1.19's kind** - the noise caves 1.18 added, not the narrow 1.8
tunnels: big **cheese** caverns (wide open rooms), long **spaghetti** tunnels (a
burrowing loop that wanders the layer) and thin **noodle** tunnels, so the deep
has somewhere to walk, to fall into and to light. The surface world stays exact
(1.8's terrain and its own caves above y=0, `t_refcmp` untouched); the deep caves
are a second pass over the deep layer, the way the deepslate ores (`deep_ores`)
are a second pass, with their own deterministic stream so the 1.8 generator's
random order never moves.

**Where.**
- `src/gen.c:2608,2632` - `cv_get`/`cv_set` (the carver's block access) both
  reject `y < 0` (`if (y < 0 || y > 255) return ...`), and the carve walk's
  window (`slab_caves`, `gen.c:5777`, and `crep_lo` at `gen.c:5794-5798`) comes
  from the slab asked for, so no 1.8 cave reaches the deep rows today. The
  carve tests (`gen.c:2660-2700`) look for `B_STONE`, `IS_DIRTISH`, `B_SAND`,
  `B_GRAVEL`... - a deep cave has to carve `B_DEEPSLATE` (and the deep `B_STONE`
  of the transition zone, [16]) too.
- `src/gen.c:6314-6370` - `gen_slab`'s overworld path: the deep rows are written
  by the fill loop and the deepslate ores by `deep_ores` (`gen.c:6298`), straight
  into `out`, **before** the terrain rows (at `tout`, the 288-byte-a-row offset)
  and `slab_caves` (`gen.c:6411`). A deep cave pass belongs there, over the deep
  rows, its own carver beside `deep_ores`; or `slab_caves` grows a deep branch
  (see the `uint8_t` note). Its result goes into `out` before the same
  `edits_apply` (`world.c`) overlays the player's changes.
- `src/gen.c:2557` - `sum_block` returns `B_BEDROCK` for `y < 0` (a stand-in:
  the column summaries are `uint8_t`), and the carver's out-of-slab reads go
  through it. A deep carver that reads the ground outside its own rows wants the
  deep layer's real block there (deepslate/stone); the `uint8_t` limit is the
  first thing to settle.
- **The stream and the slabs.** `deep_ores` (`gen.c:6298-6310`) is the pattern: a
  `JRand` seeded from the chunk alone
  (`cx * 341873128712 + cz * 132897987541 ^ salt`), every draw in a fixed order,
  cells written only where the window holds them, so the same chunk comes out the
  same whichever slab window asks (the glowstone lesson of 1.5.4, and [22]'s
  "same whichever window"). Build the deep carver the same way, and draw from
  **nothing** the 1.8 terrain (`r`) or population (`PR`) streams use, or `t_refcmp`
  stops reading 0 differ.
- **The shapes.** 1.18 makes its caves from noise fields (a "spaghetti" field's
  two tunnel families, a "cheese" cavern field, the thin "noodle" field and an
  aquifer level), thresholded with a vertical shape that is flatter and wider
  deep down; 1.8's carver is a walk of fixed-point tunnels. A faithful but
  calculator-sized version: a few simplex/perlin octaves (the generator already
  has `simplex2`, `perm_*`, the octave builders and `jr_seed`) evaluated over the
  deep rows, tuned so the layer is mostly rock with big open caverns, long
  tunnels and water at a chosen level. The seams matter: a 1.8 surface cave that
  reaches y=0 may open into a deep cavern (the two carvers meeting at the
  boundary), and a deep cavern that reaches y=0 must not break the 1.8 surface
  terrain (carve only under the surface `gen_top`/the `ColSum` give).
- **Falling and the cache.** Deep caves change what `settle_fall`/`fluid_wake`
  and the light see; the settle pass (`world.c`'s `settle_cache`) already runs
  over the whole cache after a move, so a sand shelf or standing water the caves
  leave is handled as above ground. The deep ores are not liquid or sand.
- **Cost.** A noise carver over the deep rows can be slow; measure `gen_slab`
  with `tests/gen/arm/bench.py` before and after and quote it, as [16] and [22]
  do, and keep the fill one chunk a frame. No BSS growth is the standing rule; a
  new noise field's statics are the documented exception (numbers in the commit).

**Notes / the work.**
- **The `uint8_t` block.** `cv_get`/`cv_set` and `sum_block` are `uint8_t`
  (states below 256); `B_DEEPSLATE` is 434 and does not fit. Either write the
  deep carver as its own function with `uint16_t` blocks (clean: it never touches
  the exact 1.8 walk), or widen `cv_get`/`cv_set` to `uint16_t` (ripples through
  the cave code). Prefer the former unless it proves awkward, and say which in
  the commit.
- **The reference.** `26.3.jar` (repo root) and `minecraft-textures/` are the
  source for the modern shapes and numbers (the carver in the jar is
  `NoiseChunk`/`CaveDensityFunctions`, names and all); the **1.17-1.19 look** is
  the target, not one jar's block-for-block result.
- **The light.** A deep cavern open to a dug shaft or a surface cave reads the
  sky down to it (`vtop` lowers through the changes, 1.5.26); a fully closed deep
  cavern is dark, and lighting it is the player's job, as in Minecraft.
- **Tests.** A `tests/unit.c` case that a chosen seed's deep chunk holds a cavern
  (the same size whatever slab window is asked); a `--caves` scenario in
  `tests/play.c` beside `--deep` that lands the camera in a deep cavern; the
  reference comparison stays 0 differ from y=5 up.
- One `versions.md` entry (title on the first line, as always).

**Done when.** Standing at the deepslate (from a dug shaft or a surface cave that
reaches it) there are big open caverns and long tunnels down to the floor - not
the 1.8 tunnels - with water where it pools; they are the same whichever way the
world is walked (a chunk replayed in any slab window comes out the same, and the
same from every direction); the surface is unchanged and `tests/gen/t_refcmp` is
0 differ from y=5 up; `make test` passes; `make build && make check` fits (numbers
in the commit); and scripted `./build/play --frames ... --shots ...` frames
(**not** `build/desktop`) show a cavern and a tunnel.

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
