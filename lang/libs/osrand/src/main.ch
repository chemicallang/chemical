// osrand — Shared random number generator for the Chemical ecosystem.
// Provides cryptographically secure random bytes from the OS entropy source.
// Windows: BCryptGenRandom (system-preferred RNG).
// POSIX: /dev/urandom.

public namespace osrand {

    if(def.windows) {
        @extern public func BCryptGenRandom(hAlgorithm : uintptr_t, pbBuffer : *mut u8, cbBuffer : u32, dwFlags : u32) : long
        const BCRYPT_USE_SYSTEM_PREFERRED_RNG : u32 = 0x00000002
    }

    // Fill a buffer with cryptographically secure random bytes.
    // Returns 0 on success, -1 on failure.
    public func random_fill(buf : *mut u8, len : size_t) : int {
        if(len == 0) { return 0 }
        comptime if(def.windows) {
            var ret = BCryptGenRandom(0 as uintptr_t, buf, len as u32, BCRYPT_USE_SYSTEM_PREFERRED_RNG)
            if(ret != 0) { return -1 }
            return 0
        } else {
            var path = "/dev/urandom\0" as *char
            var mode = "rb\0" as *char
            var f = fopen(path, mode)
            if(f == null) { return -1 }

            var remaining = len
            var offset : size_t = 0
            while(remaining > 0) {
                var n = fread(buf + offset, 1 as size_t, remaining, f)
                if(n <= 0) {
                    fclose(f)
                    return -1
                }
                offset += n
                remaining -= n
            }

            fclose(f)
            return 0
        }
    }

    // Generate a single random u32.
    public func random_u32() : u32 {
        var val : u32 = 0
        var ret = random_fill(&raw mut val as *mut u8, 4)
        if(ret < 0) { return 0xDEADBEEFu32 }
        return val
    }

    // Generate a random u64.
    public func random_u64() : u64 {
        var val : u64 = 0
        var ret = random_fill(&raw mut val as *mut u8, 8)
        if(ret < 0) { return 0xDEADBEEFDEADBEEFu64 }
        return val
    }

} // end namespace osrand
