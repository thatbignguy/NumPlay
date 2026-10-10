# NumBlocks — todo

Work list for `games/numblocks`. Each item: what it is, where the code lives,
what "done" means. Order matters only where noted.

- [ ] **[2] Fix the mob staring bug** — mobs hold their head on the player in
      both survival and creative.
- [x] **[4] Sprinting on the `i` key** (1.3.2).
- [x] **[5] F3 + F4 game mode switching on sin + cos** (1.3.6).
- [x] **[6] Old names (1.8.8) still left in the game** — replace them with the
      modern (1.17+) ones: terracotta showing as "stained clay", mossy
      cobblestone as "moss stone", and the rest.
- [ ] **[7] A brightness setting** — 0 (Moody) to 15 (Bright) as a slider in
      the graphics settings: 0 keeps the pitch-black caves, 15 lets you see in
      the dark.

---

## [2] Fix the mob staring bug

**What.** In creative mode — and in survival mode too — mobs stand and stare
straight at the player: head and pitch locked onto you wherever you move, at
any distance they can see, forever.

**Where.** `src/mob.c`, the AI tail of `mob_tick` (~lines 780–870):
`EntityAIWatchClosest`-ish `look` flag and the head turn that follows

```c
if (look && !chase) {
  float turn = wrap(atan2f(-dx, dz) * 57.29578f - e->yaw);
  e->yaw += turn > 10 ? 10 : turn < -10 ? -10 : turn;
  e->pitch = -atan2f(dy + 1.62f - mi->h * 0.85f, dist) * 57.29578f;
}
```

`look` is set on the wander branch (`look = !go && dist < 6 && !pl.dead`) and
on the tempt branch, so a mob that is not walking is glued to the player, and
the same happens while `chase` is false — which is every mob in creative.

**Notes / the work.**
- Vanilla `EntityAIWatchClosest` looks for a *random* length of time, then
  gives up (`maxRandomLookTime`) and `EntityAILookIdle` takes over looking in
  random directions. Reproduce that: a timer per mob, then random idle looks.
- The stare should not depend on game mode: in creative, nothing a mob does
  involves the player as a target, so hostile mobs should idle-wander with
  random looks (and `chase` already requires `pl.mode == 0`).
- Keep the tempt/food look (`EntityAITempt`) — that one is real — but time it
  like vanilla instead of holding it.
- Watch `e->pitch`: it is also the walk/aim pitch used by the models, and it
  decays (`e->pitch *= 0.8f`) only in the `else if (!look)` branch. A pitch
  that never decays is the visible "staring" even when the yaw is fine.
- Also check the `->rx = pitch` head boxes in `mob_parts()` — the head follows
  `e->pitch` directly.

**Done when.** A mob near the player looks at them for a short while, then
looks away and idles with random looks; walking/tempting/chasing still aim
correctly; creative mode has no permanent staring. Show it in
`build/desktop` and in `./build/play` frames.

---

## [4] Sprinting

**What.** Sprinting should be there to use, and be obvious that it exists.

**Where.** `src/player.c` — `player_tick()` (~lines 849–895) already carries
the 1.8 `EntityPlayerSP.onLivingUpdate` rule:

```c
bool fed = pl.food > 6 || pl.mode == 1, went = fwd >= 0.8f * 0.98f;
...
if (pl.sprinting && (!went || hit_wall || !fed)) pl.sprinting = false;
```

with `K_SPRINT` on the **`i` key** (`nb.h:25`; raw key 21 in `gui.c`'s
`actions[]` and `key_name` — the imaginary-unit `i` key by `log`, *not* the
letter `i` on the tan key), the sprint-leap knock at `player.c:891-895`, the
sprint attack knock at `mob.c:358-362`, the flying x2 speed at `player.c:881`,
and `XK_i` mapped to raw key 21 in `tests/desktop.c`.

**Notes / the work.** The plumbing exists; find out what a player actually
misses, and finish that:
- Survival gating: `fed` needs food > 6, so a hungry player silently cannot
  sprint. Decide whether that stays (it is 1.8's rule) and whether the HUD
  says so.
- Does a sprint really survive release-and-re-press of forward, and stop on a
  wall or on sneaking, as the comment promises? Play it on
  `build/desktop` (`i`) and in `./build/play`.
- Feedback: vanilla shows a sprint (fov/perspective shift and footstep
  particles). Say in the commit what was dropped for the calculator.
- README/controls sheet (`gui.c:1834` already lists Sprint) and the key sheet
  version (`KEY_SHEET` in `nb.h`) if the key changes: the starting control
  sheet must show the new key. `actions[]` (`gui.c:1825-1837`) takes
  `{K_SPRINT, 21, "Sprint", "Sprint"}` and `key_name` (`gui.c:1864`) already
  prints key 21 as `i` — the 123-mode name, like `x^2` for strafe right, not
  the letter on the tan key. Bump `KEY_SHEET` (`nb.h:321`) so the game shows
  the sheet again on the next start (`main.c:111`, `gui.c:1662`).

**Done when.** Holding the `i` key sprints on the
ground and doubles creative flight speed, it stops the ways vanilla stops it,
the starting control sheet (re-shown after the `KEY_SHEET` bump) and the README
name the `i` key, and `make host`/`build/desktop` show it.

---

## [5] F3 + F4 game mode switching

**What.** The modern game mode switcher (F3 + F4 in 1.19+): a small menu
over the world, arrows to pick survival/creative/spectator, a
release to confirm. On the calculator there is no F3/F4, so bind it to **sin +
cos** (raw keys 24 and 25, next to each other, like the keyboard) and document
it. `sin` alone already opens the debug overlay (`A_DEBUG`), and pressing
sin + cos must **not** open that overlay — the pair wins.
If you press the buttons, but cheats arent on/you have no op, the chat message (that also appears in minecraft) should appear, if you dont know it search it up.

**Where.**
- The art is already in the tree:
  `minecraft-textures/textures/gui/sprites/gamemode_switcher/slot.png` (the
  per-mode icon) and `.../selection.png` (the highlight). Note `modern_rel()`
  (`modern_map.py`) deliberately returns `None` for `gui/widgets`, `gui/icons`,
  `gui/container` and `particle/particles` only — `gui/sprites/...` passes
  through, so these come from the modern client with no mapping needed.
- `src/gui.c`: a new `GUI_*` screen, drawn over the world (see how the debug
  overlay and the pause menu stay on top), plus the key that opens it.
  `A_DEBUG` (`nb.h:306`, gui.c:1837) is the modern "F3" analogue: raw key 24
  (`sin`) alone. The combo is read from the raw scan (`plat_scan()` /
  `keys_of()`, with `dbg` at `main.c:135`) and must be tested before the
  single-key debug toggle, so sin + cos never opens the overlay.
- `tools/pack.py`: add the two sprites to `GUI_SPRITES`/`gui_images()` so they
  land in `img_*`/`spr_*` like every other GUI picture.
- `src/player.c` / `src/nb.h`: `pl.mode` (0 survival, 1 creative) — switching
  must do what the pause menu's mode change does today (inventory refresh,
  flight off, hunger/health rules), and spectator needs deciding: 1.8 has none,
  and flying through blocks is the only approximation the calculator can give.
  Say what was left out.
- Only with cheats on, like the real one.

**Done when.** The key combo opens the switcher over the world, the arrows
move through the modes, confirming changes the game mode and the mode change
is saved with the world, spectator (or its omission) is documented, the two
sprites come from `minecraft-textures/` (not the 1.8.8 jar), `sin` alone still
opens only the debug overlay, and the starting control sheet (re-shown after
a `KEY_SHEET` bump) and the README say the switcher is sin + cos.

---

## [6] Old (1.8.8) names still left in the game

**What.** Some blocks and items still carry their 1.8.8 names because
NumBlocks' data is taken from the 1.8.8 jar, and the textures/models were
later re-done from the modern client. The names stayed behind: terracotta is
still "stained clay", mossy cobblestone is still "moss stone", and the same
for sugar cane, the dyes, the slabs, snow, the `/give` names and the flat-world
layer names. Replace every one of them with the current name.

**Where.** A name is user-visible in three places; all three need the sweep.

1. The inventory / HUD labels — `tools/items.py`, the `LABELS` dict
   (`items.py:155-198`, written into `src/data.c` by `tools/pack.py`). The old
   ones found so far:

   | label | block | modern name |
   | --- | --- | --- |
   | `Moss Stone` | `MOSSY_COBBLESTONE` | Mossy Cobblestone |
   | `Hardened Clay` | `HARDENED_CLAY` | Terracotta (1.12) |
   | `<col> Stained Clay` | `STAINED_CLAY_*` | `<col> Terracotta` (1.12) |
   | `Sugar Canes` | `REEDS` item | Sugar Cane (1.13) |
   | `Snow`, `Snow` | `SNOW_LAYER` **and** `SNOW` | Snow / Snow Block — two blocks with one label today; the full block (`snow_block`) is "Snow Block" in modern, the layer is "Snow" |
   | `Rose Red` | `DYE_RED` | Red Dye (1.14) |
   | `Cactus Green` | `DYE_GREEN` | Green Dye (1.14) |
   | `Dandelion Yellow` | `DYE_YELLOW` | Yellow Dye (1.14) |
   | `Melon` | `MELON_SLICE` item | Melon Slice |
   | `Bricks Slab` | `BRICK_SLAB` | Brick Slab |
   | `Stone Bricks Slab` | `STONE_BRICK_SLAB` | Stone Brick Slab |
   | `Wooden Slab` | `WOOD_SLAB_OLD` | Oak Slab |
   | `Stone Slab` | `STONE_SLAB_DOUBLE` | Smooth Stone Slab (the 1.8 stone double slab is `smooth_stone_slab` now) |
   | `Oak Wood Planks` | `PLANKS_OAK` | Oak Planks |
   | `Double Tallgrass` | `DOUBLE_GRASS_LOWER` | Tall Grass |

   The rest of `LABELS` is already modern; compare all of it against the modern
   `en_US` anyway (the wiki pages list each block's and item's display name).

2. The command names — `tools/names.py`'s `BLOCK`/`ITEM` lists (1.8.8's
   `Block.blockRegistry` / `Item.itemRegistry`) feed `src/names.c`, which is
   what `/give`, `/setblock`, `/clear` and their completion accept. The
   1.13 "flattening" renamed the ids themselves; these are the old spellings
   in those lists and in `mc_block_name`/`mc_item_name`:

   ```text
   web → cobweb               tallgrass → grass            deadbush → dead_bush
   reeds → sugar_cane         waterlily → lily_pad          brick_block → bricks
   stonebrick → stone_bricks  snow_layer → snow            melon_block → melon
   monster_egg → infested_stone   hardened_clay → terracotta
   stained_hardened_clay → white_terracotta .. black_terracotta (one per colour)
   sapling → oak_sapling      lit_pumpkin → jack_o_lantern
   (also: double_plant / tall_flowers, the slab ids (_SLAB → smooth_stone_slab
   etc.), the per-wood ids (planks → oak_planks …) — look each up)
   ```

   `items.py`'s `RENAME` and the `tab_of`/`ITEM` index check
   (`names.py:78-94`) still key off the 1.8 names while resolving, so change
   the tables but keep the lookup consistent.

3. The flat-world layer names — `src/gen.c`'s `flat_names[]`
   (`gen.c:5479-5530`), both the canonical entries and the aliases, which
   still keep the 1.8 spellings: `"mossstone"`, `"mossycobblestone"`,
   `"stonebrick"`, `"mossystonebricks"`, `"stainedclay"`, `"stainedhardenedclay"`,
   `"hardenedclay"`, `"whiteclay"` … `"blackclay"`, `"brickblock"`,
   `"workbench"`. If old names stay as aliases, say so in the commit; the
   modern ones must be the names shown and matched.

**Notes / the work.**
- Reference list: <https://minecraft.wiki/w/Java_Edition_renamed_features>
  (display-name changes) plus the 1.13 id flattening on each block's page.
  A few sanity checks done ahead: "Moss Stone" is *not* modern (`mob_spawner`
  was "Spawner" from 1.13 to 1.19.3 but is "Monster Spawner" again today, so
  that one stays); "Hardened Clay"/"<col> Stained Clay" became Terracotta in
  1.12; the dyes were renamed in 1.14; "Reeds"/"Sugar Canes" became "Sugar
  Cane" in 1.13; "Block of Snow" became "Snow Block" in 1.13.
- **Do not** rename the texture keys (`blocks.py`'s `tex='tallgrass'`,
  `tex='reeds'`, `tex='waterlily'`, `tex='hardened_clay_stained_*'` …) or the
  left side of `tools/modern_map.py`'s maps: those are the 1.8.8 texture
  filenames on purpose, and `modern_map.py` is what translates them onto the
  modern client's textures. Only the names the player reads or types.
- Mob/entity display names (spawn eggs, `feat_*`) are already modern — a
  final `grep` pass over `tools/` and `src/` for anything left that should
  have been in the sweep is part of the job.
- Watch for collisions while renaming: `SNOW` and `SNOW_LAYER` sharing the
  label "Snow" is one today; the flat-name table's aliases are case- and
  underscore-insensitive (`gen.c:5475`), so `"stonebrick"` and
  `"mossystonebricks"` keys stay unique after the change.
- Regenerate: `make data-modern` (textures/blocks), then `tools/names.py`
  output into `src/names.c`, then the usual `make test` /
  `make host && ./build/play` frames.

**Done when.** Every label in `LABELS`, every name in
`mc_block_name`/`mc_item_name` (so `/give` and the command completion) and
every flat-world layer name is the modern one, the old spellings are gone
from the source (except any kept as deliberate aliases, called out in the
commit), a `grep` over `tools/` and `src/` for the old table above finds
nothing, and `build/desktop` / `build/play` frames show the inventory, a
`/give` completion and a flat world with the new names.

---

## [7] Brightness

**What.** How bright the world is for you. A slider in the graphics settings,
**0 (Moody)** to **15 (Bright)**, the two ends writing the word instead of the
number, the stops between writing the number. 0 to 15 so the setting fits in
four bits.

0 is what the game does today: a cave with no light block is pitch black, and
that stays. 15 is where you can comfortably make out a cave in the dark. It is
a player setting, like Minecraft's — the light the world gives is untouched
(a torch is a torch at any setting), only how much of it reaches the screen.

**Where.**
- `src/nb.h`'s `Options` (`nb.h:325-337`): a `uint8_t brightness` (0..15),
  appended after `sprint_toggle` — `save.c:13`'s
  `Options opt = {2, 0, 100, 1, 1, 0, 0, 0, {0}, 0, 2}` is positional, so that
  initializer grows with the field.
- `src/save.c`: the default there, and `load_options()`
  (`save.c:362-372`) clamping `opt.brightness > 15` the way it clamps
  `opt.distance` (a record written before the field existed is shorter than
  `sizeof opt`, so `memcpy` leaves the field at the default instead of 0).
- The graphics settings are **Video Settings** — `GUI_VIDEO`,
  `gui.c:1608-1629`, opened from Options' "Video Settings…". A new button
  (`B_BRIGHT` in the id list, `gui.c:1254-1258`), a new row in that screen's
  two-column grid.
  - It draws as a slider in `button()` (`gui.c:1685-1708`), which already draws
    `B_DIST` (2 stops) and `B_LOOK` (7 stops) over Minecraft's
    `gui/sprites/widget/slider` track and handle: 16 stops here.
  - Left/right slides it in `gui_input()` (`gui.c:1912-1932`), where `B_DIST`
    and `B_LOOK` slide theirs, and OK steps it by one in `press()`
    (`gui.c:1858` for `B_DIST`).
  - The label: `Brightness: Moody` at 0, `Brightness: Bright` at 15, the number
    between them (Minecraft writes the words at its ends too). `t` is `t[80]`
    (`gui.c:1539`), so the label fits.
  - `B_VIDEO_DONE` (`gui.c:1866`) already calls `save_options()` — the value
    goes into the calculator's `numblocks.cfg` and the desktop's
    `build/desktop-saves` with the rest, nothing to add.
- `src/render.c`, the light table. `setup_light()` (`render.c:2158-2172`) fills
  `shade[4][16]` / `shade_blk[4][16]` (`render.c:80-81`) from Minecraft's gamma
  curve and runs **every frame** (`render.c:2275`), so the setting lands at
  once with no world reload:

  ```c
  float f = 1 - l / 15.0f;
  float bri = (1 - f) / (f * 3 + 1);
  bri = bri * (sun * 0.95f + 0.05f);
  bri = bri * 0.96f + 0.03f;
  for (int s = 0; s < 4; s++) shade[s][l] = (int)(bri * face_shade[s] * 32 + 0.5f);
  ```

  `shade565` (`render.c:411`) multiplies the texel by `s / 32`, so `s == 0` is
  the pitch black — the level-0 row of the table is what brightness lifts, up
  to a readable dark at 15. `light_shade()` (`render.c:422`) keeps taking the
  brighter of sky and block light, so a lit cave looks the same at every
  setting. The sky, its gradient and the fog colour are made elsewhere
  (`sky_grad`, `render.c:2409`; `fog565`, `render.c:2407`) — leave them, and say
  in the commit that the setting is the world's light, not the sky's.

**Notes / the work.**
- The default is a choice to make and to say: 0 (today's game, every new
  player sees exactly what they see now) or the middle (Minecraft's own
  default is 50 of 100).
- Appending the byte grows the one record `plat_save(OPTIONS_REC, &opt,
  sizeof opt)` (`save.c:374`), so `make build && make check`
  (.nwa + BSS <= 153676 B) has to stay inside its budget. If it does not, pack
  the 0..15 into free bits — `opt.distance` only ever holds 1 or 2, so its high
  nibble is free — and say so in the commit.
- The word at 0 and the word at 15 are what the user reads; the number between
  them is the stop. One press of OK, or left/right, moves one stop.
- Also the debug overlay's light line (`gui.c:279`, `llight`) and the F3 lines
  are a natural place to see what a setting did while testing — no extra work
  needed, just what to look at.
- A one-line title at the top of a 1.3.11 entry in `versions.md` (agents.md
  asks for one on every change), like the render-distance one (`versions.md:136`).

**Done when.** Video Settings has a Brightness slider, 0 to 15, the ends
labelled Moody and Bright, left/right sliding it and OK stepping it, the value
clamped on load and saved with the other options; a cave is pitch black at 0
and readable at 15, a cave with torches looks the same at every setting,
`make test` passes, `make build && make check` still fits, and `build/desktop`
and `./build/play --frames … --shots …` frames show the slider at 0, at 15 and
a cave at both.

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

Rules that apply to every item: no new `-Wall -Wextra` warnings, no BSS growth,
keep `git status` clean, and ship both the calculator `.nwa` and the desktop
binary.
