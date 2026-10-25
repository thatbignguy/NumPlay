#!/usr/bin/env python3
"""Checks that NumBlocks on its own keeps its worlds through an install.

Installing an app from the NumWorks website empties the calculator's files but
the Python scripts, so NumBlocks keeps a copy of its saves in
numblocks_saves.py. In the ARM emulator:

1. a new world, a block mined, then Home: the world, its changed blocks and the
   options are saved, and the copy holds each of them, byte for byte, in a
   valid Python script;
2. "install again" (every file but the scripts deleted): started again, the
   saves come back byte for byte and the other script is untouched.

Usage: test_numblocks_progress.py build/compiled/games/NumBlocks.nwa [--out DIR]
"""
import argparse
import base64
import os
import struct
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "tools"))
from test_progress import presses, records, run, write_storage  # noqa: E402
import emu  # noqa: E402

COPY = "numblocks_saves.py"


def ours(name):
    return name == "numblocks.cfg" or (name.startswith("nb") and name[2:3].isdigit() and name[2] != "0")


def frames(c):
    """The app's own frame counter (main.c's perf_frames, read by tools/emu.py)."""
    if "perf_frames" not in c.symbols:
        raise SystemExit("the app carries no perf_frames: it cannot be waited on")
    return struct.unpack("<I", bytes(c.uc.mem_read(c.symbols["perf_frames"], 4)))[0]


def play(nwa, storage, keys, out, tag):
    """The first phase of this test: a new world, a block mined in it, then Home.

    A new world's terrain is built in one blocking pass - no frame is drawn while
    it happens (the app's frame counter stops) - and the player then stands wherever
    the seed's spawn search landed, facing whichever way it says. So the looking
    down and the mining are played once the counter moves again instead of at fixed
    times: a fixed time either lands inside the load (the key is never seen, nothing
    is mined) or on a player facing the sky (nothing to mine). The seed is random -
    it is the clock at the moment Create is pressed - so the load takes a few
    seconds and the player's facing changes from run to run."""
    c = emu.Calculator(nwa, storage_file=storage)
    c.enable_checks(reads=True)
    c.keys = keys
    c.run(keys[-1][1] + 200)   # the create press is done
    base = frames(c)
    until = c.now_ms + 120000   # the load is a few seconds; a world that never comes up is a failure
    while frames(c) - base < 5:   # the load's one frame, then the world's own
        if c.exited or c.now_ms > until:
            return None, [f"{tag}: the world never came up"]
        c.run(c.now_ms + 500)
    now = int(c.now_ms)
    c.keys += presses((now + 200, "down", 1200), (now + 1500, "ok", 3500))   # look down, mine
    home = now + 7000
    c.keys += [(home, home + 400, emu.KEYS["home"])]
    c.pending_shots = [(home - 150, os.path.join(out, f"{tag}_{home - 150}.png"))]
    c.run(home + 3000)
    problems = []
    if not c.exited:
        problems.append(f"{tag}: did not quit on Home")
    if c.violations:
        problems += [f"{tag}: {v}" for v in sorted(set(c.violations))[:5]]
    return dict(records(open(storage, "rb").read())), problems


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("nwa")
    ap.add_argument("--out", default="build/test_numblocks_progress")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    script = ("pi.py", b"\x01print(3.14159)\n\x00")
    problems = []
    with tempfile.TemporaryDirectory() as d:
        st = os.path.join(d, "storage.bin")
        write_storage(st, [script])
        # the key sheet, Singleplayer, Create New World, down to its button; then look down and mine (OK)
        keys = presses((3000, "ok"), (3600, "ok"), (4200, "ok"), (4800, "down"), (5100, "down"), (5400, "down"),
                       (5700, "down"), (6000, "down"), (6300, "ok"))
        recs, p = play(a.nwa, st, keys, a.out, "1_play")
        problems += p
        recs = recs or {}   # (a world that never came up: the problems above say so)
        saves = {n: v for n, v in recs.items() if ours(n)}
        for want in ("numblocks.cfg", "nb1.nbw"):
            if want not in saves:
                problems.append(f"{want} not written")
        if not any(n.startswith("nb1r") for n in saves):
            problems.append("no changed blocks saved (the mined block)")
        c = recs.get(COPY)
        if c is None:
            problems.append(f"{COPY} not written")
        else:
            if c[0] != 0 or c[-1] != 0 or 0 in c[1:-1]:
                problems.append(f"{COPY}: not a script")
            copied = {}
            for line in c[1:-1].decode("ascii", "replace").split("\n"):
                if line and not line.startswith("#"):
                    problems.append(f"{COPY}: a line that isn't a comment: {line[:40]!r}")
                if line.startswith("#>"):
                    name, _, data = line[2:].partition(":")
                    try:
                        copied[name] = base64.b64decode(data, validate=True)
                    except ValueError:
                        problems.append(f"{COPY}: {name} is not base64")
            if copied != saves:
                problems.append(f"{COPY} holds {sorted(copied)}, the saves are {sorted(saves)}"
                                + ("" if set(copied) != set(saves) else " (contents differ)"))
        # installed again: only the scripts are left
        write_storage(st, [(n, v) for n, v in records(open(st, "rb").read()) if n.endswith(".py")])
        recs, p = run(a.nwa, st, [], 4000, a.out, "2_restore")
        problems += p
        for n, v in saves.items():
            if recs.get(n) != v:
                problems.append(f"after installing again, {n} " + ("missing" if n not in recs else "differs"))
        if recs.get(script[0]) != script[1]:
            problems.append("the other script changed")
        print(f"   saves: {', '.join(f'{n} ({len(v)} bytes)' for n, v in sorted(saves.items()))}")
    for x in problems:
        print("   ", x)
    print("PASS" if not problems else "FAIL")
    sys.exit(1 if problems else 0)


if __name__ == "__main__":
    main()
