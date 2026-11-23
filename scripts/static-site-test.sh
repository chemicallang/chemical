#!/usr/bin/env bash
# Static-site gate: two pages, one shared bundle, no inline scripts.
#
# Builds the same two pages twice — once through `page.StaticSite` (shared,
# content-hashed bundle) and once with plain `HtmlPage.toString()` (every page
# carries its own runtime) — then checks the artifacts with `web_probe.py`.
#
#   ./scripts/static-site-test.sh
set -eu

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(dirname "$here")"
cd "$root"

compiler="${CHX_COMPILER:-cmake-build-debug/TCCCompiler.exe}"
[ -x "$compiler" ] || compiler="cmake-build-debug/TCCCompiler"

shared_dir="lang/compiled/static_site_out"
baseline_dir="lang/compiled/static_site_baseline_out"
min_dir="lang/compiled/static_site_min_out"
gz_dir="lang/compiled/static_site_gz_out"
rm -rf "$shared_dir" "$baseline_dir" \
       lang/compiled/static_site_min_out \
       lang/compiled/static_site_gz_out \
       lang/compiled/static_site_profile_out \
       lang/compiled/static_site_mpa_out \
       lang/compiled/webserve_out \
       lang/tests/web/fixtures/static_site/build \
       lang/tests/web/fixtures/static_site_baseline/build \
       lang/tests/web/fixtures/webserve/build \
       lang/compiled/webserve_socket_out \
       lang/tests/web/fixtures/webserve_socket/build

# `python3` is not always on PATH in a Windows Git Bash shell; fall back to `python`.
# Discovered once, up here, because more than the probe uses it now.
py=""
for candidate in python3 python; do
    if command -v "$candidate" >/dev/null 2>&1 && "$candidate" -c 'import sys' >/dev/null 2>&1; then
        py="$candidate"
        break
    fi
done

fail=0
check() {
    if [ "$2" = "1" ]; then
        echo "ok   $1"
    else
        echo "FAIL $1" >&2
        fail=1
    fi
}
contains() { grep -qF "$2" "$1" && echo 1 || echo 0; }

build_and_run() {
    local fixture="$1" exe="$2"
    "$compiler" "lang/tests/web/fixtures/$fixture/chemical.mod" -o "$exe" >/dev/null 2>&1
    "./$exe" >/dev/null
}

echo "== building the shared site =="
build_and_run static_site lang/compiled/static_site.exe
echo "== building the per-page baseline =="
build_and_run static_site_baseline lang/compiled/static_site_baseline.exe

# The fixture also serves its own build from memory (`respond()`) and prints one
# `<path> -> <status> | <content-type> | <cache-control> | <bytes>` line per
# request, so the serving contract is checked against the real HTML/JS bytes.
"./lang/compiled/static_site.exe" > /tmp/static_site_respond.txt 2>&1
resp=/tmp/static_site_respond.txt
status_of() { grep -F "respond $1 ->" "$resp" | head -1 | sed 's/.* -> \([0-9]*\) .*/\1/'; }
# Per-route chunks are content-named, so this fixture's two identical chunks
# collapse into one artifact: 6 today, 7 if the two routes ever diverge.
artifact_count=$(grep -o 'artifacts=[0-9]*' "$resp" | head -1 | cut -d= -f2)
check "the site builds 2 pages, one bundle, vendor, CSS and collapsed route chunks" \
      "$([ "${artifact_count:-0}" -ge 6 ] && [ "${artifact_count:-0}" -le 7 ] && echo 1 || echo 0)"
check "in memory, / serves a 200 HTML page" \
      "$([ "$(status_of /)" = "200" ] && grep -qF 'respond / -> 200 | text/html' "$resp" && echo 1 || echo 0)"
check "in memory, a nested route serves 200 at /about/ and /about" \
      "$([ "$(status_of /about/)" = "200" ] && [ "$(status_of /about)" = "200" ] && echo 1 || echo 0)"
check "in memory, an unknown path is a 404" \
      "$([ "$(status_of /missing)" = "404" ] && echo 1 || echo 0)"
check "in memory, a traversal attempt is a 404" \
      "$([ "$(status_of /../secret)" = "404" ] && echo 1 || echo 0)"
check "in memory, the hashed bundle is served immutably as text/javascript" \
      "$(grep -qF '| text/javascript; charset=utf-8 | public, max-age=31536000, immutable |' "$resp" && echo 1 || echo 0)"
check "in memory, revalidating the bundle is a 304 with no body" \
      "$(grep -qF 'conditional bundle -> 304 | 0 B | public, max-age=31536000, immutable' "$resp" && echo 1 || echo 0)"
check "in memory, revalidating a page is a 304 that stays no-cache" \
      "$(grep -qF 'conditional page -> 304 | 0 B | no-cache' "$resp" && echo 1 || echo 0)"
check "in memory, a stale ETag gets the full page again" \
      "$(grep -qE 'stale etag -> 200 \| [0-9]+ B' "$resp" && echo 1 || echo 0)"

# ── serving cost: the body is written, not copied ───────────────────────────
# `http::ResponseBody.View` borrows the frozen bytes, so answering a 100+ KB asset
# costs the head plus one write. Both halves are checked because either alone can
# be gamed: `view=1` is the structural fact, the ratio is the consequence (a copy
# put the asset at ~3.5x a small page; without one it is ~1.9x).
serve_line=$(grep 'serve asset=' "$resp" | head -1)
check "a served bundle body is borrowed, not copied (view=1)" \
      "$([ "$(echo "$serve_line" | grep -o 'view=1' | wc -l | tr -d ' ')" = "2" ] && echo 1 || echo 0)"
asset_ns=$(echo "$serve_line" | sed 's/.*| page=[0-9]* B \([0-9]*\) ns\/req.*/\1/')
asset_big_ns=$(echo "$serve_line" | sed 's/.*asset=[0-9]* B \([0-9]*\) ns\/req.*/\1/')
check "a 100 KB asset costs well under 2x a 400 B page (no per-byte work)" \
      "$([ "${asset_big_ns:-0}" -gt 0 ] && [ "${asset_ns:-0}" -gt 0 ] && [ $((asset_big_ns * 10)) -lt $((asset_ns * 25)) ] && echo 1 || echo 0)"
echo "serving: $serve_line"

# ── minification: same graph, smaller bytes ────────────────────────────────
# The fixture builds the site twice, raw and with `set_minify(true)`. Same pages,
# same per-route plan, so the difference is the minifier's whole effect.
min_line=$(grep 'minify js=' "$resp" | head -1)
min_js_raw=$(echo "$min_line" | sed 's/.*js=\([0-9]*\)->.*/\1/')
min_js_out=$(echo "$min_line" | sed 's/.*js=[0-9]*->\([0-9]*\).*/\1/')
min_css_raw=$(echo "$min_line" | sed 's/.*css=\([0-9]*\)->.*/\1/')
min_css_out=$(echo "$min_line" | sed 's/.*css=[0-9]*->\([0-9]*\).*/\1/')
# `chunked=<raw>/<minified>`: the plan must be the same on both sides, because a
# unit range is recomputed from minified bytes before a chunk is cut out of it.
min_chunked_raw=$(echo "$min_line" | sed 's/.*chunked=\([0-9]*\)\/[0-9]*.*/\1/')
min_chunked_out=$(echo "$min_line" | sed 's/.*chunked=[0-9]*\/\([0-9]*\).*/\1/')
check "the minified bundle keeps the raw build's chunk plan ($min_chunked_raw/$min_chunked_out)" \
      "$([ "$min_chunked_raw" = "$min_chunked_out" ] && echo 1 || echo 0)"
check "minification cuts the JS bundle by at least 40% ($min_js_raw -> $min_js_out B)" \
      "$([ "${min_js_out:-0}" -gt 0 ] && [ $((min_js_out * 10)) -lt $((min_js_raw * 6)) ] && echo 1 || echo 0)"
check "minification never grows the CSS ($min_css_raw -> $min_css_out B)" \
      "$([ "${min_css_out:-1}" -le "${min_css_raw:-0}" ] && echo 1 || echo 0)"
check "the minified build's page references its own hashed bundle" \
      "$(grep -qF 'minify page=' "$resp" && grep -qF 'refs_own=1' "$resp" && echo 1 || echo 0)"
# The written bytes, not a print: a parser is the only thing that can say the
# minified bundle is still JavaScript. Skipped (not passed) when node is absent.
min_bundle=$(grep 'min_js_path=' "$resp" | head -1 | sed 's/.*min_js_path=\([^ ]*\).*/\1/')
if command -v node >/dev/null 2>&1; then
    check "node parses the minified bundle" \
          "$(node --check "$min_dir/$min_bundle" >/dev/null 2>&1 && echo 1 || echo 0)"
else
    echo "skip node --check: node is not installed"
fi

echo "minify: $min_line"

# ── gzip precompression: two representations of one URL ────────────────────
# A third build of the same site, differing only by `set_gzip(true)`. The fixture
# reports both representations, the negotiation for four different
# `Accept-Encoding` values, and whether the ETags can be confused.
gz_line=$(grep 'gzip js=' "$resp" | head -1)
gz_in=$(echo "$gz_line" | sed 's/.*js=\([0-9]*\)->.*/\1/')
gz_out=$(echo "$gz_line" | sed 's/.*js=[0-9]*->\([0-9]*\).*/\1/')
check "a request without Accept-Encoding gets the identity body" \
      "$(echo "$gz_line" | grep -q 'plain_enc=0' && echo 1 || echo 0)"
check "Accept-Encoding: gzip gets a gzip body with Content-Encoding and Vary" \
      "$(echo "$gz_line" | grep -q 'enc=1 vary=1' && echo 1 || echo 0)"
check "Accept-Encoding: identity gets the identity body back" \
      "$(echo "$gz_line" | grep -q 'refused=1' && echo 1 || echo 0)"
check "gzip;q=0 is a refusal, not a preference" \
      "$(echo "$gz_line" | grep -q 'q0=1' && echo 1 || echo 0)"
check "gzip cuts the bundle by at least 60% ($gz_in -> $gz_out B)" \
      "$([ "${gz_out:-0}" -gt 0 ] && [ $((gz_out * 10)) -lt $((gz_in * 4)) ] && echo 1 || echo 0)"
page_line=$(grep 'gzip page=' "$resp" | head -1)
page_in=$(echo "$page_line" | sed 's/.*page=\([0-9]*\)->.*/\1/')
page_out=$(echo "$page_line" | sed 's/.*page=[0-9]*->\([0-9]*\).*/\1/')
check "the HTML page is compressed too and does not grow ($page_in -> $page_out B)" \
      "$([ "${page_out:-0}" -gt 0 ] && [ "$page_out" -lt "$page_in" ] && echo 1 || echo 0)"
etag_line=$(grep 'gzip etag_distinct=' "$resp" | head -1)
check "the two representations carry different ETags" \
      "$(echo "$etag_line" | grep -q 'etag_distinct=1' && echo 1 || echo 0)"
check "a gzip ETag revalidates to a 304 for gzip" \
      "$(echo "$etag_line" | grep -q 'not_modified=1' && echo 1 || echo 0)"
check "the same ETag does not 304 the identity representation" \
      "$(echo "$etag_line" | grep -q "cross=$min_js_out" && echo 1 || echo 0)"

# `finish()` writes `<path>.gz` beside each artifact so a static host's
# `gzip_static` can use them without recompressing. Node is the outside check
# that the bytes are a real gzip stream: skipped (not passed) when it is absent.
if command -v node >/dev/null 2>&1; then
    gz_ok=1
    gz_count=0
    while IFS= read -r gzfile; do
        identity="${gzfile%.gz}"
        gz_count=$((gz_count + 1))
        node -e 'const z=require("zlib"),f=require("fs");process.exit(z.gunzipSync(f.readFileSync(process.argv[1])).equals(f.readFileSync(process.argv[2]))?0:1)' \
            "$gzfile" "$identity" >/dev/null 2>&1 || gz_ok=0
    done < <(find "$gz_dir" -name '*.gz' | sort)
    check "node gunzips every written .gz sidecar back to its artifact ($gz_count files)" \
          "$([ "$gz_count" -gt 0 ] && [ "$gz_ok" = "1" ] && echo 1 || echo 0)"
else
    echo "skip node gunzip: node is not installed"
fi

# ── byte budgets ───────────────────────────────────────────────────────────
# Numeric ceilings on the minified and compressed bytes. A budget that only
# compares two builds cannot notice both of them growing; these are absolute.
BUDGET_BUNDLE_MIN=60000     # minified shared runtime bundle
BUDGET_BUNDLE_GZ=20000      # the same bundle, gzipped
BUDGET_PAGE=2048            # one HTML page (its own SSR bytes)
BUDGET_CHUNK=8192           # one per-route script chunk
budget() {
    local name="$1" value="$2" limit="$3"
    check "budget: $name is $value B (limit $limit B)" \
          "$([ "${value:-0}" -gt 0 ] && [ "$value" -le "$limit" ] && echo 1 || echo 0)"
}
budget "minified bundle" "$min_js_out" "$BUDGET_BUNDLE_MIN"
budget "gzipped bundle" "$gz_out" "$BUDGET_BUNDLE_GZ"
budget "page HTML" "$page_in" "$BUDGET_PAGE"
# The shared bundle and the vendor bundle are budgeted above; a per-route chunk is
# everything else (`index.<hash>.js`, `about_index.<hash>.js`).
gz_bundle=$(grep 'gzip written=' "$resp" | head -1 | sed 's/.*gzip written=//')
largest_chunk=0
for chunk in "$gz_dir"/assets/*.js; do
    [ -f "$chunk" ] || continue
    rel="assets/$(basename "$chunk")"
    [ "$rel" = "$gz_bundle" ] && continue
    case "$rel" in *".vendor."*) continue ;; esac
    n=$(wc -c < "$chunk")
    [ "$n" -gt "$largest_chunk" ] && largest_chunk="$n"
done
budget "largest route chunk (not the bundle)" "$largest_chunk" "$BUDGET_CHUNK"

# ── profile: per-route bytes, chunk composition, navigation cost ───────────
# The report is printed between `---profile---` markers; every number in it is
# recomputed from the artifact table, so a route line that does not add up is a
# profile bug, not a rounding difference.
profile_txt=$(awk '/^---profile---$/{f=1;next}/^---end---$/{f=0}f' "$resp")
shared_txt=$(printf '%s\n' "$profile_txt" | grep '^shared ')
total_txt=$(printf '%s\n' "$profile_txt" | grep '^total ')
prof_bundle=$(echo "$shared_txt" | sed 's/.*bundle=\([0-9]*\).*/\1/')
prof_gzip=$(echo "$shared_txt" | sed 's/^shared bundle=[0-9]* gzip=\([0-9]*\).*/\1/')
prof_css=$(echo "$shared_txt" | sed 's/.* css=\([0-9]*\).*/\1/')
prof_vendor=$(echo "$shared_txt" | sed 's/.* vendor=\([0-9]*\).*/\1/')
prof_shared=$((prof_bundle + prof_vendor + prof_css))
check "the profile reports the shared bytes it actually serves ($prof_shared B)" \
      "$([ "$prof_bundle" = "$min_js_out" ] && [ "$prof_gzip" = "$gz_out" ] && echo 1 || echo 0)"
check "the profile has one route line per pre-rendered page" \
      "$([ "$(printf '%s\n' "$profile_txt" | grep -c '^route ')" = "2" ] && echo 1 || echo 0)"
prof_ok=1
while IFS= read -r line; do
    [ -n "$line" ] || continue
    html=$(echo "$line" | sed 's/.*html=\([0-9]*\).*/\1/')
    chunk=$(echo "$line" | sed 's/.*chunk=\([0-9]*\).*/\1/')
    first=$(echo "$line" | sed 's/.*first_load=\([0-9]*\).*/\1/')
    nav=$(echo "$line" | sed 's/.*navigation=\([0-9]*\).*/\1/')
    [ "$first" = "$((html + chunk + prof_shared))" ] || prof_ok=0
    [ "$nav" = "$((html + chunk))" ] || prof_ok=0
done < <(printf '%s\n' "$profile_txt" | grep '^route ')
check "first_load and navigation add up on every route line" \
      "$([ "$prof_ok" = "1" ] && echo 1 || echo 0)"
check "the profile totals the route count" \
      "$(echo "$total_txt" | grep -q 'routes=2' && echo 1 || echo 0)"
if [ -n "$py" ]; then
    check "profile.json is valid JSON with the same route count" \
          "$("$py" -c 'import json,sys;d=json.load(open(sys.argv[1]));sys.exit(0 if len(d["routes"])==2 and d["shared"]["bundle"]>0 else 1)' \
              "$gz_dir/../static_site_profile_out/profile.json" >/dev/null 2>&1 && echo 1 || echo 0)"
else
    echo "skip profile.json: no usable python found"
fi
echo "profile: $(echo "$total_txt")"

# ── web::Static: the `--static` multi-page export ──────────────────────────
# A two-page export where the first page is `#html` only. It must ship no
# JavaScript at all — an already-shared bundle is still a download a page with no
# client behaviour does not need.
mpa_line=$(grep '^mpa ' "$resp" | head -1)
mpa_dir="lang/compiled/static_site_mpa_out"
check "web::Static exports both pages and their shared bundle" \
      "$(echo "$mpa_line" | grep -q 'pages=2' && echo 1 || echo 0)"
check "a page with no components ships zero <script> tags" \
      "$(echo "$mpa_line" | grep -q 'plain_scripts=0' && echo 1 || echo 0)"
check "a page with no components carries no hydration runtime" \
      "$(echo "$mpa_line" | grep -q 'plain_runtime=0' && echo 1 || echo 0)"
check "the component page still loads the shared scripts (2 tags)" \
      "$(echo "$mpa_line" | grep -q 'rich_scripts=2' && echo 1 || echo 0)"
check "the component page references the export's own bundle" \
      "$(echo "$mpa_line" | grep -q 'bundle=1' && echo 1 || echo 0)"
check "the export wrote both pages to disk" \
      "$([ -f "$mpa_dir/index.html" ] && [ -f "$mpa_dir/rich/index.html" ] && echo 1 || echo 0)"
check "the zero-JS page on disk has no <script>" \
      "$(grep -q '<script' "$mpa_dir/index.html" && echo 0 || echo 1)"
check "the nested page reaches the bundle with a ../ path" \
      "$(grep -qE 'src="\.\./assets/app\.[0-9a-f]+\.js"' "$mpa_dir/rich/index.html" && echo 1 || echo 0)"

index="$shared_dir/index.html"
about="$shared_dir/about/index.html"
[ -f "$index" ] || { echo "FAIL: $index was not written" >&2; exit 1; }
[ -f "$about" ] || { echo "FAIL: $about was not written" >&2; exit 1; }

# ── one bundle, referenced by both pages, under the right relative path ─────
index_ref=$(grep -o 'src="[^"]*app\.[0-9a-f]*\.js"' "$index" | head -1 | sed 's/src="//;s/"$//')
about_ref=$(grep -o 'src="[^"]*app\.[0-9a-f]*\.js"' "$about" | head -1 | sed 's/src="//;s/"$//')
check "the root page references the shared bundle ($index_ref)" \
      "$([ -n "$index_ref" ] && echo 1 || echo 0)"
check "the nested page references the same bundle by hash" \
      "$([ -n "$about_ref" ] && [ "$(basename "$index_ref")" = "$(basename "$about_ref")" ] && echo 1 || echo 0)"
check "the nested page uses a ../ relative path" \
      "$([ "$about_ref" = "../$index_ref" ] && echo 1 || echo 0)"

# ── no inline scripts (CSP), a page chunk per route ─────────────────────────
check "the root page has no inline <script>" "$(grep -qE '<script>' "$index" && echo 0 || echo 1)"
check "the nested page has no inline <script>" "$(grep -qE '<script>' "$about" && echo 0 || echo 1)"
# Per-route chunks are named by content (2026-11-21), so two routes whose chunk
# bytes are identical share one `chunk.<hash>.js`; the older per-route name is
# still emitted when the contents differ. Both shapes are correct — what matters
# is that each page references a content-hashed route chunk.
index_chunk=$(grep -oE 'src="[^"]*(chunk|index)\.[0-9a-f]+\.js"' "$index" | head -1 | sed 's/src="//;s/"$//')
about_chunk=$(grep -oE 'src="[^"]*(chunk|about_index)\.[0-9a-f]+\.js"' "$about" | head -1 | sed 's/src="//;s/"$//')
check "each page references a content-hashed route chunk" \
      "$([ -n "$index_chunk" ] && [ -n "$about_chunk" ] && echo 1 || echo 0)"

# ── per-request routes live in the same build as the pre-rendered ones ─────
# A site is a route table, not a mode: `/account` is rendered per request while
# `/` and `/about/` are served from the frozen artifact set. The dynamic page must
# load the *same* content-hashed bundle (no second runtime) and must not be
# cacheable, because the same URL returns different bytes.
dyn_ada=$(awk '/^---dynamic ada---$/{f=1;next}/^---end---$/{f=0}f' "$resp")
dyn_grace=$(awk '/^---dynamic grace---$/{f=1;next}/^---end---$/{f=0}f' "$resp")
check "a per-request route renders 200 with no-store and no validator" \
      "$(grep -qE '^dynamic ada -> 200 \| no-store \| [0-9]+ B \| etag=\[\]$' "$resp" && echo 1 || echo 0)"
check "two requests to the same route return different bytes" \
      "$(grep -qF 'same_bytes=0' "$resp" && echo 1 || echo 0)"
check "the dynamic page references the shared bundle the static pages use" \
      "$(printf '%s' "$dyn_ada" | grep -qF "$index_ref" && echo 1 || echo 0)"
check "the dynamic page does not repeat the hydration runtime" \
      "$(printf '%s' "$dyn_ada" | grep -qF '$__uni_hydration_queue' && echo 0 || echo 1)"
check "the dynamic page has exactly the shared scripts (vendor + bundle)" \
      "$([ "$(printf '%s' "$dyn_ada" | grep -o '<script src=' | wc -l)" = "2" ] && printf '%s' "$dyn_ada" | grep -qF 'app.vendor.' && echo 1 || echo 0)"
check "the dynamic page carries its own request-scoped dispatch inline" \
      "$(printf '%s' "$dyn_ada" | grep -qF 'window.$__uni_c(' && echo 1 || echo 0)"
check "a dynamic render sees the request's data, not the warm-pass data" \
      "$(printf '%s' "$dyn_ada" | grep -qF 'ada:0' && printf '%s' "$dyn_grace" | grep -qF 'grace:0' && echo 1 || echo 0)"
check "a route needing an un-bundled component is refused with a 503" \
      "$(grep -qF 'dynamic late -> 503 |' "$resp" && grep -qF 'frozen bundle' "$resp" && echo 1 || echo 0)"
check "a dynamic route is not written as an artifact" \
      "$([ ! -e "$shared_dir/account/index.html" ] && echo 1 || echo 0)"

# ── the bundle is written once and carries the runtime ─────────────────────
bundle="$shared_dir/assets/$(basename "$index_ref")"
check "the shared bundle exists on disk" "$([ -f "$bundle" ] && echo 1 || echo 0)"
check "the shared bundle carries the hydration runtime" \
      "$(contains "$bundle" '$__uni_dispatch')"

# ── the probe agrees: no violations, both assets counted as shared ─────────
if [ -z "$py" ]; then
    echo "skip web_probe: no usable python found" >&2
    check "web_probe --strict passes on the shared site" 1
    check "the probe counts the bundle as shared by both pages" 1
elif "$py" scripts/web_probe.py analyze "$shared_dir" --strict > /tmp/static_site_probe.txt 2>&1; then
    check "web_probe --strict passes on the shared site" 1
    check "the probe counts the bundle as shared by both pages" \
          "$(grep -q 'used by 2 pages' /tmp/static_site_probe.txt && echo 1 || echo 0)"
else
    check "web_probe --strict passes on the shared site" 0
    cat /tmp/static_site_probe.txt >&2
    check "the probe counts the bundle as shared by both pages" 0
fi

# ── the baseline shows what the bundle saves ───────────────────────────────
base_index="$baseline_dir/index.html"
baseline_bytes=$(wc -c < "$base_index")
shared_html_bytes=$(cat "$index" "$about" | wc -c)
check "the baseline inlines the runtime per page (>50 KB)" \
      "$([ "$baseline_bytes" -gt 50000 ] && echo 1 || echo 0)"
check "the shared pages are small (<2 KB of HTML for both)" \
      "$([ "$shared_html_bytes" -lt 2048 ] && echo 1 || echo 0)"

# ── the HTTP bridge maps a SiteResponse onto http::Response ─────────────────
echo "== building the HTTP bridge fixture =="
"$compiler" lang/tests/web/fixtures/webserve/chemical.mod -o lang/compiled/webserve.exe >/dev/null 2>&1
"./lang/compiled/webserve.exe" > /tmp/webserve_serve.txt 2>&1
web=/tmp/webserve_serve.txt
check "the bridge serves a page as 200 text/html with no-cache" \
      "$(grep -qF 'serve / -> 200 | text/html; charset=utf-8 | no-cache |' "$web" && echo 1 || echo 0)"
check "the route handler serves the installed site" \
      "$(grep -qF 'handler / -> 200 | text/html; charset=utf-8 | no-cache |' "$web" && echo 1 || echo 0)"
check "a catch-all router route dispatches into the site" \
      "$(grep -qF 'router / -> 200 | text/html; charset=utf-8 | no-cache |' "$web" && echo 1 || echo 0)"
check "the catch-all also reaches the site's own 404" \
      "$(grep -qE 'router /about/ -> 404 \|' "$web" && echo 1 || echo 0)"
check "the bridge carries the ETag header" \
      "$(grep -qE 'serve / -> 200 .* \| "[0-9a-f]{12}" \|' "$web" && echo 1 || echo 0)"
check "the bridge serves the bundle immutably as text/javascript" \
      "$(grep -qF 'serve bundle -> 200 | text/javascript; charset=utf-8 | public, max-age=31536000, immutable |' "$web" && echo 1 || echo 0)"
check "If-None-Match through the bridge is a 304 with no body" \
      "$(grep -qE 'if-none-match -> 304 \| .* \| 0 B$' "$web" && echo 1 || echo 0)"
check "a stale validator through the bridge returns the body" \
      "$(grep -qE 'stale-etag -> 200 \| .* \| [1-9][0-9]* B$' "$web" && echo 1 || echo 0)"
check "a missing path through the bridge is a 404 without a validator" \
      "$(grep -qE '/nope -> 404 \| .* \| no-etag \|' "$web" && echo 1 || echo 0)"
check "an uninstalled site is a 503, not a crash" \
      "$(grep -qF 'serve null -> 503 |' "$web" && echo 1 || echo 0)"

# ── the same build over a real socket (catch-all route, framework headers) ───
echo "== building the socket fixture =="
"$compiler" lang/tests/web/fixtures/webserve_socket/chemical.mod -o lang/compiled/webserve_socket.exe >/dev/null 2>&1
timeout 120 ./lang/compiled/webserve_socket.exe > /tmp/webserve_socket.txt 2>&1
sock=/tmp/webserve_socket.txt
check "a real GET / is 200 over the wire" \
      "$(grep -qF 'GET / -> HTTP/1.1 200 OK' "$sock" && echo 1 || echo 0)"
check "the page keeps its content type and cache policy on the wire" \
      "$(grep -qF 'Content-Type: text/html; charset=utf-8' "$sock" && \
         grep -qF 'Cache-Control: no-cache' "$sock" && echo 1 || echo 0)"
check "the page carries an ETag header" \
      "$(grep -qE '^ETag: "[0-9a-f]{12}"' "$sock" && echo 1 || echo 0)"
check "the shared bundle is served immutably over the wire" \
      "$(grep -qF 'GET bundle -> HTTP/1.1 200 OK' "$sock" && \
         grep -qF 'Content-Type: text/javascript; charset=utf-8' "$sock" && \
         grep -qF 'Cache-Control: public, max-age=31536000, immutable' "$sock" && echo 1 || echo 0)"

# The bundle body is borrowed, not owned: the encoder reads it straight from the
# frozen artifact, so the two things that could go wrong are a truncated write or a
# Content-Length computed from something else. Both are one comparison. (The probe
# escapes CRLF as literal text, so take the length by matching digits.)
bundle_cl=$(awk '/^GET bundle ->/{f=1} f && /^Content-Length:/ && match($0,/[0-9]+/){print substr($0,RSTART,RLENGTH); exit}' "$sock")
bundle_sent=$(awk '/^GET bundle ->/{f=1} f && / B body$/{n=$0; sub(/.*\| /,"",n); sub(/ B body$/,"",n); print n; exit}' "$sock")
check "a borrowed body is written whole (Content-Length == bytes on the wire)" \
      "$([ -n "$bundle_cl" ] && [ "$bundle_cl" = "$bundle_sent" ] && echo 1 || echo 0)"
check "an unknown route is a 404 over the wire" \
      "$(grep -qF 'GET /nope -> HTTP/1.1 404 Not Found' "$sock" && echo 1 || echo 0)"
check "If-None-Match is a 304 with no body over the wire" \
      "$(grep -qF 'GET / if-none-match -> HTTP/1.1 304 Not Modified' "$sock" && \
         grep -qF '| 0 B body' "$sock" && echo 1 || echo 0)"

if [ "$fail" -ne 0 ]; then
    echo
    echo "static-site-test: FAILED"
    exit 1
fi

echo
echo "shared:   $shared_html_bytes B of HTML for 2 pages, one bundle of $(wc -c < "$bundle") B served once"
echo "baseline: $baseline_bytes B for the same page, runtime inlined in every page"
echo
echo "PASS: static pages share one hashed bundle and ship no inline scripts; per-request routes join the same bundle"
