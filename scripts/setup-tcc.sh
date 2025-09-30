#!/usr/bin/env bash
#
# Copyright (c) Chemical Language Foundation 2025.
#

set -euo pipefail

REPO_URL="https://github.com/chemicallang/tcclib.git"
TARGET_DIR="lib/tcc"

# Detect OS
UNAME_S="$(uname -s)"
UNAME_M="$(uname -m)"

case "${UNAME_S}" in
    Linux*)
        case "${UNAME_M}" in
            x86_64)   BRANCH="thirdparty-linux-amd64" ;;
            armv7l)   BRANCH="thirdparty-linux-arm" ;;
            arm64)    BRANCH="thirdparty-linux-arm64" ;;
            aarch64)  BRANCH="thirdparty-linux-aarch64" ;;
            riscv64)  BRANCH="thirdparty-linux-riscv64" ;;
            *)        echo "Unsupported Linux arch: ${UNAME_M}" && exit 1 ;;
        esac
        ;;
    Darwin*)
        case "${UNAME_M}" in
            x86_64)   BRANCH="thirdparty-macos-amd64" ;;
            arm64)    BRANCH="thirdparty-macos-arm64" ;;
            *)        echo "Unsupported macOS arch: ${UNAME_M}" && exit 1 ;;
        esac
        ;;
    FreeBSD*)
        case "${UNAME_M}" in
            x86_64)   BRANCH="thirdparty-freebsd-amd64" ;;
            arm64)    BRANCH="thirdparty-freebsd-arm64" ;;
            aarch64)  BRANCH="thirdparty-freebsd-aarch64" ;;
            *)        echo "Unsupported FreeBSD arch: ${UNAME_M}" && exit 1 ;;
        esac
        ;;
    OpenBSD*)
        case "${UNAME_M}" in
            x86_64)   BRANCH="thirdparty-openbsd-amd64" ;;
            *)        echo "Unsupported OpenBSD arch: ${UNAME_M}" && exit 1 ;;
        esac
        ;;
    MINGW*|MSYS*|CYGWIN*)
        case "${UNAME_M}" in
            x86)      BRANCH="thirdparty-windows-i386" ;;
            x86_64)   BRANCH="thirdparty-windows-amd64" ;;
            *)        echo "Unsupported Windows arch: ${UNAME_M}" && exit 1 ;;
        esac
        ;;
    *)
        BRANCH="thirdparty-unknown-unknown"
        ;;
esac

echo "Detected OS=${UNAME_S}, Arch=${UNAME_M}"
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