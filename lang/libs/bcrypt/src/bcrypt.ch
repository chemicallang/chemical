public namespace bcrypt {

// Win32 Externs for Randomness
@extern func CryptAcquireContextA(phProv : *mut usize, pszContainer : *char, pszProvider : *char, dwProvType : uint, dwFlags : uint) : bool
@extern func CryptGenRandom(hProv : usize, dwLen : uint, pbBuffer : *mut u8) : bool
@extern func CryptReleaseContext(hProv : usize, dwFlags : uint) : bool

const PROV_RSA_FULL = 1u
const CRYPT_VERIFYCONTEXT = 0xF0000000u

func get_random_bytes(buf : *mut u8, len : uint) : bool {
    var hProv : usize = 0u
    if(!CryptAcquireContextA(&mut hProv, null, null, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) return false
    var ok = CryptGenRandom(hProv, len, buf)
    CryptReleaseContext(hProv, 0u)
    return ok
}

public func generate_salt(cost : int) : std::string {
    var entropy : [16]u8
    if(!get_random_bytes(&mut entropy[0], 16u)) return std::string()
    
    var res = std::string()
    res.append_view("$2b$")
    if(cost < 10) res.append('0')
    res.append_expr(`${cost as uint}`)
    res.append('$')
    
    // Entropy needs to be encoded
    var entropy_words : [4]uint
    memcpy(&mut entropy_words[0], &entropy[0], 16)
    // bcrypt expects Big Endian entropy usually, but crypt_blowfish does BF_swap on it
    // Let's just encode it using our bf_encode
    bf_encode(res, entropy_words, 16)
    
    return res
}

public func hash_password(password : std::string_view) : std::string {
    var salt = generate_salt(12)
    if(salt.empty()) return std::string()
    return bf_crypt(password, salt.to_view())
}

public func check_password(password : std::string_view, hash : std::string_view) : bool {
    if(hash.size() < 29u) return false
    var setting = hash.subview(0u, 29u)
    var new_hash = bf_crypt(password, setting)
    return new_hash.to_view().equals(hash)
}

}
