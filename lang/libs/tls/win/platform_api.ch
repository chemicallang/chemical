public namespace tls {

    // BCryptGenRandom — cryptographically secure random bytes on Windows.
    // Declared with BCRYPT_USE_SYSTEM_PREFERRED_RNG (dwFlags = 0x2) and a NULL
    // algorithm handle, which is the documented way to get OS-seeded randomness.
    @dllimport @stdcall @extern protected func BCryptGenRandom(hAlgorithm: uintptr_t, pbBuffer: *mut u8, cbBuffer: u32, dwFlags: u32): int;

    comptime const BCRYPT_USE_SYSTEM_PREFERRED_RNG : u32 = 0x00000002u

}
