# agentctl — NumBlocks agent controller

Runs NumBlocks tasks as **isolated agents**, then merges them back — with a small web
UI to drive it.

```
you ──web UI──> agentctl server ──spawns──> opencode run (agent, isolated copy)
                        │                        │ works, commits in its copy
                        │                        ▼
                        └──spawns──> opencode run (merger, in main tree)
                                       3-way merge → verify → version bump → commit
```

## Start it

```bash
python3 tools/agentctl/server.py            # http://127.0.0.1:8737, opens a browser
python3 tools/agentctl/server.py --port 9000 --no-open --no-auto-merge
```

| flag | meaning |
| --- | --- |
| `--port N` | port (default 8737) |
| `--host ADDR` | bind address (default `127.0.0.1`; `0.0.0.0` exposes it to the network) |
| `--model ID` | default model for new tasks (default `opencode-go/step-5-preview-free`, changeable per task in the UI and at runtime in the *auto-merge* checkbox's config file) |
| `--no-auto-merge` | only merge when you press **Merge now** |
| `--no-open` | don't open a browser |

The server adds `games/numblocks-agents/` to `.git/info/exclude` on first start, so
its state never shows up in `git status`.

## How a task flows

1. **Spawn** — `games/numblocks/nb-<id>/` gets a source-only copy of `games/numblocks`
   (no `build/`, no `output/`), its own git repo, and a `base:` snapshot commit. Then
   `opencode run --auto` starts in that copy with the rules in `prompts/agent.md`.
2. **Work** — the agent may edit anything *inside its copy*. It builds and tests with
   `build/play` (never the X11 `build/desktop`), saves verification screenshots to
   `shots/`, commits, writes `RESULT.md`, exits.
3. **Follow-ups** — messages you send while it runs are queued and delivered when it
   goes idle (they continue its opencode session, so it keeps its context).
4. **Merge** — a single merger agent at a time (FIFO by spawn order) runs
   `tools/agentctl/merge_files.py <id>` for the mechanical per-file 3-way merge
   (`git merge-file`), resolves whatever conflicts remain semantically, runs the full
   verification gate (`make host desktop build check`, `make test`, a `play` scenario,
   refresh of `build/apps/NumBlocks.nwa`), bumps `1.A.B` in `versions.md` +
   `NB_VERSION`, commits **only the files it wrote**, and appends to `merge-log.md`.
   Outcome is written to `<copy>/MERGE.md`: `STATUS: merged` or `STATUS: blocked`
   (blocked restores the main tree from `premerge-backup/`).

## Layout

```
tools/agentctl/
  server.py            # controller: spawner, monitor, merger launcher, JSON API, static UI
  merge_files.py       # deterministic per-file 3-way merge of a copy into the main tree
  prompts/agent.md     # rules every task agent gets ({{...}} are filled per spawn)
  prompts/merger.md    # the merger playbook
  ui/                  # the web UI (index.html, style.css, app.js — no build step)
games/numblocks-agents/            # runtime state (gitignored)
  agentctl.json        # { auto_merge, model }
  merge-log.md         # what each merge changed + notes for future tasks
  nb-<id>/
    meta.json          # status, session id, runs, merge result
    TASK.md RESULT.md MERGE.md  agent.log  merger.log
    base snapshot + agent commits   (the copy's own git repo)
    shots/  premerge-backup/  merge-report.json
```

Run one task through it end-to-end before trusting it with something real. A live
example (task `nb-adad40`, fully merged, MERGE.md + shots + logs kept for
inspection) is left in `games/numblocks-agents/` until you delete it.

Task statuses: `running → done → merging → merged | blocked`, plus `failed`
(agent crashed) and `cancelled`. **Cancel** refuses while a merger runs for that task —
killing a merger mid-flight could leave the main tree half-merged.

## Notes and known trade-offs

- Each copy needs the siblings `games/numblocks` reaches with `../common` and
  `../../minecraft-*`. The controller creates read-only symlinks for them in
  `games/numblocks-agents/` on first start, so `make host` works in every copy.
- Fresh `git init` repos need an identity: the controller configures one in each
  copy (the main repo author when available). The main repo has none configured,
  so mergers commit with `-c user.name/email` borrowed from the last commit.
- The merger commits with `git commit --only <paths>` — never `git add` + pathless
  commit: this repo's index holds thousands of the owner's staged files and a
  pathless commit would sweep them in.
- The merger commits **only the files it wrote**, so the owner's uncommitted WIP in
  `games/numblocks` stays WIP. If a file it merged already had uncommitted changes,
  those ride along in the commit and the merger lists them under `WIP-RIDDEN:` in
  `MERGE.md`.
- `opencode run` needs an explicit `--model`; the model-less default errors out on this
  machine. Pick one per task in the spawn dialog.
- Sessions created by agentctl are ordinary opencode sessions — watch them live in the
  normal TUI/web UI too.
- Two tasks that touch the same functions still collide sometimes; the merger resolves
  it (that's the point of it being an agent). Merges are serialized, so conflicts are
  always pairwise, never three-way.
