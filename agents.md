# agents.md — NumPlay contributor guide for AI agents

NumPlay is one calculator app (`build/NumPlay.nwa` + disguised variants) containing 17 games, plus each game as its own `.nwa` in `build/apps/`. Target is NumWorks Epsilon (Cortex-M7, EADK via `nwlink`).

**NumBlocks is *not* one of the launcher's games**: `games.json` marks it `"xl": true` and `"default": false`, so it is built only as its own app (`build/data/apps/NumBlocks.nwa`, published as `build/compiled/games/NumBlocks.nwa`), and as `numblocks-interest.nwa` (the Interest finance calculator with a hidden NumBlocks). The Final Earth 2 (`games/finalearth2`) is likewise its own app, brought in from upstream.

## Source of truth

- `games/games.json`: game order (= carousel order = flash block order), `records` (save files), `reset` (files Reset deletes), `shots` (carousel screenshots), `app` (only for STD_GAMES in standard layout — see `tools/games_mk.py:1-24`). Menu text/colors come from here. Generated glue (`build/gen/gametable.c`, `arena.s`, `numplay.ld`, and `build/gen/arena_extra`) comes from `tools/gen_games.py:1-13`.
- `launcher/src/np.h:1-106`: shared contract (`np_game_t`, `np_arena`, storage/config/uninstall API, `NP_MAX_GAMES=32`).
- `launcher/src/main.c:29-60`: app flow (restore/backup progress → Matrices disguise → home → `np_game_run` → backup). `launcher/src/games.c`: installed check via flash head/tail marks, `np_game_run` memcpys `.data` init + zeroes `.bss`, then calls `np_<id>_main`.
- `games/common/epsilon_app.h:1-18`, `games/common/epsilon_files.h:1-17`: every game must include these. They handle Home-hold, RAM clearing, and record-FS access.
- `games/common/np_text.h`: the `T("...")` marker for text players read, and the helpers to draw non-ASCII. Only built into the language variants — see "Languages" below.

## Game module contract (strict — `tests/test_games.py:1-13` enforces it)

Each game builds as relocatable object → `tools/module.ld` merges to one `.text`/`.data`/`.bss` → `tools/npmodule.py:1-22` renames `main` to `np_<game>_main` (only global left), redirects RAM refs to `np_<game>_data/_bss` in shared arena (only one game runs at a time). `eadk_keyboard_scan`/`eadk_event_get` are renamed to `np_*` so launcher sees Home/OnOff (`Makefile:84-89`).

A game MUST:
- Expose `int main(void)`, call `np_app_begin()` at start, `return np_app_end()` at exit (`games/common/epsilon_app.h:68-91`). Inside launcher these are no-ops (`numplay_launcher` weak symbol).
- Quit by itself on Home (Epsilon holds Home back while running). Clear its RAM on exit. Hold + release Home lock exactly once.
- No unaligned multi-word access (M7 fault), no writes to flash or firmware RAM outside the record FS, stack < 30KB (limit 32KB).
- Use `ef_read/ef_write/ef_remove` (`games/common/epsilon_files.h`) for saves. Never cache `epsilon_storage()` pointers across calls.
- Declare every save/level file in `games.json` `records` (supports `prefix*`). `reset` defaults to `records` — set it if editor-made files must survive Reset.
- Limits: app space 2031616B (`Makefile:20`). RAM is checked at three levels, all in the top-level `Makefile`:
  - `RAM_LIMIT` = 148928 (software 23.2) for the NumPlay variants;
  - `CUSTOM_RAM` = 133652 (custom builds of 25.2) for NumPlay's games, and for `BIG_APPS` (Celeste, HollowKnight, ChampionIsland, FinalEarth2, NumBlocks, NumDrive) on their own;
  - `UPSILON_RAM` = 107674 for `NumPlay-Upsilon.nwa`, which carries only the 13 games that fit (`UPSILON_GAMES`).
  The RAM arena is deliberately small (96 KB, `tools/gen_games.py`'s `LAUNCHER_RAM`): a game runs **past** it into whatever the software has left, and a game that does not fit says so on its card (`np_game_ram_missing`) instead of starting. `make check` fails if any level is over.

Standard game layout (e.g. `games/snake/Makefile:1-51`, `games/numvisuals/Makefile:1-51`): `SRC=src/main.c`, targets `build` → `output/<id>.nwa`, `module` → `output/module.o`, `sim-module` → `output/sim-module.o` with `-Dmain=np_<id>_main`. Exceptions: `games/numdash/` (own `node_modules/nwlink@0.0.19`, `NWLINK=node node_modules/...`), `games/tetris/` (Rust, `build.rs` needs `nwlink` on `PATH` — top Makefile passes `NWLINK` env but `tetris/build.rs:8` hardcodes `nwlink`), `games/finalearth2/` (own `data.S`; not in `games.json`, only its own app).

## Launcher/menu (`launcher/src/`)

- `home.c` carousel, `settings.c` settings/Reset/uninstall UI, `games.c` run/uninstall logic, `flash.c` NOR erase (erase→0xFF, program clears bits only), `storage.c` record FS + `numplay_saves.py` backup/restore, `matrices.c` disguise (incl. the Konami code secret), `gfx.c`/`ui.c` drawing, `shots_*` carousel images via `tools/shots.py` (192×144, ≤256 colors, DEFLATE; needs `pngquant` or output grows ~5KB and `check` fails).
- `live.c` the moving carousel backgrounds (and the home clock), `compat.c` what Upsilon lacks, `xfont.c` + `np_xfont.h` the 12-pixel font the language builds draw Chinese with, `platform.c` Epsilon/Upsilon and slow-model detection.

## Languages (French, Chinese)

`tools/lang.py` marks player-facing text with `T("...")` (C) / `T!("...")` (Rust); translations live in `launcher/lang/<code>.txt` and `games/<game>/lang/<code>.txt` (English text, a tab, the translation). `make lang` copies the sources to `build/lang/<code>` with the translations in place and builds `build/NumPlay-French.nwa` / `build/NumPlay-Chinese.nwa` there. `NP_TEXT_EXTRA` is 1 only in those copies, so the English apps carry none of it. `$PY tools/lang.py check fr|zh` lists what is still missing a translation. The 12-pixel font is `assets/fonts/fusion-pixel-12px.bin` (SIL OFL, see `LICENSES/`), subset by `tools/xfont.py` to the letters a build uses.

Games **on their own** are English-only: The Final Earth 2, Celeste, Hollow Knight, Champion Island. NumBlocks is not translated either (see below).

## Build output locations

All build output goes to the **global build folder** at the repository root:

- `/build/` — main output directory (e.g. `build/NumPlay.nwa`)
- `/build/apps/` — individual app `.nwa` files (e.g. `build/apps/NumBlocks.nwa`)
- `games/numblocks/build/desktop` — interactive X11 playtester for NumBlocks (human-only, needs a display; see below)

## Local build/run/test (never touch globals — everything in this folder)

```bash
export PATH="$PWD/.tools/pngquant/usr/bin:$PWD/.tools/xpack-arm-none-eabi-gcc-15.2.1-1.1/bin:$PWD/node_modules/.bin:$PATH"
export PY=$PWD/.venv/bin/python NWLINK=$PWD/node_modules/.bin/nwlink
make -j4 nwa apps check PY="$PY" NWLINK="$NWLINK"   # build/NumPlay*.nwa + build/apps/*.nwa + checks
make -j4 lang PY="$PY" NWLINK="$NWLINK"             # build/NumPlay-French.nwa, NumPlay-Chinese.nwa (slow)
$PY tools/emu.py build/NumPlay.nwa --ms 4000 --shots 3000 --out build/emu
$PY tools/emu.py build/NumPlay.nwa --model n0110 --system upsilon --ms 4000 --out build/emu-upsilon
$PY tests/test_games.py build/apps --game chess            # one game; omit --game for all 22 (slow)
$PY tests/test_uninstall.py build/NumPlay.nwa --game numdrive
$PY tests/test_progress.py build/NumPlay.nwa
$PY tests/test_numblocks_progress.py build/compiled/games/NumBlocks.nwa
$PY tests/test_upsilon.py build/NumPlay-Upsilon.nwa       # no system call Upsilon lacks
$PY tests/test_small_ram.py build/NumPlay.nwa             # NumBlocks says it lacks RAM, NumDash still runs
```

Setup (already done here): `.venv/` (pillow/numpy/zopfli/pyelftools/unicorn==2.1.4), `node_modules/` (`nwlink@1.0.0`), `.tools/` (xpack ARM 15.2.1 + local `pngquant` via `apt download`/`dpkg-deb -x`), `rustup target add thumbv7em-none-eabihf`. `make sim` is macOS-only (`Makefile` hardcodes `aarch64-apple-darwin`) — skip on Linux. The `minecraft-textures/` dump at the repo root is a local asset (like `client.jar`), tracked on purpose.

## Workflow for changes

1. Game logic: edit `games/<id>/src/*.c`. Menu: edit `launcher/src/*.c` (+ `games/games.json` if adding games/text/colors/records). Translations: `tools/lang.py strings <part>` then edit the `lang/<code>.txt` tables.
2. Always build BOTH the `.nwa` and the host test binaries where they exist (an AI must build both, never just one). Rebuild affected output: `make build/NumPlay.nwa` or `make build/apps/<Name>.nwa` with vars above (no `clean` needed). For a standalone game e.g. numblocks: `make -C games/numblocks build host` → `output/numblocks.nwa` + `build/play`, then `make apps` to refresh the `build/apps/` copies. Never run an interactive desktop/X11 binary (`games/numblocks/build/desktop`): it needs a display, never exits on its own, and is for the human owner only — automated verification uses the headless harnesses below.
3. Run: `$PY tools/emu.py <nwa> --ms <ms> --shots <t> --out build/emu` — check `last.png`, `exited`, violations.
4. Test: relevant script above. `test_games.py` plays each build twice in `tools/emu.py`, checks Home-quit, RAM cleared, no violations, save written/updated, `pi.py` untouched, `late.py` tolerated, copy-scripts (`*_saves.py`) hold saves.
5. Keep `git status` clean: local setup (`.venv/`, `node_modules/`, `.tools/`, `package.json`) is excluded via `.git/info/exclude`; game `output/`/`build/` via nested `.gitignore`s. Never edit `.gitignore` for local files, never `npm install -g` / system `pip install` / `sudo apt install`.

## NumBlocks (`games/numblocks`)

NumBlocks is Minecraft on a calculator. It was built from 1.8.8's tables, worldgen and saves (`tools/blocks.py`, `tools/items.py`, `tools/pack.py` over a 1.8.8 `client.jar`); its pictures now come from a modern client (1.17+) through `tools/modern_map.py` (`make data-modern`), with the 1.8.8 jar kept only as the fallback for what a modern client has no equivalent of. The 1.8.8 base is a starting point, not the target: **new behaviour added to the game — gameplay, items, mobs, UI — is written the way the modern game does it (the Minecraft version that introduced it, 1.9 through 1.21), never the 1.8.8 way.** 1.8.8 quirks stay only where the calculator forces them (screen, keys, RAM/BSS, flash); when a modern mechanic is simplified for those reasons, say so in the commit. Copying a 1.8.8 quirk forward into new code is a bug.

### NumBlocks versioning (`games/numblocks/versions.md`)

The version is `1.A.B`; the **first line** of `versions.md` is the current version (`A` = main, `B` = subversion).

- On **every** NumBlocks commit: add `0.0.1` to the version (bump `B`, e.g. `1.3.0` → `1.3.1`), prepend a new entry to the **top** of `versions.md` describing the change, then commit.
- **Every entry starts with a one-line title** (bold, on its own line right under the `## X.Y.Z` header) summing up the change in a sentence — a quick description of what the version is, not a list of the details (those are the bullets below it). This is mandatory on every commit: no entry without a title, and give a title to older entries when you touch them.
- The **owner** decides when to bump the main number `A` in `1.A.B` (`1.3.x` → `1.4.0`, `B` resets to `0`) and will say so — never bump `A` yourself.
- The player-facing version is always the **full** `1.A.B`, never just `1.3`: the homescreen and the F3 debug overlay print it from one macro, `NB_VERSION` in `src/nb.h` — bump that and the **first line** of `versions.md` together on every commit, and keep the two key strings that use it (`src/gui.c`: the title screen's `"Numblocks custom <version>"` and the F3 line `"Numblock custom <version> (minecraft.1.8.8)"`) taking it from the macro, not a copy of the number.

### NumBlocks test binaries: human vs agent

- `build/desktop` (`tests/desktop.c`, `make desktop`) is the HUMAN-ONLY interactive X11 player (real-time window, saves in `build/desktop-saves/`, F12 screenshot). Only the owner runs it. Agents must never launch it (needs `$DISPLAY`, blocks forever, no scripted input). **Building it is part of the job** (`make -C games/numblocks desktop`) — the owner plays that build — but it is never the emulator/tester for a change.
- **Never test a change through `build/desktop`.** The test programs are scripted and take written input:
  - `games/numblocks/build/play` (host build of the game, `tests/play.c`) — key holds by frame range, screenshots, scenario flags:
    ```bash
    cd games/numblocks
    make host && ./build/play --frames 120 --mobs --torches --rain \
      --keys "0-30:fwd,60-90:use,100-140:sprint" --shots 30,60,90,119 --out /tmp/new
    ./build/play --title --frames 60 --shots 59 --out /tmp/title   # the homescreen / title screen
    ./build/play --end --frames 60 --shots 59 --out /tmp/end       # the End (island, dragon, boss bar)
    ```
    `--keys` takes `start-end:key[,key]` (a single frame as `frame:key`); keys: `left right up down fwd back sleft sright jump use attack inv sneak sprint pause ok exe shift esc drop 1..9`.
  - `tools/emu.py` at the repo root — headless Unicorn emulation of the real `.nwa`, keys held over millisecond ranges, for device-level checks (Home-quit, RAM cleared, records written):
    ```bash
    $PY tools/emu.py build/apps/NumBlocks.nwa --ms 4000 --keys "500-600:ok,1000-1100:right" \
      --shots 1000,3000 --out build/emu        # then check last.png, exited, violations
    ```
  - `make test` (`tests/unit.c`) for unit tests, `python3 tests/render/arm/bench_render.py --fancy --rain --mobs /tmp/after` for rendering.
- **Ship both binaries on every change:** the host/desktop build (`make -C games/numblocks host desktop`) *and* the device `.nwa` (`make -C games/numblocks build`, then `make apps` to refresh the top-level copies, plus `make check` — NumBlocks is in `BIG_APPS`, so it is held to `CUSTOM_RAM` = 133652 B).
- Keep `build/desktop` quiet if touched: one startup banner on stdout, errors only. No per-frame or per-event `stderr` logging (a debug `fprintf` there spams every terminal the owner plays in).

## NumBlocks on its own, and numblocks-interest

- `games/numblocks/` is in `games.json` with `"xl": true` and `"default": false`, so it is never a module of NumPlay.nwa. It is built twice: `make -C games/numblocks build` → `build/data/apps/NumBlocks.nwa` (or `build/compiled/games/NumBlocks.nwa` after `tools/build.py`), and `make -C games/numblocks-interest build DIST_DIR=../../build/data/apps` → `build/data/apps/numblocks-interest.nwa`.
- `games/numblocks-interest/Makefile` links NumBlocks' sources (with `-Dmain=numblocks_main`) beside the Interest front (`games/finance/`), which provides `main`, the app name and the icon; the front calls `numblocks_main()`. **Anything you change in `games/numblocks/src/main.c` must still work when `main` is renamed** — it is entered from the front, not from the calculator's launcher.
- Because it is out of `games.json`, NumBlocks is not part of `make lang`, and its texts stay English. `games/numblocks/lang/` exists upstream only — do not add it here.
- `tools/agentctl/` drives isolated NumBlocks agents (spawn → merge → version bump). `games/numblocks-agents/` is its runtime state, excluded via `.git/info/exclude`. The merger commits only the files it wrote, so WIP stays WIP.
