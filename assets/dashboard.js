"use strict";

/* ⚗️ Chemical Compiler Analytics dashboard — pure static JS reading data/*.json
 *
 * Views: Overview · Releases · Daily/Commits · Failures · Modules · Backends
 * Features: light/dark theme, persisted filters, searchable/sortable tables,
 * CSV export, release comparison, run-history release modals, failure
 * aggregation, lazy platform-record loading.
 */

const $ = (sel) => document.querySelector(sel);
const $$ = (sel) => Array.from(document.querySelectorAll(sel));

const BACKENDS = ["TCCCompiler", "Compiler", "Interpreter"];

/* ── persistent + ephemeral state ───────────────────────────── */
const state = {
  manifest: null,
  daily: [],
  releases: [],
  selectedBackends: new Set(BACKENDS),
  theme: "dark",
  // release view filters
  relPlatform: null,
  relBackend: "TCCCompiler",
  relVariant: "regular",
  relSearch: "",
  relCompareA: null,
  relCompareB: null,
  // daily view filters
  dailyFrom: "",
  dailyTo: "",
  dailySearch: "",
  dailyStatus: "all",
  // modules
  modName: null,
  modBackend: "TCCCompiler",
  // failures view
  failBackend: "all",
  failSearch: "",
  failMin: 1,
  // table sort state (per table id)
  sort: {},
};

const PREFS_KEY = "chem-dashboard-prefs";
function savePrefs() {
  try {
    localStorage.setItem(PREFS_KEY, JSON.stringify({
      theme: state.theme,
      selectedBackends: Array.from(state.selectedBackends),
      relBackend: state.relBackend,
      relVariant: state.relVariant,
      relCompareA: state.relCompareA,
      relCompareB: state.relCompareB,
      modBackend: state.modBackend,
      failBackend: state.failBackend,
      failMin: state.failMin,
    }));
  } catch (e) { /* private mode etc. */ }
}
function loadPrefs() {
  try {
    const p = JSON.parse(localStorage.getItem(PREFS_KEY) || "{}");
    if (p.theme === "light" || p.theme === "dark") state.theme = p.theme;
    if (Array.isArray(p.selectedBackends) && p.selectedBackends.length) state.selectedBackends = new Set(p.selectedBackends.filter((b) => BACKENDS.includes(b)));
    if (BACKENDS.includes(p.relBackend)) state.relBackend = p.relBackend;
    if (p.relVariant === "regular" || p.relVariant === "tcc") state.relVariant = p.relVariant;
    state.relCompareA = p.relCompareA || null;
    state.relCompareB = p.relCompareB || null;
    if (BACKENDS.includes(p.modBackend)) state.modBackend = p.modBackend;
    if (p.failBackend === "all" || BACKENDS.includes(p.failBackend)) state.failBackend = p.failBackend;
    if (typeof p.failMin === "number") state.failMin = p.failMin;
  } catch (e) { /* ignore */ }
}

/* lazy caches: platform records are only fetched when a view needs them */
const platformCache = {};  // "tag/platform" → Promise<record>
const dailyCache = {};     // date → Promise<record>

const charts = {};
function destroyChart(id) {
  if (charts[id]) { charts[id].destroy(); delete charts[id]; }
}
function makeChart(id, config) {
  destroyChart(id);
  const canvas = document.getElementById(id);
  if (!canvas) return;
  try { charts[id] = new Chart(canvas.getContext("2d"), config); }
  catch (e) { console.warn("chart failed", id, e); }
}

/* ── theme ──────────────────────────────────────────────────── */
function applyTheme() {
  document.documentElement.classList.toggle("dark", state.theme === "dark");
  const btn = $("#theme-toggle");
  if (btn) btn.textContent = state.theme === "dark" ? "☀️" : "🌙";
  savePrefs();
}
function toggleTheme() {
  state.theme = state.theme === "dark" ? "light" : "dark";
  applyTheme();
  // charts read CSS vars at render time → rebuild all visible charts
  renderChartsForCurrentView();
}
function cssVar(name) {
  return getComputedStyle(document.documentElement).getPropertyValue(name).trim();
}
/* chart colors resolve from the live theme */
function chartColors() {
  return {
    tick: "hsl(" + cssVar("--chart-tick") + ")",
    grid: "hsl(" + cssVar("--chart-grid") + ")",
    tooltipBg: "hsl(" + cssVar("--tooltip-bg") + ")",
    tooltipBorder: "hsl(" + cssVar("--tooltip-border") + ")",
    text: "hsl(" + cssVar("--foreground") + ")",
    muted: "hsl(" + cssVar("--muted-foreground") + ")",
  };
}

/* ── toast notifications ────────────────────────────────────── */
function toast(msg) {
  const root = $("#toast-root");
  if (!root) return;
  const el = document.createElement("div");
  el.className = "toast";
  el.innerHTML = `<span class="dot"></span><span>${esc(msg)}</span>`;
  root.appendChild(el);
  setTimeout(() => {
    el.classList.add("out");
    setTimeout(() => el.remove(), 220);
  }, 2600);
}

/* ── small helpers ──────────────────────────────────────────── */
function setStatus(text) {
  const el = $("#data-status");
  if (!el) return;
  el.textContent = text;
  el.className = "badge " + (text === "loaded" ? "badge-ok" : "badge-loading");
  if (text !== "loaded") el.textContent = "⚠ " + text;
}

const fmtSize = (b) =>
  b == null ? "—" : b >= 1048576 ? (b / 1048576).toFixed(1) + " MB" : (b / 1024).toFixed(1) + " KB";

const fmtMs = (ms) =>
  ms == null ? "—" : ms >= 60000 ? (ms / 60000).toFixed(1) + " min" : ms >= 1000 ? (ms / 1000).toFixed(2) + " s" : ms + " ms";

const fmtDate = (s) => (s ? s.slice(0, 10) : "—");
const fmtDateTime = (s) => (s ? s.replace("T", " ").replace(/Z$/, " UTC") : "—");

function esc(s) {
  return String(s == null ? "" : s)
    .replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;").replace(/'/g, "&#39;");
}

/* CSV export of a rendered table */
function csvEscape(v) {
  const s = String(v == null ? "" : v);
  return /[",\n]/.test(s) ? '"' + s.replace(/"/g, '""') + '"' : s;
}
function exportTableCsv(tableEl, filename) {
  if (!tableEl) return;
  const rows = Array.from(tableEl.querySelectorAll("tbody tr")).map((tr) =>
    Array.from(tr.querySelectorAll("td")).map((td) => csvEscape(td.textContent.trim())));
  const heads = Array.from(tableEl.querySelectorAll("thead th")).map((th) => csvEscape(th.textContent.trim()));
  const csv = [heads.join(","), ...rows.map((r) => r.join(","))].join("\n");
  const blob = new Blob([csv], { type: "text/csv;charset=utf-8" });
  const a = document.createElement("a");
  a.href = URL.createObjectURL(blob);
  a.download = filename || "export.csv";
  document.body.appendChild(a);
  a.click();
  setTimeout(() => { URL.revokeObjectURL(a.href); a.remove(); }, 200);
  toast("Exported " + rows.length + " rows → " + a.download);
}

/* generic sortable-table wiring: <th data-sort="key"> or data-sort-num */
const boundSortables = new WeakSet(); // prevent duplicate listeners on re-render
function sortTable(tableEl, key, numeric, dir) {
  const tbody = tableEl.querySelector("tbody");
  if (!tbody) return;
  const rows = Array.from(tbody.querySelectorAll("tr"));
  const colIdx = Array.from(tableEl.querySelectorAll("thead th")).findIndex((th) => th.dataset.sort === key);
  if (colIdx < 0) return;
  rows.sort((a, b) => {
    const av = a.children[colIdx] ? a.children[colIdx].textContent.trim() : "";
    const bv = b.children[colIdx] ? b.children[colIdx].textContent.trim() : "";
    const na = parseFloat(String(av).replace(/[^0-9.\-]/g, ""));
    const nb = parseFloat(String(bv).replace(/[^0-9.\-]/g, ""));
    let cmp;
    if (numeric && !isNaN(na) && !isNaN(nb)) cmp = na - nb;
    else cmp = av.localeCompare(bv, undefined, { numeric: true });
    return dir === "desc" ? -cmp : cmp;
  });
  rows.forEach((r) => tbody.appendChild(r));
}
function bindSortable(tableEl) {
  if (boundSortables.has(tableEl)) return; // bound once per table element
  boundSortables.add(tableEl);
  tableEl.querySelectorAll("th.sortable").forEach((th) => {
    th.addEventListener("click", () => {
      const key = th.dataset.sort;
      const numeric = th.dataset.sortNum === "1";
      const cur = state.sort[key] || "asc";
      const next = cur === "asc" ? "desc" : "asc";
      state.sort[key] = next;
      tableEl.querySelectorAll("th.sortable").forEach((t) => { t.classList.remove("sorted"); t.querySelector(".sort-icon").textContent = "↕"; });
      th.classList.add("sorted");
      th.querySelector(".sort-icon").textContent = next === "asc" ? "↑" : "↓";
      sortTable(tableEl, key, numeric, next);
    });
  });
}
function sortableHeader(label, key, numeric) {
  return `<th class="sortable ${state.sort[key] ? "sorted" : ""}" data-sort="${esc(key)}" ${numeric ? 'data-sort-num="1"' : ""}>${esc(label)}<span class="sort-icon">${state.sort[key] ? (state.sort[key] === "asc" ? "↑" : "↓") : "↕"}</span></th>`;
}

/* ── labels ─────────────────────────────────────────────────── */
const STATUS_LABEL = {
  success: "success",
  missing_asset: "missing asset",
  unavailable: "unavailable",
  test_failure: "test failure",
  test_crash: "test crash",
  build_failure: "build failure",
  benchmark_failure: "benchmark failure",
  timeout: "timeout",
  failed: "failed",
};

function statusBadge(status, label) {
  const s = status || "unavailable";
  return `<span class="badge badge-${esc(s)}">${esc(label || STATUS_LABEL[s] || s)}</span>`;
}

function testStatusBadge(t) {
  if (!t) return `<span class="badge badge-neutral">no tests</span>`;
  let label = t.status || "unavailable";
  if (t.failed > 0 && (t.status === "success" || !t.status)) label = "test_failure";
  return `<span class="badge badge-${esc(label)}" title="${esc(t.reason || "")}">${esc(STATUS_LABEL[label] || label)}</span>`;
}

function failedTestsHtml(t, limit) {
  if (!t) return "";
  const items = [];
  for (const f of (t.failed_tests || [])) {
    const crash = (t.crashed_tests || []).find((c) => c.name === f);
    const timed = (t.timed_out_tests || []).includes(f);
    const tag = crash ? ` <span class="chip up" title="exit code ${esc(crash.exit_code)}">💥 ${esc(crash.exit_code)}</span>`
      : timed ? ` <span class="chip up" title="timed out after 10s">⏱ timed out</span>`
      : "";
    items.push(`<li class="missing">${esc(f)}${tag}</li>`);
  }
  if (!items.length) return "";
  let html = `<ul class="flat">${items.slice(0, limit).join("")}</ul>`;
  if (limit && items.length > limit) {
    html += `<details class="summary-box"><summary>${items.length} failing tests…</summary><ul class="flat">${items.slice(limit).join("")}</ul></details>`;
  }
  return html;
}

function regressChip(current, prev) {
  if (current == null || prev == null || prev === 0) return "";
  const diff = ((current - prev) / prev) * 100;
  const cls = Math.abs(diff) < 0.5 ? "flat" : diff > 0 ? "up" : "down";
  const sign = diff > 0 ? "+" : "";
  const icon = cls === "flat" ? "•" : cls === "up" ? "▲" : "▼";
  return `<span class="chip ${cls}" title="${esc(prev.toFixed(2))} → ${esc(current.toFixed(2))}">${icon} ${sign}${diff.toFixed(1)}%</span>`;
}

/* asset-name → (platform, arch, variant) — mirrors scripts/common.sh */
function assetParts(name) {
  let base = name.replace(/\.zip$/, "");
  let variant = "regular";
  if (base.endsWith("-tcc")) { variant = "tcc"; base = base.slice(0, -4); }
  let platform = "", arch = "";
  if (base.startsWith("linux-alpine-")) { platform = "linux-alpine"; arch = base.slice(13); }
  else if (base.startsWith("linux-")) { platform = "linux"; arch = base.slice(6); }
  else if (base.startsWith("macos-")) { platform = "macos"; arch = base.slice(6); }
  else if (base.startsWith("windows-mingw-")) { platform = "windows-mingw"; arch = base.slice(14); }
  else if (base.startsWith("windows-")) { platform = "windows"; arch = base.slice(8); }
  return { platform, arch, variant };
}
function assetNameFor(platform, arch, variant) {
  return `${platform}-${arch}${variant === "tcc" ? "-tcc" : ""}.zip`;
}

const PLATFORM_LABEL = {
  linux: "Linux", "linux-alpine": "Alpine", macos: "macOS",
  windows: "Windows", "windows-mingw": "Windows (MinGW)",
};
const BACKEND_LABEL = { TCCCompiler: "TCC", Compiler: "LLVM", Interpreter: "Interpreter" };

/* ── data access ────────────────────────────────────────────── */
function backendOf(dailyRec, backend) {
  return (dailyRec.backends && dailyRec.backends[backend]) || null;
}
function helloOf(rec, name) {
  if (!rec || !Array.isArray(rec.benchmarks)) return null;
  return rec.benchmarks.find((b) => b.name === name) || null;
}
function testsOf(rec) { return (rec && rec.tests) || null; }

async function fetchJson(url) {
  const r = await fetch(url);
  if (!r.ok) throw new Error(url + " → " + r.status);
  return r.json();
}

function loadDailyRecord(date) {
  if (!dailyCache[date]) {
    dailyCache[date] = fetchJson("data/daily/" + date + ".json").catch((e) => { console.warn("missing daily", date); return null; });
  }
  return dailyCache[date];
}
function loadPlatformRecord(tag, pf) {
  const key = tag + "/" + pf;
  if (!platformCache[key]) {
    platformCache[key] = fetchJson(`data/releases/${tag}/${pf}.json`).catch((e) => { console.warn("missing platform", key); return null; });
  }
  return platformCache[key];
}
async function ensurePlatformRecord(rec, pf) {
  if (rec.platforms[pf]) return rec.platforms[pf];
  const r = await loadPlatformRecord(rec.info.tag, pf);
  if (r) rec.platforms[pf] = r;
  return r;
}
async function ensurePlatformRecords(rec) {
  const manifestPlatforms = (state.manifest.releases || {})[rec.info.tag] || [];
  const missing = manifestPlatforms.filter((pf) => !rec.platforms[pf]);
  if (!missing.length) return rec;
  const results = await Promise.all(missing.map((pf) =>
    loadPlatformRecord(rec.info.tag, pf).then((r) => ({ pf, r }))));
  for (const { pf, r } of results) if (r) rec.platforms[pf] = r;
  return rec;
}

async function init() {
  try {
    loadPrefs();
    applyTheme();
    state.manifest = await fetchJson("data/manifest.json");
    setStatus("loading…");
    const tasks = [];
    for (const d of state.manifest.daily || []) {
      tasks.push(loadDailyRecord(d).then((rec) => { if (rec) state.daily.push(rec); }));
    }
    for (const [tag] of Object.entries(state.manifest.releases || {})) {
      tasks.push(fetchJson(`data/releases/${tag}/info.json`).then((info) => {
        state.releases.push({ info, platforms: {} });
      }).catch((e) => { console.warn("missing release", tag); }));
    }
    await Promise.all(tasks);
    state.daily.sort((a, b) => (a.date < b.date ? -1 : a.date > b.date ? 1 : 0));
    state.releases.sort((a, b) => {
      const da = a.info.published_at || "", db = b.info.published_at || "";
      if (da && db) return da < db ? -1 : 1;
      return a.info.tag < b.info.tag ? -1 : 1;
    });
    setStatus("loaded");
    render();
  } catch (e) {
    setStatus("load failed: " + e.message);
    console.error(e);
  }
}

/* ── view rendering ─────────────────────────────────────────── */
function render() {
  renderOverview();
  renderReleases();
  renderDaily();
  renderFailures();
  renderModules();
  renderBackends();
}

function renderChartsForCurrentView() {
  const active = $(".view.active");
  if (!active) return;
  const id = active.id;
  if (id === "view-overview") renderOverviewCharts();
  else if (id === "view-releases") renderReleaseCharts();
  else if (id === "view-daily") renderDailyCharts();
  else if (id === "view-failures") renderFailureCharts();
  else if (id === "view-modules") renderModuleChart();
  else if (id === "view-backends") renderBackendCharts();
}

/* ══ Overview ═════════════════════════════════════════════════ */

function renderOverview() {
  const el = $("#view-overview");
  const latest = state.daily[state.daily.length - 1] || null;
  const lastRel = state.releases[state.releases.length - 1] || null;

  // overall pass rate across all daily records (any backend with results)
  let allPassed = 0, allTotal = 0, allFailed = 0;
  for (const d of state.daily) {
    for (const b of BACKENDS) {
      const t = testsOf(backendOf(d, b));
      if (t && t.total) { allPassed += t.passed || 0; allTotal += t.total; allFailed += t.failed || 0; }
    }
  }
  const passRate = allTotal ? (100 * allPassed / allTotal).toFixed(1) : null;

  let html = `<div class="grid grid-4 section">
    <div class="stat"><div class="label">Releases tracked</div><div class="value">${state.releases.length}</div><div class="sub">${lastRel ? "latest " + esc(lastRel.info.tag) : ""}</div></div>
    <div class="stat"><div class="label">Daily points</div><div class="value">${state.daily.length}</div><div class="sub">${latest ? "latest " + esc(latest.date) : ""}</div></div>
    <div class="stat"><div class="label">Tests run</div><div class="value ${allFailed > 0 ? "bad" : "good"}">${allTotal ? allTotal.toLocaleString() : "—"}</div><div class="sub">${passRate != null ? passRate + "% pass rate across all runs" : ""}</div></div>
    <div class="stat"><div class="label">Failing tests</div><div class="value ${allFailed > 0 ? "bad" : "good"}">${allFailed}</div><div class="sub">across all daily runs</div></div>
  </div>`;

  if (latest) {
    html += `<div class="section"><h2>Latest daily build — <span class="mono">${esc(latest.date)}</span> <span class="dim">${esc(latest.commit && latest.commit.subject)}</span></h2>`;
    html += `<div class="card"><div class="table-wrap"><table>
      <thead><tr><th>Backend</th><th>Build</th><th>Tests</th><th class="num">Passed</th><th class="num">Failed</th><th class="num">Duration</th></tr></thead><tbody>`;
    for (const b of BACKENDS) {
      const rec = backendOf(latest, b);
      if (!rec) { html += `<tr><td>${esc(BACKEND_LABEL[b])}</td><td colspan="5" class="dim">no data</td></tr>`; continue; }
      const t = rec.tests || {};
      html += `<tr class="${t.failed > 0 ? "fail-row" : ""}">
        <td><b>${esc(BACKEND_LABEL[b])}</b></td>
        <td>${statusBadge(rec.build.status, "")}</td>
        <td>${testStatusBadge(t)}${t.complete === false ? ` <span class="chip up" title="run ended before summary — possible crash">⚠ incomplete</span>` : ""}</td>
        <td class="num">${t.passed != null ? t.passed : "—"}</td>
        <td class="num ${t.failed > 0 ? "missing" : ""}">${t.failed != null ? t.failed : "—"}</td>
        <td class="num">${fmtMs(t.duration_ms)}</td>
      </tr>`;
      const fh = failedTestsHtml(t, 12);
      if (fh) html += `<tr class="sub-row"><td></td><td colspan="5" class="dim"><b>Failed:</b> ${fh}</td></tr>`;
    }
    html += `</tbody></table></div></div></div>`;
  }

  if (lastRel) {
    html += `<div class="section"><h2>Latest release — <span class="mono">${esc(lastRel.info.tag)}</span> <button class="btn btn-sm btn-outline" id="ov-open-rel">view full report</button></h2>`;
    html += `<div class="card"><div class="table-wrap"><table><thead><tr><th>Platform</th><th class="num">Size</th><th>Status</th></tr></thead><tbody>`;
    for (const [asset, a] of Object.entries(lastRel.info.assets || {})) {
      const p = assetParts(asset);
      if (p.variant !== "regular") continue;
      html += `<tr><td>${esc(PLATFORM_LABEL[p.platform] || p.platform)} ${esc(p.arch)} <span class="dim">(${esc(asset)})</span></td>
        <td class="num">${a.status === "success" ? fmtSize(a.size_bytes) : "—"}</td>
        <td>${statusBadge(a.status, a.status === "missing_asset" ? "missing asset" : "")}</td></tr>`;
    }
    html += `</tbody></table></div></div>`;
    html += `<div class="card" id="ov-rel-tests"><div class="note">loading test results…</div></div></div>`;
    loadLatestReleaseTests(lastRel);
  }

  if (state.daily.length > 1) {
    html += `<div class="section"><h2>Hello world compile time (daily trend)</h2><div class="card"><canvas id="ov-hello"></canvas></div></div>`;
  }
  if (state.releases.length > 1) {
    html += `<div class="section"><h2>Hello world compile time per release <span class="hint">(std-lib hello — spot release regressions)</span></h2><div class="card"><canvas id="ov-rel-hello"></canvas></div></div>`;
  }

  el.innerHTML = html;
  const openBtn = $("#ov-open-rel");
  if (openBtn) openBtn.addEventListener("click", () => openReleaseModal(lastRel.info.tag));
  el.classList.contains("active") && renderOverviewCharts();
}

async function loadLatestReleaseTests(rel) {
  const box = document.getElementById("ov-rel-tests");
  if (!box) return;
  const pk = mostCoveredPlatform();
  if (!pk) { box.innerHTML = `<div class="note">no platform records available</div>`; return; }
  const rec = await ensurePlatformRecord(rel, pk);
  if (!rec || !rec.backends) { box.innerHTML = `<div class="note">no test results recorded for ${esc(pk)}</div>`; return; }
  let rows = "";
  for (const b of BACKENDS) {
    const r = rec.backends[b];
    if (!r) continue;
    const t = r.tests || {};
    rows += `<tr class="${t.failed > 0 ? "fail-row" : ""}">
      <td><b>${esc(BACKEND_LABEL[b])}</b> ${statusBadge(r.build.status, "")}</td>
      <td>${testStatusBadge(t)}${t.complete === false ? ` <span class="chip up" title="run ended before summary — possible crash">⚠ incomplete</span>` : ""}</td>
      <td class="num">${t.passed != null ? t.passed : "—"}</td>
      <td class="num ${t.failed > 0 ? "missing" : ""}">${t.failed != null ? t.failed : "—"}</td>
      <td class="num">${fmtMs(t.duration_ms)}</td>
    </tr>`;
    const fh = failedTestsHtml(t, 8);
    if (fh) rows += `<tr class="sub-row"><td></td><td colspan="4" class="dim"><b>Failed:</b> ${fh}</td></tr>`;
  }
  if (!rows) { box.innerHTML = `<div class="note">no test results for ${esc(pk)}</div>`; return; }
  box.innerHTML = `<h3>Test results — ${esc(pk)}</h3><div class="table-wrap"><table>
    <thead><tr><th>Backend</th><th>Tests</th><th class="num">Passed</th><th class="num">Failed</th><th class="num">Duration</th></tr></thead><tbody>${rows}</tbody></table></div>`;
}

function renderOverviewCharts() {
  const labels = state.daily.map((d) => d.date);
  const datasets = [];
  for (const b of BACKENDS) {
    const data = state.daily.map((d) => {
      const h = helloOf(backendOf(d, b), "hello_bare");
      return h && h.status === "success" ? h.duration_ms : null;
    });
    if (data.some((v) => v != null)) datasets.push({ label: BACKEND_LABEL[b], data, borderColor: colorOf(b), backgroundColor: colorOf(b), spanGaps: true, tension: 0.25, pointRadius: 2 });
  }
  makeChart("ov-hello", { type: "line", data: { labels, datasets }, options: baseLineOpts("compilation ms") });

  if (state.releases.length > 1) {
    const pk = "linux-x64";
    Promise.all(state.releases.map((r) => ensurePlatformRecord(r, pk))).then(() => {
      const relTags = state.releases.map((x) => x.info.tag);
      const fresh = [];
      for (const b of BACKENDS) {
        const data = state.releases.map((r) => {
          const pf = r.platforms && r.platforms[pk];
          const rec = pf && pf.backends && pf.backends[b];
          const h = helloOf(rec, "hello_std");
          return h && h.status === "success" ? h.duration_ms : null;
        });
        if (data.some((v) => v != null)) fresh.push({ label: BACKEND_LABEL[b] + " std", data, borderColor: colorOf(b), backgroundColor: colorOf(b), spanGaps: false, tension: 0.2, pointRadius: 2 });
      }
      if (fresh.length) makeChart("ov-rel-hello", { type: "line", data: { labels: relTags, datasets: fresh }, options: baseLineOpts("compilation ms") });
    });
  }
}

/* ══ Releases ═════════════════════════════════════════════════ */

function releasePlatformsForChart() {
  const set = new Set();
  for (const pf of Object.values(state.manifest.releases || {})) {
    for (const k of pf) set.add(k);
  }
  return Array.from(set).sort();
}
function mostCoveredPlatform() {
  const counts = {};
  for (const pf of Object.values(state.manifest.releases || {})) {
    for (const k of pf) counts[k] = (counts[k] || 0) + 1;
  }
  let best = null, bestCount = -1;
  for (const [k, c] of Object.entries(counts)) {
    if (c > bestCount) { best = k; bestCount = c; }
  }
  return best;
}

function renderReleases() {
  const el = $("#view-releases");
  if (!state.releases.length) { el.innerHTML = `<div class="note">No release data yet.</div>`; return; }

  const platforms = releasePlatformsForChart();
  if (!state.relPlatform || !platforms.includes(state.relPlatform)) state.relPlatform = mostCoveredPlatform();
  // validate persisted compare tags against releases that still exist
  const validTags = new Set(state.releases.map((r) => r.info.tag));
  if (state.relCompareA && !validTags.has(state.relCompareA)) state.relCompareA = null;
  if (state.relCompareB && !validTags.has(state.relCompareB)) state.relCompareB = null;

  // platform record loading indicator
  let html = `<div class="filters">
    <label class="filter-label">Platform <select id="rel-platform-select">${platforms.map((p) => `<option value="${esc(p)}"${p === state.relPlatform ? " selected" : ""}>${esc(p)}</option>`).join("")}</select></label>
    <label class="filter-label">Backend <select id="rel-backend-select">${BACKENDS.map((b) => `<option value="${esc(b)}"${b === state.relBackend ? " selected" : ""}>${esc(BACKEND_LABEL[b])}</option>`).join("")}</select></label>
    <label class="filter-label">Variant <select id="rel-variant-select"><option value="regular"${state.relVariant === "regular" ? " selected" : ""}>regular</option><option value="tcc"${state.relVariant === "tcc" ? " selected" : ""}>tcc</option></select></label>
    <div class="search-box"><input type="search" id="rel-search" placeholder="Filter releases…" value="${esc(state.relSearch)}" aria-label="Filter releases"></div>
    <button class="btn btn-sm" id="rel-export">⬇ CSV</button>
    <span id="rel-loading" class="dim" style="display:none">loading platform data…</span>
  </div>`;

  html += `<div class="grid grid-2 section">
    <div class="card"><h3>Binary size — ${esc(state.relPlatform)} ${esc(state.relVariant)}</h3><canvas id="rel-size-bar"></canvas></div>
    <div class="card"><h3>Binary size over time — all platforms</h3><canvas id="rel-size-line"></canvas></div>
    <div class="card"><h3>Hello world compile time per release</h3><canvas id="rel-hello"></canvas></div>
    <div class="card"><h3>Release test results <span class="hint">${esc(BACKEND_LABEL[state.relBackend])} · ${esc(state.relPlatform)}</span></h3><canvas id="rel-tests"></canvas></div>
  </div>`;

  // compare releases card
  const tags = state.releases.map((r) => r.info.tag);
  html += `<div class="section"><h2>Compare releases <span class="hint">assets + test results side by side</span></h2><div class="card">
    <div class="filters" style="box-shadow:none;border:none;padding:0;margin:0 0 12px">
      <label class="filter-label">A <select id="rel-cmp-a">${tags.map((t) => `<option value="${esc(t)}"${t === (state.relCompareA || tags[tags.length - 1]) ? " selected" : ""}>${esc(t)}</option>`).join("")}</select></label>
      <label class="filter-label">B <select id="rel-cmp-b">${tags.map((t) => `<option value="${esc(t)}"${t === (state.relCompareB || tags[tags.length - 2]) ? " selected" : ""}>${esc(t)}</option>`).join("")}</select></label>
      <span class="dim">latest platform: ${esc(state.relPlatform)}</span>
    </div>
    <div id="rel-cmp-body"><div class="note">select two releases to compare</div></div>
  </div></div>`;

  html += `<div class="section"><h2>All releases <span class="hint">click a row for the full report · sizes in MB</span></h2>
    <div class="table-wrap"><table id="rel-table" class="rel-table">
    <thead><tr>${sortableHeader("Version", "version", false)}${sortableHeader("Published", "published", false)}${sortableHeader("Windows x64", "win", true)}${sortableHeader("Linux x64", "linux", true)}${sortableHeader("Alpine x64", "alpine", true)}${sortableHeader("macOS ARM", "mac", true)}${sortableHeader("macOS x64", "macx64", true)}</tr></thead>
    <tbody></tbody></table></div></div>`;
  el.innerHTML = html;

  // event wiring
  const ps = $("#rel-platform-select"), bs = $("#rel-backend-select"), vs = $("#rel-variant-select");
  ps.addEventListener("change", () => { state.relPlatform = ps.value; savePrefs(); renderReleaseCharts(); });
  bs.addEventListener("change", () => { state.relBackend = bs.value; savePrefs(); renderReleaseCharts(); });
  vs.addEventListener("change", () => { state.relVariant = vs.value; savePrefs(); renderReleaseCharts(); });
  const rs = $("#rel-search");
  rs.addEventListener("input", () => { state.relSearch = rs.value; renderReleaseTable(); });
  $("#rel-export").addEventListener("click", () => exportTableCsv($("#rel-table"), "releases.csv"));

  // compare
  const ca = $("#rel-cmp-a"), cb = $("#rel-cmp-b");
  ca.addEventListener("change", () => { state.relCompareA = ca.value; savePrefs(); renderCompare(); });
  cb.addEventListener("change", () => { state.relCompareB = cb.value; savePrefs(); renderCompare(); });

  renderReleaseTable();
  renderCompare();
  renderReleaseCharts();
}

function renderReleaseTable() {
  const tableEl = $("#rel-table");
  if (!tableEl) return;
  const tbody = tableEl.querySelector("tbody");
  const q = state.relSearch.toLowerCase();
  const cols = [
    { key: "win", pf: "windows", arch: "x64" },
    { key: "linux", pf: "linux", arch: "x64" },
    { key: "alpine", pf: "linux-alpine", arch: "x64" },
    { key: "mac", pf: "macos", arch: "arm64" },
    { key: "macx64", pf: "macos", arch: "x64" },
  ];
  let rows = "";
  let count = 0;
  for (const r of state.releases.slice().reverse()) {
    const tag = r.info.tag;
    if (q && !tag.toLowerCase().includes(q)) continue;
    count++;
    rows += `<tr class="clickable" data-tag="${esc(tag)}">`;
    rows += `<td><b>${esc(tag)}</b></td><td class="dim">${esc(fmtDate(r.info.published_at))}</td>`;
    for (const c of cols) {
      const name = assetNameFor(c.pf, c.arch, "regular");
      const a = r.info.assets && r.info.assets[name];
      let cell = `<td class="num dim">—</td>`;
      if (a) {
        if (a.status === "success") cell = `<td class="num">${fmtSize(a.size_bytes)}</td>`;
        else if (a.status === "missing_asset") cell = `<td class="num"><span class="missing" title="asset not published for ${esc(tag)}">MISSING</span></td>`;
        else cell = `<td class="num"><span class="missing" title="${esc(STATUS_LABEL[a.status] || a.status)}">${esc(STATUS_LABEL[a.status] || a.status)}</span></td>`;
      }
      rows += cell;
    }
    rows += `</tr>`;
  }
  tbody.innerHTML = rows || `<tr class="empty-row"><td colspan="7">No releases match “${esc(state.relSearch)}”</td></tr>`;
  tbody.querySelectorAll("tr.clickable").forEach((tr) => {
    tr.addEventListener("click", () => openReleaseModal(tr.dataset.tag));
  });
  bindSortable(tableEl);
  // re-apply current sort if any
  const sortedKey = Object.keys(state.sort).find((k) => tableEl.querySelector(`th[data-sort="${k}"]`));
  if (sortedKey) {
    const th = tableEl.querySelector(`th[data-sort="${sortedKey}"]`);
    sortTable(tableEl, sortedKey, th.dataset.sortNum === "1", state.sort[sortedKey]);
  }
}

const MB = (b) => (b == null ? null : b / 1048576);

async function renderReleaseCharts() {
  const ps = $("#rel-platform-select"), bs = $("#rel-backend-select"), vs = $("#rel-variant-select");
  if (!ps || !bs || !vs) return;
  const platformKey = ps.value, backend = bs.value, variant = vs.value;
  if (!platformKey) return;
  const pp = platformKey.split("-");
  let platform, arch;
  if (pp[0] === "linux-alpine") { platform = "linux-alpine"; arch = pp[1]; }
  else if (pp[0] === "windows-mingw") { platform = "windows-mingw"; arch = pp[1]; }
  else { platform = pp[0]; arch = pp[1]; }
  const assetName = assetNameFor(platform, arch, variant);
  const loading = $("#rel-loading");
  if (loading) loading.style.display = "inline";

  await Promise.all(state.releases.map((r) => ensurePlatformRecord(r, platformKey)));
  if (loading) loading.style.display = "none";

  const tags = state.releases.map((r) => r.info.tag);
  const sizes = state.releases.map((r) => {
    const a = r.info.assets && r.info.assets[assetName];
    return a && a.status === "success" ? MB(a.size_bytes) : null;
  });

  makeChart("rel-size-bar", {
    type: "bar",
    data: { labels: tags, datasets: [{ label: platformKey + " " + variant, data: sizes, backgroundColor: hslAlpha(colorOf("Compiler"), 0.55), borderRadius: 4 }] },
    options: barOpts("MB"),
  });

  const lineSets = [];
  for (const pk of releasePlatformsForChart()) {
    const p = pk.split("-");
    const pl = p[0] === "linux-alpine" ? "linux-alpine" : p[0] === "windows-mingw" ? "windows-mingw" : p[0];
    const an = assetNameFor(pl, p[1], variant);
    const data = state.releases.map((r) => {
      const a = r.info.assets && r.info.assets[an];
      return a && a.status === "success" ? MB(a.size_bytes) : null;
    });
    if (data.some((v) => v != null)) lineSets.push({ label: pk + " (" + variant + ")", data, borderColor: colorFor(pk), spanGaps: false, tension: 0.2, pointRadius: 2 });
  }
  makeChart("rel-size-line", { type: "line", data: { labels: tags, datasets: lineSets }, options: baseLineOpts("MB") });

  const helloSets = [];
  for (const kind of ["hello_bare", "hello_std"]) {
    const data = state.releases.map((r) => {
      const pf = r.platforms && r.platforms[platformKey];
      const rec = pf && pf.backends && pf.backends[backend];
      const h = helloOf(rec, kind);
      return h && h.status === "success" ? h.duration_ms : null;
    });
    helloSets.push({ label: kind === "hello_bare" ? "bare" : "std", data, borderColor: kind === "hello_bare" ? colorOf("TCCCompiler") : colorOf("Interpreter"), spanGaps: false, tension: 0.2, pointRadius: 2 });
  }
  makeChart("rel-hello", { type: "line", data: { labels: tags, datasets: helloSets }, options: baseLineOpts("compilation ms") });

  const passSets = [], failSets = [];
  for (const r of state.releases) {
    const pf = r.platforms && r.platforms[platformKey];
    const t = pf && pf.backends && pf.backends[backend] && pf.backends[backend].tests;
    passSets.push(t && t.passed != null ? t.passed : null);
    failSets.push(t && t.failed != null ? t.failed : null);
  }
  makeChart("rel-tests", {
    type: "bar",
    data: {
      labels: tags,
      datasets: [
        { label: "passed", data: passSets, backgroundColor: hslAlpha(colorOf("TCCCompiler"), 0.6), borderRadius: 4 },
        { label: "failed", data: failSets, backgroundColor: hslAlpha(colorOf("Interpreter"), 0.75), borderRadius: 4 },
      ],
    },
    options: barOpts("tests"),
  });
}

/* convert #rrggbb → hsl() with alpha (for theme-independent chart fills) */
function hslAlpha(hex, alpha) {
  const m = /^#?([0-9a-f]{6})$/i.exec(hex);
  if (!m) return hex;
  const n = parseInt(m[1], 16);
  const r = (n >> 16) & 255, g = (n >> 8) & 255, b = n & 255;
  return `hsla(${((r / 255) * 360).toFixed(0)} ${((g / 255) * 100).toFixed(0)}% ${((b / 255) * 100).toFixed(0)}% / ${alpha})`;
}

async function renderCompare() {
  const box = $("#rel-cmp-body");
  if (!box) return;
  const ca = $("#rel-cmp-a"), cb = $("#rel-cmp-b");
  if (!ca || !cb) return;
  const aTag = ca.value, bTag = cb.value;
  if (!aTag || !bTag || aTag === bTag) { box.innerHTML = `<div class="note">pick two different releases to compare</div>`; return; }
  const ra = state.releases.find((x) => x.info.tag === aTag);
  const rb = state.releases.find((x) => x.info.tag === bTag);
  if (!ra || !rb) return;
  const mySeq = ++compareSeq;
  box.innerHTML = `<div class="note">loading…</div>`;
  const pk = state.relPlatform || mostCoveredPlatform();
  const [pa, pb] = await Promise.all([ensurePlatformRecord(ra, pk).catch(() => null), ensurePlatformRecord(rb, pk).catch(() => null)]);
  if (mySeq !== compareSeq) return; // a newer selection superseded this one

  // assets side by side (regular variant only, union of names)
  const assetNames = new Set([...Object.keys(ra.info.assets || {}), ...Object.keys(rb.info.assets || {})]);
  const regularAssets = Array.from(assetNames).filter((n) => assetParts(n).variant === "regular").sort();
  let rows = "";
  for (const name of regularAssets) {
    const a1 = ra.info.assets && ra.info.assets[name];
    const a2 = rb.info.assets && rb.info.assets[name];
    const cell = (a) => a ? (a.status === "success" ? fmtSize(a.size_bytes) : `<span class="missing">${esc(STATUS_LABEL[a.status] || a.status)}</span>`) : `<span class="dim">—</span>`;
    rows += `<tr><td>${esc(name)}</td><td class="num">${cell(a1)}</td><td class="num">${cell(a2)}</td></tr>`;
  }
  // test results for the selected platform + TCC backend (most comparable)
  const testRow = (rec) => {
    if (!rec || !rec.backends || !rec.backends.TCCCompiler) return `<span class="dim">—</span>`;
    const t = rec.backends.TCCCompiler.tests || {};
    return `${testStatusBadge(t)} <span class="num">${t.passed != null ? t.passed + "/" + t.failed : "—"}</span> <span class="dim">${fmtMs(t.duration_ms)}</span>`;
  };
  const html = `<div class="table-wrap"><table>
    <thead><tr><th></th><th class="num">${esc(aTag)}</th><th class="num">${esc(bTag)}</th></tr></thead>
    <tbody>
      <tr><td><b>Assets</b></td><td colspan="2" class="dim">${regularAssets.length} regular-platform assets each</td></tr>
      ${rows}
      <tr><td><b>TCC tests — ${esc(pk)}</b></td><td>${testRow(pa)}</td><td>${testRow(pb)}</td></tr>
    </tbody></table></div>
    <div class="note">hello benchmarks + per-backend details: open each release's full report (click its row in All releases).</div>`;
  box.innerHTML = html;
}

/* sequence token so rapid compare-select changes can't race (stale response
 * must not overwrite the pair after a newer selection) */
let compareSeq = 0;

async function openReleaseModal(tag) {
  const r = state.releases.find((x) => x.info && x.info.tag === tag);
  if (!r) return;
  try {
    await ensurePlatformRecords(r);
  } catch (e) {
    console.warn("failed to load platform records for " + tag, e);
    r.platforms = r.platforms || {};
  }

  const assets = r.info.assets || {};
  const assetEntries = Object.entries(assets);
  const okCount = assetEntries.filter(([, a]) => a.status === "success").length;
  const missCount = assetEntries.filter(([, a]) => a.status === "missing_asset").length;
  const otherCount = assetEntries.length - okCount - missCount;

  let html = `<h2>${esc(tag)}</h2>
    <div class="modal-sub">${esc(r.info.name || "")} · published ${esc(fmtDateTime(r.info.published_at))} · commit <span class="mono">${esc(r.info.commit_sha || "—")}</span></div>
    <div class="note">Assets collected ${esc(fmtDateTime(r.info.generated_at))} · ${assetEntries.length} assets total (${okCount} available, ${missCount} missing${otherCount ? `, ${otherCount} other` : ""})</div>`;

  html += `<h3>Assets (${assetEntries.length})</h3><div class="table-wrap"><table><thead><tr><th>Asset</th><th>Platform</th><th>Arch</th><th>Variant</th><th class="num">Size</th><th>Status</th></tr></thead><tbody>`;
  for (const [name, a] of assetEntries) {
    const p = assetParts(name);
    html += `<tr><td>${esc(name)}</td><td>${esc(PLATFORM_LABEL[p.platform] || p.platform)}</td><td>${esc(p.arch)}</td><td>${esc(p.variant)}</td>
      <td class="num">${a.status === "success" ? fmtSize(a.size_bytes) : "—"}</td><td>${statusBadge(a.status, a.status === "missing_asset" ? "missing asset" : "")}</td></tr>`;
  }
  html += `</tbody></table></div>`;

  const pkeys = Object.keys(r.platforms || {}).sort();
  for (const pk of pkeys) {
    const pf = r.platforms[pk];
    const bks = pf.backends || {};
    const hasAny = BACKENDS.some((b) => bks[b]);
    html += `<h3>${esc(pk)} <span class="dim">${esc(pf.libc || "")}</span> ${statusBadge(pf.status, "")}</h3>`;
    if (pf.generated_at) html += `<div class="dim">benchmarked ${esc(fmtDateTime(pf.generated_at))}${pf.reason ? ` · ${esc(pf.reason)}` : ""}</div>`;
    if (!hasAny) { html += `<div class="note">no benchmark/test data for this platform</div>`; continue; }
    html += `<div class="table-wrap"><table><thead><tr><th>Backend</th><th>Build</th><th class="num">Hello bare</th><th class="num">Hello std</th><th>Tests</th><th class="num">Passed</th><th class="num">Failed</th><th class="num">Duration</th></tr></thead><tbody>`;
    for (const b of BACKENDS) {
      const rec = bks[b];
      if (!rec) { html += `<tr><td>${esc(BACKEND_LABEL[b])}</td><td colspan="7" class="dim">no data</td></tr>`; continue; }
      const t = rec.tests || {};
      const hb = helloOf(rec, "hello_bare");
      const hs = helloOf(rec, "hello_std");
      const helloCell = (h) => h ? (h.status === "success" ? fmtMs(h.duration_ms) : statusBadge(h.status)) : "—";
      html += `<tr class="${t.failed > 0 ? "fail-row" : ""}">
        <td><b>${esc(BACKEND_LABEL[b])}</b></td>
        <td>${statusBadge(rec.build.status, "")}</td>
        <td class="num">${helloCell(hb)}</td>
        <td class="num">${helloCell(hs)}</td>
        <td>${testStatusBadge(t)}${t.complete === false ? ` <span class="chip up" title="run ended before summary — possible crash">⚠ incomplete</span>` : ""}</td>
        <td class="num">${t.passed != null ? t.passed : "—"}</td>
        <td class="num ${t.failed > 0 ? "missing" : ""}">${t.failed != null ? t.failed : "—"}</td>
        <td class="num">${fmtMs(t.duration_ms)}</td>
      </tr>`;
      const fh = failedTestsHtml(t, 20);
      if (fh) html += `<tr class="sub-row"><td></td><td colspan="7" class="dim"><b>Failed:</b> ${fh}</td></tr>`;
    }
    html += `</tbody></table></div>`;
  }

  const runPlats = pkeys.filter((pk) => {
    const pf = r.platforms[pk];
    return Array.isArray(pf.runs) && pf.runs.length >= 1;
  });
  if (runPlats.length) {
    html += `<h3>Run history <span class="hint">every benchmark/test run recorded for this release</span></h3>`;
    for (const pk of runPlats) {
      const pf = r.platforms[pk];
      const runs = pf.runs.slice().reverse();
      html += `<h4>${esc(pk)} — ${runs.length} run${runs.length > 1 ? "s" : ""}</h4><div class="table-wrap"><table><thead><tr><th>Run time</th>${BACKENDS.map((b) => `<th class="num">${esc(BACKEND_LABEL[b])} pass/fail</th>`).join("")}<th>Status</th></tr></thead><tbody>`;
      for (const run of runs) {
        html += `<tr><td class="dim">${esc(fmtDateTime(run.generated_at))}</td>`;
        for (const b of BACKENDS) {
          const rb = run.backends && run.backends[b];
          const rt = rb && rb.tests;
          html += `<td class="num">${rt && rt.passed != null ? `${rt.passed}<span class="${rt.failed > 0 ? "missing" : ""}">/${rt.failed}</span>` : "—"}</td>`;
        }
        html += `<td>${statusBadge(run.status, "")}${run.reason ? ` <span class="dim">${esc(run.reason)}</span>` : ""}</td></tr>`;
      }
      html += `</tbody></table></div>`;
    }
  }

  openModal(html);
}

/* ══ Daily / commits ═══════════════════════════════════════════ */

function filteredDaily() {
  let list = state.daily;
  if (state.dailyFrom) list = list.filter((d) => d.date >= state.dailyFrom);
  if (state.dailyTo) list = list.filter((d) => d.date <= state.dailyTo);
  if (state.dailySearch) {
    const q = state.dailySearch.toLowerCase();
    list = list.filter((d) =>
      (d.date || "").includes(q) ||
      ((d.commit && d.commit.subject) || "").toLowerCase().includes(q) ||
      ((d.commit && d.commit.short) || "").toLowerCase().includes(q));
  }
  if (state.dailyStatus !== "all") {
    list = list.filter((d) => {
      if (state.dailyStatus === "failure") {
        return BACKENDS.some((b) => { const t = testsOf(backendOf(d, b)); return t && t.failed > 0; });
      }
      return (d.status || "") === state.dailyStatus;
    });
  }
  return list;
}

function renderDaily() {
  const el = $("#view-daily");
  if (!state.daily.length) { el.innerHTML = `<div class="note">No daily data yet — the daily workflow and backfill populate this.</div>`; return; }

  const minDate = state.daily[0].date, maxDate = state.daily[state.daily.length - 1].date;

  let html = `<div class="filters">
    <label class="filter-label"><input type="checkbox" class="bk-check" value="TCCCompiler" ${state.selectedBackends.has("TCCCompiler") ? "checked" : ""}> TCC</label>
    <label class="filter-label"><input type="checkbox" class="bk-check" value="Compiler" ${state.selectedBackends.has("Compiler") ? "checked" : ""}> LLVM</label>
    <label class="filter-label"><input type="checkbox" class="bk-check" value="Interpreter" ${state.selectedBackends.has("Interpreter") ? "checked" : ""}> Interpreter</label>
    <label class="filter-label">From <input type="date" id="daily-from" value="${esc(state.dailyFrom)}" min="${esc(minDate)}" max="${esc(maxDate)}"></label>
    <label class="filter-label">To <input type="date" id="daily-to" value="${esc(state.dailyTo)}" min="${esc(minDate)}" max="${esc(maxDate)}"></label>
    <label class="filter-label">Status <select id="daily-status">
      <option value="all"${state.dailyStatus === "all" ? " selected" : ""}>all</option>
      <option value="failure"${state.dailyStatus === "failure" ? " selected" : ""}>any failure</option>
      <option value="success"${state.dailyStatus === "success" ? " selected" : ""}>success</option>
    </select></label>
    <div class="search-box"><input type="search" id="daily-search" placeholder="Search commit / subject…" value="${esc(state.dailySearch)}" aria-label="Search daily records"></div>
    <button class="btn btn-sm" id="daily-export">⬇ CSV</button>
  </div>`;

  html += `<div class="grid grid-2 section">
    <div class="card"><h3>Hello world compile time (bare)</h3><canvas id="daily-hello-bare"></canvas></div>
    <div class="card"><h3>Hello world compile time (std)</h3><canvas id="daily-hello-std"></canvas></div>
    <div class="card"><h3>Test suite duration</h3><canvas id="daily-test-dur"></canvas></div>
    <div class="card"><h3>Tests failed</h3><canvas id="daily-test-fail"></canvas></div>
  </div>`;

  html += `<div class="section"><h2>Daily records <span class="hint">▲/▼ = change vs previous point (time: up = regression)</span></h2>
    <div class="table-wrap"><table id="daily-table">
    <thead><tr>${sortableHeader("Date", "date", false)}${sortableHeader("Commit", "commit", false)}${sortableHeader("Subject", "subject", false)}${BACKENDS.map((b) => sortableHeader(BACKEND_LABEL[b] + " tests", "tests_" + b, true)).join("")}${sortableHeader("Status", "status", false)}</tr></thead>
    <tbody></tbody></table></div></div>`;
  el.innerHTML = html;

  $$(".bk-check").forEach((c) => c.addEventListener("change", () => {
    state.selectedBackends = new Set($$(".bk-check:checked").map((x) => x.value));
    savePrefs();
    renderDailyTable();
    renderDailyCharts();
    renderFailures();
  }));
  const df = $("#daily-from"), dt = $("#daily-to"), ds = $("#daily-status"), dse = $("#daily-search");
  df.addEventListener("change", () => { state.dailyFrom = df.value; renderDailyTable(); renderDailyCharts(); });
  dt.addEventListener("change", () => { state.dailyTo = dt.value; renderDailyTable(); renderDailyCharts(); });
  ds.addEventListener("change", () => { state.dailyStatus = ds.value; renderDailyTable(); renderDailyCharts(); });
  dse.addEventListener("input", () => { state.dailySearch = dse.value; renderDailyTable(); });
  $("#daily-export").addEventListener("click", () => exportTableCsv($("#daily-table"), "daily.csv"));

  renderDailyTable();
  renderDailyCharts();
}

function renderDailyTable() {
  const tableEl = $("#daily-table");
  if (!tableEl) return;
  const tbody = tableEl.querySelector("tbody");
  const rows = filteredDaily().slice().reverse();
  let html = "";
  rows.forEach((d, ri) => {
    const prevD = rows[ri + 1] || null;
    let anyFail = false;
    for (const b of BACKENDS) { const t = testsOf(backendOf(d, b)); if (t && t.failed > 0) anyFail = true; }
    html += `<tr class="clickable${anyFail ? " fail-row" : ""}" data-date="${esc(d.date)}">`;
    html += `<td><b>${esc(d.date)}</b></td><td class="mono">${esc((d.commit && d.commit.short) || "—")}</td><td class="dim" style="max-width:320px;overflow:hidden;text-overflow:ellipsis;white-space:nowrap" title="${esc(d.commit && d.commit.subject)}">${esc(d.commit && d.commit.subject)}</td>`;
    for (const b of BACKENDS) {
      const rec = backendOf(d, b);
      const prevRec = prevD && backendOf(prevD, b);
      const t = rec && rec.tests;
      let chip = "";
      if (t && prevRec && prevRec.tests) {
        chip = regressChip(t.duration_ms, prevRec.tests.duration_ms);
      }
      html += `<td class="num">${t && t.passed != null
        ? `${testStatusBadge(t)} <span class="${t.failed > 0 ? "missing" : ""}">${t.passed}/${t.failed}</span> ${chip}`
        : "—"}</td>`;
    }
    html += `<td>${statusBadge(d.status || "success", "")}</td></tr>`;
  });
  tbody.innerHTML = html || `<tr class="empty-row"><td colspan="${5 + BACKENDS.length}">No daily records match the filters</td></tr>`;
  tbody.querySelectorAll("tr.clickable").forEach((tr) => {
    tr.addEventListener("click", () => openDailyModal(tr.dataset.date));
  });
  bindSortable(tableEl);
  // re-apply the current sort after re-rendering (same as the releases table)
  const sortedKey = Object.keys(state.sort).find((k) => tableEl.querySelector(`th[data-sort="${k}"]`));
  if (sortedKey) {
    const th = tableEl.querySelector(`th[data-sort="${sortedKey}"]`);
    sortTable(tableEl, sortedKey, th.dataset.sortNum === "1", state.sort[sortedKey]);
  }
}

function renderDailyCharts() {
  const labels = filteredDaily().map((d) => d.date);
  const bks = BACKENDS.filter((b) => state.selectedBackends.has(b));

  for (const [id, kind] of [["daily-hello-bare", "hello_bare"], ["daily-hello-std", "hello_std"]]) {
    const datasets = bks.map((b) => ({
      label: BACKEND_LABEL[b], borderColor: colorOf(b), backgroundColor: colorOf(b),
      data: filteredDaily().map((d) => {
        const h = helloOf(backendOf(d, b), kind);
        return h && h.status === "success" ? h.duration_ms : null;
      }),
      spanGaps: false, tension: 0.2, pointRadius: 2,
    })).filter((s) => s.data.some((v) => v != null));
    makeChart(id, { type: "line", data: { labels, datasets }, options: baseLineOpts("compilation ms") });
  }

  makeChart("daily-test-dur", {
    type: "line",
    data: {
      labels,
      datasets: bks.map((b) => ({
        label: BACKEND_LABEL[b], borderColor: colorOf(b),
        data: filteredDaily().map((d) => { const t = backendOf(d, b) && backendOf(d, b).tests; return t && t.duration_ms != null ? t.duration_ms : null; }),
        spanGaps: false, tension: 0.2, pointRadius: 2,
      })).filter((s) => s.data.some((v) => v != null)),
    },
    options: baseLineOpts("ms"),
  });

  makeChart("daily-test-fail", {
    type: "bar",
    data: {
      labels,
      datasets: bks.map((b) => ({
        label: BACKEND_LABEL[b], backgroundColor: hslAlpha(colorOf(b), 0.7), borderRadius: 3,
        data: filteredDaily().map((d) => { const t = backendOf(d, b) && backendOf(d, b).tests; return t && t.failed != null ? t.failed : null; }),
      })).filter((s) => s.data.some((v) => v != null)),
    },
    options: barOpts("failed tests"),
  });
}

function openDailyModal(date) {
  const d = state.daily.find((x) => x.date === date);
  if (!d) return;
  let html = `<h2>${esc(d.date)}</h2><div class="modal-sub">commit <span class="mono">${esc(d.commit && d.commit.sha)}</span> · ${esc(d.commit && d.commit.author_date)}</div>`;
  html += `<div class="note">${esc(d.commit && d.commit.subject)}</div>`;

  for (const b of BACKENDS) {
    const rec = backendOf(d, b);
    if (!rec) continue;
    html += `<h3>${esc(BACKEND_LABEL[b])}</h3><div class="table-wrap"><table><tbody>
      <tr><td>Build</td><td>${statusBadge(rec.build.status)} ${rec.build.reason ? `<span class="dim">${esc(rec.build.reason)}</span>` : ""}</td></tr>`;
    for (const h of rec.benchmarks || []) {
      html += `<tr><td>${esc(h.name)}</td><td>${h.status === "success" ? fmtMs(h.duration_ms) : statusBadge(h.status) + " " + esc(h.reason || "")}</td></tr>`;
    }
    const t = rec.tests || {};
    html += `<tr><td>Tests</td><td>${testStatusBadge(t)} total <b>${t.total ?? "—"}</b> passed <b class="${t.failed > 0 ? "missing" : ""}">${t.passed ?? "—"}</b> failed <b>${t.failed ?? "—"}</b> duration ${fmtMs(t.duration_ms)}${t.complete === false ? " <span class='missing'>⚠ run incomplete — possible crash</span>" : ""}</td></tr>`;
    if (t.sequential && t.runner && (t.sequential.total > 0 || t.runner.total > 0)) {
      html += `<tr><td>Breakdown</td><td class="dim">sequential ${t.sequential.total} (${t.sequential.failed} failed) · @test runner ${t.runner.total} (${t.runner.failed} failed)</td></tr>`;
    }
    const fh = failedTestsHtml(t);
    if (fh) html += `<tr><td>Failed tests</td><td>${fh}</td></tr>`;
    if (rec.modules && rec.modules.length) html += `<tr><td>Modules (${rec.modules.length})</td><td class="dim">see Modules view</td></tr>`;
    html += `</tbody></table></div>`;
  }
  openModal(html);
}

/* ══ Failures ═════════════════════════════════════════════════ */
/* aggregate every failing/crashed/timed-out test across daily records —
   the dashboard's core focus: WHICH tests fail, HOW OFTEN, and WHEN. */

function collectFailures() {
  const map = new Map(); // testName → {count, dates:Set, backends:Set, crashed, timedOut}
  for (const d of state.daily) {
    for (const b of BACKENDS) {
      const rec = backendOf(d, b);
      const t = testsOf(rec);
      if (!t) continue;
      const seen = new Set();
      const add = (name, kind) => {
        if (!name || seen.has(name)) return;
        seen.add(name);
        let e = map.get(name);
        if (!e) { e = { test: name, count: 0, dates: new Set(), backends: new Set(), crashed: 0, timedOut: 0 }; map.set(name, e); }
        e.count++;
        e.dates.add(d.date);
        e.backends.add(b);
        if (kind === "crash") e.crashed++;
        if (kind === "timeout") e.timedOut++;
      };
      for (const f of t.failed_tests || []) {
        const crash = (t.crashed_tests || []).some((c) => c.name === f);
        const timed = (t.timed_out_tests || []).includes(f);
        add(f, crash ? "crash" : timed ? "timeout" : "fail");
      }
      for (const c of t.crashed_tests || []) add(c.name, "crash");
      for (const f of t.timed_out_tests || []) add(f, "timeout");
    }
  }
  return Array.from(map.values()).map((e) => ({
    ...e,
    dates: Array.from(e.dates).sort(),
    backends: Array.from(e.backends),
  })).sort((a, b) => b.count - a.count);
}

function renderFailures() {
  const el = $("#view-failures");
  const all = collectFailures();
  const badge = $("#failures-count");
  if (badge) { badge.textContent = all.length; badge.hidden = all.length === 0; }

  let html = `<div class="filters">
    <label class="filter-label">Backend <select id="fail-backend">
      <option value="all"${state.failBackend === "all" ? " selected" : ""}>all backends</option>
      ${BACKENDS.map((b) => `<option value="${esc(b)}"${state.failBackend === b ? " selected" : ""}>${esc(BACKEND_LABEL[b])}</option>`).join("")}
    </select></label>
    <label class="filter-label">Min occurrences <select id="fail-min">
      ${[1, 2, 3, 5, 10].map((n) => `<option value="${n}"${state.failMin === n ? " selected" : ""}>≥ ${n}</option>`).join("")}
    </select></label>
    <div class="search-box"><input type="search" id="fail-search" placeholder="Search failing tests…" value="${esc(state.failSearch)}" aria-label="Search failing tests"></div>
    <button class="btn btn-sm" id="fail-export">⬇ CSV</button>
    <span class="dim">aggregated from ${state.daily.length} daily records</span>
  </div>`;

  const totalFailures = all.reduce((s, e) => s + e.count, 0);
  const crashes = all.reduce((s, e) => s + e.crashed, 0);
  const timeouts = all.reduce((s, e) => s + e.timedOut, 0);

  html += `<div class="grid grid-4 section">
    <div class="stat"><div class="label">Failing tests</div><div class="value ${all.length ? "bad" : "good"}">${all.length}</div><div class="sub">unique test names</div></div>
    <div class="stat"><div class="label">Total failures</div><div class="value ${totalFailures ? "bad" : "good"}">${totalFailures}</div><div class="sub">across all runs</div></div>
    <div class="stat"><div class="label">Crashes</div><div class="value ${crashes ? "bad" : "good"}">${crashes}</div><div class="sub">exit-code crashes</div></div>
    <div class="stat"><div class="label">Timeouts</div><div class="value ${timeouts ? "bad" : "good"}">${timeouts}</div><div class="sub">10s timeouts</div></div>
  </div>`;

  html += `<div class="section"><h2>Failure frequency — top 25</h2><div class="card"><canvas id="fail-chart"></canvas></div></div>`;

  html += `<div class="section"><h2>All failing tests <span class="hint">searchable · click a row for where/when</span></h2>
    <div class="table-wrap"><table id="fail-table">
    <thead><tr>${sortableHeader("Test", "test", false)}${sortableHeader("Count", "count", true)}${sortableHeader("Backends", "backends", false)}${sortableHeader("First seen", "first", false)}${sortableHeader("Last seen", "last", false)}${sortableHeader("Kind", "kind", false)}</tr></thead>
    <tbody></tbody></table></div></div>`;
  el.innerHTML = html;

  const fb = $("#fail-backend"), fm = $("#fail-min"), fs = $("#fail-search");
  fb.addEventListener("change", () => { state.failBackend = fb.value; savePrefs(); renderFailureTable(); renderFailureCharts(); });
  fm.addEventListener("change", () => { state.failMin = +fm.value; savePrefs(); renderFailureTable(); renderFailureCharts(); });
  fs.addEventListener("input", () => { state.failSearch = fs.value; renderFailureTable(); });
  $("#fail-export").addEventListener("click", () => exportTableCsv($("#fail-table"), "failures.csv"));

  renderFailureTable();
  renderFailureCharts();
}

function filteredFailures() {
  let list = collectFailures();
  if (state.failBackend !== "all") list = list.filter((e) => e.backends.includes(state.failBackend));
  if (state.failMin > 1) list = list.filter((e) => e.count >= state.failMin);
  if (state.failSearch) {
    const q = state.failSearch.toLowerCase();
    list = list.filter((e) => e.test.toLowerCase().includes(q));
  }
  return list;
}

function renderFailureTable() {
  const tableEl = $("#fail-table");
  if (!tableEl) return;
  const tbody = tableEl.querySelector("tbody");
  const list = filteredFailures();
  let html = "";
  for (const e of list) {
    const kindChip = e.crashed ? `<span class="chip up" title="crashed (exit code)">💥 crash</span>`
      : e.timedOut ? `<span class="chip up">⏱ timeout</span>`
      : `<span class="chip flat">fail</span>`;
    html += `<tr class="clickable" data-test="${esc(e.test)}">
      <td class="mono">${esc(e.test)}</td>
      <td class="num ${e.count > 1 ? "missing" : ""}">${e.count}</td>
      <td>${e.backends.map((b) => `<span class="badge badge-neutral" style="margin-right:4px">${esc(BACKEND_LABEL[b])}</span>`).join("")}</td>
      <td class="dim">${esc(e.dates[0])}</td>
      <td class="dim">${esc(e.dates[e.dates.length - 1])}</td>
      <td>${kindChip}</td>
    </tr>`;
  }
  tbody.innerHTML = html || `<tr class="empty-row"><td colspan="6">No failing tests match the filters</td></tr>`;
  tbody.querySelectorAll("tr.clickable").forEach((tr) => {
    tr.addEventListener("click", () => openFailureModal(tr.dataset.test));
  });
  bindSortable(tableEl);
}

function renderFailureCharts() {
  const list = filteredFailures().slice(0, 25).reverse(); // ascending for horizontal bar
  makeChart("fail-chart", {
    type: "bar",
    data: {
      labels: list.map((e) => e.test),
      datasets: [{
        label: "occurrences",
        data: list.map((e) => e.count),
        backgroundColor: list.map((e) => e.crashed ? hslAlpha(colorOf("Interpreter"), 0.75) : hslAlpha(colorOf("TCCCompiler"), 0.6)),
        borderRadius: 4,
      }],
    },
    options: {
      indexAxis: "y",
      responsive: true,
      maintainAspectRatio: false,
      plugins: {
        legend: { display: false },
        tooltip: tooltipOpts(),
      },
      scales: {
        x: { ticks: { color: chartColors().tick, font: { size: 10 } }, grid: { color: chartColors().grid } },
        y: { ticks: { color: chartColors().tick, font: { size: 10 } }, grid: { display: false } },
      },
    },
  });
}

function openFailureModal(testName) {
  const all = collectFailures();
  const e = all.find((x) => x.test === testName);
  if (!e) return;
  // find every occurrence with details
  let rows = "";
  for (const d of state.daily) {
    for (const b of BACKENDS) {
      const rec = backendOf(d, b);
      const t = testsOf(rec);
      if (!t) continue;
      const crash = (t.crashed_tests || []).find((c) => c.name === testName);
      const timed = (t.timed_out_tests || []).includes(testName);
      const failed = (t.failed_tests || []).includes(testName);
      if (!failed && !crash && !timed) continue;
      rows += `<tr><td class="mono">${esc(d.date)}</td><td>${esc(BACKEND_LABEL[b])}</td><td>${statusBadge(d.status || "success", "")}</td>
        <td>${crash ? `<span class="chip up">💥 exit ${esc(crash.exit_code)}</span>` : timed ? `<span class="chip up">⏱ timeout</span>` : `<span class="chip flat">assertion</span>`}</td>
        <td class="num">${t.passed != null ? t.passed + "/" + t.failed : "—"}</td></tr>`;
    }
  }
  let html = `<h2 class="mono">${esc(testName)}</h2>
    <div class="modal-sub">${e.count} occurrence${e.count > 1 ? "s" : ""} · seen on ${e.dates.length} date${e.dates.length > 1 ? "s" : ""} · first ${esc(e.dates[0])} · last ${esc(e.dates[e.dates.length - 1])}</div>`;
  html += `<div class="table-wrap"><table><thead><tr><th>Date</th><th>Backend</th><th>Day status</th><th>Kind</th><th class="num">Day pass/fail</th></tr></thead><tbody>${rows || `<tr class="empty-row"><td colspan="5">no occurrences</td></tr>`}</tbody></table></div>`;
  openModal(html);
}

/* ══ Modules ═══════════════════════════════════════════════════ */

function moduleNames() {
  const set = new Set();
  for (const d of state.daily) {
    for (const b of BACKENDS) {
      const rec = backendOf(d, b);
      for (const m of (rec && rec.modules) || []) if (m.name) set.add(m.name);
    }
  }
  return Array.from(set).sort();
}

function renderModules() {
  const el = $("#view-modules");
  const names = moduleNames();
  if (!names.length) { el.innerHTML = `<div class="note">No module benchmark data yet.</div>`; return; }
  if (!state.modName || !names.includes(state.modName)) state.modName = names[0];

  let html = `<div class="filters">
    <label class="filter-label">Module <select id="mod-select">${names.map((n) => `<option value="${esc(n)}"${n === state.modName ? " selected" : ""}>${esc(n)}</option>`).join("")}</select></label>
    <label class="filter-label">Backend <select id="mod-backend">${BACKENDS.map((b) => `<option value="${esc(b)}"${b === state.modBackend ? " selected" : ""}>${esc(BACKEND_LABEL[b])}</option>`).join("")}</select></label>
    <button class="btn btn-sm" id="mod-export">⬇ CSV</button>
  </div>`;
  html += `<div class="section"><h2>Module compilation time over dates</h2><div class="card"><canvas id="mod-trend"></canvas></div></div>`;
  html += `<div class="section"><h2>Latest module timings per backend</h2><div class="table-wrap"><table id="mod-table">
    <thead><tr>${sortableHeader("Module", "module", false)}${BACKENDS.map((b) => sortableHeader(BACKEND_LABEL[b] + " ms", "mod_" + b, true)).join("")}</tr></thead>
    <tbody>`;
  const latest = state.daily[state.daily.length - 1];
  for (const n of names) {
    html += `<tr><td class="mono">${esc(n)}</td>`;
    for (const b of BACKENDS) {
      const rec = latest && backendOf(latest, b);
      const m = rec && rec.modules && rec.modules.find((x) => x.name === n);
      html += `<td class="num">${m && m.millis != null ? m.millis.toFixed ? m.millis.toFixed(1) : m.millis : "—"}</td>`;
    }
    html += `</tr>`;
  }
  html += `</tbody></table></div></div>`;
  el.innerHTML = html;
  const sel = $("#mod-select"), bsel = $("#mod-backend");
  sel.addEventListener("change", () => { state.modName = sel.value; renderModuleChart(); });
  bsel.addEventListener("change", () => { state.modBackend = bsel.value; savePrefs(); renderModuleChart(); });
  $("#mod-export").addEventListener("click", () => exportTableCsv($("#mod-table"), "modules.csv"));
  bindSortable($("#mod-table"));
  renderModuleChart();
}

function renderModuleChart() {
  const sel = $("#mod-select"), bsel = $("#mod-backend");
  if (!sel || !bsel) return;
  const name = sel.value, backend = bsel.value;
  const data = state.daily.map((d) => {
    const rec = backendOf(d, backend);
    const m = rec && rec.modules && rec.modules.find((x) => x.name === name);
    return m && m.millis != null ? m.millis : null;
  });
  makeChart("mod-trend", {
    type: "line",
    data: { labels: state.daily.map((d) => d.date), datasets: [{ label: name + " (" + BACKEND_LABEL[backend] + ")", data, borderColor: colorOf(backend), spanGaps: false, tension: 0.2, pointRadius: 2 }] },
    options: baseLineOpts("ms"),
  });
}

/* ══ Backends ══════════════════════════════════════════════════ */

function renderBackends() {
  const el = $("#view-backends");
  const latest = state.daily[state.daily.length - 1];
  if (!latest) { el.innerHTML = `<div class="note">No data yet.</div>`; return; }

  let html = `<div class="filters">
    ${BACKENDS.map((b) => `<label class="filter-label"><input type="checkbox" class="be-check" value="${esc(b)}" ${state.selectedBackends.has(b) ? "checked" : ""}> ${esc(BACKEND_LABEL[b])}</label>`).join("")}
    <span class="dim">backend comparison across all daily records</span>
  </div>`;

  html += `<div class="section"><h2>Latest daily comparison — <span class="mono">${esc(latest.date)}</span></h2><div class="card"><div class="table-wrap"><table>
    <thead><tr><th>Metric</th>${BACKENDS.map((b) => `<th class="num">${esc(BACKEND_LABEL[b])}</th>`).join("")}</tr></thead><tbody>`;
  const metrics = [
    ["Build", (r) => r ? statusBadge(r.build.status, "") : "—"],
    ["Hello bare", (r) => { const h = helloOf(r, "hello_bare"); return h && h.status === "success" ? fmtMs(h.duration_ms) : "—"; }],
    ["Hello std", (r) => { const h = helloOf(r, "hello_std"); return h && h.status === "success" ? fmtMs(h.duration_ms) : "—"; }],
    ["Tests passed", (r) => r && r.tests && r.tests.passed != null ? r.tests.passed : "—"],
    ["Tests failed", (r) => r && r.tests && r.tests.failed != null ? `<span class="${r.tests.failed > 0 ? "missing" : ""}">${r.tests.failed}</span>` : "—"],
    ["Test duration", (r) => r && r.tests && r.tests.duration_ms != null ? fmtMs(r.tests.duration_ms) : "—"],
    ["Pass rate", (r) => r && r.tests && r.tests.total ? (100 * r.tests.passed / r.tests.total).toFixed(1) + "%" : "—"],
  ];
  for (const [label, fn] of metrics) {
    html += `<tr><td>${esc(label)}</td>${BACKENDS.map((b) => `<td class="num">${fn(backendOf(latest, b))}</td>`).join("")}</tr>`;
  }
  html += `</tbody></table></div></div></div>`;

  html += `<div class="grid grid-2 section">
    <div class="card"><h3>Hello world (bare) — daily, per backend</h3><canvas id="be-hello"></canvas></div>
    <div class="card"><h3>Test suite duration — daily, per backend</h3><canvas id="be-dur"></canvas></div>
  </div>`;
  html += `<div class="section"><h2>Backend history — pass rate</h2><div class="card"><canvas id="be-passrate"></canvas></div></div>`;
  el.innerHTML = html;

  $$(".be-check").forEach((c) => c.addEventListener("change", () => {
    state.selectedBackends = new Set($$(".be-check:checked").map((x) => x.value));
    savePrefs();
    renderBackendCharts();
  }));
  renderBackendCharts();
}

function renderBackendCharts() {
  const labels = state.daily.map((d) => d.date);
  const bks = BACKENDS.filter((b) => state.selectedBackends.has(b));
  const mk = (get) => bks.map((b) => ({
    label: BACKEND_LABEL[b], borderColor: colorOf(b),
    data: state.daily.map((d) => get(backendOf(d, b))),
    spanGaps: false, tension: 0.2, pointRadius: 2,
  })).filter((s) => s.data.some((v) => v != null));

  makeChart("be-hello", { type: "line", data: { labels, datasets: mk((r) => { const h = helloOf(r, "hello_bare"); return h && h.status === "success" ? h.duration_ms : null; }) }, options: baseLineOpts("ms") });
  makeChart("be-dur", { type: "line", data: { labels, datasets: mk((r) => r && r.tests && r.tests.duration_ms != null ? r.tests.duration_ms : null) }, options: baseLineOpts("ms") });
  makeChart("be-passrate", {
    type: "line",
    data: { labels, datasets: mk((r) => r && r.tests && r.tests.total ? +(100 * r.tests.passed / r.tests.total).toFixed(1) : null) },
    options: baseLineOpts("%"),
  });
}

/* ── shared chart options + colors ──────────────────────────── */

const COLORS = { TCCCompiler: "#3fb950", Compiler: "#58a6ff", Interpreter: "#d29922" };
const colorOf = (b) => COLORS[b] || "#8b949e";
const colorFor = (s) => ["#58a6ff", "#3fb950", "#d29922", "#f85149", "#bc8cff", "#39c5cf", "#ffa657", "#8b949e"][hashStr(s) % 8];
function hashStr(s) { let h = 0; for (let i = 0; i < s.length; i++) h = (h * 31 + s.charCodeAt(i)) | 0; return Math.abs(h); }

function tooltipOpts() {
  const c = chartColors();
  return { backgroundColor: c.tooltipBg, borderColor: c.tooltipBorder, borderWidth: 1, titleColor: c.text, bodyColor: c.text, padding: 10, cornerRadius: 8 };
}
function baseOpts() {
  const c = chartColors();
  return {
    responsive: true,
    maintainAspectRatio: false,
    interaction: { mode: "index", intersect: false },
    plugins: {
      legend: { labels: { color: c.muted, boxWidth: 10, font: { size: 11 } } },
      tooltip: tooltipOpts(),
    },
    scales: {
      x: { ticks: { color: c.tick, maxRotation: 45, autoSkip: true, maxTicksLimit: 24, font: { size: 10 } }, grid: { color: c.grid } },
      y: { ticks: { color: c.tick, font: { size: 10 } }, grid: { color: c.grid } },
    },
  };
}
function baseLineOpts(yLabel) {
  const o = baseOpts();
  o.scales.y.title = { display: !!yLabel, text: yLabel, color: chartColors().muted, font: { size: 10 } };
  return o;
}
function barOpts(yLabel) {
  const o = baseOpts();
  o.scales.y.title = { display: !!yLabel, text: yLabel, color: chartColors().muted, font: { size: 10 } };
  return o;
}

/* ── modal + tabs + boot ────────────────────────────────────── */

function openModal(html) {
  $("#modal-body").innerHTML = html;
  $("#modal").classList.remove("hidden");
}
function closeModal() { $("#modal").classList.add("hidden"); }
document.addEventListener("click", (e) => {
  if (e.target.closest("[data-close]")) closeModal();
});
document.addEventListener("keydown", (e) => {
  if (e.key === "Escape") closeModal();
  // "/" focuses the first visible search box
  if (e.key === "/" && !/INPUT|SELECT|TEXTAREA/.test(document.activeElement.tagName)) {
    const v = $(".view.active");
    const sb = v && v.querySelector(".search-box input");
    if (sb) { e.preventDefault(); sb.focus(); }
  }
});

function switchView(name) {
  $$(".tab").forEach((t) => t.classList.toggle("active", t.dataset.view === name));
  $$(".view").forEach((v) => v.classList.toggle("active", v.id === "view-" + name));
  renderChartsForCurrentView();
}
$$(".tab").forEach((t) => t.addEventListener("click", () => switchView(t.dataset.view)));
$("#theme-toggle").addEventListener("click", toggleTheme);

document.addEventListener("DOMContentLoaded", init);
