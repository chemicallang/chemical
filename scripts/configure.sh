#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

NO_LLVM=false
RELEASE_BUILD=false
GENERATOR=""

while [ $# -gt 0 ]; do
  case "$1" in
    --no-llvm) NO_LLVM=true; shift ;;
    --release) RELEASE_BUILD=true; shift ;;
    --generator|-G)
      if [ -n "${2-}" ]; then
        GENERATOR="$2"; shift 2
      else
        echo "Error: --generator/-G requires a value" >&2; exit 1
      fi
      ;;
    --help|-h)
      echo "Usage: $0 [options]"
      echo ""
      echo "Options:"
      echo "  --no-llvm                 Disable LLVM, sets BUILD_COMPILER=OFF"
      echo "  --release                 Sets CMAKE_BUILD_TYPE to Release instead of Debug"
      echo "  --generator <name>, -G    CMake generator (Ninja, Unix Makefiles, etc.)"
      echo "  --help, -h                Show this help"
      echo ""
      echo "Environment variables:"
      echo "  CHEMICAL_MSVC_AUTO=0    Disable automatic MSVC environment setup on Windows"
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2; exit 1 ;;
  esac
done

# On Windows (Git Bash / MSYS2), ensure MSVC flags are not mangled by MSYS2
source "$SCRIPT_DIR/msvc_env.sh"

CMAKE_ARGS=(-S . -B cmake-build-debug)
if [ -n "$GENERATOR" ]; then
  CMAKE_ARGS+=(-G "$GENERATOR")
elif [ -n "${MSYSTEM-}" ]; then
  # Windows Git Bash / MSYS2
  # → use MinGW Makefiles (matches CLion default, works with both MSVC and MinGW GCC)
  CMAKE_ARGS+=(-G "MinGW Makefiles")
fi

# ── Detect a usable C++ compiler ─────────────────────────────────────────────
# On Windows with MSYS2 / Git Bash, cl.exe may be in PATH from a previous VS
# installation even when the MSVC environment (INCLUDE/LIB) is NOT properly
# set up.  This causes "fatal error C1083: Cannot open include file" failures.
#
# If CHEMICAL_MSVC_READY is not set (msvc_env.sh failed to validate INCLUDE
# paths) but cl.exe is present, we must force CMake to use MinGW GCC instead.
#
if [ -n "${MSYSTEM-}" ] && (command -v cl.exe &>/dev/null || command -v cl &>/dev/null); then
  if [ "${CHEMICAL_MSVC_READY:-0}" != "1" ]; then
    # MSVC binary is in PATH but environment is broken — fall back to MinGW
    if command -v g++.exe &>/dev/null; then
      echo "[configure] MSVC environment broken (INCLUDE/LIB missing). Falling back to MinGW GCC." >&2
      CMAKE_ARGS+=(-DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++)
    else
      echo "[configure] ERROR: cl.exe found but MSVC environment is broken (INCLUDE/LIB not set)" >&2
      echo "[configure]   and no MinGW GCC fallback is available." >&2
      echo "[configure] Install MinGW GCC (e.g. via 'pacman -S mingw-w64-x86_64-gcc' in MSYS2)" >&2
      echo "[configure] or run from a Visual Studio Developer Command Prompt." >&2
      exit 1
    fi
  fi
fi
if [ "$NO_LLVM" = true ]; then
  CMAKE_ARGS+=(-DBUILD_COMPILER=OFF)
else
  # Explicitly ON to override any cached value from a previous --no-llvm run
  CMAKE_ARGS+=(-DBUILD_COMPILER=ON)
fi
if [ "$RELEASE_BUILD" = true ]; then
  CMAKE_ARGS+=(-DCMAKE_BUILD_TYPE=Release)
else
  CMAKE_ARGS+=(-DCMAKE_BUILD_TYPE=Debug)
fi

cmake "${CMAKE_ARGS[@]}"
