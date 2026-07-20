// ============================================================================
// TLS Cipher Suite Definitions
// ============================================================================
// Port of mbedTLS ssl_ciphersuites.h to Chemical.
// Defines all available cipher suites and their properties.
// ============================================================================

public namespace tls {

    // ─── TLS 1.3 Cipher Suites ──────────────────────────────────────────────

    // TLS 1.3 cipher suite IDs (from RFC 8446)
    public comptime const TLS1_3_AES_128_GCM_SHA256 = 0x1301
    public comptime const TLS1_3_AES_256_GCM_SHA384 = 0x1302
    public comptime const TLS1_3_CHACHA20_POLY1305_SHA256 = 0x1303
    public comptime const TLS1_3_AES_128_CCM_SHA256 = 0x1304
    public comptime const TLS1_3_AES_128_CCM_8_SHA256 = 0x1305

    // ─── TLS 1.2 Cipher Suites ──────────────────────────────────────────────

    public comptime const TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256 = 0xC02B
    public comptime const TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384 = 0xC02C
    public comptime const TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256 = 0xC02F
    public comptime const TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384 = 0xC030
    public comptime const TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256 = 0xC023
    public comptime const TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384 = 0xC024
    public comptime const TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256 = 0xC027
    public comptime const TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384 = 0xC028
    public comptime const TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA = 0xC009
    public comptime const TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA = 0xC00A
    public comptime const TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA = 0xC013
    public comptime const TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA = 0xC014
    public comptime const TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256 = 0xCCA8
    public comptime const TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256 = 0xCCA9
    public comptime const TLS_PSK_WITH_CHACHA20_POLY1305_SHA256 = 0xCCAB
    public comptime const TLS_ECDHE_PSK_WITH_CHACHA20_POLY1305_SHA256 = 0xCCAC
    public comptime const TLS_ECDHE_ECDSA_WITH_AES_128_CCM = 0xC0AC
    public comptime const TLS_ECDHE_ECDSA_WITH_AES_256_CCM = 0xC0AD
    public comptime const TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8 = 0xC0AE
    public comptime const TLS_ECDHE_ECDSA_WITH_AES_256_CCM_8 = 0xC0AF
    public comptime const TLS_PSK_WITH_AES_128_GCM_SHA256 = 0x00A8
    public comptime const TLS_PSK_WITH_AES_256_GCM_SHA384 = 0x00A9
    public comptime const TLS_PSK_WITH_AES_128_CBC_SHA256 = 0x00AE
    public comptime const TLS_PSK_WITH_AES_256_CBC_SHA384 = 0x00AF
    public comptime const TLS_PSK_WITH_AES_128_CBC_SHA = 0x008C
    public comptime const TLS_PSK_WITH_AES_256_CBC_SHA = 0x008D
    public comptime const TLS_ECDHE_PSK_WITH_AES_128_CBC_SHA256 = 0xC037
    public comptime const TLS_ECDHE_PSK_WITH_AES_256_CBC_SHA384 = 0xC038
    public comptime const TLS_ECDHE_PSK_WITH_AES_128_CBC_SHA = 0xC035
    public comptime const TLS_ECDHE_PSK_WITH_AES_256_CBC_SHA = 0xC036
    public comptime const TLS_ECJPAKE_WITH_AES_128_CCM_8 = 0xC0FF

    // ─── Cipher Type Identifiers ────────────────────────────────────────────

    public comptime const CIPHER_NONE = 0
    public comptime const CIPHER_AES_128_GCM = 1
    public comptime const CIPHER_AES_256_GCM = 2
    public comptime const CIPHER_AES_128_CCM = 3
    public comptime const CIPHER_AES_256_CCM = 4
    public comptime const CIPHER_AES_128_CBC = 5
    public comptime const CIPHER_AES_256_CBC = 6
    public comptime const CIPHER_CHACHA20_POLY1305 = 7
    public comptime const CIPHER_NULL = 8

    public comptime const HASH_NONE = 0
    public comptime const HASH_SHA1 = 1
    public comptime const HASH_SHA256 = 2
    public comptime const HASH_SHA384 = 3

    public comptime const KE_NONE = 0
    public comptime const KE_ECDHE_ECDSA = 1
    public comptime const KE_ECDHE_RSA = 2
    public comptime const KE_PSK = 3
    public comptime const KE_ECDHE_PSK = 4
    public comptime const KE_ECJPAKE = 5

    public comptime const CIPHERSUITE_SHORT_TAG : u8 = 1

    // ─── Cipher Suite Info Struct ───────────────────────────────────────────

    public struct CipherSuiteInfo {
        var id : u16
        var name : *char
        var cipher : u8
        var hash : u8
        var key_exchange : u8
        var flags : u8
        var min_tls_version : u8
        var max_tls_version : u8
        var key_size : u8
        var iv_size : u8
        var tag_size : u8
        var mac_key_len : u8
    }

    // ─── Helper to cast comptime const to u16 ───────────────────────────

    func cs(cs_id : int) : u16 { return cs_id as u16 }

    // ─── Lazy Initialization ────────────────────────────────────────────────

    private var _initialized : int

    public func ensure_init() {
        if(_initialized == 0) {
            _initialized = 1
        }
    }

    // ─── Cipher Suite Info Functions ────────────────────────────────────────

    // Get info about a ciphersuite by checking all known suites
    public func get_ciphersuite_info(ciphersuite_id : u16) : CipherSuiteInfo {
        ensure_init()
        // Check TLS 1.3 suites
        if(ciphersuite_id == cs(TLS1_3_AES_128_GCM_SHA256)) {
            return CipherSuiteInfo {
                id: cs(TLS1_3_AES_128_GCM_SHA256),
                name: null,
                cipher: CIPHER_AES_128_GCM as u8,
                hash: HASH_SHA256 as u8,
                key_exchange: KE_NONE as u8,
                flags: 0, min_tls_version: 4, max_tls_version: 4,
                key_size: 16, iv_size: 12, tag_size: 16, mac_key_len: 0
            }
        }
        if(ciphersuite_id == cs(TLS1_3_AES_256_GCM_SHA384)) {
            return CipherSuiteInfo {
                id: cs(TLS1_3_AES_256_GCM_SHA384),
                name: null,
                cipher: CIPHER_AES_256_GCM as u8,
                hash: HASH_SHA384 as u8,
                key_exchange: KE_NONE as u8,
                flags: 0, min_tls_version: 4, max_tls_version: 4,
                key_size: 32, iv_size: 12, tag_size: 16, mac_key_len: 0
            }
        }
        if(ciphersuite_id == cs(TLS1_3_CHACHA20_POLY1305_SHA256)) {
            return CipherSuiteInfo {
                id: cs(TLS1_3_CHACHA20_POLY1305_SHA256),
                name: null,
                cipher: CIPHER_CHACHA20_POLY1305 as u8,
                hash: HASH_SHA256 as u8,
                key_exchange: KE_NONE as u8,
                flags: 0, min_tls_version: 4, max_tls_version: 4,
                key_size: 32, iv_size: 12, tag_size: 16, mac_key_len: 0
            }
        }
        if(ciphersuite_id == cs(TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256)) {
            return CipherSuiteInfo {
                id: cs(TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256),
                name: null,
                cipher: CIPHER_AES_128_GCM as u8,
                hash: HASH_SHA256 as u8,
                key_exchange: KE_ECDHE_RSA as u8,
                flags: 0, min_tls_version: 3, max_tls_version: 3,
                key_size: 16, iv_size: 12, tag_size: 16, mac_key_len: 0
            }
        }
        if(ciphersuite_id == cs(TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256)) {
            return CipherSuiteInfo {
                id: cs(TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256),
                name: null,
                cipher: CIPHER_AES_128_GCM as u8,
                hash: HASH_SHA256 as u8,
                key_exchange: KE_ECDHE_ECDSA as u8,
                flags: 0, min_tls_version: 3, max_tls_version: 3,
                key_size: 16, iv_size: 12, tag_size: 16, mac_key_len: 0
            }
        }
        if(ciphersuite_id == cs(TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384)) {
            return CipherSuiteInfo {
                id: cs(TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384),
                name: null,
                cipher: CIPHER_AES_256_GCM as u8,
                hash: HASH_SHA384 as u8,
                key_exchange: KE_ECDHE_RSA as u8,
                flags: 0, min_tls_version: 3, max_tls_version: 3,
                key_size: 32, iv_size: 12, tag_size: 16, mac_key_len: 0
            }
        }
        if(ciphersuite_id == cs(TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384)) {
            return CipherSuiteInfo {
                id: cs(TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384),
                name: null,
                cipher: CIPHER_AES_256_GCM as u8,
                hash: HASH_SHA384 as u8,
                key_exchange: KE_ECDHE_ECDSA as u8,
                flags: 0, min_tls_version: 3, max_tls_version: 3,
                key_size: 32, iv_size: 12, tag_size: 16, mac_key_len: 0
            }
        }
        if(ciphersuite_id == cs(TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256)) {
            return CipherSuiteInfo {
                id: cs(TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256),
                name: null,
                cipher: CIPHER_CHACHA20_POLY1305 as u8,
                hash: HASH_SHA256 as u8,
                key_exchange: KE_ECDHE_RSA as u8,
                flags: 0, min_tls_version: 3, max_tls_version: 3,
                key_size: 32, iv_size: 12, tag_size: 16, mac_key_len: 0
            }
        }
        return CipherSuiteInfo {
            id: 0, name: null, cipher: CIPHER_NONE as u8, hash: 0,
            key_exchange: 0, flags: 0, min_tls_version: 0, max_tls_version: 0,
            key_size: 0, iv_size: 0, tag_size: 0, mac_key_len: 0
        }
    }

    public func num_preferred_ciphersuites() : u32 {
        return 8
    }

    public func get_preferred_ciphersuite(index : u32) : u16 {
        if(index == 0) { return cs(TLS1_3_AES_128_GCM_SHA256) }
        if(index == 1) { return cs(TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256) }
        if(index == 2) { return cs(TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384) }
        if(index == 3) { return cs(TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256) }
        if(index == 4) { return cs(TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384) }
        if(index == 5) { return cs(TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256) }
        if(index == 6) { return cs(TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256) }
        if(index == 7) { return cs(TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384) }
        return 0
    }

    public func ciphersuite_is_aead(ciphersuite_id : u16) : bool {
        if(ciphersuite_id == cs(TLS1_3_AES_128_GCM_SHA256)) { return true }
        if(ciphersuite_id == cs(TLS1_3_AES_256_GCM_SHA384)) { return true }
        if(ciphersuite_id == cs(TLS1_3_CHACHA20_POLY1305_SHA256)) { return true }
        if(ciphersuite_id == cs(TLS1_3_AES_128_CCM_SHA256)) { return true }
        if(ciphersuite_id == cs(TLS1_3_AES_128_CCM_8_SHA256)) { return true }
        if(ciphersuite_id == cs(TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256)) { return true }
        if(ciphersuite_id == cs(TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384)) { return true }
        if(ciphersuite_id == cs(TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256)) { return true }
        if(ciphersuite_id == cs(TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384)) { return true }
        if(ciphersuite_id == cs(TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256)) { return true }
        if(ciphersuite_id == cs(TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256)) { return true }
        if(ciphersuite_id == cs(TLS_PSK_WITH_CHACHA20_POLY1305_SHA256)) { return true }
        if(ciphersuite_id == cs(TLS_ECDHE_PSK_WITH_CHACHA20_POLY1305_SHA256)) { return true }
        if(ciphersuite_id == cs(TLS_ECDHE_ECDSA_WITH_AES_128_CCM)) { return true }
        if(ciphersuite_id == cs(TLS_ECDHE_ECDSA_WITH_AES_256_CCM)) { return true }
        return false
    }

} // namespace tls
