## 1.5.45
**Big contiguous nether biomes; soul sand valleys are soul ground throughout**

- **Biomes.** The nether biome lattice is coarser: 192-block cells, 112-block seed
  radius, about three quarters of cells seeded. Each biome is one large connected
  area with no foreign pockets inside it, instead of many small noise-defined
  pockets. The wastes remain unseeded areas, and forests no longer run forever.
- **Soul Sand Valley.** The whole exposed floor is soul sand now (with fossils),
  not soil/sand noise patches cut out of netherrack. Valleys also grow nether
  sprouts rather than crimson roots.

## 1.5.44
**Nether biomes are contiguous lattice patches, not noise pockets; soul sand valleys get nether sprouts**

- **Biomes.** The nether's three modern biomes are now chosen from a coarse lattice of
  biome seeds: each 64-block cell at its centre holds a seed about two times in three
  (crimson forest, warped forest or soul sand valley), or nothing (nether wastes). A
  column takes the seed it lies in, so a biome is one contiguous area with no pockets of
  another biome inside it, and no forest runs on forever. The wastes stay the default.
- **Soul sand valleys.** They now grow nether sprouts, not crimson roots, and at a
  sparser rate, so their ground reads as soul sand/soil instead of a carpet of crops.

## 1.5.43
**A third world type - Test: an empty world that is always the same, whatever the seed, with everything the game has in it once**

- **The world.** `WT_TEST` (after `WT_DEFAULT`, `WT_FLAT`) makes the generator build one
  fixed world out of nothing: no seed, no noise, no terrain. `gen_set_test` puts the
  generator into it (kept across `gen_init`, as the flat mode is) and `gen_slab`'s new
  branch writes it; `gen_biome` answers plains, `gen_top` answers the row each column
  stands on, `gen_spawn` answers the middle of the spawn platform. A chunk comes out the
  same whatever the seed and whatever slab window asks for it, so the world is the same
  every time and does not depend on what has been visited. The create screen's World Type
  row steps Default, Superflat, Test (Customize stays Superflat's), and a test world saves
  and loads like any other.
- **The sections.** The player spawns in the middle of a 16 x 16 platform of bedrock. Every
  block state there is - one of each, in the order of the ids, air and the two reserved
  slots aside - floats in a grid of 21 columns x 23 rows with one air gap between each; the
  platform under it is a barrier (invisible until you hold one, 1.5.42), which is what keeps
  the sand and the gravel from falling and the sources from spreading, and the liquids have
  barriers on their four sides as well. Every mob stands in a cage of its own on a floating
  bedrock platform - glass walls and a floor, a roof with a glowstone over the room - so the
  daylight does not set the monsters alight; `mob.c` puts the ten of them there and puts one
  back the moment it is gone. The structures the world generates - a desert pyramid and a
  dungeon - are built by the generator's own structure code, block for block as the real
  ones, each on its own platform of bedrock two blocks wider than the structure on every
  side; the dungeon keeps its spawner and chest (unset, as one from the inventory is) and a
  doorway in its north wall so the room can be walked into, and the pyramid keeps its plate
  and its nine TNT: stepping on it is the trap, as in Minecraft. The pyramid's platform sits
  on SEA's row because its foundation fills down to the first block that is not air or
  liquid, and a dungeon's box has to be below SEA because WorldGenDungeons asks for rock of
  its own to carve.
- **It never changes.** Nothing in it moves on its own: `tick.c` keeps the weather and the
  random block ticks off it (no rain, no fire spreading or burning out, no grass spreading,
  no saplings growing, no leaves decaying, no snow or ice melting, no crops or canes
  growing) and `mob.c` keeps the natural spawning off it, so what is put there stays as it
  is put. `new_world` turns the daylight cycle off for it, as `/gamerule` would, so the sun
  stands at noon: a showcase is looked at in the light.
- **Tests and build.** `make test` passes (214 recipes), with a test-world section: the
  same slab whatever the seed or window, every block state once on its barrier with the
  liquids held in and the gaps air, the ten cages with their mobs in them (put back when
  one is taken away), the pyramid's sandstone and its nine TNT, plate and four chests on
  its platform, the dungeon's walls, spawner, chest and doorway, the spawn point, the
  weather never starting, the fire and the sapling staying put and the world coming back
  the same out of a save. `./build/play --test --frames 60 --shots 59` flies the showcase
  (`--fly "x,y,z,yaw,pitch"` puts the camera somewhere else) and logs what it holds;
  `build/desktop` was built, not run as a test.
- **Cost.** `NumBlocks.nwa` 656,880 B (1.5.42: 654,248 B), `numblocks.bin` 583,908 B.
  `.data` 5,079 B and `.bss` 122,152 B (127,231 B combined, below `CUSTOM_RAM` 133,652 B;
  the test world adds no static of its own beyond one flag). `make -C games/numblocks build
  check host desktop`, `make -C games/numblocks-interest build check`, `apps`,
  `test_numblocks_progress`, `test_games --game numblocks` and `test_small_ram` pass.

## 1.5.42
**Minecraft's barrier block: invisible until you hold one, and an Operator Utilities tab (cheats on) for it**

- **The block.** The barrier (1.8's id 166, `barrier` for `/give` and `/setblock`) is a whole
  cube that stands in the way of the player, the mobs and anything that moves, and gives no
  light and no drop: bedrock-hard (`blk_hard` 255, so it is gone at once in Creative and
  unbreakable in Survival) with no loot table, as Java's `BarrierBlock` has. Its picture is
  the item's, since neither client has a block picture of its own (its block model has no
  elements at all), and the inventory icon is that same flat no-entry sign, as the item's is.
- **What it shows.** You see it only holding one in your hand, as Minecraft's does: the
  renderer reads a barrier as air wherever it draws (render.c's `seen`, one test over the
  world reads it makes), so the rays go through it, the faces beside it are faces against
  air, and the world behind it shows. It still stands in the way, the crosshair still finds
  it and Creative still breaks it, so a wall of them reads as an invisible boundary with the
  outline of the one you look at. Holding one, the sign shows on its faces.
- **The operator items tab.** Mine's second-to-last, one tab left of the Survival Inventory
  tab at the end of the bottom row, shown only with cheats on (Allow Cheats, or Open to LAN
  with them): gui.c's `tab_shown` leaves it out without them, and the Search Items tab
  leaves the barrier out too, as Java's search does. The barrier is its only item; NumBlocks
  has no command block, so the barrier is the tab's icon as well.
- **The light goes through it** (both kinds), as the modern BarrierBlock's does
  (`propagatesSkylightAbove`, no occlusion) - a wall of barriers casts no shadow - which is
  the glass model here: solid, see-through, no light lost. 1.8.8's barrier was an opaque
  cube; the modern one is not, and new blocks follow the modern game.
- Everything that depends on a block below it stands on a barrier too, torches and redstone
  as the wiki says, so an invisible floor is a floor; nothing else about placing changes.
- Left out: the barrier particles modern spawns around barriers while one is held, and the
  operator tab's Debug Stick / structure void (the block is the item here). `tests/play.c
  --barrier` is the rig for it: a wall five blocks ahead of the player with one in the hand,
  hidden again by moving to an empty hotbar slot.

## 1.5.41
Nether biomes split into larger regions (crimson/warped forests and soul sand valleys now span several chunks); fixed ground-block glitch in generated nether terrain.

## 1.5.40
**The Nether gains its Crimson, Warped and Soul Sand biomes, with their own ground, growth and fog**

- **The biomes.** The Nether Wastes remains id 8 (also named `nether_wastes`; the old `hell`
  name still resolves). The Crimson Forest, Warped Forest and Soul Sand Valley use 1.16's ids
  49, 50 and 51. Two seed-derived, smoothly interpolated value-noise fields split the nether
  into chunk-scale regions; one function supplies both the biome line (`gen_biome`, F3 and
  `/locate`) and the terrain, so a column's answer is stable whichever slab is made. This is a
  small deterministic split, not modern's multi-noise climate sampler. Temperature and rainfall
  for all three are nominal 2.0 / 0 (warm and dry); nether weather stays disabled.
- **Ground and growth.** Crimson and warped nylium surface exposed netherrack in their forests;
  crimson fungi/roots, warped fungi/roots and nether sprouts decorate them. Chunk-seeded huge
  fungi have stems, wart-block caps and shroomlight; crimson weeping vines hang from cave
  ceilings, and twisting vines grow on warped fungi. Soul Sand Valley patches soul sand and
  soul soil across its surface, adds buried bone blocks, crimson roots and soul fire. The
  generator's separate chunk stream and whole-column floor/ceiling maps keep every feature
  identical across slab windows. Basalt Deltas, basalt pillars, hoglins and striders are not
  added; the simplified huge-fungus shapes are small box-model clusters, not modern's full
  structure placement.
- **Blocks and fire.** Added the two nyliums, crimson/warped stems and stripped stems (axis
  states included), planks, wart blocks, shroomlight, bone block (axis states), fungi, roots,
  sprouts, both vines, soul soil and soul fire. Item blocks have modern textures, labels,
  creative entries, `/give` names and stem-to-plank recipes; soul fire remains block-only like
  fire in Minecraft. Axes strip either stem while preserving its axis. Nether wood is not
  flammable, furnace fuel or charcoal material. Soul fire lights at level 10, becomes the fire
  kind when lit on soul sand or soul soil, and stays there without spreading or aging out.
  `/setblock soul_fire` places the non-item state. Existing block ids are unchanged: the
  pressed plate and Nether states append after the old states, and unused state slots 447/448
  remain reserved for the piglin egg item ids.
- **Spawning.** Piglins and zombified piglins spawn on wastes and crimson-forest ground; the
  existing skeleton spawns in Soul Sand Valleys. As in the earlier mob pass, nether spawns
  ignore light, keep the hostile cap, and avoid the lava sea. The game has no enderman or ghast,
  so the warped forest has no natural mob yet, and valley ghasts are omitted.
- **Pressure plates.** The already-present temple plate work is included with this release:
  plates show their pressed state, release when entities leave, light the pyramid's nine TNT,
  and open/close adjacent oak doors. The plate record waits while its cell is outside the cache;
  only living mobs and the player hold it down.
- **Tests and build.** `make test` passes (214 recipes), including all four nether biomes,
  surface/growth and repeated-window determinism, soul-fire persistence/release, biome-specific
  spawns, temple TNT, plate release/door power and saved-state id assertions. Scripted
  `./build/play --biome nether_wastes|crimson_forest|warped_forest|soul_sand_valley --frames 40
  --shots 39` captures each environment; `build/desktop` was built, not run as a test.
- **Cost.** `NumBlocks.nwa` 635,108 -> 650,676 B; `numblocks.bin` 566,508 -> 582,152 B.
  `.data` 5,079 B and `.bss` 122,136 B (127,215 B combined, below `CUSTOM_RAM` 133,652 B;
  no generator static field or BSS growth). `make build && make check`, `make desktop`, and
  `make -C ../numblocks-interest build check` pass.

## 1.5.39
**The nether has life: piglins and zombified piglins spawn on the wastes - the piglin hostile, the zombified neutral until hit and then its neighbours with it - with the modern skins and models, drops, spawn eggs, `/summon` and spawners**

- **Spawning.** The every-second pass (`entity.c`) no longer gates the nether off: `mobs_spawn`
  (`mob.c`) takes the dimension. In the nether it rolls on solid ground of the wastes about the
  player, 14 blocks out and more, in groups of two or three, with the hostiles' cap
  (`count(true) < 8`) and the difficulty in charge - and it never looks at light: there is no
  sky light there, so vanilla's nether mobs ignore the light level, and the overworld's
  `light <= rnd(8)` gate would mean nothing. `mob_can_stand`'s opaque floor is what keeps a
  spawn out of the lava sea. Seven in ten are zombified piglins, the rest piglins. Spawners
  make their mob in the nether too now (they made nothing there).
- **Behaviour.** The piglin is the hostile one: the sighted target, the zombie's chase (16
  blocks) and the melee bite (3, scaled by difficulty), 16 health, 5 XP on the kill. The
  zombified piglin is 1.8's zombie pigman: neutral - `EntityAIHurtByTarget` and no
  nearest-target task, so it wanders and never picks the player itself; hit, it turns (the
  panic it then chases by) and every zombified piglin within 32 blocks turns with it, the old
  pigman's one good trick. 20 health. Neither burns in daylight (that rule is the zombie's and
  the skeleton's, and it stays theirs).
- **The models, from the jar.** `AbstractPiglinModel.addHead` read out of `26.3.jar`: a 10-wide
  skull with the 4 x 4 x 1 snout at (31,1), the two cream tusks at (2,0) and (2,4), and the two
  1 x 5 x 4 ears at (39,6) and (51,6) turned 30 degrees out about (4.5, -6, 0) - all on the
  player's own body, arms and legs (the 1.8-scale ones this game has). The zombified piglin
  holds its arms out (1.8's PigZombie pose); the piglin swings them at its sides. 11 boxes
  each, `MAX_PARTS` is 12. The skins come from `minecraft-textures/`
  (`entity/piglin/piglin.png`, `zombified_piglin.png`), folded 64 x 32 into two new `SKIN_*`.
  The zombified piglin shows one ear, as its own art does.
- **Drops.** The piglin's loot table is empty (modern's `piglin.json`: nothing but XP). The
  zombified piglin drops 0-1 rotten flesh and 0-1 gold nuggets (`zombified_piglin.json`); its
  2.5% gold ingot is a looting chance, so it is left out.
- **Eggs, `/summon`, spawners.** `PIGLIN_SPAWN_EGG` and `ZOMBIFIED_PIGLIN_SPAWN_EGG` sit in the
  Spawn Eggs tab, set into a monster spawner, and go by `/give` (the modern
  `piglin_spawn_egg`, `zombified_piglin_spawn_egg`, or 1.8's one egg 383 with data 8 and 57 -
  57 the old PigZombie's, 8 a value the eight 1.8 eggs never used). `/summon Piglin` and
  `/summon ZombifiedPiglin` work and the toolbox finishes them. `mob_of_egg` and the new
  `egg_of_mob` are lookup tables (the eggs are no longer a contiguous run) and the spawner's
  egg (`render.c`) asks `egg_of_mob` rather than doing `set - E_ZOMBIE` arithmetic, which no
  longer pointed at the right egg.
- **The ids.** The new items could not simply follow QUARTZ: an item id is what a save keeps
  and so is a block state's, and the two number spaces share 256 up - the reserved states
  258..395 alias the items' ids, and past them the states are ids of their own (396's chiseled
  quartz block and on). The two eggs therefore take **447 and 448**, in the state space's
  unused tail past every block; the item tables now hold a row an id's slot (`ITEM_SLOTS` 193)
  with the slots between left empty, and `is_item_id` reads an id as an item's where the tables
  have a label for it (`nb.h`). No existing item, block state or save moves. (Item ids in the
  tools are the items' own now - `items.py`'s `item_id`, `pack.py`, `names.py` - since the
  table's index is no longer the id.)
- **Left out, said here.** [15]'s stretch past the pair, the enderman: its teleport is what
  makes it one, and its drop, the ender pearl, has no item in this game. Bartering, the
  golden-armour truce and any piglin anger beyond being hit are out of scope too.
- **Tests.** `make test` passes (210 recipes): the models' box counts (11 each), spawning in
  the nether only - never the nether's mobs in the overworld, and nothing on a peaceful world -
  the piglin's chase and bite, the zombified's quiet then its own and its neighbours' turn when
  hit, no burning in daylight, the despawn, the drops' items and counts, the ids'
  classification (`is_item_id`), the eggs in the Spawn Eggs tab, `/summon` and its completion,
  and an egg on a spawner with the table's save and load.
- **The rigs.** `tests/play.c`: `--mobs` places all ten types (the eight in their old row; the
  nether's two just beyond its ends, so the frames it always gave are unchanged), and with
  `--nether` it is the nether's pair instead, on cleared netherrack, Survival and Normal so the
  piglin gives chase. `--spectate` goes to 0..9.
- **Recorded.** `./build/play --nether --mobs --bright 12 --frames 60 --shots 2,15,35,59 --out
  /tmp/nbmobs` (**not** `build/desktop`): frame 2 the pair on the wastes facing the player -
  the zombified piglin green with its arms out, the piglin pink with its ears and snout -
  frame 35 the piglin walking in, frame 59 it in the player's face, biting (the player at
  Normal is down to 8.5 hearts).
- **Cost.** No BSS growth: `.data` 5,079 and `.bss` 119,064 (the game's 124,135 B inside
  `CUSTOM_RAM` 133,652). `NumBlocks.nwa` 628,912 -> **635,108 B** (+6,196: the two skins 2.2 KB,
  the eggs' sprites, the item tables' slots the rest), `numblocks.bin` 560,744 -> **566,508 B**.
  The picture is untouched: the 18-frame `bench_render.py --rain --mobs` set is
  checksum-identical in Fast (104.91 -> 104.78 M insns) and Fancy (166.26 -> 166.08 M, noise),
  deepest stack 22,224 B unchanged. No new `-Wall -Wextra` warnings. Recorded: `make build &&
  make check` and `make -C games/numblocks-interest build check` fit; `build/desktop` rebuilt.

## 1.5.38
**No more holes in a fall the cache moves through: ticks that run while the move is still queued wait too, not only ones past its edge**

- **The holes.** 1.5.35 kept a stream whose source leaves the loaded cache, but the nether
  still holed: a lava bucket placed high flows down whole, jumping down and flying back up
  leaves gaps in the fall. The move queues several chunks (one a frame); ticks run in between,
  while a column still to be made reads as stone (`world_get`, `world.c`) - solid, but equally
  unknown. A fall re-levelled against that stone - no liquid neighbour above or beside, so the
  level went to nothing and the block aired - and the next cell did the same, punching holes
  the slow lava (30 ticks a move) never refills before it is seen.
- **The fix (`src/tick.c`).** The 1.5.35 guard learns the queued half: `fluid_update` waits when
  any cell it reads is still to be made (`world_pending_at`, `world.c`) as well as out of the
  cache; `below()` answers bedrock below a queued cell; `flow_into()` refuses a queued target;
  `slope_dist()` answers -1 (unknown) on meeting one and spreads it. `fluid_wake` needs nothing:
  stone is not open, so a queued side never schedules. `fluids_tick` still drops an entry whose
  own cell is outside; one whose cell is queued reads as stone (not liquid) and drops the same
  way, and `settle_cache` wakes it once whole.
- **Tests.** `tests/unit.c`: the two `utdrain` moves tick 60 times while queued
  (`world_follow` + `world_tick` interleaved, as `game_frame` does) before waiting whole -
  the edge flows and the falls stand through the queue as well as past it.
- **Recorded.** `make test` passes (210 recipes, all good); the 1.5.35 `--drain` run still reads
  the source outside with the fall `falling` inside.
- **Cost.** No new state, no BSS growth: `.data` 5,079 and `.bss` 119,064; a few
  `world_pending_at` calls beside the `world_loaded` ones, the queue stays `N_PENDING` 160.
  Recorded: `make build && make check` and `make -C games/numblocks-interest build check` fit.

## 1.5.37
**/locate: the nearest biome or structure, at [x, ~, z] in Minecraft's own words**

- **`/locate <biome|structure> <name>`**, as modern Minecraft has it: `biome` (a 1.8
  name, in lower case with underscores, or a modern alias - "plains", "swampland" and
  "swamp" all answer; a numeric id does too) and `structure` (this game's one, the
  `desert_pyramid`, since [23] made it). The answer is Minecraft's:
  "The nearest Plains is at [x, y, z] (n blocks away)" for a biome, and
  "The nearest desert_pyramid is at [x, ~, z] (n blocks away)" for a structure - the y
  is not known without making the chunk, so it is "~", as modern's is. Nothing found
  says so in modern's words ("within a reasonable distance" / "nearby"), an unknown
  name is a red "There is no such biome with name ...", and the toolbox key finishes
  `biome`/`structure` and then the names. `/locate` needs cheats, as in Minecraft.
- **The biomes' names** moved out of the F3 line into `gen.c`'s own table
  (`gen_biome_name`, the F3 and the command share it) with the keys and the modern
  aliases beside it, so the names cannot drift apart.
- **The searches** are `gen.c`'s, and take its scratch for the call (as `world_follow`
  does about its own): the structure by the placement grid's cells outward from the
  player's - the same tests the piece itself makes (a desert at the origin chunk's
  centre, dry ground at the piece's four corner columns, the summaries answering the
  corners) - and the answer is the origin chunk's corner, which is modern's own
  `getLocatePos`. The biome by modern's shape too: a spiral of 32-block sample cells
  outward from the player's, and the cell that first holds the biome read at 4-block
  steps, so the answer is within 4 blocks of the biome's edge (finer than modern, which
  answers with the coarse sample). The work is bounded (1200 samples, ~1000 blocks of
  reach: a chunk's biome layers cost about a hundredth of making the chunk, and
  modern's 6400-block radius is out of the calculator's reach - the not-found message
  is the one for it).
- **Tests.** `tests/unit.c`: from a place in a known temple's own grid cell,
  `/locate structure desert_pyramid` answers exactly "[1376, ~, -224] (302 blocks
  away)"; `/locate biome desert`'s answer is read back (the biome there is the desert,
  the y the column's own top + 1, the distance the 3D one), `biome 2` and `biome swamp`
  answer as the id and the modern alias, the errors and the usage are Minecraft's
  words, and the toolbox finishes `biome`, `minecraft:desert_pyramid` and
  `minecraft:desert`. `tests/play.c` gains `--cmd "line"` (up to four lines run when
  the world is up, their answers in the chat for the frames).
- **Recorded.** `./build/play --cheats --cmd "/locate biome desert" --cmd
  "/locate structure desert_pyramid" --frames 40 --shots 20 --out /tmp/locate`
  (**not** `build/desktop`): frame 20's chat reads "The nearest Desert is at [-196, 66,
  -100] (365 blocks away)" and "The nearest desert_pyramid is at [608, ~, 1680] (1606
  blocks away)".
- **Cost.** The biome tables and the two searches: `NumBlocks.nwa` 623,416 -> **628,400 B**,
  `numblocks.bin` 556,476 -> 560,744 B, `.bss` 119,064 (the chat reader is a walk over
  the lines, not a buffer; nothing new is kept). `make test` passes (210 recipes) and
  `make build && make check` fits.
## 1.5.36
**The desert pyramid: the first structure, placed by modern's own random spread and built block for block from `26.3.jar`**

- **The pass.** `populate` (`gen.c`) has held the dungeons' shape - a replayed
  population, its outcomes kept in the memo - and now takes the next structure as its own
  row: `temple_try` runs before the lakes and dungeons, draws nothing from the
  population's stream, and remembers what it decided (`PMemo.temple`), so the same temple
  comes out whichever direction the world is walked, exactly as the dungeons' `pm_known`
  bits do. Every write goes through `put`/`cget` and lands in each chunk's own slab as
  that chunk is generated, which is how the piece spans its 2 x 2 chunks whole.
- **The placement** is `RandomSpreadStructurePlacement` read out of the jar: spacing 32
  chunks, separation 8, salt 14357617, LINEAR (two `nextInt(24)`s on a `LegacyRandomSource`
  seeded `gridX * 341873128712 + gridZ * 132897987541 + seed + salt`), from the
  population's own chunk. The biome check is `Structure.couldValidBiomeExistOnTopOfChunkCenter`:
  deserts only (the `has_structure/desert_pyramid` tag), asked at the origin chunk's
  centre (block +8, +8) and answered before the populations run, because the biome layers
  and the population view share one scratch buffer (`gen_scratch`) and the layers may not
  be run inside a population. `SinglePieceStructure`'s guard is in too: the lowest of the
  21 x 21's four corners against the sea level. The base row is
  `updateHeightPositionToLowestGroundHeight(level, -nextInt(3))`: the minimum
  `cs_top` over the whole 21 x 21, plus one (the heightmap's first free cell), minus a
  draw of the temple's own stream - never the population's, so nothing else moves.
- **The piece** is `DesertPyramidPiece.postProcess`, transcribed block for block: the
  base box, the nine stepped rings, the foundation that fills down through what is
  replaceable, the four corner towers with their stairs, the entrance and its cut
  sandstone, the gallery, the side aisles with the chiselled pillars, the orange and blue
  terracotta patterns on the floor and the outer walls, and the burial chamber with its
  stone pressure plate, the nine TNT under it and the four chests. `CUT_SANDSTONE` is the
  existing `B_SANDSTONE_SMOOTH` (`24:2`, modern's `cut_sandstone` texture), the terracotta
  reuses `STAINED_CLAY_ORANGE`/`BLUE`; only `B_STONE_PRESSURE_PLATE` (`70:0`) and the four
  `SANDSTONE_STAIRS` states (`128:0..3`) are new blocks (B_COUNT 442 -> 447, sprites
  appended at 503/504 by `tools/patch_temple_data.py`, so no existing sprite index moves
  and `SP_EYE` stays).
- **The orientation is fixed** (the entrance always on the pyramid's low-z side). Modern
  hands the piece a random horizontal direction, but that direction only rotates the four
  stair states and the block states are the only thing it changes: the 21 x 21 is square,
  so the footprint and shape are the same. The four stair facings of the reference piece
  are what this draws.
- **The trap.** `temple_trap` (`player.c`) lights the 3 x 3 TNT two blocks under the
  plate when a player or a mob stands on it (stone pressure plates take mobs), lighting
  the TNT the way fire and flint and steel do (`tnt_light`, 80-tick fuse). The pyramid
  needs no redstone anywhere: the plate is the one place a redstone block behaves.
- **The chests** hold modern's `desert_pyramid.json` loot, drawn when a chest is first
  opened from the world seed and the chest's own place (`gui.c`'s `temple_fill`), so
  nothing new is stored and the same world always gives the same loot: the main pool's
  2-4 rolls over its 16 entries and 247 weight (the counts as the table has them: 2-7 gold,
  4-6 bone, ...), the bone pool's four rolls of 1-8, each item in a random empty slot as
  the table's container fill does. The items this game has no item for are dropped and
  said here: the four horse armours, the enchanted golden apple and the dune armour trim
  (the enchanted book is a plain book). The `temple_chest_at` check is the placement cell
  plus the chamber's own blocks under and over the chest, so it needs no generator call
  when a chest is opened.
- **Left out, said here:** the cellar under the chamber (suspicious sand, the collapsed
  roof and the stairs down from it) - there is no brushing in this game.
- **The reference check.** The piece's whole box was replayed in Python from the
  decompiled `DesertPyramidPiece` (cellar aside) and compared against the generator's own
  slab: 4388 cells written, 0 mismatches, and the four chunks come out byte for byte the
  same in two different generation orders. `tests/gen/t_refcmp.c` is unaffected: it
  compares `chunk_terrain`, which runs before any population.
- **Tests.** `tests/unit.c`: the placement grid finds the temple at chunk (86, -14) for
  seed 12345 (the nearest desert cell) and the same chunk comes out the same in two
  orders; the piece's counts (191 cut, 39 chiselled, 54 + 1 terracotta, 3/3/2/6 stairs,
  2 slabs, 1 plate, 9 TNT, 4 chests) and 38 cells it is known by; the world's cache holds
  the same temple and the plate lights exactly the nine TNT (and nothing five blocks
  west); a chest's first open draws 4-8 stacks, all of the table's items within the
  table's counts, and the same chest draws them again; and the sandstone stairs place
  the four ways a player looks. `tests/play.c` has `--temple` (the player outside the
  entrance looking at it) and `--templetrap` (in the chamber on the plate, the frames
  running past the fuse, the log counting the TNT gone).
- **Recorded.** `./build/play --temple --seed 12345 --frames 10 --shots 5 --out
  /tmp/temple` (**not** `build/desktop`): frame 5 stands over the north slope - the
  stepped face, the corner towers with their terracotta, the cut-sandstone entrance and
  the pattern band all in view, `temple: ... plate 1402 51 -1190, TNT 9 ... chests 4` in
  the log. `./build/play --templetrap --seed 12345 --frames 100 --shots 5,85 --out
  /tmp/ttrap`: frame 5 the lit chamber with the chests and the plate underfoot, frame 85
  the crater the blast left with the player thrown, and
  `templetrap: after 100 frames, 9 of the 9 TNT blocks are gone from the chamber`.
- **Cost.** The piece is code, not tables: `NumBlocks.nwa` 617,092 -> **623,416 B**
  (+6,324), `numblocks.bin` 550,360 -> 556,476 B; `.bss` 119,064 -> 119,072
  (`temple_pb[4]` and its padding, the only new state - the population memo's new byte
  fits its padding). `make test` passes (210 recipes) and `make build && make check` fits.
## 1.5.35
**Liquids at the cache's edge stand instead of draining: a stream whose source leaves the loaded cache no longer empties to air**

- **The drain.** A liquid whose source (or supply) leaves the loaded cache drained: every
  read goes through `world_get` (`world.c`), which answers `B_AIR` outside the cache, so
  `fluid_update` found no same-liquid neighbour, took the level to nothing (`nl < 0`) and
  aired the block (`tick.c`), and the next cell down did the same until the stream was gone.
  `settle_fall` already had the right rule for its own case - out of the cache is not air,
  so a fall waits until the ground is in sight (`player.c`) - and liquids have it now: an
  unloaded neighbour reads **unknown**, never "nothing", so a liquid stands until the world
  that feeds it is whole again.
- **The fix (`src/tick.c`).** `fluid_update` does nothing this run when any cell it reads is
  out of the cache - the four sides, the block above (unless past the build height, which is
  air), the block below and the four below-sides - rather than re-levelling or draining from
  an unseen side. `below()` answers `B_BEDROCK` (solid, not air) below an unloaded cell, so a
  fall is not re-levelled against air; `flow_into()` refuses an unloaded target (its write
  already did through `world_set`, so the tail waits there too); `slope_dist()` answers -1
  (unknown) on meeting an unloaded cell and spreads it, so a way down that runs out of the
  cache waits instead of reading air as a path. `fluids_tick` still drops an entry whose own
  cell is outside (`tick.c`), which stays safe because `settle_cache` (`world.c`) wakes
  liquids again once the move's chunks are whole. One rule for water and lava (reach 4 vs 2,
  decay 1 vs 2 - both checked).
- **Java has no such edge** (its force-loaded chunks keep the region); this is the game's own
  cache, so the rule is the game's own, as `below()`'s world-floor note is.
- **A player-placed source lives in the edit records** and comes back when the cache does, so
  only the standing stream has to be kept alive in between; a source removed beside a loaded
  stream still drains it as before (the edge does not freeze live flow).
- **Tests.** `tests/unit.c`: a level-2 water flow (and a lava flow) two blocks past the edge
  keeps its level when forced (`fluid_schedule`) while its source is out, and the same with
  the cache moved back; a water fall and a lava fall two blocks past the top keep falling;
  the old seven-out water still dries when its (loaded) source is taken. `tests/play.c`
  gains `--drain`: a high source with its fall to a floor, the cache slid down so the source
  sits above it - the frames show the fall intact with the source out of sight.
- **Recorded.** `./build/play --drain --frames 60 --shots 59 --out /tmp/drain` (**not**
  `build/desktop`): the log reads the source outside (`loaded 0`) with the fall `falling` at
  two heights inside; frame 59 shows the pool the fall keeps on the floor. Without the fix the
  new unit cases fail (the falls drain, the forced flows re-level).
- **Cost.** No new state, no BSS growth: `.data` 5,079 and `.bss` 119,064 (the game's 124,143 B
  inside `CUSTOM_RAM` 133,652); a few `world_loaded` calls a liquid update, the pending queue
  stays `N_PENDING` 160. `NumBlocks.nwa` 617,012 -> **617,092 B** (+80). Recorded: `make test`
  passes (208 recipes, all good); `make build && make check` and
  `make -C games/numblocks-interest build check` fit.

## 1.5.34
**The F3 biome line reads again: the nether shows Hell and the overworld its own biome, instead of Unknown everywhere**

- **Two bugs behind one Unknown.** The overlay read the column's cached biome
  (`vbiome`) only when `world_loaded(bx, 1, bz)` held (`gui.c`) - but the cache
  is 24 blocks tall and follows the player, so y = 1 is outside it whenever the
  player is away from the floor (since 1.3.12 traded the direct `gen_biome` call
  for the cache read): the line fell back to -1, Unknown, almost everywhere. It
  reads `world_loaded(bx, by, bz)` now - the player's own cell, which the cache
  always holds - so a loaded column shows the renderer's own answer again, and
  only a truly unloaded one reads Unknown. And even a loaded nether column read
  Unknown: `biome_name` knew every overworld id but had no case 8 or 9, while
  the nether's `gen_biome` answers 8 (`BI_HELL`) for every column.
- **The names, from 1.8.8's `client.jar`.** Case 8 returns `Hell` and case 9
  `The End`: `ady.class` (BiomeGenBase) lists them in id order (Ocean, Plains,
  Desert, Extreme Hills, Forest, Taiga, Swampland, River, **Hell**, **The End**,
  FrozenOcean ...) and `anp.class` (BiomeGenEnd) carries `The End`. No other
  name moves (`MushroomShore` and the rest stay as they were - out of scope).
- **Recorded.** `./build/play --nether --frames 12 --raws "5:24" --shots 10`
  (**not** `build/desktop`): frame 10 shows `Biome: Hell` under `Dimension:
  Nether`; `./build/play --frames 12 --shots 10` with the overlay left on shows
  `Biome: Plains` under `Dimension: Overworld`. Before, both read Unknown.
- **Cost.** No new state; `.data` 5,079 and `.bss` 119,064 unchanged (the game's
  124,143 B inside `CUSTOM_RAM` 133,652). `NumBlocks.nwa` 616,996 ->
  **617,012 B** (+16: the two names), `numblocks-interest.nwa` 644,332 ->
  **644,276 B** (LTO layout noise the other way). Recorded: `make test` passes
  (208 recipes, all good); `make build && make check` and
  `make -C games/numblocks-interest build check` fit; `tools/emu.py` boots the
  device build (132 frames, 44.0 fps); `build/desktop` rebuilt.

## 1.5.33
**No more water on lava: the water table drops to -50 and its contact with the lava sea crusts to obsidian**

- **Less water, and none of it floating.** The -44 table flooded ten rows; at -50 the
  layer is four rows (-53..-50), so measured water drops from ~2.8% to ~0.7% of the
  deep layer. And round rooms spanning the boundary used to hold a lens of water
  sitting right on the lava. The contact now crusts to obsidian - exactly what the
  fluid tick's own mixing makes of lava touching water (`mix` in tick.c turns a lava
  source beside water to obsidian), so the crust is what the game would have built
  there anyway, deterministically and at once. Checked over 100 chunks: 1,738 contact
  cells crusted, **0 water cells with lava below**. The crust reads only in-window
  cells and lava only ever exists at -54 and below, so it is identical in every slab
  window (the deep-window check still reports 0 differ).
- **Recorded.** The section dumps show water sitting flat at -50 with the obsidian
  rind and the lava below it; `./build/play --caves ... --shots 5,29` shows the dry
  caves above (a cavern with redstone in its walls, then its tunnel). `t_refcmp`
  still 0 differ from y=5 up (the unit case gains the no-water-on-lava check).
- **Cost.** No BSS growth; deepest stack 11,704 B unchanged. The deep window is 4.76 M
  instructions (22.0 ms, from 4.67 M - the crust pass scans the window once) and the
  straddling one 27.61 M (from 27.55 M). `NumBlocks.nwa` 616,792 -> **616,996 B**.
  Recorded: `make test` passes; `make build && make check` and
  `make -C games/numblocks-interest build check` fit; `tools/emu.py` boots the device
  build; `build/desktop` rebuilt.

## 1.5.32
**Fewer caves and honest water: a tenth less cave volume, and the water is a flat layer above the lava sea instead of floating balls**

- **Fewer caves.** Tunnels run 18-27 steps instead of 20-30 and rooms come in two
  chunks in three instead of three in four. Measured air goes from ~12.8% to ~11.2%
  on average over 25 chunks on five seeds - about a tenth less cave to walk.
- **Water as a layer.** The per-column aquifer table (a smooth noise field saying
  which columns were wet) filled isolated pockets while neighbouring tunnels stayed
  dry: balls of water floating in rock. There is a flat water table now,
  `DEEP_WATER_Y` at -44 - ten rows above the lava sea, the way the lava is a flat
  layer at -54. Everything carved below -44 down to the lava is water, everything
  above is dry: pools sit flat in room bottoms and low tunnels run flooded, and no
  water hangs in the rock above the table. Measured water goes from ~0.7% to ~2.8%
  of the deep layer; the per-column table (256 simplex pairs a chunk) is gone, so a
  fully-deep slab window drops from 4.94 M to 4.67 M instructions (21.6 ms).
- **Recorded.** `./build/play --caves --creative --bright 15 --time 18000 --keys
  "6-29:fwd" --frames 30 --shots 5,29` (**not** `build/desktop`): frame 5 a cavern
  with redstone showing in its wall, frame 29 the tunnel past it with more ore -
  and the section dumps show the water sitting flat at -44. `t_refcmp` still 0 differ
  from y=5 up; the deep windows still 0 differ over 144 chunks (the unit case's
  cavern chunk moved to (1, 2)).
- **Cost.** No BSS growth; deepest stack 11,704 B unchanged. The straddling window is
  27.55 M (from 27.81 M). `NumBlocks.nwa` 617,056 -> **616,792 B**. Recorded: `make
  test` passes; `make build && make check` and `make -C games/numblocks-interest
  build check` fit; `tools/emu.py` boots the device build; `build/desktop` rebuilt.

## 1.5.31
**The deep caves are walked, not noised: round rooms and tunnels with circular sections instead of the faceted, glitchy noise shapes**

- **What was wrong.** 1.5.27-1.5.30 carved the caves from noise fields sampled on a
  4-block lattice. That is all this calculator can afford, and it is too coarse for
  2-3-wide tubes: tunnels aliased into beads and gaps, and the thresholded union came
  out as grid-aligned diamonds - glitchy almost-Far-Lands terrain instead of round
  holes. No threshold tuning removes that; the method is wrong for the hardware.
- **The fix.** The deep layer is carved the way its surface caves are, by walking:
  every chunk draws three tunnels (20-30 steps, radius 1.6-2.7, breathing slowly so
  they bulge and narrow), sometimes a room (a single 6-13-wide ellipsoid, squashed
  flat) and one thin vein from a stream of its own, and a chunk is carved by the
  walks of the 5 x 5 chunks around it - a tunnel runs whole across chunk borders and
  comes out the same whichever chunk, and whichever slab window, asks. A rasterized
  sphere is round by construction: rooms are rooms and tunnels are circular in
  section, with no lattice to alias against. The lava sea, the water pools (the
  aquifer table is kept) and the bedrock floor work as before, and the ores still
  come after the caves, onto their walls.
- **Two bugs caught on the way.** The walks first shipped without the world seed in
  their stream, so every world grew the same caves (fixed: the stream mixes `g_seed`,
  like the 1.8 carver's); and a pitch random walk barely leaves its start depth, so
  nothing ever descended to the lava - each walk now has its own persistent slope.
- **Measured.** Air averages ~11-15% over 25 chunks on five seeds (lava ~1%, water
  ~1%), the rock connected everywhere; sections show distinct rooms, tunnels and a
  lava lake with the surface intact. `t_refcmp` still 0 differ from y=5 up; the deep
  windows still 0 differ over 144 chunks (the unit case now checks two chunks'
  windows and a cavern chunk with rock dominant).
- **Recorded.** `./build/play --caves --creative --bright 15 --time 18000 --keys
  "6-29:fwd" --frames 30 --shots 5,29` (**not** `build/desktop`): frame 5 a lit
  cavern with its tunnel, frame 29 inside that tunnel - rock all around, nothing
  floating, nothing grid-aligned.
- **Cost.** A fully-deep slab window is 4.94 M instructions (22.9 ms at 216 MHz, from
  3.92 M - the 5 x 5 replay's walks cost more than the lattice did, but stay far
  below a surface chunk's ~128 ms) and one straddling y=0 27.81 M (from 26.76 M);
  deepest stack 11,704 B unchanged, no BSS growth. `NumBlocks.nwa` 617,676 ->
  **617,056 B** (the lattice code is gone). Recorded: `make test` passes; `make build
  && make check` and `make -C games/numblocks-interest build check` fit; `tools/emu.py`
  boots the device build; `build/desktop` rebuilt.
- **Found, not fixed.** The deep ores' stream (and the nether's quartz/glowstone one)
  never mixed in the world seed either, so those veins sit in the same places in
  every world. Same one-line shape as this fix; left for its own item since it moves
  existing worlds' content.

## 1.5.30
**The deep caves read as caves again: vanilla's own thresholds replace 1.5.29's blown-out ones, so the layer is rock with rooms, tunnels and veins instead of a floating-island mess**

- **What was wrong.** 1.5.29's carve opened ~58% of the deep layer: the spaghetti
  threshold alone (0.26) carved ~45%, the noodle was a single-field *sheet* at 0.10
  (~30%, slicing the rock into slabs) and the cheese rooms (0.19, ~26%) merged into one
  void. Past about a third open the air connects and the rock disconnects, so the
  deepslate broke into floating islands in water and air - the screenshot's mess.
- **The fix, from 26.3.jar's density functions.** The 3D spaghetti carves where
  max(|n1|, |n2|) runs under ~0.08 and the noodle where its two ridge fields' max runs
  under ~0.05: both are thin *tubes*, never sheets. The thresholds are now cheese 0.34
  (rooms, separated by rock), spaghetti 0.10 (winding ~3-wide tunnels) and the noodle as
  a two-field tube at 0.06 (~1-block veins). Measured over 25 chunks on five seeds: air
  averages ~11-22% (was ~58%), the rock is connected everywhere, and a vertical section
  shows distinct rooms, tunnels and veins with the surface intact. The lava sea, the
  water pools and the bedrock floor are unchanged.
- **Tests.** The unit case now checks window-consistency on two chunks (a nearly-solid
  one and a cavern one) and asserts chunk (2, 0) at seed 12345 holds rooms and tunnels
  with rock dominant. `t_refcmp` still 0 / 3,212,800 blocks differ from y=5 up; the deep
  windows still 0 differ over 144 chunks.
- **Recorded.** `./build/play --caves --creative --bright 15 --time 18000 --keys
  "6-29:fwd" --frames 30 --shots 5,29` (**not** `build/desktop`): frame 5 a lit cavern
  with a tunnel leading on, frame 29 inside that tunnel - rock all around, nothing
  floating.
- **Cost.** No BSS growth: `.data` 5,079 and `.bss` 119,064. A fully-deep slab window is
  3.92 M instructions (18.2 ms at 216 MHz, from 3.96 M - fewer cells carved outweighs the
  noodle's second field) and one straddling y=0 26.76 M (from 26.78 M); deepest stack
  11,704 B unchanged. `NumBlocks.nwa` 617,564 -> **617,676 B**. Recorded: `make test`
  passes; `make build && make check` and `make -C games/numblocks-interest build check`
  fit; `tools/emu.py` boots the device build.

## 1.5.29
**The deep caves are far denser: the fields' thresholds open the deepslate into a cavern network (about 58% air) instead of 1.5.27's sparse rooms, so exploring below y=0 finds caves at once**

- **More of everything.** 1.5.27's carve left the deep layer mostly rock - about 24% air on
  average, and as little as 3% in a chunk - so a player exploring below y=0 found almost
  nothing. The cheese threshold drops from 0.32 to 0.19, the spaghetti's from 0.14 to 0.26
  and the noodle's from 0.05 to 0.10, so the same three fields open the layer into big
  caverns, fat winding tunnels and thin veins that meet: about 58% air on average over 25
  chunks (was 24%), and no chunk is mostly solid (the minimum is about 26%, was 3%). The
  lava sea, the water pools and the bedrock floor are unchanged, and so is the surface
  (`t_refcmp` still 0 / 3,212,800 blocks differ in 50 chunks from y=5 up).
- **Cost.** No BSS growth: `.data` 5,079 and `.bss` 119,064. A fully-deep slab window is
  3.96 M instructions (18.3 ms at 216 MHz, from 3.86 M) and one straddling y=0 26.78 M
  (from 26.71 M) - the same noise calls, only more of the cells carved. `NumBlocks.nwa`
  **617,564 B** (unchanged: the thresholds are the same constants). Recorded: `make test`
  passes (the deep-cave case now expects a network); `make build && make check` fits; the
  `./build/play --caves ...` frames show the denser network.

## 1.5.28
**Water evaporates in the nether: a water bucket emptied there places nothing - the fizz and a puff of smoke, and the bucket comes back empty**

- **The bucket.** In the nether (`world_dim == DIM_NETHER`) a water bucket used on a
  replaceable block no longer calls `set_block`: the water is never placed and no level
  calculation runs for it. The bucket still empties into an empty bucket (Creative keeps
  it). Lava buckets are untouched. No `neighbours_changed` runs for an evaporated cell.
- **The tell.** `particles_evaporate` (entity.c) puffs a short, dense burst of grey smoke
  (the spawner's `SMOKE`) from the target cell, since the game has no sound for the fizz.
- **Tests.** `make test` passes; `make build && make check` fits (no BSS growth, no state).

## 1.5.27
**The deepslate is hollowed out: 1.17-1.19's noise caves under y=0 - cheese caverns, spaghetti and noodle tunnels, water pools and a lava sea - carved in a pass of their own, so the 1.8 surface stays exact**

- **The caves.** The 1.8 carver stops at the old floor, so under y=0 the deepslate was
  solid rock. A new pass (`deep_caves`, `gen.c`) carves it the way 1.18 does: a broad
  "cheese" field for the open rooms, two crossing "spaghetti" fields whose intersection is
  a winding tunnel, and a thin "noodle" field. Each is a few perlin octaves over the deep
  rows, sampled on a 4 x 4 x 4 lattice and trilinearly interpolated (as Minecraft
  interpolates its density), drawn from the existing perlin permutation with offsets of
  this world's seed. It is a pure function of a block's world position, so a chunk comes
  out the same in any slab window (checked: 0 blocks differ over 144 chunks across four
  window splits and four seeds), and it draws from nothing 1.8's terrain or population
  streams use.
- **The liquids.** An open deep cell at or below y = -54 (1.18's lava level) becomes the
  lava sea; above it a smooth, low-frequency water table (`deep_water_level`) floods some
  caverns and leaves others dry, so water pools where the noise says. The bedrock floor is
  never carved, and the surface (y >= 0) is untouched.
- **The order.** The caves are carved first and the deepslate ores (`deep_ores`) after, so
  a vein never hangs in a cavern and the ores a cave cut into show on its wall. `t_refcmp`
  still reports **0 / 3,212,800 blocks differ in 50 chunks from y=5 up** (seeds 12345 and
  987654321).
- **The tests, and two tools.** `tests/unit.c` checks a chosen seed's deep chunk holds a
  cavern and that it is the same in three slab windows; `tests/play.c` gains `--caves`
  (the camera in the biggest deep cavern, lit with a little glowstone, facing the most open
  way). `tests/gen/t_slabs` is fixed for the 9-bit slabs (its buffers were 256 bytes a row
  and overflowed; it reads the same blocks either way now). `gen_slab`'s local `H` is
  renamed `Htop` so `tests/gen/arm/bench.py`'s `-DH` no longer collides with it.
- **Recorded.** `./build/play --caves --creative --bright 15 --time 18000 --keys
  "6-29:fwd" --frames 30 --shots 5,29 --out /tmp/caves` (**not** `build/desktop`) shows
  the cavern (frame 5) and a tunnel through it (29).
- **Cost.** No BSS growth: `.data` 5,079 and `.bss` 119,064 (the game's 124,143 B inside
  `CUSTOM_RAM` 133,652). The deep pass costs +0.70 M instructions a fully-deep slab window
  (3.16 -> 3.86 M, 14.6 -> 17.8 ms at 216 MHz) and +0.51 M one straddling y=0 (26.20 ->
  26.71 M), measured with `tests/gen/arm/bench.py --y0 -64 --h 24 --game-flags` before and
  after the pass; the surface windows never run it. `NumBlocks.nwa` 614,188 -> **617,548 B**,
  `numblocks-interest.nwa` 641,140 -> **644,496 B**, the `.bin` 547,508 -> **550,224 B**
  (this machine's toolchain). Recorded: `make test` passes (208 recipes; the new deep-cave
  case); `make build && make check` and `make -C games/numblocks-interest build check` fit;
  `tests/gen/t_refcmp` 0 differ from y=5 up.

## 1.5.26
**A dug shaft keeps its sky light as the cache follows the player down below the surface: the sky top is a signed world y now and lowers through a dug shaft, where before the whole deep column went black just after the deepslate began**

- **The black shaft.** A column's sky top (`vtop`, 1 + its highest opaque y) was a `uint8_t`, and
  `edits_top` could only *raise* it. Once the cache left the surface - right after the deepslate
  began, the player well under y=0 - the column's top read as the surface above, so the light pass
  gave the whole column sky light 0: the shaft went black, the black looked like blocks overhead,
  and breaking more made no difference. `vtop` is a signed world y now (`int16_t`, -64..128), set
  from the ground when the cache holds it and, in `world_set`, to the next opaque block down (or
  the cache's own bottom, all lit) when the top one is dug; `edits_top` also **lowers** the top
  through the changes now, so a column rebuilt from its records keeps a dug shaft open to the sky.
  (`edits_top`'s column range also had to move: since the y is based on the deepslate floor it was
  only looking at y >= 0.)
- **Recorded.** `tests/unit.c` digs a shaft from the open air above the surface straight down past
  the deepslate, leaves the cache and comes back (the column rebuilt from its records), and checks
  every loaded shaft cell is lit; `./build/play --creative --keys "0-560:down,0-560:attack,
  570-900:up"` shows the shaft lit to its bottom instead of black.
- **Cost.** `.data` 5,079 and `.bss` 119,064 (the game's 124,143 B inside `CUSTOM_RAM` 133,652:
  `vtop` stays the same 1600 B, the sky top biased by 64 in a byte rather than widened). The code
  grows a little: `NumBlocks.nwa` 617,304 -> **617,420 B**, `numblocks-interest.nwa`
  643,572 -> **643,660 B**, the `.bin` 546,600 -> **546,712 B**.

## 1.5.25
**The change from stone to deepslate is gradual and random now, each column's own (all deepslate by y = -4) as 1.18 makes it, and a block whose chunk is still to be made can no longer be mined into a drop that then stays**

- **The boundary is no longer a line at 0.** A column's deepslate top is its own, drawn from a
  hash of its x and z and landing anywhere from y = 0 down to y = -4, so the deep layer starts the
  way 1.18's does: a little different from column to column and gradually, and every column is all
  deepslate by y = -4 (the bedrock floor below is unchanged). The 1.8 terrain above stays exact
  (t_refcmp still reports 0 differ from y = 5 up).
- **Mining a chunk that is still to be made.** A column whose chunk the cache has not made yet
  reads as stone - that keeps everything that moves out of it - but it has no block to mine: the
  pick ray now passes such a column over, and breaking and placing refuse it, where before the
  block was "mined" (particles, a drop, the crack) and then stayed, because the write had nowhere
  to go. It was easy to walk into while digging down, where the cache moves a lot and chunks are
  queued. (`world_pending_at`, used by `pick_block`, `break_block` and `place`.)
- **A deep-only slab no longer rebuilds the chunk.** When the window is all deep layer (the player
  is far down), `gen_slab` now leaves the column summaries and biomes behind as well as the deep
  rows, so `gen_top`/`gen_biome` do not each rebuild the whole chunk - caves and population and
  all - a second time; chunks are made sooner.
- **Cost.** No BSS growth: `.data` 5,079 and `.bss` 119,056 (the game's 124,135 B inside
  `CUSTOM_RAM` 133,652). `NumBlocks.nwa` 616,788 -> **617,304 B**, `numblocks-interest.nwa`
  642,960 -> **643,572 B**, the `.bin` 546,192 -> **546,600 B**. Recorded: `make test` passes (the
  new pending-column case); `tests/gen/t_refcmp` 0 differ from y=5 up; a column dump shows the
  random boundary (deepslate tops 0..-4, all deepslate by -4); `tests/test_games.py build/apps
  --game numblocks` and `tests/test_numblocks_progress.py` pass; a scripted dig
  (`./build/play --creative --keys "0-600:down,0-600:attack,610-900:up"`) still reaches the deep
  layer and the shaft holds.

## 1.5.24
**The overworld goes down to -64: a deepslate layer under the surface world with a bedrock floor at the bottom, its seven deepslate ores, and a cache, records and guards that follow it down**

- **The deep layer.** The overworld is y -64..127 now (it was 0..127): deepslate fills y 0 down to
  the floor and bedrock sits on it (about -64..-60), 1.18's min build height. The old bedrock floor
  at y 0..4 is deepslate now - a clean boundary at 0, as the request says - and everything above
  0 is 1.8's terrain, block for block: `tests/gen/t_refcmp` (fixed to read the 9-bit slabs) reports
  **0 / 3,212,800 blocks differ in 50 chunks from y=5 up**. The nether keeps its own 0..127 column
  (below 0 there is air, no deepslate), and superflat worlds keep their layers where the preset
  puts them (below y=0 is air). The deep rows go straight into the slab; the unchanged terrain
  windows read from a byte offset (256 cells x 9 bits = 288 bytes a row), so `col[]` and the
  generator's indices are as before. The five 1.8 bedrock draws a column are still drawn (and
  discarded), so 1.8's random stream above 0 is untouched.
- **The deepslate ores (1.18-style).** Every ore has its deepslate variant - coal, iron, gold,
  redstone, diamond, lapis and emerald - placed by a pass of their own that replaces deepslate only,
  so a stone ore never sits in the deep rock. The pass has its own per-chunk stream (never 1.8's),
  and its ranges are 1.18's deepslate targets in spirit: coal and iron the whole deep layer, gold,
  redstone and diamond the lower half, lapis sparse; emerald, always a mountain ore above y 32 in
  1.18, does not generate below 0 though its block exists. Each variant drops what its stone ore
  drops - iron ore's raw iron, redstone's dust, lapis's four-to-eight dye, and so on - and gives
  its XP ([`block_drops`], the ore-XP block in `player.c`).
- **The cache, the world, the guards.** The block cache is still 24 tall and still slides in 8-block
  steps; only its window's place reaches down to -64 (`nb.h`'s `WORLD_MIN_Y`, `dim_min_y()`). Every
  "y < 0" guard is the dimension's own floor now: placing and `/setblock`/`/summon`, the nether
  portal's frame and dry-spot search, the falling-block drop (sand, gravel, concrete powder) and the
  fluid floor (`tick.c`'s `below`), the light rule that reads below the world, and the player's
  respawn-unstick. `VOID_Y` moves to -128 (64 below the new floor), so a fall out of the world is a
  longer drop; a spectator still flies down into it unharmed.
- **The records (NBW7).** An entry is still one 32-bit word, but the overworld's y is based on the
  new floor - the byte reads 0..191 for y -64..127 - so the world's head is **NBW7**. A world with
  an older head is read and re-encoded once on load (NBW1..4 = the 8-bit-block shape, NBW5 the
  19-bit one, NBW6 the 20-bit one): the same position/block conversion as before, then +64 on the
  overworld's y so it sits on the new floor; the nether, which has no deep layer, is unchanged.
  Old worlds therefore load and re-save losslessly; an edit placed at y -40 survives a save and
  reload (`tests/unit.c`).
- **The blocks.** Nine states appended past the concrete, so no existing block id (and no save)
  moves: `B_DEEPSLATE` (drops itself - the cobbled family is out of scope) and its seven ores, with
  the modern pictures (`deepslate`, `deepslate_top`, `deepslate_<ore>_ore`), each in the Natural
  Blocks tab and under `/give` (`deepslate`, `deepslate_coal_ore`, ...). `B_COUNT` 434 -> 442;
  `tools/blocks.py`/`items.py`, and `data.c`/`data.h`/`names.c` carry them.
- **Cost.** No BSS growth: the same `.data` 5,079 and `.bss` 119,056 (the game's 124,135 B inside
  `CUSTOM_RAM` 133,652); the generator's statics do not grow (the pass has no new statics). The nine
  new textures and eight icons are flash: `NumBlocks.nwa` 611,328 -> **616,788 B**,
  `numblocks-interest.nwa` 637,520 -> **642,960 B**, the `.bin` 541,332 -> **546,192 B**. Recorded:
  `make test` passes (208 recipes; the new deep-edit save/reload case); `make -C games/numblocks
  host desktop build check` and `games/numblocks-interest build check` build; both `.nwa`s fit
  `nwlink nwa-bin --ram-length 133652`; `tests/test_numblocks_progress.py`,
  `tests/test_games.py build/apps --game numblocks` (the head magic check now takes NBW7) and
  `tools/emu.py` pass; `tests/render/arm/bench_render.py --fancy --rain --mobs` runs (deepest stack
  22,192 B). Frames (**not** `build/desktop`): `./build/play --deep --creative --fancy --bright 15
  --frames 10 --shots 9` shows deepslate and its seven ores, and the deep layer itself is checked
  column by column (`deepdump`: stone at y 5, deepslate y 4..0 and down, bedrock at -64, the ores
  below 0).

## 1.5.23
**Skeletons turn to face the player while they are shooting, instead of firing over their shoulder once the player steps outside the 60-degree cone**
- **The bug.** `mob_tick`'s chase steers a hostile's body at the player only while it is
  within 60 degrees of facing them (`mob.c`), the limiter that keeps a walking mob on its
  path and stops it spinning at a goal it cannot reach. A skeleton that had stopped to draw
  (bow range, 10 blocks, in sight) is past that cone the moment the player steps round it, so
  it kept firing with the player over its shoulder.
- **The fix.** Stopped to shoot - exactly the skeleton's `go == false` (in range and in
  sight) - the whole turn is taken, capped at the same 30 degrees a tick the limiter allowed,
  so the skull and the drawn bow come round to face what is being shot at
  (`EntityAIArrowAttack`'s look at its target). Walking, the path still steers it and the
  60-degree rule is untouched for it and every other mob.
- **The body is the head here.** NumBlocks' mobs have no separate head yaw (the look AIs turn
  the whole body), so this is the whole mob turning; the pitch that aims the bow at the
  player's eyes is unchanged. Nothing else moves: a zombie or a spider still steers to its
  path, and the row of `--mobs` frames is byte-identical.
- **Cost.** Three instructions, no new state. `.data` 5,079 and `.bss` 119,056 (the game's
  124,135 B inside `CUSTOM_RAM` 133,652). `NumBlocks.nwa` 611,080 -> **611,328 B**,
  `numblocks-interest.nwa` 637,520 -> **637,616 B**, the `.bin` 541,164 -> **541,332 B**.
  Recorded: `make test` passes (208 recipes; the new case has a skeleton seven blocks ahead
  facing away turn +/-180 within a few ticks and keep firing); `make -C games/numblocks host
  desktop build check` and `games/numblocks-interest build check` build; both `.nwa`s fit
  `nwlink nwa-bin --ram-length 133652`; `tests/test_numblocks_progress.py`,
  `tests/test_games.py build/apps --game numblocks` and `tools/emu.py` pass; the
  `./build/play --skeleton --frames 160 --shots 2,8,20,60,159` run (**not** `build/desktop`)
  shows it looking the other way, turning, and firing at the player.

## 1.5.22
**The eye in lava is Java's dark orange again, reaching twelve blocks, with the far lava traced rather than filled so its grid of sample pixels is gone**
- **Darker and further.** The fog colour goes back to Java's `(0.6, 0.1, 0.0)` = `#991900`
  (1.5.21's bright `#FF6E00` read as washed out), and its reach is far longer here than Java's
  two blocks: from `fog0` 1 to `fog1` 12 (1.5.20 was full at two, 1.5.21 at six), so the sea
  floor and the shore read out to where the fog closes. The sky and plain-face tint and the
  `gui.c` screen wash follow the colour.
- **The grid of dots and the outlines.** A large lava surface seen at a grazing angle under lava
  came out as a regular grid of sample pixels over the far lava, with a jagged outline at the
  steps: the adaptive renderer's fills each draw the surface the 4 x 4 samples agreed on, and a
  nearby surface between the samples slips through - hidden by the ordinary fog (whose partial
  band is a quarter as wide), but not by lava's. With the eye in lava the fills are now skipped
  and the textured pixels are traced instead (`render.c`: `under_liq == 2` in `agreed` and at
  the three fill sites), so the view is exactly what a per-pixel trace gives - no grid, and what
  is too far away is the fog. Everything else is untouched.
- **Cost, and the rest unchanged.** Under lava a frame traces more (the sea scene's steps +55%,
  1.05 -> 1.34 ms a host frame), the price of the correct picture there; nothing of the game
  outside lava moves (the whole 18-frame `bench_render.py` set is still checksum-identical to
  1.5.19 in Fast and Fancy, `85.92 / 112.80 M insns`, from 85.67 / 112.43 - the one `under_liq`
  test). `.data` 5,079 and `.bss` 119,056, the game's 124,135 B inside `CUSTOM_RAM` 133,652.
  `NumBlocks.nwa` 611,024 -> **611,080 B**, `numblocks-interest.nwa` 637,408 -> **637,520 B**,
  the `.bin` 541,196 -> **541,164 B**. Recorded: `make test` passes (208 recipes);
  `make -C games/numblocks host desktop build check` and `games/numblocks-interest build check`
  build; both `.nwa`s fit `nwlink nwa-bin --ram-length 133652`;
  `tests/test_numblocks_progress.py`, `tests/test_games.py build/apps --game numblocks` and
  `tools/emu.py` pass; the `./build/play --lava --nether --frames 90 --keys "10-40:sneak"
  --shots 2,45` run (**not** `build/desktop`) shows the surface and the darker, further inside.

## 1.5.21
**The eye in lava is brighter and sees further: a bright orange fog reaching six blocks, where it was Java's dark #991900 full at two**
- **Brighter.** The lava fog colour goes from Java's `(0.6, 0.1, 0.0)` = `#991900` to `#FF6E00`
  (255, 110, 0); the sky and plain-face tint and the `gui.c` screen wash follow it. Java's own
  colour reads nearly black-orange on the calculator's screen - the whole view is one dark
  wash - so it is not kept.
- **Further.** The fog runs from 1 to 6 blocks (`fog0`/`fog1`; it was 0 to 2): near surfaces are
  crisp and the fog is full at six blocks instead of two, so the sea floor and the shore read
  through it and the eye can tell where it is. `render.c`'s comment says what was kept from
  Java and what was overridden, and why.
- **Nothing else moves.** No new state; where the eye is not in lava the pixels are unchanged
  (the whole 18-frame `bench_render.py` set is still checksum-identical to 1.5.19, `85.80 M
  insns`, deepest stack 21,200 B). `.data` 5,079 and `.bss` 119,056, the game's 124,135 B inside
  `CUSTOM_RAM` 133,652. `NumBlocks.nwa` 611,000 -> **611,024 B**, `numblocks-interest.nwa`
  637,384 -> **637,408 B**, the `.bin` 541,172 -> **541,196 B**. Recorded: `make test` passes
  (208 recipes); `make -C games/numblocks host desktop build check` and
  `games/numblocks-interest build check` build; the `./build/play --lava --nether --frames 90
  --keys "10-40:sneak" --shots 2,45` run (**not** `build/desktop`) shows the brighter, wider
  view.

## 1.5.20
**The eye in lava reads as lava: Java's dense orange fog and the orange screen wash, where before it looked like the eye beside it**
- **The state.** `render.c`'s single `under_water` flag becomes one `under_liq` (0 air, 1 water, 2
  lava), set from the camera's own cell with `is_lava` (any state, so a flowing cell counts too)
  after `is_water`, lava winning if a cell could be both. It is the whole of the new state: the
  hot sky paths test the one value once, so nothing of the picture where the eye is not in lava
  moves or slows.
- **The fog.** Java's `FogRenderer.getFogColor` gives the eye in lava the colour
  `(0.6, 0.1, 0.0)` = `#991900`, and `setupColor` puts its fog near full within a block or two -
  much shorter than water's nine. The override sits after both dimensions' fog, so it holds in
  the nether and the overworld alike, as Java's dimension-independent `getFogColor` does.
- **The sky and the plain faces.** Every arm water had gains its lava arm - `sky()`, `sky_px()`,
  the below-horizon face path (`agreed`) and `sky_fill`'s `plain` test - tinted towards that
  orange (half of what was there, half the fog colour, its blue cut), not the water blue, so the
  sky above and the faces the dense fog has not reached read as lava too. Rain is not drawn with
  the eye in a liquid now, lava as well as water.
- **The screen wash.** `gui.c`'s `hud_strip` lays Java's lava overlay (`EntityRenderer`'s
  textured orange, here a flat `tint_rect` the way the hurt flash already is) over the whole
  picture at half opacity, so a wall of blocks right in front reads as lava and not only the fog
  does. `render_under_lava()` is all `gui.c` asks render.c.
- **The recorded run.** `./build/play --lava --nether --frames 90 --keys "10-40:sneak" --shots
  2,45 --out /tmp/lava-nether` (scripted keys, **not** `build/desktop`): frame 2 the sea's
  surface under the camera, frame 45 the eye sunk into it - a dense orange haze with the near
  lava faintly through it and the picture washed orange. `tests/play.c`'s `--lava` header says
  how; the overworld (`--lava`, no `--nether`) shows the same with the sky above tinted.
- **The pixels and the cost.** `.data` 5,079 and `.bss` 119,056, the game's 124,135 B inside
  `CUSTOM_RAM` 133,652. Where the eye is not in lava nothing moves: the whole 18-frame
  `bench_render.py` set is checksum-identical to 1.5.19 in Fast and Fancy, `85.80 / 112.60 M
  insns` (from 85.67 / 112.43 - noise), deepest stack 21,200 B. `NumBlocks.nwa` 610,256 ->
  **611,000 B**, `numblocks-interest.nwa` 636,612 -> **637,384 B**, the `.bin` 540,460 ->
  **541,172 B**. Recorded: `make test` passes (208 recipes); `make -C games/numblocks host
  desktop build check` and `games/numblocks-interest build check` build; both `.nwa`s fit
  `nwlink nwa-bin --ram-length 133652`; `tests/test_numblocks_progress.py` and
  `tests/test_games.py build/apps --game numblocks` pass; `tools/emu.py` runs the app clean.

## 1.5.19
**Fast graphics stops tracing a centre sample the corners already decide: ~3% fewer instructions a frame, the same pixels**
- **CPU.** The 3D view draws each 4 x 4 cell from five traced samples - the four corners and the
  middle - and only a cell whose samples all agree is filled. But when the four corners already
  met the *same face* (kind 1), the middle is decided: a fill reproduces it, and the trace was
  only there to confirm what `render_frame` then throws away. This commit traces the middle on
  demand (`src/render.c`, its sample_at loop is gone from the strip setup) and, **in Fast
  graphics**, skips it entirely when the corners agree on one face:
  - the four corners in one cell (`face_fill`): one block holds them, so its face is the middle
    pixel too - the corner's colour, the fill's depth;
  - the four corners on one plane in different cells (`plane_fill`): a plane, which `plane_fill`
    draws per pixel (and a pixel whose block does not show the plane is still traced) - the same
    per-pixel `face_pixel` result the subdivided path would have drawn.
- **Why Fast only.** In Fancy graphics a leaf's or glass's see-through texels can fall on a texel
  boundary, where the fill's texel and a traced sample can differ by one texel (measured: 44
  pixels across three of the eighteen benchmark frames). In Fast graphics those holes are drawn
  black, so fill and trace agree exactly and every checksum is unchanged; Fancy keeps tracing
  the middle, byte-identical to 1.5.18.
- **Measured (the M7, `bench_render.py`, instruction counts at 216 MHz).** **fast 88.47 -> 85.67
  M insns** (409.6 -> 396.6 ms) and **fast `--rain --mobs` 106.19 -> 103.36 M** (491.6 -> 478.5 ms),
  **-3.2% / -2.7%**; `--fancy` is 112.43 M unchanged (checksums identical) and `--fancy --rain
  --mobs` 163.73 -> 163.75 M (noise, identical). The device (`tools/emu.py`, a fixed
  `-DBENCH=12345` build, 15 s): 389 -> **431 frames**, 25.9 -> **28.7 fps**, 38.6 -> **34.8 ms a
  frame**, slowest 46 -> 43 ms, no violations. Verified against 1.5.18's pixels with the whole
  18-frame set in all four modes and, for the Fast change, at seeds 7 and 424242 as well.
- **RAM and size.** No new state: `.data` 5,079 and `.bss` 119,064 unchanged, the game's RAM
  124,143 B inside `CUSTOM_RAM` 133,652. The code moves the links a little: `NumBlocks.nwa`
  609,712 -> **610,256 B**, `numblocks-interest.nwa` 636,032 -> **636,612 B**, the `.bin`
  540,020 -> **540,460 B** - all still below 1.5.16's. The bench's deepest stack 21,320 B
  (`--fancy --rain --mobs` 22,304).
- Against 1.5.16 the four sets are now **-4.8% / -3.4% / -8.3% / -6.9%** (fast graphics more than
  the required 2-5%).
- Recorded: `make test` passes (208 recipes); `make -C games/numblocks host desktop build check`
  and `games/numblocks-interest build` build (`build/play_ref`, the FULL_TRACE reference, still
  compiles with the shortcut off); both `.nwa`s fit `nwlink nwa-bin --ram-length 133652`;
  `tests/test_numblocks_progress.py build/apps/NumBlocks.nwa` and `tests/test_games.py
  build/apps --game numblocks` pass; the host `./build/play` scenarios run.

## 1.5.18
**Two more pixel loops: a sprite's two pixels a byte, and the cloud's floor in one instruction - another 1% a frame, the same pixels**
- **CPU.** Both changes are in the innermost loops and leave every one of the 18 documented
  `bench_render.py` checksums identical in all four sets, so the picture does not move:
  - **The sprite loop** (`src/gui.c`, `sprite_part`). A sprite is nibble-packed, so the loop
    loaded the same source byte twice, once for the even column and once for the odd. It now
    steps two screen pixels a byte: one load, its low nibble for the even column and its high
    nibble for the odd (a single leading pixel when the crop starts on an odd column, and a
    single trailer when the width is odd). This draws the HUD, the hotbar's items, the hand
    and the menus, all of which are redrawn every frame.
  - **The cloud's floor** (`src/render.c`). The plain sky tests a cloud by flooring two ray
    plane coordinates `(int)floorf(x)` and masking them; on the Cortex-M7 that is a `vrintm`
    and a `vcvt` a coordinate, four instructions a pixel. `floor_i` is the one instruction
    the FPU has for it, `vcvtm.s32.f32` (round towards minus infinity is exactly what
    `(int)floorf` is), under `#if defined(__ARM_FP) && !defined(HOST)`; the host build, which
    has no such instruction, keeps the two-step form. The clouds of a clouded sky were ~0.35%
    of a frame.
- **Measured (the M7, `bench_render.py`, instruction counts at 216 MHz).** Against 1.5.17's
  113.51 / 164.85 / 89.55 / 107.31 M insns, **`--fancy` 113.51 -> 112.43 M** (525.5 -> 520.5 ms),
  **`--fancy --rain --mobs` 164.85 -> 163.73 M** (763.2 -> 758.0 ms), **fast 89.55 -> 88.47 M**
  (414.6 -> 409.6 ms), **fast `--rain --mobs` 107.31 -> 106.19 M** (496.8 -> 491.6 ms) -
  **-1.0% / -0.7% / -1.2% / -1.0%** this commit. Against 1.5.16 that is **-4.8% / -3.4% /
  -5.3% / -4.3%** over the two commits, all with 1.5.16's pixels (all 18 checksums of every
  set). The device (`tools/emu.py`, a fixed `-DBENCH=12345` build, 15 s): 387 -> **389 frames**,
  25.8 -> **25.9 fps**, 38.8 -> **38.6 ms a frame**, slowest 47 -> 46 ms, no violations.
- **RAM and size.** No new state: `.data` 5,079 and `.bss` 119,064 unchanged from 1.5.17 (so
  the game's RAM stays 124,143 B, inside `CUSTOM_RAM` 133,652), and the bench's deepest stack
  stays 21,320 B (`--fancy --rain --mobs` 22,304). The little code moves the links by under
  150 B: `NumBlocks.nwa` 609,568 -> **609,712 B**, `numblocks-interest.nwa` 635,896 ->
  **636,032 B**, the `.bin` 539,884 -> **540,020 B** - all still below 1.5.16's.
- Recorded: `make test` passes (208 recipes); `make -C games/numblocks host desktop build
  check` and `games/numblocks-interest build` build (the host `cc` builds the fallback
  `floor_i`); both `.nwa`s fit `nwlink nwa-bin --ram-length 133652`;
  `tests/test_numblocks_progress.py build/apps/NumBlocks.nwa` and `tests/test_games.py
  build/apps --game numblocks` pass; the host `./build/play` scenarios run.

## 1.5.17
**Three hot-loop shortcuts: the block-cache read loses a multiply, the region map a shift, and a plain cloud a dead sky branch - 4% fewer instructions a frame, the same pixels**
- **CPU.** Three expressions the renderer runs millions of times a frame were made shorter,
  with every one of the 18 documented `bench_render.py` checksums unchanged in all four sets
  (`--fancy`, `--fancy --rain --mobs`, fast, fast `--rain --mobs`) - the picture is
  byte-identical, so nothing below is a visual trade:
  - **The block-cache read** (`src/states.h`). `b9_get`/`b9_set` found cell `i`'s byte at
    `(i >> 3) * 9 + (i & 7)`; the same place is `i + (i >> 3)` (8 cells are 9 bytes, so cell
    `i` starts `i + i / 8` bytes in), which loses the multiply-by-9 and its operand shuffle.
    This accessor is the single most-executed expression in the game: `trace` reads a block
    every cell of every ray. It alone is ~1.5% of the frame.
  - **The region map** (`src/nb.h`, `src/world.c`). `vmac` packed a 4 x 4 x 4 region's two-bit
    kind into a shared byte, so every `vmac_get` shifted and masked; it is now one byte a
    region (600 B, +450 over the 150 B it was; `vmac_set` a plain store). A ray asks it at
    every region crossing, so the lookup became a load.
  - **The plain sky's cloud pixel** (`src/render.c`). `sky_fill` draws a 4 x 4 cell of daytime
    sky by gradient, and where a cloud stands over a pixel it called `sky_px`. But it only
    takes that path for a cell it has already proved *plain* - the sun and the moon cannot be
    over it, `sky_fill`'s own test - so all of `sky_px`'s celestial work (sun, moon, their
    texture lookup) is dead there, and it re-tests the very cloud the caller found. The new
    `sky_px_plain` does the gradient and the cloud blend only, with the same arithmetic, which
    is what made the clouds of a clouded sky cost ~2% less.
- **Measured (the M7, `bench_render.py`, instruction counts at 216 MHz).** `--fancy`
  118.09 -> **113.51 M insns** (546.7 -> 525.5 ms), `--fancy --rain --mobs` 169.47 -> **164.85 M**
  (784.6 -> 763.2 ms), fast 93.38 -> **89.55 M** (432.3 -> 414.6 ms), fast `--rain --mobs`
  110.96 -> **107.31 M** (513.7 -> 496.8 ms): **-3.9% / -2.7% / -4.1% / -3.3%**, the same
  pixels (all 18 checksums of each set match 1.5.16's). The device (`tools/emu.py`, a fixed
  `-DBENCH=12345` build, 15 s): 377 -> **387 frames**, 25.1 -> **25.8 fps**, 39.8 -> **38.8 ms
  a frame**, slowest 48 -> 47 ms, no violations.
- **RAM and size.** `vmac`'s byte a region costs 450 B: `.data` 5,079 unchanged, `.bss`
  118,616 -> **119,064**, the game's RAM 123,695 -> **124,143** - still inside `CUSTOM_RAM`
  (133,652), with the bench's deepest stack 21,316 -> 21,320 B (`--fancy --rain --mobs`
  22,296 -> 22,304). The shorter expressions shrank the links a little: `NumBlocks.nwa`
  610,448 -> **609,568 B**, `numblocks-interest.nwa` 636,784 -> **635,896 B**, the `.bin`
  540,732 -> **539,884 B**.
- Recorded: `make test` passes (208 recipes); `make -C games/numblocks build host desktop`
  builds; `games/numblocks-interest build` and both `.nwa`s `nwlink nwa-bin --ram-length
  133652` fit; `tests/test_numblocks_progress.py build/apps/NumBlocks.nwa` and
  `tests/test_games.py build/apps --game numblocks` pass; the host `./build/play` scenarios
  run and the device build renders without violations.

## 1.5.16
**Smaller and quicker: the download loses 131 KB to one linked piece, the pixel loops gain 5% at O3, and the game's RAM is 3 KB less**
- **File size.** The standalone and Interest links now build as a **single LTO partition** and
  no longer split code and data into per-function/per-data sections (`games/numblocks/Makefile`,
  `games/numblocks-interest/Makefile`): the relocatable `.nwa` then carries almost no
  relocations, and the file the owner downloads drops from **741,964 to 610,448 bytes**
  (-131,516, -17.7%); `numblocks-interest` from 752,224 to 636,784 (-15.4%). `--gc-sections`
  was doing nothing there an LTO dead-code pass does not. The layout change alone also shrank
  the linked `.bin` by ~2 KB and left every frame byte-identical (the 18 documented
  `bench_render.py` checksums unchanged).
- **CPU.** `render.c` now compiles at `-O3` (a file pragma) while the rest of the game stays at
  `-O2`, where the extra inlining bought nothing (measured: one file at O3 equals the whole
  game at O3 in frames, for a twentieth of the code). The pixel loops gain ~5%: the 18
  documented `bench_render.py --fancy --rain --mobs` frames fall 177.66 -> 169.47 M insns
  (the `--fancy` set 125.80 -> 118.09 M, checksums identical), and the device (`tools/emu.py`,
  a fixed `-DBENCH=12345` build, 15 s) 23.7 -> 25.1 fps / 42.1 -> 39.8 ms a frame. It costs
  ~21 KB of code, so the linked `.bin` grows 521,652 -> 540,732 bytes - the only number here
  that goes up, and well inside the app space.
- **RAM.** `zbuf` moved into `render_frame` (the strip helpers take the depth buffer as a
  parameter already, so it was a plain move off the app's RAM and onto the frame's stack) -
  2,560 B; the per-state shape top and liquid kind are one packed byte array (`lshape`)
  instead of two - 512 B; the light tables are `uint8_t` - 384 B; and the tracer keeps the hit
  cell's place and block alongside `hit_i`, so `sample_at` needs none of its three divides.
  `.bss` 121,942 -> 118,616, `.data` 5,073 -> 5,079: the game's RAM 127,015 -> 123,695
  (-3,320, -2.6%). The bench's deepest stack grows 20,016 -> 22,296 B (the moved `zbuf`),
  still under the 32 KB the calculator gives the stack; the RAM freed widens the unused room
  below it, so the margin a stack overrun would eat is ~1.6 KB larger than before.
- **Menus.** `dirt_background` unpacks its 16 x 16 tile once a strip and paints each screen row
  from the table, instead of a divide and a palette read a pixel on every menu frame.
- Recorded: `make test` passes; `make build && make check` and
  `make -C games/numblocks-interest build check` fit; `tests/test_numblocks_progress.py`,
  `tests/test_games.py --game numblocks` and the host `./build/play` scenarios pass; the 18
  documented `bench_render.py --fancy --rain --mobs` checksums are unchanged
  (`7bb6bf7a ...`) and so are the `--fancy` ones.

## 1.5.15
**Panorama: the fixed overworld and nether of one seed, and the three images gone**
- Personalisation's Panorama offers **Random / World / Overworld / Nether** now. **Random**
  (the default) rolls one of the three each time the title is entered; **World** is the
  world played last with the player where they left off (a seed-0 world where there is
  none, as before); **Overworld** and **Nether** are a fixed world of the game's own -
  seed 123, its own nether - drawn live, the camera at one spot each (138/63/548 in the
  overworld, 4/33/106 in the nether) and turning as the title does. So a fresh install
  shows a world without one having been played, and the nether without walking to a
  portal, and none of it is a stored picture.
- The three image panoramas (1.16/1.20/1.21) are **out**: `src/panorama.c` and
  `panorama.h`, `tools/panorama.py`, the `make data-panorama` target, and the fetched
  1.16/1.20/1.21 dump pictures (18 PNGs) are gone. They were 921,600 bytes of the app
  (the 160 x 160 RGB565 tables); the live views cost nothing, and the whole feature is
  now ~1 KB of code.
- A record from 1.5.13/1.5.14 reads on: pick 2 (was 1.16) is Overworld, 3 (was 1.20) is
  Nether, and 4 (was 1.21) clamps to Random.
- Recorded (the host harness, **not** `build/desktop`): `--panorama 1/2/3` show the World
  (the seed-0 world), the seed-123 overworld (a forest with a sunny hill) and its nether
  (the shore over the lava sea - four yaws checked, open on every side); nine
  `--panorama 0` runs landed World 4, Overworld 4, Nether 1. The app is back to **741,964
  bytes** (1.5.12's 742,884 before any of this) and numblocks-interest 752,224. `make
  test` passes; the bench's 18 `--fancy --rain --mobs` frames are pixel-identical with the
  same deepest stack (20032 B); the device run boots (164 frames, 24.4 ms of work a frame
  behind a world backdrop) and `tests/test_numblocks_progress.py` passes.

## 1.5.14
**Random rolls on the first open now: the host's clock said the same thing every run**
- The title's Random pick mixed `plat_millis()` only: on the calculator that is the real
  uptime, so it rolled as it should, but the host's simulated clock starts at 0 every run
  (tests/play.c drives it), so the desktop player's first title screen always rolled the
  same backdrop - the World - and only the later visits rolled. The pick also mixes
  `plat_entropy()` now: on the host it is the real clock (`plat_host.c`: CLOCK_MONOTONIC's
  nanoseconds mixed with the pid), on the calculator its uptime, so a run's first pick is
  a roll like any other. A run that wants a fixed backdrop passes `--panorama N`, which
  the harness always could.
- Recorded: `./build/play --title --frames 30 --keys "5:ok" --shots 29` twelve times over,
  each with a fresh save dir, landed on World 4, 1.20 3, 1.16 3, 1.21 2 (the shots
  identified against the `--panorama 1/2/3/4` reference frames); the same loop before the
  change was the World every time. `make test` passes; `make build && make check` fits and
  `make -C games/numblocks-interest build check` passes (the `.nwa` is 24 bytes over
  1.5.13's - 1,664,992 against 1,664,968 - the entropy code; no data changed).

## 1.5.13
**Personalisation: High Contrast Block Outlines and the title's panorama, at the most detail the app space holds**
- Options gains a **Personalisation...** row beside **Mods and Cheats...** (both 150 wide
  now, the two side by side as the rows above them), and behind it the list screen the
  next switch will go on: **High Contrast Block Outlines: ON/OFF** and **Panorama:
  Random/World/1.16/1.20/1.21** (OK switches and steps; left and right step the Panorama
  row, which wraps; Done saves). The two option bytes are the last of the bytes `Options`
  had as padding (42 and 43), so the record stays 44 bytes and an older one reads them as
  off and Random; the title's own state is one packed byte (`main.c`), so the feature adds
  1 byte of BSS (121941 -> 121942).
- **High Contrast** draws the block under the crosshair outlined in the reference's own
  cyan, #57FFE1 (the 1.21.2 accessibility option, whose line is that cyan with a small
  black edge - at these one-pixel edges the cyan alone is the readable one), blended at
  24/32 instead of the usual black at 13/32: on a dark cave wall or a night scene the
  usual line is invisible and the cyan one is not, which is the point. With it off the
  frames are byte-identical to 1.5.12's (`--place` and the 120-frame `--mobs --torches
  --rain` run: every pixel the same).
- **Panorama** picks the title's backdrop. The versions are the clients' own title
  cubemaps, fetched from the Minecraft Wiki (1024 x 1024 a face: 1.16 from
  Category:Outdated panorama textures, 1.20 and 1.21 from Category:Panorama textures) by
  `tools/panorama.py --fetch`, which keeps 256 x 256 copies in the texture dump; the pack
  (`make data-panorama`) writes `src/panorama.c`: **six 160 x 160 RGB565 faces a version**
  - a texel a picture pixel, since the picture is 160 x 120 and 86 degrees of a face's 90
  fill its width - 921,600 bytes of tables in all. The faces run 0..5 as the client shoots
  them (0..3 the horizon clockwise south/west/north/east, 4 up, 5 down; `render.c`'s
  `render_panorama` samples them by the ray's direction, exactly the mapping the wiki's
  `/camera` shots define; each face re-rendered from its own camera reproduces the source
  image). The file is a screenshot: no world light, no fog, no clouds, full bright.
- **No world is loaded behind a version's panorama**: `title()` picks first and only
  builds the backdrop world (the last played, else seed 0) for the **World** pick -
  the panorama needs none, and its frames cost the calculator less than the world's do.
  The roll happening on each entry to the title (startup, quit to title, back to title)
  is **Random**'s: it mixes the clock as `parse_seed` does (never the world's `rnd()`
  stream, which the tests replay), so different visits show different ones; a fixed pick
  keeps itself. Changing the pick under the title (Options -> Personalisation -> Done)
  picks again at once.
- **Mods and Cheats** from the *title's* Options says nothing and opens nothing now: with
  no world there is nothing to cheat in, and the chat is the world's (the refusal line,
  "Mods and cheats are only available if you turned cheats on when creating a world", is
  for a world whose cheats are off, where it is there once Options closes, as before).
- Recorded (the host harness, **not** `build/desktop`): `./build/play --title --panorama
  2/3/4 --frames 40 --keys "5:ok" --shots 39 --out /tmp/pan` shows each version's
  panorama behind the menus, and `--panorama 0` rolls one per entry (the same script at
  `--ms-per-frame` 50, 60 and 70 landed on 1.16, 1.21 and 1.16 again - different entries,
  different picks); `--place --contrast` against `--place` shows the hovered block
  outlined in the cyan where the plain run's line is black (and `--place --time 18000
  --bright 0` shows the same in the dark); the Options/Personalisation screenshots show
  the two rows and their labels cycling. The bench's 18 `--fancy --rain --mobs` frames are
  **pixel-identical** (checksums) and the deepest stack is 20032 B; the instruction count
  reads 180.53 M against 177.38 M, which is the link's own shuffling rather than the
  feature: bumping only the version string gives 180.40 M on the 1.5.12 tree, with the
  same pixels. The device run (`tools/emu.py`: 4000 ms, 864.0 M instructions, 234 frames,
  58.5 fps, 17.1 ms of work a frame) is on a title showing a version panorama, whose
  frames cost less than the world backdrop's (the 1.5.12 build in the same harness draws
  155 frames at 25.8 ms behind the same screen). `make test` passes; `make build && make
  check` fits (bss 121942, the one byte above 1.5.12's 121941; text and data as the
  tables say). `tests/test_numblocks_progress.py` passes (`numblocks.cfg` still 44
  bytes) and `make -C games/numblocks-interest build check` passes. The app grows with
  the panoramas: `.nwa` 742,884 -> 1,664,968 bytes (the calculator's app space is
  2,031,616, so 366,648 to spare); numblocks-interest 793,328 -> 1,677,368.

## 1.5.12
**The moving fluids wear the modern client's pictures now - the flow at its own size, and daylight water lit as the air is**
- The four fluid pictures are the modern dump's (`minecraft-textures/`): water_still,
  water_flow, lava_still and lava_flow, packed by `make data-fluids`
  (`tools/data_add.py --replace ...`, which packs them again into the rows
  `src/data.c` already has and the icons of the states that show them, so a later
  full pack writes the same bytes; a second run changes nothing). The two stills
  pack to the bytes that were already in `data.c`, so the real change is the moving
  pair.
- The **flowing** pictures were the wrong size: the flow strip is 32 x 32 a frame
  (two 16 x 16 tiles) and `first_frame` squeezed the whole frame into the game's
  16 x 16, so a falling or spreading block drew the picture at half scale - the
  modern lava read as the same texture "really small" on a block. `pack.py`'s
  `tex_image` now takes one 16 x 16 quarter of the frame at its own texel size,
  which is how Minecraft draws a face of the flow (the strip spans two blocks
  there), so the blobs on a lava fall and a spreading stream are the size the game
  shows them. `FLOW_TEX` names the two, so a later full pack writes the same bytes.
- **Daylight water was drawn at the water cell's own light** - the sky less water's
  opacity, 12 under an open sky, half the light's worth - where the air above the
  surface holds 15: a pool read a good deal darker than the vanilla one. Water
  faces are lit by the brighter of the cell and the cell above it now (cell_hit,
  and water_pixel for the pixels between the samples, so the adaptive picture keeps
  to `play_ref`'s full trace as it did: the --water frames 12/16/20/20 differing
  pixels, the --xray and --concrete ones 0, all as in 1.5.11). Lava glows by itself
  and keeps its own cell's light, so the nether is untouched.
- `tests/play.c` gains `--lava`: a source standing in the air over a cleared field
  (the stream runs down and spreads; Creative, so nothing burns), and `--lava
  --nether`, which looks out over the nether's lava sea from above it.
- Recorded (the host harness, **not** `build/desktop`): `./build/play --water
  --frames 60 --shots 3,20,40,59 --out /tmp/water` has shot 3's source block a
  bright blue with its surface's pattern where 1.5.11 draws a flat slate, and shot
  40's spread surface mean (41, 62, 77) -> (48, 78, 111) over the same field;
  `./build/play --lava --frames 120 --shots 2,10,60,119 --out /tmp/lava` shows the
  falling column and the spreading layers carrying the 16 x 16 quarter (blobs
  twice the size of the squeezed picture) under the still source; `./build/play
  --lava --nether --frames 8 --shots 7 --out /tmp/lava-nether` shows the sea's
  surface over the shore.
- Ordinary views are untouched: the 18 `bench_render.py --fancy --rain --mobs`
  frames are pixel-identical (checksums; 177.38 M insns, the deepest stack 20016 B,
  both the same as before), the documented `./build/play --frames 120 --mobs
  --torches --rain --shots 30,60,90,119` frames are byte-identical, and the nether
  shots are unchanged. `make test` passes; `make build && make check` fits (BSS
  121941 and data 5073 unchanged, text 517482 -> 517610, the .nwa 742724 -> 742884
  bytes, the .bin 520932 -> 521060). The device build runs (`tools/emu.py`: 272
  frames, 45.3 fps, 22.1 ms a frame, no violations), `tests/test_numblocks_progress.py`
  passes (numblocks.cfg still 44 bytes) and `make -C games/numblocks-interest build
  check` passes.

## 1.5.11
**Mods and Cheats: Options gains the screen, and Fullbright**
- Options has a **Mods and Cheats...** row now (under Video Settings/Controls, before Done), and
  behind it the list screen the next switch will go on: one row for now, **Fullbright: ON/OFF**,
  and Done. The screen is the world's: it opens only from a world's Options and with cheats on
  (`cheats_on()`); from anywhere else - a world without them, or the title's Options, even when
  the world the title shows behind was played with cheats and `world_cheats` is still set - the
  press says so in the chat and no screen opens, in the switcher's red: "Mods and cheats are only
  available if you turned cheats on when creating a world". In a world the line is there once
  Options closes, as the chat draws over the world; at the title the chat is never drawn, so the
  button there says it into nothing (the reading the spec left open; chosen because the title has
  no world open to be cheating in, and the button is kept there rather than hidden so the
  refusal is the same answer everywhere).
- Fullbright is a read helper in the renderer (`draw_light`, render.c): while the option is on
  **and** cheats are on, every light the picture is drawn with answers the whole byte, 0xFF (sky
  15 and block 15), as if a level-15 light stood next to every block. It wraps every read the
  picture's light comes from: `face_lit`'s faces, `cell_hit`'s shaped blocks and cross plants,
  `face_pixel`'s and `box_hit`'s faces for the pixels between samples, the water seen through its
  top, the hand (`view_light`), the entities' `dr->light`, the particles, the rain and snow
  sheets, and the spawner's egg. `vl[]` itself is not touched - the light engine, mob spawning,
  burning and the F3 overlay read the same bytes - and the sky's gradient, the fog and
  `setup_light`'s tables stay as they are: a fullbright block is a level-15 block at every
  Brightness stop (Moody and Bright draw the same picture).
- The option is the byte `Options` had as padding (41): the record is still 44 bytes, so no BSS,
  and a record from before reads it as off. A world loaded without cheats keeps a stored
  fullbright and gives it no effect, as asked; it is saved with the other options on Done.
- Recorded (the host harness, not `build/desktop`): `./build/play --cave --cheats --bright 8
  --frames 60 --shots 20,24,28,44 --keys "5:pause,8:down,10:ok,13:down,15:down,17:down,19:down,
  21:ok,26:ok,30:down,32:ok,36:esc,40:pause" --out /tmp/mods` has shot 20 the Options row, 24
  the screen with Fullbright: OFF, 28 ON (the world behind it already lighter), 44 the sealed
  room's every block lit as at 15, where the same camera on the build before is a dark room. The
  same keys without `--cheats` (`--shots 24,36 --keys "...21:ok,26:esc,30:esc"`) leave the screen
  unopened (shot 24 still Options) and show the red line over the world (36); the title's Options
  refuses too even with cheats forced on (`--title --cheats --frames 30 --shots 17,21 --keys
  "2:ok,5:down,7:ok,10:down,12:down,14:down,16:down,18:ok"`, the first ok dismissing the fresh
  harness's key sheet: shot 21 is still the title's Options). Off, the documented frames are
  byte-identical:
  the 18 `bench_render.py --fancy --rain --mobs` checksums match (180.58 -> 180.45 M insns - code
  layout, the branch is not taken; the settle phase 253.86 M insns and the deepest stack 20032 B
  unchanged), and the `--cave`, night and `--frames 120 --mobs --torches --rain` shots are
  byte-for-byte the same as before.
- `make test` passes (a new block: the byte is at 41, is kept with the options, and an older
  record reads off); `make build && make check` fits - BSS 121941 and data 5073 unchanged, text
  515881 -> 517482 (+1601: 325 the row, the screen and its line, 1276 the inlined check at the
  light reads; the .nwa 740564 -> 742724 bytes, the .bin 519332 -> 520932). The device build runs
  (`tools/emu.py`: 690 frames, 32.1 fps, 31.2 ms a frame, slowest 52 ms, no violations), the
  screen opened there with cheats turned on at world creation and Fullbright lit the world,
  `tests/test_numblocks_progress.py` passes (numblocks.cfg still 44 bytes) and
  `games/numblocks-interest` builds (`make -C games/numblocks-interest build check`).

## 1.5.10
**A spectator's head in rock sees the water and the lava the rock hides, not the blocks behind them**
- The xray view (a spectator's head inside solid rock sees the world through it) left out
  any face the ray came into from a full block (`BF_OPAQUE`), so a liquid face the rock
  itself hid - water or lava directly under a block, or against its side - was left out
  too and the view showed what stood behind it: a block-sized square of the sand under a
  pool, the stone behind a lava block. `cell_hit` and the trace loop's skip (render.c)
  ask both sides now: a face is buried, and does not draw, only where the rock is on
  both sides of it; rock against water or lava goes on into the liquid (a top face
  starts the water's see-through layer there, as looking into water does anywhere), and
  rock against glass, leaves, a slab or another shaped block draws that block's face.
  Rock against rock stays as hidden as it was, and a head inside a slab (not a full
  block) still does not start the xray view.
- Recorded: `tests/play.c`'s `--xray` builds a stone-rimmed pool of water with a stone
  block over its middle and a lava block in a stone pocket against the block's side, the
  spectator's head inside the block, looking down at the water below and the lava beside
  it. `./build/play --xray --frames 12 --shots 0,11 --bright 15 --time 6000
  --out /tmp/xray` (the host harness, not `build/desktop`) shows the water surface under
  the block (24,32,41: the shadowed water hue) where 1.5.9 draws the sand (57,52,41),
  and the lava beside it exactly as the same camera sees it with the block gone
  (131,76,16) where 1.5.9 draws the stone wall (106,109,106). The same camera flown out
  of the block (a normal view) is byte-identical between the two builds, and the
  adaptive picture equals the full-trace reference there (0 wrong pixels).
- `make test` passes; the 18 documented `bench_render.py --fancy --rain --mobs` frames
  are pixel-identical (checksums; 177.29 -> 177.40 M insns, +0.06%, code layout - the
  skip is behind `xray` and never runs in those frames, the settle phase 264.21 M insns
  and the deepest stack 20016 B unchanged). BSS 121941, unchanged; the .nwa grows by 68
  bytes of text (515811 -> 515879, 740452 -> 740560 bytes). The device build runs
  (`tools/emu.py`: 155 frames, 38.8 fps, no violations), `tests/test_numblocks_progress.py`
  passes, and `games/numblocks-interest` builds (`make -C games/numblocks-interest build
  check`) - its stale `output/fin.o` was rebuilt, it held bytecode from the older
  toolchain and would not link.

## 1.5.9
**A falling block placed with nothing under it drops at once**
- A placement runs `neighbours_changed` on the cell it placed into, and that ran the fall only
  for the six *neighbours* of the cell: a sand, gravel or concrete powder block set into the
  air (against a wall's side, over a hole) hung where it was put until some change beside it
  came along. The change itself is checked now (`falling`, `fall_if_loose`, `drop_fall`,
  player.c): a falling block the change left loose - a placement, `/setblock`, a flow, a fire -
  drops the moment it is there, exactly as a loose neighbour of a change does.
- The concrete powder keeps its rule: placed beside water it sets where it is (the wet check
  runs first), and a dry powder placed in the air drops and sets where it lands.
- `make test` passes (the new test places sand, gravel and powder into the air the way
  `place()` and `/setblock` do, and checks them on the floor below; the recorded-block test now
  puts its powder on the ground, where the name is what is being checked) and
  `make build && make check` fits (BSS 121941, unchanged; the .nwa grows 24 bytes of text,
  517857 -> 517881). The documented frames are untouched (`bench_render.py --fancy --rain
  --mobs`: the 18 checksums identical; `./build/play --frames 120 --mobs --torches --rain`
  byte-identical), the device build runs (`tools/emu.py`: 780 frames, 37.1 fps, no violations;
  `tests/test_numblocks_progress.py` passes) and `./build/play --place` frames show it: OK at
  frame 25 places the sand against a pillar's side over the air, and the frame after it sits on
  the grass below - the same keys on 1.5.8 leave it hanging in mid-air.

## 1.5.8
**Generation's own blocks get their first update: sand and gravel no longer hang, and water and lava no longer stand, in a chunk that has just been made**
- The generator places blocks; it does not tick them, so a chunk could hold sand and gravel
  hanging over the air a cave carved out from under them, and water and lava standing where a
  cave, a ravine or a spring opened into them, until some change beside them ran
  `neighbours_changed`: a shelf visibly floating, water that would not run. The settle pass
  (`settle_cache`, world.c) is that first update - sand and gravel drop (`settle_fall`,
  player.c: the same straight fall a change gives them, but only where the cache can see the
  ground it lands on, so a fall that would leave the loaded world waits for the cache to move
  there) and a liquid that can move at all - down through an open cell, or sideways into one -
  is scheduled (`fluid_wake`, tick.c) and flows on its own from there.
- It runs once a move of the cache is complete (every chunk whole, so what it reads is what
  the world holds) and over the whole cache: the chunks just made are the ones entering the
  render distance - the cost follows what the player is about to see, as the note asked - and
  the kept ones cost a scan and find nothing. A move's chunks are made one a frame, so it runs
  when the last of them is made, never inside a chunk's own making. Measured
  (`tests/render/arm/bench_render.py --fancy --rain --mobs`): the settle phase 253.20 -> 253.86
  M insns (+0.26%), and the 18 documented frames are pixel-identical (checksums). BSS unchanged
  at 121941; the .nwa grows by 696 bytes of text (517161 -> 517857).
- The bench's own source list was missing `portal.c`, so the documented command could not link
  since the nether landed: added, and `bench_render.py` runs again (that is what the numbers
  above are from).
- Tests: the sand the generator left hanging over a beach's cave pocket is checked to have come
  down onto the ground under it (seed 0 at -286,63,-250); a sand a save brings back in the air
  and a water source with an open cell under it settle the frame the chunk is made again, and
  the water then flows down and out along the floor on its own; the recorded-block tests take
  the whole region now (the settle's drops sit beside their own entries). The dungeon-spawner
  test counts what the spawner itself made - a slot that holds a spider and held none a tick
  ago, with the cap relieved every couple of seconds - on a random stream of its own
  (`rnd_seed`), so where an earlier test left the shared stream cannot move the count.
- `make test` passes and `make build && make check` fits; the device build runs (`tools/emu.py`:
  784 frames, 35.6 fps, 28.1 ms a frame, no violations) and `tests/test_numblocks_progress.py`
  passes; and `./build/play --settle` frames show it: the sand that hung in the air sits on the
  ground and the source that stood still has run down and spread - the same scenario built on
  1.5.7 leaves both standing through the last frame.

## 1.5.7
**Concrete: the powder falls, and the water that touches it sets it, in the sixteen colours**
- The pair is in, all sixteen colours: **concrete powder** falls like sand and gravel and
  stays powder while it is dry, and the moment water - a source or a flow, placed by the
  player or flowing in on its own - touches it, it sets into the **concrete** of its colour,
  which then sits where it set. Powder that falls into water sets where it lands. Both ride
  `neighbours_changed` (player.c): the powder joins the sand and gravel fall list, and a
  wet neighbour (or a wet change beside it) turns it in place; the states run the powders
  first and the concretes after, in the same colour order, so a powder's concrete is its
  state + 16 (`is_concrete_powder` / `concrete_of`, nb.h).
- The 32 states are appended past the quartz family (402..433, `B_COUNT` 434). Concrete is
  modern-only - 1.8 has no id for it - so 1.12's own numeric ids stand in (251 concrete,
  252 concrete powder) and `/give` takes the modern names (`white_concrete`,
  `light_gray_concrete_powder`, ...). The pieces show in Building Blocks and the search
  tab, and are /setblock-able by name.
- The texture list passed 256 with the 32 new pictures, so `blk_tex` and `blk_front` hold
  16-bit indices now (3.5 KB more flash, no RAM). The pictures are the modern dump's
  (`make data-modern`'s set), packed by tools/data_add.py; the labels, the tab and the
  recipe are the tools' tables, `names.py` the /give names.
- The powder crafts the vanilla way: shapeless four sand and four gravel with a dye, eight
  at a time (16 recipes); concrete is not crafted. The powder mines like sand and gravel
  (any tool drops it), the concrete like stone (a pickaxe).
- `make test` passes (the new test falls a column into a pit, sets one from a source and one
  from a flow, walks a set block with water beside it and drops a powder into a pool; the
  recipes, labels, /give names, /setblock and the Building Blocks tab are checked too) and
  `make build && make check` fits (BSS unchanged at 121941). `./build/play --concrete`
  frames show the powder placed, the column falling when the shelf goes, and the stream
  reaching the red block: its speckled face turns the flat concrete.

## 1.5.6
**A hole under the world and over the nether's roof renders right**
- A block over a broken floor block was drawn at the broken block's own depth -
  a ghost standing in the hole's way, one block lower than the real block, which
  you only got past by walking through it. `trace()`'s below-world ray start took
  the plane the ray starts on instead of the next one up: `(y - oy)` where it is
  `(y + by - oy)`, so the first vertical step entered the cell above without
  spending a block of distance.
- A face at the cache's edge lights the way the tracer lights it: `face_pixel()`
  and `plane_fill()` lit a face whose front cell lies outside the cache at 15
  (full sky) where `face_lit` lights it by the face's own cell, which read as a
  darker square with a lighter 2-px outline on the nether's ceiling. Both use the
  tracer's rule now (15 kept only for the world-bottom void face).
- The void's light also reaches the light data: the bottom row of the cache takes
  light from below (`light_box`, world.c), as Minecraft's out-of-world layer
  reads 15, so a hole in the overworld's floor is a lit pit (light 14) instead of
  pitch black. Overworld only - the nether's model has no sky light.
- Ordinary views are untouched: the 18 documented `bench_render.py` frames
  (--fancy --rain --mobs) are byte-identical with and without (checksums), the
  deepest stack 20008 -> 20016 B; the below-world repro (bedrock at y=0, the
  camera in the void below looking up) draws the hole lit, and the adaptive
  picture equals the full trace there (0 wrong pixels). `make test` passes and
  `make build && make check` fits (BSS 121941, unchanged).

## 1.5.5
**The nether's build limit is 200: its roof is open air to build on, not a cap**
- The nether is 200 blocks tall now (the overworld's 128, and the 72 the ceiling
  leaves open above it): blocks place and break above y 127 up to y 199, the cache
  follows the player up there, and what is built stands against the red sky. Its
  terrain is untouched - the ceiling still closes at 123..127 and the rows above it
  generate as air (the window above 128 skips the density work altogether).
- The dimension's own limit is `dim_height()` (nb.h): the nether's 200, the
  overworld's 128. Placement, breaking, `/setblock`, `/summon`, flint and steel,
  the spectator's ceiling and the portal frame's scan all read it, so a portal can
  also be lit above the roof.
- Edit records hold the position in 20 bits now (y is 8 bits under the chunk and
  x, z; the block states moved from bit 19 to bit 20 - four bytes an entry still).
  A world saved before this version re-encodes its records once when it is first
  loaded, and its head is written as NBW6 right away, so a game killed before its
  next save does not convert them a second time; renaming a world converts it the
  same way (the old head's NBW5 said the records were the 19-bit shape, and the
  nether's own could not hold anything above y 127 at all).
- `make test` (the new roof test places at 128 and 199, checks the limit at 200,
  the records, the save and the NBW5 re-encode; the target also clears
  `build/unit-saves` first, so records a build with the old shape left there are
  not read as the current one) and `make build && make check` pass; the host
  `./build/play --nether --roof` frames show a block placed and one mined on the
  ceiling, in Creative, above the red sky.

## 1.5.4
**Glowstone is a lumpy vein up in the ceiling, not a shape, and it is rarer**
- The glowstone is small clusters of one or two little lumps now: each try picks a column's
  highest open ceiling and grows a lump or two of a block or two about it, keeping the cells
  that are open air (the rock keeps the rest). It has no outline to recognise, and the ceiling
  line through it breaks it up. Three tries a chunk where 1.5.3 made ten, so the blocks are
  about 23 a chunk against 1.5.3's 155 (1.8's own are near this, in clusters to match), and
  most veins are a handful of blocks where the ellipsoid read as one solid mass.
- The ceiling each vein hangs from is read off the whole column once, so a vein lands in the
  same place whichever slab window the generator is asked for (before, the window the chunk
  was made in chose where it went, and asking for another window grew another vein).
- Quartz comes in ninety veins of ten to twenty-four blocks a chunk (1.5.3 had sixty-five of
  eight to twenty; 1.8 had about twenty of four to eleven), so much more of it shows in the
  cavern walls.

## 1.5.3
**Glowstone grows as solid balls, new portals land on dry ground, and the quartz family moves in**
- Glowstone is solid clumps now, not trails: an ellipsoid ball a few blocks across, hung so
  its top meets the ceiling and filled only where air. The branching growth read as a spider
  web of thin lines; the balls sit in the open like ore veins. A nether chunk is 1.08 M insns
  / 5.0 ms, back at the pre-clump cost.
- New portals never spawn underwater: the far side goes to the nearest spot in rings out to
  8 blocks whose whole carved volume and floor layer hold no water or lava, scanning upward
  first, so an ocean crossing lands above the water on its obsidian floor. Only loaded ground
  counts (outside the cache reads as air, which would approve spots the build cannot write);
  mid-ocean with no land in the cache falls back to the mapped spot, as before.
- The quartz family's cut blocks: chiseled quartz and the pillar (1.8's 155:1-4), smooth
  quartz (smelted from the block, as modern does) and quartz bricks, all in Building Blocks,
  with their recipes, `/give` names and superflat aliases, textured from minecraft-textures.
- Room for them past the items: block states that live in the inventory cannot share ids with
  the tools at 256 and up (the labels, icons, kinds and stacking all read those as items, and
  the generator's `bflags[256]` was already overflowing), so states 258-395 are `UNUSED_*`
  reserves and the quartz blocks are states 396-401 (`B_COUNT` 402 of the 512 packed ids). The
  item/block helpers take items as 256..256+N_ITEMS-1, and `bflags` is `NSTATES`-sized. Old
  saves are untouched (they hold nothing up there but the never-stacked portal states).
- `tools/data_add.py` works again: it handles the 9-bit-padded tables, emits the new states'
  `TX_` defines (and backfills the 7 missing nether ones), rebuilds the smelting table, and a
  second run is byte-identical.
- `make test` passes (portal pairing, the pool landing, glowstone clumps, quartz crafting,
  smelting, tabs and labels), `make build && make check` fits (BSS 121941 of the 153676 budget).

## 1.5.2
**Portals link in pairs at 1:8 both ways, and glowstone grows in clumps, not staircases**
- Portals pair one-to-one like Minecraft's: both directions map 1:8 (overworld / 8
  to the nether, nether * 8 to the overworld) and arrive at the nearest portal
  around the mapped spot, building a new one where there is none - so a second
  nether portal at 0,10 lands at 0,80 in the overworld instead of coming back
  through the first. The single remembered return position is gone (old saves
  still load; their stored return is simply ignored). The search covers the
  loaded 40 x 40 x 24 cache (~20 blocks out: vanilla looks 128, which does not
  fit), and arriving at a found portal recenters the cache on it.
- Glowstone grows the vanilla way (WorldGenGlowStone1): a starter under rock,
  then random offsets around it with a block only on air with exactly one
  glowstone neighbour, so it branches into hanging clumps. Before it was a
  random walk stepping sideways and down every block - a diagonal staircase
  whose blocks never face-touched. A nether chunk is 1.64 M insns / 7.6 ms
  (was 1.07 M / 4.9 ms), still inside the 5-15 ms budget.
- Quartz comes in sixty-five veins of eight to twenty blocks (1.8 had about
  twenty of four to eleven).
- The portal overlay never goes fully opaque: even at the end of the wait the
  world shows through (28/32, almost opaque).

## 1.5.1
**The far portal lands on both sides, and a nether save reloads in the nether**
- The portal the far side builds comes with a floor on both sides now: two obsidian
  under the interior one block out on each side (four in all, only where air was),
  with air kept over them. Before it laid three in a row on the one side in front -
  the middle of three with a stray third - so walking one block forward through a
  floating portal stepped straight off the edge.
- Saving in the nether and leaving stays in the nether: loading put the world back
  in the overworld at the nether's coordinates (world_new restarts in the overworld
  and the saved dimension was set before it, so it was lost), which put the title's
  backdrop and the loaded spawn in the wrong dimension. The saved dimension, return
  position and generator/records are restored after the world is made, and a new
  world starts with no return portal.

## 1.5.0
**The nether opens up and lights up: brighter everywhere, more quartz, no more 4x4 pillars, glowstone you can actually find, and a portal overlay like Minecraft's**
- The nether's block light runs eight levels brighter (lava still stops at 15): no block
  light below 8 anywhere, so the wastes stay readable far from any glow.
- Quartz comes in fifty veins of four to fourteen blocks (1.8 had about twenty of four
  to eleven): two to three times as much, in veins up to fourteen like modern's.
- The 4x4 pillars are gone: the grid-cell classes were written in place over the grid-point
  classes they were still read from, so whole cells read already-written neighbours and
  showed as square pillars of rock in the open and square holes in the rock. They live
  after the point classes now, in the same scratch, so no new RAM.
- Less terrain: the density threshold went from -0.12 to 0.0, so about half the column
  between the lava sea and the ceiling is open air instead of two fifths.
- Glowstone hangs off the ceiling where there is open air under rock: each try scans its
  column down from the top for the highest open ceiling instead of asking one random block,
  then grows its blob in the open only. About thirty blocks a chunk where barely one
  generated before.
- Standing in a portal covers the screen with the portal's own swirl, thicker until you go
  through - the nether-portal texture tiled large, scrolling and wobbling, blended over the
  world by the wait's progress - instead of a flat purple wash.

## 1.4.7
**The portal is thin like Minecraft's, and the nether's blocks are in the right creative tabs**
- The portal is a thin plane now, not an X: two states, NS facing north/south (thin
  in Z, for frames running east-west) and EW facing east/west (thin in X, for frames
  running north-south), lit by the frame's axis. Old single-state portals load as NS.
- The creative inventory: netherrack is in Natural Blocks and Building Blocks, soul
  sand and nether quartz ore are only in Natural Blocks (the ore with the other ores),
  and the block of quartz stays only in Building Blocks.

## 1.4.6
**The nether lit and finished: glowstone that generates, two brighter light, and portals that break**
- Glowstone finally hangs off the ceiling: its blob writer indexed the slab in absolute rows
  (nothing ever matched a 24-row window) and asked for rock *under* it instead of above. The
  quartz veins had the same absolute-rows bug (only y 10..22 ever got ore); both are
  window-relative now.
- The nether's block light runs two levels brighter (lava still stops at 15), so the dark reads
  and the lava sea reaches.
- A portal breaks as one: breaking any of the frame's obsidian pops the whole interior through
  the block update, mining a portal block takes the rest of its interior with it, and explosions
  break portal blocks (they had no blast resistance handling) with the same flood. Portal blocks
  are no longer washed away by liquids either.

## 1.4.5
**The portal finished: a purple overlay while you stand in it, creative goes at once, and the far side's portal always spawns whole**
- Standing in a lit portal purples the screen, worse until you go through (the wait's progress,
  0 to 24/32 over the 80 ticks); creative travels on the first tick, as in Minecraft.
- The far side's portal is carved room first (an air pocket around its place), then given its whole
  obsidian frame even inside solid rock - no more coming out entombed with frameless portal blocks.
  Portal blocks no longer pop on the next block update (they asked the soil plants stand on for) and
  liquids stop at them.
- Fixed along the way: the creative icons' +5 shift (data_add.py moved old item icons but never
  wrote the new sprites), the hang lighting anything but a whole frame (the scan is bounded at
  21 x 21 now), the "portal" superflat alias truncating to air (uint8_t), and the nether return
  position's z lost on save (sizeof on a decayed array).

## 1.4.4
**The nether: netherrack everywhere, caverns, a lava sea and a red sky - and a portal to get there and back**
- A second generator in gen.c: one density over the whole 128-block column (two noise fields on a
  coarse 4 x 8 x 4 cell grid, thresholded, a cell's blocks filled whole when its corners agree),
  netherrack where it is solid, a lava sea filling everything at y <= 31, a rough bedrock floor and
  ceiling, glowstone hanging under the ceiling, the soul sand and gravel patches modern puts around
  the sea and 1.8's twenty quartz veins of four to eleven blocks. No caves, no population, no
  structures, one biome (1.8's Hell, id 8) and no biome layers at all. The octaves are the
  overworld's, reseeded from the world seed mixed with a constant, so the second dimension costs no
  RAM of its own.
- The five new blocks: netherrack, soul sand, nether quartz ore (which drops the quartz item, as
  1.8's does), the block of quartz (crafted from four quartz items) and the nether portal, which
  lights what is near it, stops no light and is walked through.
- The dimension is the world's (world_dim, world_switch_dim): one cache, shared, refilled with the
  other generator when the player crosses, and its own edit records ("nb1r-3_12.nbe" beside
  "nb1n-3_12.nbe"). The save is NBW5: the dimension and the nether's return position; worlds from
  before have their records re-encoded 9-bit once when they load.
- The screen: the nether's red #330808 is both the sky and the fog, closing from 10 blocks out to
  the render distance, with no sun, moon, stars or clouds and no time of day or weather - cheaper
  than the overworld's frame, not dearer. The ticks: no sky light (only the blocks' own light
  spreads), no weather, no snow or ice, no mobs.
- The portal: the obsidian frame lit with flint and steel, standing in it for a few seconds, and
  the 1:8 mapping - with the portal on the far side made where there is none, and a floor to stand
  on when you arrive.
- Measured (bench.py, the M7): a nether chunk is 1.07 M insns, 4.9 ms, where an overworld one is
  25.80 M, 119.4 ms; the overworld's pixel checksums and the generator's are unchanged. The game's
  RAM is 121426 bytes (from 115200), still well inside what the calculator gives.

## 1.4.3
**Block states are 9 bits each now, so there is room for 512 of them: the nether's blocks have somewhere to live**
- The block cache, the chunk slab, the generator's columns and the edit records hold 9 bits a state
  (states.h's b9_get/b9_set over 8-cell runs, which stay byte-aligned: the cache's slides move whole
  multiples of 8 cells, so they are plain memmoves of 9 bytes to 8 cells). B_COUNT still says how many
  states are defined (252); NSTATES says what the tables and the packing are sized for (512), so the
  per-state tables in data.c are padded out to it and the new states go at the end, where the ids stay.
- The edit log's entry is position (19 bits) | block (9 bits) at the top, so a record entry is still
  4 bytes; the records a world saved before this (NBW1..4) are re-encoded 9-bit when it is loaded.
- The RAM the game uses went from 115200 to 121332 bytes (the cache 38400 to 43204, the chunk slab 6144
  to 6916, the generator's column buffer and its one-row slab, and render's liquid[]/shape_top[] at
  NSTATES): about 6 KB more, still well under the 153676 the calculator gives. The frames cost 2.9%
  more (bench_render.py: 449.3 to 462.5 ms for its 18 frames), and their pixel checksums are identical,
  as are the generator's checksums and spawn point.

## 1.4.2
**Dungeons are ten times as many as 1.3.17's, and about one chunk in six has a sealed room of its own**
- Ninety-six dungeon tries a chunk where Minecraft makes eight and 1.3.17 made thirty-two: three
  and a third times as many dungeons with a way in, ten times as many as 1.3.17 gave and twenty
  times what the game had before 1.3.16. A monster spawner is the one thing a dungeon has that
  nothing else makes, and they were still too rare to find by flying or digging.
- The first try of a population makes its dungeon even when no cave opens onto it: the floor and
  the ceiling still have to be solid ground, so what it makes is a sealed room the ground keeps,
  with its chest and its spawner like any other and no way in but digging. About one try in six
  is geometrically valid, so about one chunk in six has one; the other ninety-five tries still
  ask for WorldGenDungeons's one to five openings onto a cave, which is where a dungeon you can
  walk into comes from.
- The population's memo keeps a bit for each of the ninety-six tries (twelve bytes where
  thirty-two bits had held thirty-two), so a population replayed by the chunks it writes into
  still makes the same dungeons whichever order the world is walked in.
- The world only exists in a 24-block-tall slice around the player, and dungeons live from the
  bedrock to the surface, so a flight through the rock at one height meets the rooms at that
  height: with this many of them there is one every few chunks to meet.

## 1.4.1
**The world generator's summary cache left the RAM it used to sit in for the stack, so NumBlocks needs 13 KB less of it and fits where it did not**
- The summary cache (`sumc`, 25 entries of a chunk's column summaries) was a `static SumEntry[NSUM]`
  in `.bss`, 13 KB of the app's RAM. It is a pointer now, pointed at a buffer on `main`'s stack
  before `plat_begin()` (`gen_set_cache`, `games/numblocks/src/main.c`), and the calculator is known
  to give apps 32 KB of stack apart from their RAM.
- NumBlocks' RAM went from 133340 to 120144 bytes, under the 133652 bytes custom builds of Epsilon
  25.2 give apps. On the calculator `sumc` is that stack buffer; on the host build it stays a static
  array (`#ifdef HOST`), so tests and screenshots are unchanged — the same frames, the same worlds.
- Ported from upstream commit `d5228d5`, which did the same there. The rest of that commit (a strip's
  colours and depths on `render_frame`'s stack) was already here in 1.4.0.

## 1.4.0
**A void opens under the world — fall out of it and it kills you in Survival and Creative alike, a spectator flies down into it unharmed — and the game mode switcher opens on the next mode round its cycle**
- There is a void under the world now, as in Minecraft under its lowest block: what was solid
  rock under `y = 0` is empty, so a player who breaks the bedrock floor out from under
  themselves (Creative breaks it at once) falls past the end of the world and takes the 4
  damage a tick `Entity.checkBelowWorld` gives below 64 blocks under it. Health goes 20, 16,
  12, 8, 4, 0 over the five ticks the 10-tick grace between hits lets through, so it is the
  slow death Minecraft gives rather than the instant one before it. Spectator is immune to
  it (`Player.isInvulnerableTo`) and flies on down; `/kill` kills a spectator, as it does
  there, so it is its own damage kind rather than the void's.
- The pictures come with it: a camera under the world used to see sky whichever way it
  looked, because the rays gave up below the cache's bottom. They come up in through it now,
  as the ones above the cache come down in through its top, so the rock overhead shows, and
  the underside of the world's lowest layer is lit by the void's full sky light, which is the
  default value Minecraft's light layer gives outside the world, so it reads as the lit
  bedrock it is. Under the world the cells are drawn trace by trace rather than from the
  plane their samples agree on: that plane can be the ceiling of a hole a nearer face of
  stands in front of, and only a ray knows which comes first — the fast fills would have drawn
  the rock across the hole's walls. The adaptive picture is checked against `play_ref`'s full
  trace either way: it agrees to the pixel, in the void and in the world above, before and
  after alike (`bench_render.py`'s checksums are unchanged).
- A liquid stops at the world's floor as it does at its top: water on the bedrock spreads
  sideways as it did, rather than draining into the void as Minecraft's does, because there
  is nothing here under the world for it to drain into.
- `sin + cos` opened the switcher on Creative from Survival and on Survival from Creative, so
  from Creative alone the third mode stood one press of cos away, and the other two could not
  reach it in one press. It now opens on the mode after the one you are in, as Minecraft's
  does, so letting go of sin (or OK) takes you round the cycle: Creative, Survival, Spectator,
  Creative. cos still moves the outline one on from there, and Back still leaves nothing
  changed. Tests: the switcher from each of the three modes, and from Survival two cos presses
  round to Survival again.

## 1.3.17
**Dungeons are found two to three times as often as 1.3.16's, and a dungeon is the same whichever way the world is walked**
- The dungeon tries of a population that passed were remembered in eight bits, so the
  ninth and later ones were forgotten: a population replayed by the chunks it writes
  into stopped making their dungeons, and the world came to depend on the order its
  chunks were generated in (the same place held different dungeons, and different
  blocks, walked into from one side than from the other). They are kept in the
  thirty-two bits their number asks for, and the memo says so with a static assert,
  so the tries cannot outgrow it again.
- Thirty-two tries a chunk where 1.3.16 made twenty-four (Minecraft makes eight): two
  to three times as many dungeons as 1.3.16 gave, four or so times what the game had
  before them. A monster spawner is the one thing a dungeon has that nothing else
  makes, and they were too rare to find.
- Two tests that leaned on the world: the hall of the cow that watches the player
  stands in the dark with monsters about it, and a creeper's blast could take the
  floor from under the cow (the test is about the watching, so the world's own
  spawning is off while it runs); and the cache that slides up a step at a time is
  now compared with one made at the destination where the two windows overlap, as
  the climb flies and arriving at once does not yet know to, so they can come to rest
  in different bands without holding anything different.

## 1.3.16
**Dungeons are found twice as often, and a monster spawner's mobs come out about three times as fast**
- WorldGenDungeons is tried eight times a chunk in Minecraft, and eight times a chunk here;
  a dungeon is the one place a monster spawner comes from, and they were too rare to find.
  Twenty-four tries a chunk give about twice as many dungeons (not twice the tries: the ones
  of a population that pass dig the ground out from under the floor and ceiling test of the
  tries after them). The population's memo keeps a bit for each of the twenty-four, so a
  population replayed by the chunks it writes into still makes the same dungeons.
- A spawner near the player tries about once in six a second, where it was once in sixteen:
  the first mob is out within seconds rather than the minute or two it took, and the ones
  after it keep coming (the world still keeps at most six of a kind about one, and the try
  stays the short one the spawners here have always had against MobSpawner's 200 to 800
  ticks, now shorter still).

## 1.3.15
**Brightness comes in at 8, its top stop is a little dimmer, and it sits alone under the Video Settings' pairs**
- The Brightness slider of 1.3.14 comes in at 8, just over the middle of Moody to
  Bright, rather than at 0: a room with no light block in it can be made out
  without opening the settings (Minecraft's own default is half of its slider),
  and 0 is still there at the first stop for the pitch black it has always been.
- 15 gives a little less light than it did: it is what stops 12 and 13 used to
  give (level 0 lifted by 0.375 where the light table's curve puts 0.03).
- It has moved under the two pairs of buttons in Video Settings, as the pairs
  are laid out there: Render Distance beside Graphics, View Bobbing beside
  Clouds, then Brightness on its own row under them, with Done below it.

## 1.3.14
**A spectator flying into a tree sees the world through its leaves, and the Video Settings have a Brightness slider**
- The eyes' own block was drawn as the inside of one face at zero distance, so flying into
  a canopy filled the whole screen with one leaf (and the same in a pane of glass): the
  green flashes a spectator saw with its head inside a tree. A ray that starts inside a
  see-through block now goes on through it, the way Minecraft's back-face culling leaves
  nothing of the block you are in, so you see the world through the leaves you are flying
  through, or the sky above the canopy.
- Brightness is in Video Settings beside the Render Distance and the Graphics buttons, a
  slider of sixteen stops, 0 (Moody) to 15 (Bright): the word at either end where
  Minecraft writes it, the number between them, one stop a press of left or right (or OK,
  which steps it along and wraps round). It is the player's setting, as Minecraft's is,
  and not the world's: the light a block has is the same at every stop (a torch is a torch
  wherever the slider is) and only the dark end of the light table is lifted, so a cave
  with no light in it stays the pitch black it has always been at 0 and can be made out at
  15, while a lit cave and the sky look the same at every stop. Minecraft's own is a gamma
  over every level of its table; here the lift fades away as the light comes up, which
  keeps 0 exactly the game it was and a torch-lit cave unchanged. The setting comes in at
  0: nothing about the game's look changes until the player asks for it.

## 1.3.13
**The control sheet no longer crashes the game when the keys fit on one line**
- The three notes under the key list ("Jump twice: fly", "sin + cos: the game
  mode switcher", "Menus: OK takes or puts") were drawn by a loop that counted
  to five rows instead of to the three notes there are, so as soon as the keys
  above them wrapped onto one line the sheet's fourth line read past the end of
  the list and drew text from whatever followed it in memory. On the calculator
  that was the address 0, and the game stopped there on the first boot — the
  sheet every new player sees before anything else. The loop now stops at the
  list's length.
- The other fix the device build needed to bring the sheet back: nothing else.
  Verified on the device emulator (tools/emu.py): the first-boot sheet, then
  Home quitting on its own, 46 fps, no violations.

## 1.3.12
**Spectator mode: fly through the blocks, see through them, and look at the world through a mob's eyes**
- `/gamemode spectator` (cheats on) is Minecraft's spectator: you always fly,
  through every block, with nothing to touch — no mining, placing, eating or
  opening, no damage, and no mob noticing you, so the world goes on living
  around you. Its whole HUD is the crosshair, and that only while it is on a mob:
  OK then sees the world through that mob's eyes, at its eye height, turning
  where it looks, until alpha brings you back to your own flying body. Flying
  into the ground, the rock around your head turns see-through, so the cave walls
  and the grass above show through the stone.
- sin + cos together is the game mode switcher, as Minecraft's F3 + F4: the modes
  come up in the middle of the screen with the one last used outlined, cos moves
  the outline along them, and letting go of sin (or pressing OK) takes the one
  outlined — Back leaves without changing anything. It needs cheats, as the
  command does; sin on its own still turns the F3 overlay on and off.
- The hand is drawn from a model of its own (`src/hand.c` + `src/hand.h`) put
  together for the frame it is drawn in, on render_frame's stack, instead of the
  fixed animation tables: it follows the swing, the mining progress and the item
  held, and the spectator's empty hand is nothing at all.
- The generator's scratch buffer (7.7 KB) is given to it by its caller and lives
  on that caller's stack — `world_follow`'s for a move, `world_spawn`'s for the
  spawn search — instead of in the app's RAM, and a dungeon's monster spawner is
  placed with the mob its draw made of it, which the world then keeps (as a spawn
  egg on it would have set it), so a dungeon keeps its mob once it is loaded again.
- NumBlocks no longer rides inside NumPlay: `games/games.json` drops it, the
  launcher builds without it, and it ships as `build/apps/NumBlocks.nwa` (its own
  app, "too big to share the calculator with NumPlay", as Champion Island and
  Hollow Knight already do). The interest front that hides NumBlocks behind an
  Interest calculator (`games/numblocks-interest`) and the Interest app itself
  (`games/finance`) build beside it.

## 1.3.11
**The modern names: terracotta, mossy cobblestone, smooth stone, oak planks**
- Every block and item reads the way the modern game names it (todo [6]): the
  inventory and the creative tabs show Oak Planks and Oak Log (not "Oak Wood
  Planks" and "Oak Wood"), Mossy Cobblestone, Smooth Stone and Smooth Stone
  Slab, Petrified Oak Slab, the sixteen Terracottas ("White Terracotta" ..
  "Black Terracotta"), Snow Block beside Snow, Dead Bush, Tall Grass, the Brown
  and Red Mushrooms, Oak Stairs, and the items 1.13 and 1.14 renamed: Wheat
  Seeds, Sugar Cane, Clay Ball, Milk Bucket, White Bed, Melon Slice, Raw Cod,
  Cooked Cod, Tropical Fish and the dyes (Red Dye, Yellow Dye ... Bone Meal).
- The commands name a block state or an item, 1.13's way, so a name carries no
  data value beside it: `/give @p oak_planks`, `/give @p zombie_spawn_egg` (the
  spawn eggs are give-able by name for the first time), `/setblock ~ ~ ~
  orange_terracotta` (its handling word now follows the name directly), and
  `/clear @p red_wool` clears the one item it names. Tab completion offers the
  same names; the numeric ids and the 1.8 ids keep taking what has no single
  modern name.
- The superflat customizer's layer names put each block's modern name first and
  keep its 1.8 spelling as an alias, so a preset saved with the old names
  ("hardenedclay", "whiteclay", "logoak", "snow") still loads and is shown
  modern.
- tools/names.py now writes a third table, `mc_state_name`: the modern name of
  every block state and item `/give` knows. The give keys, the ids, the
  recipes and the saves are untouched, so nothing moves but the names; the
  build grows by 4 KB of flash for the new table.

## 1.3.10
**A dungeon's spawner comes set to a mob: skeletons, zombies and spiders**
- The monster spawner a dungeon is built with now has the mob Minecraft gives it, instead of
  coming out of the ground empty: WorldGenDungeons' own draw (a nextInt(4) — half skeletons,
  a quarter zombies and a quarter spiders, as 1.8 picks them), which the generator now hands
  out rather than throwing away. The world keeps it in its monster spawner record, so the same
  dungeon has the same mob again when the world is loaded and its chunk is made once more.
- Such a spawner shows its egg turning inside the cage and makes its mobs as the game is
  played, exactly as one you have set with a spawn egg does: a dungeon is a working farm from
  the moment you find it. A spawner fresh out of the inventory stays empty until you set it,
  as before.
- The world itself is unchanged: that draw was always taken from the population's stream, and
  the terrain still comes out block for block as it did.
## 1.3.9
**Spectator: fly through the world, and see through a mob's eyes**
- A third game mode, Minecraft's spectator: /gamemode spectator (or sp, or 3),
  and the sin + cos switcher has it as a third slot after Survival, with an eye
  for its icon. It needs cheats, as the other two modes do.
- You fly, always, and through every block: nothing stops you, nothing lands
  you, and nothing you do touches the world - no mining, placing, eating or
  opening, no damage, no mob noticing you (a zombie wanders off instead of
  coming, animals graze). The HUD goes away with it all: no hotbar, hearts,
  hunger, air or experience, no first-person hand, no block outlined, only the
  crosshair - and that only while it is on a mob, as in Minecraft.
- With the crosshair on a mob, OK sees the world through its eyes: the camera
  is the mob's, at its eye height, and turns where it looks, for as long as it
  lives. alpha, the dismount key, brings you back to your own flying body. The
  chat, the commands, the F3 overlay and the save all work as before.
- Flying into the ground, the rock goes see-through around your head, as in
  Minecraft: the faces with nothing solid in front of them show - a cave's
  walls, the grass above - through the stone, until you are out the other side.
  It is the most a frame does: 14 steps a pixel where the surface is 7, and
  only with your head inside a block.
## 1.3.8
**The sprint key alone sprints: forward twice no longer does**
- Pressing forward twice in quick succession no longer starts a sprint: it is
  the i key that sprints, held while walking in Hold, or tapped in Toggle, as
  Options, Controls, Sprint chooses. The 7-tick double-tap window and everything
  it needed (the timer, forward's last state) are gone; the sprint still ends
  when forward is let go, on a wall, on sneaking, while eating or when hungry.
- The key sheet drops the "Forward twice: sprint" note and the "EXE: as OK"
  line, which no longer said anything the sheet's own keys did not. With the
  sheet changed it shows once more on the next start (KEY_SHEET 8).
## 1.3.7
**Hold mode: letting go of the sprint key ends the sprint**
- Sprint: Hold ran on while forward was held, whatever the key was doing: it now ends
  the moment the i key is let go, which is what Hold means. A double-tap of forward
  keeps its own sprint - that one is not the key's to end, so it carries on while
  forward is held, as before.
- Toggle is as it was: a tap switches sprinting on, letting go of the key does not
  end it, and letting go of forward does.
## 1.3.6
**The game mode switcher on sin + cos, as Minecraft's F3 + F4**
- Hold sin and press cos (the key beside it, the two of them next to each other
  like F3 and F4) to switch game mode. It opens the switcher's own panel, as
  Minecraft draws it: the gray bar across the top says which mode is picked
  ("Creative Mode"), the two modes sit in the middle in their slots with
  Minecraft's own icons - the grass block and the iron sword - the one picked
  inside its golden outline, and "[cos] Next" under them says the key that moves
  along the row. The world stays bright behind the panel, as in Minecraft.
- cos marches the outline along the modes while sin is held, the arrows move it
  too, letting go of sin takes the mode outlined, OK takes it straight away, and
  Back leaves without changing anything. It opens on the other mode - in Creative
  on Survival, in Survival on Creative - which is Minecraft's rule of opening on
  the mode you were in last, and with two modes that is always the other one. The
  mode is kept with the world.
- Adventure and Spectator are left out: the first needs "you cannot break a
  block" all through the game, and the second a view that flies through blocks
  and shows no player, which the calculator can only fake. The switcher shows the
  two modes NumBlocks has, so cos just goes back and forth.
- The switcher needs cheats on, as it is the /gamemode command; without them the
  chat says "You do not have permission to use this command", exactly as running
  that command by hand does.
- sin alone still turns the debug overlay on and off, and it now waits until sin
  is let go to do it, so that holding sin and pressing cos never brings the
  overlay up: the pair wins, as F3 + F4 does.
- The control sheet shows the pair and shows again at startup (KEY_SHEET 7), and
  the panel, the slots and the outline are packed from the modern client's
  gui/container/gamemode_switcher.png and gui/sprites/gamemode_switcher, not
  the 1.8.8 jar. The panel's body is 40% see-through, as the texture's own alpha.
## 1.3.5
**Toggle sprint ends with forward; F3 counts the velocity; version entries get titles**
- Sprint: Toggle now ends the sprint when forward is let go, as Hold does: the
  toggle means the i key need not be held while walking, nothing more. Releasing
  forward was keeping the sprint on across a walk, which was wrong.
- The F3 overlay has a velocity line under XYZ: the speed in blocks a second, with
  the three axes with it, so sprinting, swimming and falling can be measured.
- Every entry in versions.md now starts with a one-line title saying what the
  change is, and agents.md asks for one on every change.
## 1.3.4
**The version in full, on the title screen and in F3**
- The version is written in full where it shows: the title screen reads "Numblocks
  custom 1.3.4" and the F3 overlay "Numblock custom 1.3.4 (minecraft.1.8.8)",
  where both used to stop at 1.3. One version in the source now feeds both, so
  they cannot drift from versions.md again.
## 1.3.3
**Controls is a menu of its own, and sprint may be toggled**
- Options, Controls is a menu of its own now, not the key bindings opened at once:
  the key sheet wide at the top, with Sprint and Keybinds side by side under
  it. The key sheet also opens from the key bindings' first row as before, and Done
  on either screen goes one step back out.
- Sprint can be toggled instead of held. Options, Controls, Sprint switches between
  Hold (the i key held while walking, as it was) and Toggle, where a tap switches
  sprinting on and another switches it off. Sneaking, walking into a wall, eating or
  getting hungry still end it either way, as does letting go of forward.
## 1.3.2
**Sprint on the i key**
- Sprint is on the i key, next to forward, where it can be held while walking with the
  other hand. It was on backspace, which is also the key that deletes while you type a
  world's name or a seed, so it got in the way of typing and was a key nobody would
  look for. Keys saved on backspace move to i; keys set by hand stay as they were set,
  and the control sheet shows again now that the keys changed, with i marked Sprint.
  Forward twice still sprints too.
## 1.3.1
**More of everything alive: bigger entity, particle and mob caps, spawners that start empty**
- More of everything alive: the world's entity slots went from 24 to 56 (mobs, dropped
  items, arrows and TNT share them), the particle cap from 16 to 64, and the mobs
  the world may have from 8 to 16 - eight of a kind at most for monsters and eight
  for animals naturally, six of a kind about a spawner, thirty-two mobs in all
  where the spawners keep to what the slots allow. A mob costs 0.9 us a tick to
  keep up, so none of it shows in the frame rate.
- Monster spawners are empty until set: a spawner fresh from the inventory, or one of
  the dungeons', shows no egg and makes no mobs until a spawn egg is used on it, which
  sets it to that mob and starts it making them.
- The egg inside a spawner is the spawn egg item's own picture, flat and turning about
  the cage's centre (a turn in five seconds), no longer a small cube with the sprite
  on every face; it costs less to draw than the cube did.
- A spawner's mobs appear in the air within three blocks of it and fall down unhurt
  (mobs take no fall damage here), in any light: torches do not switch a spawner off.
- A spawn egg used on one sets it quietly: no chat line any more, and a set spawner
  keeps its mob across a save and load (a world record with a sheep or chicken
  spawner used to read back as unset, leaving it empty).
- When a spawner makes a mob, it puffs: smoke about the mob where it appears, and flames
  rising out of the top of the cage for a moment.
## 1.3.0
**Render distance setting (1-2 chunks)**
- Added a render distance setting (1–2 chunks, default 2, saved in the config)
  in Video Settings, as a slider.
- The whole world in sight stays the 40 x 40 x 24 block view cache, fully in
  memory as before — nothing is cached or simplified past it. A chunk here is 8
  blocks, the cache's slide step: 1 -> 11, 2 -> 19 blocks of rays. The fog closes
  where the ground does (the cache's edge, as the camera keeps 16 of its blocks
  to each side), so the ground fades into the sky without a seam at either
  setting, and the rays never walk past the world into nothing.
