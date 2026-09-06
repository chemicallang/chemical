using namespace std;
using namespace bcrypt;

// ---- internal helpers ----

func is_valid_bcrypt_base64(c : char) : bool {
    if(c == '.') return true
    if(c == '/') return true
    if(c >= 'A' && c <= 'Z') return true
    if(c >= 'a' && c <= 'z') return true
    if(c >= '0' && c <= '9') return true
    return false
}

// ---- generate_salt tests ----

@test
func test_bcrypt_generate_salt_format(env : &mut TestEnv) {
    var salt = bcrypt::generate_salt(12)
    if(salt.empty()) {
        env.error("generate_salt returned empty salt")
        return
    }
    expect_uint_eq(env, salt.size(), 29u, "salt should be 29 chars")
    if(salt.get(0) != '$') { env.error("salt[0] should be '$'") }
    if(salt.get(1) != '2') { env.error("salt[1] should be '2'") }
    if(salt.get(2) != 'b') { env.error("salt[2] should be 'b' for $2b$") }
    if(salt.get(3) != '$') { env.error("salt[3] should be '$'") }
    if(salt.get(4) != '1') { env.error("salt[4] should be '1' for cost 12") }
    if(salt.get(5) != '2') { env.error("salt[5] should be '2' for cost 12") }
    if(salt.get(6) != '$') { env.error("salt[6] should be '$' separator") }
    for(var i = 7u; i < salt.size(); i++) {
        if(!is_valid_bcrypt_base64(salt.get(i))) {
            env.error("invalid base64 char in salt encoding")
            break
        }
    }
}

@test
func test_bcrypt_generate_salt_cost_variations(env : &mut TestEnv) {
    // Cost below 10 should be zero-padded
    var salt4 = bcrypt::generate_salt(4)
    if(salt4.empty()) { env.error("salt(4) failed"); return }
    expect_uint_eq(env, salt4.size(), 29u, "salt(4) should be 29 chars")
    if(salt4.get(4) != '0' || salt4.get(5) != '4') { env.error("cost 4 should be encoded as '04'") }

    // Cost exactly 10
    var salt10 = bcrypt::generate_salt(10)
    if(salt10.empty()) { env.error("salt(10) failed"); return }
    if(salt10.get(4) != '1' || salt10.get(5) != '0') { env.error("cost 10 should be '10'") }

    // Cost at typical value
    var salt8 = bcrypt::generate_salt(8)
    if(salt8.empty()) { env.error("salt(8) failed"); return }
    if(salt8.get(4) != '0' || salt8.get(5) != '8') { env.error("cost 8 should be '08'") }

    // Cost at upper range (just format, not used for hashing)
    var salt31 = bcrypt::generate_salt(31)
    if(salt31.empty()) { env.error("salt(31) failed"); return }
    if(salt31.get(4) != '3' || salt31.get(5) != '1') { env.error("cost 31 should be '31'") }
}

@test
func test_bcrypt_generate_salt_uniqueness(env : &mut TestEnv) {
    var salt1 = bcrypt::generate_salt(12)
    var salt2 = bcrypt::generate_salt(12)
    if(salt1.empty() || salt2.empty()) {
        env.error("salt generation failed")
        return
    }
    var same = true
    for(var i = 7u; i < 29u; i++) {
        if(salt1.get(i) != salt2.get(i)) {
            same = false
            break
        }
    }
    if(same) {
        env.error("two consecutive salt calls produced identical salt portion")
    }
}

// ---- hash_password tests ----

@test
func test_bcrypt_hash_password_format(env : &mut TestEnv) {
    var hash = bcrypt::hash_password(std::string_view("test_verify_format"))
    if(hash.empty()) {
        env.error("hash_password returned empty")
        return
    }
    expect_uint_eq(env, hash.size(), 60u, "full bcrypt hash should be 60 chars")
    if(hash.get(0) != '$' || hash.get(1) != '2' || hash.get(2) != 'b' || hash.get(3) != '$') {
        env.error("hash should start with '$2b$'")
    }
    if(hash.get(6) != '$') { env.error("hash should have '$' after cost digits") }
    for(var i = 7u; i < hash.size(); i++) {
        if(!is_valid_bcrypt_base64(hash.get(i))) {
            env.error("invalid base64 char in hash output")
            break
        }
    }
}

@test
func test_bcrypt_hash_password_various_inputs(env : &mut TestEnv) {
    // Empty password
    var h_empty = bcrypt::hash_password(std::string_view(""))
    if(h_empty.empty()) { env.error("hash of empty pw failed"); return }
    expect_uint_eq(env, h_empty.size(), 60u, "hash of empty pw should be 60 chars")

    // Minimal single-char password
    var h_min = bcrypt::hash_password(std::string_view("a"))
    if(h_min.empty()) { env.error("hash of single char failed"); return }
    expect_uint_eq(env, h_min.size(), 60u, "hash of single char should be 60 chars")

    // Long password (72 chars, bcrypt's key-length limit)
    var long_pw = std::string()
    for(var i = 0u; i < 72u; i++) { long_pw.append('A') }
    var h_long = bcrypt::hash_password(long_pw.to_view())
    if(h_long.empty()) { env.error("hash of 72-char pw failed"); return }
    expect_uint_eq(env, h_long.size(), 60u, "hash of 72-char pw should be 60 chars")

    // Very long password beyond the 72-byte limit
    var very_long = std::string()
    for(var i = 0u; i < 200u; i++) {
        if((i % 2u) == 0u) very_long.append('X') else very_long.append('Y')
    }
    var h_vlong = bcrypt::hash_password(very_long.to_view())
    if(h_vlong.empty()) { env.error("hash of 200-char pw failed"); return }
    expect_uint_eq(env, h_vlong.size(), 60u, "hash of 200-char pw should be 60 chars")

    // Password with special / non-alphanumeric characters
    var special = std::string()
    special.append('!'); special.append('@'); special.append('#'); special.append('$')
    special.append('%'); special.append('^'); special.append('&'); special.append('*')
    special.append('('); special.append(')'); special.append('-'); special.append('_')
    special.append('+'); special.append('='); special.append('{'); special.append('}')
    special.append('['); special.append(']'); special.append('|'); special.append('\\')
    special.append(':'); special.append(';'); special.append('"')
    special.append('<'); special.append('>'); special.append(','); special.append('.')
    special.append('?'); special.append('~'); special.append('`'); special.append(' ')
    var h_special = bcrypt::hash_password(special.to_view())
    if(h_special.empty()) { env.error("hash of special chars failed"); return }
    expect_uint_eq(env, h_special.size(), 60u, "hash of special chars should be 60 chars")

    // Numeric-only password
    var numeric = std::string()
    for(var i = 0u; i < 30u; i++) { numeric.append('0' + ((i % 10u) as char)) }
    var h_num = bcrypt::hash_password(numeric.to_view())
    if(h_num.empty()) { env.error("hash of numeric pw failed"); return }
    expect_uint_eq(env, h_num.size(), 60u, "hash of numeric pw should be 60 chars")
}

@test
func test_bcrypt_hash_password_uniqueness(env : &mut TestEnv) {
    var h1 = bcrypt::hash_password(std::string_view("same_password"))
    var h2 = bcrypt::hash_password(std::string_view("same_password"))
    if(h1.empty() || h2.empty()) {
        env.error("hash_password failed")
        return
    }
    // Each call uses a fresh random salt, so outputs must differ
    if(h1.to_view().equals(h2.to_view())) {
        env.error("two hashes of the same password must differ (unique salt)")
    }
}

// ---- check_password tests ----

@test
func test_bcrypt_check_password_correct(env : &mut TestEnv) {
    var hash = bcrypt::hash_password(std::string_view("correct_password"))
    if(hash.empty()) { env.error("hash_password failed"); return }
    if(!bcrypt::check_password(std::string_view("correct_password"), hash.to_view())) {
        env.error("check_password must return true for the correct password")
    }
}

@test
func test_bcrypt_check_password_incorrect(env : &mut TestEnv) {
    var hash = bcrypt::hash_password(std::string_view("secret"))
    if(hash.empty()) { env.error("hash_password failed"); return }

    // Completely different password
    if(bcrypt::check_password(std::string_view("wrong"), hash.to_view())) {
        env.error("check_password must reject a completely different password")
    }
    // Case difference
    if(bcrypt::check_password(std::string_view("Secret"), hash.to_view())) {
        env.error("check_password must reject case-different password")
    }
    // Prefix of the real password
    if(bcrypt::check_password(std::string_view("sec"), hash.to_view())) {
        env.error("check_password must reject a prefix of the real password")
    }
    // Real password with extra trailing chars
    if(bcrypt::check_password(std::string_view("secrets"), hash.to_view())) {
        env.error("check_password must reject real password with extra chars")
    }
    // Empty string
    if(bcrypt::check_password(std::string_view(""), hash.to_view())) {
        env.error("check_password must reject empty password against real hash")
    }
    // Null-like byte string
    if(bcrypt::check_password(std::string_view("\0wrong"), hash.to_view())) {
        env.error("check_password must reject password with leading null")
    }
}

@test
func test_bcrypt_check_password_edge_cases(env : &mut TestEnv) {
    // Empty hash
    if(bcrypt::check_password(std::string_view("password"), std::string_view(""))) {
        env.error("check_password must return false for empty hash")
    }
    // Short hash (<29 chars triggers early size guard)
    if(bcrypt::check_password(std::string_view("password"), std::string_view("short"))) {
        env.error("check_password must return false for hash < 29 chars")
    }
    // Hash with invalid prefix ($2x$ is not $2b$)
    var invalid_prefix = std::string::make_no_len("$2x$04$abcdefghijklmnopqrstuvabcdefghijklmnopqrstuvabcdefghijklm")
    if(bcrypt::check_password(std::string_view("password"), invalid_prefix.to_view())) {
        env.error("check_password must return false for hash with $2x$ prefix")
    }
    // Malformed hash that starts with $ but has no '2'
    var malformed = std::string::make_no_len("$3b$04$abcdefghijklmnopqrstuvabcdefghijklmnopqrstuvabcdefghijklm")
    if(bcrypt::check_password(std::string_view("password"), malformed.to_view())) {
        env.error("check_password must return false when hash[1] != '2'")
    }
    // Very long junk input (must not crash)
    var junk = std::string()
    for(var i = 0u; i < 200u; i++) { junk.append('$') }
    if(bcrypt::check_password(std::string_view("password"), junk.to_view())) {
        env.error("check_password must return false for long junk input")
    }
}

@test
func test_bcrypt_check_password_different_cost_salts(env : &mut TestEnv) {
    var pw = std::string_view("cost_test_pw")

    var salt = bcrypt::generate_salt(4)
    if(salt.empty()) { env.error("salt(4) failed"); return }
    var hash = bcrypt::bf_crypt(pw, salt.to_view())
    if(hash.empty()) { env.error("bf_crypt(cost=4) failed"); return }
    if(!bcrypt::check_password(pw, hash.to_view())) {
        env.error("check_password failed for cost 4")
    }
    if(bcrypt::check_password(std::string_view("wrong"), hash.to_view())) {
        env.error("check_password must reject wrong pw for cost 4")
    }

    var salt8 = bcrypt::generate_salt(8)
    if(salt8.empty()) { env.error("salt(8) failed"); return }
    var hash8 = bcrypt::bf_crypt(pw, salt8.to_view())
    if(hash8.empty()) { env.error("bf_crypt(cost=8) failed"); return }
    if(!bcrypt::check_password(pw, hash8.to_view())) {
        env.error("check_password failed for cost 8")
    }
}

// ---- bf_crypt (core) direct tests ----

@test
func test_bcrypt_bf_crypt_invalid_settings(env : &mut TestEnv) {
    // Empty setting
    var r1 = bcrypt::bf_crypt(std::string_view("pw"), std::string_view(""))
    if(!r1.empty()) {
        env.error("bf_crypt must return empty for empty setting")
    }
    // No leading dollar sign
    var r2 = bcrypt::bf_crypt(std::string_view("pw"), std::string_view("2b$04$abcdefghijklmnopqrstuv"))
    if(!r2.empty()) {
        env.error("bf_crypt must return empty when setting[0] != '$'")
    }
    // Second char not '2'
    var r3 = bcrypt::bf_crypt(std::string_view("pw"), std::string_view("$3b$04$abcdefghijklmnopqrstuv"))
    if(!r3.empty()) {
        env.error("bf_crypt must return empty when setting[1] != '2'")
    }
    // Too short (< 7 chars)
    var r4 = bcrypt::bf_crypt(std::string_view("pw"), std::string_view("$2b$"))
    if(!r4.empty()) {
        env.error("bf_crypt must return empty for setting < 7 chars")
    }
    // Setting with valid structure but invalid base64 chars in salt
    var invalid_b64 = std::string::make_no_len("$2b$04$!!!!!!!!!!!!!!!!!!!!!!")
    var r5 = bcrypt::bf_crypt(std::string_view("pw"), invalid_b64.to_view())
    if(!r5.empty()) {
        env.error("bf_crypt must return empty for invalid base64 in salt")
    }

    // Setting with valid structure but salt too short for 16 bytes
    var short_salt = std::string::make_no_len("$2b$04$ABCD")
    var r6 = bcrypt::bf_crypt(std::string_view("pw"), short_salt.to_view())
    if(!r6.empty()) {
        env.error("bf_crypt must return empty for salt shorter than 16 bytes")
    }
}

// ---- round-trip reliability tests ----

@test
func test_bcrypt_round_trip_various_costs(env : &mut TestEnv) {
    var passwords : [4]std::string_view = [
        std::string_view("alpha"),
        std::string_view("beta"),
        std::string_view("gamma"),
        std::string_view("delta")
    ]
    var costs : [4]int = [4, 5, 6, 8]

    for(var ci = 0u; ci < 4u; ci++) {
        var cost = costs[ci]
        var pw = passwords[ci]

        var salt = bcrypt::generate_salt(cost)
        if(salt.empty()) {
            env.error("generate_salt failed")
            return
        }
        var hash = bcrypt::bf_crypt(pw, salt.to_view())
        if(hash.empty()) {
            env.error("bf_crypt returned empty hash")
            continue
        }
        expect_uint_eq(env, hash.size(), 60u, "hash size should be 60")

        if(!bcrypt::check_password(pw, hash.to_view())) {
            env.error("check_password failed for correct pw at cost")
        }
        if(bcrypt::check_password(std::string_view("wrong"), hash.to_view())) {
            env.error("check_password incorrectly passed for wrong pw at cost")
        }
    }
}

@test
func test_bcrypt_round_trip_default_cost(env : &mut TestEnv) {
    var hash = bcrypt::hash_password(std::string_view("roundtrip_default_cost"))
    if(hash.empty()) { env.error("hash_password returned empty"); return }

    if(!bcrypt::check_password(std::string_view("roundtrip_default_cost"), hash.to_view())) {
        env.error("round trip at cost 12 failed")
    }
    if(bcrypt::check_password(std::string_view("wrong_password"), hash.to_view())) {
        env.error("wrong password must fail at cost 12")
    }
}

@test
func test_bcrypt_round_trip_various_passwords(env : &mut TestEnv) {
    // Test that multiple different passwords all work correctly
    var test_cases : [6]std::string_view = [
        std::string_view(""),
        std::string_view("p"),
        std::string_view("password123"),
        std::string_view("a_b_c_d_e_f_g"),
        std::string_view("!@#$%^&*()_+-=[]{}|;:,.<>?"),
        std::string_view("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789")
    ]

    for(var i = 0u; i < 6u; i++) {
        var pw = test_cases[i]

        // Hash at low cost (4) for speed
        var salt = bcrypt::generate_salt(4)
        if(salt.empty()) {
            env.error("generate_salt failed")
            return
        }
        var hash = bcrypt::bf_crypt(pw, salt.to_view())
        if(hash.empty()) {
            env.error("bf_crypt returned empty hash")
            continue
        }
        if(!bcrypt::check_password(pw, hash.to_view())) {
            env.error("round trip failed for a test password")
        }
    }
}

@test
func test_bcrypt_consistency_deterministic(env : &mut TestEnv) {
    // Same salt + same password must produce identical hash
    var pw = std::string_view("deterministic_test")
    var salt = bcrypt::generate_salt(4)
    if(salt.empty()) { env.error("salt failed"); return }

    var hash1 = bcrypt::bf_crypt(pw, salt.to_view())
    var hash2 = bcrypt::bf_crypt(pw, salt.to_view())

    if(hash1.empty() || hash2.empty()) {
        env.error("bf_crypt returned empty")
        return
    }
    if(!hash1.to_view().equals(hash2.to_view())) {
        env.error("same salt + same pw must produce identical hash")
    }
}

@test
func test_bcrypt_known_answer_cross_compat(env : &mut TestEnv) {
    // Known hash generated by Python bcrypt (which is compatible with Go, OpenBSD, etc.)
    var password = std::string_view("cross_compat_test")
    var known_hash = std::string_view("$2b$04$HhXMXDsKyF64bI5TZwhH4elX263P5DX8Hnu181oNOuuLc0dcJ9pje")
    var known_setting = std::string_view("$2b$04$HhXMXDsKyF64bI5TZwhH4e")

    // 1. check_password must verify the known hash
    if(!bcrypt::check_password(password, known_hash)) {
        env.error("cross-compat: check_password failed to verify known Python hash")
    }

    // 2. bf_crypt with same setting must produce identical hash
    var computed = bcrypt::bf_crypt(password, known_setting)
    if(computed.empty()) {
        env.error("cross-compat: bf_crypt returned empty")
        return
    }
    if(!computed.to_view().equals(&known_hash)) {
        env.error("cross-compat: bf_crypt output differs from known Python hash")
    }

    // 3. Wrong password must fail
    if(bcrypt::check_password(std::string_view("wrong_password"), known_hash)) {
        env.error("cross-compat: check_password must reject wrong password")
    }
}
