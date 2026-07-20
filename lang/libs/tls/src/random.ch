// ============================================================================
// Cryptographic Secure Random Number Generator
// ============================================================================
// Reads cryptographically secure random bytes from /dev/urandom.
// Uses C standard library file I/O (portable across TCC and LLVM backends).
// ============================================================================

public namespace tls {

    // ─── Secure Random Fill ────────────────────────────────────────────────
    // Fill a buffer with cryptographically secure random bytes from /dev/urandom.
    // Returns 0 on success, ERR_SSL_NO_RNG on failure.
    public func random_fill(buf : *mut u8, len : size_t) : int {
        var path = "/dev/urandom\0" as *char
        var mode = "rb\0" as *char
        var f = fopen(path, mode)
        if(f == null) { return ERR_SSL_NO_RNG }

        var remaining = len
        var offset : size_t = 0
        while(remaining > 0) {
            var n = fread(buf + offset, 1 as size_t, remaining, f)
            if(n <= 0) {
                fclose(f)
                return ERR_SSL_NO_RNG
            }
            offset += n
            remaining -= n
        }

        fclose(f)
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
    public func random_32bit() : u32 {
        var val : u32 = 0
        var ret = random_fill(&raw mut val as *mut u8, 4)
        if(ret < 0) {
            return 0xDEADBEEFu32
        }
        return val
    }

} // namespace tls
