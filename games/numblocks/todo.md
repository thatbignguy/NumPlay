# NumBlocks — todo

Work list for `games/numblocks`. Each item: what it is, where the code lives,
what "done" means. Order matters only where noted.

- [ ] **[2] Fix the mob staring bug** — mobs hold their head on the player in
      both survival and creative.
- [x] **[4] Sprinting on the `i` key** (1.3.2).
- [ ] **[5] F3 + F4 game mode switching on sin + cos** — the modern game mode
      switcher, with its GUI art from `minecraft-textures/`.

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
  `build/desktop` (`i`, double-tap forward) and in `./build/play`.
- Feedback: vanilla shows a sprint (fov/perspective shift and footstep
  particles). Say in the commit what was dropped for the calculator.
- README/controls sheet (`gui.c:1834` already lists Sprint) and the key sheet
  version (`KEY_SHEET` in `nb.h`) if the key changes: the starting control
  sheet must show the new key. `actions[]` (`gui.c:1825-1837`) takes
  `{K_SPRINT, 21, "Sprint", "Sprint"}` and `key_name` (`gui.c:1864`) already
  prints key 21 as `i` — the 123-mode name, like `x^2` for strafe right, not
  the letter on the tan key. Bump `KEY_SHEET` (`nb.h:321`) so the game shows
  the sheet again on the next start (`main.c:111`, `gui.c:1662`).

**Done when.** Holding the `i` key (or double-tapping forward) sprints on the
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
