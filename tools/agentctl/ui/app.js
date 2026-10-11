/* agentctl web UI — vanilla JS, polls the controller's JSON API. */
"use strict";

const state = {
  tasks: [],
  selected: null,
  tab: "log",
  logKind: "agent",
  log: { text: "", offset: 0 },       // active task log accumulator
  detail: null,
  autoMerge: true,
  models: [],
  defaultModel: null,
};

const DEFAULT_FALLBACK = "opencode-go/step-5-preview-free";

const $ = (sel) => document.querySelector(sel);
const el = (tag, cls, text) => {
  const n = document.createElement(tag);
  if (cls) n.className = cls;
  if (text !== undefined) n.textContent = text;
  return n;
};

async function api(path, opts) {
  const r = await fetch(path, opts);
  const body = await r.json().catch(() => ({}));
  if (!r.ok) throw new Error(body.error || r.statusText);
  return body;
}

/* ------------------------------------------------------------------ polling */

async function refreshList() {
  try {
    const data = await api("/api/tasks");
    state.tasks = data.tasks;
    state.autoMerge = data.auto_merge;
    renderPills(data);
    renderTaskList();
  } catch (e) { /* server restarting; keep last state */ }
}

async function refreshDetail() {
  const id = state.selected;
  if (!id) return;
  try {
    const { meta } = await api(`/api/tasks/${id}`);
    state.detail = meta;
    renderDetailHead();
    if (state.tab === "log") await refreshLog();
    else if (state.tab === "changes") await refreshDiff();
    else if (state.tab === "shots") await refreshShots();
    else if (state.tab === "result") refreshDoc("result", meta.result);
    else if (state.tab === "merge") refreshDoc("merge-doc", meta.merge_report);
  } catch (e) { if (String(e.message).includes("404")) selectTask(null); }
}

async function refreshLog() {
  const id = state.selected;
  if (!id) return;
  const pre = $("#log");
  const atBottom = pre.scrollHeight - pre.scrollTop - pre.clientHeight < 60;
  const fmt = $("#log-raw").checked ? "raw" : "pretty";
  try {
    const data = await api(`/api/tasks/${id}/log?kind=${state.logKind}&format=${fmt}&offset=${state.log.offset}`);
    if (data.offset === 0 || data.offset < state.log.offset) { // new run / truncated
      state.log.text = "";
      state.log.offset = 0;
      const again = await api(`/api/tasks/${id}/log?kind=${state.logKind}&format=${fmt}&offset=0`);
      state.log.text += again.text;
      state.log.offset = again.offset;
    } else {
      state.log.text += data.text;
      state.log.offset = data.offset;
    }
    pre.textContent = state.log.text || "(no output yet)";
    if (atBottom || $("#log-autoscroll").checked) pre.scrollTop = pre.scrollHeight;
  } catch (e) { /* ignore */ }
}

async function refreshDiff() {
  const id = state.selected;
  try {
    const d = await api(`/api/tasks/${id}/diff`);
    const head = `—— ${d.source === "merge" ? "merge commit " + (d.commit || "") : "agent work: base → HEAD"} ——\n`;
    $("#diff").textContent = head + (d.stat || "") + "\n" + (d.patch || "");
  } catch (e) { /* ignore */ }
}

async function refreshShots() {
  const id = state.selected;
  const box = $("#shots");
  try {
    const { shots } = await api(`/api/tasks/${id}/shots`);
    box.textContent = "";
    if (!shots.length) {
      box.appendChild(el("div", "empty", "no screenshots — agents save verification shots into their copy's shots/ folder"));
      return;
    }
    for (const name of shots) {
      const fig = el("figure");
      const img = document.createElement("img");
      img.loading = "lazy";
      img.src = `/api/tasks/${id}/shots/${encodeURIComponent(name)}`;
      fig.appendChild(img);
      fig.appendChild(el("figcaption", null, name));
      box.appendChild(fig);
    }
  } catch (e) { /* ignore */ }
}

function refreshDoc(nodeId, text) {
  $(nodeId ? `#${nodeId}` : "#result").textContent = text || "(nothing written yet)";
}

/* ------------------------------------------------------------------ render */

const STATUS_LABEL = {
  running: "running", done: "done", merging: "merging", merged: "merged",
  blocked: "blocked", failed: "failed", cancelled: "cancelled",
};

function renderPills(data) {
  const box = $("#status-pills");
  box.textContent = "";
  const mk = (cls, text, title) => {
    const p = el("span", "pill " + cls, text);
    if (title) p.title = title;
    box.appendChild(p);
  };
  mk(data.merger_active ? "on" : "", data.merger_active ? `merging ${data.merger_task}` : "merger idle",
     data.merger_active ? `Merge in progress for ${data.merger_task}` : "No merge running");
  const running = state.tasks.filter((t) => t.status === "running").length;
  const waiting = state.tasks.filter((t) => t.status === "done").length;
  mk("", `${running} running`);
  if (waiting) mk("on", `${waiting} waiting to merge`);
}

function renderTaskList() {
  const box = $("#task-list");
  box.textContent = "";
  $("#sidebar-empty").hidden = state.tasks.length > 0;
  for (const t of state.tasks) {
    const item = el("div", "task-item" + (t.id === state.selected ? " sel" : ""));
    const row = el("div", "row1");
    row.appendChild(el("span", "dot " + t.status));
    row.appendChild(el("span", "tid mono", t.id));
    if (t.busy) row.appendChild(el("span", "chip subtle", "busy"));
    item.appendChild(row);
    item.appendChild(el("div", "text", t.task));
    item.appendChild(el("div", "when", (t.spawned_at || "").replace("T", " ").slice(0, 19) +
      (t.merge && t.merge.version ? `  ·  v${t.merge.version}` : "")));
    item.onclick = () => selectTask(t.id);
    box.appendChild(item);
  }
}

function selectTask(id) {
  state.selected = id;
  state.log = { text: "", offset: 0 };
  state.detail = null;
  $("#empty-state").hidden = !!id;
  $("#detail").hidden = !id;
  renderTaskList();
  refreshDetail();
}

function renderDetailHead() {
  const m = state.detail;
  if (!m) return;
  const idEl = $("#detail-id");
  idEl.textContent = "";
  const a = el("a", null, m.id);
  a.href = `/task/${encodeURIComponent(m.id)}`;
  a.target = "_blank";
  a.style.color = "var(--blue)";
  idEl.appendChild(a);
  idEl.appendChild(document.createTextNode(`  ·  ${m.title || ""}`));
  const st = $("#detail-status");
  st.textContent = STATUS_LABEL[m.status] || m.status;
  st.className = "chip " + m.status;
  $("#detail-busy").hidden = !m.busy;
  $("#detail-task").textContent = m.task;
  const bits = [
    `spawned ${(m.spawned_at || "").replace("T", " ")}`,
    `model ${m.model || "?"}`,
    m.session_id ? `session ${m.session_id}` : "",
    m.pid && m.busy ? `pid ${m.pid}` : "",
    m.copy_dirty ? "uncommitted work in copy" : "",
    m.merge && m.merge.version ? `merged as v${m.merge.version}` : "",
  ].filter(Boolean);
  $("#detail-meta").textContent = bits.join("   ·   ");

  const terminal = ["merged", "cancelled"].includes(m.status);
  $("#message-input").disabled = terminal;
  $("#message-form button[type=submit]").disabled = terminal;
  $("#btn-merge").disabled = !(m.status === "done" || m.status === "blocked" || m.status === "failed");
  $("#btn-cancel").disabled = !(m.busy && m.status !== "merging");
  $("#btn-cancel").title = m.status === "merging" ? "cannot cancel a running merge" : "";
}

/* ------------------------------------------------------------------ actions */

async function postTask(id, action) {
  try {
    await api(`/api/tasks/${id}/${action}`, { method: "POST" });
    refreshList(); refreshDetail();
  } catch (e) { alert(e.message); }
}

async function spawn() {
  const task = $("#spawn-task").value.trim();
  if (!task) return;
  const model = $("#spawn-model").value;
  try {
    await api("/api/tasks", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ task, model }),
    });
    $("#spawn-task").value = "";
    $("#modal-backdrop").hidden = true;
    refreshList();
  } catch (e) { alert(e.message); }
}

/* ------------------------------------------------------------------- wiring */

function initTabs() {
  $("#tabs").addEventListener("click", (e) => {
    const btn = e.target.closest("button[data-tab]");
    if (!btn) return;
    state.tab = btn.dataset.tab;
    document.querySelectorAll("#tabs button").forEach((b) =>
      b.classList.toggle("active", b === btn));
    document.querySelectorAll(".panel").forEach((p) =>
      p.hidden = p.dataset.panel !== state.tab);
    refreshDetail();
  });
  $("#log-kind").addEventListener("change", (e) => {
    state.logKind = e.target.value;
    state.log = { text: "", offset: 0 };
    refreshLog();
  });
  $("#log-raw").addEventListener("change", () => {
    state.log = { text: "", offset: 0 };
    refreshLog();
  });
}

function init() {
  const sel = $("#spawn-model");
  const loadModels = async () => {
    if (state.models.length) return;
    sel.textContent = "";
    sel.appendChild(el("option", null, "loading models…"));
    sel.disabled = true;
    let data = null;
    for (let attempt = 0; attempt < 3 && !data; attempt++) {
      if (attempt) await new Promise((r) => setTimeout(r, 800 * attempt));
      data = await api("/api/models").catch(() => null);
    }
    state.models = (data && data.models) || [];
    state.defaultModel = data && data.default;
    sel.textContent = "";
    if (!state.models.length) {
      sel.appendChild(el("option", null, "(could not load models — close and reopen to retry)"));
    } else {
      for (const m of state.models) sel.appendChild(el("option", null, m));
      if (state.defaultModel && state.models.includes(state.defaultModel))
        sel.value = state.defaultModel;
      else {
        const fb = state.models.indexOf(DEFAULT_FALLBACK);
        sel.selectedIndex = fb >= 0 ? fb : 0;
      }
    }
    sel.disabled = false;
  };
  const openModal = async () => {
    await loadModels();
    $("#modal-backdrop").hidden = false;
    $("#spawn-task").focus();
  };

  $("#new-task").onclick = openModal;
  $("#spawn-cancel").onclick = () => ($("#modal-backdrop").hidden = true);
  $("#spawn-form").onsubmit = (e) => { e.preventDefault(); spawn(); };
  $("#modal-backdrop").addEventListener("click", (e) => {
    if (e.target === e.currentTarget) $("#modal-backdrop").hidden = true;
  });

  $("#message-form").onsubmit = (e) => {
    e.preventDefault();
    const input = $("#message-input");
    const text = input.value.trim();
    if (!text || !state.selected) return;
    api(`/api/tasks/${state.selected}/message`, {
      method: "POST", headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ text }),
    }).then((r) => {
      input.value = "";
      if (r.queued) flash("queued — the agent is busy, it will get this when it goes idle");
    }).catch((err) => alert(err.message));
  };

  $("#btn-merge").onclick = () => postTask(state.selected, "merge");
  $("#btn-cancel").onclick = () => { if (confirm("Cancel this agent?")) postTask(state.selected, "cancel"); };
  $("#btn-delete").onclick = () => {
    if (confirm("Delete this task and its copy? The work in the copy is lost."))
      postTask(state.selected, "delete").then(() => selectTask(null));
  };

  $("#auto-merge").checked = true;
  api("/api/config").then((c) => { state.autoMerge = c.auto_merge; $("#auto-merge").checked = c.auto_merge; });
  $("#auto-merge").onchange = (e) => api("/api/config", {
    method: "POST", headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ auto_merge: e.target.checked }),
  });

  initTabs();
  const params = new URLSearchParams(location.search);
  if (params.get("task")) selectTask(params.get("task"));
  if (params.has("new")) openModal();
  refreshList();
  setInterval(refreshList, 2000);
  setInterval(() => { if (state.selected) refreshDetail(); }, 1500);
}

function flash(msg) {
  const t = $("#message-input");
  t.placeholder = msg;
  setTimeout(() => { t.placeholder = "Send a follow-up message to the agent…"; }, 4000);
}

document.addEventListener("keydown", (e) => {
  if (e.key === "Escape") $("#modal-backdrop").hidden = true;
});

init();
