#!/usr/bin/env bash
# wasm32 target gate for the LLVM backend and the C-emitter backend.
#
#   scripts/wasm-test.sh [--skip-playwright] [--llvm PATH] [--tcc PATH]
#
# W0: trivial freestanding app -> Node returns 42.
# W1: std string/vector/Option/Result/map/format app -> Node output.
# W2: the same app through TCCCompiler --emit-c -> clang -> wasm-ld -> Node,
#     with identical observable output.
# W3: browser DOM bridge demo -> Node DOM shim + Chromium (Playwright).
#
# This suite is not part of --tcc/--llvm; run it explicitly on a machine with
# clang/wasm-ld (the prebuilt out/host/bin/wasm-ld is enough).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

LLVM="${LLVM_COMPILER:-$ROOT/cmake-build-debug/Compiler}"
TCC="${TCC_COMPILER:-$ROOT/cmake-build-debug/TCCCompiler}"
NODE="${NODE:-node}"
BUILD="$ROOT/lang/tests/build/wasm"
SKIP_PLAYWRIGHT=false

while [ $# -gt 0 ]; do
    case "$1" in
        --skip-playwright) SKIP_PLAYWRIGHT=true; shift ;;
        --llvm) LLVM="$2"; shift 2 ;;
        --tcc) TCC="$2"; shift 2 ;;
        -h|--help) sed -n '2,14p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) echo "wasm-test: unknown argument '$1'" >&2; exit 1 ;;
    esac
done

if [ ! -x "$LLVM" ]; then
    echo "wasm-test: Compiler not found at '$LLVM' (build with ./scripts/build.sh --llvm)" >&2
    exit 1
fi
if [ ! -x "$TCC" ]; then
    echo "wasm-test: TCCCompiler not found at '$TCC' (build with ./scripts/build.sh --tcc)" >&2
    exit 1
fi

rm -rf "$BUILD"
mkdir -p "$BUILD"/{hello,std,std-c,ui}

build_llvm() {
    local mod="$1" out="$2"
    shift 2
    "$LLVM" "$mod" --target wasm32-unknown-unknown -o "$out" --mode debug_quick "$@"
}

PASS=0
FAIL=0
FAILED_NAMES=()

record() {
    local name="$1" status="$2"
    if [ "$status" = 0 ]; then
        echo "PASS  $name"
        PASS=$((PASS + 1))
    else
        echo "FAIL  $name"
        FAIL=$((FAIL + 1))
        FAILED_NAMES+=("$name")
    fi
}

echo "==> W0: freestanding hello (LLVM)"
if build_llvm lang/tests/wasm/hello/chemical.mod "$BUILD/hello/app.wasm" >/dev/null; then
    if "$NODE" lang/tests/wasm/run.mjs "$BUILD/hello/app.wasm" --expect-rc 42; then
        record "w0-hello" 0
    else
        record "w0-hello" 1
    fi
else
    record "w0-hello" 1
fi

echo "==> W1: std core (LLVM)"
if build_llvm lang/tests/wasm/std_smoke/chemical.mod "$BUILD/std/app.wasm" >/dev/null; then
    if "$NODE" lang/tests/wasm/run.mjs "$BUILD/std/app.wasm" \
        --expect-text "wasm std: s=chem!!!!! len=9 vector=10 map=20 option=30 result=30"; then
        record "w1-std-llvm" 0
    else
        record "w1-std-llvm" 1
    fi
else
    record "w1-std-llvm" 1
fi

echo "==> W2: std core through the C emitter"
if scripts/wasm-c-build.sh lang/tests/wasm/std_smoke/chemical.mod "$BUILD/std-c/app.wasm" >/dev/null; then
    if "$NODE" lang/tests/wasm/run.mjs "$BUILD/std-c/app.wasm" \
        --expect-text "wasm std: s=chem!!!!! len=9 vector=10 map=20 option=30 result=30"; then
        record "w2-std-c" 0
    else
        record "w2-std-c" 1
    fi
else
    record "w2-std-c" 1
fi

echo "==> W3: browser UI demo (LLVM)"
UI=lang/compiled/wasm_ui
if build_llvm "$UI/chemical.mod" "$UI/app.wasm" \
    --wasm-export start --wasm-export chc_wasm_on_event >/dev/null; then
    if "$NODE" "$UI/node_test.mjs"; then
        record "w3-ui-node" 0
    else
        record "w3-ui-node" 1
    fi

    if [ "$SKIP_PLAYWRIGHT" = true ]; then
        echo "SKIP  w3-ui-playwright (--skip-playwright)"
    else
        cp "$ROOT/lang/runtime/wasm/host.mjs" "$UI/host.mjs"
        if [ -d "$ROOT/lang/compiled/components-e2e/node_modules" ]; then
            ln -sfn ../components-e2e/node_modules "$UI/node_modules"
            if (cd "$UI" && ./node_modules/.bin/playwright test --config playwright.config.ts); then
                record "w3-ui-playwright" 0
            else
                record "w3-ui-playwright" 1
            fi
        else
            echo "SKIP  w3-ui-playwright (Playwright not installed under lang/compiled/components-e2e/node_modules)"
        fi
    fi
else
    record "w3-ui-node" 1
fi

echo
echo "wasm suite: $PASS passed, $FAIL failed"
if [ "$FAIL" -ne 0 ]; then
    printf 'failed: %s\n' "${FAILED_NAMES[*]}"
    exit 1
fi
