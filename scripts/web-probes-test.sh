#!/usr/bin/env bash
# Web stack probe gate.
#
# Builds and runs the three probe apps in `lang/tests/web/probes/` — the shapes
# `lang/docs/web-stack-gaps.md` measures — and fails if any self-check prints
# `FAIL` or the expected measurement line is missing.
#
#   shop   storefront: frozen pages + per-request renders + shell, sessions, POST
#   blog   content site: 47 pre-rendered pages, one shared island, per-route chunks
#   spa    single-page app: `--web` split, shell page, JSON endpoint, client job
#   tasks  authenticated multi-user app: sign-in/sign-up, per-user state, PRG, flash
#   docs   documentation site: 43 pages, sectioned layout, prev/next, zero JS
#   bench  served runtime: in-process vs socket vs keep-alive, per delivery shape
#
#   ./scripts/web-probes-test.sh            # build + run all six
#   ./scripts/web-probes-test.sh shop       # only one
set -eu

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(dirname "$here")"
cd "$root"

compiler="${CHX_COMPILER:-cmake-build-debug/TCCCompiler.exe}"
[ -x "$compiler" ] || compiler="cmake-build-debug/TCCCompiler"
if [ ! -x "$compiler" ]; then
    echo "error: no TCCCompiler at cmake-build-debug/TCCCompiler (run ./scripts/build.sh --tcc)" >&2
    exit 1
fi

which_="${1:-all}"
out="lang/compiled/web_probes"
mkdir -p "$out"

fail=0
pass_note() { echo "ok   $1"; }
fail_note() { echo "FAIL $1" >&2; fail=1; }

# build_probe <name> <extra compiler args...>
build_probe() {
    local name="$1"; shift
    local exe="$out/$name.exe"
    rm -f "$exe"
    "$compiler" "lang/tests/web/probes/$name/chemical.mod" -o "$exe" \
        --mode debug_quick "$@" >"$out/$name.build.txt" 2>&1 || true
    if [ ! -x "$exe" ]; then
        fail_note "$name: did not compile (see $out/$name.build.txt)"
        tail -15 "$out/$name.build.txt" >&2
        return 1
    fi
    return 0
}

# run_probe <name> <log> — fails on any FAIL line, reports the ok count
run_probe() {
    local name="$1" log="$2"
    if ! "$out/$name.exe" >"$log" 2>&1; then
        fail_note "$name: exited non-zero"
        tail -20 "$log" >&2
        return 1
    fi
    local bad ok
    bad=$(grep -c '^FAIL' "$log" || true)
    ok=$(grep -c '^ok ' "$log" || true)
    if [ "$bad" != "0" ]; then
        fail_note "$name: $bad check(s) failed"
        grep '^FAIL' "$log" >&2
        return 1
    fi
    if [ "$ok" -lt 5 ]; then
        fail_note "$name: only $ok checks ran (expected the full self-check)"
        return 1
    fi
    pass_note "$name: $ok checks"
    return 0
}

want() { [ "$which_" = "all" ] || [ "$which_" = "$1" ]; }

# ---------------------------------------------------------------- shop
if want shop; then
    echo "== shop (storefront) =="
    rm -rf lang/tests/web/probes/shop/build
    if build_probe shop; then
        run_probe shop "$out/shop.log" || true
        # The site summary the gap analysis quotes must still be printed.
        grep -q '^site artifacts=' "$out/shop.log" \
            && pass_note "shop: site summary printed" \
            || fail_note "shop: no 'site artifacts=' line"
        # Per-request renders must be reported (the ~0.36 ms/render figure).
        grep -q 'render' "$out/shop.log" \
            && pass_note "shop: render timings printed" \
            || fail_note "shop: no render timings"
    fi
fi

# ---------------------------------------------------------------- blog
if want blog; then
    echo "== blog (content site) =="
    rm -rf lang/tests/web/probes/blog/build
    if build_probe blog; then
        # The blog asserts its served bytes as well as reporting counts: a page
        # rendered into freed memory used to keep its size and lose its content.
        run_probe blog "$out/blog.log" || true
        pages=$(grep -o 'pages=[0-9]*' "$out/blog.log" | head -1 | cut -d= -f2 || true)
        if [ -n "${pages:-}" ] && [ "$pages" -ge 40 ]; then
            pass_note "blog: $pages pages rendered"
        else
            fail_note "blog: page count missing or too low (${pages:-none})"
        fi
        if grep -q 'respond status=200' "$out/blog.log"; then
            pass_note "blog: a rendered page is served"
        else
            fail_note "blog: no served page in the log"
        fi
        grep -q '^ok   a post keeps its title' "$out/blog.log" \
            && pass_note "blog: a served page keeps its content" \
            || fail_note "blog: served page content is missing (corrupt render?)"
        if grep -q '^bundle=assets/blog\.' "$out/blog.log"; then
            pass_note "blog: shared bundle emitted"
        else
            fail_note "blog: no bundle line"
        fi
        if grep -q 'artifacts=[0-9]' "$out/blog.log"; then
            pass_note "blog: artifact count reported"
        else
            fail_note "blog: no artifact count"
        fi
    fi
fi

# ---------------------------------------------------------------- spa
if want spa; then
    echo "== spa (single-page app, --web) =="
    rm -rf lang/tests/web/probes/spa/build
    web_dir="$out/spa_web"
    rm -rf "$web_dir"
    if build_probe spa --web --build-dir "$web_dir"; then
        run_probe spa "$out/spa.log" || true
        bundle="$web_dir/web-client/assets/web-client.mjs"
        if [ -f "$bundle" ]; then
            pass_note "spa: client bundle $(wc -c <"$bundle") B"
        else
            fail_note "spa: no web-client/assets/web-client.mjs"
        fi
        grep -q 'register_fns' "$bundle" 2>/dev/null \
            && pass_note "spa: client bundle carries the dom bindings" \
            || fail_note "spa: client bundle lacks the dom bindings"
        # The bundle is one ES module per Chemical module: `fetch` lives in
        # the emitted `dom.mjs`, not in the application's `client.mjs`.
        grep -q 'rt.dom.fetch_text' "$web_dir/web-client/assets/dom.mjs" 2>/dev/null \
            && pass_note "spa: client bundle carries the fetch path" \
            || fail_note "spa: client bundle lacks the fetch path"
        # The export written by `finish()` holds the served bundle. Boot it in Node
        # against a stub DOM: the probe's own checks prove the bytes are wired, this
        # proves the wired bytes actually run and render.
        if command -v node >/dev/null 2>&1; then
            if node lang/tests/web/probes/spa/client_boot_check.mjs lang/compiled/spa_probe \
                    >"$out/spa_boot.log" 2>&1; then
                pass_note "spa: served bundle boots in node ($(grep -c '^ok  ' "$out/spa_boot.log") checks)"
            else
                fail_note "spa: served bundle failed to boot (see $out/spa_boot.log)"
            fi
        else
            echo "skip spa: node not installed, bundle not booted"
        fi
    fi
fi

# ---------------------------------------------------------------- tasks
if want tasks; then
    echo "== tasks (authenticated app) =="
    rm -rf lang/tests/web/probes/tasks/build
    if build_probe tasks; then
        run_probe tasks "$out/tasks.log" || true
        # An app with no component and no client module must ship no JS at all.
        grep -q '^site artifacts=.*bundle=$' "$out/tasks.log" \
            && pass_note "tasks: no JS bundle for a component-free app" \
            || fail_note "tasks: unexpected bundle (see $out/tasks.log)"
        grep -q '^bench renders=' "$out/tasks.log" \
            && pass_note "tasks: per-request render timing printed" \
            || fail_note "tasks: no render timing"
    fi
fi

# ---------------------------------------------------------------- docs
if want docs; then
    echo "== docs (documentation site) =="
    rm -rf lang/tests/web/probes/docs/build
    if build_probe docs; then
        run_probe docs "$out/docs.log" || true
        grep -q '^measured pages=43 ' "$out/docs.log" \
            && pass_note "docs: 43 pages measured" \
            || fail_note "docs: page count missing (see $out/docs.log)"
        grep -q '^bundle= gzip=1' "$out/docs.log" \
            && pass_note "docs: zero JS bundle, gzip on" \
            || fail_note "docs: expected an empty bundle"
    fi
fi

# ---------------------------------------------------------------- bench
if want bench; then
    echo "== bench (served runtime) =="
    rm -rf lang/tests/web/probes/bench/build
    if build_probe bench; then
        if "$out/bench.exe" >"$out/bench.log" 2>&1; then
            grep -a '^bench route=' "$out/bench.log" | sed 's/^/     /'
            # A kept-alive request that takes tens of milliseconds is Nagle holding the
            # response body for the peer's delayed ACK: it was ~40 ms before
            # `tcp_nodelay` was set on accepted sockets. 5 ms is loose enough for a
            # loaded CI box and far below the regression it guards.
            slow=$(grep -ao 'keepalive_p50_us=[0-9]*' "$out/bench.log" | cut -d= -f2 \
                | awk '$1 > 5000' | wc -l | tr -d ' ')
            if [ "$slow" = "0" ]; then
                pass_note "bench: keep-alive requests answer in microseconds"
            else
                fail_note "bench: keep-alive p50 over 5 ms (Nagle regression?)"
                grep -a '^bench route=' "$out/bench.log" >&2
            fi
            # The framework alone must stay in the low microseconds for a frozen page; a
            # render is allowed to cost more. 20 us is a load-tolerant ceiling (an idle
            # machine reads 2-4 us) and still ~2000x below what a Nagle regression costs.
            frozen=$(grep -a 'bench route=/ kind=frozen ' "$out/bench.log" \
                | grep -o 'inproc_p50_us=[0-9]*' | cut -d= -f2)
            if [ -n "$frozen" ] && [ "$frozen" -le 20 ]; then
                pass_note "bench: a frozen page answers in ${frozen} us (in-process p50)"
            else
                fail_note "bench: frozen page in-process p50 above 20 us (${frozen:-missing})"
            fi
            # Dynamic gzip: the rendered page must arrive compressed, decode back to
            # the same body, and be smaller — a broken negotiation is a corrupt page.
            grep -a '^bench gzip ' "$out/bench.log" | sed 's/^/     /'
            if grep -aq '^bench gzip .* ok$' "$out/bench.log"; then
                pass_note "bench: a rendered page compresses, decodes back, and shrinks"
            else
                fail_note "bench: dynamic gzip did not round-trip (see $out/bench.log)"
            fi
        else
            fail_note "bench: exited non-zero"
            tail -20 "$out/bench.log" >&2
        fi
    fi
fi

echo
if [ "$fail" = "0" ]; then
    echo "web probes: PASS"
else
    echo "web probes: FAIL" >&2
fi
exit "$fail"
