#!/usr/bin/env bash
#
# Copyright (c) Chemical Language Foundation 2026.
#
# run-tests.sh
# Clone the chemical repo (shallow), keep only lang/tests, then run or compile
# the test suite using the `chemical` binary already on PATH.
#
# Usage:
#   ./run-tests.sh [--tag <tag>] [--no-run] [--output|-o <file>] [-- <extra args>]
#
# Options:
#   --tag <tag>       Clone at this specific git tag (default: latest)
#   --no-run          Compile to an executable instead of running directly
#   --output|-o <f>   Output executable name (implies --no-run behaviour for naming;
#                     only meaningful with --no-run)
#   Extra args after the known flags are forwarded to the `chemical` command.

set -euo pipefail

# ── Defaults ─────────────────────────────────────────────────────────────────
REPO_URL="https://github.com/chemicallang/chemical"
TAG=""
NO_RUN=0
OUTPUT_FILE="tests.exe"
OUTPUT_SET=0
EXTRA_ARGS=()

# ── Argument parsing ──────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        --tag)
            TAG="$2"
            shift 2
            ;;
        --no-run)
            NO_RUN=1
            shift
            ;;
        --output|-o)
            OUTPUT_FILE="$2"
            OUTPUT_SET=1
            shift 2
            ;;
        --)
            shift
            EXTRA_ARGS+=("$@")
            break
            ;;
        *)
            EXTRA_ARGS+=("$1")
            shift
            ;;
    esac
done

# ── Clone ─────────────────────────────────────────────────────────────────────
WORK_DIR="$(mktemp -d)"
REPO_DIR="${WORK_DIR}/chemical"

cleanup() {
    rm -rf "${WORK_DIR}"
}
trap cleanup EXIT

echo "Cloning ${REPO_URL} ..."

if [ -n "${TAG}" ]; then
    git clone --depth 1 --branch "${TAG}" "${REPO_URL}" "${REPO_DIR}"
else
    git clone --depth 1 "${REPO_URL}" "${REPO_DIR}"
fi

# ── Keep only lang/tests ──────────────────────────────────────────────────────
echo "Stripping repo to lang/tests ..."

TESTS_SRC="${REPO_DIR}/lang/tests"
TESTS_KEEP="${WORK_DIR}/tests_keep"

if [ ! -d "${TESTS_SRC}" ]; then
    echo "ERROR: lang/tests not found in cloned repo." >&2
    exit 1
fi

# Move lang/tests out, delete everything else, restore
mv "${TESTS_SRC}" "${TESTS_KEEP}"
rm -rf "${REPO_DIR}"
mkdir -p "${REPO_DIR}/lang"
mv "${TESTS_KEEP}" "${REPO_DIR}/lang/tests"

# ── Run or compile ────────────────────────────────────────────────────────────
BUILD_LAB="${REPO_DIR}/lang/tests/build.lab"

if [ ! -f "${BUILD_LAB}" ]; then
    echo "ERROR: build.lab not found at ${BUILD_LAB}" >&2
    exit 1
fi

if [ "${NO_RUN}" -eq 1 ]; then
    echo "Compiling tests -> ${OUTPUT_FILE} ..."
    chemical "${BUILD_LAB}" -o "${OUTPUT_FILE}" "${EXTRA_ARGS[@]}"
else
    echo "Running tests ..."
    chemical run "${BUILD_LAB}" "${EXTRA_ARGS[@]}"
fi
