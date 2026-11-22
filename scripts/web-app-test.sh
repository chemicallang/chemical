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

# Shared chunk-graph verifier (closure + duplicate checks).
source "$here/web-graph.sh"

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
# Two pages, the runtime bundle, the vendor bundle, CSS, one content-addressed
# chunk (both routes' dispatches are identical, so they share the file),
# sitemap and robots.
check "the app builds 8 artifacts: 2 pages, bundle, vendor, CSS, 1 chunk, sitemap, robots" \
      "$([ "$(contains "$log" 'artifacts=8 pages=2')" = "1" ] && echo 1 || echo 0)"
check "the shared component definitions ship in one vendor bundle" \
      "$([ "$(contains "$log" 'artifact assets/app.vendor.')" = "1" ] && echo 1 || echo 0)"
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
# A component-free page mounts no runtime, so the mount's correctness shows up as
# the *absence* of any root-relative asset URL: nothing may point outside `/app/`.
check "the mounted page references no root-relative asset" \
      "$([ "$(body_of mount | grep -c 'href="assets/')" = "0" ] && [ "$(body_of mount | grep -c 'src="assets/')" = "0" ] && echo 1 || echo 0)"
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
      "$([ "$(contains "$log" 'shell routes=4 frozen=')" = "1" ] && [ "$(contains "$log" 'compiled=1 slots=1')" = "1" ] && echo 1 || echo 0)"
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
check "a shell variant serves its own frozen document per request" \
      "$([ "$(contains "$log" 'variant admin=1 guest=1')" = "1" ] && echo 1 || echo 0)"

# ── Unit graph (§3.13) ───────────────────────────────────────────────────
# `/pricing` is the only route that renders `PricingNote`, so its definition
# travels in that route's chunk; `DashBody` is rendered by the shell and live
# routes, so it must stay in the bundle (a per-request page dispatches from the
# bundle, never from a static route's chunk).
check "the graph moves a route-only component and keeps a per-request one" \
      "$([ "$(contains "$log" 'graph chunked=1 retained=1 unbacked=0')" = "1" ] && [ "$(contains "$log" 'note=[]')" = "1" ] && echo 1 || echo 0)"
check "a per-request route's component stays in the shared bundle" \
      "$(grep -qF 'function app_test_DashBody(props)' "$out_dir"/assets/app.*.js && echo 1 || echo 0)"
check "the route-only component is not in the shared bundle" \
      "$(grep -qF 'function app_test_PricingNote(props)' "$out_dir"/assets/app.*.js && echo 0 || echo 1)"
# Chunks are content-addressed, so the definition lives in whichever `chunk.*.js`
# carries those dispatch bytes.
check "the route chunk defines the component it dispatches" \
      "$(grep -qF 'function app_test_PricingNote(props)' "$out_dir"/assets/chunk.*.js && echo 1 || echo 0)"
check "every dispatched component is reachable from its page" \
      "$(graph_check "$out_dir" >/tmp/web_app_graph.txt && echo 1 || echo 0)"

# This fixture builds minified (`app.set_minify(true)`), which is the only place a
# *chunked* graph meets the minifier: a unit range is recomputed from minified
# bytes, then `plan_chunks` cuts a route's chunk out of it. Everything above this
# line runs on those bytes, so the graph checks double as the minifier's graph
# check; here we add the shape of the output itself.
check "the app bundle is minified (no indented lines left)" \
      "$(grep -qE '^[[:space:]]{2,}' "$out_dir"/assets/app.*.js && echo 0 || echo 1)"
check "the app bundle carries no comment lines" \
      "$(grep -qE '^[[:space:]]*//' "$out_dir"/assets/app.*.js && echo 0 || echo 1)"
if command -v node >/dev/null 2>&1; then
    check "node parses the minified app bundle and its routes' chunks" \
          "$(for f in "$out_dir"/assets/*.js; do node --check "$f" >/dev/null 2>&1 || exit 1; done; echo 1)"
else
    echo "skip node --check: node is not installed"
fi
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
check "rotating a session keeps its values and kills the old id" \
      "$([ "$(contains "$log" 'session rotated new=1 kept=1 old_dead=1')" = "1" ] && echo 1 || echo 0)"
check "destroying a session drops the server record" \
      "$([ "$(contains "$log" 'session destroyed loaded=0 count=0')" = "1" ] && echo 1 || echo 0)"

# ── Durable snapshots ─────────────────────────────────────────────────────
# `SessionStore.save_to`/`load_from` write and read a whole store, so a session
# survives the process that created it (a worker restart, a redeploy).
check "a session store writes a snapshot and reads it back" \
      "$([ "$(contains "$log" 'persist saved=1 count=1')" = "1" ] && echo 1 || echo 0)"
check "a snapshot round-trips every value shape (note, utf8, empty, 400-byte)" \
      "$([ "$(contains "$log" 'user=1 note=1 utf8=1 empty=1 long=400')" = "1" ] && echo 1 || echo 0)"
check "a truncated snapshot keeps the complete records before it" \
      "$([ "$(contains "$log" 'persist damaged wrote=1 loaded=1 count=1 value=1')" = "1" ] && echo 1 || echo 0)"
check "a missing snapshot loads an empty store instead of erroring" \
      "$([ "$(contains "$log" 'persist missing loaded=0 count=0')" = "1" ] && echo 1 || echo 0)"
saved_line="$(grep -F 'persist saved=' "$log" | head -1)"
snap_id="$(echo "$saved_line" | sed -n 's/.* id=\([0-9a-f]*\).*/\1/p')"
snap_csrf="$(echo "$saved_line" | sed -n 's/.* csrf=\([0-9a-f]*\).*/\1/p')"
restore_log=/tmp/web_app_restore.txt
CHX_SESSION_RESTORE=1 CHX_SESSION_ID="$snap_id" CHX_SESSION_CSRF="$snap_csrf" \
    "$exe" > "$restore_log" 2>&1 || true
check "a snapshot restores in a process that never saw the session" \
      "$([ "$(contains "$restore_log" 'persist restored loaded=1 count=1 user=1 note=1')" = "1" ] && echo 1 || echo 0)"
check "a restored session keeps every value and its CSRF token" \
      "$([ "$(contains "$restore_log" 'utf8=1 empty=1 csrf=1')" = "1" ] && echo 1 || echo 0)"

# ── Durable store: the directory is the store ────────────────────────────
# `make_durable(dir)` writes one file per session, so a second process over the same
# directory reads the session back with no snapshot call, and a restart loses nothing.
check "a durable store reads its records back from the directory" \
      "$([ "$(contains "$log" 'durable writer reopened=1 user=1')" = "1" ] && echo 1 || echo 0)"
check "a session id that is not hex never reaches the filesystem" \
      "$([ "$(contains "$log" 'durable writer reopened=1 user=1 refused=1')" = "1" ] && echo 1 || echo 0)"
check "an idle session expires and sweep_expired reclaims the abandoned one" \
      "$([ "$(contains "$log" 'durable ttl live=1 expired=1 swept=1')" = "1" ] && echo 1 || echo 0)"
durable_line="$(grep -F 'durable writer' "$log" | head -1)"
dur_id="$(echo "$durable_line" | sed -n 's/.* id=\([0-9a-f]*\).*/\1/p')"
dur_csrf="$(echo "$durable_line" | sed -n 's/.* csrf=\([0-9a-f]*\).*/\1/p')"
durable_log=/tmp/web_app_durable.txt
CHX_SESSION_DURABLE_RESTORE=1 CHX_SESSION_DURABLE_ID="$dur_id" CHX_SESSION_DURABLE_CSRF="$dur_csrf" \
    "$exe" > "$durable_log" 2>&1 || true
check "a durable session survives in a process that only has the directory" \
      "$([ "$(contains "$durable_log" 'durable restored loaded=1 user=1 note=1')" = "1" ] && echo 1 || echo 0)"
check "a restored durable session still verifies its CSRF token" \
      "$([ "$(contains "$durable_log" 'csrf=1 count=1')" = "1" ] && echo 1 || echo 0)"

if [ "$fail" -ne 0 ]; then
    echo "web-app-test: FAILED" >&2
    exit 1
fi
echo "web-app-test: PASS (one route table, mixed delivery, one bundle)"
