# NumBlocks — todo

Work list for `games/numblocks`. Each item: what it is, where the code lives,
what "done" means. Order matters only where noted.

- [ ] **[2] Fix the mob staring bug** — mobs hold their head on the player in
      both survival and creative.
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
