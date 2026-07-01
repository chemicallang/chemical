# ── MSVC environment setup (Windows/Git Bash) ─────────────────────────────────
# Source this from any build script that needs to build with MSVC on Windows.
#
# Two MSYS2/Git Bash quirks are handled here:
#
#  1. MSYS2_ARG_CONV_EXCL
#     Git Bash automatically converts Unix-style paths like "/nologo" or
#     "/EHsc" into Windows paths like "C:/Program Files/Git/nologo", which
#     breaks MSVC command-line flags.  Setting MSYS2_ARG_CONV_EXCL tells the
#     runtime to leave arguments matching these prefixes untouched.
#
#  2. INCLUDE / LIB
#     cl.exe relies on the INCLUDE and LIB environment variables to locate
#     the C++ standard library headers (<utility>, <cassert>, <string>, …)
#     and import libraries.  These are normally set by running vcvarsall.bat,
#     but that is hard to invoke reliably from inside Git Bash.
#     Instead we construct the paths ourselves from the Visual Studio
#     installation directory, the MSVC tool version, and the Windows SDK.

# ── Run automatically when sourced ───────────────────────────────────────────
case "$(uname -s 2>/dev/null || true)" in
  MINGW*|MSYS*) ;;
  *) return ;;  # not on Windows (Linux, macOS, …) — nothing to do
esac

# Users can opt out of the automatic MSVC environment by setting
# CHEMICAL_MSVC_AUTO=0 (or any falsy value) before sourcing these scripts.
case "${CHEMICAL_MSVC_AUTO-}" in
  0|false|no|off) return ;;
esac

# ── 1. Prevent MSYS2 from mangling MSVC-style flags ─────────────────────────
export MSYS2_ARG_CONV_EXCL="/nologo;/EHsc;/Fe;/Fo;/I;/D;/c;/GR;/GR-;/Zi;/Ob;/Od;/RTC;/MD;/MT;/W4;/we;/wd;/FS"

# ── 2. Set INCLUDE / LIB for MSVC standard library ──────────────────────────
# Already set?  Skip (user has run vcvarsall manually or has custom setup).
if [ -n "${INCLUDE-}" ] && [ -n "${LIB-}" ]; then
  # Even though INCLUDE/LIB are set, verify cl.exe is actually reachable
  if command -v cl.exe &>/dev/null; then
    CHEMICAL_MSVC_READY=1
    export CHEMICAL_MSVC_READY
  fi
  return
fi

# Locate vswhere
_vs_vswhere=""
if command -v vswhere &>/dev/null; then
  _vs_vswhere="vswhere"
else
  for _p in "/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe" \
            "/c/Program Files/Microsoft Visual Studio/Installer/vswhere.exe"; do
    [ -f "$_p" ] && { _vs_vswhere="$_p"; break; }
  done
fi

[ -z "$_vs_vswhere" ] || [ ! -f "$_vs_vswhere" ] && return

# Get VS installation path (returns Windows-style path, e.g. D:\Software\...)
_vs_path=$("$_vs_vswhere" -latest -property installationPath 2>/dev/null | tr -d '\r\n')
[ -z "$_vs_path" ] && return

# Convert to Unix-style for bash file-checks (e.g. /d/Software/...)
_vs_ux=$(echo "$_vs_path" | sed 's|\\|/|g; s|^\([a-zA-Z]\):|/\1|')

# Read MSVC tools version
_vs_ver_file="${_vs_ux}/VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt"
[ ! -f "$_vs_ver_file" ] && return
_vs_ver=$(cat "$_vs_ver_file" 2>/dev/null | tr -d '\r\n')
[ -z "$_vs_ver" ] && return

# Windows Kits root (probe common locations, keep as Unix path for bash checks)
_kit_root_ux=""
for _kr in "/c/Program Files (x86)/Windows Kits/10" \
           "/d/Windows Kits/10" \
           "/c/Program Files/Windows Kits/10"; do
  [ -d "$_kr/Include" ] && { _kit_root_ux="$_kr"; break; }
done
[ -z "$_kit_root_ux" ] && return

# Convert to Windows path: /d/Windows Kits/10 → D:\Windows Kits\10
_kit_path=$(echo "$_kit_root_ux" | sed 's|^/\([a-zA-Z]\)/|\U\1:\\|; s|/|\\|g')

# Latest SDK version from the Include directory
_sdk_ver=$(ls "$_kit_root_ux/Include/" 2>/dev/null | sort -V | tail -1)
[ -z "$_sdk_ver" ] && return

# Architecture directory
case "$(uname -m 2>/dev/null || true)" in
  x86_64|amd64) _arch="x64" ;;
  i686|x86)     _arch="x86" ;;
  aarch64|arm64) _arch="arm64" ;;
  arm)          _arch="arm" ;;
  *)            _arch="x64" ;;  # sensible default
esac

# ── Build include paths (validate each exists) ──────────────────────────────
_INCLUDE=""
for _base in \
    "${_vs_path}\\VC\\Tools\\MSVC\\${_vs_ver}\\include" \
    "${_vs_path}\\VC\\Tools\\MSVC\\${_vs_ver}\\ATLMFC\\include" \
    "${_vs_path}\\VC\\Auxiliary\\VS\\include" \
    "${_kit_path}\\Include\\${_sdk_ver}\\ucrt" \
    "${_kit_path}\\Include\\${_sdk_ver}\\um" \
    "${_kit_path}\\Include\\${_sdk_ver}\\shared" \
    "${_kit_path}\\Include\\${_sdk_ver}\\winrt" \
    "${_kit_path}\\Include\\${_sdk_ver}\\cppwinrt"; do
    # Convert Windows path to Unix for existence check
    _ux=$(echo "$_base" | sed 's|\\|/|g; s|^\([a-zA-Z]\):|/\1|')
    if [ -d "$_ux" ]; then
        _INCLUDE="${_INCLUDE}${_base};"
    fi
done

# ── Build library paths (validate each exists) ────────────────────────────────
_LIB=""
for _base in \
    "${_vs_path}\\VC\\Tools\\MSVC\\${_vs_ver}\\lib\\${_arch}" \
    "${_vs_path}\\VC\\Tools\\MSVC\\${_vs_ver}\\ATLMFC\\lib\\${_arch}" \
    "${_kit_path}\\Lib\\${_sdk_ver}\\ucrt\\${_arch}" \
    "${_kit_path}\\Lib\\${_sdk_ver}\\um\\${_arch}"; do
    _ux=$(echo "$_base" | sed 's|\\|/|g; s|^\([a-zA-Z]\):|/\1|')
    if [ -d "$_ux" ]; then
        _LIB="${_LIB}${_base};"
    fi
done

# Strip trailing semicolons
_INCLUDE="${_INCLUDE%;}"
_LIB="${_LIB%;}"

# ── Validation ────────────────────────────────────────────────────────────────
CHEMICAL_MSVC_READY=0
if [ -n "$_INCLUDE" ] && [ -n "$_LIB" ]; then
    # Verify cl.exe exists in the expected bin directory
    _VSBIN="${_vs_path}\\VC\\Tools\\MSVC\\${_vs_ver}\\bin\\Host${_arch}\\${_arch}"
    _cl_ux=$(echo "$_VSBIN" | sed 's|\\|/|g; s|^\([a-zA-Z]\):|/\1|')
    if [ -f "${_cl_ux}/cl.exe" ]; then
        CHEMICAL_MSVC_READY=1
        export CHEMICAL_MSVC_READY

        # ── Build PATH ────────────────────────────────────────────────────────
        export PATH="${_VSBIN};${PATH-}"

        # ── Export ────────────────────────────────────────────────────────────
        export INCLUDE="$_INCLUDE"
        export LIB="$_LIB"
    fi
fi

# ── Fallback: use vcvarsall.bat if manual paths failed ──────────────────────
if [ "$CHEMICAL_MSVC_READY" != 1 ] && [ -n "${_vs_path-}" ]; then
    _vcvars_win="${_vs_path}\\VC\\Auxiliary\\Build\\vcvarsall.bat"
    _vcvars_ux=$(echo "$_vcvars_win" | sed 's|\\|/|g; s|^\([a-zA-Z]\):|/\1|')
    if [ -f "$_vcvars_ux" ]; then
        echo "[msvc_env] Manual path construction failed, trying vcvarsall.bat..." >&2
        # Invoke vcvarsall.bat via cmd.exe, then echo each variable between markers
        # to reliably capture potentially multiline values from cmd's 'set' output
        _vs_env=$(cmd //c "\"${_vcvars_win}\" ${_arch} >nul 2>&1 && echo XX_INCLUDE_XX && echo %INCLUDE% && echo XX_LIB_XX && echo %LIB% && echo XX_PATH_XX && echo %PATH%" 2>/dev/null || true)
        if [ -n "$_vs_env" ]; then
            _new_include=$(echo "$_vs_env" | sed -n '/^XX_INCLUDE_XX$/,/^XX_LIB_XX$/{/^XX_INCLUDE_XX$/d;/^XX_LIB_XX$/d;p}')
            _new_lib=$(echo "$_vs_env" | sed -n '/^XX_LIB_XX$/,/^XX_PATH_XX$/{/^XX_LIB_XX$/d;/^XX_PATH_XX$/d;p}')
            _new_path=$(echo "$_vs_env" | sed -n '/^XX_PATH_XX$/,/^$/{/^XX_PATH_XX$/d;p}')
            if [ -n "$_new_include" ] && [ -n "$_new_lib" ]; then
                export INCLUDE="$_new_include"
                export LIB="$_new_lib"
                export PATH="$_new_path"
                CHEMICAL_MSVC_READY=1
                export CHEMICAL_MSVC_READY
                echo "[msvc_env] vcvarsall.bat succeeded." >&2
            fi
        fi
    fi
fi

if [ "$CHEMICAL_MSVC_READY" != 1 ]; then
    echo "[msvc_env] MSVC environment setup failed — INCLUDE/LIB paths or cl.exe not found." >&2
    echo "[msvc_env] If MinGW is available, configure.sh will fall back to it automatically." >&2
fi
