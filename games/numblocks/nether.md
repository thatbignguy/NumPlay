# NumBlocks — the Nether

A plan for a basic Nether dimension: modern Nether Wastes terrain (netherrack
everywhere, caverns, lava sea, red fog and red sky), a portal to get there, and
the block-state widening it needs. Written as a prompt: what each piece is,
where the code lives, what "done" means. Nothing is implemented yet.

Numbering follows `todo.md` ([7] and up). [7] is prerequisite infrastructure —
512 block states, without which the rest has nowhere to live — and is meant to
be its own commit.

- [ ] **[7] Widen block states to 512** (9-bit packing) — the enabling change
- [ ] **[8] The nether generator** — nether wastes: netherrack everywhere
- [ ] **[9] The nether blocks** — netherrack, soul sand, quartz, portal
- [ ] **[10] The dimension in the world** — one cache, one generator state, reseeded
- [ ] **[11] The nether on the screen** — red fog, red sky, no sun
- [ ] **[12] The nether's ticks** — no weather, static lava, no sky light
- [ ] **[13] Two dimensions in the save** — per-dimension edits, NBW5
- [ ] **[14] The portal** — obsidian frame, enter, come back

---

## Constraints (RAM, CPU, storage)

Everything below is measured against the same three budgets, and a smaller
number beats a cleverer one every time — while still delivering the look in the
next section.

- **RAM.** The calculator's budget is 153676 B and the game uses 128199 today:
  ≈ 25 KB free, ≈ 19.5 KB after [7]. That is the *whole* allowance for the
  nether. It is met by sharing: one 40×40×24 block cache for both dimensions,
  one generator state (reseeded from a derived seed, never duplicated — the
  generator's 27108 B of statics serve both), and no second warm cache (a second
  one would be another 77 KB and does not fit).
- **CPU.** Frames cost 13-39 ms on the M7 today (bench_render.py) and the nether
  must not add to them: the short red fog, the missing sky and the missing
  celestial maths make it cheaper instead. Generation has to come out of the
  same budget: a nether chunk in ~5-15 ms (vs. 59 ms for an overworld chunk),
  and the portal crossing must fill through the existing one-chunk-a-frame
  machinery rather than a multi-second hitch.
- **Storage.** The edit record entry stays 4 bytes per changed block (19-bit
  position + 9-bit block state), the world record grows ~13 bytes, and the
  nether's own edit records cost nothing until the player builds there.
  `copy_write()`'s base64 copy doubles whatever it saves, so do not add
  per-dimension bulk that is usually empty.

---

## The look (the point of the whole thing)

Regular modern nether terrain — Nether Wastes: **netherrack everywhere**. The
ground, the walls of every cavern and the ceiling are all netherrack. Nothing
else is in view but the open spaces, the lava sea and the fog: no grass, no
dirt, no stone, no sky.

What the player must see:

- netherrack ground and walls, everywhere
- open caverns where the density noise leaves air (the walls are just cliffs, in
  the ordinary sense — nothing exotic)
- a lava sea filling all air at y ≤ 31 (modern Java: the lava sea forms at and
  below Y=31 in every Nether biome)
- a rough bedrock floor at y=0 and a rough 5-layer bedrock ceiling at y=123..127
- glowstone blobs hanging from the ceiling; optional soul sand and gravel
  patches around the lava sea level
- fog closing at ~10-16 blocks, and the nether's red where the rays run out of
  it: **#330808** (51, 8, 8) — Java's Nether fog colour, and what the skybox
  shows wherever there is nothing to draw. Modern Nether wastes fog starts at 10
  blocks and is opaque well before 96, and it never follows the time of day or
  the weather.
- no sun, moon, stars, clouds or rain. There is no sky: the red *is* the sky.

What it must NOT look like: the overworld with netherrack swapped for grass.
The overworld's shape comes from a height field (`chunk_density`, gen.c:1826:
depth noise blended into the biome's depth/scale, `colD3 = 8.5 + d1 * 4.0`) and a
sea level (`#define SEA 63`, gen.c:148) that `column_density` (gen.c:2338) and
`build_surface` (gen.c:2223) are both built around — rolling hills around y=63
with grass and dirt on top. The nether runs the same machinery with none of
that: one density over the whole 128-block column, thresholded
(`> 0 → netherrack`, else air). Its low frequency is what makes solid netherrack
form big masses with caverns between them instead of hills.

Modern wastes carve their very largest rooms with a separate "erosion" noise;
1.8 runs no cave carver in the nether at all and gets the same effect from the
density alone, which is what this does. So: reuse gen.c's *machinery* (octaves,
`perm_build`, `coord_q32`, the slab plumbing, `JRand`), never its *formula*.

## [7] Widen block states to 512 (9-bit packing)

**What.** Block state ids are `uint8_t` everywhere: `vc[]` (nb.h:93), the edit
log's top byte (edits.c:161,170), and every `blk_*[B_COUNT]` table. `B_COUNT =
252` (blocks.h:259), and that number counts *states*, not blocks — this game
bakes metadata into the id like 1.8 does (`B_LOG_OAK`, `_X`, `_Z`, `_BARK` are
four ids; oak doors are 16), so only **4 spare ids** remain. Items are 16-bit
(`Stack.id`, `blk_item[]`), so only *placed block states* compete for slots.
Packing the storage to 9 bits gives **512 states** (2×), which covers the whole
nether family with ~250 spare.

**Where.**
- `src/blocks.h`: add `NSTATES 512`; keep `B_COUNT` as the count of *defined*
  states (252 today), enumerate new ids by appending.
- `src/data.h` + `tools/blocks.py`/`tools/pack.py`: emit the per-state tables at
  `[NSTATES]` (padded); the padding is ~+10 KB of `.rodata`.
- `src/world.c`: one `vc_get(int i)` / `vc_set(int i, int b)` pair (an unaligned
  32-bit load, `>> (bit & 7)`, `& 511`, with 2-3 bytes of tail padding) and
  switch every direct site: 33 of them — 15 in render.c, 14 in world.c, 1 in
  mob.c. Everything else already goes through `world_get`/`world_set`.
- `src/gen.c`: `gen_slab`'s `out` buffer and the fill-path buffers (`tov`, `col`,
  `col_tmp`) hold states too — ~10 more sites, all in slow paths; `GEN_SCRATCH`
  grows ~1 KB (it is a stack frame — re-measure the depth).
- `src/edits.c`: the entry becomes position(19 bits) | block(9 bits) at
  `b << 19` — 28 bits, so the record entry is still 4 bytes; `edits_top` and
  `edits_apply` read `(e >> 19) & 511`.

**Notes / the work.**
- RAM: `vc` 38400 → 43200 B, `slab32` 6144 → 6912 B, `liquid[]`/`shape_top[]`
  (render.c:604-605, read per ray step) +512 B, gen.c scratch ~+1 KB of stack.
  Total ≈ **+6 KB of BSS**: 128199 → ~134 200 of the 153676 budget, ~19.5 KB
  left.
- The cache slide survives: `recenter()`'s row `memmove` (world.c:266-272) moves
  in multiples of 8 blocks and **8 cells × 9 bits = 9 bytes exactly**, so every
  slide stays byte-aligned; only the row stride (45 B instead of 40) and the
  `VC_I` index math change.
- 10 bits was considered and rejected: one more instruction per cell read
  (`i * 9` is a single `add` with a shifted operand on Thumb-2, `i * 10` takes
  two), +13.6 KB of BSS, and ~2× the flash table padding, for 1024 states the
  game has no use for.
- Render cost to *measure*, not assume: the adaptive picture traces ~33 k DDA
  steps per frame (`play`'s counters: 6.7 steps × 4955 traced pixels), plus the
  fill paths — call it ~40-60 k cell reads. At +4-5 instructions per read that
  is ~+1 ms per frame (≈ +3-8 % on the 13-39 ms frames). Keep a running bit
  counter in the DDA loop instead of re-multiplying from `i`, which makes 9-bit
  cost the same as 10-bit would.
- Old 8-bit edit records must be re-encoded on load — ride that on the NBW5
  save bump from [13], so the two save changes land together.
- The repo's standing rule is "no BSS growth": this item is the explicit,
  documented exception, with the before/after bench numbers in the commit.

**Done when.** `NSTATES`-sized tables, every direct `vc[]` site on the accessor,
old worlds load and re-save their edits losslessly, `bench_render.py`
(before/after) shows byte-identical pixel checksums for the overworld with the
render delta quoted, `bench.py` reports the new deepest stack, `make test` and
`make check` pass, and `git status` is clean.

## [8] The nether generator

**What.** A second generator branch in `gen.c`: a `nether_slab()` that fills a
chunk with modern nether wastes terrain (netherrack everywhere, caverns where the
density leaves air, a lava sea), and `gen_set_dim(int)` to choose it. The
recipe:

- density = noiseA + noiseB sampled on a coarse 4×8×4 cell grid (5×17×5 = 425
  cells per chunk), `> 0 → netherrack`, else air. No height field, no sea level;
  above the netherrack there is nothing but the ceiling.
- lava sea: every air block at y ≤ 31 becomes lava (modern Java: at and below
  Y=31, in every Nether biome).
- bedrock floor at y=0 (rough) and a rough 5-layer ceiling at y=123..127.
- v1 surface block: netherrack only. Optional refinement, from the same
  low-frequency noise as modern uses for its patches: soul sand and gravel
  around the lava sea level — that noise doubles as the 1-3 "biome" split if one
  is wanted.
- quartz ore veins, kept small and cheap: 1.8 placed about 20 veins of 4-11
  blocks per chunk between y=10 and y=117 (a line or two over the density slab,
  no extra machinery). No other ores — nothing else in the nether needs them.
- no caves, no carvers, no population, no structures: the open spaces are what
  the density noise leaves, and the decorations are the three lines above.

The coarse grid is what makes it read as nether rather than hills: its low
frequency lets solid netherrack form big masses with caverns between them.

**Where.** `src/gen.c`: the reseed in `gen_init` (gen.c:5374), a new slab writer
dispatched from `gen_slab` (gen.c:5891), and `gen_biome` (gen.c:5944) returning
one constant "hell" biome (1.8's id 8, unused in the `BI_*` enum) so no biome
layers, no Voronoi and no biome cache run at all.

**Notes / the work.**
- Reuse the octave statics, do not duplicate them: `gen_init` reseeds
  `oc_min/oc_max/oc_main/oc_depth/oc_surf` from a *derived* seed (the world seed
  mixed with a constant), so the second dimension costs **0 B** — the generator's
  27108 B of statics are shared. (Vanilla reuses the same seed with a different
  generator chain; here the derivation is a deliberate deviation.)
- Skip everything the nether has no use for: `sum_get`/`ColSum` (the summary
  cache exists so population can read the terrain view), `slab_caves`, the four
  `populate()` calls. A nether chunk is then 2-3 octave sets on 425 cells —
  measured against the overworld's 12.78 M insns ≈ 59 ms per chunk (bench.py),
  expect ~1-3 M insns ≈ 5-15 ms. That also shrinks the portal-crossing hitch:
  ~25 chunks to fill becomes ~0.3 s instead of ~1.5 s.
- `ColSum`'s 4-bit top-block code (16 slots) only matters to population code the
  nether never calls — but do not let the summary table overflow if a later
  feature needs it.
- If exact vanilla nether determinism is ever wanted (a Java reference the way
  the overworld has `tests/gen/ref_terrain.h`), that is a much bigger port and
  out of scope here.
- Later refinement if the caverns feel too small: modern wastes carve their
  largest rooms with a dedicated low-frequency "erosion" noise (a term
  *subtracted* from the density). One more octave set, no new machinery.

**Done when.** `./build/play` frames of the nether show netherrack ground and
walls with open caverns and a lava sea (and *not* rolling hills, grass or
dirt), `bench.py` numbers for the nether path are quoted, and the generator's
static RAM is unchanged (still `gen_ram_bytes()`).

## [9] The nether blocks

**What.** Append to `blocks.h` (after id 250, so `is_water`/`is_lava`/
`is_door_lower` ranges stay intact). Three are needed for the target look:
**netherrack** (the one terrain block), **soul sand** (patches around the lava
sea, plus the slow-sand mechanic), and the **nether portal** block.

Also in scope: **nether quartz ore** and **block of quartz** — add them as a
pair, since neither is much use alone (the ore is the only source of quartz, and
the block is where quartz goes). They cost two of the 512 states and ~320 B of
textures, and they need the crafting recipe (modern: 4 quartz items → 1 block)
in `tools/items.py`. Gravel, glowstone, lava, fire, obsidian and both mushrooms
already exist. Nether bricks, nether wart block and magma stay optional.

**Where.** `src/blocks.h`; `tools/blocks.py` (the tables), `tools/items.py`
(the label, the icon request, the recipe), `tools/pack.py` (textures),
`src/names.c` via `tools/names.py` (the `/give` names), `src/gen.c`'s
`flat_names[]` (optional aliases).

**Notes / the work.**
- **Textures come from `minecraft-textures/`** — the modern client dump the
  Makefile already points at (`MODERN_TEX ?= ../../minecraft-textures`, used by
  `make data-modern`). Ask for the netherrack, soul sand, quartz ore, quartz
  block and portal pictures there, as the GUI sprites and every other modern
  block picture already are. Per new texture: 128 B of pixels + a 16-entry
  palette in flash; per new state: ~35 B of `.rodata` table rows and, in
  `render.c`'s `liquid[]`/`shape_top[]`, 1 B of RAM each.
- Portal block: self-lit (`blk_light` ~11, as vanilla), opaque-ish; a static
  texture is fine — the game has no texture animation.
- Soul sand: the 1.8 movement slowdown is a `phys.c`/`player.c` tweak; magma:
  damage on contact. Both optional for a first pass — say in the commit what was
  left out.
- Quartz ore drops the quartz item directly (no furnace), and the item is free:
  item ids are 16-bit, so only the two *block* states spend ids.
- New ids must not fall inside the contiguous ranges in `nb.h`'s `is_*`
  helpers; appending keeps that true.

**Done when.** Every new block places, breaks, drops, appears in the creative
search tab, and works with `/give` and its completion; quartz ore generates in
the nether and crafts into the block; the new pictures come from
`minecraft-textures/`, regenerated with `make data-modern`.

## [10] The dimension in the world

**What.** A `world_dim` global (the `world_type`/`WT_*` analog), `gen_set_dim()`
wired into `world_new`, and the dimension switch treated as a cache recenter:
`recenter()` already regenerates the whole 40×40×24 volume around a far-away
position (that is what a long `/tp` does), so walking through a portal is just a
recenter on the far side with the other generator active.

**Where.** `src/nb.h` (`enum { DIM_OVER, DIM_NETHER }`), `src/world.c`
(`world_new` → `gen_set_dim`), `src/gen.c` (`gen_set_dim` reseed), `src/save.c`
(it stores the dimension — see [13]).

**Notes / the work.**
- One cache, not two: a second warmed cache would be another 77 KB and does not
  fit. The overworld's blocks around the portal come back from their edit
  records when the player returns.
- The nether uses the full y range 0..127 like the overworld, so `VCY`'s
  24-block window and the vertical slide (`high` mode in `world_follow_gen`) are
  enough — no new vertical machinery.
- Portal ↔ world coordinates: vanilla's 1:8 mapping (nether x/8, z/8), with the
  return position derived from where the portal was lit.

**Done when.** A round trip through the portal swaps generator, light, sky and
biome consistently, and returning restores the overworld's edits around the
portal.

## [11] The nether on the screen

**What.** A `dim_nether()` branch in `render_frame`: the sky and the fog are the
nether's red, there is no sun, moon, stars or clouds, and the whole per-frame
sky calculation is skipped instead of run.

- **The red is the skybox.** Where a ray runs out of the world (past the render
  distance, out through the sides, into the parts of the sky none of the drawn
  geometry reaches), `sky()` must return the nether's fog colour, **#330808**
  (51, 8, 8) — not the overworld's blue gradient. The same colour is what the
  fog fades everything to, so the horizon dissolves into red haze in every
  direction, as in Java's Nether.
- Fog starts ~10 blocks out (modern Nether wastes) and is full by the render
  distance — shorter than the overworld's, which is also a speed win.
- No celestial bodies at all: the existing `sky_fast()` path already skips the
  sun/moon/star maths when there is no sunset and no stars, and the
  temperature/time-of-day sky-colour block is bypassed entirely — the nether
  fog never changes with the time of day or the weather. Cheaper *and* correct.

**Where.** `src/render.c`: the sky colour block (~render.c:2292-2331), the fog
setup (~render.c:2380), `sky()`'s early-outs (render.c:191-255), and the F3
overlay (a "NETHER" line next to the world type).

**Notes / the work.**
- On the 160×120 picture the red sky is also what makes the nether *feel*
  enclosed: the fog and the sky being the same colour is the whole point.
- Underwater/lava-in-your-face tinting already exists for water; lava needs the
  equivalent only if it looks wrong (say what was left out).

**Done when.** `./build/play` frames show red fog and a red sky everywhere a ray
leaves the world, with no sun, moon, stars or clouds anywhere; and
`bench_render.py` shows the nether's frame cost at or below the overworld's.

## [12] The nether's ticks

**What.** The dimension-conditional behaviour in the tick path: sky light is 0
(`light_box` skips the sky pass and only block light spreads), no weather, the
lava seas are static (no random fluid ticks — vanilla's nether lava does not
flow), no mob spawning, no snow or ice, and `sky_sub()` (day/night darkening)
stops applying.

**Where.** `src/world.c` (`light_box`, world.c:41), `src/tick.c` (`world_tick`,
`sky_sub`, `rain_at`, the random ticks), `src/mob.c` (`mobs_spawn`),
`src/entity.c` (the spawn rules at entity.c:74).

**Notes / the work.**
- Skipping the sky-light pass also undoes the `light_box` worst case: in the
  nether every cell would otherwise spread both light kinds.
- Fire and lava still burn; the nether's fire does not go out from rain either.

**Done when.** The nether is lit only by lava, glowstone, fire and torches; the
world tick cost is not higher than the overworld's; no mobs appear.

## [13] Two dimensions in the save

**What.** A `NBW5` head: the current dimension (1 byte) and the nether return
position (12 bytes), keeping the `NBW1..4` readers; edit records tagged per
dimension (`region_name` gains a dimension marker, e.g. `nb1n-3_12.nbe`
alongside `nb1r-3_12.nbe`); and the 8→9-bit re-encode from [7].

**Where.** `src/save.c` (the `Head5` record and the `read_head` ladder),
`src/edits.c` (`region_name`, the entry's block field).

**Notes / the work.**
- The region name's marker is what keeps the two sets of player edits from
  mixing; `plat_remove_prefix` on the world prefix still cleans both up.
- `copy_write()`'s base64 copy grows only for the dimensions actually built in.
- The 8→9-bit conversion: read the old records, re-encode with the block at
  bits 19.., write back once (and flag the world as converted so it happens
  exactly once).

**Done when.** Worlds from 1.3.x load, convert and re-save losslessly; both
dimensions' edits persist across a portal round trip and an app restart.

## [14] The portal

**What.** The player stands inside an obsidian frame, lights it with flint and
steel, and after 1.8's stand-in-the-portal wait (a few seconds — confirm the
exact tick count against `Entity.setInPortal`'s `timeInPortal`) travels. Portal
blocks fill the interior; on arrival the portal is spawned/created on the far
side and lit.

**Where.** `src/player.c` (`player_tick`'s portal check), a new small
`portal.c` (or `tick.c`) for the frame scan, enter/exit, and the far-side spawn;
`src/render.c` for the portal block.

**Notes / the work.**
- The frame scan follows vanilla: try both axes, require the 4x5 minimum, then
  light it. Keep it small — this is a detection routine, not structure finding.
- Handle the failure cases: no room on the far side, coming back into solid
  rock (the `loading()` unstick loop, main.c:54, already exists).
- The nether's spawn point: arriving player is placed at the portal they came
  through; a fresh nether visit (none remembered) needs a safe spot — reuse
  `gen_spawn`'s flat-preset trick ("anywhere within 256, above the top").

**Done when.** A lit portal in the overworld takes the player to the nether,
the return portal is created or found, and the round trip works in `./build/play`
scripted frames.

---

## Decisions to confirm before coding

1. **Exact fog start**: modern Nether wastes starts at 10 blocks and is opaque
   well before 96; ours must close inside the ~19-block render distance, so
   ~10 blocks in. Recommendation: start 10, full by the render distance.
2. **Caves in the nether**: off for v1 — the density noise leaves the open
   spaces, and carvers can come later.
3. **Lava sea level**: decided — y ≤ 31, as modern Java (in 1.8 the sea sat
   much lower; we want the modern look).
4. **Biomes**: nether wastes only for v1, or the netherrack / gravel /
   soul-sand patches from the surface noise. Recommendation: the patches — they
   come out of the same noise.
5. **Soul sand / magma mechanics** (slowdown, contact damage) in v1 or later.
6. **Exact vanilla determinism** of the nether noise: no (documented deviation,
   derived seed).

## What this is not

No fortresses or any structures, no modern nether *biomes* (crimson/warped
forests, soul sand valleys, basalt deltas — their textures and blocks included),
no carvers, no second warm chunk cache, and no bit-exact vanilla determinism of
the noise. The 1.8-style portal mechanic and the modern wastes terrain and block
set are what is wanted; everything else is out of scope.

## Checking any of it

```bash
cd games/numblocks
make data-modern                                  # regenerates src/data.c from minecraft-textures/ (new blocks)
make test                                       # unit tests
make host && ./build/play --frames 200 --shots 60,120 --out /tmp/nether   # scripted frames
python3 tests/gen/arm/bench.py /tmp/nbgen        # generator cost + stack depth
python3 tests/render/arm/bench_render.py /tmp/nb0   # render cost (before)
python3 tests/render/arm/bench_render.py /tmp/nb1   # render cost (after): pixel checksums must match
make build && make check                         # device .nwa + BSS <= 153676 B
```

Test with `./build/play` (scriptable keys and shots), not `build/desktop`.

Rules that apply to every item: no new `-Wall -Wextra` warnings, keep
`git status` clean. [7]'s BSS growth is the one deliberate exception to the
"no BSS growth" rule and must be argued with the before/after numbers in its
commit. Ship both the calculator `.nwa` and the desktop binary at the end.
