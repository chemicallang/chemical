module std

source "src"
source "win" if windows && !wasm32
source "posix" if !windows && !wasm32
// Freestanding wasm: no threads/processes/files. The safe subset (string,
// vector, Option/Result, maps, mem, math, formatting) lives in src and works;
// std/wasm supplies the single-threaded lock/time stubs.
source "wasm" if wasm32

import cstd
import core
