// ============================================================================
// TLS Library Configuration
// ============================================================================
// Port of mbedTLS mbedtls_config.h to Chemical comptime constants
// Enables/disables TLS features selectively.
// ============================================================================

public namespace tls_config {

    // Version info
    public comptime const VERSION_MAJOR = 4
    public comptime const VERSION_MINOR = 2
    public comptime const VERSION_PATCH = 0
    public comptime const VERSION_NUMBER = 0x04020000
    public comptime const VERSION_STRING = "Chemical TLS 4.2.0"

    // ─── Platform ───────────────────────────────────────────────────────────

    // Use our net module instead of mbedTLS net_sockets.c
    public comptime const NET_C = true

    // ─── TLS Features ───────────────────────────────────────────────────────

    // TLS 1.2 support
    public comptime const SSL_PROTO_TLS1_2 = true

    // TLS 1.3 support (enabled)
    public comptime const SSL_PROTO_TLS1_3 = true

    // DTLS support (disabled for now)
    public comptime const SSL_PROTO_DTLS = false

    // Client mode
    public comptime const SSL_CLI_C = true

    // Server mode (also enabled for completeness)
    public comptime const SSL_SRV_C = true

    // Generic TLS core
    public comptime const SSL_TLS_C = true

    // ─── Key Exchanges ──────────────────────────────────────────────────────

    // ECDHE key exchange with ECDSA certificates
    public comptime const KEY_EXCHANGE_ECDHE_ECDSA_ENABLED = true

    // ECDHE key exchange with RSA certificates
    public comptime const KEY_EXCHANGE_ECDHE_RSA_ENABLED = true

    // PSK key exchange
    public comptime const KEY_EXCHANGE_PSK_ENABLED = true

    // ECDHE-PSK key exchange
    public comptime const KEY_EXCHANGE_ECDHE_PSK_ENABLED = true

    // EC-JPAKE key exchange (disabled)
    public comptime const KEY_EXCHANGE_ECJPAKE_ENABLED = false

    // ─── TLS 1.3 Specific ───────────────────────────────────────────────────

    // TLS 1.3 key exchange modes
    public comptime const SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL_ENABLED = true
    public comptime const SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_ENABLED = true
    public comptime const SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_EPHEMERAL_ENABLED = true

    // TLS 1.3 compatibility mode
    public comptime const SSL_TLS1_3_COMPATIBILITY_MODE = true

    // ─── TLS Extensions ─────────────────────────────────────────────────────

    // ALPN (Application-Layer Protocol Negotiation)
    public comptime const SSL_ALPN = true

    // Server Name Indication
    public comptime const SSL_SERVER_NAME_INDICATION = true

    // Session tickets
    public comptime const SSL_SESSION_TICKETS = true

    // Max fragment length extension
    public comptime const SSL_MAX_FRAGMENT_LENGTH = true

    // Encrypt-then-MAC
    public comptime const SSL_ENCRYPT_THEN_MAC = true

    // Extended Master Secret
    public comptime const SSL_EXTENDED_MASTER_SECRET = true

    // Renegotiation support
    public comptime const SSL_RENEGOTIATION = true

    // Keep peer certificate after handshake
    public comptime const SSL_KEEP_PEER_CERTIFICATE = true

    // Send all alert messages
    public comptime const SSL_ALL_ALERT_MESSAGES = true

    // Context serialization
    public comptime const SSL_CONTEXT_SERIALIZATION = false

    // Keying material export
    public comptime const SSL_KEYING_MATERIAL_EXPORT = false

    // Session cache
    public comptime const SSL_CACHE_C = true

    // ─── X.509 ──────────────────────────────────────────────────────────────

    // X.509 certificate parsing
    public comptime const X509_CRT_PARSE_C = true

    // X.509 CRL parsing
    public comptime const X509_CRL_PARSE_C = false

    // X.509 CSR parsing
    public comptime const X509_CSR_PARSE_C = false

    // X.509 writing
    public comptime const X509_CRT_WRITE_C = false
    public comptime const X509_CSR_WRITE_C = false

    // X.509 RSASSA-PSS support
    public comptime const X509_RSASSA_PSS_SUPPORT = false

    // X.509 usage core
    public comptime const X509_USE_C = true

    // ─── Buffer Sizes ──────────────────────────────────────────────────────

    // Max incoming plaintext fragment length
    public comptime const SSL_IN_CONTENT_LEN = 16384

    // Max outgoing plaintext fragment length
    public comptime const SSL_OUT_CONTENT_LEN = 16384

    // PSK max length (32 bytes = 256 bits)
    public comptime const PSK_MAX_LEN = 32

    // Session ID length
    public comptime const SSL_SESSION_ID_LEN = 32

    // Max host name length (RFC 1035)
    public comptime const SSL_MAX_HOST_NAME_LEN = 255

    // Max ALPN name length
    public comptime const SSL_MAX_ALPN_NAME_LEN = 255

    // Verify data length for secure renegotiation
    public comptime const SSL_VERIFY_DATA_MAX_LEN = 12

    // TLS 1.3 ticket resumption key length (SHA-256)
    public comptime const SSL_TLS1_3_TICKET_RESUMPTION_KEY_LEN = 32

    // ─── Cipher Suite Feature Defines ───────────────────────────────────────

    // Chacha20-Poly1305
    public comptime const CIPHER_CHACHA20_POLY1305 = 1
    // AES-128-GCM
    public comptime const CIPHER_AES_128_GCM = 2
    // AES-256-GCM
    public comptime const CIPHER_AES_256_GCM = 3
    // AES-128-CCM
    public comptime const CIPHER_AES_128_CCM = 4
    // AES-256-CCM
    public comptime const CIPHER_AES_256_CCM = 5
    // AES-128-CBC
    public comptime const CIPHER_AES_128_CBC = 6
    // AES-256-CBC
    public comptime const CIPHER_AES_256_CBC = 7

} // namespace tls_config
