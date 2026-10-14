#!/usr/bin/env bash
# mir-debug-info-check.sh
#
# Debug-info smoke check for the MIR -> LLVM path (checkpoint 8.10 of
# lang/docs/mir-llvm-migration.md, §5.1 "Debug information must be preserved").
#
# The policy is: "a change that drops debug info is a regression even if tests
# pass". Run this after any change to MIR lowering, MIR types/layout, or the
# MIR -> LLVM emitter.
#
# A `--mode debug_complete` build must carry DWARF:
#   * in the IR        : DICompileUnit, DISubprogram, DILocation, DILexicalBlock,
#                        DIBasicType, DICompositeType, and per-local
#                        dbg.declare records (`llvm.dbg.declare` intrinsics —
#                        LLVM >= 19 prints them as `#dbg_declare` debug records)
#   * in the executable: .debug_info / .debug_line sections
# A `--mode debug_quick` build must carry none of it, and both builds must
# produce the same program output.
#
# Usage:
#   ./scripts/mir-debug-info-check.sh                 # build the compiler, then check
#   ./scripts/mir-debug-info-check.sh --no-build      # use the existing binary
#   ./scripts/mir-debug-info-check.sh -j 8            # parallel build jobs
#   ./scripts/mir-debug-info-check.sh --keep          # keep the temp module + build dirs
#
# Exit status: 0 when every check passes, 1 otherwise.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$ROOT"

COMPILER_BIN="cmake-build-debug/Compiler"
BUILD_DIR="cmake-build-debug"
BUILD=true
JOBS="$(nproc 2>/dev/null || echo 4)"
KEEP=false
WORK="lang/tests/build/mir-debug-info"

usage() {
  sed -n '2,26p' "$0" | sed 's/^# \{0,1\}//'
  exit 1
}

while [ $# -gt 0 ]; do
  case "$1" in
    --no-build) BUILD=false; shift ;;
    -j) JOBS="$2"; shift 2 ;;
    --keep) KEEP=true; shift ;;
    -h|--help) usage ;;
    *) echo "unknown argument: $1" >&2; usage ;;
  esac
done

FAILED=0
pass() { printf '  ok    %s\n' "$1"; }
fail() { printf '  FAIL  %s\n' "$1"; FAILED=1; }

# count_ir PATTERN FILE -> number of matches (never fails the script)
count_ir() { grep -c -- "$1" "$2" 2>/dev/null || true; }

if [ ! -x "$COMPILER_BIN" ]; then
  echo "error: $COMPILER_BIN not found; build it first (./scripts/build.sh --llvm)" >&2
  exit 1
fi

if [ "$BUILD" = true ]; then
  echo "== building Compiler =="
  cmake --build "$BUILD_DIR" --target Compiler -j "$JOBS" >/dev/null || {
    echo "error: build failed" >&2; exit 1; }
fi

rm -rf "$WORK"
mkdir -p "$WORK/src"
mkdir -p "$WORK/dc" "$WORK/dq"

# A self-contained fixture: a struct-returning method, a loop with a local, and
# a nested block, so lexical scopes and local variables both have to be emitted.
cat > "$WORK/chemical.mod" <<'MOD'
application mir_debug_info_check
source "src"
import cstd
import std
MOD

cat > "$WORK/src/main.ch" <<'CH'
struct Point {
    var x : int
    var y : int

    func dist2(self) : int {
        return self.x * self.x + self.y * self.y
    }
}

func classify(v : int) : int {
    var out = 0
    if(v > 10) {
        var high = v * 2
        out = high
    } else {
        out = v
    }
    return out
}

public func main() : int {
    var p = Point { x: 3, y: 4 }
    var total = 0
    for(var i = 0; i < 10; i = i + 1) {
        total = total + classify(p.dist2())
    }
    printf("total=%d\n", total)
    return 0
}
CH

echo "== debug_complete =="
"$COMPILER_BIN" "$WORK/chemical.mod" -o "$WORK/dc/app.exe" \
    --mode debug_complete --assertions --out-ll-all --build-dir "$WORK/dc" --no-cache \
    >"$WORK/dc.log" 2>&1
DC_STATUS=$?
if [ "$DC_STATUS" -eq 0 ]; then
  pass "debug_complete build succeeds"
else
  fail "debug_complete build failed (see $WORK/dc.log)"
  tail -20 "$WORK/dc.log"
fi

DC_IR="$WORK/dc.ll"
cat "$WORK"/dc/modules/*/llvm_ir.ll > "$DC_IR" 2>/dev/null || : > "$DC_IR"
if [ ! -s "$DC_IR" ]; then
  fail "debug_complete emitted no llvm_ir.ll"
else
  for pat in DICompileUnit DISubprogram DILocation DILexicalBlock DIBasicType DICompositeType; do
    n="$(count_ir "!$pat" "$DC_IR")"
    if [ "${n:-0}" -gt 0 ]; then pass "IR has $pat ($n)"; else fail "IR is missing $pat"; fi
  done
  # `dbg.declare` is printed as a debug record (`#dbg_declare`) by LLVM >= 19 and
  # as an `@llvm.dbg.declare` call by older versions; accept either spelling.
  n="$(count_ir '#dbg_declare' "$DC_IR")"
  if [ "${n:-0}" -gt 0 ]; then
    pass "IR has dbg.declare records ($n)"
  else
    n="$(count_ir 'llvm.dbg.declare' "$DC_IR")"
    if [ "${n:-0}" -gt 0 ]; then pass "IR has dbg.declare calls ($n)"
    else fail "IR has no dbg.declare (locals carry no debug info)"; fi
  fi
  n="$(count_ir 'DILocalVariable' "$DC_IR")"
  if [ "${n:-0}" -gt 0 ]; then pass "IR has DILocalVariable ($n)"; else fail "IR is missing DILocalVariable"; fi
fi

if [ -f "$WORK/dc/app.exe" ]; then
  SECTIONS="$(readelf -S "$WORK/dc/app.exe" 2>/dev/null || true)"
  case "$SECTIONS" in
    *".debug_info"*) pass "executable has .debug_info" ;;
    *) fail "executable has no .debug_info section" ;;
  esac
  case "$SECTIONS" in
    *".debug_line"*) pass "executable has .debug_line" ;;
    *) fail "executable has no .debug_line section" ;;
  esac
  readelf --debug-dump=info "$WORK/dc/app.exe" >"$WORK/dc.dwarf" 2>/dev/null || : > "$WORK/dc.dwarf"
  n="$(count_ir 'DW_TAG_variable' "$WORK/dc.dwarf")"
  if [ "${n:-0}" -gt 0 ]; then pass "DWARF has DW_TAG_variable ($n)"; else fail "DWARF has no DW_TAG_variable"; fi
  n="$(count_ir 'DW_TAG_lexical_block' "$WORK/dc.dwarf")"
  if [ "${n:-0}" -gt 0 ]; then pass "DWARF has DW_TAG_lexical_block ($n)"; else fail "DWARF has no DW_TAG_lexical_block"; fi
  "$WORK/dc/app.exe" > "$WORK/dc.stdout" 2>&1
  if [ $? -eq 0 ]; then pass "debug_complete executable runs"; else fail "debug_complete executable exited non-zero"; fi
else
  fail "debug_complete produced no executable"
fi

echo "== debug_quick =="
"$COMPILER_BIN" "$WORK/chemical.mod" -o "$WORK/dq/app.exe" \
    --mode debug_quick --assertions --out-ll-all --build-dir "$WORK/dq" --no-cache \
    >"$WORK/dq.log" 2>&1
DQ_STATUS=$?
if [ "$DQ_STATUS" -eq 0 ]; then
  pass "debug_quick build succeeds"
else
  fail "debug_quick build failed (see $WORK/dq.log)"
  tail -20 "$WORK/dq.log"
fi

DQ_IR="$WORK/dq.ll"
cat "$WORK"/dq/modules/*/llvm_ir.ll > "$DQ_IR" 2>/dev/null || : > "$DQ_IR"
if [ ! -s "$DQ_IR" ]; then
  fail "debug_quick emitted no llvm_ir.ll"
else
  for pat in DICompileUnit DISubprogram DILocation DILexicalBlock '#dbg_' DILocalVariable; do
    n="$(count_ir "$pat" "$DQ_IR")"
    if [ "${n:-0}" -eq 0 ]; then pass "release IR has no $pat"; else fail "release IR must not contain $pat ($n)"; fi
  done
fi

if [ -f "$WORK/dq/app.exe" ]; then
  SECTIONS="$(readelf -S "$WORK/dq/app.exe" 2>/dev/null || true)"
  case "$SECTIONS" in
    *".debug_info"*) fail "release executable must not have .debug_info" ;;
    *) pass "release executable has no .debug_info" ;;
  esac
  "$WORK/dq/app.exe" > "$WORK/dq.stdout" 2>&1
  if [ $? -eq 0 ]; then pass "debug_quick executable runs"; else fail "debug_quick executable exited non-zero"; fi
fi

if [ -f "$WORK/dc.stdout" ] && [ -f "$WORK/dq.stdout" ] && diff -q "$WORK/dc.stdout" "$WORK/dq.stdout" >/dev/null; then
  pass "debug and release builds print identical output"
else
  fail "debug and release builds print different output"
fi

if [ "$KEEP" = false ]; then rm -f "$DC_IR" "$DQ_IR"; fi

echo
if [ "$FAILED" -eq 0 ]; then
  echo "mir debug-info check: PASS"
else
  echo "mir debug-info check: FAIL (artifacts left in $WORK)"
fi
exit "$FAILED"
