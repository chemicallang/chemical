/**
 * wasm32 base typedefs.
 *
 * wasm32 is LLP32: pointers, long, size_t and uintptr_t are all 32-bit. The
 * shared cstd/src layer already derives size_t from the data model; these are
 * the POSIX-shaped aliases the rest of the library expects.
 */

public type usize = ulong

public type INT_PTR = int
public type PINT_PTR = *mut INT_PTR

public type UINT_PTR = uint
public type PUINT_PTR = *mut UINT_PTR

public type LONG_PTR = long
public type PLONG_PTR = *mut LONG_PTR

public type ULONG_PTR = ulong
public type PULONG_PTR = *mut ULONG_PTR

public type UINT = uint
