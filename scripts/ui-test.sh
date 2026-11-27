#!/usr/bin/env bash
# Build and run the standalone UI test suite (P0.3a scaffold).
#
#   scripts/ui-test.sh --tcc --filter scaffold
#   scripts/ui-test.sh --llvm --sanitize=address,undefined
#   scripts/ui-test.sh --tcc --emit-c --cc-sanitize
#
# Exit codes: 0 pass, 1 test/build failure, 2 usage error.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BACKEND="tcc"
FILTER=""
NO_BUILD=0
SANITIZE=""
EMIT_C=0
CC_SANITIZE=0
CONTRACT_CHECK_ONLY=0
JSON=0
BUILD_DIR="$ROOT/lang/tests/ui/build"
MOD="$ROOT/lang/tests/ui/chemical.mod"
EXE=""

usage() {
    cat <<'EOF'
Usage: scripts/ui-test.sh [options]

  --tcc | --llvm           Backend (default: --tcc)
  --filter <tag|glob>      scaffold | core | design | geometry | error | arena | keys | golden | contract (default: scaffold)
  --no-build               Use an existing build artifact
  --sanitize=<list>        LLVM only: address,undefined (rejected on TCC)
  --emit-c                 TCC: emit C and compile it with the system cc
  --cc-sanitize            With --emit-c: add -fsanitize=address,undefined
  --update-goldens         Export CHEMICAL_UI_UPDATE_GOLDENS=1 for the run
  --contract-check-only    Only run the front-end contract check (no build/run)
  --json                   Print one JSON result object
  --help                   Show this help
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --tcc) BACKEND=tcc ;;
        --llvm) BACKEND=llvm ;;
        --filter) shift; [ $# -gt 0 ] || { echo "ui-test: --filter needs a value" >&2; exit 2; }; FILTER="$1" ;;
        --filter=*) FILTER="${1#*=}" ;;
        --no-build) NO_BUILD=1 ;;
        --sanitize=*) SANITIZE="${1#*=}" ;;
        --sanitize) shift; [ $# -gt 0 ] || { echo "ui-test: --sanitize needs a value" >&2; exit 2; }; SANITIZE="$1" ;;
        --emit-c) EMIT_C=1 ;;
        --cc-sanitize) CC_SANITIZE=1; EMIT_C=1 ;;
        --update-goldens) export CHEMICAL_UI_UPDATE_GOLDENS=1 ;;
        --contract-check-only) CONTRACT_CHECK_ONLY=1 ;;
        --json) JSON=1 ;;
        --help|-h) usage; exit 0 ;;
        *) echo "ui-test: unknown argument '$1'" >&2; usage >&2; exit 2 ;;
    esac
    shift
done

if [ "$BACKEND" = "tcc" ] && [ -n "$SANITIZE" ]; then
    echo "ui-test: --sanitize is LLVM-only; TCC has no sanitizer instrumentation" >&2
    exit 2
fi
if [ "$BACKEND" = "llvm" ] && [ "$EMIT_C" = "1" ]; then
    echo "ui-test: --emit-c/--cc-sanitize use the TCC C emitter; run with --tcc" >&2
    exit 2
fi
if [ "$BACKEND" = "llvm" ] && [ -n "$SANITIZE" ]; then
    if ! find "$ROOT/out/host/lib/clang" -name 'libclang_rt.asan*.a' -print -quit 2>/dev/null | grep -q .; then
        echo "ui-test: BLOCKED: LLVM sanitizer runtime is not installed under out/host/lib/clang (use --tcc --emit-c --cc-sanitize)" >&2
        exit 3
    fi
fi
case "$FILTER" in
    ""|scaffold|scaffold*|"*"|core|core*|design|design*|geometry|geometry*|error|error*|arena|arena*|keys|keys*|golden|golden*) FILTER="${FILTER:-scaffold}" ;;
    contract|contract*) FILTER=contract ;;
    *) echo "ui-test: no test matches filter '$FILTER' (P1.2 has scaffold|core|design|geometry|error|arena|keys|golden|contract)" >&2; exit 2 ;;
esac

mkdir -p "$BUILD_DIR"
BUILD_LOG="$BUILD_DIR/ui-test-build.log"
RUN_LOG="$BUILD_DIR/ui-test-run.log"
EXE="$BUILD_DIR/ui_tests_${BACKEND}.exe"

if [ "$FILTER" = "contract" ]; then
    CONTRACT_MOD="$ROOT/lang/tests/ui/contracts/chemical.mod"
    CONTRACT_EXE="$BUILD_DIR/ui_contracts_${BACKEND}.exe"
    CONTRACT_CHECK_BUILD="$BUILD_DIR/contract_check_${BACKEND}"
    CONTRACT_BUILD="$BUILD_DIR/contract_build_${BACKEND}"
    if [ "$BACKEND" = "tcc" ]; then
        CHECKER="$ROOT/cmake-build-debug/TCCCompiler"
    else
        CHECKER="$ROOT/cmake-build-debug/Compiler"
    fi
    rm -rf "$CONTRACT_CHECK_BUILD"
    if ! "$CHECKER" "$CONTRACT_MOD" --check --build-dir "$CONTRACT_CHECK_BUILD" \
            --mode debug_quick >"$BUILD_LOG" 2>&1; then
        cat "$BUILD_LOG" >&2
        echo "ui-test: contract check failed ($BACKEND)" >&2
        if [ "$JSON" = "1" ]; then
            printf '{"suite":"ui","backend":"%s","filter":"contract","status":"fail","exit_code":1}\n' "$BACKEND"
        fi
        exit 1
    fi
    if [ "$CONTRACT_CHECK_ONLY" = "1" ]; then
        echo "ui contract CHECK PASS"
        if [ "$JSON" = "1" ]; then
            printf '{"suite":"ui","backend":"%s","filter":"contract","mode":"check-only","status":"pass","exit_code":0}\n' "$BACKEND"
        fi
        exit 0
    fi
    if [ "$NO_BUILD" = "0" ]; then
        if [ "$BACKEND" = "tcc" ]; then
            rm -rf "$CONTRACT_BUILD"
            if ! "$CHECKER" "$CONTRACT_MOD" -o "$CONTRACT_EXE" --build-dir "$CONTRACT_BUILD" \
                    --mode debug_quick --no-cache >"$BUILD_LOG" 2>&1; then
                cat "$BUILD_LOG" >&2
                echo "ui-test: contract build failed ($BACKEND)" >&2
                if [ "$JSON" = "1" ]; then
                    printf '{"suite":"ui","backend":"%s","filter":"contract","status":"fail","exit_code":1}\n' "$BACKEND"
                fi
                exit 1
            fi
        else
            SAN_FLAG=()
            [ -n "$SANITIZE" ] && SAN_FLAG=("--sanitize=$SANITIZE")
            rm -rf "$CONTRACT_BUILD"
            if ! "$CHECKER" "$CONTRACT_MOD" -o "$CONTRACT_EXE" --build-dir "$CONTRACT_BUILD" \
                    --mode debug_quick --no-cache "${SAN_FLAG[@]}" >"$BUILD_LOG" 2>&1; then
                cat "$BUILD_LOG" >&2
                echo "ui-test: contract build failed ($BACKEND)" >&2
                if [ "$JSON" = "1" ]; then
                    printf '{"suite":"ui","backend":"%s","filter":"contract","status":"fail","exit_code":1}\n' "$BACKEND"
                fi
                exit 1
            fi
        fi
    fi
    if [ ! -x "$CONTRACT_EXE" ]; then
        echo "ui-test: missing contract executable '$CONTRACT_EXE' (omit --no-build)" >&2
        exit 1
    fi
    set +e
    "$CONTRACT_EXE" >"$RUN_LOG" 2>&1
    rc=$?
    set -e
    if [ "$JSON" = "1" ]; then
        cat "$RUN_LOG" >&2
    else
        cat "$RUN_LOG"
    fi
    if [ "$rc" -ne 0 ] || ! grep -q "ui contract PASS" "$RUN_LOG"; then
        if [ "$JSON" = "1" ]; then
            printf '{"suite":"ui","backend":"%s","filter":"contract","status":"fail","exit_code":%d}\n' "$BACKEND" "$rc"
        else
            echo "ui-test: contract FAIL (exit $rc)" >&2
        fi
        exit 1
    fi
    if [ "$JSON" = "1" ]; then
        printf '{"suite":"ui","backend":"%s","filter":"contract","status":"pass","tests":1,"exit_code":0}\n' "$BACKEND"
    else
        echo "ui-test: PASS ($BACKEND, filter=contract)"
    fi
    exit 0
fi

if [ "$NO_BUILD" = "0" ]; then
    rm -f "$EXE"
    if [ "$EMIT_C" = "1" ]; then
        EMIT_DIR="$BUILD_DIR/emit-c"
        C_FILE="$EMIT_DIR/Translated.c"
        mkdir -p "$EMIT_DIR"
        if ! "$ROOT/cmake-build-debug/TCCCompiler" "$MOD" --emit-c \
                -o "$EMIT_DIR/ui_emitc.exe" --build-dir "$EMIT_DIR" \
                --mode debug_quick >"$BUILD_LOG" 2>&1; then
            cat "$BUILD_LOG" >&2
            echo "ui-test: C emission failed" >&2
            exit 1
        fi
        if [ ! -s "$C_FILE" ]; then
            echo "ui-test: no translated C at $C_FILE" >&2
            exit 1
        fi
        CC_FLAGS=(-std=c11 -O0 -g -o "$EXE" "$C_FILE" -lm -pthread)
        if [ "$CC_SANITIZE" = "1" ]; then
            CC_FLAGS=(-fsanitize=address,undefined -fno-omit-frame-pointer "${CC_FLAGS[@]}")
        fi
        if ! cc "${CC_FLAGS[@]}" >"$BUILD_LOG" 2>&1; then
            cat "$BUILD_LOG" >&2
            echo "ui-test: system cc failed" >&2
            exit 1
        fi
    elif [ "$BACKEND" = "tcc" ]; then
        if ! "$ROOT/cmake-build-debug/TCCCompiler" "$MOD" -o "$EXE" \
                --mode debug_quick --no-cache >"$BUILD_LOG" 2>&1; then
            cat "$BUILD_LOG" >&2
            echo "ui-test: TCCCompiler build failed" >&2
            exit 1
        fi
    else
        SAN_FLAG=()
        [ -n "$SANITIZE" ] && SAN_FLAG=("--sanitize=$SANITIZE")
        if ! "$ROOT/cmake-build-debug/Compiler" "$MOD" -o "$EXE" \
                --mode debug_quick --no-cache "${SAN_FLAG[@]}" >"$BUILD_LOG" 2>&1; then
            cat "$BUILD_LOG" >&2
            echo "ui-test: Compiler build failed" >&2
            exit 1
        fi
    fi
fi

if [ ! -x "$EXE" ]; then
    echo "ui-test: missing executable '$EXE' (omit --no-build)" >&2
    exit 1
fi

set +e
"$EXE" --filter "$FILTER" >"$RUN_LOG" 2>&1
rc=$?
set -e
if [ "$JSON" = "1" ]; then
    cat "$RUN_LOG" >&2
else
    cat "$RUN_LOG"
fi
EXPECT="ui scaffold PASS"
if [ "$FILTER" = "core" ]; then EXPECT="ui core PASS"; fi
if [ "$FILTER" = "design" ]; then EXPECT="ui design PASS"; fi
if [ "$FILTER" = "geometry" ]; then EXPECT="ui geometry PASS"; fi
if [ "$FILTER" = "error" ]; then EXPECT="ui error PASS"; fi
if [ "$FILTER" = "arena" ]; then EXPECT="ui arena PASS"; fi
if [ "$FILTER" = "keys" ]; then EXPECT="ui keys PASS"; fi
if [ "$FILTER" = "golden" ]; then EXPECT="ui golden PASS"; fi
if [ "$rc" -ne 0 ] || ! grep -q "$EXPECT" "$RUN_LOG"; then
    if [ "$JSON" = "1" ]; then
        printf '{"suite":"ui","backend":"%s","filter":"%s","status":"fail","exit_code":%d}\n' "$BACKEND" "$FILTER" "$rc"
    else
        echo "ui-test: FAIL (exit $rc)" >&2
    fi
    exit 1
fi
if [ "$JSON" = "1" ]; then
    printf '{"suite":"ui","backend":"%s","filter":"%s","status":"pass","tests":1,"exit_code":0}\n' "$BACKEND" "$FILTER"
else
    echo "ui-test: PASS ($BACKEND, filter=$FILTER)"
fi
