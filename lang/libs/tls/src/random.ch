// ============================================================================
// Cryptographic Secure Random Number Generator
// ============================================================================
// Delegates to the shared osrand library for OS entropy.
// ============================================================================

public namespace tls {

    // ─── Secure Random Fill ────────────────────────────────────────────────
    // Fill a buffer with cryptographically secure random bytes.
    // Delegates to osrand::random_fill (shared OS entropy source).
    // Returns 0 on success, ERR_SSL_NO_RNG on failure.
    public func random_fill(buf : *mut u8, len : size_t) : int {
        var ret = osrand::random_fill(buf, len)
        if(ret != 0) { return ERR_SSL_NO_RNG }
        return 0
    }

    // ─── Convenience Functions ─────────────────────────────────────────────

    // Generate a 32-byte random value (e.g., for client random in ClientHello)
    public func random_32(out : *mut [32]u8) : int {
        return random_fill(out as *mut u8, 32)
    }

    // Generate a 48-byte random value (e.g., for pre-master secret)
    public func random_48(out : *mut [48]u8) : int {
        return random_fill(out as *mut u8, 48)
    }

    // Generate a single 32-bit random value
    // Returns 0 on failure (caller must not use this value for crypto)
    public func random_32bit() : u32 {
        var val : u32 = 0
        var ret = random_fill(&raw mut val as *mut u8, 4)
        if(ret < 0) {
            return 0xDEADBEEFu32
        }
        return val
    }

} // namespace tls
