#!/usr/bin/env bash
# Copyright (c) Chemical Language Foundation 2025.
set -euo pipefail

REPO_OWNER="chemicallang"
REPO_NAME="tcclib"
TARGET_DIR="lib/tcc"

UNAME_S="$(uname -s 2>/dev/null || echo Unknown)"
UNAME_M="$(uname -m 2>/dev/null || echo unknown)"

# normalize to lowercase
UNAME_S_L="$(printf '%s' "$UNAME_S" | tr '[:upper:]' '[:lower:]')"
UNAME_M_L="$(printf '%s' "$UNAME_M" | tr '[:upper:]' '[:lower:]')"


# ---------------------------------------
# Parse args: support --arch <value>, --musl <true|false>, --tag <version>
# ---------------------------------------
ARG_ARCH=""
ARG_MUSL=""
ARG_TAG=""
ARG_MINGW=false
ARG_CRT=""

while [ $# -gt 0 ]; do
  case "$1" in
    --arch) ARG_ARCH="${2:-}"; shift 2 ;;
    --arch=*) ARG_ARCH="${1#*=}"; shift ;;
    --musl) ARG_MUSL="${2:-}"; shift 2 ;;
    --musl=*) ARG_MUSL="${1#*=}"; shift ;;
    --tag) ARG_TAG="${2:-}"; shift 2 ;;
    --tag=*) ARG_TAG="${1#*=}"; shift ;;
    --mingw) ARG_MINGW=true; shift ;;
    --ucrt) ARG_CRT="ucrt"; shift ;;
    --msvcrt) ARG_CRT="msvcrt"; shift ;;
    --help|-h) printf '%s\n' "Usage: $0 [--arch <arch>] [--musl <true|false>] [--tag <version>] [--mingw] [--ucrt] [--msvcrt]" ; exit 0 ;;
    *) # stop parsing unknown args
       break ;;
  esac
done

ARG_ARCH_L="$(printf '%s' "$ARG_ARCH" | tr '[:upper:]' '[:lower:]')"
ARG_MUSL_L="$(printf '%s' "$ARG_MUSL" | tr '[:upper:]' '[:lower:]')"

# ---------- Check for required tools ----------
if ! command -v curl >/dev/null 2>&1; then
    echo "Error: curl is required but not found." >&2
    exit 1
fi

if ! command -v unzip >/dev/null 2>&1; then
    echo "Error: unzip is required but not found." >&2
    exit 1
fi

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

# ---------- Select asset name (based on previous branch logic) ----------
# Naming convention: {OS}[musl]-{ARCH}.zip
# We derive this from the old branch names by removing 'thirdparty-' and matching the structure.
ASSET_BASE="unknown-unknown"

case "$UNAME_S_L" in
  linux*)
    if [ "$MUSL" = true ]; then
      case "$ARCH" in
        amd64)  ASSET_BASE="linuxmusl-amd64" ;;
        arm64)  ASSET_BASE="linuxmusl-arm64" ;;
        armv7)  ASSET_BASE="linuxmusl-arm" ;;
        riscv64)ASSET_BASE="linuxmusl-riscv64" ;;
        i386)   ASSET_BASE="linuxmusl-i386" ;;
        *) printf '%s\n' "Unsupported Linux arch for musl: $UNAME_M" >&2; exit 1 ;;
      esac
    else
      case "$ARCH" in
        amd64)  ASSET_BASE="linux-amd64" ;;
        arm64)  ASSET_BASE="linux-arm64" ;;
        armv7)  ASSET_BASE="linux-arm" ;;
        riscv64)ASSET_BASE="linux-riscv64" ;;
        i386)   ASSET_BASE="linux-i386" ;;
        *) printf '%s\n' "Unsupported Linux arch: $UNAME_M" >&2; exit 1 ;;
      esac
    fi
    ;;

  darwin*)
    case "$ARCH" in
      amd64) ASSET_BASE="macos-amd64" ;;
      arm64) ASSET_BASE="macos-arm64" ;;
      *) printf '%s\n' "Unsupported macOS arch: $UNAME_M" >&2; exit 1 ;;
    esac
    ;;

  freebsd*)
    case "$ARCH" in
      amd64) ASSET_BASE="freebsd-amd64" ;;
      arm64) ASSET_BASE="freebsd-arm64" ;;
      riscv64) ASSET_BASE="freebsd-aarch64" ;; # Kept from original script logic
      i386) ASSET_BASE="freebsd-i386" ;;
      *) printf '%s\n' "Unsupported FreeBSD arch: $UNAME_M" >&2; exit 1 ;;
    esac
    ;;

  openbsd*)
    case "$ARCH" in
      amd64) ASSET_BASE="openbsd-amd64" ;;
      i386) ASSET_BASE="openbsd-i386" ;;
      *) printf '%s\n' "Unsupported OpenBSD arch: $UNAME_M" >&2; exit 1 ;;
    esac
    ;;

  mingw*|msys*|cygwin*)
    if [ "$ARG_MINGW" = true ]; then
      CRT="${ARG_CRT:-ucrt}"
      if [ "$ARCH" = "amd64" ]; then
        ASSET_BASE="windows-mingw-${CRT}-x64"
      elif [ "$ARCH" = "arm64" ]; then
        ASSET_BASE="windows-mingw-${CRT}-arm64"
      else
        printf '%s\n' "Unsupported MinGW arch: $UNAME_M" >&2; exit 1
      fi
    else
      case "$ARCH" in
        amd64) ASSET_BASE="windows-amd64" ;;
        i386)  ASSET_BASE="windows-i386" ;;
        arm64) ASSET_BASE="windows-arm64" ;;
        *) printf '%s\n' "Unsupported Windows arch: $UNAME_M / $WIN_PROC" >&2; exit 1 ;;
      esac
    fi
    ;;

  *)
    printf '%s\n' "Unsupported OS: $UNAME_S" >&2
    exit 1
    ;;
esac

ASSET_NAME="${ASSET_BASE}.zip"
echo "Detected platform asset: ${ASSET_NAME}"

# ---------- Determine version/tag ----------
TAG="${ARG_TAG}"

if [ -z "${TAG}" ]; then
    echo "Fetching latest tag from GitHub..."
    # Use git ls-remote to avoid GitHub API network issues (like error 56) and rate limits.
    # We sort by version and take the last one.
    TAG=$(git ls-remote --tags "https://github.com/${REPO_OWNER}/${REPO_NAME}.git" | \
          grep -v '\^{}' | \
          cut -d '/' -f 3 | \
          sort -V 2>/dev/null || \
          git ls-remote --tags "https://github.com/${REPO_OWNER}/${REPO_NAME}.git" | \
          grep -v '\^{}' | \
          cut -d '/' -f 3 | \
          sort) # Fallback if sort -V is not supported
    
    TAG=$(echo "$TAG" | tail -n 1)

    if [ -z "${TAG}" ]; then
        echo "Error: Could not determine latest release tag via git ls-remote." >&2
        exit 1
    fi
    echo "Latest release is: ${TAG}"
else
    echo "Using specified tag: ${TAG}"
fi

# ---------- Download and Extract ----------
DOWNLOAD_URL="https://github.com/${REPO_OWNER}/${REPO_NAME}/releases/download/${TAG}/${ASSET_NAME}"
TEMP_ZIP="tcclib_download.zip"

# Remove old checkout if exists
if [ -d "${TARGET_DIR}" ]; then
    rm -rf "${TARGET_DIR}"
fi

echo "Downloading tcclib..."
# Use -sS to show errors but hide progress meter
if ! curl -sSLf -o "${TEMP_ZIP}" "${DOWNLOAD_URL}"; then
    echo "Error: Failed to download asset." >&2
    echo "URL: ${DOWNLOAD_URL}" >&2
    rm -f "${TEMP_ZIP}"
    exit 1
fi

# Create target directory (parent)
mkdir -p "$(dirname "${TARGET_DIR}")"

TEMP_EXTRACT_DIR="tcclib_extract_temp"
rm -rf "${TEMP_EXTRACT_DIR}"
mkdir -p "${TEMP_EXTRACT_DIR}"

# 1. Try unzip (standard, works on many platforms)
# On some Windows ZIP files created with backslashes, Info-ZIP unzip might fail with a warning.
EXTRACT_SUCCESS=false
if unzip -q "${TEMP_ZIP}" -d "${TEMP_EXTRACT_DIR}" 2>/dev/null; then
    EXTRACT_SUCCESS=true
fi

# 2. Fallback for Windows if unzip failed (common for backslash path separators in ZIPs)
if [ "$EXTRACT_SUCCESS" = false ] && [[ "$UNAME_S_L" == mingw* || "$UNAME_S_L" == msys* || "$UNAME_S_L" == cygwin* ]]; then
    # Try powershell Expand-Archive (guaranteed on GH Actions Windows runners)
    if command -v powershell.exe >/dev/null 2>&1; then
        echo "unzip failed, trying powershell.exe Expand-Archive..."
        if powershell.exe -NoProfile -Command "Expand-Archive -Path '${TEMP_ZIP}' -DestinationPath '${TEMP_EXTRACT_DIR}' -Force" >/dev/null 2>&1; then
            EXTRACT_SUCCESS=true
        fi
    fi

    # Try tar (modern Windows has a built-in tar that handles ZIPs well)
    if [ "$EXTRACT_SUCCESS" = false ] && command -v tar >/dev/null 2>&1; then
        echo "Trying tar..."
        if tar -xf "${TEMP_ZIP}" -C "${TEMP_EXTRACT_DIR}" >/dev/null 2>&1; then
            EXTRACT_SUCCESS=true
        fi
    fi
fi

if [ "$EXTRACT_SUCCESS" = false ]; then
    echo "Error: Failed to extract downloaded file '${TEMP_ZIP}'. Please ensure 'unzip' is installed or use a supported environment." >&2
    rm -f "${TEMP_ZIP}"
    rm -rf "${TEMP_EXTRACT_DIR}"
    exit 1
fi

rm -f "${TEMP_ZIP}"

# Move contents to TARGET_DIR
# We need to handle if the zip contains a top-level directory or valid files directly.
# If there's a single directory inside, we move that. If multiple files, we move all to TARGET_DIR.
# Let's see what's in there.
FILES_IN_ZIP=$(ls -1 "${TEMP_EXTRACT_DIR}")
FILE_COUNT=$(echo "$FILES_IN_ZIP" | wc -l)

# Prepare target variable
mkdir -p "${TARGET_DIR}"

if [ "$FILE_COUNT" -eq 1 ] && [ -d "${TEMP_EXTRACT_DIR}/$(echo "$FILES_IN_ZIP" | head -n1)" ]; then
    # It's a single directory, move its contents or the directory itself?
    # Usually we want the contents of that directory to be in lib/tcc
    SINGLE_DIR="${TEMP_EXTRACT_DIR}/$(echo "$FILES_IN_ZIP" | head -n1)"
    echo "Detected single directory in zip: $(basename "$SINGLE_DIR"). Moving contents."
    # Move contents of that dir to target
    # Use cp -r and then rm to avoid "Directory not empty" or cross-device move issues with mv on some systems
    cp -r "$SINGLE_DIR"/* "${TARGET_DIR}/"
    cp -r "$SINGLE_DIR"/.* "${TARGET_DIR}/" 2>/dev/null || true # hidden files
else
    echo "Detected flat files or multiple items. Moving all to ${TARGET_DIR}."
    cp -r "${TEMP_EXTRACT_DIR}"/* "${TARGET_DIR}/"
fi

rm -rf "${TEMP_EXTRACT_DIR}"

echo ""
echo "Done. tccbin is installed in ${TARGET_DIR}"