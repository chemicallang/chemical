#!/usr/bin/env bash
# Reproduce the P0.1 probes. Builds with both compilers where the probe makes
# sense, runs each, and writes raw logs to lang/tests/ui_probe/build/logs.
#
# Usage: lang/tests/ui_probe/run-probes.sh [output-dir]
#
# Safe: only writes under lang/tests/ui_probe/build (gitignored), /tmp, and the
# requested output dir. Does not modify sources or system packages.

set -u
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
OUT="${1:-$ROOT/lang/tests/ui_probe/build/logs}"
mkdir -p "$OUT"

TCC="$ROOT/cmake-build-debug/TCCCompiler"
LLVM="$ROOT/cmake-build-debug/Compiler"
fail=0
pass=0

say() { printf '\n== %s ==\n' "$*"; }
ok() { pass=$((pass + 1)); printf 'PASS %s\n' "$1"; }
bad() { fail=$((fail + 1)); printf 'FAIL %s\n' "$1"; }

build_run() { # build_run <name> <module> <compiler> <mode> [run]
    local name="$1" mod="$2" cc="$3" mode="$4" run="${5:-1}"
    local exe="$OUT/${name}_$(basename "$cc").exe"
    say "build/run $name with $(basename "$cc")"
    if ! timeout 180 "$cc" "$mod" -o "$exe" --mode "$mode" >"$OUT/${name}_$(basename "$cc").build.log" 2>&1; then
        bad "$name build $(basename "$cc")"
        return 1
    fi
    if [ "$run" = "1" ]; then
        if timeout 60 "$exe" >"$OUT/${name}_$(basename "$cc").run.log" 2>&1; then
            ok "$name run $(basename "$cc")"
        else
            bad "$name run $(basename "$cc")"
            return 1
        fi
    fi
    return 0
}

# ---- environment -----------------------------------------------------------
say "environment"
"$ROOT/scripts/ui-probe-env.sh" >"$OUT/env.log" 2>&1 && ok "ui-probe-env" || bad "ui-probe-env"

# ---- language capabilities -------------------------------------------------
for pair in "foundation:lang/tests/ui_probe/foundation/chemical.mod:debug_quick" \
            "embed:lang/tests/ui_probe/embed/chemical.mod:debug_quick" \
            "linkc:lang/tests/ui_probe/linkc/chemical.mod:debug_quick" \
            "image_bench:lang/tests/ui_probe/image_bench/chemical.mod:release_fast" \
            "dce_with:lang/tests/ui_probe/dce/with/chemical.mod:release_fast" \
            "dce_without:lang/tests/ui_probe/dce/without/chemical.mod:release_fast" \
            "baseline:lang/tests/ui_probe/baseline/chemical.mod:release_fast" \
            "window_probe:lang/tests/ui_probe/window_probe/chemical.mod:debug_quick" \
            "font_probe:lang/tests/ui_probe/font_probe/chemical.mod:release_fast"; do
    IFS=: read -r name rel mode <<< "$pair"
    build_run "$name" "$ROOT/$rel" "$TCC" "$mode" || true
    build_run "$name" "$ROOT/$rel" "$LLVM" "$mode" || true
done

# ---- negative: [no_panic] must reject a panicking call --------------------
say "negative no_panic"
if timeout 120 "$TCC" "$ROOT/lang/tests/ui_probe/negative/no_panic/chemical.mod" \
        -o "$OUT/neg_no_panic.exe" --mode debug_quick >"$OUT/neg_no_panic.log" 2>&1; then
    bad "no_panic should reject vector::push"
else
    if grep -q "SAFE-PANIC-001" "$OUT/neg_no_panic.log"; then
        ok "no_panic rejects unannotated std (SAFE-PANIC-001)"
    else
        bad "no_panic failed without SAFE-PANIC-001"
    fi
fi

# ---- allocation / OOM ------------------------------------------------------
say "allocation shim"
SHIM="$ROOT/lang/tests/ui_probe/build/alloc_shim.so"
mkdir -p "$ROOT/lang/tests/ui_probe/build"
if cc -shared -fPIC -O2 -o "$SHIM" "$ROOT/lang/tests/ui_probe/alloc/alloc_shim.c" -ldl >"$OUT/alloc_shim.build.log" 2>&1; then
    FOUND="$OUT/foundation_TCCCompiler.exe"
    if [ -x "$FOUND" ]; then
        UI_PROBE_COUNT=1 LD_PRELOAD="$SHIM" "$FOUND" >"$OUT/alloc_count.log" 2>&1 && ok "allocation counter" || bad "allocation counter"
        UI_PROBE_FAIL_AFTER=5 LD_PRELOAD="$SHIM" "$FOUND" >"$OUT/oom.log" 2>&1
        rc=$?
        if [ "$rc" -ne 0 ] && grep -q "panic:" "$OUT/oom.log" && grep -q "__chc_panic" "$OUT/oom.log"; then
            ok "OOM aborts through __chc_panic (rc=$rc)"
        else
            bad "OOM did not abort as expected (rc=$rc)"
        fi
    else
        bad "allocation shim needs foundation_TCCCompiler.exe"
    fi
else
    bad "allocation shim build"
fi

# ---- raster ----------------------------------------------------------------
say "raster prototype"
mkdir -p "$ROOT/lang/tests/ui_probe/raster/build"
if cc -O2 -std=c11 -o "$ROOT/lang/tests/ui_probe/raster/build/raster_probe" \
        "$ROOT/lang/tests/ui_probe/raster/raster_probe.c" >"$OUT/raster.build.log" 2>&1; then
    if taskset -c 2 "$ROOT/lang/tests/ui_probe/raster/build/raster_probe" 11 >"$OUT/raster.log" 2>&1; then
        ok "raster probe"
    else
        bad "raster probe run"
    fi
else
    bad "raster probe build"
fi

# ---- GPU -------------------------------------------------------------------
say "GPU Vulkan spike"
if python3 "$ROOT/lang/tests/ui_probe/gpu/vulkan_probe.py" \
        --icd /usr/share/vulkan/icd.d/lvp_icd.json >"$OUT/vulkan_lavapipe.json" 2>&1; then
    ok "Vulkan lavapipe device init"
else
    bad "Vulkan lavapipe device init"
fi
python3 "$ROOT/lang/tests/ui_probe/gpu/vulkan_probe.py" >"$OUT/vulkan_default.json" 2>&1 \
    && ok "Vulkan default adapter init" || bad "Vulkan default adapter init"

# ---- wasm ------------------------------------------------------------------
say "wasm float"
if "$ROOT/scripts/wasm-c-build.sh" "$ROOT/lang/tests/ui_probe/wasm_float/chemical.mod" \
        "$OUT/wasm_float_c.wasm" --export main >"$OUT/wasm_float_c.build.log" 2>&1; then
    if node "$ROOT/lang/tests/wasm/run.mjs" "$OUT/wasm_float_c.wasm" \
            --expect-rc 0 --expect-text "wasm_float PASS" >"$OUT/wasm_float_c.run.log" 2>&1; then
        ok "wasm float (C emitter)"
    else
        bad "wasm float (C emitter) run"
    fi
else
    bad "wasm float (C emitter) build"
fi
if "$LLVM" "$ROOT/lang/tests/ui_probe/wasm_float/chemical.mod" \
        --target wasm32-unknown-unknown -o "$OUT/wasm_float_llvm.wasm" --mode debug_quick \
        >"$OUT/wasm_float_llvm.build.log" 2>&1; then
    if node "$ROOT/lang/tests/wasm/run.mjs" "$OUT/wasm_float_llvm.wasm" \
            --expect-rc 0 --expect-text "wasm_float PASS" >"$OUT/wasm_float_llvm.run.log" 2>&1; then
        ok "wasm float (LLVM)"
    else
        bad "wasm float (LLVM) run"
    fi
else
    bad "wasm float (LLVM) build"
fi

# ---- sizes ----------------------------------------------------------------
say "stripped sizes"
for f in "$OUT"/dce_with_*.exe "$OUT"/dce_without_*.exe "$OUT"/baseline_*.exe "$OUT"/window_probe_*.exe; do
    [ -x "$f" ] || continue
    cp "$f" /tmp/ui_probe_size
    strip -s /tmp/ui_probe_size
    printf '%-46s %8s bytes\n' "$(basename "$f")" "$(stat -c%s /tmp/ui_probe_size)"
done | tee "$OUT/sizes.log"

echo
echo "P0.1 probes: PASS=$pass FAIL=$fail"
echo "logs: $OUT"
[ "$fail" -eq 0 ]
