module cstd

source "src"
source "windows" if windows && !wasm32
source "posix" if !windows && !wasm32
source "posix_linux" if linux && !wasm32
source "posix_macos" if macos && !wasm32
source "posix_bsd" if freebsd && !wasm32

// WebAssembly is a freestanding target: it gets its own small platform layer
// (stdio sentinels, errno, base typedefs) and never the host POSIX/Windows
// directories. The C symbols those declarations need are provided by
// lang/runtime/wasm/runtime.c.
source "wasm" if wasm32

// Freestanding runtime (allocator, mem/str, printf, abort, console). Compiled
// by clang for wasm32; linked into every wasm build automatically. The C
// emitter path (TCCCompiler) does not compile C modules, so
// scripts/wasm-c-build.sh compiles this file explicitly.
link c "../../runtime/wasm/runtime.c" if wasm32

link "libcmt" if windows && !wasm32 && !tcc && !mingw32 && !mingw64
link "legacy_stdio_definitions" if windows && !wasm32 && !tcc && !mingw32 && !mingw64
link "c" if linux or freebsd
