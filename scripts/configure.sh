#!/usr/bin/env bash
# Copyright (c) Chemical Language Foundation 2025.
set -euo pipefail

REPO_URL="https://github.com/chemicallang/tcclib.git"
TARGET_DIR="lib/tcc"

UNAME_S="$(uname -s 2>/dev/null || echo Unknown)"
UNAME_M="$(uname -m 2>/dev/null || echo unknown)"

# normalize to lowercase
UNAME_S_L="$(printf '%s' "$UNAME_S" | tr '[:upper:]' '[:lower:]')"
UNAME_M_L="$(printf '%s' "$UNAME_M" | tr '[:upper:]' '[:lower:]')"


# ---------------------------------------
# Parse args: support --arch <value>, --musl <true|false>
# ---------------------------------------
ARG_ARCH=""
ARG_MUSL=""

while [ $# -gt 0 ]; do
  case "$1" in
    --arch) ARG_ARCH="${2:-}"; shift 2 ;;
    --arch=*) ARG_ARCH="${1#*=}"; shift ;;
    --musl) ARG_MUSL="${2:-}"; shift 2 ;;
    --musl=*) ARG_MUSL="${1#*=}"; shift ;;
    --help|-h) printf '%s\n' "Usage: $0 [--arch <arch>] [--musl <true|false>]" ; exit 0 ;;
    *) # stop parsing unknown args
       break ;;
  esac
done

ARG_ARCH_L="$(printf '%s' "$ARG_ARCH" | tr '[:upper:]' '[:lower:]')"
ARG_MUSL_L="$(printf '%s' "$ARG_MUSL" | tr '[:upper:]' '[:lower:]')"

# ---------- Determine ARCH ----------
ARCH="unknown"

# 1) If user supplied --arch, use it (map common synonyms to canonical)
if [ -n "${ARG_ARCH_L}" ]; then
  case "${ARG_ARCH_L}" in
    x64|x86_64|x86-64|amd64) ARCH=amd64 ;;
    arm64|aarch64)          ARCH=arm64 ;;
    arm|armv7|armv7l|armv6l) ARCH=armv7 ;; # adjust if you want plain 'arm' to mean arm64
    x86|i386|i486|i586|i686) ARCH=i386 ;;
    riscv64)                ARCH=riscv64 ;;
    *)
      printf '%s\n' "Unrecognized --arch value: ${ARG_ARCH}. Supported: x64, amd64, x86_64, arm64, aarch64, arm, armv7, x86, i386, riscv64" >&2
      exit 1
      ;;
  esac
fi

# 2) If not provided, fall back to uname -m mapping
if [ "$ARCH" = "unknown" ]; then
  case "$UNAME_M_L" in
    x86_64|x86-64|amd64|x64) ARCH=amd64 ;;
    i386|i486|i586|i686|x86) ARCH=i386 ;;
    aarch64|arm64) ARCH=arm64 ;;
    armv7l|armv7|armv6l|arm) ARCH=armv7 ;;
    riscv64) ARCH=riscv64 ;;
    *) ARCH="unknown" ;;
  esac
fi

# ---------- Detect musl / alpine (unless overridden) ----------
MUSL=false
if [ -n "${ARG_MUSL_L}" ]; then
  case "${ARG_MUSL_L}" in
    1|true|yes|y) MUSL=true ;;
    0|false|no|n) MUSL=false ;;
    *)
      printf '%s\n' "Unrecognized --musl value: ${ARG_MUSL}. Use true/false." >&2
      exit 1
      ;;
  esac
else
  MUSL=false
  if [ -r /etc/os-release ]; then
    if grep -qiE '^id=alpine' /etc/os-release 2>/dev/null || grep -qiE '^id_like=.*alpine' /etc/os-release 2>/dev/null; then
      MUSL=true
    fi
  fi
fi

# ---------- Select branch ----------
BRANCH="thirdparty-unknown-unknown"

case "$UNAME_S_L" in
  linux*)
    if [ "$MUSL" = true ]; then
      # use musl-specific branch names (example: thirdparty-linuxmusl-amd64)
      case "$ARCH" in
        amd64)  BRANCH="thirdparty-linuxmusl-amd64" ;;
        arm64)  BRANCH="thirdparty-linuxmusl-arm64" ;;
        armv7)  BRANCH="thirdparty-linuxmusl-arm" ;;
        riscv64)BRANCH="thirdparty-linuxmusl-riscv64" ;;
        i386)   BRANCH="thirdparty-linuxmusl-i386" ;;
        *) printf '%s\n' "Unsupported Linux arch for musl: $UNAME_M" >&2; exit 1 ;;
      esac
    else
      case "$ARCH" in
        amd64)  BRANCH="thirdparty-linux-amd64" ;;
        arm64)  BRANCH="thirdparty-linux-arm64" ;;
        armv7)  BRANCH="thirdparty-linux-arm" ;;
        riscv64)BRANCH="thirdparty-linux-riscv64" ;;
        i386)   BRANCH="thirdparty-linux-i386" ;;
        *) printf '%s\n' "Unsupported Linux arch: $UNAME_M" >&2; exit 1 ;;
      esac
    fi
    ;;

  darwin*)
    case "$ARCH" in
      amd64) BRANCH="thirdparty-macos-amd64" ;;
      arm64) BRANCH="thirdparty-macos-arm64" ;;
      *) printf '%s\n' "Unsupported macOS arch: $UNAME_M" >&2; exit 1 ;;
    esac
    ;;

  freebsd*)
    case "$ARCH" in
      amd64) BRANCH="thirdparty-freebsd-amd64" ;;
      arm64) BRANCH="thirdparty-freebsd-arm64" ;;
      riscv64) BRANCH="thirdparty-freebsd-aarch64" ;; # adjust if you don't have this
      i386) BRANCH="thirdparty-freebsd-i386" ;;
      *) printf '%s\n' "Unsupported FreeBSD arch: $UNAME_M" >&2; exit 1 ;;
    esac
    ;;

  openbsd*)
    case "$ARCH" in
      amd64) BRANCH="thirdparty-openbsd-amd64" ;;
      i386) BRANCH="thirdparty-openbsd-i386" ;;
      *) printf '%s\n' "Unsupported OpenBSD arch: $UNAME_M" >&2; exit 1 ;;
    esac
    ;;

  mingw*|msys*|cygwin*)
    case "$ARCH" in
      amd64) BRANCH="thirdparty-windows-amd64" ;;
      i386)  BRANCH="thirdparty-windows-i386" ;;
      arm64) BRANCH="thirdparty-windows-arm64" ;;
      *) printf '%s\n' "Unsupported Windows arch: $UNAME_M / $WIN_PROC" >&2; exit 1 ;;
    esac
    ;;

  *)
    printf '%s\n' "Unsupported OS: $UNAME_S" >&2
    exit 1
    ;;
esac

# Debug output (very helpful in CI)
echo "Detected UNAME_S=${UNAME_S} UNAME_M=${UNAME_M}"
echo "Derived UNAME_S_L=${UNAME_S_L} UNAME_M_L=${UNAME_M_L}"
echo "Final ARCH=${ARCH}, MUSL=${MUSL}"
echo "Using branch: ${BRANCH}"

# Remove old checkout if exists
if [ -d "${TARGET_DIR}" ]; then
    echo "Removing old ${TARGET_DIR}"
    rm -rf "${TARGET_DIR}"
fi

# Clone the repo
echo "Cloning ${REPO_URL} (branch ${BRANCH}) into ${TARGET_DIR}"
git clone --depth 1 --branch "${BRANCH}" "${REPO_URL}" "${TARGET_DIR}"

echo "";
echo "Done. tccbin is available in ${TARGET_DIR}"