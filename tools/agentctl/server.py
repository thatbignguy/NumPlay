#!/usr/bin/env python3
"""agentctl controller server for NumBlocks agent tasks.

Architecture (see tools/agentctl/README.md):
  * every task gets an isolated copy of games/numblocks at
    games/numblocks-agents/<id>/ (source only, its own git repo, base snapshot)
  * a task agent runs unattended: opencode run --auto --format json
  * follow-up messages continue the agent's session (queued while it is busy)
  * when an agent finishes, a single merger agent (one at a time, FIFO) folds the
    copy into the main tree, verifies, bumps the version and commits
  * this server exposes a small web UI + JSON API to drive all of that

Usage:
    python3 tools/agentctl/server.py [--port 8737] [--host 127.0.0.1]
                                      [--model opencode-go/step-5-preview-free]
                                      [--no-auto-merge] [--no-open]
"""
from __future__ import annotations

import argparse
import json
import os
import re
import secrets
import shutil
import subprocess
import sys
import threading
import time
import traceback
import webbrowser
from datetime import datetime, timezone
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, unquote, urlparse

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]                      # repo root (tools/agentctl -> repo)
NUMBLOCKS = ROOT / "games" / "numblocks"
AGENTS = ROOT / "games" / "numblocks-agents"
UI_DIR = HERE / "ui"
PROMPTS = HERE / "prompts"
CONFIG_PATH = AGENTS / "agentctl.json"
MERGELOG = AGENTS / "merge-log.md"
EXCLUDE = ROOT / ".git" / "info" / "exclude"

DEFAULT_MODEL = "opencode-go/step-5-preview-free"
TERMINAL = {"merged", "blocked", "failed", "cancelled"}
ACTIVE = {"running", "done", "merging"}
STATUS_LABEL = {
    "running": "running", "done": "done", "merging": "merging", "merged": "merged",
    "blocked": "blocked", "failed": "failed", "cancelled": "cancelled",
}

STATE_LOCK = threading.RLock()
PROCS: dict[str, dict] = {}                 # task id -> {"proc": Popen, "kind": ...}
MODELS_CACHE: dict = {"at": 0.0, "models": []}


def now() -> str:
    return datetime.now(timezone.utc).astimezone().isoformat(timespec="seconds")


# --------------------------------------------------------------------------- io


def load_json(path: Path, default):
    try:
        return json.loads(path.read_text())
    except Exception:
        return default


def save_json(path: Path, obj) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_suffix(path.suffix + ".tmp")
    tmp.write_text(json.dumps(obj, indent=2))
    tmp.replace(path)


def task_dirs() -> list[Path]:
    if not AGENTS.is_dir():
        return []
    return sorted(
        (d for d in AGENTS.iterdir() if d.is_dir() and (d / "meta.json").is_file()),
        key=lambda d: load_json(d / "meta.json", {}).get("spawned_at", ""),
        reverse=True,
    )


def load_meta(tid: str):
    return load_json(AGENTS / tid / "meta.json", None)


def save_meta(meta) -> None:
    meta["updated_at"] = now()
    save_json(AGENTS / meta["id"] / "meta.json", meta)


def all_metas() -> list[dict]:
    out = []
    for d in task_dirs():
        m = load_json(d / "meta.json", None)
        if m:
            out.append(m)
    return out


def agent_env() -> dict:
    env = dict(os.environ)
    prepend = [
        str(ROOT / ".tools" / "pngquant" / "usr" / "bin"),
        str(ROOT / ".tools" / "xpack-arm-none-eabi-gcc-15.2.1-1.1" / "bin"),
        str(ROOT / "node_modules" / ".bin"),
    ]
    env["PATH"] = os.pathsep.join(prepend + [env.get("PATH", "")])
    env["PY"] = str(ROOT / ".venv" / "bin" / "python")
    env["NWLINK"] = str(ROOT / "node_modules" / ".bin" / "nwlink")
    return env


def short(text: str, n: int = 70) -> str:
    text = " ".join(text.split())
    return text if len(text) <= n else text[: n - 1] + "…"


# ------------------------------------------------------------------ log format
#
# `opencode run --format json` emits one JSON event per line. Raw, it is unreadable
# (80 tool events for 10 lines of prose in a typical task). These helpers turn the
# stream into "what the operator should know": the agent's messages, the files it
# writes, the commands it runs, and anything that went wrong — nothing else.

_SILENT_TOOLS = {"read", "glob", "grep", "list", "ls", "todoread"}
_WRITE_TOOLS = {"edit", "write", "patch", "multiedit", "str_replace", "apply_patch"}
_RUN_TOOLS = {"bash", "shell", "execute", "run", "process"}


def _clip(text: str, n: int) -> str:
    text = text.strip()
    return text if len(text) <= n else text[: n - 1] + "…"


def _rel(path: str, cwd: Path) -> str:
    p = str(path)
    try:
        rp = str(Path(p).resolve())
        for base in (str(Path(cwd).resolve()), str(ROOT)):
            if rp == base:
                return "."
            if rp.startswith(base.rstrip("/") + "/"):
                return rp[len(base.rstrip("/")) + 1:]
    except OSError:
        pass
    return p


def format_log_line(line: str, cwd: Path, max_text: int = 1400):
    """One raw log line -> one operator-visible line, or None to drop it."""
    s = line.rstrip()
    if not s.strip():
        return None
    if s.startswith("====="):                      # our own run separators
        return s
    try:
        ev = json.loads(s)
    except Exception:
        return s                                  # not JSON: pass through
    part = ev.get("part") or {}
    ptype = part.get("type") or ev.get("type")

    if ev.get("type") == "error":
        err = ev.get("error") or {}
        return f"!! ERROR: {_clip(str(err.get('message') or ev), 300)}"

    if ptype == "text":
        text = _clip(part.get("text") or "", max_text)
        return text or None

    if ptype == "tool" or ev.get("type") == "tool_use":
        tool = part.get("tool") or "tool"
        st = part.get("state") or {}
        status = st.get("status") or ""
        inp = st.get("input") or {}
        if status and status != "completed":
            err = _clip(str(st.get("error") or status), 200)
            return f"!! {tool} {status}: {err}"
        if tool in _SILENT_TOOLS:
            return None
        if tool in _WRITE_TOOLS:
            path = inp.get("path") or inp.get("filePath") or "?"
            return f"edited {_rel(path, cwd)}"
        if tool in _RUN_TOOLS:
            cmd = _clip(str(inp.get("command") or inp.get("code") or ""), 110)
            return f"$ {cmd}" if cmd else f"$ ({tool})"
        head = next(iter(inp.values()), "") if inp else ""
        return f"· {tool}: {_clip(str(head), 90)}" if head else f"· {tool}"

    if ptype in ("step-start", "step_start", "step-finish", "step_finish"):
        return None                               # pure bookkeeping
    if ptype == "reasoning":
        return None
    return None


def format_log_text(text: str, cwd: Path) -> str:
    out = []
    for line in text.splitlines():
        f = format_log_line(line, cwd)
        if f:
            out.append(f)
    return "\n".join(out)


# ------------------------------------------------------------------ opencode run


def run_opencode(args: list[str], cwd: Path, logfile: Path, title: str, model: str,
                 message: str) -> subprocess.Popen:
    """Start an unattended opencode session; stdout+stderr append to logfile."""
    cmd = [
        "opencode", "run",
        "--auto",                 # no permission prompts: this is unattended
        "--format", "json",
        "--title", title,
        "-m", model,
        *args,
        message,
    ]
    log = logfile.open("ab")
    log.write(f"\n===== {now()} :: {' '.join(cmd[:8])} <message {len(message)} chars> =====\n".encode())
    log.flush()
    return subprocess.Popen(
        cmd, cwd=str(cwd), stdout=log, stderr=subprocess.STDOUT,
        env=agent_env(), start_new_session=True,
    )


def proc_alive(tid: str) -> bool:
    ent = PROCS.get(tid)
    return bool(ent) and ent["proc"].poll() is None


def extract_session_id(logfile: Path, from_offset: int = 0) -> str | None:
    try:
        with logfile.open("rb") as f:
            f.seek(from_offset)
            head = f.read(200_000).decode("utf-8", "replace")
    except OSError:
        return None
    for line in head.splitlines():
        m = re.search(r'"sessionID"\s*:\s*"([^"]+)"', line)
        if m:
            return m.group(1)
    return None


# ------------------------------------------------------------------------ spawn


def render_prompt(name: str, mapping: dict[str, str]) -> str:
    text = (PROMPTS / name).read_text()
    for k, v in mapping.items():
        text = text.replace("{{" + k + "}}", v)
    return text


def git_identity() -> tuple[str, str]:
    """Copies are fresh `git init` repos; they need an identity to commit —
    reuse the main repo's author when it has one."""
    def cfg(key: str) -> str:
        return subprocess.run(["git", "config", key], cwd=str(ROOT),
                              capture_output=True, text=True).stdout.strip()

    name = cfg("user.name")
    email = cfg("user.email")
    if not name:
        name = subprocess.run(["git", "log", "-1", "--format=%an"], cwd=str(ROOT),
                              capture_output=True, text=True).stdout.strip() or "agentctl"
    if not email:
        email = subprocess.run(["git", "log", "-1", "--format=%ae"], cwd=str(ROOT),
                               capture_output=True, text=True).stdout.strip() or "agentctl@localhost"
    return name, email


def spawn_task(task_text: str, model: str) -> dict:
    tid = "nb-" + secrets.token_hex(3)
    dest = AGENTS / tid
    dest.mkdir(parents=True)

    subprocess.run(
        ["rsync", "-a", "--exclude", "build/", "--exclude", "output*/",
         "--exclude", "__pycache__/", "--exclude", ".agentctl-base-tmp",
         f"{NUMBLOCKS}/", f"{dest}/"],
        check=True,
    )
    (dest / ".gitignore").write_text(
        (dest / ".gitignore").read_text()
        + "\n# agentctl runtime state — never part of the merge\n"
        "shots/\npremerge-backup/\nTASK.md\nRESULT.md\nMERGE.md\n"
        "meta.json\nmerge-report.json\nagent.log\nmerger.log\n.agentctl-base-tmp\n"
    )

    subprocess.run(["git", "init", "-q"], cwd=str(dest), capture_output=True)
    name, email = git_identity()
    for key, val in (("user.name", name), ("user.email", email)):
        subprocess.run(["git", "config", key, val], cwd=str(dest), capture_output=True)
    subprocess.run(["git", "add", "-A"], cwd=str(dest), capture_output=True)
    commit = subprocess.run(
        ["git", "commit", "-q", "-m", f"base: snapshot of games/numblocks @ {now()}"],
        cwd=str(dest), capture_output=True, text=True,
    )
    base = subprocess.run(["git", "rev-parse", "HEAD"], cwd=str(dest),
                          capture_output=True, text=True).stdout.strip()
    if not base or not re.fullmatch(r"[0-9a-f]{7,40}", base):
        raise RuntimeError(f"git snapshot failed: {commit.stderr or commit.stdout}")

    (dest / "TASK.md").write_text(
        f"# Task {tid}\n\n{task_text}\n\n(spawned {now()}, model {model})\n"
    )
    title = f"[{tid}] {short(task_text, 60)}"
    meta = {
        "id": tid,
        "task": task_text,
        "title": title,
        "model": model,
        "status": "running",
        "spawned_at": now(),
        "updated_at": now(),
        "base_commit": base,
        "session_id": None,
        "pid": None,
        "outbox": [],
        "runs": [],
        "merge": None,
        "error": None,
    }
    save_json(dest / "meta.json", meta)

    prompt = render_prompt("agent.md", {
        "TASK_ID": tid,
        "TASK": task_text,
        "COPY_DIR": str(dest),
        "REPO": str(ROOT),
    })
    p = run_opencode([], dest, dest / "agent.log", title, model, prompt)
    meta["pid"] = p.pid
    meta["runs"].append({"kind": "agent", "pid": p.pid, "started": now(), "exit": None})
    save_meta(meta)
    with STATE_LOCK:
        PROCS[tid] = {"proc": p, "kind": "agent", "log_at": 0}
    return meta


def send_followup(meta: dict, text: str) -> None:
    tid = meta["id"]
    dest = AGENTS / tid
    sid = meta.get("session_id")
    if not sid:
        sid = extract_session_id(dest / "agent.log")
        meta["session_id"] = sid
    p = run_opencode(["--session", sid] if sid else [], dest, dest / "agent.log",
                    meta["title"], meta["model"], text)
    meta["pid"] = p.pid
    meta["status"] = "running"
    meta["runs"].append({"kind": "agent", "pid": p.pid, "started": now(),
                         "exit": None, "followup": short(text, 80)})
    save_meta(meta)
    with STATE_LOCK:
        PROCS[tid] = {"proc": p, "kind": "agent", "log_at": 0}


# ----------------------------------------------------------------------- merger


def start_merger(meta: dict) -> None:
    tid = meta["id"]
    dest = AGENTS / tid
    model = meta.get("model") or DEFAULT_MODEL
    prompt = render_prompt("merger.md", {
        "TASK_ID": tid,
        "COPY_DIR": str(dest),
        "NUMBLOCKS": str(NUMBLOCKS),
        "REPO": str(ROOT),
        "BASE_COMMIT": meta.get("base_commit", ""),
        "AGENTS_DIR": str(AGENTS),
    })
    title = f"[merge {tid}] {short(meta['task'], 50)}"
    meta["status"] = "merging"
    meta["runs"].append({"kind": "merger", "started": now(), "exit": None})
    save_meta(meta)
    p = run_opencode([], NUMBLOCKS, dest / "merger.log", title, model, prompt)
    meta["pid"] = p.pid
    save_meta(meta)
    with STATE_LOCK:
        PROCS[tid] = {"proc": p, "kind": "merger", "log_at": 0}


def handle_merge_end(meta: dict, code: int) -> None:
    dest = AGENTS / meta["id"]
    text = ""
    for name in ("MERGE.md",):
        f = dest / name
        if f.is_file():
            text = f.read_text()
            break
    status = None
    m = re.search(r"STATUS:\s*(merged|blocked)", text, re.I)
    if m:
        status = m.group(1).lower()
    meta["merge"] = meta.get("merge") or {}
    meta["merge"]["result"] = status or "unknown"
    meta["merge"]["exit_code"] = code
    for pat, key in ((r"Version:\s*([0-9]+\.[0-9]+\.[0-9]+)", "version"),
                     (r"Commit:\s*([0-9a-f]{7,40})", "commit")):
        mm = re.search(pat, text)
        if mm:
            meta["merge"][key] = mm.group(1)
    meta["status"] = status or "blocked"
    if status == "blocked" and not meta["merge"].get("error"):
        meta["merge"]["error"] = "merger reported blocked"
    save_meta(meta)


# ---------------------------------------------------------------------- monitor


def cancel_proc(tid: str, hard: bool = False) -> int | None:
    ent = PROCS.pop(tid, None)
    if not ent:
        return None
    p = ent["proc"]
    if p.poll() is not None:
        return p.returncode
    p.terminate()
    try:
        p.wait(timeout=5)
    except subprocess.TimeoutExpired:
        if hard:
            p.kill()
            p.wait(timeout=5)
    return p.returncode


def tick() -> None:
    metas = all_metas()
    merger_busy = any(
        load_json(d / "meta.json", {}).get("status") == "merging" for d in task_dirs()
    )

    for meta in metas:
        tid = meta["id"]
        st = meta.get("status")
        alive = proc_alive(tid)

        if st == "running" and not alive:
            ent = PROCS.pop(tid, None)
            code = ent["proc"].returncode if ent else None
            meta["runs"][-1]["ended"] = now()
            if meta["runs"]:
                meta["runs"][-1]["exit"] = code
            if code == 0:
                meta["status"] = "done"
                meta["error"] = None
            else:
                meta["status"] = "failed"
                meta["error"] = f"agent process exited with code {code} — see agent.log"
            save_meta(meta)
            continue

        if st == "merging" and not alive:
            ent = PROCS.pop(tid, None)
            handle_merge_end(meta, ent["proc"].returncode if ent else -1)
            continue

        # follow-up messages: queue while busy, send when the agent goes idle
        if not alive and meta.get("outbox") and st in ("running", "done"):
            msg = meta["outbox"].pop(0)
            save_meta(meta)
            send_followup(meta, msg)
            continue

    if not merger_busy and load_config().get("auto_merge", True):
        done = [m for m in all_metas() if m.get("status") == "done"]
        if done:
            start_merger(done[-1])   # oldest spawned first


def monitor() -> None:
    while True:
        try:
            tick()
        except Exception:
            traceback.print_exc(file=sys.stderr)
        time.sleep(1.0)


# ---------------------------------------------------------------------- config


def load_config() -> dict:
    cfg = load_json(CONFIG_PATH, {})
    cfg.setdefault("auto_merge", True)
    return cfg


def save_config(cfg: dict) -> None:
    save_json(CONFIG_PATH, cfg)


def ensure_layout() -> None:
    AGENTS.mkdir(parents=True, exist_ok=True)
    # A copy at games/numblocks-agents/nb-x/ needs the siblings the numblocks
    # Makefile reaches with ../common and ../../minecraft-*; read-only symlinks
    # keep `make host` working inside every copy without duplicating them.
    # ../common            -> games/common           (one level up)
    # ../../minecraft-*    -> repo root               (two levels up)
    for name in ("common", "minecraft-textures"):
        link = AGENTS / name
        target = (ROOT / "games" / name) if name == "common" else (ROOT / name)
        if target.is_dir() and not link.exists():
            try:
                rel = os.path.relpath(target, AGENTS)
                link.symlink_to(rel, target_is_directory=True)
            except OSError:
                pass
    if not MERGELOG.is_file():
        MERGELOG.write_text(
            "# NumBlocks agent merge log\n\n"
            "One entry per merge: what changed and what later tasks should know.\n"
            "The merger agent appends here after every successful merge.\n"
        )
    if not CONFIG_PATH.is_file():
        save_config({"auto_merge": True, "model": DEFAULT_MODEL})
    try:
        excl = EXCLUDE.read_text() if EXCLUDE.is_file() else ""
        EXCLUDE.parent.mkdir(parents=True, exist_ok=True)
        if "games/numblocks-agents/" not in excl:
            with EXCLUDE.open("a") as f:
                f.write("\n# agentctl runtime state (agent copies, logs, shots)\n"
                        "games/numblocks-agents/\n")
    except OSError:
        pass


# ----------------------------------------------------------------------- models


def list_models() -> list[str]:
    if time.time() - MODELS_CACHE["at"] < 300 and MODELS_CACHE["models"]:
        return MODELS_CACHE["models"]
    models: list[str] = []
    for attempt in range(2):
        try:
            out = subprocess.run(["opencode", "models"], capture_output=True,
                                 text=True, timeout=90)
            models = [l.strip() for l in out.stdout.splitlines() if "/" in l]
            if models:
                break
        except Exception:
            pass
        time.sleep(1.5)
    if models:
        MODELS_CACHE.update({"at": time.time(), "models": models})
    else:
        MODELS_CACHE.update({"at": time.time() - 240, "models": []})  # retry in ~60s
    return models


# ------------------------------------------------------------------------- http


def json_bytes(obj) -> bytes:
    return json.dumps(obj).encode()


class Handler(BaseHTTPRequestHandler):
    server_version = "agentctl/1.0"
    protocol_version = "HTTP/1.1"

    def log_message(self, fmt, *args):  # quieter logs
        sys.stderr.write("[http] %s\n" % (fmt % args))

    # -- helpers ------------------------------------------------------------

    def _send(self, code: int, body: bytes, ctype: str = "application/json"):
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        try:
            self.wfile.write(body)
        except (BrokenPipeError, ConnectionResetError):
            pass

    def _json(self, obj, code: int = 200):
        self._send(code, json_bytes(obj))

    def _err(self, msg: str, code: int = 400):
        self._json({"error": msg}, code)

    def _body(self) -> dict:
        n = int(self.headers.get("Content-Length") or 0)
        if not n:
            return {}
        try:
            return json.loads(self.rfile.read(n))
        except Exception:
            return {}

    def _static(self, rel: str):
        rel = unquote(rel).lstrip("/")
        ctype = {".html": "text/html", ".css": "text/css", ".js": "text/javascript",
                 ".svg": "image/svg+xml"}.get(Path(rel).suffix, "text/plain")
        path = (UI_DIR / rel).resolve()
        if not str(path).startswith(str(UI_DIR.resolve())) or not path.is_file():
            return self._err("not found", 404)
        self._send(200, path.read_bytes(), ctype)

    def _shots(self, tid: str, name: str):
        base = (AGENTS / tid / "shots").resolve()
        path = (base / unquote(name)).resolve()
        if not str(path).startswith(str(base)) or not path.is_file():
            return self._err("not found", 404)
        self._send(200, path.read_bytes(),
                   "image/png" if path.suffix == ".png" else "application/octet-stream")

    # -- routes ---------------------------------------------------------------

    def do_GET(self):
        try:
            u = urlparse(self.path)
            p = u.path.rstrip("/") or "/"
            q = parse_qs(u.query)

            if p == "/":
                return self._static("index.html")
            if p in ("/style.css", "/app.js", "/favicon.svg"):
                return self._static(p.lstrip("/"))
            if p == "/api/tasks":
                return self._tasks()
            if p == "/api/models":
                return self._json({"models": list_models(), "default": DEFAULT_MODEL})
            if p == "/api/config":
                return self._json(load_config())
            m = re.fullmatch(r"/task/([\w-]+)", p)
            if m:
                return self._task_page(m.group(1))
            m = re.fullmatch(r"/api/tasks/([\w-]+)", p)
            if m:
                return self._task_detail(m.group(1))
            m = re.fullmatch(r"/api/tasks/([\w-]+)/log", p)
            if m:
                return self._task_log(m.group(1), q)
            m = re.fullmatch(r"/api/tasks/([\w-]+)/diff", p)
            if m:
                return self._task_diff(m.group(1))
            m = re.fullmatch(r"/api/tasks/([\w-]+)/shots", p)
            if m:
                return self._task_shots(m.group(1))
            m = re.fullmatch(r"/api/tasks/([\w-]+)/shots/([\w.\-]+)", p)
            if m:
                return self._shots(m.group(1), m.group(2))
            return self._err("not found", 404)
        except BrokenPipeError:
            pass
        except Exception as e:
            traceback.print_exc(file=sys.stderr)
            self._err(f"server error: {e}", 500)

    def do_POST(self):
        try:
            u = urlparse(self.path)
            p = u.path.rstrip("/")
            body = self._body()

            if p == "/api/tasks":
                task = (body.get("task") or "").strip()
                if not task:
                    return self._err("task text required")
                with STATE_LOCK:
                    meta = spawn_task(task, body.get("model") or DEFAULT_MODEL)
                return self._json(meta, 201)
            m = re.fullmatch(r"/api/tasks/([\w-]+)/message", p)
            if m:
                return self._post_message(m.group(1), (body.get("text") or "").strip())
            m = re.fullmatch(r"/api/tasks/([\w-]+)/cancel", p)
            if m:
                return self._post_cancel(m.group(1))
            m = re.fullmatch(r"/api/tasks/([\w-]+)/merge", p)
            if m:
                return self._post_merge(m.group(1))
            m = re.fullmatch(r"/api/tasks/([\w-]+)/delete", p)
            if m:
                return self._post_delete(m.group(1))
            if p == "/api/config":
                cfg = load_config()
                cfg.update({k: v for k, v in body.items()
                            if k in ("auto_merge", "model")})
                save_config(cfg)
                return self._json(cfg)
            return self._err("not found", 404)
        except BrokenPipeError:
            pass
        except Exception as e:
            traceback.print_exc(file=sys.stderr)
            self._err(f"server error: {e}", 500)

    # -- api impls ----------------------------------------------------------


    def _task_page(self, tid: str):
        """Read-only, server-rendered view of one task (no JS, no actions)."""
        meta = load_meta(tid)
        if not meta:
            return self._err("unknown task", 404)
        dest = AGENTS / tid
        esc = lambda s: (s or "").replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
        result = read_text(dest / "RESULT.md") or "_(the agent has not written RESULT.md yet)_"
        merge = read_text(dest / "MERGE.md") or "_(no merge report yet)_"
        diffstat = git_text(["diff", "--stat", meta["base_commit"], "HEAD"], dest).strip()
        logs = ""
        for name, label in (("agent.log", "agent"), ("merger.log", "merger")):
            f = dest / name
            if f.is_file():
                tail = f.read_text(errors="replace").splitlines()[-400:]
                formatted = format_log_text("\n".join(tail), dest)
                logs += f"----- {label} -----\n" + formatted + "\n"
        if not logs:
            logs = "_(no log output yet)_"
        shots = sorted(p.name for p in (dest / "shots").glob("*.png")) if (dest / "shots").is_dir() else []
        shots_html = "".join(
            f'<figure><img src="/api/tasks/{tid}/shots/{n}" alt="{esc(n)}">'
            f"<figcaption>{esc(n)}</figcaption></figure>" for n in shots
        ) or '<div class="empty">no screenshots</div>'
        mv = meta.get("merge") or {}

        page = f"""<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{esc(tid)} — {esc(STATUS_LABEL.get(meta['status'], meta['status']))}</title>
<link rel="stylesheet" href="/style.css"></head>
<body class="taskpage">
<header id="topbar">
  <div class="brand">{esc(tid)}&nbsp;<span>agentctl</span></div>
  <span class="chip {meta['status']}">{STATUS_LABEL.get(meta['status'], meta['status'])}</span>
  <div class="spacer"></div>
</header>
<div class="taskpage-body">
  <div id="detail-head">
    <div id="detail-task">{esc(meta['task'])}</div>
    <div id="detail-meta" class="muted mono">spawned {esc(meta.get('spawned_at',''))}
      · model {esc(meta.get('model',''))} · session {esc(meta.get('session_id') or '—')}
      {(' · merged as v' + esc(mv['version'])) if mv.get('version') else ''}</div>
  </div>
  <section><h2>Result</h2><pre class="doc">{esc(result)}</pre></section>
  <section><h2>Merge report</h2><pre class="doc">{esc(merge)}</pre></section>
  <section><h2>Changed files</h2><pre class="mono diffbox">{esc(diffstat) or '(nothing)'}</pre></section>
  <section><h2>Screenshots</h2><div class="shots">{shots_html}</div></section>
  <section><h2>Log tail</h2><pre class="mono logbox">{esc(logs)}</pre></section>
</div>
</body></html>"""
        self._send(200, page.encode(), "text/html; charset=utf-8")

    def _tasks(self):
        metas = all_metas()
        for m in metas:
            m["busy"] = proc_alive(m["id"])
        merger = next((m for m in metas if m["status"] == "merging"), None)
        self._json({
            "tasks": metas,
            "merger_active": bool(merger),
            "merger_task": merger["id"] if merger else None,
            "auto_merge": load_config().get("auto_merge", True),
        })

    def _task_detail(self, tid: str):
        meta = load_meta(tid)
        if not meta:
            return self._err("unknown task", 404)
        dest = AGENTS / tid
        meta["busy"] = proc_alive(tid)
        if not meta.get("session_id"):
            sid = extract_session_id(dest / "agent.log")
            if sid:
                meta["session_id"] = sid
                save_meta(meta)
        meta["result"] = read_text(dest / "RESULT.md")
        meta["merge_report"] = read_text(dest / "MERGE.md")
        meta["diffstat"] = git_text(["diff", "--stat", meta["base_commit"], "HEAD"], dest)
        try:
            st = subprocess.run(
                ["git", "status", "--porcelain", "--untracked-files=no"], cwd=str(dest),
                capture_output=True, text=True).stdout.strip()
            meta["copy_dirty"] = bool(st)
        except Exception:
            meta["copy_dirty"] = None
        self._json({"meta": meta})

    def _task_log(self, tid: str, q):
        kind = (q.get("kind") or ["agent"])[0]
        if kind not in ("agent", "merger"):
            return self._err("kind must be agent|merger")
        fmt = (q.get("format") or ["pretty"])[0]
        off = int((q.get("offset") or ["0"])[0])
        f = AGENTS / tid / f"{kind}.log"
        if not f.is_file():
            return self._json({"offset": 0, "size": 0, "text": ""})
        size = f.stat().st_size
        if off > size:
            off = 0
        with f.open("rb") as fh:
            fh.seek(off)
            chunk = fh.read()
        text = chunk.decode("utf-8", "replace")
        if fmt != "raw":
            text = format_log_text(text, AGENTS / tid)
        self._json({"offset": size, "size": size, "text": text})

    def _task_diff(self, tid: str):
        meta = load_meta(tid)
        if not meta:
            return self._err("unknown task", 404)
        dest = AGENTS / tid
        out = {
            "source": "agent",
            "stat": git_text(["diff", "--stat", meta["base_commit"], "HEAD"], dest),
            "patch": git_text(["diff", meta["base_commit"], "HEAD"], dest),
        }
        mc = (meta.get("merge") or {}).get("commit")
        if mc:
            out["source"] = "merge"
            out["commit"] = mc
            out["stat"] = git_text(["show", "--stat", "--oneline", mc], NUMBLOCKS)
            out["patch"] = git_text(["show", mc, "--format="], NUMBLOCKS)
        self._json(out)

    def _task_shots(self, tid: str):
        d = AGENTS / tid / "shots"
        files = sorted(p.name for p in d.glob("*.png")) if d.is_dir() else []
        self._json({"shots": files})

    def _post_message(self, tid: str, text: str):
        meta = load_meta(tid)
        if not meta:
            return self._err("unknown task", 404)
        if not text:
            return self._err("message text required")
        if meta["status"] in ("merged", "cancelled", "failed"):
            return self._err(f"agent is {meta['status']}; cannot send messages")
        meta.setdefault("outbox", []).append(text)
        save_meta(meta)
        if proc_alive(tid):
            return self._json({"ok": True, "queued": True})
        return self._json({"ok": True, "queued": False})

    def _post_cancel(self, tid: str):
        meta = load_meta(tid)
        if not meta:
            return self._err("unknown task", 404)
        if meta["status"] == "merging":
            return self._err("a merge is in progress for this task; cancelling a "
                             "merger could leave games/numblocks half-merged", 409)
        code = cancel_proc(tid, hard=True)
        meta["status"] = "cancelled"
        meta["error"] = f"cancelled by user (process exit {code})"
        save_meta(meta)
        self._json({"ok": True})

    def _post_merge(self, tid: str):
        meta = load_meta(tid)
        if not meta:
            return self._err("unknown task", 404)
        if meta["status"] == "merging":
            return self._err("already merging", 409)
        if meta["status"] not in ("done", "blocked", "failed"):
            return self._err(f"nothing to merge from status '{meta['status']}'", 409)
        if any(m["status"] == "merging" for m in all_metas()):
            return self._err("another merge is already running", 409)
        with STATE_LOCK:
            start_merger(meta)
        self._json({"ok": True})

    def _post_delete(self, tid: str):
        meta = load_meta(tid)
        if not meta:
            return self._err("unknown task", 404)
        if meta["status"] == "merging" and proc_alive(tid):
            return self._err("a merge is running for this task; deleting its copy "
                             "mid-merge would strand games/numblocks", 409)
        with STATE_LOCK:
            cancel_proc(tid, hard=True)
            shutil.rmtree(AGENTS / tid, ignore_errors=True)
        self._json({"ok": True})


def read_text(p: Path):
    try:
        return p.read_text()
    except OSError:
        return None


def git_text(args: list[str], cwd: Path) -> str:
    try:
        out = subprocess.run(["git", *args], cwd=str(cwd), capture_output=True,
                             text=True, timeout=30)
        return (out.stdout + (("\n" + out.stderr) if out.stderr.strip() else ""))
    except Exception as e:
        return f"(git failed: {e})"


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", type=int, default=8737)
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--no-auto-merge", action="store_true")
    ap.add_argument("--no-open", action="store_true")
    a = ap.parse_args()

    ensure_layout()
    cfg = load_config()
    if a.no_auto_merge:
        cfg["auto_merge"] = False
        save_config(cfg)

    threading.Thread(target=monitor, daemon=True).start()

    httpd = ThreadingHTTPServer((a.host, a.port), Handler)
    url = f"http://{a.host}:{a.port}"
    print(f"agentctl controller: {url}")
    print(f"  tasks dir:    {AGENTS}")
    print(f"  merging:      {'auto' if cfg.get('auto_merge', True) else 'manual'}")
    print(f"  default model:{DEFAULT_MODEL}")
    if not a.no_open:
        try:
            webbrowser.open(url)
        except Exception:
            pass
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
