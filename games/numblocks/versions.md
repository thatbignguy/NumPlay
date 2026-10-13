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
