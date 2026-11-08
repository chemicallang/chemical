#!/usr/bin/env bash
# Phase 2 public-API audit. Reports ordinary public raw-pointer surfaces and
# public unsafe functions per Tier A library. Legacy raw-buffer APIs are listed
# so migrations are visible; a reviewed allowlist lives in
# lang/docs/safety-sharpen/API_ALLOWLIST.md.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LIBS=(std core fs path process environment net http tls encoding crypto compression archive image audio font regex json mime datetime uuid osrand cbor window webview page components router server)
printf '%-14s %8s %8s\n' LIB RAW_PUBLIC UNSAFE_PUBLIC
for lib in "${LIBS[@]}"; do
    dir="$ROOT/lang/libs/$lib"
    [ -d "$dir" ] || continue
    paths=("$dir/src")
    [ -d "$dir/posix" ] && paths+=("$dir/posix")
    [ -d "$dir/win" ] && paths+=("$dir/win")
    raw=$({ rg --no-heading '^[[:space:]]*public[[:space:]]+(unsafe[[:space:]]+)?func[^\n]*\*' "${paths[@]}" 2>/dev/null || true; } | wc -l)
    unsafe=$({ rg --no-heading '^[[:space:]]*public[[:space:]]+unsafe[[:space:]]+(func|struct|type|variant|interface)' "${paths[@]}" 2>/dev/null || true; } | wc -l)
    printf '%-14s %8s %8s\n' "$lib" "$raw" "$unsafe"
done
