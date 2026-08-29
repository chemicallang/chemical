public namespace bcrypt {

public func generate_salt(cost : int) : std::string {
    var entropy : [16]u8
    if(!get_random_bytes(&raw mut entropy[0], 16u)) return std::string()
    
    var res = std::string()
    res.append_view("$2b$")
    if(cost < 10) res.append('0')
    res.append_expr(`${cost as uint}`)
    res.append('$')
    
    // Entropy needs to be encoded
    var entropy_words : [4]uint
    memcpy(&raw mut entropy_words[0], &raw entropy[0], 16)
    // bcrypt expects Big Endian entropy usually, but crypt_blowfish does BF_swap on it
    // Let's just encode it using our bf_encode
    bf_encode(&mut res, entropy_words, 16)
    
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
    return new_hash.to_view().equals(&hash)
}

}
