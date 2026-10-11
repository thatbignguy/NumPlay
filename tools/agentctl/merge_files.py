#!/usr/bin/env python3
"""Mechanical part of an agentctl merge: fold an agent copy into the main NumBlocks tree.

The merger *agent* runs this first, then resolves whatever this script reports as
conflicted (it reads main, base, and theirs versions and decides semantically).

Per changed file (from `git diff base..HEAD` in the agent copy):
  A (added)     -> copy into main
  D (deleted)   -> delete from main
  M (modified)  -> fast path: main is byte-identical to base  -> take theirs
                   slow path: real 3-way merge (git merge-file ours base theirs)
                               -> clean result written, or conflict markers left
                               in place and the file listed in "conflicts"
  binary        -> fast path only, otherwise reported as a conflict

Every file this script writes is first backed up to
games/numblocks-agents/<task>/premerge-backup/ so the merger can restore the main
tree if the merge has to be abandoned (STATUS: blocked).

Usage:  python3 tools/agentctl/merge_files.py <task-id> [--dry-run]
Output: JSON report on stdout, also written to games/numblocks-agents/<task>/merge-report.json
"""
from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
AGENTS = ROOT / "games" / "numblocks-agents"
NUMBLOCKS = ROOT / "games" / "numblocks"


def git(args: list[str], cwd: Path, binary: bool = False):
    return subprocess.run(
        ["git", *args], cwd=cwd, capture_output=True, text=not binary, check=False
    )


def is_binary(data: bytes) -> bool:
    return b"\x00" in data[:8192]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("task", help="task id, e.g. nb-7k2m9q")
    ap.add_argument("--dry-run", action="store_true", help="report only, write nothing")
    a = ap.parse_args()

    copy = AGENTS / a.task
    meta_path = copy / "meta.json"
    if not meta_path.is_file():
        print(f"no task dir {copy}", file=sys.stderr)
        return 2
    meta = json.loads(meta_path.read_text())
    base = meta.get("base_commit")
    if not base:
        print("meta.json has no base_commit", file=sys.stderr)
        return 2
    if not (copy / ".git").is_dir():
        print(f"{copy} is not a git repo", file=sys.stderr)
        return 2

    out = git(["diff", "--name-status", base, "HEAD"], copy)
    if out.returncode != 0:
        print(out.stderr, file=sys.stderr)
        return 2

    backup_root = copy / "premerge-backup"
    report: dict[str, list] = {
        "task": a.task,
        "base": base,
        "merged": [],       # main file now equals theirs
        "added": [],
        "deleted": [],
        "conflicts": [],    # markers left in main, merger must resolve
        "failed": [],       # unexpected errors
    }

    for line in out.stdout.splitlines():
        if not line.strip():
            continue
        parts = line.split("\t")
        st = parts[0][0]
        path = parts[-1]  # rename targets: R100 old<TAB>new
        theirs = copy / path
        ours = NUMBLOCKS / path

        if st == "A":
            if not theirs.is_file():
                report["failed"].append({"path": path, "why": "missing in copy"})
                continue
            if not a.dry_run:
                ours.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(theirs, ours)
            report["added"].append(path)
            continue

        if st == "D":
            if ours.is_file() and not a.dry_run:
                backup(path, ours, backup_root)
                ours.unlink()
            report["deleted"].append(path)
            continue

        # M (and renames, treated as modify at the new path)
        if not theirs.is_file():
            report["failed"].append({"path": path, "why": "missing in copy"})
            continue
        if not ours.is_file():
            if not a.dry_run:
                ours.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(theirs, ours)
            report["added"].append(path)
            continue

        theirs_b = theirs.read_bytes()
        ours_b = ours.read_bytes()
        if is_binary(theirs_b) or is_binary(ours_b):
            base_blob = git(["show", f"{base}:{path}"], copy, binary=True)
            if base_blob.returncode == 0 and base_blob.stdout == ours_b:
                if not a.dry_run:
                    backup(path, ours, backup_root)
                    shutil.copy2(theirs, ours)
                report["merged"].append(path)
            else:
                report["conflicts"].append({"path": path, "why": "binary, main moved"})
            continue

        base_blob = git(["show", f"{base}:{path}"], copy, binary=True)
        if base_blob.returncode != 0:
            # not in base after all -> treat as add
            if not a.dry_run:
                backup(path, ours, backup_root)
                shutil.copy2(theirs, ours)
            report["added"].append(path)
            continue

        if base_blob.stdout == ours_b:
            if not a.dry_run:
                backup(path, ours, backup_root)
                shutil.copy2(theirs, ours)
            report["merged"].append(path)
            continue

        base_file = copy / ".agentctl-base-tmp"
        base_file.write_bytes(base_blob.stdout)
        if not a.dry_run:
            backup(path, ours, backup_root)
            mf = subprocess.run(
                ["git", "merge-file", "-p", str(ours), str(base_file), str(theirs)],
                capture_output=True, text=True, check=False,
            )
            ours.write_text(mf.stdout)
            if mf.returncode == 0:
                report["merged"].append(path)
            elif " <<<<<<< " in mf.stdout:
                report["conflicts"].append(
                    {"path": path, "why": "both sides changed; markers left in file"}
                )
            else:
                report["failed"].append({"path": path, "why": mf.stderr.strip()})
        else:
            report["conflicts"].append({"path": path, "why": "would conflict (dry run)"})
        base_file.unlink(missing_ok=True)

    print(json.dumps(report, indent=2))
    (copy / "merge-report.json").write_text(json.dumps(report, indent=2))
    return 0 if not report["failed"] else 1


def backup(path: str, src: Path, backup_root: Path) -> None:
    dst = backup_root / path
    dst.parent.mkdir(parents=True, exist_ok=True)
    if not dst.exists():
        shutil.copy2(src, dst)


if __name__ == "__main__":
    sys.exit(main())
