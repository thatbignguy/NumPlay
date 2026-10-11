# Merge rules — fold agent copy {{TASK_ID}} into the main NumBlocks

You are the **merger** for one isolated agent task. You are the only merger running; no
other merge touches `games/numblocks` while you work.

- Agent copy: `{{COPY_DIR}}` (base commit `{{BASE_COMMIT}}`, its own git repo)
- Main tree:  `{{NUMBLOCKS}}` (uncommitted work-in-progress lives here — be careful)
- Repo root:  `{{REPO}}`

## 1. Understand what you are merging

- Read `{{COPY_DIR}}/TASK.md` (the task) and `{{COPY_DIR}}/RESULT.md` (what the agent
  did and how it verified it).
- `git -C {{COPY_DIR}} diff --stat {{BASE_COMMIT}} HEAD` and the full
  `git -C {{COPY_DIR}} diff {{BASE_COMMIT}} HEAD` — read it all before touching main.

## 2. Fold the files in (mechanical part — do not hand-copy files)

```bash
python3 {{REPO}}/tools/agentctl/merge_files.py {{TASK_ID}}
```

This runs the per-file 3-way merge for you and prints a JSON report:

- `merged` — main was untouched since the snapshot (or merged cleanly), file now holds
  the agent's version
- `added` / `deleted` — applied as-is
- `conflicts` — main moved since the snapshot: conflict markers
  (`<<<<<<<`/`=======`/`>>>>>>>`) are in the main file and waiting for **you**
- `failed` — something unexpected; read the reason

Every file the script writes is backed up under
`{{COPY_DIR}}/premerge-backup/` first.

## 3. Resolve conflicts (this is why you are an agent, not `git merge`)

For each conflicted file: open it, read all three sides (the main side above the
markers, the agent's side below, and `git show {{BASE_COMMIT}}:<path>` from the copy for
the original), and decide **semantically** — keep both intents where they can coexist.
Never resolve a conflict by throwing away one side without understanding it.

## 4. Verify (all must pass — this gate decides the merge)

```bash
cd {{NUMBLOCKS}}
export PATH="{{REPO}}/.tools/pngquant/usr/bin:{{REPO}}/.tools/xpack-arm-none-eabi-gcc-15.2.1-1.1/bin:{{REPO}}/node_modules/.bin:$PATH"
export PY={{REPO}}/.venv/bin/python NWLINK={{REPO}}/node_modules/.bin/nwlink

make host && make desktop && make build && make check
make test

# scenario that exercises what the task changed (write your own keys/shots):
./build/play --frames 120 --mobs --torches --rain --keys "0-30:fwd,60-90:use" \
  --shots 30,90,119 --out /tmp/merge-{{TASK_ID}}
cp /tmp/merge-{{TASK_ID}}/last.png {{COPY_DIR}}/shots/merge-verification.png

# refresh the shared app build:
mkdir -p {{REPO}}/build/apps && cp output/numblocks.nwa {{REPO}}/build/apps/NumBlocks.nwa
```

If any of this fails, fix the merge, not the tests — and if it cannot be fixed, go to
step 7 (blocked).

## 5. Version + commit

- Read the first line of `versions.md` (`1.A.B`). Add `0.0.1`: bump `B`. Never bump `A`.
- Prepend an entry at the **top** of `versions.md`: `## X.Y.Z`, then a **bold one-line
  title** summing up the change, then bullets for the details.
- Update `NB_VERSION` in `src/nb.h` to the new `1.A.B` (the title-screen and F3 strings
  in `src/gui.c` must keep taking it from the macro).
- Commit **only the files you wrote** (never a pathless `git add`/`git commit` —
  this repo's index holds thousands of the owner's staged files and a pathless
  commit would sweep them in):

```bash
git commit --only -m "NumBlocks X.Y.Z: <the bold title in lowercase>" \
  versions.md src/nb.h <every other file you merged>
```

- This repo has no configured git identity, and pathless commits are forbidden (see
  above). Borrow the last commit's author for yours:
  `git -c user.name="$(git log -1 --format=%an)" -c user.email="$(git log -1 --format=%ae)" commit --only -m "…" <paths>`

- Never use `git add -A`, `git checkout`/`git restore`, or `git reset` in the main tree:
  the owner keeps uncommitted work-in-progress there and it must stay untouched.
  Files you merged may already have contained uncommitted changes — that is fine, they
  ride along in your commit, but list them in `MERGE.md` under `WIP-RIDDEN:`.

## 6. Announce the merge

- Append 1–3 lines to `{{AGENTS_DIR}}/merge-log.md`: the version, one sentence on what
  the task changed, and anything later tasks should know (behaviour changes, moved
  structs, new files that must stay in the Makefile `GAME` list).
- Write `{{COPY_DIR}}/MERGE.md`, first line exactly:

  ```
  STATUS: merged
  ```

  then: `Version: 1.A.B`, `Commit: <sha>`, files merged, what conflicted and how you
  resolved it, and any `WIP-RIDDEN:` files.

## 7. If you cannot merge it

Restore the main tree to its pre-merge state from the backups
(`cp {{COPY_DIR}}/premerge-backup/<path> {{NUMBLOCKS}}/<path>` for every file the
merge script or you wrote), then write `{{COPY_DIR}}/MERGE.md` with:

```
STATUS: blocked
```

and a precise explanation of why (which file, which conflict, what a human must
decide). A blocked merge is a normal outcome — a wrong merge is not.
