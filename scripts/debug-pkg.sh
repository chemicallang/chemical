#!/usr/bin/env sh
# package_exe.sh
# Usage: package_exe.sh <exe_dir> <is_compiler>
#  exe_dir     - target directory (where source binaries may live and where zip will be produced)
#  is_compiler - boolean: true/false (also accepts 1/0, yes/no)

set -eu

usage() {
  cat <<EOF
Usage: $0 <exe_dir> <is_compiler>
  exe_dir      - target directory (e.g. /path/to/myexe)
  is_compiler  - boolean: true/false (also accepts 1/0, yes/no)
EOF
  exit 2
}

# Normalize boolean
is_true() {
  case "$(printf '%s' "$1" | tr '[:upper:]' '[:lower:]')" in
    1|true|yes|y) return 0 ;;
    *) return 1 ;;
  esac
}

# --- Auto-detect clang version -------------------------------------------------
CLANG_VERSION=""
if [ -d "out/host/lib/clang" ]; then
    CLANG_VERSION=$(ls -1 out/host/lib/clang 2>/dev/null | head -n 1)
    if [ -n "$CLANG_VERSION" ]; then
        echo "Detected clang version: $CLANG_VERSION"
    else
        echo "warning: Could not detect clang version in out/host/lib/clang" >&2
    fi
else
    echo "warning: out/host/lib/clang directory not found" >&2
fi

# --- Arg parsing ----------------------------------------------------------
if [ "$#" -ne 2 ]; then
  usage
fi

EXE_DIR_RAW="$1"
IS_COMPILER_RAW="$2"

if [ -z "$EXE_DIR_RAW" ]; then
  echo "error: exe_dir must not be empty" >&2
  usage
fi

if is_true "$IS_COMPILER_RAW"; then
  IS_COMPILER=1
else
  IS_COMPILER=0
fi

# Make EXE_DIR an absolute path (preferred). This avoids the zip-path-inside-dir problem.
# If cd fails, keep the original (rare).
if abspath="$(cd "$EXE_DIR_RAW" >/dev/null 2>&1 && pwd)"; then
  EXE_DIR="$abspath"
else
  # If the directory doesn't exist yet, convert to absolute relative to cwd
  # This mirrors mkdir -p behavior used later.
  case "$EXE_DIR_RAW" in
    /*) EXE_DIR="$EXE_DIR_RAW" ;;               # already absolute
    *) EXE_DIR="$(pwd)/$EXE_DIR_RAW" ;;         # make absolute
  esac
fi

# --- Platform detection ---------------------------------------------------
UNAME_OUT="$(uname -s 2>/dev/null || echo unknown)"
case "$UNAME_OUT" in
  *CYGWIN*|*MINGW*|*MSYS*|*NT-*) PLATFORM=windows ;;
  *) PLATFORM=unix ;;
esac

echo "Platform detected: $PLATFORM"
echo "Target exe_dir: $EXE_DIR"
if [ "$IS_COMPILER" -eq 1 ]; then
  echo "Mode: is_compiler = true"
else
  echo "Mode: is_compiler = false"
fi

# --- Prepare target directories -------------------------------------------
PKG_DIR="$EXE_DIR/pkg"
mkdir -p "$PKG_DIR" || { echo "failed to create $PKG_DIR" >&2; exit 1; }

# --- Choose filenames based on platform ----------------------------------
if [ "$PLATFORM" = "windows" ]; then
  TCC_BIN_NAME="TCCCompiler.exe"
  COMPILER_BIN_NAME="Compiler.exe"
  LIBTCC_NAME="libtcc.dll"
  CHEMICAL_NAME="chemical.exe"
else
  TCC_BIN_NAME="TCCCompiler"
  COMPILER_BIN_NAME="Compiler"
  LIBTCC_NAME="libtcc.so"
  CHEMICAL_NAME="chemical"
fi

# --- Copy binary (TCCCompiler or Compiler) and rename to chemical[.exe] ---
if [ "$IS_COMPILER" -eq 1 ]; then
  SRC_BIN="$EXE_DIR/$COMPILER_BIN_NAME"
else
  SRC_BIN="$EXE_DIR/$TCC_BIN_NAME"
fi

if [ ! -e "$SRC_BIN" ]; then
  echo "error: source binary '$SRC_BIN' not found" >&2
  exit 1
fi

DST_BIN="$PKG_DIR/$CHEMICAL_NAME"
cp -p "$SRC_BIN" "$DST_BIN" || { echo "failed to copy binary" >&2; exit 1; }
echo "copied $SRC_BIN -> $DST_BIN"

# --- Copy libtcc shared library -------------------------------------------
SRC_LIB="lib/tcc/$LIBTCC_NAME"
DST_LIB="$PKG_DIR/$LIBTCC_NAME"
if [ ! -e "$SRC_LIB" ]; then
  echo "error: source library '$SRC_LIB' not found" >&2
  exit 1
fi
cp -p "$SRC_LIB" "$DST_LIB" || { echo "failed to copy libtcc" >&2; exit 1; }
echo "copied $SRC_LIB -> $DST_LIB"

# --- Copy include and lib directories ------------------------------------
mkdir -p "$PKG_DIR/lib/tcc"
if [ ! -d "lib/tcc/include" ]; then
  echo "error: source directory 'lib/tcc/include' not found" >&2
  exit 1
fi
cp -a "lib/tcc/include" "$PKG_DIR/lib/tcc/" || { echo "failed to copy lib/tcc/include" >&2; exit 1; }
echo "copied lib/tcc/include -> $PKG_DIR/lib/tcc/include"

if [ ! -d "lib/tcc/lib" ]; then
  echo "error: source directory 'lib/tcc/lib' not found" >&2
  exit 1
fi
cp -a "lib/tcc/lib" "$PKG_DIR/lib/tcc/" || { echo "failed to copy lib/tcc/lib" >&2; exit 1; }
echo "copied lib/tcc/lib -> $PKG_DIR/lib/tcc/lib"

# --- Copy lang/libs -> pkg/lang/libs -------------------------------------
if [ -d "lang/libs" ]; then
  cp -a "lang/libs" "$PKG_DIR/" || { echo "failed to copy lang/libs" >&2; exit 1; }
  echo "copied lang/libs -> $PKG_DIR/libs"
else
  echo "warning: lang/libs not found; skipping copy of lang/libs" >&2
fi

# --- If is_compiler true, copy clang resources -> exe_dir/pkg/resources ----
if [ "$IS_COMPILER" -eq 1 ]; then
  if [ -n "$CLANG_VERSION" ] && [ -d "out/host/lib/clang/$CLANG_VERSION/include" ]; then
    mkdir -p "$PKG_DIR/resources"
    cp -a "out/host/lib/clang/$CLANG_VERSION/include" "$PKG_DIR/resources/" || { echo "failed to copy clang resources" >&2; exit 1; }
    echo "copied out/host/lib/clang/$CLANG_VERSION/include -> $PKG_DIR/resources/include"
  elif [ -d "lib/include" ]; then
    # Fallback to old location for backwards compatibility
    mkdir -p "$PKG_DIR/lib"
    cp -a "lib/include" "$PKG_DIR/lib/" || { echo "failed to copy lib/include" >&2; exit 1; }
    echo "copied lib/include -> $PKG_DIR/lib/include"
  else
    echo "warning: clang resources not found, skipping (expected at out/host/lib/clang/<version>/include)" >&2
  fi
fi

# --- Create zip containing the 'pkg' directory at top-level ----------------
# Result: the zip will have a top-level entry 'pkg/'.
ZIP_PATH="$EXE_DIR/debug-package.zip"

# Ensure parent directory for zip exists (should, because we created PKG_DIR)
mkdir -p "$(dirname "$ZIP_PATH")"

if [ "$PLATFORM" = "windows" ]; then
  # Convert Unix paths to Windows format for PowerShell
  WIN_PKG_DIR=$(cygpath -w "$PKG_DIR" 2>/dev/null || echo "$PKG_DIR")
  WIN_ZIP_PATH=$(cygpath -w "$ZIP_PATH" 2>/dev/null || echo "$ZIP_PATH")
  powershell.exe -NoProfile -Command "Compress-Archive -Path '$WIN_PKG_DIR' -DestinationPath '$WIN_ZIP_PATH' -Force" \
    || { echo "Compress-Archive failed" >&2; exit 1; }
else
  if ! command -v zip >/dev/null 2>&1; then
    echo "error: 'zip' command not found. Please install zip and retry." >&2
    exit 1
  fi
  oldpwd="$(pwd)"
  cd "$EXE_DIR" || { echo "failed to cd into $EXE_DIR" >&2; exit 1; }

  # Write the zip into the current directory using basename of ZIP_PATH to avoid double-dir issues.
  zip -r "$(basename "$ZIP_PATH")" "pkg" || { echo "zip failed" >&2; cd "$oldpwd"; exit 1; }
  cd "$oldpwd"
fi

echo "Created zip: $ZIP_PATH (contains top-level 'pkg' directory)"
echo "Done."
