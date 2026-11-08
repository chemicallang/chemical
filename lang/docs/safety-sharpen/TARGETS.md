# Phase 2 claimed-target matrix

| Library | Linux/TCC | Linux/LLVM | Windows | macOS | Notes |
|---|---|---|---|---|---|
| std/core | tested | tested | cross-checked where toolchain available | cross-checked where toolchain available | host suites |
| fs/path | tested | tested | platform sources compiled; no Windows runner here | POSIX sources | explicit platform exception recorded |
| process/environment | tested | tested | Windows source compiles; no runner here | POSIX source | environment mutation is unsafe |
| net/http | tested (loopback) | tested (loopback) | IOCP source exists; no runner here | POSIX source | safe TCP loopback in `--async`; HTTP/TLS loopback tests |
| tls | tested (opt-in `--tls`) | same | source exists; no runner here | POSIX source | safe client/server tests |
| archive/image/audio/font | tested | tested | format code is portable | portable | malformed-input tests |
| encoding/crypto/compression | tested | tested | portable | portable | round-trip/misuse tests |
| regex/json/mime/cbor | tested | tested | portable | portable | borrowed-view and malformed-input tests |
| window/webview | compile-only unless display available | compile-only | platform source exists | platform source exists | UI-thread affinity/`!Send`; environment exception |
| components/router/page/server | tested | tested | portable | portable | SSR/escaping tests |
| compiler/plugin/lab | tested | tested | host CBI | host CBI | raw CBI remains Tier C |
