# Atomic Library — Complete Reference

## Overview

The atomic library (`lang/libs/atomic/`) provides lock-free atomic operations for the Chemical programming language. It replaces all `@extern __atomic_*_N` C runtime calls with pure-Chemical inline asm CAS primitives, eliminating the `-latomic` dependency entirely.

**Key files:**
- `lang/libs/atomic/chemical.mod` — Module declaration with conditional arch-specific source paths
- `lang/libs/atomic/src/atomic.ch` — Common code (~1289 lines): enums, dispatch functions, public API
- `lang/libs/atomic/arch/x86/asm.ch` — x86_64/i386 inline asm primitives (327 lines)
- `lang/libs/atomic/arch/aarch64/asm.ch` — AArch64 inline asm primitives (395 lines)
- `lang/libs/atomic/arch/arm/asm.ch` — ARM32 ldrex/strex primitives (419 lines)
- `lang/libs/atomic/arch/riscv/asm.ch` — RISC-V lr/sc primitives (555 lines)
- `lang/libs/atomic/arch/powerpc/asm.ch` — PowerPC lwarx/stwcx primitives (277 lines)
- `lang/libs/atomic/src/types/atomic_u64.ch` — `atomic_u64` struct with method wrappers
- `lang/libs/atomic/src/types/atomic_u32.ch` — `atomic_u32` struct with method wrappers
- `lang/libs/atomic/src/types/atomic_u16.ch` — `atomic_u16` struct with method wrappers
- `lang/libs/atomic/src/types/atomic_u8.ch` — `atomic_u8` struct with method wrappers
- `lang/libs/atomic/src/preamble_posix.ch` — `@extern` declarations for `__atomic_*_N` symbols (legacy, dead code on C backend)

**Test file:** `lang/tests/libs/atomic/tests.ch` — ~614 tests.

---

## Architecture of the Implementation

### chemical.mod Structure

The module uses conditional source paths to compile only the relevant arch-specific file:

```chmod
module atomic
import std

source "src"
source "arch/x86" if x86_64 or i386
source "arch/aarch64" if aarch64
source "arch/riscv" if riscv
source "arch/arm" if arm
source "arch/powerpc" if powerpc or powerpc64
```

**CRITICAL**: Arch files are in `arch/<arch>/asm.ch` (sibling to `src/`, NOT child). Using `source "src/x86"` would cause duplicate symbols because `source "src"` is recursive and would also load files under `src/x86/`.

### Three-tier design

```
Public API (comptime funcs)
  │
  ├─ LLVM backend path → intrinsics::llvm::atomic_* (LLVM handles everything)
  │
  └─ C backend path → dispatch functions → arch-specific inline asm helpers
```

Every public function (e.g. `atomic_load_u64`) has this structure:

```chemical
public comptime func atomic_load_u64(x : %runtime<*u64>, order : memory_order = memory_order.seq_cst) : u64 {
    comptime if(has_atomic_builtins()) {
        comptime if(intrinsics::get_backend_name() == "C") {
            return %runtime_value(__chx__load_u64_dispatch(x)) as u64
        } else {
            return intrinsics::llvm::atomic_load(x, llvm_mem_order(order), ...) as u64
        }
    } else {
        intrinsics::llvm::atomic_signal_fence(llvm_mem_order(order))
        return intrinsics::llvm::atomic_load(x, 0, ...) as u64
    }
}
```

- `has_atomic_builtins()` returns `intrinsics::supports("atomic")`
- On the C backend (TCCCompiler), `has_atomic_builtins()` is **always true** (the C backend supports atomic builtins)
- On the LLVM backend (Compiler), LLVM intrinsics are used directly — inline asm is never touched

### Dispatch functions

36 dispatch functions route to arch-specific implementations:

```chemical
@retained func __chx__cas_u64_dispatch(ptr, expected, desired) : bool {
    comptime if(def.x86_64 || def.i386) {
        return __chx__cas_u64(ptr, expected, desired)
    } else comptime if(def.aarch64) {
        return __chx__cas_u64_aarch64(ptr, expected, desired)
    } else comptime if(def.arm) {
        return __chx__cas_u64_arm(ptr, expected, desired)
    } else comptime if(def.riscv64) {
        return __chx__cas_u64_riscv(ptr, expected, desired)
    } else comptime if(def.powerpc64) {
        return __chx__cas_u64_ppc(ptr, expected, desired)
    } else {
        return false  // ← SILENT STUB for unsupported archs
    }
}
```

**CRITICAL**: The `else { return false/0/val }` fallback means unsupported architectures silently return wrong values. No error, no panic.

### Function counts per architecture

| Architecture | Inline asm functions | u64 | u32 | u16 | u8 | Notes |
|-------------|---------------------|-----|-----|-----|----|-------|
| x86_64/i386 | 32 | ✅ native | ✅ native | ✅ native | ✅ native | `lock cmpxchg`, `xchg`, `lock xadd` |
| AArch64 | 32 | ✅ native | ✅ native | ✅ CAS loop | ✅ CAS loop | `ldxr/stxr`, `ldar/stlr` |
| ARM32 | 28 | ✅ `ldrexd/strexd` | ✅ `ldrex/strex` | ✅ CAS loop | ✅ CAS loop | |
| RISC-V 64 | 38 | ✅ `lr.d/sc.d` | ✅ `lr.w/sc.w` | ✅ CAS loop | ✅ CAS loop | |
| RISC-V 32 | — | ❌ | ✅ `lr.w/sc.w` | ❌ | ❌ | u16/u8 not implemented |
| PowerPC 64 | 18 | ✅ `ldarx/stdcx.` | ✅ `lwarx/stwcx.` | ❌ stub | ❌ stub | u16/u8 return wrong values |
| PowerPC 32 | — | ❌ | ✅ `lwarx/stwcx.` | ❌ | ❌ | u64/u16/u8 not implemented |

### Inline asm helper naming convention

```
__chx__<operation>_<size>_<arch_suffix>

Examples:
__chx__cas_u64              → x86_64/i386 (no suffix = x86)
__chx__cas_u64_aarch64      → AArch64
__chx__cas_u64_arm          → ARM32
__chx__cas_u64_riscv        → RISC-V 64
__chx__cas_u64_ppc          → PowerPC 64
```

### `@retained` + `%runtime_value(...)` pattern

All inline asm helpers are `@retained` regular functions (not comptime). They are called from comptime funcs via `%runtime_value(func(...))`:

```chemical
public comptime func atomic_load_u64(x, order) : u64 {
    comptime if(intrinsics::get_backend_name() == "C") {
        return %runtime_value(__chx__load_u64_dispatch(x)) as u64
    }
}
```

**Parser requirement**: `return` is required before `%runtime_value(...)`. Void comptime funcs cannot emit code because discarding RuntimeValue produces empty C output.

---

## Per-Architecture Inline Asm Details

### x86_64 / i386

**CAS** (`__chx__cas_u64`):
```asm
lock cmpxchgq %3, %2    # or cmpxchgl/w/b for u32/u16/u8
sete %1                  # set success flag
```
- Uses `lock` prefix for atomicity
- `cmpxchg` compares `eax` with `*ptr`, swaps if equal
- `sete` captures success/failure

**Exchange** (`__chx__exchange_u64`):
```asm
xchgq %0, %1
```
- `xchg` with memory operand is implicitly locked on x86
- Optimal single-instruction exchange

**Fetch-add/sub** (`__chx__fetch_add_u64`):
```asm
lock xaddq %0, %1
```
- `lock xadd` is the optimal atomic fetch-add on x86
- Sub is implemented as `lock xadd` with negated operand

**Fetch-and/or/xor** — CAS loop (no single instruction on x86):
```asm
movq %1, %0              # load current value
# loop:
lock cmpxchgq %3, %2    # try CAS
sete %1
if(!success) goto loop
```

**Load** (`__chx__load_u64`):
```asm
movq %1, %0
```
- Plain `mov` + `memory` clobber is sufficient on x86 (TSO memory model)

**Store** (`__chx__store_u64`):
```asm
movq %1, %0
```
- Same — x86 stores are naturally ordered

**Fence**:
```asm
mfence              # seq_cst (order == 7)
"" ::: "memory"     # acq/rel (order >= 4, compiler barrier only)
```

**All sizes u64/u32/u16/u8 have native x86 implementations.**

### AArch64

**CAS** (`__chx__cas_u64_aarch64`):
```asm
dmb ish             # full barrier before
1: ldxr %0, %2      # load exclusive
   cmp %0, %4       # compare with expected
   b.ne 2f          # mismatch → fail
   stxr %w1, %3, %2 # store exclusive
   cbnz %w1, 1b     # store failed → retry
2:
dmb ish             # full barrier after
```

**CAS for u16/u8** — Aligned word CAS loop:
- Compute `byte_off = ptr & 3`, `shift = byte_off * 8`
- Load aligned u32 with `ldxr/stxr`
- Extract/insert the sub-word value using shifts and masks
- Retry on mismatch or store failure

**Exchange** — `ldxr/stxr` loop (for u64/u32), CAS loop (for u16/u8)

**Fetch-add/sub/and/or/xor** — `ldxr + <op> + stxr` loop

**Load** — `ldar` (load-acquire) with `dmb ish` barriers

**Store** — `stlr` (store-release) with `dmb ish` barriers

**Fence**:
```asm
dmb ish             # seq_cst
"" ::: "memory"     # acq/rel
```

### ARM32

**CAS for u64** — `ldrexd/strexd` (double-word exclusive):
```asm
dmb ish
1: ldrexd %0, %H0, %2     # load exclusive double
   cmp %0, %4              # compare low word
   cmpne %H0, %H4          # compare high word
   bne 2f
   strexd %1, %5, %H5, %2  # store exclusive double
   cmp %1, #0
   bne 1b
2:
dmb ish
```

**CAS for u32** — `ldrex/strex`

**CAS for u16/u8** — Same aligned word CAS loop as AArch64

**All other operations** — CAS loops through the CAS primitives

**Fence**:
```asm
dmb ish             # seq_cst
"" ::: "memory"     # acq/rel
```

### RISC-V 64

**CAS for u64** (`__chx__cas_u64_riscv`):
```asm
1: lr.d %0, %3          # load reserved
   bne %0, %4, 2f        # mismatch → fail
   sc.d %1, %5, %3       # store conditional
   bnez %1, 1b            # store failed → retry
2:
```

**CAS for u32** — `lr.w/sc.w`

**CAS for u16/u8** — Aligned word CAS loop (same technique as AArch64)

**Store** — `amoswap.d.rl` (atomic swap with release semantics) preceded by `fence rw, rw`

**Load** — `amoswap.aq` (atomic swap with acquire semantics, swapping with 0)

**All other operations** — CAS loops

**Fence**:
```asm
fence rw, rw        # all orderings >= 4
```

### RISC-V 32

**Only u32 is implemented** — `lr.w/sc.w` based CAS, CAS loops for everything else.

**u16/u8 are NOT implemented** — dispatch functions return `false`/`0`.

### PowerPC 64

**CAS for u64** (`__chx__cas_u64_ppc`):
```asm
1: ldarx %0, 0, %3       # load reserved double
   cmpd %0, %4            # compare
   bne 2f
   stdcx. %5, 0, %3       # store conditional double
   bne- 1b                # store failed → retry
2:
mfcr %1                   # read condition register
rwlnm %1, %1, 0, 1, 0    # extract CR0.LT bit (success flag)
```

**CAS for u32** — `lwarx/stwcx.` with `sync` before and `isync` after

**All other operations** — CAS loops

**Fence**:
```asm
sync                    # seq_cst
"" ::: "memory"         # acq/rel
```

### PowerPC 32

**Only u32 is implemented** — `lwarx/stwcx.` based.

**u64/u16/u8 are NOT implemented** — dispatch functions return `false`/`0`.

### WASM / s390x / MIPS

**NOT IMPLEMENTED.** All dispatch functions return `false`/`0`/`val`. Operations silently produce wrong results.

### atomic_flag

`atomic_flag` is implemented using CAS (compare-and-swap) on a `u32` value:

```chemical
@retained public func atomic_flag_test_and_set(ptr : *mut atomic_flag) : bool {
    var expected : u32 = 0
    var desired : u32 = 1
    var result = __chx__cas_u32_dispatch(ptr as *mut u32, &raw mut expected, desired)
    return expected == 0  // returns previous value (C11 spec)
}

@retained public func atomic_flag_clear(ptr : *mut atomic_flag) {
    var zero : u32 = 0
    __chx__store_u32_dispatch(ptr as *mut u32, zero)
    __chx__fence(7)  // seq_cst
}
```

**Bug fix (fixed during this session)**: `atomic_flag_test_and_set` originally returned `true` on CAS success (was 0→1) instead of the **previous** value. Per C11 spec, `atomic_flag_test_and_set` must return the value **before** the swap: `false` if the flag was clear (now set), `true` if it was already set. The fix inverted the return logic.

---

## Memory Ordering

### Current implementation

All operations always emit full barriers (seq_cst equivalent):
- x86: `lock` prefix (implicit full barrier on RMW), `mfence` for fence
- ARM: `dmb ish` before and after every operation
- RISC-V: `fence rw, rw` for everything
- PowerPC: `sync` for seq_cst, compiler barrier for acq/rel

### What this means

- **Correctness**: Always correct — full barriers are a superset of all weaker orderings
- **Performance**: Sub-optimal on weakly-ordered architectures (ARM, RISC-V, PowerPC) where relaxed/acquire/release could skip some barriers
- **On x86**: No performance impact — x86's TSO model means all RMW ops are already seq_cst

### `memory_order` enum values

```chemical
public enum memory_order : int {
    relaxed,    // 0
    consume,    // 1
    acquire,    // 2
    release,    // 3
    acq_rel,    // 4
    seq_cst     // 5
};
```

These are mapped to LLVM memory orderings:
```
relaxed  → monotonic (2)
consume  → acquire (4)
acquire  → acquire (4)
release  → release (5)
acq_rel  → acquire_release (6)
seq_cst  → sequentially_consistent (7)
```

### Fence implementation

`__chx__fence(order)` uses the LLVM integer values (0-7):
- `order == 7` (seq_cst): hardware fence (`mfence`/`dmb ish`/`fence rw,rw`/`sync`)
- `order >= 4` (acquire/release/acq_rel): compiler barrier (`asm("" ::: "memory")`)
- `order < 4` (relaxed/consume): nothing

`__chx__signal_fence(order)`: always compiler barrier for `order >= 4`, nothing otherwise.

---

## Known Issues and Gaps

### Critical (must fix before production on non-x86)

1. **Silent stubs on unsupported architectures** — wasm, s390x, MIPS, RISC-V 32 (u16/u8), PowerPC 32 (u64/u16/u8) all return 0/false. Users get wrong results with no error.

2. **Memory ordering not honored** — All operations always use full barriers. This is correct but slower than necessary on ARM/RISC-V/PowerPC for relaxed/acquire/release orderings.

3. **No cross-compilation testing** — arm32, aarch64, RISC-V, PowerPC asm has never been tested on real hardware.

### Medium

4. **RISC-V 32 missing u16/u8** — Dispatch functions return `false`/`0`.

5. **PowerPC 32 missing u64/u16/u8** — Dispatch functions return `false`/`0`.

6. **preamble_posix.ch is dead code** — Still declares `@extern` for libatomic symbols that are never called on the C backend path.

### Low

7. **No `atomic_fetch_nand`** — Old runtime had this. Rarely needed.

8. **No `atomic_is_lock_free`** — Not critical.

---

## Testing Guide

### Running tests

```bash
# Full test suite (main + lib tests):
./scripts/test.sh --tcc

# Library tests only (includes atomic):
./scripts/test.sh --tcc --libs

# Build only (no test run):
./scripts/build.sh --tcc
```

### Test structure

- `lang/tests/libs/atomic/tests.ch` — 614 tests organized in sections:
  - Fence tests (5 orderings)
  - u64 tests (load/store, exchange, fetch_add/sub, bitwise, CAS strong/weak, overflow)
  - u32 tests (same)
  - u16 tests (same)
  - u8 tests (same)
  - @volatile tests
  - Concurrency stress tests (6 tests with 2 threads × 10000 iterations)
  - Memory ordering tests (relaxed/acquire/release/acq_rel for all sizes)
  - Edge case tests (boundary values, underflow wrap, identity exchange, spurious CAS, zero-operand)
  - atomic_flag tests (already-set, clear-and-retest)

### What to test on Windows

1. **Build succeeds** — `./scripts/build.sh --tcc` should complete without errors
2. **All 614 lib tests pass** — `./scripts/test.sh --tcc --libs`
3. **All 2126 main tests pass** — `./scripts/test.sh --tcc`
4. **x86_64 inline asm works** — The tests exercise all inline asm paths
5. **Concurrency stress tests pass** — These test thread-safety

### What to test on CI (multi-arch)

For each target architecture:
1. **Build succeeds** — Cross-compile with the appropriate target triple
2. **Tests pass** — Run on target hardware or QEMU
3. **No silent failures** — Check that unsupported arch stubs don't produce wrong results

### Architecture matrix for CI

| Target | Cross-compile triple | QEMU | Notes |
|--------|---------------------|------|-------|
| x86_64 Linux | `x86_64-linux-gnu` | Not needed | Primary target |
| i386 Linux | `i686-linux-gnu` | Not needed | 32-bit x86 |
| AArch64 Linux | `aarch64-linux-gnu` | `qemu-aarch64` | 64-bit ARM |
| ARM32 Linux | `arm-linux-gnueabihf` | `qemu-arm` | 32-bit ARM |
| RISC-V 64 Linux | `riscv64-linux-gnu` | `qemu-riscv64` | 64-bit RISC-V |
| RISC-V 32 Linux | `riscv32-linux-gnu` | `qemu-riscv32` | 32-bit RISC-V (partial) |
| PowerPC 64 Linux | `powerpc64le-linux-gnu` | `qemu-ppc64le` | Little-endian PPC64 |
| PowerPC 32 Linux | `powerpc-linux-gnu` | `qemu-ppc` | 32-bit PPC |
| Windows x86_64 | MSVC or MinGW | Not needed | Primary Windows target |
| macOS AArch64 | Native (Apple Silicon) | Not needed | Apple Silicon Mac |

### Common failure patterns

| Symptom | Likely cause |
|---------|-------------|
| Test returns 0/false unexpectedly | Dispatch function fell through to silent stub (wrong `def.*` check) |
| CAS always fails | Inline asm clobber list missing `"cc"` or `"memory"` |
| Wrong value after exchange | Inline asm constraint mismatch (operand numbering) |
| Segfault in concurrency test | Missing `memory` clobber allows compiler to reorder |
| Test passes in single-threaded but fails multi-threaded | Missing barrier in CAS loop or exchange |
| `asm` syntax error on build | TCC doesn't support a specific asm construct (check constraint syntax) |
| Undefined symbol `__atomic_load_8` etc. | preamble_posix.ch being compiled when it shouldn't be |

### Debugging tips

1. **Add debug output to dispatch functions** — Temporarily add `printf` to see which arch path is taken:
   ```chemical
   @retained func __chx__cas_u64_dispatch(ptr, expected, desired) : bool {
       comptime if(def.x86_64 || def.i386) {
           printf("x86 CAS\n")
           return __chx__cas_u64(ptr, expected, desired)
       } ...
   }
   ```

2. **Check which `def.*` is true** — The compiler sets exactly one arch flag. Use `printf` or compile-time `comptime if` to verify.

3. **Inspect generated C code** — Use `--emit-c` flag to see the C translation output and verify inline asm appears correctly:
   ```bash
   cmake-build-debug/TCCCompiler lang/compiled/temp.ch -o temp.c --emit-c -v
   ```

4. **Check for compiler warnings** — GCC/Clang warnings about asm constraints indicate problems.

5. **Test with `-bt` flag for backtrace** — If a test crashes:
   ```bash
   ./scripts/test.sh --tcc --libs -bt
   ```

---

## Writing New Atomic Operations

To add a new operation (e.g. `atomic_max_u64`):

1. **Add inline asm helpers** for each arch (x86, aarch64, arm32, riscv, ppc)
2. **Add dispatch function** `__chx__max_u64_dispatch` with all arch branches
3. **Add public comptime func** `atomic_max_u64` with the three-tier backend dispatch
4. **Add to all 36 dispatch functions** if adding for all sizes
5. **Add tests** — single-threaded correctness + concurrency stress test

### Adding a new architecture

1. Create `lang/libs/atomic/arch/<arch>/asm.ch` with all inline asm helpers
2. Add `source "arch/<arch>"` conditional in `lang/libs/atomic/chemical.mod`
3. Add `def.<arch>` checks to all 36 dispatch functions in `src/atomic.ch`
4. Add fence implementation in `__chx__fence` and `__chx__signal_fence`
5. Add the arch to `TargetData.h` and `declare_def_values()` in the C++ compiler
6. Test on real hardware (or QEMU for cross-compiled targets)

---

## C Backend Codegen

The C codegen backend (`preprocess/2c/2cASTVisitor.cpp`) stubs out all atomic methods:

```cpp
Value* ToCBackendContext::atomic_load(...) {
    visitor->error("atomic_load is unsupported on C backend (use %runtime_value() inline asm)", ptr);
}
```

This means the inline asm approach is the **only** code path for the C backend. The old `ensure_atomic_preamble()` and atomic method implementations have been removed.

**Note on `@volatile`**: The `@volatile` annotation is supported on `var` and `const` declarations. On the C backend, it emits `volatile` in the generated C variable declarations, preventing the compiler from optimizing away or reordering loads/stores to the variable.

---

## LLVM Backend

The LLVM backend (`compiler/backend/LLVM.cpp`) uses LLVM's own atomic intrinsics directly. It never touches the inline asm. The public comptime functions route to `intrinsics::llvm::atomic_load/store/op/cmp_exch_*` on the LLVM backend path.

---

## Concurrency Stress Tests

6 tests verify thread-safety:

| Test | What it does |
|------|-------------|
| `concurrent_fetch_add_u64` | 2 threads × 10000 increments → counter == 20000 |
| `concurrent_fetch_add_u32` | 2 threads × 10000 increments → counter == 20000 |
| `concurrent_exchange_u64` | 2 threads × 10000 exchanges → val is in valid range |
| `concurrent_cas_u64` | 2 threads × 10000 CAS retries → counter <= 20000 |
| `concurrent_fetch_sub_u64` | 2 threads × 10000 decrements from 20000 → counter == 0 |
| `concurrent_atomic_flag` | 2 threads × 10000 set/clear cycles → flag is clear |

Uses `std::concurrent::spawn` and `thread.join()`.
