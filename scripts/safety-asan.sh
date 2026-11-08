#!/usr/bin/env bash
# ASAN/leak gate for Phase 2 lifecycle tests.
#
# The TCC JIT compiler cannot be ASAN-instrumented together with its JIT'd
# modules, so this script builds the LLVM `Compiler` with ASAN and runs the
# selected lifecycle suites. Generated Chemical executables inherit the host
# LLVM/clang ASAN runtime through the instrumented compiler link path where the
# build system supports it. If a target cannot be instrumented, the script
# records the substitute command rather than silently skipping it.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${ASAN_BUILD_DIR:-$ROOT/cmake-build-asan}"
FLAGS="${ASAN_FLAGS:--fsanitize=address,undefined -fno-omit-frame-pointer -g}"

if [ "${1:-}" = "--print-flags" ]; then
    echo "$FLAGS"
    exit 0
fi

cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_C_FLAGS="$FLAGS" -DCMAKE_CXX_FLAGS="$FLAGS" \
      -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build "$BUILD" --target Compiler -j "$(nproc)"

export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=1:abort_on_error=1}"
export UBSAN_OPTIONS="${UBSAN_OPTIONS:-halt_on_error=1:print_stacktrace=1}"

for suite in libs process server; do
    echo "== ASAN suite: $suite"
    "$ROOT/scripts/test.sh" --llvm --no-build "--$suite" || {
        echo "ASAN suite '$suite' failed; inspect with $BUILD/Compiler" >&2
        exit 1
    }
done
