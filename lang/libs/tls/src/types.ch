// ============================================================================
// TLS Core Types - Port of mbedTLS ssl.h types to Chemical
// ============================================================================

public namespace tls {

    // ─── Error Codes ─────────────────────────────────────────────────────────
    // Ported from mbedTLS error codes

    // General errors
    public comptime const ERR_SSL_CRYPTO_IN_PROGRESS = -0x7000
    public comptime const ERR_SSL_FEATURE_UNAVAILABLE = -0x7080
    public comptime const ERR_SSL_BAD_INPUT_DATA = -0x7100
    public comptime const ERR_SSL_INVALID_MAC = -0x7180
    public comptime const ERR_SSL_INVALID_RECORD = -0x7200
    public comptime const ERR_SSL_CONN_EOF = -0x7280
    public comptime const ERR_SSL_DECODE_ERROR = -0x7300
    public comptime const ERR_SSL_NO_RNG = -0x7400
    public comptime const ERR_SSL_NO_CLIENT_CERTIFICATE = -0x7480
    public comptime const ERR_SSL_UNSUPPORTED_EXTENSION = -0x7500
    public comptime const ERR_SSL_NO_APPLICATION_PROTOCOL = -0x7580
    public comptime const ERR_SSL_PRIVATE_KEY_REQUIRED = -0x7600
    public comptime const ERR_SSL_CA_CHAIN_REQUIRED = -0x7680
    public comptime const ERR_SSL_UNEXPECTED_MESSAGE = -0x7700
    public comptime const ERR_SSL_FATAL_ALERT_MESSAGE = -0x7780
    public comptime const ERR_SSL_UNRECOGNIZED_NAME = -0x7800
    public comptime const ERR_SSL_PEER_CLOSE_NOTIFY = -0x7880
    public comptime const ERR_SSL_BAD_CERTIFICATE = -0x7A00
    public comptime const ERR_SSL_RECEIVED_NEW_SESSION_TICKET = -0x7B00
    public comptime const ERR_SSL_CACHE_ENTRY_NOT_FOUND = -0x7E80
    public comptime const ERR_SSL_ALLOC_FAILED = -0x7F00
    public comptime const ERR_SSL_HW_ACCEL_FAILED = -0x7F80
    public comptime const ERR_SSL_BAD_PROTOCOL_VERSION = -0x6E80
    public comptime const ERR_SSL_HANDSHAKE_FAILURE = -0x6E00
    public comptime const ERR_SSL_PK_TYPE_MISMATCH = -0x6D00
    public comptime const ERR_SSL_UNKNOWN_IDENTITY = -0x6C80
    public comptime const ERR_SSL_INTERNAL_ERROR = -0x6C00
    public comptime const ERR_SSL_COUNTER_WRAPPING = -0x6B80
    public comptime const ERR_SSL_BUFFER_TOO_SMALL = -0x6A00
    public comptime const ERR_SSL_WANT_READ = -0x6900
    public comptime const ERR_SSL_WANT_WRITE = -0x6880
    public comptime const ERR_SSL_TIMEOUT = -0x6800
    public comptime const ERR_SSL_UNEXPECTED_RECORD = -0x6700
    public comptime const ERR_SSL_NON_FATAL = -0x6680
    public comptime const ERR_SSL_ILLEGAL_PARAMETER = -0x6600
    public comptime const ERR_SSL_CERT_VERIFY_FAILED = -0x6500
    public comptime const ERR_SSL_CONTINUE_PROCESSING = -0x6580
    public comptime const ERR_SSL_VERSION_MISMATCH = -0x5F00
    public comptime const ERR_SSL_BAD_CONFIG = -0x5E80
    public comptime const ERR_SSL_CERTIFICATE_VERIFICATION_WITHOUT_HOSTNAME = -0x5D80

    // ─── Transport Types ────────────────────────────────────────────────────

    public comptime const SSL_TRANSPORT_STREAM = 0    // TCP/TLS
    public comptime const SSL_TRANSPORT_DATAGRAM = 1  // UDP/DTLS

    // ─── Endpoint Types ─────────────────────────────────────────────────────

    public comptime const SSL_IS_CLIENT = 0
    public comptime const SSL_IS_SERVER = 1

    // ─── TLS Protocol Versions ──────────────────────────────────────────────

    public comptime const SSL_VERSION_UNKNOWN = 0
    public comptime const SSL_VERSION_TLS1_2 = 0x0303
    public comptime const SSL_VERSION_TLS1_3 = 0x0304

    // ─── Verification Modes ─────────────────────────────────────────────────

    public comptime const SSL_VERIFY_NONE = 0
    public comptime const SSL_VERIFY_OPTIONAL = 1
    public comptime const SSL_VERIFY_REQUIRED = 2
    public comptime const SSL_VERIFY_UNSET = 3

    // ─── Record Layer Constants ─────────────────────────────────────────────

    // Content types
    public comptime const SSL_MSG_CHANGE_CIPHER_SPEC = 20
    public comptime const SSL_MSG_ALERT = 21
    public comptime const SSL_MSG_HANDSHAKE = 22
    public comptime const SSL_MSG_APPLICATION_DATA = 23
    public comptime const SSL_MSG_CID = 25

    // Handshake types
    public comptime const SSL_HS_HELLO_REQUEST = 0
    public comptime const SSL_HS_CLIENT_HELLO = 1
    public comptime const SSL_HS_SERVER_HELLO = 2
    public comptime const SSL_HS_HELLO_VERIFY_REQUEST = 3
    public comptime const SSL_HS_HELLO_RETRY_REQUEST = 6
    public comptime const SSL_HS_NEW_SESSION_TICKET = 4
    public comptime const SSL_HS_END_OF_EARLY_DATA = 5
    public comptime const SSL_HS_ENCRYPTED_EXTENSIONS = 8
    public comptime const SSL_HS_CERTIFICATE = 11
    public comptime const SSL_HS_SERVER_KEY_EXCHANGE = 12
    public comptime const SSL_HS_CERTIFICATE_REQUEST = 13
    public comptime const SSL_HS_SERVER_HELLO_DONE = 14
    public comptime const SSL_HS_CERTIFICATE_VERIFY = 15
    public comptime const SSL_HS_CLIENT_KEY_EXCHANGE = 16
    public comptime const SSL_HS_FINISHED = 20
    public comptime const SSL_HS_KEY_UPDATE = 24

    // ─── Alert Messages ─────────────────────────────────────────────────────

    // Alert levels
    public comptime const SSL_ALERT_LEVEL_WARNING = 1
    public comptime const SSL_ALERT_LEVEL_FATAL = 2

    // Alert descriptions
    public comptime const SSL_ALERT_MSG_CLOSE_NOTIFY = 0
    public comptime const SSL_ALERT_MSG_UNEXPECTED_MESSAGE = 10
    public comptime const SSL_ALERT_MSG_BAD_RECORD_MAC = 20
    public comptime const SSL_ALERT_MSG_DECRYPTION_FAILED = 21
    public comptime const SSL_ALERT_MSG_RECORD_OVERFLOW = 22
    public comptime const SSL_ALERT_MSG_HANDSHAKE_FAILURE = 40
    public comptime const SSL_ALERT_MSG_BAD_CERT = 42
    public comptime const SSL_ALERT_MSG_UNSUPPORTED_CERT = 43
    public comptime const SSL_ALERT_MSG_CERT_REVOKED = 44
    public comptime const SSL_ALERT_MSG_CERT_EXPIRED = 45
    public comptime const SSL_ALERT_MSG_CERT_UNKNOWN = 46
    public comptime const SSL_ALERT_MSG_ILLEGAL_PARAMETER = 47
    public comptime const SSL_ALERT_MSG_UNKNOWN_CA = 48
    public comptime const SSL_ALERT_MSG_ACCESS_DENIED = 49
    public comptime const SSL_ALERT_MSG_DECODE_ERROR = 50
    public comptime const SSL_ALERT_MSG_DECRYPT_ERROR = 51
    public comptime const SSL_ALERT_MSG_PROTOCOL_VERSION = 70
    public comptime const SSL_ALERT_MSG_INSUFFICIENT_SECURITY = 71
    public comptime const SSL_ALERT_MSG_INTERNAL_ERROR = 80
    public comptime const SSL_ALERT_MSG_INAPROPRIATE_FALLBACK = 86
    public comptime const SSL_ALERT_MSG_USER_CANCELED = 90
    public comptime const SSL_ALERT_MSG_NO_RENEGOTIATION = 100
    public comptime const SSL_ALERT_MSG_UNSUPPORTED_EXT = 110
    public comptime const SSL_ALERT_MSG_UNRECOGNIZED_NAME = 112
    public comptime const SSL_ALERT_MSG_CERT_REQUIRED = 116
    public comptime const SSL_ALERT_MSG_NO_APPLICATION_PROTOCOL = 120

    // ─── TLS Extensions ─────────────────────────────────────────────────────

    public comptime const TLS_EXT_SERVERNAME = 0
    public comptime const TLS_EXT_MAX_FRAGMENT_LENGTH = 1
    public comptime const TLS_EXT_TRUNCATED_HMAC = 4
    public comptime const TLS_EXT_SUPPORTED_GROUPS = 10
    public comptime const TLS_EXT_SUPPORTED_POINT_FORMATS = 11
    public comptime const TLS_EXT_SIG_ALG = 13
    public comptime const TLS_EXT_ALPN = 16
    public comptime const TLS_EXT_ENCRYPT_THEN_MAC = 22
    public comptime const TLS_EXT_EXTENDED_MASTER_SECRET = 0x0017
    public comptime const TLS_EXT_SESSION_TICKET = 35
    public comptime const TLS_EXT_PRE_SHARED_KEY = 41
    public comptime const TLS_EXT_EARLY_DATA = 42
    public comptime const TLS_EXT_SUPPORTED_VERSIONS = 43
    public comptime const TLS_EXT_COOKIE = 44
    public comptime const TLS_EXT_PSK_KEY_EXCHANGE_MODES = 45
    public comptime const TLS_EXT_KEY_SHARE = 51
    public comptime const TLS_EXT_RENEGOTIATION_INFO = 0xFF01

    // ─── Named Groups (for supported_groups extension) ──────────────────────

    public comptime const TLS_GROUP_NONE = 0
    public comptime const TLS_GROUP_SECP256R1 = 0x0017
    public comptime const TLS_GROUP_SECP384R1 = 0x0018
    public comptime const TLS_GROUP_SECP521R1 = 0x0019
    public comptime const TLS_GROUP_X25519 = 0x001D
    public comptime const TLS_GROUP_X448 = 0x001E
    public comptime const TLS_GROUP_FFDHE2048 = 0x0100
    public comptime const TLS_GROUP_FFDHE3072 = 0x0101

    // ─── Signature Algorithms (TLS 1.2) ─────────────────────────────────────

    public comptime const SSL_HASH_NONE = 0
    public comptime const SSL_HASH_MD5 = 1
    public comptime const SSL_HASH_SHA1 = 2
    public comptime const SSL_HASH_SHA224 = 3
    public comptime const SSL_HASH_SHA256 = 4
    public comptime const SSL_HASH_SHA384 = 5
    public comptime const SSL_HASH_SHA512 = 6

    public comptime const SSL_SIG_ANON = 0
    public comptime const SSL_SIG_RSA = 1
    public comptime const SSL_SIG_ECDSA = 3

    // ─── TLS 1.3 Signature Algorithms ───────────────────────────────────────

    public comptime const TLS1_3_SIG_RSA_PKCS1_SHA256 = 0x0401
    public comptime const TLS1_3_SIG_RSA_PKCS1_SHA384 = 0x0501
    public comptime const TLS1_3_SIG_RSA_PKCS1_SHA512 = 0x0601
    public comptime const TLS1_3_SIG_ECDSA_SECP256R1_SHA256 = 0x0403
    public comptime const TLS1_3_SIG_ECDSA_SECP384R1_SHA384 = 0x0503
    public comptime const TLS1_3_SIG_ECDSA_SECP521R1_SHA512 = 0x0603
    public comptime const TLS1_3_SIG_RSA_PSS_RSAE_SHA256 = 0x0804
    public comptime const TLS1_3_SIG_RSA_PSS_RSAE_SHA384 = 0x0805

    // ─── Max Fragment Length Codes ─────────────────────────────────────────

    public comptime const SSL_MAX_FRAG_LEN_NONE = 0
    public comptime const SSL_MAX_FRAG_LEN_512 = 1
    public comptime const SSL_MAX_FRAG_LEN_1024 = 2
    public comptime const SSL_MAX_FRAG_LEN_2048 = 3
    public comptime const SSL_MAX_FRAG_LEN_4096 = 4

    // ─── State Machine ──────────────────────────────────────────────────────

    public variant SSLState {
        HELLO_REQUEST()
        CLIENT_HELLO()
        SERVER_HELLO()
        SERVER_CERTIFICATE()
        SERVER_KEY_EXCHANGE()
        CERTIFICATE_REQUEST()
        SERVER_HELLO_DONE()
        CLIENT_CERTIFICATE()
        CLIENT_KEY_EXCHANGE()
        CERTIFICATE_VERIFY()
        CLIENT_CHANGE_CIPHER_SPEC()
        CLIENT_FINISHED()
        SERVER_CHANGE_CIPHER_SPEC()
        SERVER_FINISHED()
        HANDSHAKE_OVER()
        // TLS 1.3 states
        ENCRYPTED_EXTENSIONS()
        HELLO_RETRY_REQUEST()
        END_OF_EARLY_DATA()
        TLS1_3_NEW_SESSION_TICKET()
    }

    // ─── Session Structure ──────────────────────────────────────────────────

    public struct Session {
        var id : [32]u8
        var id_len : size_t
        var master : [48]u8           // TLS 1.2 master secret
        var ciphersuite : u16
        var tls_version : u8
        var endpoint : u8             // SSL_IS_CLIENT or SSL_IS_SERVER
        var peer_cert : *mut X509Cert // Peer certificate chain
        var verify_result : u32

        // TLS 1.3 fields
        var resumption_key : [32]u8
        var resumption_key_len : u8
        var ticket_age_add : u32
        var ticket_flags : u8

        // Session ticket
        var ticket : *mut u8
        var ticket_len : size_t
        var ticket_lifetime : u32

        @constructor
        func constructor() {
            return Session {
                id: [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
                id_len: 0,
                master: [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                         0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                         0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
                ciphersuite: 0,
                tls_version: 0,
                endpoint: 0,
                peer_cert: null,
                verify_result: 0,
                resumption_key: [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
                resumption_key_len: 0,
                ticket_age_add: 0,
                ticket_flags: 0,
                ticket: null,
                ticket_len: 0,
                ticket_lifetime: 0
            }
        }

        @delete
        func destruct(&self) {
            if(self.ticket != null) {
                unsafe { dealloc self.ticket }
            }
            // peer_cert is owned by the caller, don't free it
        }
    }

    // ─── Record Header ──────────────────────────────────────────────────────

    public struct RecordHeader {
        var content_type : u8       // SSL_MSG_*
        var legacy_version : u8     // Record layer version
        var length : u16            // Payload length
    }

    // ─── Handshake Header ───────────────────────────────────────────────────

    public struct HandshakeHeader {
        var msg_type : u8           // SSL_HS_*
        var length : u24            // 3-byte length
        var seq : u16               // TLS 1.3 sequence number; TLS 1.2 uses separate counter
    }

    // 3-byte integer helper
    public struct u24 {
        var data : [3]u8
    }

    // ─── TLS 1.3 Key Schedule State ─────────────────────────────────────────

    public struct TLS13KeySchedule {
        var hash_algorithm : u8    // HASH_SHA256 or HASH_SHA384
        var early_secret : [48]u8
        var handshake_secret : [48]u8
        var master_secret : [48]u8
        var client_handshake_traffic_secret : [48]u8
        var server_handshake_traffic_secret : [48]u8
        var client_application_traffic_secret : [48]u8
        var server_application_traffic_secret : [48]u8
        var exporter_master_secret : [48]u8
        var resumption_master_secret : [48]u8
    }

    public func tls13_key_schedule_init(ks : *mut TLS13KeySchedule) {
        ks.hash_algorithm = 0
        var i : size_t = 0
        while(i < 48) {
            ks.early_secret[i] = 0
            ks.handshake_secret[i] = 0
            ks.master_secret[i] = 0
            ks.client_handshake_traffic_secret[i] = 0
            ks.server_handshake_traffic_secret[i] = 0
            ks.client_application_traffic_secret[i] = 0
            ks.server_application_traffic_secret[i] = 0
            ks.exporter_master_secret[i] = 0
            ks.resumption_master_secret[i] = 0
            i += 1
        }
    }

    // ─── Key Share Entry (TLS 1.3) ──────────────────────────────────────────

    public struct KeyShareEntry {
        var group : u16             // NamedGroup
        var key_exchange : *mut u8  // Key exchange data
        var key_exchange_len : u16
    }

    // ─── SSL Config ─────────────────────────────────────────────────────────

    public struct SSLConfig {
        var endpoint : int          // SSL_IS_CLIENT or SSL_IS_SERVER
        var transport : int         // SSL_TRANSPORT_STREAM or DATAGRAM
        var min_tls_version : int   // Minimum TLS version
        var max_tls_version : int   // Maximum TLS version
        var authmode : int          // Verification mode

        @constructor
        func constructor(endpoint_type : int) {
            ensure_init()  // Initialize ciphersuite database
            var pref_count = num_preferred_ciphersuites()
            var suite_list : [64]u16
            var i : u32 = 0
            while(i < pref_count) {
                suite_list[i] = get_preferred_ciphersuite(i)
                i += 1
            }
            if(pref_count < 64) {
                suite_list[pref_count] = 0
            }

            // Need to initialize fields one by one
            var cfg : SSLConfig
            cfg.endpoint = endpoint_type
            cfg.transport = SSL_TRANSPORT_STREAM
            cfg.min_tls_version = SSL_VERSION_TLS1_2
            cfg.max_tls_version = SSL_VERSION_TLS1_3
            cfg.authmode = SSL_VERIFY_REQUIRED
            cfg.f_send = null
            cfg.f_recv = null
            cfg.f_recv_timeout = null
            cfg.p_send = null
            cfg.p_recv = null
            cfg.ciphersuite_count = pref_count
            i = 0
            while(i < pref_count) {
                cfg.ciphersuite_list[i] = suite_list[i]
                i += 1
            }
            cfg.own_cert = null
            cfg.own_key = null
            cfg.ca_chain = null
            cfg.ca_crl = null
            cfg.alpn_list = null
            cfg.alpn_count = 0
            cfg.f_get_cache = null
            cfg.f_set_cache = null
            cfg.p_cache = null
            cfg.session_tickets = 1
            cfg.extended_ms = 1
            cfg.encrypt_then_mac = 1
            cfg.renegotiation_enabled = 0
            cfg.hostname = null
            cfg.read_timeout = 10000
            cfg.write_timeout = 10000
            cfg.hs_timeout_min = 1000
            cfg.hs_timeout_max = 60000
            return cfg
        }

        // Callback pointers (function pointers)
        var f_send : *void          // Send callback
        var f_recv : *void          // Receive callback
        var f_recv_timeout : *void  // Receive timeout callback
        var p_send : *void          // Send context
        var p_recv : *void          // Receive context

        // Cipher suite preference
        var ciphersuite_list : [64]u16  // Preferred ciphersuites (0-terminated)
        var ciphersuite_count : u32

        // Certificate chain
        var own_cert : *mut X509Cert
        var own_key : *mut void      // Private key (opaque)
        var ca_chain : *mut X509Cert // Trusted CA certificates
        var ca_crl : *mut void       // CRL chain

        // TLS extensions
        var alpn_list : *mut *char   // ALPN protocol list
        var alpn_count : u8

        // Session cache callbacks
        var f_get_cache : *void
        var f_set_cache : *void
        var p_cache : *void

        // Session tickets
        var session_tickets : int   // 0=disabled, 1=enabled

        // Extended master secret
        var extended_ms : int

        // Encrypt-then-MAC
        var encrypt_then_mac : int

        // Renegotiation
        var renegotiation_enabled : int

        // SNI hostname
        var hostname : *char

        // Read/write timeout (seconds)
        var read_timeout : u32
        var write_timeout : u32

        // Handshake timeout (ms)
        var hs_timeout_min : u32
        var hs_timeout_max : u32

    }

    // ─── SSL Context (Main Connection State) ────────────────────────────────

    public struct SSLContext {
        var conf : *mut SSLConfig    // Configuration (shared)
        var session : *mut Session   // Current session
        var state : SSLState         // Handshake state machine

        // I/O buffers
        var in_buf : [17408]u8       // Input buffer (16384 + 1024 for overhead)
        var in_msglen : i32
        var in_left : i32
        var in_hdr : [5]u8           // Current record header
        var in_offt : i32

        var out_buf : [17408]u8      // Output buffer
        var out_msglen : i32
        var out_left : i32

        // Sequence numbers
        var in_ctr : [8]u8
        var out_ctr : [8]u8

        // Handshake state
        var handshake : *mut HandshakeParams

        // TLS version negotiated
        var tls_version : u8
        var major_ver : u8
        var minor_ver : u8

        // Key schedule (TLS 1.3)
        var tls13_keys : TLS13KeySchedule

        // Peer certificate
        var peer_cert : *mut X509Cert

        // Hostname for SNI
        var hostname : *char
        var hostname_len : size_t

        // ALPN negotiated
        var alpn_negotiated : *char
        var alpn_negotiated_len : size_t

        // Transform (encryption context)
        var transform_in : *mut Transform
        var transform_out : *mut Transform
        var transform_negotiated : *mut Transform

        // Handshake message hash
        var handshake_hash : [64]u8
        var handshake_hash_len : size_t

        // I/O callbacks
        var transport_socket : net::Socket
        var transport_connected : bool

        // Negotiated values from handshake
        var negotiated_ciphersuite : u16

        // Alert handling
        var last_alert_level : u8
        var last_alert_desc : u8

        // Early data
        var early_data_status : u8

        // Destructor: close socket if connected
        @delete
        func destruct(&self) {
            if(self.transport_connected) {
                net::close_socket(self.transport_socket)
            }
        }
    }

    public func ssl_context_init(ssl : *mut SSLContext) {
        ssl.conf = null
        ssl.session = null
        ssl.state = SSLState.HELLO_REQUEST()
        ssl.in_msglen = 0
        ssl.in_left = 0
        var i : size_t = 0
        while(i < 17408) {
            ssl.in_buf[i] = 0
            ssl.out_buf[i] = 0
            i += 1
        }
        i = 0
        while(i < 5) {
            ssl.in_hdr[i] = 0
            i += 1
        }
        ssl.in_offt = 0
        ssl.out_msglen = 0
        ssl.out_left = 0
        i = 0
        while(i < 8) {
            ssl.in_ctr[i] = 0
            ssl.out_ctr[i] = 0
            i += 1
        }
        ssl.handshake = null
        ssl.tls_version = 0
        ssl.major_ver = 0
        ssl.minor_ver = 0
        tls13_key_schedule_init(&raw mut ssl.tls13_keys)
        ssl.peer_cert = null
        ssl.hostname = null
        ssl.hostname_len = 0
        ssl.alpn_negotiated = null
        ssl.alpn_negotiated_len = 0
        ssl.transform_in = null
        ssl.transform_out = null
        ssl.transform_negotiated = null
        i = 0
        while(i < 64) {
            ssl.handshake_hash[i] = 0
            i += 1
        }
        ssl.handshake_hash_len = 0
        ssl.transport_socket = 0
        ssl.transport_connected = false
        ssl.negotiated_ciphersuite = 0
        ssl.last_alert_level = 0
        ssl.last_alert_desc = 0
        ssl.early_data_status = 0
    }

    // ─── Transform (Encryption Context) ─────────────────────────────────────

    public struct Transform {
        var cipher_type : u8        // CIPHER_*
        var hash_type : u8          // HASH_*

        // Keys
        var iv_enc : [16]u8
        var iv_dec : [16]u8
        var key_enc : [32]u8
        var key_dec : [32]u8
        var mac_key_enc : [48]u8
        var mac_key_dec : [48]u8
        var iv_len : u8
        var key_len : u8
        var mac_key_len : u8
        var fixed_iv_len : u8

        // TLS 1.3
        var base_iv_enc : [16]u8
        var base_iv_dec : [16]u8
    }

    public func transform_init(tr : *mut Transform) {
        tr.cipher_type = 0
        tr.hash_type = 0
        var i : size_t = 0
        while(i < 16) {
            tr.iv_enc[i] = 0
            tr.iv_dec[i] = 0
            tr.base_iv_enc[i] = 0
            tr.base_iv_dec[i] = 0
            i += 1
        }
        i = 0
        while(i < 32) {
            tr.key_enc[i] = 0
            tr.key_dec[i] = 0
            i += 1
        }
        i = 0
        while(i < 48) {
            tr.mac_key_enc[i] = 0
            tr.mac_key_dec[i] = 0
            i += 1
        }
        tr.iv_len = 0
        tr.key_len = 0
        tr.mac_key_len = 0
        tr.fixed_iv_len = 0
    }

    // ─── Handshake Parameters ──────────────────────────────────────────────

    public struct HandshakeParams {
        // Client and server random
        var randbytes : [64]u8

        // TLS 1.2 handshake secrets
        var premaster : [256]u8
        var premaster_len : size_t

        // TLS 1.3
        var key_schedule : TLS13KeySchedule
        var hello_retry_requested : bool

        // ECDHE parameters
        var ecdhe_curve : u16
        var ecdhe_public : *mut u8
        var ecdhe_public_len : u16
        var ecdhe_private : *mut u8
        var ecdhe_private_len : u16

        // x25519 parameters
        var x25519_public : *mut u8
        var x25519_public_len : u16
        var x25519_private : *mut u8

        // PSK
        var psk : [32]u8
        var psk_len : u8
        var psk_identity : *mut u8
        var psk_identity_len : u16

        // Certificate verify
        var sni_authmode : u8

        // Extended master secret
        var extended_ms : u8

        // Session ticket
        var new_session_ticket : [4096]u8
        var new_session_ticket_len : u16
    }

    public func handshake_params_init(hs : *mut HandshakeParams) {
        var i : size_t = 0
        while(i < 64) {
            hs.randbytes[i] = 0
            i += 1
        }
        i = 0
        while(i < 256) {
            hs.premaster[i] = 0
            i += 1
        }
        hs.premaster_len = 0
        tls13_key_schedule_init(&raw mut hs.key_schedule)
        hs.hello_retry_requested = false
        hs.ecdhe_curve = 0
        hs.ecdhe_public = null
        hs.ecdhe_public_len = 0
        hs.ecdhe_private = null
        hs.ecdhe_private_len = 0
        hs.x25519_public = null
        hs.x25519_public_len = 0
        hs.x25519_private = null
        i = 0
        while(i < 32) {
            hs.psk[i] = 0
            i += 1
        }
        hs.psk_len = 0
        hs.psk_identity = null
        hs.psk_identity_len = 0
        hs.sni_authmode = 0
        hs.extended_ms = 1
        i = 0
        while(i < 4096) {
            hs.new_session_ticket[i] = 0
            i += 1
        }
        hs.new_session_ticket_len = 0
    }

    // ─── I/O Callback Types ─────────────────────────────────────────────────

    // Send callback: returns bytes sent or negative error
    public type SendCallback = (ctx : *void, buf : *u8, len : size_t) => int

    // Receive callback: returns bytes received or negative error
    public type RecvCallback = (ctx : *void, buf : *mut u8, len : size_t) => int

    // Receive with timeout callback
    public type RecvTimeoutCallback = (ctx : *void, buf : *mut u8, len : size_t, timeout : u32) => int

} // namespace tls
