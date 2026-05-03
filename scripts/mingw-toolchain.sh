#!/usr/bin/env bash
# Copyright (c) Chemical Language Foundation 2025.
set -euo pipefail

REPO_OWNER="mstorsjo"
REPO_NAME="llvm-mingw"
TARGET_DIR="toolchains/llvm-mingw"

# Default configuration
TAG="20240917"
CRUNTIME="ucrt"
ARCH="x86_64"

while [ $# -gt 0 ]; do
  case "$1" in
    --tag) TAG="${2:-}"; shift 2 ;;
    --tag=*) TAG="${1#*=}"; shift ;;
    --ucrt) CRUNTIME="ucrt"; shift ;;
    --msvcrt) CRUNTIME="msvcrt"; shift ;;
    --arch) ARCH="${2:-}"; shift 2 ;;
    --arch=*) ARCH="${1#*=}"; shift ;;
    --help|-h) 
       printf '%s\n' "Usage: $0 [--tag <version>] [--ucrt] [--msvcrt] [--arch <x86_64|aarch64|...>]"
       printf '%s\n' "Downloads and extracts the mstorsjo/llvm-mingw toolchain on Windows."
       exit 0 
       ;;
    *) # skip unknown args
       shift ;;
  esac
done

if ! command -v curl >/dev/null 2>&1; then
    echo "Error: curl is required but not found." >&2
    exit 1
fi

if ! command -v unzip >/dev/null 2>&1; then
    echo "Error: unzip is required but not found." >&2
    exit 1
fi

# Determine the asset name
# Format: llvm-mingw-{TAG}-{CRUNTIME}-{ARCH}.zip
ASSET_NAME="llvm-mingw-${TAG}-${CRUNTIME}-${ARCH}.zip"

DOWNLOAD_URL="https://github.com/${REPO_OWNER}/${REPO_NAME}/releases/download/${TAG}/${ASSET_NAME}"
TEMP_FILE="llvm_mingw_temp.zip"

echo "Downloading llvm-mingw toolchain from ${DOWNLOAD_URL}..."
if ! curl -sSLf -o "${TEMP_FILE}" "${DOWNLOAD_URL}"; then
    echo "Error: Failed to download llvm-mingw asset." >&2
    echo "URL: ${DOWNLOAD_URL}" >&2
    rm -f "${TEMP_FILE}"
    exit 1
fi

echo "Extracting..."
mkdir -p "${TARGET_DIR}"

TEMP_EXTRACT_DIR="llvm_mingw_extract_temp"
rm -rf "${TEMP_EXTRACT_DIR}"
mkdir -p "${TEMP_EXTRACT_DIR}"

if ! unzip -q "${TEMP_FILE}" -d "${TEMP_EXTRACT_DIR}"; then
    echo "Error: Failed to unzip downloaded file." >&2
    rm -f "${TEMP_FILE}"
    rm -rf "${TEMP_EXTRACT_DIR}"
    exit 1
fi

rm -f "${TEMP_FILE}"

# Move contents to TARGET_DIR
# Usually the zip contains one top-level directory: llvm-mingw-20240917-ucrt-x86_64
FILES_IN_ZIP=$(ls -1 "${TEMP_EXTRACT_DIR}")
FILE_COUNT=$(echo "$FILES_IN_ZIP" | wc -l)

if [ "$FILE_COUNT" -eq 1 ] && [ -d "${TEMP_EXTRACT_DIR}/$(echo "$FILES_IN_ZIP" | head -n1)" ]; then
    SINGLE_DIR="${TEMP_EXTRACT_DIR}/$(echo "$FILES_IN_ZIP" | head -n1)"
    echo "Moving contents from ${SINGLE_DIR} to ${TARGET_DIR}."
    cp -r "$SINGLE_DIR"/* "${TARGET_DIR}/"
    cp -r "$SINGLE_DIR"/.* "${TARGET_DIR}/" 2>/dev/null || true
else
    echo "Moving all contents to ${TARGET_DIR}."
    cp -r "${TEMP_EXTRACT_DIR}"/* "${TARGET_DIR}/"
fi

rm -rf "${TEMP_EXTRACT_DIR}"

echo ""
echo "Done. llvm-mingw installed in ${TARGET_DIR}"
