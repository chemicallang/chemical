// Atomic preamble for posix platforms.
// Provides @extern declarations for __atomic_*_N symbols.
// These are resolved from libatomic (linked via -latomic by the build system).
//
// This file is compiled as part of the atomic module (via chemical_dir_module).
// It replaces the C++-emitted C preamble in ToCBackendContext::ensure_atomic_preamble().

// Fence cell — used by atomic_fence synthesis when compiling with TCC.
// The C backend's atomic_fence method accesses this via __atomic_exchange_1.
// Marked volatile so the C compiler doesn't optimize away accesses.
@volatile
var __chx__fence_cell : u8 = 0

// ---- 64-bit operations ----

@extern
protected func __atomic_load_8(x : *u64, mo : int) : u64
@extern
protected func __atomic_store_8(x : *mut u64, y : u64, mo : int)
@extern
protected func __atomic_exchange_8(x : *mut u64, y : u64, mo : int) : u64
@extern
protected func __atomic_compare_exchange_8(x : *mut u64, expected : *u64, y : u64, mo : int, mo2 : int) : bool
@extern
protected func __atomic_fetch_add_8(x : *mut u64, y : u64, mo : int) : u64
@extern
protected func __atomic_fetch_sub_8(x : *mut u64, y : u64, mo : int) : u64
@extern
protected func __atomic_fetch_and_8(x : *mut u64, y : u64, mo : int) : u64
@extern
protected func __atomic_fetch_or_8(x : *mut u64, y : u64, mo : int) : u64
@extern
protected func __atomic_fetch_xor_8(x : *mut u64, y : u64, mo : int) : u64
@extern
protected func __atomic_fetch_nand_8(x : *mut u64, y : u64, mo : int) : u64

// ---- 32-bit operations ----

@extern
protected func __atomic_load_4(x : *u32, mo : int) : u32
@extern
protected func __atomic_store_4(x : *mut u32, y : u32, mo : int)
@extern
protected func __atomic_exchange_4(x : *mut u32, y : u32, mo : int) : u32
@extern
protected func __atomic_compare_exchange_4(x : *mut u32, expected : *u32, y : u32, mo : int, mo2 : int) : bool
@extern
protected func __atomic_fetch_add_4(x : *mut u32, y : u32, mo : int) : u32
@extern
protected func __atomic_fetch_sub_4(x : *mut u32, y : u32, mo : int) : u32
@extern
protected func __atomic_fetch_and_4(x : *mut u32, y : u32, mo : int) : u32
@extern
protected func __atomic_fetch_or_4(x : *mut u32, y : u32, mo : int) : u32
@extern
protected func __atomic_fetch_xor_4(x : *mut u32, y : u32, mo : int) : u32
@extern
protected func __atomic_fetch_nand_4(x : *mut u32, y : u32, mo : int) : u32

// ---- 16-bit operations ----

@extern
protected func __atomic_load_2(x : *u16, mo : int) : u16
@extern
protected func __atomic_store_2(x : *mut u16, y : u16, mo : int)
@extern
protected func __atomic_exchange_2(x : *mut u16, y : u16, mo : int) : u16
@extern
protected func __atomic_compare_exchange_2(x : *mut u16, expected : *u16, y : u16, mo : int, mo2 : int) : bool
@extern
protected func __atomic_fetch_add_2(x : *mut u16, y : u16, mo : int) : u16
@extern
protected func __atomic_fetch_sub_2(x : *mut u16, y : u16, mo : int) : u16
@extern
protected func __atomic_fetch_and_2(x : *mut u16, y : u16, mo : int) : u16
@extern
protected func __atomic_fetch_or_2(x : *mut u16, y : u16, mo : int) : u16
@extern
protected func __atomic_fetch_xor_2(x : *mut u16, y : u16, mo : int) : u16
@extern
protected func __atomic_fetch_nand_2(x : *mut u16, y : u16, mo : int) : u16

// ---- 8-bit operations ----

@extern
protected func __atomic_load_1(x : *u8, mo : int) : u8
@extern
protected func __atomic_store_1(x : *mut u8, y : u8, mo : int)
@extern
protected func __atomic_exchange_1(x : *mut u8, y : u8, mo : int) : u8
@extern
protected func __atomic_compare_exchange_1(x : *mut u8, expected : *u8, y : u8, mo : int, mo2 : int) : bool
@extern
protected func __atomic_fetch_add_1(x : *mut u8, y : u8, mo : int) : u8
@extern
protected func __atomic_fetch_sub_1(x : *mut u8, y : u8, mo : int) : u8
@extern
protected func __atomic_fetch_and_1(x : *mut u8, y : u8, mo : int) : u8
@extern
protected func __atomic_fetch_or_1(x : *mut u8, y : u8, mo : int) : u8
@extern
protected func __atomic_fetch_xor_1(x : *mut u8, y : u8, mo : int) : u8
@extern
protected func __atomic_fetch_nand_1(x : *mut u8, y : u8, mo : int) : u8
