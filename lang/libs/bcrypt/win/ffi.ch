public namespace bcrypt {

    const PROV_RSA_FULL = 1u
    const CRYPT_VERIFYCONTEXT = 0xF0000000u

    // Win32 Externs for Randomness
    @extern public func CryptAcquireContextA(phProv : *mut usize, pszContainer : *char, pszProvider : *char, dwProvType : uint, dwFlags : uint) : bool
    @extern public func CryptGenRandom(hProv : usize, dwLen : uint, pbBuffer : *mut u8) : bool
    @extern public func CryptReleaseContext(hProv : usize, dwFlags : uint) : bool

    public func get_random_bytes(buf : *mut u8, len : uint) : bool {
        var hProv : usize = 0u
        if(!CryptAcquireContextA(&raw mut hProv, null, null, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) return false
        var ok = CryptGenRandom(hProv, len, buf)
        CryptReleaseContext(hProv, 0u)
        return ok
    }

}