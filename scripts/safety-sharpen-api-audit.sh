#!/usr/bin/env bash
# Phase 2 public-API audit. Reports ordinary public raw-pointer surfaces and
# public unsafe functions per Tier A library. Legacy raw-buffer APIs are listed
# so migrations are visible; a reviewed allowlist lives in
# lang/docs/safety-sharpen/API_ALLOWLIST.md.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LIBS=(std core fs path process environment net http tls encoding crypto compression archive image audio font regex json mime datetime uuid osrand cbor window webview page components router server)
printf '%-14s %8s %12s %8s %10s\n' LIB RAW_PUBLIC RAW_NONRAW UNSAFE_PUBLIC SAFE_NAMED
total_safe_named=0
nonzero_nonraw=""
for lib in "${LIBS[@]}"; do
    dir="$ROOT/lang/libs/$lib"
    [ -d "$dir" ] || continue
    paths=()
    [ -d "$dir/src" ] && paths+=("$dir/src")
    [ -d "$dir/posix" ] && paths+=("$dir/posix")
    [ -d "$dir/win" ] && paths+=("$dir/win")
    [ ${#paths[@]} -gt 0 ] || continue
    # RAW_NONRAW counts public raw-pointer functions declared outside a
    # `raw`/`ffi`/`sys` namespace. That number is the safe-surface gate; the
    # RAW_PUBLIC total also counts reviewed `::raw` migration surface.
    counts="$(find "${paths[@]}" -name '*.ch' -type f -print0 2>/dev/null |
      xargs -0 -r awk '
        BEGIN { sq = sprintf("%c", 39); dq = sprintf("%c", 34); bs = sprintf("%c", 92) }
        FNR == 1 { depth = 0; rawdepth = -1; incomment = 0 }
        {
          code = ""
          instring = 0
          n = length($0)
          for (i = 1; i <= n; i++) {
            c = substr($0, i, 1)
            if (incomment) {
              if (c == "*" && substr($0, i + 1, 1) == "/") { incomment = 0; i++ }
              continue
            }
            if (instring) {
              if (c == bs) { i++; continue }
              if (c == dq) { instring = 0 }
              continue
            }
            if (c == "/" && substr($0, i + 1, 1) == "/") { break }
            if (c == "/" && substr($0, i + 1, 1) == "*") { incomment = 1; i++; continue }
            if (c == dq) { instring = 1; continue }
            if (c == sq) {
              i++
              while (i <= n) {
                cc = substr($0, i, 1)
                if (cc == bs) { i += 2; continue }
                if (cc == sq) { break }
                i++
              }
              continue
            }
            code = code c
          }
          if (code ~ /^[ \t]*(public[ \t]+)?namespace[ \t]+(raw|ffi|sys)[ \t]*\{/) { rawdepth = depth + 1 }
          israw = (rawdepth >= 0 && depth >= rawdepth)
          if (code ~ /^[ \t]*public[ \t]+(unsafe[ \t]+)?func/ && code ~ /\*/) {
            raw++
            if (israw) inside++; else outside++
          }
          if (code ~ /^[ \t]*public[ \t]+unsafe[ \t]+(func|struct|type|variant|interface)/) unsafe++
          if (code ~ /^[ \t]*public[ \t]+(unsafe[ \t]+)?func[ \t]+/ &&
              (code ~ /func[ \t]+[A-Za-z0-9_]*_safe[ \t]*\(/ || code ~ /func[ \t]+safe_[A-Za-z0-9_]*[ \t]*\(/)) safenamed++
          o = gsub(/\{/, "{", code); cl = gsub(/\}/, "}", code)
          depth += o - cl
          if (rawdepth >= 0 && depth < rawdepth) rawdepth = -1
        }
        END { printf "%d %d %d %d\n", raw+0, outside+0, unsafe+0, safenamed+0 }
      ')"
    printf '%-14s %8s %12s %8s %10s\n' "$lib" $counts
    set -- $counts
    total_safe_named=$((total_safe_named + $4))
    if [ "$2" -gt 0 ]; then
        nonzero_nonraw="$nonzero_nonraw $lib($2)"
    fi
done

echo
echo "RAW_NONRAW is the safe-surface gate (reviewed allowlist: API_ALLOWLIST.md)."
if [ -n "$nonzero_nonraw" ]; then
    echo "reviewed non-raw raw-pointer surface:$nonzero_nonraw"
fi
# Safe-by-default convention: the plain name is safe; no `*_safe`/`safe_*` API.
if [ "$total_safe_named" -ne 0 ]; then
    echo "error: $total_safe_named public API name(s) still carry a 'safe' qualifier;" >&2
    echo "       rename the API so the plain name is the safe one (see AGENTS.md)." >&2
    exit 1
fi
echo "SAFE_NAMED total: 0 (safe is the default name)"
