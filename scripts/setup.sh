#!/usr/bin/env bash
# Copyright (c) Chemical Language Foundation 2025.
set -euo pipefail

# ---------------------------------------
# Parse args: support --with-llvm, pass rest to others
# ---------------------------------------
WITH_LLVM=false
FORWARD_ARGS=()

while [ $# -gt 0 ]; do
  case "$1" in
    --with-llvm) WITH_LLVM=true; shift ;;
    --help|-h)
       printf '%s\n' "Usage: $0 [--with-llvm] [other options passed to libtcc.sh/llvm.sh]"
       printf '%s\n' ""
       printf '%s\n' "Options:"
       printf '%s\n' "  --with-llvm       Download prebuilt LLVM in addition to libtcc"
       printf '%s\n' ""
       printf '%s\n' "Other options like --arch, --musl, --mingw, --ucrt will be passed to exactly"
       printf '%s\n' "the downloaded scripts (libtcc.sh or llvm.sh)."
       exit 0
       ;;
    *)
       FORWARD_ARGS+=("$1")
       shift
       ;;
  esac
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"

echo "==> Setting up libtcc..."
bash "${SCRIPT_DIR}/libtcc.sh" "${FORWARD_ARGS[@]}"

if [ "$WITH_LLVM" = true ]; then
    echo "==> Setting up prebuilt LLVM..."
    bash "${SCRIPT_DIR}/llvm.sh" "${FORWARD_ARGS[@]}"
fi

echo "Configuration complete."
