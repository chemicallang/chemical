#!/usr/bin/env bash
# Golden gate for the Phase 0 web fixtures (lang/docs/web-stack-plan.md §4).
#
# Each fixture builds a small site, and its artifact tree is compared byte for
# byte with `lang/tests/web/golden/<fixture>`. That is what makes the promises
# checkable: a change that adds JavaScript to a static page, drops a route
# title, loses the noindex on a miss, or moves hydration ids shows up as a diff
# instead of in a browser.
#
#   ./scripts/web-golden-test.sh            # verify
#   ./scripts/web-golden-test.sh --update   # rewrite the goldens (review the diff!)
#
# The fixtures and what each one pins:
#
#   static     pure `#html`, two pages, 0 bytes of JavaScript
#   universal  five instances of three call sites, hydration ids u1..u5
#   router     two deep links, a fallback, a miss without one (noindex)
#   shell      one layout, two personalised renders (the slot-shell baseline)
set -uo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(dirname "$here")"
cd "$root"

# Shared chunk-graph verifier (closure + duplicate checks).
source "$here/web-graph.sh"

update=0
for arg in "$@"; do
    case "$arg" in
        --update) update=1 ;;
        *) echo "web-golden-test: unknown option '$arg'" >&2; exit 2 ;;
    esac
done

compiler="${CHX_COMPILER:-cmake-build-debug/TCCCompiler}"
[ -x "$compiler" ] || compiler="cmake-build-debug/TCCCompiler.exe"
if [ ! -x "$compiler" ]; then
    echo "web-golden-test: no compiler at cmake-build-debug/TCCCompiler (build it with ./scripts/build.sh --tcc)" >&2
    exit 1
fi
compiler="$(cd "$(dirname "$compiler")" && pwd)/$(basename "$compiler")"

golden_root="lang/tests/web/golden"
fail=0
check() {
    if [ "$2" = "1" ]; then
        echo "ok   $1"
    else
        echo "FAIL $1" >&2
        fail=1
    fi
}
has()  { grep -qF -- "$2" "$1" && echo 1 || echo 0; }
hasnt() { grep -qF -- "$2" "$1" && echo 0 || echo 1; }

# A python for the probe. `python3` is not always on PATH in a Windows Git Bash.
py=""
for candidate in python3 python; do
    if command -v "$candidate" >/dev/null 2>&1 && "$candidate" -c 'import sys' >/dev/null 2>&1; then
        py="$candidate"
        break
    fi
done

probe_strict() {
    local dir="$1" label="$2"
    if [ -z "$py" ]; then
        check "$label: web_probe --strict (skipped: no python)" 1
        return
    fi
    if "$py" scripts/web_probe.py analyze "$dir" --strict > "/tmp/web_golden_probe_$(basename "$dir").txt" 2>&1; then
        check "$label: web_probe --strict" 1
    else
        check "$label: web_probe --strict (see /tmp/web_golden_probe_$(basename "$dir").txt)" 0
    fi
}

build_fixture() {
    local fixture="$1"
    local dir="lang/tests/web/fixtures/$fixture"
    rm -rf "$dir/output" "$dir/fixture.exe"
    if ! ( cd "$dir" && "$compiler" chemical.mod -o fixture.exe --mode debug_quick --no-cache \
             > "/tmp/web_golden_$fixture.build.log" 2>&1 ); then
        echo "FAIL $fixture: did not compile (see /tmp/web_golden_$fixture.build.log)" >&2
        fail=1
        return 1
    fi
    if ! ( cd "$dir" && ./fixture.exe > "/tmp/web_golden_$fixture.run.log" 2>&1 ); then
        echo "FAIL $fixture: did not run (see /tmp/web_golden_$fixture.run.log)" >&2
        fail=1
        return 1
    fi
    return 0
}

compare_golden() {
    local fixture="$1"
    local dir="lang/tests/web/fixtures/$fixture"
    local golden="$golden_root/$fixture"
    if [ ! -d "$dir/output" ]; then
        check "$fixture: produced an artifact tree" 0
        return
    fi
    # The shared runtime bundle (`assets/app.<hash>.js`) is ~100 KB of runtime
    # that has its own tests; snapshotting it would put 250 KB of goldens in the
    # tree and drown every review diff. Its hash still appears in each page's
    # golden, so a runtime change is never silent.
    if [ "$update" = 1 ]; then
        rm -rf "$golden"
        mkdir -p "$golden"
        ( cd "$dir/output" && tar -cf - --exclude='app.*.js' --exclude='*/app.*.js' . ) | ( cd "$golden" && tar -xf - )
        echo "updated $golden"
        return
    fi
    if [ ! -d "$golden" ]; then
        check "$fixture: golden exists (run with --update to create it)" 0
        return
    fi
    if diff -r -x 'app.*.js' "$golden" "$dir/output" > "/tmp/web_golden_$fixture.diff" 2>&1; then
        check "$fixture: artifact tree matches its golden" 1
    else
        check "$fixture: artifact tree matches its golden (see /tmp/web_golden_$fixture.diff)" 0
    fi
}

echo "== static: a page with no features ships no JavaScript"
if build_fixture static; then
    d="lang/tests/web/fixtures/static/output"
    js_bytes=$(find "$d" -name '*.js' -type f -exec cat {} + 2>/dev/null | wc -c | tr -d ' ')
    check "static: 0 bytes of JavaScript ($js_bytes)" "$([ "$js_bytes" = "0" ] && echo 1 || echo 0)"
    check "static: home page has no <script>" "$(hasnt "$d/index.html" '<script')"
    check "static: about page has no <script>" "$(hasnt "$d/about.html" '<script')"
    check "static: both pages render their own heading" \
          "$([ "$(has "$d/index.html" '<h1>Static home</h1>')" = 1 ] && [ "$(has "$d/about.html" '<h1>About</h1>')" = 1 ] && echo 1 || echo 0)"
    probe_strict "$d" "static"
fi
compare_golden static

echo
echo "== universal: five instances of three call sites get five ids"
if build_fixture universal; then
    d="lang/tests/web/fixtures/universal/output"
    ids=$(grep -o 'id="u[0-9]*"' "$d/index.html" 2>/dev/null | sort -u | wc -l | tr -d ' ')
    check "universal: 5 distinct per-instance hydration ids ($ids)" \
          "$([ "$ids" = "5" ] && echo 1 || echo 0)"
    check "universal: the dispatch names the same five ids" \
          "$([ "$(grep -c '\$__uni_c([1-5],' "$d/index.js" 2>/dev/null | tr -d ' ')" = "5" ] && echo 1 || echo 0)"
fi
compare_golden universal

echo
echo "== router: deep links, a fallback, and noindex on a miss"
if build_fixture router; then
    d="lang/tests/web/fixtures/router/output"
    check "router: the default route renders with its title" \
          "$([ "$(has "$d/home/index.html" '<title>Home</title>')" = 1 ] && [ "$(has "$d/home/index.html" 'class="view home"')" = 1 ] && echo 1 || echo 0)"
    check "router: selecting an id route renders it with its title" \
          "$([ "$(has "$d/about/index.html" '<title>About</title>')" = 1 ] && [ "$(has "$d/about/index.html" 'class="view about"')" = 1 ] && echo 1 || echo 0)"
    check "router: an unselected route's title is not emitted" \
          "$(hasnt "$d/about/index.html" '<title>Home</title>')"
    check "router: the selected wrapper is marked active server-side" \
          "$([ "$(has "$d/about/index.html" 'data-uni-route="main#about" data-uni-route-active="true"')" = 1 ] && [ "$(has "$d/about/index.html" 'data-uni-route="main#home" data-uni-route-active="false"')" = 1 ] && echo 1 || echo 0)"
    check "router: a route-missing page emits robots noindex" \
          "$([ "$(has "$d/missing/index.html" '<meta name="robots" content="noindex">')" = 1 ] && [ "$(hasnt "$d/home/index.html" 'noindex')" = 1 ] && echo 1 || echo 0)"
    # `import router` must stay possible: the library's links read
    # `window.$__uni_link_active`, which the hydration-safety pass rejects in
    # render position. Their active state is post-mount, so SSR has the href and
    # no `aria-current` — that is the contract this golden pins.
    check "router: <NavLink> SSRs its href without a render-time aria-current" \
          "$([ "$(has "$d/home/index.html" 'href="/about"')" = 1 ] && [ "$(hasnt "$d/home/index.html" 'aria-current')" = 1 ] && echo 1 || echo 0)"
    home_ref=$(grep -o 'src="[^"]*app\.[0-9a-f]*\.js"' "$d/home/index.html" | head -1 | sed 's/src="//;s/"$//')
    miss_ref=$(grep -o 'src="[^"]*app\.[0-9a-f]*\.js"' "$d/missing/index.html" | head -1 | sed 's/src="//;s/"$//')
    check "router: every page shares one hashed bundle ($home_ref)" \
          "$([ -n "$home_ref" ] && [ "$miss_ref" = "$home_ref" ] && [ "$(basename "$home_ref")" != "$home_ref" ] && echo 1 || echo 0)"

    # Unit graph: `NavLink` renders on the home route only, `RouterApp` on all
    # three, so the route-only definition must travel in that route's chunk and
    # the shared one must stay in the bundle.
    # The per-route chunk is `<name>.chunk.<hash>.js` (it was `<route>_index.<hash>.js`
    # before the hydration-runtime change renamed it; the gate follows the scheme).
    home_chunk_ref=$(grep -o 'src="[^"]*chunk\.[0-9a-f]*\.js"' "$d/home/index.html" | head -1 | sed 's/src="//;s/"$//')
    home_chunk="$d/assets/$(basename "$home_chunk_ref")"
    bundle_file="$d/assets/$(basename "$home_ref")"
    check "router: a route-only component ships in that route's chunk" \
          "$([ -n "$home_chunk_ref" ] && [ "$(has "$home_chunk" 'function router_NavLink(props)')" = 1 ] && [ "$(hasnt "$bundle_file" 'function router_NavLink(props)')" = 1 ] && echo 1 || echo 0)"
    # Shared definitions live in the vendor bundle (one file every page loads and
    # caches forever), never in a single route's chunk.
    vendor_file=$(ls "$d"/assets/app.vendor.*.js 2>/dev/null | head -1)
    check "router: a component two routes share stays in the vendor bundle" \
          "$([ -n "$vendor_file" ] && [ "$(has "$vendor_file" 'function router_fixture_RouterApp(props)')" = 1 ] && [ "$(hasnt "$home_chunk" 'function router_fixture_RouterApp(props)')" = 1 ] && echo 1 || echo 0)"
    check "router: the runtime bundle carries no component definition" \
          "$([ "$(hasnt "$bundle_file" 'function router_fixture_RouterApp(props)')" = 1 ] && [ "$(hasnt "$bundle_file" 'function router_NavLink(props)')" = 1 ] && echo 1 || echo 0)"
    check "router: every dispatched component is reachable from its page" \
          "$(graph_check "$d" >/tmp/web_golden_router_graph.txt && echo 1 || echo 0)"
    probe_strict "$d" "router"
fi
compare_golden router

echo
echo "== shell: one layout, two personalised renders"
if build_fixture shell; then
    d="lang/tests/web/fixtures/shell/output"
    check "shell: the chrome renders around the route body" \
          "$([ "$(has "$d/ada/index.html" 'class="shell"')" = 1 ] && [ "$(has "$d/ada/index.html" 'class="route dashboard"')" = 1 ] && echo 1 || echo 0)"
    check "shell: each render carries only its own fill" \
          "$([ "$(has "$d/ada/index.html" '>ada<')" = 1 ] && [ "$(hasnt "$d/ada/index.html" '>grace<')" = 1 ] && [ "$(has "$d/grace/index.html" '>grace<')" = 1 ] && [ "$(hasnt "$d/grace/index.html" '>ada<')" = 1 ] && echo 1 || echo 0)"
    ada_ref=$(grep -o 'src="[^"]*app\.[0-9a-f]*\.js"' "$d/ada/index.html" | head -1 | sed 's/src="//;s/"$//')
    grace_ref=$(grep -o 'src="[^"]*app\.[0-9a-f]*\.js"' "$d/grace/index.html" | head -1 | sed 's/src="//;s/"$//')
    check "shell: the two renders share one hashed bundle ($ada_ref)" \
          "$([ -n "$ada_ref" ] && [ "$grace_ref" = "$ada_ref" ] && [ "$(basename "$ada_ref")" != "$ada_ref" ] && echo 1 || echo 0)"
    # Both renders use both components, so nothing may move out of the vendor
    # bundle: one copy, shared, is the point of a chunk graph with a shared
    # baseline.
    shell_vendor=$(ls "$d"/assets/app.vendor.*.js 2>/dev/null | head -1)
    check "shell: components both routes use stay in the vendor bundle" \
          "$([ -n "$shell_vendor" ] && [ "$(has "$shell_vendor" 'function shell_fixture_AppShell(props)')" = 1 ] && [ "$(has "$shell_vendor" 'function shell_fixture_Dashboard(props)')" = 1 ] && echo 1 || echo 0)"
    check "shell: every dispatched component is reachable from its page" \
          "$(graph_check "$d" >/tmp/web_golden_shell_graph.txt && echo 1 || echo 0)"
    probe_strict "$d" "shell"
fi
compare_golden shell

echo
if [ "$fail" -ne 0 ]; then
    echo "web-golden-test: FAILED" >&2
    exit 1
fi
echo "web-golden-test: all fixtures match their goldens"
