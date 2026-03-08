#!/usr/bin/env bash
# Copyright (c) Chemical Language Foundation 2025.
set -euo pipefail

REPO_OWNER="chemicallang"
REPO_NAME="llvm-prebuilt"
TARGET_DIR="out/host"

UNAME_S="$(uname -s 2>/dev/null || echo Unknown)"
UNAME_M="$(uname -m 2>/dev/null || echo unknown)"

# normalize to lowercase
UNAME_S_L="$(printf '%s' "$UNAME_S" | tr '[:upper:]' '[:lower:]')"
UNAME_M_L="$(printf '%s' "$UNAME_M" | tr '[:upper:]' '[:lower:]')"

# ---------------------------------------
# Parse args
# ---------------------------------------
ARG_ARCH=""
ARG_MUSL=""
ARG_TAG="llvm18"
ARG_MINGW=false
ARG_UCRT=false
ARG_MSVCRT=false

while [ $# -gt 0 ]; do
  case "$1" in
    --arch) ARG_ARCH="${2:-}"; shift 2 ;;
    --arch=*) ARG_ARCH="${1#*=}"; shift ;;
    --musl) ARG_MUSL="${2:-}"; shift 2 ;;
    --musl=*) ARG_MUSL="${1#*=}"; shift ;;
    --tag) ARG_TAG="${2:-}"; shift 2 ;;
    --tag=*) ARG_TAG="${1#*=}"; shift ;;
    --mingw) ARG_MINGW=true; shift ;;
    --ucrt) ARG_UCRT=true; shift ;;
    --msvcrt) ARG_MSVCRT=true; shift ;;
    --help|-h) 
       printf '%s\n' "Usage: $0 [--arch <arch>] [--musl <true|false>] [--tag <version>] [--mingw] [--ucrt] [--msvcrt]"
       exit 0 
       ;;
    *) # skip unknown args
       shift ;;
  esac
done

ARG_ARCH_L="$(printf '%s' "$ARG_ARCH" | tr '[:upper:]' '[:lower:]')"

# ---------- Check for required tools ----------
if ! command -v curl >/dev/null 2>&1; then
    echo "Error: curl is required but not found." >&2
    exit 1
fi

if ! command -v unzip >/dev/null 2>&1; then
    echo "Error: unzip is required but not found." >&2
    exit 1
fi

if ! command -v tar >/dev/null 2>&1; then
    echo "Error: tar is required but not found." >&2
    exit 1
fi

# ---------- Determine ARCH ----------
ARCH="unknown"

# 1) If user supplied --arch, use it (map common synonyms to canonical)
if [ -n "${ARG_ARCH_L}" ]; then
  case "${ARG_ARCH_L}" in
    x64|x86_64|x86-64|amd64) ARCH=x64 ;;
    arm64|aarch64)          ARCH=arm64 ;;
    *)
      printf '%s\n' "Unrecognized --arch value: ${ARG_ARCH}. Supported: x64, arm64" >&2
      exit 1
      ;;
  esac
fi

# 2) If not provided, fall back to uname -m mapping
if [ "$ARCH" = "unknown" ]; then
  case "$UNAME_M_L" in
    x86_64|x86-64|amd64|x64) ARCH=x64 ;;
    aarch64|arm64) ARCH=arm64 ;;
    *) 
      printf '%s\n' "Unsupported arch: $UNAME_M" >&2
      exit 1
      ;;
  esac
fi

# ---------- Select asset name ----------
ASSET_BASE="unknown"
EXT="tar.gz"

case "$UNAME_S_L" in
  linux*)
    case "$ARCH" in
      x64)    ASSET_BASE="linux-x64" ;;
      arm64)  ASSET_BASE="linux-arm64" ;;
      *) printf '%s\n' "Unsupported Linux arch: $ARCH" >&2; exit 1 ;;
    esac
    ;;

  darwin*)
    case "$ARCH" in
      x64)   ASSET_BASE="macos-x64" ;;
      arm64) ASSET_BASE="macos-arm64" ;;
      *) printf '%s\n' "Unsupported macOS arch: $ARCH" >&2; exit 1 ;;
    esac
    ;;

  mingw*|msys*|cygwin*)
    EXT="zip"
    if [ "$ARG_UCRT" = true ]; then
        # user explicitly requested ucrt
        ASSET_BASE="windows-mingw-ucrt-${ARCH}"
    elif [ "$ARG_MSVCRT" = true ]; then
        # user explicitly requested msvcrt
        ASSET_BASE="windows-mingw-msvcrt-${ARCH}"
    elif [ "$ARG_MINGW" = true ]; then
        # user requested mingw, default to ucrt
        ASSET_BASE="windows-mingw-ucrt-${ARCH}"
    else
        # default to msvc
        ASSET_BASE="windows-${ARCH}"
    fi
    ;;

  *)
    printf '%s\n' "Unsupported OS: $UNAME_S" >&2
    exit 1
    ;;
esac

ASSET_NAME="${ASSET_BASE}.${EXT}"
echo "Detected platform asset: ${ASSET_NAME}"

# ---------- Determine version/tag ----------
TAG="${ARG_TAG}"
echo "Using tag: ${TAG}"

# ---------- Download and Extract ----------
DOWNLOAD_URL="https://github.com/${REPO_OWNER}/${REPO_NAME}/releases/download/${TAG}/${ASSET_NAME}"
TEMP_FILE="llvm_download_temp.${EXT}"

echo "Downloading LLVM from ${DOWNLOAD_URL}..."
# Use -sS to show errors but hide progress meter
if ! curl -sSLf -o "${TEMP_FILE}" "${DOWNLOAD_URL}"; then
    echo "Error: Failed to download asset." >&2
    echo "URL: ${DOWNLOAD_URL}" >&2
    rm -f "${TEMP_FILE}"
    exit 1
fi

echo "Extracting..."

if [ "$EXT" = "zip" ]; then
    if ! unzip -q "${TEMP_FILE}" -d .; then
        echo "Error: Failed to unzip downloaded file." >&2
        rm -f "${TEMP_FILE}"
        exit 1
    fi
else
    # tar.gz
    if ! tar -xzf "${TEMP_FILE}" -C .; then
        echo "Error: Failed to extract tar.gz file." >&2
        rm -f "${TEMP_FILE}"
        exit 1
    fi
fi

rm -f "${TEMP_FILE}"

echo ""
echo "Done. LLVM installed in ${TARGET_DIR}"
