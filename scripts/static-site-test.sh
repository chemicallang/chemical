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
rm -rf "$shared_dir" "$baseline_dir" \
       lang/compiled/webserve_out \
       lang/tests/web/fixtures/static_site/build \
       lang/tests/web/fixtures/static_site_baseline/build \
       lang/tests/web/fixtures/webserve/build

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
check "the site builds 6 artifacts (2 pages, bundle, CSS, 2 chunks)" \
      "$([ "$(grep -o 'artifacts=[0-9]*' "$resp" | head -1)" = "artifacts=6" ] && echo 1 || echo 0)"
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
      "$(grep -qF 'stale etag -> 200 | 409 B' "$resp" && echo 1 || echo 0)"

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
check "each page has its own hashed chunk" \
      "$(grep -qE 'src="[^"]*index\.[0-9a-f]+\.js"' "$index" && \
         grep -qE 'src="[^"]*about_index\.[0-9a-f]+\.js"' "$about" && echo 1 || echo 0)"

# ── the bundle is written once and carries the runtime ─────────────────────
bundle="$shared_dir/assets/$(basename "$index_ref")"
check "the shared bundle exists on disk" "$([ -f "$bundle" ] && echo 1 || echo 0)"
check "the shared bundle carries the hydration runtime" \
      "$(contains "$bundle" '$__uni_dispatch')"

# ── the probe agrees: no violations, both assets counted as shared ─────────
# `python3` is not always on PATH in a Windows Git Bash shell; fall back to `python`.
py=""
for candidate in python3 python; do
    if command -v "$candidate" >/dev/null 2>&1 && "$candidate" -c 'import sys' >/dev/null 2>&1; then
        py="$candidate"
        break
    fi
done
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

if [ "$fail" -ne 0 ]; then
    echo
    echo "static-site-test: FAILED"
    exit 1
fi

echo
echo "shared:   $shared_html_bytes B of HTML for 2 pages, one bundle of $(wc -c < "$bundle") B served once"
echo "baseline: $baseline_bytes B for the same page, runtime inlined in every page"
echo
echo "PASS: static pages share one hashed bundle and ship no inline scripts"
