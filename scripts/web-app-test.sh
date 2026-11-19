#!/usr/bin/env bash
# App gate: `web::App` — one route table, mixed delivery modes, one bundle.
#
# Builds `lang/tests/web/fixtures/app` (two pre-rendered routes, two per-request
# routes, a redirect, a 404 route, a mounted copy) and checks the serving contract
# against the real bytes the build produces: cache policy per route, ETag/304 for
# pre-rendered pages, per-request data, pattern captures, redirects, the 404 route,
# sitemap/robots export, a mount prefix, and the `web::serve_request` entry point.
#
#   ./scripts/web-app-test.sh
set -eu

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(dirname "$here")"
cd "$root"

compiler="${CHX_COMPILER:-cmake-build-debug/TCCCompiler.exe}"
[ -x "$compiler" ] || compiler="cmake-build-debug/TCCCompiler"

out_dir="lang/compiled/app_out"
mount_dir="lang/compiled/app_mount_out"
rm -rf "$out_dir" "$mount_dir" lang/tests/web/fixtures/app/build
mkdir -p "$out_dir" "$mount_dir"

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
lacks() { grep -qF "$2" "$1" && echo 0 || echo 1; }

log=/tmp/web_app_test.txt
exe=lang/compiled/app_test.exe

echo "== building the mixed app =="
# Remove the old binary first: a silent compile failure must not be masked by a
# stale executable from a previous run.
rm -f "$exe"
"$compiler" lang/tests/web/fixtures/app/chemical.mod -o "$exe" >/tmp/web_app_build.txt 2>&1 || true
if [ ! -x "$exe" ]; then
    echo "FAIL fixture did not compile" >&2
    tail -20 /tmp/web_app_build.txt >&2
    exit 1
fi
"$exe" > "$log" 2>&1

line_of() { grep -F "app $1 ->" "$log" | head -1; }
status_of() { line_of "$1" | awk -F' \\| ' '{split($1,a," -> "); print a[2]}'; }
cc_of() { line_of "$1" | awk -F' \\| ' '{print $2}'; }
etag_of() { line_of "$1" | awk -F' \\| ' '{print $3}'; }
loc_of() { line_of "$1" | awk -F' \\| ' '{print $4}'; }
bytes_of() { line_of "$1" | awk -F' \\| ' '{print $5}'; }
body_of() { grep -F "body $1 = " "$log" | head -1 | sed "s/.*body $1 = //"; }

# ── Build ────────────────────────────────────────────────────────────────
check "the app builds (no error message)" \
      "$([ "$(contains "$log" 'build=1 msg=')" = "1" ] && echo 1 || echo 0)"
check "the app builds 8 artifacts: 2 pages, bundle, CSS, 2 chunks, sitemap, robots" \
      "$([ "$(contains "$log" 'artifacts=8 pages=2')" = "1" ] && echo 1 || echo 0)"
check "a pre-rendered route is exported as <route>/index.html" \
      "$([ "$(contains "$log" 'artifact index.html')" = "1" ] && [ "$(contains "$log" 'artifact pricing/index.html')" = "1" ] && echo 1 || echo 0)"
check "a per-request route is not exported (no account artifact)" \
      "$(lacks "$log" 'artifact account')"
check "sitemap.xml and robots.txt are part of the artifact set" \
      "$([ "$(contains "$log" 'artifact sitemap.xml')" = "1" ] && [ "$(contains "$log" 'artifact robots.txt')" = "1" ] && echo 1 || echo 0)"

# ── Pre-rendered route: cacheable, validatable ───────────────────────────
check "/ is served from the frozen artifact (200)" \
      "$([ "$(status_of /)" = "200" ] && echo 1 || echo 0)"
check "/ carries no-cache (revalidate every time)" \
      "$([ "$(cc_of /)" = "no-cache" ] && echo 1 || echo 0)"
check "/ carries a strong ETag" \
      "$([ -n "$(etag_of /)" ] && echo 1 || echo 0)"
check "/ serves the pre-rendered markup" \
      "$(contains "$log" 'pre-rendered once')"
check "a conditional request for / is a 304 with no body" \
      "$([ "$(status_of '/ conditional')" = "304" ] && [ "$(bytes_of '/ conditional')" = "0" ] && echo 1 || echo 0)"

# ── Per-request route: no-store, per-request data, pattern captures ──────
check "/account/42 is rendered per request (200)" \
      "$([ "$(status_of '/account/42')" = "200" ] && echo 1 || echo 0)"
check "a per-request route is no-store" \
      "$([ "$(cc_of '/account/42')" = "no-store" ] && echo 1 || echo 0)"
check "a per-request route has no ETag (its bytes are not frozen)" \
      "$([ -z "$(etag_of '/account/42')" ] && echo 1 || echo 0)"
check "{id} is captured into the render" \
      "$([ "$(body_of account42)" != "$(body_of account7)" ] && echo 1 || echo 0)"
check "/account/42 renders 42" \
      "$([ "$(body_of account42 | grep -c 'id=42')" = "1" ] && echo 1 || echo 0)"
check "/account/7 renders 7" \
      "$([ "$(body_of account7 | grep -c 'id=7')" = "1" ] && echo 1 || echo 0)"
check "/files/{path*} captures the rest of the path" \
      "$([ "$(status_of '/files/reports/2026/q1.pdf')" = "200" ] && [ "$(body_of files | grep -c 'path=reports/2026/q1.pdf')" = "1" ] && echo 1 || echo 0)"

# ── Redirects, 404 route, traversal ──────────────────────────────────────
check "/prices redirects with 308 to /pricing" \
      "$([ "$(status_of /prices)" = "308" ] && [ "$(loc_of /prices)" = "/pricing" ] && echo 1 || echo 0)"
check "an unknown path uses the 404 route" \
      "$([ "$(status_of /nope)" = "404" ] && [ "$(body_of missing | grep -c 'Nothing here')" = "1" ] && echo 1 || echo 0)"
check "a 404 route is not cached" \
      "$([ "$(cc_of /nope)" = "no-store" ] && echo 1 || echo 0)"
check "a traversal path is refused (404, not the asset table)" \
      "$([ "$(status_of '/assets/../etc/passwd')" = "404" ] && echo 1 || echo 0)"

# ── Export extras ────────────────────────────────────────────────────────
check "sitemap.xml lists the pre-rendered routes absolutely" \
      "$([ "$(body_of sitemap | grep -c '<loc>https://example.com/</loc>')" = "1" ] && [ "$(body_of sitemap | grep -c '<loc>https://example.com/pricing/</loc>')" = "1" ] && echo 1 || echo 0)"
check "sitemap.xml does not advertise a per-request route" \
      "$([ "$(body_of sitemap | grep -c 'account')" = "0" ] && echo 1 || echo 0)"
check "robots.txt points at the sitemap" \
      "$([ "$(body_of robots | grep -c 'Sitemap: https://example.com/sitemap.xml')" = "1" ] && echo 1 || echo 0)"

# ── Mount path ───────────────────────────────────────────────────────────
check "the mounted app answers /app/ (200)" \
      "$([ "$(status_of '/app/ mounted')" = "200" ] && echo 1 || echo 0)"
check "the mounted page references the bundle under the mount" \
      "$([ "$(body_of mount | grep -c '/app/assets/app.')" -ge 1 ] && [ "$(body_of mount | grep -c 'href="assets/')" = "0" ] && echo 1 || echo 0)"
check "the mounted app does not answer the bare root (404)" \
      "$([ "$(status_of '/ mounted')" = "404" ] && echo 1 || echo 0)"

# ── Router-shaped entry point ────────────────────────────────────────────
check "web::serve_request serves the installed app" \
      "$([ "$(status_of installed)" = "200" ] && echo 1 || echo 0)"
check "serve_request answers the pre-rendered root with its ETag" \
      "$([ "$(etag_of installed)" = "$(etag_of /)" ] && echo 1 || echo 0)"

# ── Export to disk ───────────────────────────────────────────────────────
check "finish() writes the artifact set" \
      "$([ "$(contains "$log" 'finish=1 msg=')" = "1" ] && [ -f "$out_dir/index.html" ] && [ -f "$out_dir/pricing/index.html" ] && [ -f "$out_dir/sitemap.xml" ] && [ -f "$out_dir/robots.txt" ] && echo 1 || echo 0)"
check "the bundle is written with its content hash" \
      "$(ls "$out_dir"/assets/app.*.js >/dev/null 2>&1 && echo 1 || echo 0)"
check "no per-request route is written to disk" \
      "$([ ! -e "$out_dir/account" ] && echo 1 || echo 0)"
check "a request after the build keeps working" \
      "$([ "$(status_of '/account/42 again')" = "200" ] && echo 1 || echo 0)"

# ── Shell + slots: frozen chrome, spliced fill (§3.12) ────────────────────
check "shell routes are compiled at build with their slots recorded" \
      "$([ "$(contains "$log" 'shell routes=2 frozen=')" = "1" ] && [ "$(contains "$log" 'compiled=1 slots=1')" = "1" ] && echo 1 || echo 0)"
check "a shell route is served with its per-user cache policy" \
      "$([ "$(status_of /dash)" = "200" ] && [ "$(cc_of /dash)" = "private, no-cache" ] && [ -z "$(etag_of /dash)" ] && echo 1 || echo 0)"
check "the fill lands in the frozen region and the chrome is intact" \
      "$([ "$(contains "$log" 'shell fill=1 chrome=1')" = "1" ] && echo 1 || echo 0)"
check "shell emit is byte-identical to a full SSR of the same route" \
      "$([ "$(contains "$log" 'shell eq=1')" = "1" ] && echo 1 || echo 0)"
check "a text fill is escaped (no raw markup can break out)" \
      "$([ "$(contains "$log" 'shell escaped=1 raw=0')" = "1" ] && echo 1 || echo 0)"
check "a shell route is not exported as a pre-rendered page" \
      "$(lacks "$log" 'artifact dash')"
check "a malformed slot marker is refused at compile, not served" \
      "$([ "$(contains "$log" 'shell malformed=1')" = "1" ] && echo 1 || echo 0)"
check "emit refuses a wrong fill count instead of serving a hole" \
      "$([ "$(contains "$log" 'arity=1 slots=1')" = "1" ] && echo 1 || echo 0)"
check "a shell route is at least 2x a full SSR of the same page" \
      "$([ "$(grep -F 'shell ns_shell=' "$log" | sed 's/.*ratio=//' | awk '{print ($1 >= 2.0)}')" = "1" ] && echo 1 || echo 0)"
echo "     $(grep -F 'shell ns_shell=' "$log" | head -1)"

# ── Sessions, cookies, CSRF ───────────────────────────────────────────────
check "an anonymous request has no session" \
      "$([ "$(contains "$log" 'session anonymous=0 count=0')" = "1" ] && echo 1 || echo 0)"
check "a new session id is 48 hex characters" \
      "$([ "$(contains "$log" 'session created id_len=48')" = "1" ] && echo 1 || echo 0)"
check "a session gets a record only when it is written to" \
      "$([ "$(contains "$log" 'session created id_len=48 count=0')" = "1" ] && [ "$(contains "$log" 'count=1')" = "1" ] && echo 1 || echo 0)"
check "session values round-trip through the store" \
      "$([ "$(contains "$log" 'session values user=1 role=1')" = "1" ] && [ "$(contains "$log" 'session user=ada')" = "1" ] && echo 1 || echo 0)"
check "the session cookie is HttpOnly, Lax and Path=/" \
      "$([ "$(contains "$log" 'Path=/; SameSite=Lax; HttpOnly')" = "1" ] && echo 1 || echo 0)"
check "the session cookie is a 48-hex id and carries no session values" \
      "$([ "$(grep -F 'session setcookie=' "$log" | grep -cE 'session=[0-9a-f]{48}; Path=/; SameSite=Lax; HttpOnly')" = "1" ] && [ "$(grep -F 'session setcookie=' "$log" | grep -c 'ada')" = "0" ] && echo 1 || echo 0)"
check "cookie attributes are serialised (Max-Age, SameSite, Secure)" \
      "$([ "$(contains "$log" 'cookie custom=theme=dark; Path=/; Max-Age=3600; SameSite=Lax; Secure; HttpOnly')" = "1" ] && echo 1 || echo 0)"
check "an unserialisable cookie name yields no header" \
      "$([ "$(contains "$log" 'cookie bad_len=0')" = "1" ] && echo 1 || echo 0)"
check "a request carrying the session cookie loads the session back" \
      "$([ "$(contains "$log" 'session loaded=1 user=ada')" = "1" ] && echo 1 || echo 0)"
check "the CSRF token verifies itself and rejects another value" \
      "$([ "$(contains "$log" 'session csrf_ok=1 csrf_bad=0')" = "1" ] && echo 1 || echo 0)"
check "the cookie jar parses several cookies from one header" \
      "$([ "$(contains "$log" 'jar cookies=3 theme=dark')" = "1" ] && echo 1 || echo 0)"
check "two new sessions never share an id" \
      "$([ "$(contains "$log" 'session distinct=1')" = "1" ] && echo 1 || echo 0)"
check "destroying a session drops the server record" \
      "$([ "$(contains "$log" 'session destroyed loaded=0 count=0')" = "1" ] && echo 1 || echo 0)"

if [ "$fail" -ne 0 ]; then
    echo "web-app-test: FAILED" >&2
    exit 1
fi
echo "web-app-test: PASS (one route table, mixed delivery, one bundle)"
