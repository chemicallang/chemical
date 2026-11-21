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
#
#   ./scripts/web-probes-test.sh            # build + run all three
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
        # The blog probe reports rather than asserting, so its page/artifact
        # counts are the checks here.
        "$out/blog.exe" >"$out/blog.log" 2>&1 || true
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
        grep -q 'fetch_text' "$bundle" 2>/dev/null \
            && pass_note "spa: client bundle carries the fetch path" \
            || fail_note "spa: client bundle lacks the fetch path"
    fi
fi

echo
if [ "$fail" = "0" ]; then
    echo "web probes: PASS"
else
    echo "web probes: FAIL" >&2
fi
exit "$fail"
