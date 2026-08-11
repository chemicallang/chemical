"use strict";

/* ⚗️ Chemical Compiler Analytics dashboard — pure static JS reading data/*.json */

const $ = (sel) => document.querySelector(sel);
const $$ = (sel) => Array.from(document.querySelectorAll(sel));

const BACKENDS = ["TCCCompiler", "Compiler", "Interpreter"];

const state = {
  manifest: null,
  daily: [],
  dailyLoaded: false,
  releases: [],
  releasesLoaded: false,
  selectedBackends: new Set(BACKENDS),
};

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

/* ── small helpers ─────────────────────────────────────────────── */

function setStatus(text) {
  const el = $("#data-status");
  el.textContent = text;
  el.className = "badge " + (text === "loaded" ? "badge-ok" : "badge-loading");
  if (text !== "loaded") el.textContent = "⚠ " + text;
}

const fmtSize = (b) =>
  b == null ? "—" : b >= 1048576 ? (b / 1048576).toFixed(1) + " MB" : (b / 1024).toFixed(1) + " KB";

const fmtMs = (ms) =>
  ms == null ? "—" : ms >= 60000 ? (ms / 60000).toFixed(1) + " min" : ms >= 1000 ? (ms / 1000).toFixed(2) + " s" : ms + " ms";

const fmtDate = (s) => (s ? s.slice(0, 10) : "—");

function esc(s) {
  return String(s == null ? "" : s)
    .replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;").replace(/'/g, "&#39;");
}

function statusBadge(status, label) {
  const s = status || "unavailable";
  return `<span class="badge badge-${esc(s)}">${esc(label || STATUS_LABEL[s] || s)}</span>`;
}

/* human-readable labels for machine statuses (cryptic "failed" → "missing asset") */
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

/* test-result helpers — the dashboard's primary focus is test results: which
 * tests failed, crashed or timed out. Each backend record carries:
 *   tests: {status,total,passed,failed,complete,sequential:{..},runner:{..},
 *           failed_tests:[names],crashed_tests:[{name,exit_code}],timed_out_tests:[names]}
 * A non-zero exit code (e.g. 139) means the child process crashed. */
function testStatusBadge(t) {
  if (!t) return `<span class="badge badge-unavailable">no tests</span>`;
  let label = t.status || "unavailable";
  if (t.failed > 0 && (t.status === "success" || !t.status)) label = "test_failure";
  const cls = label === "success" ? "success" : label;
  return `<span class="badge badge-${esc(cls)}" title="${esc(t.reason || "")}">${esc(STATUS_LABEL[label] || label)}</span>`;
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
    html += `<details class="dim"><summary>${items.length} failing tests…</summary><ul class="flat">${items.slice(limit).join("")}</ul></details>`;
  }
  return html;
}

function regressChip(current, prev, inverse) {
  if (current == null || prev == null || prev === 0) return "";
  const diff = ((current - prev) / prev) * 100;
  const cls = Math.abs(diff) < 0.5 ? "flat" : (inverse ? (diff > 0 ? "up" : "down") : (diff > 0 ? "up" : "down"));
  const sign = diff > 0 ? "+" : "";
  const icon = cls === "flat" ? "•" : cls === "up" ? "▲" : "▼";
  return `<span class="chip ${cls}" title="${esc(prev.toFixed(2))} → ${esc(current.toFixed(2))}">${icon} ${sign}${diff.toFixed(1)}%</span>`;
}

/* asset-name → (platform, arch, variant)  — mirrors scripts/common.sh */
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

const PLATFORM_LABEL = {
  linux: "Linux", "linux-alpine": "Alpine", macos: "macOS",
  windows: "Windows", "windows-mingw": "Windows (MinGW)",
};

const BACKEND_LABEL = { TCCCompiler: "TCC", Compiler: "LLVM", Interpreter: "Interpreter" };

/* ── data access ─────────────────────────────────────────────── */

function backendOf(dailyRec, backend) {
  return (dailyRec.backends && dailyRec.backends[backend]) || null;
}

function helloOf(rec, name) {
  if (!rec || !Array.isArray(rec.benchmarks)) return null;
  return rec.benchmarks.find((b) => b.name === name) || null;
}

function tagDate(tag) {
  const r = state.releases.find((x) => x.info.tag === tag);
  return r && r.info.published_at ? r.info.published_at : tag;
}

/* Lazily ensure a release's platform records are loaded (fetched from the
 * manifest's platform list on first use, then cached). Used by the Releases
 * charts and the release modal — keeps the initial page load light. */
async function ensurePlatformRecords(rec) {
  const manifestPlatforms = (state.manifest.releases || {})[rec.info.tag] || [];
  const missing = manifestPlatforms.filter((pf) => !rec.platforms[pf]);
  if (!missing.length) return rec;
  const results = await Promise.all(missing.map((pf) =>
    loadPlatformRecord(rec.info.tag, pf).then((r) => ({ pf, r }))));
  for (const { pf, r } of results) if (r) rec.platforms[pf] = r;
  return rec;
}

/* load only ONE platform's record for a release — the charts only need the
 * selected platform, so we must not fetch all platforms of all releases
 * (48 releases × 5 platforms = hundreds of fetches on every chart render). */
async function ensurePlatformRecord(rec, pf) {
  if (rec.platforms[pf]) return rec.platforms[pf];
  const r = await loadPlatformRecord(rec.info.tag, pf);
  if (r) rec.platforms[pf] = r;
  return r;
}

/* ── data loading ────────────────────────────────────────────── */

async function fetchJson(url) {
  const r = await fetch(url);
  if (!r.ok) throw new Error(url + " → " + r.status);
  return r.json();
}

/* data loading — the manifest is small and always fetched first; daily
 * records and release info are fetched in the background; per-platform
 * benchmark records are fetched LAZILY when a view/modal needs them. */

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

async function init() {
  try {
    state.manifest = await fetchJson("data/manifest.json");
    setStatus("loading…");
    // background: daily records + release info (small-ish; parallel)
    const tasks = [];
    for (const d of state.manifest.daily || []) {
      tasks.push(loadDailyRecord(d).then((rec) => { if (rec) state.daily.push(rec); }));
    }
    for (const [tag] of Object.entries(state.manifest.releases || {})) {
      tasks.push(fetchJson(`data/releases/${tag}/info.json`).then((info) => {
        const rec = { info, platforms: {} };
        // platform records are NOT fetched here — see loadPlatformRecord
        state.releases.push(rec);
      }).catch((e) => { console.warn("missing release", tag); }));
    }
    await Promise.all(tasks);
    state.daily.sort((a, b) => (a.date < b.date ? -1 : a.date > b.date ? 1 : 0));
    state.releases.sort((a, b) => {
      const da = a.info.published_at || "", db = b.info.published_at || "";
      if (da && db) return da < db ? -1 : 1;
      return a.info.tag < b.info.tag ? -1 : 1;
    });
    state.dailyLoaded = true;
    state.releasesLoaded = true;
    setStatus("loaded");
    render();
  } catch (e) {
    setStatus("load failed: " + e.message);
    console.error(e);
  }
}

/* ── view rendering ──────────────────────────────────────────── */

function render() {
  renderOverview();
  renderReleases();
  renderDaily();
  renderModules();
  renderBackends();
}

/* ══ Overview ══════════════════════════════════════════════════ */

function renderOverview() {
  const el = $("#view-overview");
  const latest = state.daily[state.daily.length - 1] || null;
  const lastRel = state.releases[state.releases.length - 1] || null;

  let html = `<div class="grid grid-4 section">
    <div class="stat"><div class="label">Releases tracked</div><div class="value">${state.releases.length}</div><div class="sub">${lastRel ? "latest " + esc(lastRel.info.tag) : ""}</div></div>
    <div class="stat"><div class="label">Daily points</div><div class="value">${state.daily.length}</div><div class="sub">${latest ? "latest " + esc(latest.date) : ""}</div></div>
    <div class="stat"><div class="label">Backends</div><div class="value">${BACKENDS.length}</div><div class="sub">TCC · LLVM · Interpreter</div></div>
    <div class="stat"><div class="label">Data updated</div><div class="value">${esc(fmtDate(state.manifest.generated_at))}</div><div class="sub">raw JSON in gh-pages/data</div></div>
  </div>`;

  if (latest) {
    html += `<div class="section"><h2>Latest daily build — <span class="mono">${esc(latest.date)}</span> <span class="dim">${esc(latest.commit && latest.commit.subject)}</span></h2>`;
    html += `<div class="card"><table>
      <thead><tr><th>Backend</th><th>Build</th><th>Tests</th><th class="num">Passed</th><th class="num">Failed</th><th class="num">Duration</th></tr></thead><tbody>`;
    for (const b of BACKENDS) {
      const rec = backendOf(latest, b);
      if (!rec) { html += `<tr><td>${esc(BACKEND_LABEL[b])}</td><td colspan="5" class="dim">no data</td></tr>`; continue; }
      const t = rec.tests || {};
      html += `<tr>
        <td><b>${esc(BACKEND_LABEL[b])}</b></td>
        <td>${statusBadge(rec.build.status, "")}</td>
        <td>${testStatusBadge(t)}${t.complete === false ? ` <span class="chip up" title="run ended before summary — possible crash">⚠ incomplete</span>` : ""}</td>
        <td class="num">${t.passed != null ? t.passed : "—"}</td>
        <td class="num ${t.failed > 0 ? "missing" : ""}">${t.failed != null ? t.failed : "—"}</td>
        <td class="num">${fmtMs(t.duration_ms)}</td>
      </tr>`;
      const fh = failedTestsHtml(t, 12);
      if (fh) {
        html += `<tr class="sub-row"><td></td><td colspan="5" class="dim"><b>Failed:</b> ${fh}</td></tr>`;
      }
    }
    html += `</tbody></table></div></div>`;
  }

  if (lastRel) {
    html += `<div class="section"><h2>Latest release — <span class="mono">${esc(lastRel.info.tag)}</span></h2>`;
    html += `<div class="card"><table><thead><tr><th>Platform</th><th class="num">Size</th></tr></thead><tbody>`;
    for (const [asset, a] of Object.entries(lastRel.info.assets || {})) {
      const p = assetParts(asset);
      if (p.variant !== "regular") continue;
      html += `<tr><td>${esc(PLATFORM_LABEL[p.platform] || p.platform)} ${esc(p.arch)} <span class="dim">(${esc(asset)})</span></td>
        <td class="num">${a.status === "success" ? fmtSize(a.size_bytes) : `<span class="missing">${esc(a.status)}</span>`}</td></tr>`;
    }
    html += `</tbody></table></div>`;
    // test results for the latest release (most-covered platform, e.g.
    // linux-x64) — the dashboard's primary concern is test pass/fail data
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
  el.classList.contains("active") && renderOverviewCharts();
}

/* load the latest release's platform record (most-covered platform) and show
 * its test results directly on the Overview — test results are the primary
 * dashboard focus, so the latest release's pass/fail data must be visible
 * without opening a modal. */
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
    rows += `<tr>
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
  box.innerHTML = `<h3>Test results — ${esc(pk)}</h3><table>
    <thead><tr><th>Backend</th><th>Tests</th><th class="num">Passed</th><th class="num">Failed</th><th class="num">Duration</th></tr></thead><tbody>${rows}</tbody></table>`;
}

function renderOverviewCharts() {
  const latest = state.daily[state.daily.length - 1];
  const labels = state.daily.map((d) => d.date);
  const datasets = [];
  for (const b of BACKENDS) {
    const data = state.daily.map((d) => {
      const h = helloOf(backendOf(d, b), "hello_bare");
      return h && h.status === "success" ? h.duration_ms : null;
    });
    if (data.some((v) => v != null)) datasets.push({ label: BACKEND_LABEL[b], data, borderColor: colorOf(b), backgroundColor: colorOf(b), spanGaps: true, tension: 0.25, pointRadius: 2 });
  }
  makeChart("ov-hello", {
    type: "line",
    data: { labels, datasets },
    options: baseLineOpts("compilation ms"),
  });

  // release std-lib hello across ALL releases (Linux x64 / TCC + LLVM where
  // available) — a quick way to see whether a new release regressed compile time
  if (state.releases.length > 1) {
    const pk = "linux-x64";
    // platform records are lazy; load only the one platform, then draw
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

/* ══ Releases ══════════════════════════════════════════════════ */

function releasePlatformsForChart() {
  // union of platform-arch keys across ALL releases. Sourced from the manifest
  // (not state.releases[].platforms — those are lazily loaded on demand).
  const set = new Set();
  for (const pf of Object.values(state.manifest.releases || {})) {
    for (const k of pf) set.add(k);
  }
  return Array.from(set).sort();
}

/* the platform with benchmark records in the MOST releases (e.g. linux-x64) —
 * used as the default selection so the release charts show data immediately
 * instead of the alphabetically-first platform (which has records for only a
 * single release and therefore renders every chart empty). */
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

  const defPlatform = mostCoveredPlatform();
  let html = `<div class="filters">
    <label>Platform <select id="rel-platform-select">${platforms.map((p) => `<option value="${esc(p)}"${p === defPlatform ? " selected" : ""}>${esc(p)}</option>`).join("")}</select></label>
    <label>Backend <select id="rel-backend-select">${BACKENDS.map((b) => `<option value="${esc(b)}">${esc(BACKEND_LABEL[b])}</option>`).join("")}</select></label>
    <label>Variant <select id="rel-variant-select"><option value="regular">regular</option><option value="tcc">tcc</option></select></label>
  </div>`;

  html += `<div class="section"><h2>Binary size by release <span class="hint">(choose platform + variant)</span></h2><div class="card"><canvas id="rel-size-bar"></canvas></div></div>`;
  html += `<div class="section"><h2>Binary size over time — all platforms <span class="hint">(respects the variant selector above)</span></h2><div class="card"><canvas id="rel-size-line"></canvas></div></div>`;
  html += `<div class="section"><h2>Hello world compile time per release <span class="hint">(bare vs std)</span></h2><div class="card"><canvas id="rel-hello"></canvas></div></div>`;
  html += `<div class="section"><h2>Release test results <span class="hint">(selected platform + backend)</span></h2><div class="card"><canvas id="rel-tests"></canvas></div></div>`;

  html += `<div class="section"><h2>All releases <span class="hint">sizes in MB · missing assets marked MISSING</span></h2><div class="card" style="overflow-x:auto"><table class="rel-table">
    <thead><tr><th>Version</th><th>Published</th><th>Windows x64</th><th>Linux x64</th><th>Alpine x64</th><th>macOS ARM</th><th>macOS x64</th></tr></thead><tbody>`;

  const cols = [
    { pf: "windows", arch: "x64" },
    { pf: "linux", arch: "x64" },
    { pf: "linux-alpine", arch: "x64" },
    { pf: "macos", arch: "arm64" },
    { pf: "macos", arch: "x64" },
  ];

  for (const r of state.releases.slice().reverse()) {
    const tag = r.info.tag;
    html += `<tr class="clickable" data-tag="${esc(tag)}">`;
    html += `<td><b>${esc(tag)}</b></td><td class="dim">${esc(fmtDate(r.info.published_at))}</td>`;
    for (const c of cols) {
      const name = assetNameFor(c.pf, c.arch, "regular");
      const a = r.info.assets && r.info.assets[name];
      if (a) html += `<td class="num" ${a.status === "missing_asset" ? `title="this asset was not published for ${tag}"` : ""}>${a.status === "success" ? fmtSize(a.size_bytes) : a.status === "missing_asset" ? `<span class="missing">MISSING</span>` : `<span class="missing" title="${esc(STATUS_LABEL[a.status] || a.status)}">${esc(STATUS_LABEL[a.status] || a.status)}</span>`}</td>`;
      else html += `<td class="num dim">—</td>`;
    }
    html += `</tr>`;
  }
  html += `</tbody></table></div></div>`;
  el.innerHTML = html;

  $$("#view-releases tbody tr.clickable").forEach((tr) => {
    tr.addEventListener("click", () => openReleaseModal(tr.dataset.tag));
  });
  ["rel-platform-select", "rel-backend-select", "rel-variant-select"].forEach((id) => {
    const s = document.getElementById(id);
    s && s.addEventListener("change", () => renderReleaseCharts());
  });
  renderReleaseCharts();
}

function assetNameFor(platform, arch, variant) {
  const pf = platform === "linux-alpine" ? "linux-alpine" : platform;
  return `${pf}-${arch}${variant === "tcc" ? "-tcc" : ""}.zip`;
}

/* binary sizes on the y axis are reported in MB (bytes are unwieldy) */
const MB = (b) => (b == null ? null : b / 1048576);

async function renderReleaseCharts() {
  const ps = $("#rel-platform-select"), bs = $("#rel-backend-select"), vs = $("#rel-variant-select");
  if (!ps || !bs || !vs) return;
  const platformKey = ps.value, backend = bs.value, variant = vs.value;
  if (!platformKey) return;
  const pp = platformKey.split("-"); // e.g. linux-x64 / windows-mingw-x64
  let platform, arch;
  if (pp[0] === "linux-alpine") { platform = "linux-alpine"; arch = pp[1]; }
  else if (pp[0] === "windows-mingw") { platform = "windows-mingw"; arch = pp[1]; }
  else { platform = pp[0]; arch = pp[1]; }
  const assetName = assetNameFor(platform, arch, variant);

  // lazily load ONLY the selected platform's records (not every platform of
  // every release — that caused the slow initial load / empty charts)
  await Promise.all(state.releases.map((r) => ensurePlatformRecord(r, platformKey)));

  const tags = state.releases.map((r) => r.info.tag);
  const sizes = state.releases.map((r) => {
    const a = r.info.assets && r.info.assets[assetName];
    return a && a.status === "success" ? MB(a.size_bytes) : null;
  });

  makeChart("rel-size-bar", {
    type: "bar",
    data: { labels: tags, datasets: [{ label: platformKey + " " + variant, data: sizes, backgroundColor: "#58a6ff99" }] },
    options: barOpts("MB"),
  });

  // line: all platforms, variant follows the selector (was hardcoded regular)
  const lineSets = [];
  for (const pk of releasePlatformsForChart()) {
    const p = pk.split("-");
    const pl = p[0] === "linux-alpine" ? "linux-alpine" : p[0] === "windows-mingw" ? "windows-mingw" : p[0];
    const ar = p[0] === "linux-alpine" || p[0] === "windows-mingw" ? p[1] : p[1];
    const an = assetNameFor(pl, ar, variant);
    const data = state.releases.map((r) => {
      const a = r.info.assets && r.info.assets[an];
      return a && a.status === "success" ? MB(a.size_bytes) : null;
    });
    if (data.some((v) => v != null)) lineSets.push({ label: pk + " (" + variant + ")", data, borderColor: colorFor(pk), spanGaps: false, tension: 0.2, pointRadius: 2 });
  }
  makeChart("rel-size-line", { type: "line", data: { labels: tags, datasets: lineSets }, options: baseLineOpts("MB") });

  // hello world per release (from the selected platform record)
  const helloSets = [];
  for (const kind of ["hello_bare", "hello_std"]) {
    const data = state.releases.map((r) => {
      const pf = r.platforms && r.platforms[platformKey];
      const rec = pf && pf.backends && pf.backends[backend];
      const h = helloOf(rec, kind);
      return h && h.status === "success" ? h.duration_ms : null;
    });
    helloSets.push({ label: kind === "hello_bare" ? "bare" : "std", data, borderColor: kind === "hello_bare" ? "#3fb950" : "#d29922", spanGaps: false, tension: 0.2, pointRadius: 2 });
  }
  makeChart("rel-hello", { type: "line", data: { labels: tags, datasets: helloSets }, options: baseLineOpts("compilation ms") });

  // tests per release (selected platform, selected backend): passed & failed
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
        { label: "passed", data: passSets, backgroundColor: "#3fb95099" },
        { label: "failed", data: failSets, backgroundColor: "#f8514999" },
      ],
    },
    options: barOpts("tests"),
  });
}

async function openReleaseModal(tag) {
  const r = state.releases.find((x) => x.info.tag === tag);
  if (!r) return;
  // lazily fetch this release's platform benchmark records
  await ensurePlatformRecords(r);
  let html = `<h2>${esc(tag)}</h2><div class="modal-sub">published ${esc(r.info.published_at || "—")} · commit <span class="mono">${esc(r.info.commit_sha || "—")}</span></div>`;

  html += `<h3>Assets</h3><table><thead><tr><th>Asset</th><th>Platform</th><th>Arch</th><th>Variant</th><th class="num">Size</th><th>Status</th></tr></thead><tbody>`;
  for (const [name, a] of Object.entries(r.info.assets || {})) {
    const p = assetParts(name);      html += `<tr><td>${esc(name)}</td><td>${esc(PLATFORM_LABEL[p.platform] || p.platform)}</td><td>${esc(p.arch)}</td><td>${esc(p.variant)}</td>
      <td class="num">${a.status === "success" ? fmtSize(a.size_bytes) : "—"}</td><td>${statusBadge(a.status, a.status === "missing_asset" ? "missing asset" : "")}</td></tr>`;
  }
  html += `</tbody></table>`;

  for (const [pk, pf] of Object.entries(r.platforms || {})) {
    html += `<h3>Test results — ${esc(pk)}</h3><table><thead><tr><th>Backend</th><th>Tests</th><th class="num">Passed</th><th class="num">Failed</th><th class="num">Duration</th></tr></thead><tbody>`;
    for (const b of BACKENDS) {
      const rec = pf.backends && pf.backends[b];
      if (!rec) continue;
      const t = rec.tests || {};
      html += `<tr><td><b>${esc(BACKEND_LABEL[b])}</b> ${statusBadge(rec.build.status, "")}</td>
        <td>${testStatusBadge(t)}${t.complete === false ? " <span class='missing'>⚠ incomplete</span>" : ""}</td>
        <td class="num">${t.passed != null ? t.passed : "—"}</td>
        <td class="num ${t.failed > 0 ? "missing" : ""}">${t.failed != null ? t.failed : "—"}</td>
        <td class="num">${fmtMs(t.duration_ms)}</td></tr>`;
      const fh = failedTestsHtml(t, 20);
      if (fh) {
        html += `<tr class="sub-row"><td></td><td colspan="4" class="dim"><b>Failed:</b> ${fh}</td></tr>`;
      }
    }
    html += `</tbody></table>`;
    if (pf.status && pf.status !== "success") html += `<div class="note">${esc(pf.status)}: ${esc(pf.reason || "")}</div>`;
  }
  openModal(html);
}

/* ══ Daily / commits ═══════════════════════════════════════════ */

function renderDaily() {
  const el = $("#view-daily");
  if (!state.daily.length) { el.innerHTML = `<div class="note">No daily data yet — the daily workflow and backfill populate this.</div>`; return; }

  let html = `<div class="filters checkbox-group">${BACKENDS.map((b) =>
    `<label><input type="checkbox" class="bk-check" value="${esc(b)}" ${state.selectedBackends.has(b) ? "checked" : ""}> ${esc(BACKEND_LABEL[b])}</label>`).join("")}
    <span class="dim">latest vs previous regressions are shown as ▲/▼ chips</span>
  </div>`;

  html += `<div class="grid grid-2 section">
    <div class="card"><h3>Hello world compile time (bare)</h3><canvas id="daily-hello-bare"></canvas></div>
    <div class="card"><h3>Hello world compile time (std)</h3><canvas id="daily-hello-std"></canvas></div>
    <div class="card"><h3>Test suite duration</h3><canvas id="daily-test-dur"></canvas></div>
    <div class="card"><h3>Tests failed</h3><canvas id="daily-test-fail"></canvas></div>
  </div>`;

  html += `<div class="section"><h2>Daily records <span class="hint">▲/▼ = change vs previous point (time: up = regression)</span></h2><div class="card" style="overflow-x:auto"><table>
    <thead><tr><th>Date</th><th>Commit</th><th>Subject</th>${BACKENDS.map((b) => `<th>${esc(BACKEND_LABEL[b])} tests</th>`).join("")}<th>Status</th></tr></thead><tbody>`;
  const rows = state.daily.slice().reverse();
  rows.forEach((d, ri) => {
    const prevD = rows[ri + 1] || null;
    html += `<tr class="clickable" data-date="${esc(d.date)}">`;
    html += `<td><b>${esc(d.date)}</b></td><td class="mono">${esc((d.commit && d.commit.short) || "—")}</td><td class="dim" style="max-width:280px;overflow:hidden;text-overflow:ellipsis;white-space:nowrap">${esc(d.commit && d.commit.subject)}</td>`;
    for (const b of BACKENDS) {
      const rec = backendOf(d, b);
      const prevRec = prevD && backendOf(prevD, b);
      const t = rec && rec.tests;
      const chip = "";
      if (t) {
        const prevT = prevRec && prevRec.tests;
        if (prevT && t.failed != null && prevT.failed != null && t.duration_ms != null && prevT.duration_ms != null) {
          chip = regressChip(t.duration_ms, prevT.duration_ms, false);
        }
      }
      html += `<td>${t && t.passed != null
        ? `${testStatusBadge(t)} <span class="num ${t.failed > 0 ? "missing" : ""}">${t.passed}/${t.failed}</span> ${chip}`
        : "—"}</td>`;
    }
    html += `<td>${statusBadge(d.status || "success", "")}</td></tr>`;
  });
  html += `</tbody></table></div></div>`;
  el.innerHTML = html;

  $$("#view-daily tbody tr.clickable").forEach((tr) => {
    tr.addEventListener("click", () => openDailyModal(tr.dataset.date));
  });
  $$(".bk-check").forEach((c) => c.addEventListener("change", () => {
    state.selectedBackends = new Set($$(".bk-check:checked").map((x) => x.value));
    renderDailyCharts();
    renderModules();
    renderBackends();
  }));
  renderDailyCharts();
}

function renderDailyCharts() {
  const labels = state.daily.map((d) => d.date);
  const bks = BACKENDS.filter((b) => state.selectedBackends.has(b));

  for (const [id, kind] of [["daily-hello-bare", "hello_bare"], ["daily-hello-std", "hello_std"]]) {
    const datasets = bks.map((b) => ({
      label: BACKEND_LABEL[b], borderColor: colorOf(b), backgroundColor: colorOf(b),
      data: state.daily.map((d) => {
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
        data: state.daily.map((d) => { const t = backendOf(d, b) && backendOf(d, b).tests; return t && t.duration_ms != null ? t.duration_ms : null; }),
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
        label: BACKEND_LABEL[b], backgroundColor: colorOf(b) + "aa",
        data: state.daily.map((d) => { const t = backendOf(d, b) && backendOf(d, b).tests; return t && t.failed != null ? t.failed : null; }),
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
    html += `<h3>${esc(BACKEND_LABEL[b])}</h3><table><tbody>
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
    if (fh) {
      html += `<tr><td>Failed tests</td><td>${fh}</td></tr>`;
    }
    if (rec.modules && rec.modules.length) {
      html += `<tr><td>Modules (${rec.modules.length})</td><td class="dim">see Modules view</td></tr>`;
    }
    html += `</tbody></table>`;
  }
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

  let html = `<div class="filters">
    <label>Module <select id="mod-select">${names.map((n) => `<option value="${esc(n)}">${esc(n)}</option>`).join("")}</select></label>
    <label>Backend <select id="mod-backend"><option value="TCCCompiler">TCC</option><option value="Compiler">LLVM</option><option value="Interpreter">Interpreter</option></select></label>
  </div>`;
  html += `<div class="section"><h2>Module compilation time over dates</h2><div class="card"><canvas id="mod-trend"></canvas></div></div>`;
  html += `<div class="section"><h2>Latest module timings per backend</h2><div class="card" style="overflow-x:auto"><table>
    <thead><tr><th>Module</th>${BACKENDS.map((b) => `<th class="num">${esc(BACKEND_LABEL[b])} ms</th>`).join("")}</tr></thead><tbody>`;
  const latest = state.daily[state.daily.length - 1];
  for (const n of names) {
    html += `<tr><td>${esc(n)}</td>`;
    for (const b of BACKENDS) {
      const rec = latest && backendOf(latest, b);
      const m = rec && rec.modules && rec.modules.find((x) => x.name === n);
      html += `<td class="num">${m && m.millis != null ? m.millis : "—"}</td>`;
    }
    html += `</tr>`;
  }
  html += `</tbody></table></div></div>`;
  el.innerHTML = html;
  const sel = $("#mod-select"), bsel = $("#mod-backend");
  if (sel) sel.addEventListener("change", renderModuleChart);
  if (bsel) bsel.addEventListener("change", renderModuleChart);
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

  let html = `<div class="section"><h2>Latest daily comparison — <span class="mono">${esc(latest.date)}</span></h2><div class="card"><table>
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
  html += `</tbody></table></div></div>`;

  html += `<div class="grid grid-2 section">
    <div class="card"><h3>Hello world (bare) — daily, per backend</h3><canvas id="be-hello"></canvas></div>
    <div class="card"><h3>Test suite duration — daily, per backend</h3><canvas id="be-dur"></canvas></div>
  </div>`;
  html += `<div class="section"><h2>Backend history</h2><div class="card"><canvas id="be-passrate"></canvas></div></div>`;
  el.innerHTML = html;
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

/* ── shared chart options + colors ───────────────────────────── */

const COLORS = { TCCCompiler: "#3fb950", Compiler: "#58a6ff", Interpreter: "#d29922" };
const colorOf = (b) => COLORS[b] || "#8b949e";
const colorFor = (s) => ["#58a6ff", "#3fb950", "#d29922", "#f85149", "#bc8cff", "#39c5cf", "#ffa657", "#8b949e"][hashStr(s) % 8];
function hashStr(s) { let h = 0; for (let i = 0; i < s.length; i++) h = (h * 31 + s.charCodeAt(i)) | 0; return Math.abs(h); }

function baseOpts() {
  return {
    responsive: true,
    maintainAspectRatio: false,
    interaction: { mode: "index", intersect: false },
    plugins: {
      legend: { labels: { color: "#8b949e", boxWidth: 10, font: { size: 11 } } },
      tooltip: { backgroundColor: "#161b22", borderColor: "#2d333b", borderWidth: 1, titleColor: "#e6edf3", bodyColor: "#e6edf3" },
    },
    scales: {
      x: { ticks: { color: "#8b949e", maxRotation: 45, autoSkip: true, maxTicksLimit: 20, font: { size: 10 } }, grid: { color: "#1c2330" } },
      y: { ticks: { color: "#8b949e", font: { size: 10 } }, grid: { color: "#1c2330" } },
    },
  };
}
function baseLineOpts(yLabel) {
  const o = baseOpts();
  o.scales.y.title = { display: !!yLabel, text: yLabel, color: "#8b949e", font: { size: 10 } };
  return o;
}
function barOpts(yLabel) {
  const o = baseOpts();
  o.scales.y.title = { display: !!yLabel, text: yLabel, color: "#8b949e", font: { size: 10 } };
  return o;
}

/* ── modal ───────────────────────────────────────────────────── */

function openModal(html) {
  $("#modal-body").innerHTML = html;
  $("#modal").classList.remove("hidden");
}
function closeModal() { $("#modal").classList.add("hidden"); }
document.addEventListener("click", (e) => {
  if (e.target.closest("[data-close]")) closeModal();
  if (e.key && e.key === "Escape") closeModal();
});

/* ── tabs ────────────────────────────────────────────────────── */

function switchView(name) {
  $$(".tab").forEach((t) => t.classList.toggle("active", t.dataset.view === name));
  $$(".view").forEach((v) => v.classList.toggle("active", v.id === "view-" + name));
  if (name === "overview") renderOverviewCharts();
  if (name === "releases") renderReleaseCharts(); // async — platform records load lazily
  if (name === "daily") renderDailyCharts();
  if (name === "modules") renderModuleChart();
  if (name === "backends") renderBackendCharts();
}
$$(".tab").forEach((t) => t.addEventListener("click", () => switchView(t.dataset.view)));

document.addEventListener("DOMContentLoaded", init);
