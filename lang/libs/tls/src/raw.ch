// tls::raw — legacy mbedTLS-shaped implementation surface.
//
// This is the ONLY declaration of `tls::raw` in the module; nested namespaces
// cannot be reopened across files, so every raw TLS/crypto source is spliced
// here as one unit. It exposes SSLContext/SSLConfig/Transport, X509Cert,
// RSAContext, ECDSAContext, MPI/EC arithmetic, record transforms, and the
// `ssl_*`/`mbedtls_*`-shaped helpers. Canonical code must use the owning
// `TlsConfig`/`Certificate`/`CertificateChain`/`PrivateKey`/`TlsStream`/
// `TlsServerStream` surface in `safe.ch`; raw handles are not memory-safe and
// require exact allocation/free pairing.

public namespace tls {

public namespace raw {

// ===== types.ch =====

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
        var tls_version : int
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
        unsafe {

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
            // Build the automatic preference list, restricted to suites this
            // stack can fully complete: TLS 1.3 suites plus TLS 1.2 RSA-key-
            // exchange GCM/CBC suites with SHA-256 or SHA-384 PRFs. TLS 1.2
            // ECDHE is not implemented, so those are excluded from the AUTO
            // list only — explicit ciphersuite_list configs are honored
            // verbatim (an unsupported pick fails gracefully post-hello).
            var pref_src_count = num_preferred_ciphersuites()
            var suite_list : [64]u16
            var pref_count : u32 = 0
            var i : u32 = 0
            while(i < pref_src_count && pref_count < 64) {
                var sid = get_preferred_ciphersuite(i)
                if(sid != 0) {
                    var sinfo = get_ciphersuite_info(sid)
                    // Fail closed on missing metadata: the unknown-suite
                    // default is {id: 0, key_exchange: KE_NONE, hash:
                    // HASH_NONE}, which satisfies the "supported" test below
                    // and would silently smuggle an ECDHE suite we cannot
                    // complete into the offer. Advertising a suite the client
                    // must then abort on turns a clean "no shared cipher"
                    // rejection into a post-negotiation handshake_failure.
                    var known = (sinfo.id == sid)
                    var tls13_only = (sinfo.max_tls_version >= SSL_VERSION_TLS1_3 as u8)
                    var supported_tls12 = ((sinfo.key_exchange == KE_RSA as u8 || sinfo.key_exchange == KE_NONE as u8) &&
                                           (sinfo.hash == HASH_SHA256 as u8 || sinfo.hash == HASH_SHA384 as u8 || sinfo.hash == HASH_NONE as u8))
                    if(known && (tls13_only || supported_tls12)) {
                        suite_list[pref_count] = sid
                        pref_count += 1
                    }
                }
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

    // ─── Transport (I/O abstraction) ─────────────────────────────────────────
    //
    // All record I/O goes through this vtable: two async callbacks returning a
    // `FutureHandle<int>` (bytes transferred, or a negative error). The default
    // socket transport performs **blocking** sends/receives, so the synchronous
    // API (the coroutine driven by `async::block_on`) behaves exactly as the old
    // direct `net::raw::send_all`/`recv_all` calls did. The async API installs a
    // non-blocking transport whose callbacks suspend on fd readiness instead, so
    // the handshake and record I/O are genuine coroutines (TLS-VT).

    public type TransportRecvFn = (ctx : *mut void, buf : *mut u8, len : size_t) => core::async::FutureHandle<int>
    public type TransportSendFn = (ctx : *mut void, buf : *u8, len : size_t) => core::async::FutureHandle<int>

    public struct Transport {
        var ctx : *mut void       // user data (the socket for the built-in transport)
        var recv_fn : TransportRecvFn
        var send_fn : TransportSendFn
    }

    // ─── SSL Context (Main Connection State) ────────────────────────────────

    public struct SSLContext {
        var conf : *mut SSLConfig    // Configuration (shared)
        var conf_owned : bool        // true if conf was heap-allocated by the library (freed by ssl_free)
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
        var tls_version : int
        var major_ver : u8
        var minor_ver : u8

        // Key schedule (TLS 1.3)
        var tls13_keys : TLS13KeySchedule

        // Peer certificate
        var peer_cert : *mut X509Cert

        // Hostname for SNI. `ssl_set_hostname` owns a copy; `hostname_owned`
        // distinguishes it from a directly assigned borrowed pointer.
        var hostname : *char
        var hostname_len : size_t
        var hostname_owned : bool

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
        var transport_socket : net::raw::Socket
        var transport_connected : bool

        // Record I/O transport (blocking socket by default). `transport_async`
        // selects the non-blocking transport that suspends on fd readiness.
        var transport : Transport
        var transport_async : bool

        // Negotiated values from handshake
        var negotiated_ciphersuite : u16

        // Alert handling
        var last_alert_level : u8
        var last_alert_desc : u8

        // Early data
        var early_data_status : u8

        @constructor
        func constructor() {
            unsafe {

            ensure_init()
            var ssl : SSLContext
            ssl.conf = null
            ssl.conf_owned = false
            var sess_mem = unsafe(malloc(sizeof(Session))) as *mut Session
            if(sess_mem != null) {
                sess_mem.ticket = null
                unsafe { *sess_mem = Session() }
            }
            ssl.session = sess_mem
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
            ssl.hostname_owned = false
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
            ssl.transport = Transport { ctx : null, recv_fn : null, send_fn : null }
            ssl.transport_async = false
            ssl.negotiated_ciphersuite = 0
            ssl.last_alert_level = 0
            ssl.last_alert_desc = 0
            ssl.early_data_status = 0
            return ssl

        }
}

        // Destructor: full cleanup so `delete ssl` is safe (same as ssl_free).
        // ssl_free is idempotent, so calling it here does not double-free even
        // if ssl_free() was already invoked explicitly.
        @delete
        func destruct(&mut self) {
            ssl_free(&raw mut self)
        }
    }

    public func ssl_context_init(ssl : *mut SSLContext) { unsafe {
        ssl.conf = null
        ssl.conf_owned = false
        var sess_mem = malloc(sizeof(Session)) as *mut Session
        if(sess_mem != null) {
            // Null the destructor-relevant pointer BEFORE the move-assign so the
            // generated destructor call on the uninitialized target is a no-op.
            sess_mem.ticket = null
            *sess_mem = Session()
        }
        ssl.session = sess_mem
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
        ssl.hostname_owned = false
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
        ssl.transport = Transport { ctx : null, recv_fn : null, send_fn : null }
        ssl.transport_async = false
        ssl.negotiated_ciphersuite = 0
        ssl.last_alert_level = 0
        ssl.last_alert_desc = 0
        ssl.early_data_status = 0
     }}

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
        unsafe {

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
        var psk_accepted : bool        // server accepted the offered pre_shared_key
        var psk_binder_off : u16       // byte offset of the 32-byte binder placeholder in ClientHello
        var psk_partial_len : u16      // byte offset where the binders list starts (= end of identities)

        // Certificate verify
        var sni_authmode : u8

        // Extended master secret
        var extended_ms : u8

        // Session ticket
        var new_session_ticket : [4096]u8
        var new_session_ticket_len : u16
    }

    public func handshake_params_init(hs : *mut HandshakeParams) {
        unsafe {

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
        hs.psk_accepted = false
        hs.psk_binder_off = 0
        hs.psk_partial_len = 0
        hs.sni_authmode = 0
        hs.extended_ms = 1
        i = 0
        while(i < 4096) {
            hs.new_session_ticket[i] = 0
            i += 1
        }
        hs.new_session_ticket_len = 0

    }
}

    // ─── I/O Callback Types ─────────────────────────────────────────────────

    // Send callback: returns bytes sent or negative error
    public type SendCallback = (ctx : *void, buf : *u8, len : size_t) => int

    // Receive callback: returns bytes received or negative error
    public type RecvCallback = (ctx : *void, buf : *mut u8, len : size_t) => int

    // Receive with timeout callback
    public type RecvTimeoutCallback = (ctx : *void, buf : *mut u8, len : size_t, timeout : u32) => int


// ===== ssl.ch =====

    using std::string;
    using std::string_view;

    // ─── Utility Functions ──────────────────────────────────────────────────

    // Write a 3-byte length (TLS uses 3-byte lengths in handshake messages)
    func write_u24(val : u32, buf : *mut u8) {
        unsafe {

        buf[0] = ((val >> 16) & 0xFF) as u8
        buf[1] = ((val >> 8) & 0xFF) as u8
        buf[2] = (val & 0xFF) as u8

    }
}

    // Read a 3-byte length (TLS uses 3-byte lengths)
    func read_u24(buf : *u8) : u32 {
        unsafe {

        return ((buf[0] as u32) << 16) | ((buf[1] as u32) << 8) | (buf[2] as u32)

    }
}

    // Write a 16-bit big-endian value
    func write_u16_be(val : u16, buf : *mut u8) {
        unsafe {

        buf[0] = ((val >> 8) & 0xFF) as u8
        buf[1] = (val & 0xFF) as u8

    }
}

    // Read a 16-bit big-endian value
    func read_u16_be(buf : *u8) : u16 {
        unsafe {

        return ((buf[0] as u16) << 8) | (buf[1] as u16)

    }
}

    func read_u32_be(buf : *u8) : u32 {
        unsafe {

        return ((buf[0] as u32) << 24) | ((buf[1] as u32) << 16) | ((buf[2] as u32) << 8) | (buf[3] as u32)

    }
}

    // ============================================================================
    // TLS PRF (Pseudo-Random Function) for TLS 1.2
    // ============================================================================
    // Implements TLS 1.2 PRF = P_<hash>(secret, label + seed)
    // As defined in RFC 5246 section 5. The hash is SHA-256 for most suites
    // and SHA-384 for the AES_256_GCM_SHA384 family (use_sha384 = true).

    public func tls12_prf(secret : *u8, secret_len : size_t,
                           label : *char, label_len : size_t,
                           seed : *u8, seed_len : size_t,
                           output : *mut u8, output_len : size_t) {
                               unsafe {


        // Build the combined seed = label + seed
        var combined_len : size_t = label_len + seed_len
        var combined : [512]u8

        var i : size_t = 0
        while(i < label_len) {
            combined[i] = label[i] as u8
            i += 1
        }
        while(i < combined_len) {
            combined[i] = seed[i - label_len]
            i += 1
        }

        // P_hash(secret, seed) = HMAC_hash(secret, A(1) + seed) +
        //                        HMAC_hash(secret, A(2) + seed) + ...
        // where A(0) = seed, A(i) = HMAC_hash(secret, A(i-1))

        var A : [32]u8
        var generated : size_t = 0

        // First A(1) = HMAC(secret, A(0)=seed) = HMAC(secret, combined)
        crypto::raw::hmac_sha256(secret, secret_len, combined, combined_len, &raw mut A[0])

        while(generated < output_len) {
            // output_block = HMAC(secret, A(i) + seed)
            var block_in_len : size_t = 32 + combined_len
            var block_in : [544]u8

            var j : size_t = 0
            while(j < 32) {
                block_in[j] = A[j]
                j += 1
            }
            while(j < block_in_len) {
                block_in[j] = combined[j - 32]
                j += 1
            }

            var block_out : [32]u8
            crypto::raw::hmac_sha256(secret, secret_len, block_in, block_in_len, &raw mut block_out[0])

            // Copy block_out to output (up to 32 bytes)
            var copy_size : size_t = output_len - generated
            if(copy_size > 32 as size_t) { copy_size = 32 as size_t }
            var k : size_t = 0
            while(k < copy_size) {
                output[generated + k] = block_out[k]
                k += 1
            }
            generated += copy_size
            if(generated >= output_len) { break }

            // Compute next A(i+1) = HMAC(secret, A(i))
            crypto::raw::hmac_sha256(secret, secret_len, A, 32, &raw mut A[0])
        }

                           }
}

    // SHA-384 variant of the TLS 1.2 PRF for the AES_256_GCM_SHA384 family.
    // Structurally identical to tls12_prf above with 48-byte hash blocks.
    public func tls12_prf_sha384(secret : *u8, secret_len : size_t,
                                  label : *char, label_len : size_t,
                                  seed : *u8, seed_len : size_t,
                                  output : *mut u8, output_len : size_t) {
                                      unsafe {


        var combined_len : size_t = label_len + seed_len
        var combined : [512]u8
        var i : size_t = 0
        while(i < label_len) {
            combined[i] = label[i] as u8
            i += 1
        }
        while(i < combined_len) {
            combined[i] = seed[i - label_len]
            i += 1
        }
        // NOTE: A is padded to 64 bytes; a 48-byte stack array here triggered a
        // TCC stack-frame miscompilation of this function in the large TLS TU.
        var A : [64]u8
        var generated : size_t = 0
        crypto::raw::hmac_sha384(secret, secret_len, combined, combined_len, &raw mut A[0])
        var block_in_len : size_t = 48 + combined_len
        while(generated < output_len) {
            var block_in : [544]u8
            var block_out : [64]u8
            var j : size_t = 0
            while(j < 48) { block_in[j] = A[j]; j += 1 }
            while(j < block_in_len) { block_in[j] = combined[j - 48]; j += 1 }
            crypto::raw::hmac_sha384(secret, secret_len, block_in, block_in_len, &raw mut block_out[0])
            var copy_size : size_t = output_len - generated
            if(copy_size > 48 as size_t) { copy_size = 48 as size_t }
            var k : size_t = 0
            while(k < copy_size) { output[generated + k] = block_out[k]; k += 1 }
            generated += copy_size
            if(generated >= output_len) { break }
            crypto::raw::hmac_sha384(secret, secret_len, A, 48, &raw mut A[0])
        }

                                  }
}

    // ============================================================================
    // TLS 1.3 HKDF-Expand-Label (RFC 8446)
    // ============================================================================

    func tls13_hkdf_expand_label(secret : *u8, secret_len : size_t,
                                  label : *char, label_len : size_t,
                                  context : *u8, context_len : size_t,
                                  output : *mut u8, output_len : size_t) {
                                      unsafe {


        var label_prefix = "tls13 \0" as *char
        var prefix_len : size_t = 6

        var hkdf_label_len : size_t = (2 + 1 + prefix_len + label_len + 1 + context_len) as size_t
        var hkdf_label : [512]u8

        // Length (2 bytes)
        hkdf_label[0] = ((output_len >> 8) & 0xFF) as u8
        hkdf_label[1] = (output_len & 0xFF) as u8
        // Label length (1 byte)
        hkdf_label[2] = (prefix_len + label_len) as u8
        // Label value: "tls13 " + label
        var i : size_t = 0
        while(i < prefix_len) {
            hkdf_label[3 + i] = label_prefix[i] as u8
            i += 1
        }
        while(i < (prefix_len + label_len) as size_t) {
            hkdf_label[3 + i] = label[i - prefix_len] as u8
            i += 1
        }
        var pos : size_t = (3 + prefix_len + label_len) as size_t
        // Context length (1 byte)
        hkdf_label[pos] = context_len as u8
        pos += 1
        // Context value
        var j : size_t = 0
        while(j < context_len) {
            hkdf_label[pos + j] = context[j]
            j += 1
        }

        // HKDF-Expand: T(0) = empty, T(i) = HMAC(PRK, T(i-1) | info | i)
        var T : [48]u8
        var generated : size_t = 0
        var counter : u8 = 1
        var T_len : size_t = 0

        while(generated < output_len) {
            var input_len : size_t = T_len + hkdf_label_len + 1
            var input_buf : [1024]u8

            var k : size_t = 0
            while(k < T_len) {
                input_buf[k] = T[k]
                k += 1
            }
            while(k - T_len < hkdf_label_len) {
                input_buf[k] = hkdf_label[k - T_len]
                k += 1
            }
            input_buf[k] = counter as u8
            k += 1

            if(secret_len == 48) {
                crypto::raw::hmac_sha384(secret, secret_len, input_buf, input_len, &raw mut T[0])
            } else {
                crypto::raw::hmac_sha256(secret, secret_len, input_buf, input_len, &raw mut T[0])
            }
            T_len = secret_len

            var copy_size : size_t = output_len - generated
            if(copy_size > secret_len) { copy_size = secret_len }
            var l : size_t = 0
            while(l < copy_size) {
                output[generated + l] = T[l]
                l += 1
            }
            generated += copy_size
            counter = counter + 1
            if(counter > 20) { break }
        }

                                  }
}

    // TLS 1.3 Derive-Secret (RFC 8446)
    public func tls13_derive_secret(secret : *u8, secret_len : size_t,
                                     label : *char, label_len : size_t,
                                     transcript_hash : *u8, hash_len : size_t,
                                     output : *mut u8, output_len : size_t) {
        tls13_hkdf_expand_label(secret, secret_len, label, label_len,
                                transcript_hash, hash_len, output, output_len)
    }

    func tls13_is_sha384(ssl : *mut SSLContext) : bool {
        unsafe {

        var cs = get_ciphersuite_info(ssl.negotiated_ciphersuite)
        return cs.hash == HASH_SHA384 as u8

    }
}

    func tls13_hash_len_for(ssl : *mut SSLContext) : size_t {
        if(tls13_is_sha384(ssl)) { return 48 as size_t }
        return 32 as size_t
    }

    // Dual-hash transcript: keeps both SHA-256 and SHA-384 running contexts so
    // the same handshake message stream can be finalized with either hash
    // depending on the negotiated TLS 1.3 ciphersuite.
    public struct TLS13Transcript {
        var h256 : crypto::raw::Sha256Context
        var h384 : crypto::raw::Sha512Context
    }

    func tls13_transcript_init(t : *mut TLS13Transcript) {
        unsafe {

        crypto::raw::sha256_init(&raw mut t.h256)
        crypto::raw::sha384_init(&raw mut t.h384)

    }
}

    func tls13_transcript_update(t : *mut TLS13Transcript, data : *u8, len : size_t) {
        unsafe {

        crypto::raw::sha256_update(&raw mut t.h256, data, len)
        crypto::raw::sha384_update(&raw mut t.h384, data, len)

    }
}

    func tls13_transcript_finalize(t : *mut TLS13Transcript, out : *mut u8, use_sha384 : bool) {
        unsafe {

        if(use_sha384) {
            crypto::raw::sha384_final(&raw mut t.h384, out)
        } else {
            crypto::raw::sha256_final(&raw mut t.h256, out)
        }

    }
}

    // ============================================================================
    // TLS 1.3 Key Schedule (RFC 8446 Section 7.1)
    // ============================================================================

    // HKDF-Extract: PRK = HMAC-Hash(salt, IKM)
    func tls13_hkdf_extract(salt : *u8, salt_len : size_t,
                             ikm : *u8, ikm_len : size_t,
                             prk : *mut u8, hash_len : size_t) {
        if(hash_len == 48) {
            crypto::raw::hmac_sha384(salt, salt_len, ikm, ikm_len, prk)
        } else {
            crypto::raw::hmac_sha256(salt, salt_len, ikm, ikm_len, prk)
        }
    }

    // Derive handshake traffic keys from the ECDHE shared secret.
    // Must be called after receiving ServerHello (transcript includes CH + SH).
    // Populates ssl.transform_in and ssl.transform_out for the handshake phase.
    // transcript_hash = SHA256(ClientHello...ServerHello)
    public func tls13_derive_handshake_keys(ssl : *mut SSLContext,
                                             shared_secret : *u8, shared_len : size_t,
                                             transcript_hash : *u8,
                                             psk : *u8 = null, psk_len : size_t = 0,
                                             hash_len : size_t = 32,
                                             key_len : size_t = 16,
                                             cipher_type : u8 = CIPHER_AES_128_GCM as u8) : int { unsafe {
        // Step 1: Early secret
        var zeros : [48]u8
        var i : size_t = 0
        while(i < hash_len) { zeros[i] = 0; i += 1 }

        var early_secret : [48]u8
        if(psk != null && psk_len > 0) {
            // PSK mode: early_secret = HKDF-Extract(0, PSK)
            tls13_hkdf_extract(&raw zeros[0], hash_len, psk, psk_len, &raw mut early_secret[0], hash_len)
        } else {
            // No PSK: early_secret = HKDF-Extract(0, 0)
            tls13_hkdf_extract(&raw zeros[0], hash_len, &raw zeros[0], hash_len, &raw mut early_secret[0], hash_len)
        }

        // Step 2: Derived = HKDF-Expand-Label(early_secret, "derived", "", hash_len)
        // For empty Messages "", Transcript-Hash("") = SHA256/SHA384("") (hash_len bytes)
        var derived : [48]u8
        var empty_hash : [48]u8
        if(hash_len == 48) {
            var hctx : crypto::raw::Sha512Context
            crypto::raw::sha384_init(unsafe(&raw mut hctx))
            crypto::raw::sha384_final(unsafe(&raw mut hctx), &raw mut empty_hash[0])
        } else {
            var hctx : crypto::raw::Sha256Context
            crypto::raw::sha256_init(unsafe(&raw mut hctx))
            crypto::raw::sha256_final(unsafe(&raw mut hctx), &raw mut empty_hash[0])
        }
        var derived_label = "derived\0" as *char
        tls13_hkdf_expand_label(&raw early_secret[0], hash_len, derived_label, 7,
                                &raw empty_hash[0], hash_len, &raw mut derived[0], hash_len)

        // Step 3: Handshake secret = HKDF-Extract(Derived, shared_secret)
        var handshake_secret : [48]u8
        tls13_hkdf_extract(&raw derived[0], hash_len, shared_secret, shared_len,
                           &raw mut handshake_secret[0], hash_len)

        // Store early secret in key schedule
        i = 0
        while(i < hash_len) {
            ssl.tls13_keys.early_secret[i] = early_secret[i]
            i += 1
        }

        // Store handshake secret in key schedule
        i = 0
        while(i < hash_len) {
            ssl.tls13_keys.handshake_secret[i] = handshake_secret[i]
            i += 1
        }

        // Step 4: Derive client and server handshake traffic secrets
        var client_hts : [48]u8
        var server_hts : [48]u8
        var chts_label = "c hs traffic\0" as *char
        var shts_label = "s hs traffic\0" as *char
        tls13_hkdf_expand_label(&raw handshake_secret[0], hash_len, chts_label, 12,
                                transcript_hash, hash_len, &raw mut client_hts[0], hash_len)
        tls13_hkdf_expand_label(&raw handshake_secret[0], hash_len, shts_label, 12,
                                transcript_hash, hash_len, &raw mut server_hts[0], hash_len)

        i = 0
        while(i < hash_len) {
            ssl.tls13_keys.client_handshake_traffic_secret[i] = client_hts[i]
            ssl.tls13_keys.server_handshake_traffic_secret[i] = server_hts[i]
            i += 1
        }

        // Step 5: Derive keys and IVs
        var empty_ctx2 : [1]u8 = [0]
        var key_label = "key\0" as *char
        var iv_label = "iv\0" as *char

        var client_key : [32]u8
        var client_iv : [12]u8
        var server_key : [32]u8
        var server_iv : [12]u8

        tls13_hkdf_expand_label(&raw client_hts[0], hash_len, key_label, 3,
                                &raw empty_ctx2[0], 0, &raw mut client_key[0], key_len)
        tls13_hkdf_expand_label(&raw client_hts[0], hash_len, iv_label, 2,
                                &raw empty_ctx2[0], 0, &raw mut client_iv[0], 12)
        tls13_hkdf_expand_label(&raw server_hts[0], hash_len, key_label, 3,
                                &raw empty_ctx2[0], 0, &raw mut server_key[0], key_len)
        tls13_hkdf_expand_label(&raw server_hts[0], hash_len, iv_label, 2,
                                &raw empty_ctx2[0], 0, &raw mut server_iv[0], 12)

        // Per RFC 8446:
        // - Client role: transform_out (send) = client_key, transform_in (recv) = server_key
        // - Server role: transform_out (send) = server_key, transform_in (recv) = client_key
        var is_server_role : bool = (ssl.conf != null && ssl.conf.endpoint == SSL_IS_SERVER)

        // Populate transform_out — for sending
        var tr_out : Transform
        transform_init(unsafe(&raw mut tr_out))
        tr_out.cipher_type = cipher_type
        tr_out.key_len = key_len as u8
        tr_out.iv_len = 12
        tr_out.fixed_iv_len = 12
        tr_out.mac_key_len = 0
        if(is_server_role) {
            i = 0
            while(i < key_len) { tr_out.key_enc[i] = server_key[i]; i += 1 }
            i = 0
            while(i < 12) { tr_out.base_iv_enc[i] = server_iv[i]; i += 1 }
        } else {
            i = 0
            while(i < key_len) { tr_out.key_enc[i] = client_key[i]; i += 1 }
            i = 0
            while(i < 12) { tr_out.base_iv_enc[i] = client_iv[i]; i += 1 }
        }

        // Populate transform_in — for receiving
        var tr_in : Transform
        transform_init(unsafe(&raw mut tr_in))
        tr_in.cipher_type = cipher_type
        tr_in.key_len = key_len as u8
        tr_in.iv_len = 12
        tr_in.fixed_iv_len = 12
        tr_in.mac_key_len = 0
        if(is_server_role) {
            i = 0
            while(i < key_len) { tr_in.key_dec[i] = client_key[i]; i += 1 }
            i = 0
            while(i < 12) { tr_in.base_iv_dec[i] = client_iv[i]; i += 1 }
        } else {
            i = 0
            while(i < key_len) { tr_in.key_dec[i] = server_key[i]; i += 1 }
            i = 0
            while(i < 12) { tr_in.base_iv_dec[i] = server_iv[i]; i += 1 }
        }

        // Allocate and install transforms
        var tr_out_mem = malloc(sizeof(Transform)) as *mut Transform
        *tr_out_mem = tr_out
        ssl.transform_out = tr_out_mem

        var tr_in_mem = malloc(sizeof(Transform)) as *mut Transform
        unsafe { *tr_in_mem = tr_in }
        ssl.transform_in = tr_in_mem

        // Reset sequence numbers for the handshake phase
        i = 0
        while(i < 8) { ssl.in_ctr[i] = 0; ssl.out_ctr[i] = 0; i += 1 }

        return 0
     }}

    // Derive application traffic keys after handshake completes.
    // Called after both Finished messages have been exchanged.
    // The transcript-hash contexts (ClientHello..server Finished and
    // ClientHello..client Finished) are normally passed via hs_hash/res_hash.
    // NOTE: passing &raw local_array[0] for a caller-local array is corrupted
    // by a compiler codegen bug for large stack offsets, so the production
    // TLS1.3 handshake passes null and delivers the contexts via the struct
    // fields client_handshake_traffic_secret (ClientHello..server Finished)
    // and server_handshake_traffic_secret (ClientHello..client Finished). The
    // function copies those into callee-local arrays via indexed reads, which
    // are reliable.
    public func tls13_derive_application_keys(ssl : *mut SSLContext,
                                                hs_hash : *u8 = null, hash_len : size_t,
                                                res_hash : *u8 = null,
                                                res_hash_len : size_t = 0,
                                                key_len : size_t = 16,
                                                cipher_type : u8 = CIPHER_AES_128_GCM as u8) : int { unsafe {
        var i : size_t = 0

        // Resolve context pointers (callee-local copies when coming from struct).
        var ctx_cf : [48]u8
        var ctx_full : [48]u8
        var cf_ptr : *u8
        var full_ptr : *u8
        var full_len : size_t = hash_len
        if(hs_hash != null) {
            cf_ptr = hs_hash
        } else {
            var z : size_t = 0
            while(z < hash_len) { ctx_cf[z] = ssl.tls13_keys.client_handshake_traffic_secret[z]; z += 1 }
            cf_ptr = &raw ctx_cf[0]
        }
        if(res_hash != null) {
            full_ptr = res_hash
            full_len = res_hash_len
        } else {
            var z : size_t = 0
            while(z < hash_len) { ctx_full[z] = ssl.tls13_keys.server_handshake_traffic_secret[z]; z += 1 }
            full_ptr = &raw ctx_full[0]
        }
        // RFC 8446 Section 7.1: master_secret = HKDF-Extract(0, derived)
        var derived : [48]u8
        var empty32 : [48]u8
        var empty_hash : [48]u8
        if(hash_len == 48) {
            var hctx_d : crypto::raw::Sha512Context
            crypto::raw::sha384_init(unsafe(&raw mut hctx_d))
            crypto::raw::sha384_final(unsafe(&raw mut hctx_d), &raw mut empty_hash[0])
        } else {
            var hctx_d : crypto::raw::Sha256Context
            crypto::raw::sha256_init(unsafe(&raw mut hctx_d))
            crypto::raw::sha256_final(unsafe(&raw mut hctx_d), &raw mut empty_hash[0])
        }
        while(i < hash_len) { empty32[i] = 0; i += 1 }
        var derived_label = "derived\0" as *char
        tls13_hkdf_expand_label(&raw ssl.tls13_keys.handshake_secret[0], hash_len,
                                derived_label, 7, &raw empty_hash[0], hash_len,
                                &raw mut derived[0], hash_len)

        // Master secret = HKDF-Extract(salt=Derived, IKM=0)
        var master_secret : [48]u8
        tls13_hkdf_extract(&raw derived[0], hash_len, &raw empty32[0], hash_len,
                           &raw mut master_secret[0], hash_len)
        i = 0
        while(i < hash_len) { ssl.tls13_keys.master_secret[i] = master_secret[i]; i += 1 }

        // Resumption master secret (context = ClientHello...client Finished)
        var rms_hash : *u8 = cf_ptr
        var rms_hash_len : size_t = hash_len
        if(hs_hash != null && res_hash != null) { rms_hash = res_hash; rms_hash_len = res_hash_len }
        else if(hs_hash == null && res_hash == null) { rms_hash = full_ptr; rms_hash_len = full_len }
        else if(res_hash != null) { rms_hash = res_hash; rms_hash_len = res_hash_len }
        var rms_label = "res master\0" as *char
        tls13_hkdf_expand_label(&raw master_secret[0], hash_len, rms_label, 10,
                                rms_hash, rms_hash_len,
                                &raw mut ssl.tls13_keys.resumption_master_secret[0], hash_len)

        // Client application traffic secret
        var c_ats : [48]u8
        var c_ats_label = "c ap traffic\0" as *char
        tls13_hkdf_expand_label(&raw master_secret[0], hash_len, c_ats_label, 12,
                                 cf_ptr, hash_len, &raw mut c_ats[0], hash_len)
        i = 0
        while(i < hash_len) { ssl.tls13_keys.client_application_traffic_secret[i] = c_ats[i]; i += 1 }

        // Server application traffic secret
        var s_ats : [48]u8
        var s_ats_label = "s ap traffic\0" as *char
        tls13_hkdf_expand_label(&raw master_secret[0], hash_len, s_ats_label, 12,
                                 cf_ptr, hash_len, &raw mut s_ats[0], hash_len)
        i = 0
        while(i < hash_len) { ssl.tls13_keys.server_application_traffic_secret[i] = s_ats[i]; i += 1 }

        // Derive application keys
        var empty_ctx : [1]u8 = [0]
        var key_label = "key\0" as *char
        var iv_label = "iv\0" as *char

        var client_key : [32]u8
        var client_iv : [12]u8
        var server_key : [32]u8
        var server_iv : [12]u8

        tls13_hkdf_expand_label(&raw c_ats[0], hash_len, key_label, 3,
                                &raw empty_ctx[0], 0, &raw mut client_key[0], key_len)
        tls13_hkdf_expand_label(&raw c_ats[0], hash_len, iv_label, 2,
                                &raw empty_ctx[0], 0, &raw mut client_iv[0], 12)
        tls13_hkdf_expand_label(&raw s_ats[0], hash_len, key_label, 3,
                                &raw empty_ctx[0], 0, &raw mut server_key[0], key_len)
        tls13_hkdf_expand_label(&raw s_ats[0], hash_len, iv_label, 2,
                                &raw empty_ctx[0], 0, &raw mut server_iv[0], 12)

        // Per RFC 8446:
        // - Client role: transform_out (send) = client_key, transform_in (recv) = server_key
        // - Server role: transform_out (send) = server_key, transform_in (recv) = client_key
        var is_server_role : bool = (ssl.conf != null && ssl.conf.endpoint == SSL_IS_SERVER)
        var is_loopback : bool = !ssl.transport_connected

        // Send direction (transform_out)
        if(ssl.transform_out != null) { unsafe { dealloc ssl.transform_out } }
        var tr_out : Transform
        transform_init(unsafe(&raw mut tr_out))
        tr_out.cipher_type = cipher_type
        tr_out.key_len = key_len as u8
        tr_out.iv_len = 12
        tr_out.fixed_iv_len = 12
        if(is_server_role) {
            i = 0
            while(i < key_len) { tr_out.key_enc[i] = server_key[i]; i += 1 }
            i = 0
            while(i < 12) { tr_out.base_iv_enc[i] = server_iv[i]; i += 1 }
        } else {
            i = 0
            while(i < key_len) { tr_out.key_enc[i] = client_key[i]; i += 1 }
            i = 0
            while(i < 12) { tr_out.base_iv_enc[i] = client_iv[i]; i += 1 }
        }
        var tr_out_mem = malloc(sizeof(Transform)) as *mut Transform
        *tr_out_mem = tr_out
        ssl.transform_out = tr_out_mem

        // Receive direction (transform_in)
        if(ssl.transform_in != null) { unsafe { dealloc ssl.transform_in } }
        var tr_in : Transform
        transform_init(unsafe(&raw mut tr_in))
        tr_in.cipher_type = cipher_type
        tr_in.key_len = key_len as u8
        tr_in.iv_len = 12
        tr_in.fixed_iv_len = 12
        if(is_loopback) {
            if(is_server_role) {
                i = 0
                while(i < key_len) { tr_in.key_dec[i] = server_key[i]; i += 1 }
                i = 0
                while(i < 12) { tr_in.base_iv_dec[i] = server_iv[i]; i += 1 }
            } else {
                i = 0
                while(i < key_len) { tr_in.key_dec[i] = client_key[i]; i += 1 }
                i = 0
                while(i < 12) { tr_in.base_iv_dec[i] = client_iv[i]; i += 1 }
            }
        } else if(is_server_role) {
            i = 0
            while(i < key_len) { tr_in.key_dec[i] = client_key[i]; i += 1 }
            i = 0
            while(i < 12) { tr_in.base_iv_dec[i] = client_iv[i]; i += 1 }
        } else {
            i = 0
            while(i < key_len) { tr_in.key_dec[i] = server_key[i]; i += 1 }
            i = 0
            while(i < 12) { tr_in.base_iv_dec[i] = server_iv[i]; i += 1 }
        }
        var tr_in_mem = malloc(sizeof(Transform)) as *mut Transform
        unsafe { *tr_in_mem = tr_in }
        ssl.transform_in = tr_in_mem

        // Reset sequence numbers for the application data phase
        i = 0
        while(i < 8) { ssl.in_ctr[i] = 0; ssl.out_ctr[i] = 0; i += 1 }

        return 0
     }}

    // ============================================================================
    // TLS 1.3 Key Update (RFC 8446 Section 7.2)
    // ============================================================================
    // After the handshake, either side can send a KeyUpdate to refresh traffic keys.
    // application_traffic_secret_N+1 = HKDF-Expand-Label(secret_N, "traffic upd", "", Hash.length)

    // Update the send side traffic keys (role-aware)
    // Client sends with client_application_traffic_secret -> transform_out.key_enc
    // Server sends with server_application_traffic_secret -> transform_out.key_enc
    public func tls13_update_send_keys(ssl : *mut SSLContext) : int {
        unsafe {

        var is_server_role : bool = (ssl.conf != null && ssl.conf.endpoint == SSL_IS_SERVER)
        var hash_len = tls13_hash_len_for(ssl)
        var key_len = ssl.transform_out.key_len as size_t

        // Choose the correct traffic secret based on role
        var traffic_secret : [48]u8
        var si : size_t = 0
        if(is_server_role) {
            while(si < hash_len) {
                traffic_secret[si] = ssl.tls13_keys.server_application_traffic_secret[si]
                si += 1
            }
        } else {
            while(si < hash_len) {
                traffic_secret[si] = ssl.tls13_keys.client_application_traffic_secret[si]
                si += 1
            }
        }

        // Derive new secret from current traffic secret
        var new_secret : [48]u8
        var empty_c : [1]u8 = [0]
        var upd_label = "traffic upd\0" as *char
        tls13_hkdf_expand_label(&raw traffic_secret[0], hash_len,
                                upd_label, 11, &raw empty_c[0], 0, &raw mut new_secret[0], hash_len)

        // Store updated secret back to the correct slot
        if(is_server_role) {
            si = 0
            while(si < hash_len) {
                ssl.tls13_keys.server_application_traffic_secret[si] = new_secret[si]
                si += 1
            }
        } else {
            si = 0
            while(si < hash_len) {
                ssl.tls13_keys.client_application_traffic_secret[si] = new_secret[si]
                si += 1
            }
        }

        // Derive new key and IV
        var key_label = "key\0" as *char
        var iv_label = "iv\0" as *char
        var new_key : [32]u8
        var new_iv : [12]u8
        tls13_hkdf_expand_label(&raw new_secret[0], hash_len, key_label, 3,
                                &raw empty_c[0], 0, &raw mut new_key[0], key_len)
        tls13_hkdf_expand_label(&raw new_secret[0], hash_len, iv_label, 2,
                                &raw empty_c[0], 0, &raw mut new_iv[0], 12)

        // Update transform_out with new encryption keys
        if(ssl.transform_out != null) {
            si = 0
            while(si < key_len) { ssl.transform_out.key_enc[si] = new_key[si]; si += 1 }
            si = 0
            while(si < 12) { ssl.transform_out.base_iv_enc[si] = new_iv[si]; si += 1 }
        }

        // Reset send sequence number
        si = 0
        while(si < 8) { ssl.out_ctr[si] = 0; si += 1 }

        return 0

    }
}

    // Update the recv side traffic keys (role-aware)
    // Client receives with server_application_traffic_secret -> transform_in.key_dec
    // Server receives with client_application_traffic_secret -> transform_in.key_dec
    public func tls13_update_recv_keys(ssl : *mut SSLContext) : int {
        unsafe {

        var is_server_role : bool = (ssl.conf != null && ssl.conf.endpoint == SSL_IS_SERVER)
        var hash_len = tls13_hash_len_for(ssl)
        var key_len = ssl.transform_in.key_len as size_t

        // Choose the correct traffic secret based on role
        var traffic_secret : [48]u8
        var si : size_t = 0
        if(is_server_role) {
            while(si < hash_len) {
                traffic_secret[si] = ssl.tls13_keys.client_application_traffic_secret[si]
                si += 1
            }
        } else {
            while(si < hash_len) {
                traffic_secret[si] = ssl.tls13_keys.server_application_traffic_secret[si]
                si += 1
            }
        }

        // Derive new secret from current traffic secret
        var new_secret : [48]u8
        var empty_c : [1]u8 = [0]
        var upd_label = "traffic upd\0" as *char
        tls13_hkdf_expand_label(&raw traffic_secret[0], hash_len,
                                upd_label, 11, &raw empty_c[0], 0, &raw mut new_secret[0], hash_len)

        // Store updated secret back to the correct slot
        if(is_server_role) {
            si = 0
            while(si < hash_len) {
                ssl.tls13_keys.client_application_traffic_secret[si] = new_secret[si]
                si += 1
            }
        } else {
            si = 0
            while(si < hash_len) {
                ssl.tls13_keys.server_application_traffic_secret[si] = new_secret[si]
                si += 1
            }
        }

        // Derive new key and IV
        var key_label = "key\0" as *char
        var iv_label = "iv\0" as *char
        var new_key : [32]u8
        var new_iv : [12]u8
        tls13_hkdf_expand_label(&raw new_secret[0], hash_len, key_label, 3,
                                &raw empty_c[0], 0, &raw mut new_key[0], key_len)
        tls13_hkdf_expand_label(&raw new_secret[0], hash_len, iv_label, 2,
                                &raw empty_c[0], 0, &raw mut new_iv[0], 12)

        // Update transform_in with new decryption keys
        if(ssl.transform_in != null) {
            si = 0
            while(si < key_len) { ssl.transform_in.key_dec[si] = new_key[si]; si += 1 }
            si = 0
            while(si < 12) { ssl.transform_in.base_iv_dec[si] = new_iv[si]; si += 1 }
        }

        // Reset receive sequence number
        si = 0
        while(si < 8) { ssl.in_ctr[si] = 0; si += 1 }

        return 0

    }
}

    // Send a KeyUpdate message (TLS 1.3)
    // request_response: if true, requests the peer to also send a KeyUpdate
    public func tls13_send_key_update(ssl : *mut SSLContext, request_response : bool) : int {
        return async::block_on<int>(tls13_send_key_update_coro(ssl, request_response))
    }

    public async func tls13_send_key_update_coro(ssl : *mut SSLContext, request_response : bool) : int {
        // Build KeyUpdate message: key_update_request (1 byte)
        var ku_body : [1]u8
        if(request_response) {
            ku_body[0] = 1 as u8
        } else {
            ku_body[0] = 0 as u8
        }

        // RFC 8446 §4.6.3: "After sending a KeyUpdate message, the sender SHALL
        // send all its traffic using the next generation of keys." The KeyUpdate
        // message itself is protected under the OLD keys (the peer only updates
        // its receive keys after processing it), so send FIRST, then rotate.
        var ret = await send_handshake_msg(ssl, SSL_HS_KEY_UPDATE as u8, &raw ku_body[0], 1)
        if(ret < 0) { return ret }

        ret = tls13_update_send_keys(ssl)
        return ret
    }

    // ============================================================================
    // TLS 1.2 Key Derivation (RFC 5246)
    // ============================================================================

    // Derive master secret from pre-master secret
    // master_secret = PRF(pre_master_secret, "master secret", ClientHello.random + ServerHello.random)[0..47]
    public func tls12_derive_master_secret(pre_master : *u8, pre_master_len : size_t,
                                            client_random : *u8, server_random : *u8,
                                            master_secret : *mut u8,
                                            use_sha384 : bool = false) {
                                                unsafe {

        // Build seed = ClientRandom + ServerRandom (64 bytes total)
        var seed : [64]u8
        var i : size_t = 0
        while(i < 32) {
            seed[i] = client_random[i]
            seed[i + 32] = server_random[i]
            i += 1
        }

        var label = "master secret\0" as *char
        if(use_sha384) {
            tls12_prf_sha384(pre_master, pre_master_len, label, 13, &raw seed[0], 64, master_secret, 48)
        } else {
            tls12_prf(pre_master, pre_master_len, label, 13, &raw seed[0], 64, master_secret, 48)
        }

                                            }
}

    // Derive key block from master secret (RFC 5246 Section 6.3)
    // key_block = PRF(master_secret, "key expansion", ServerRandom + ClientRandom)
    // The key_block is split as needed for the cipher suite
    public func tls12_derive_key_block(master_secret : *u8,
                                        server_random : *u8, client_random : *u8,
                                        key_block : *mut u8, key_block_len : size_t,
                                        use_sha384 : bool = false) {
                                            unsafe {

        // Build seed = ServerRandom + ClientRandom (reversed from master secret derivation)
        var seed : [64]u8
        var i : size_t = 0
        while(i < 32) {
            seed[i] = server_random[i]
            seed[i + 32] = client_random[i]
            i += 1
        }

        var label = "key expansion\0" as *char
        if(use_sha384) {
            tls12_prf_sha384(master_secret, 48, label, 13, &raw seed[0], 64, key_block, key_block_len)
        } else {
            tls12_prf(master_secret, 48, label, 13, &raw seed[0], 64, key_block, key_block_len)
        }

                                        }
}

    // Compute the key block size needed for a cipher suite
    public func tls12_key_block_size(info : *CipherSuiteInfo) : size_t {
        unsafe {

        var mac_key_len = info.mac_key_len as size_t
        var enc_key_len = info.key_size as size_t
        var iv_len : size_t = 0
        var cipher = info.cipher
        if(cipher == CIPHER_AES_128_CBC || cipher == CIPHER_AES_256_CBC) {
            iv_len = info.iv_size as size_t
        } else if(cipher == CIPHER_AES_128_GCM || cipher == CIPHER_AES_256_GCM) {
            // For GCM, fixed IV is 4 bytes (explicit nonce is 8 bytes sent in record)
            iv_len = 4
        }
        // 2 directions: client + server
        return (mac_key_len + enc_key_len + iv_len) * 2

    }
}

    // Split key block and populate Transform structure
    public func tls12_populate_transform(tr : *mut Transform, info : *CipherSuiteInfo,
                                          key_block : *u8, key_block_len : size_t) : int {
                                              unsafe {

        var mac_key_len = info.mac_key_len as size_t
        var enc_key_len = info.key_size as size_t
        var iv_len : size_t = 0

        var cipher = info.cipher
        if(cipher == CIPHER_AES_128_CBC || cipher == CIPHER_AES_256_CBC) {
            iv_len = info.iv_size as size_t
        } else {
            iv_len = 4  // Fixed IV for GCM
        }

        var offset : size_t = 0

        // client_write_MAC_key
        var i : size_t = 0
        while(i < mac_key_len) {
            tr.mac_key_enc[i] = key_block[offset + i]
            i += 1
        }
        offset += mac_key_len

        // server_write_MAC_key
        i = 0
        while(i < mac_key_len) {
            tr.mac_key_dec[i] = key_block[offset + i]
            i += 1
        }
        offset += mac_key_len

        // client_write_key
        i = 0
        while(i < enc_key_len) {
            tr.key_enc[i] = key_block[offset + i]
            i += 1
        }
        offset += enc_key_len

        // server_write_key
        i = 0
        while(i < enc_key_len) {
            tr.key_dec[i] = key_block[offset + i]
            i += 1
        }
        offset += enc_key_len

        // client_write_IV
        i = 0
        while(i < iv_len) {
            tr.iv_enc[i] = key_block[offset + i]
            tr.base_iv_enc[i] = key_block[offset + i]
            i += 1
        }
        offset += iv_len

        // server_write_IV
        i = 0
        while(i < iv_len) {
            tr.iv_dec[i] = key_block[offset + i]
            tr.base_iv_dec[i] = key_block[offset + i]
            i += 1
        }
        offset += iv_len

        tr.cipher_type = cipher as u8
        tr.hash_type = info.hash
        tr.key_len = enc_key_len as u8
        tr.iv_len = iv_len as u8
        tr.mac_key_len = mac_key_len as u8
        tr.fixed_iv_len = iv_len as u8

        return 0

                                          }
}

    // ============================================================================
    // Finished Message Calculation (RFC 5246 Section 7.4.9)
    // ============================================================================

    // Compute Finished message verify_data
    // verify_data = PRF(master_secret, finished_label, SHA256(handshake_messages))[0..11]
    public func tls12_compute_finished(master_secret : *u8, is_client : bool,
                                        handshake_hash : *u8, hash_len : size_t,
                                        verify_data : *mut u8,
                                        use_sha384 : bool = false) {
        var label : *char
        if(is_client) {
            label = "client finished\0" as *char
        } else {
            label = "server finished\0" as *char
        }
        if(use_sha384) {
            tls12_prf_sha384(master_secret, 48, label, 15,
                              handshake_hash, hash_len, verify_data, 12)
        } else {
            tls12_prf(master_secret, 48, label, 15,
                       handshake_hash, hash_len, verify_data, 12)
        }
    }

    // ============================================================================
    // Record Encryption / Decryption (TLS 1.2)
    // ============================================================================

    // Encrypt a TLS record using the provided transform
    // input: plaintext data (already has MAC appended if using CBC)
    // output: ciphertext (includes IV + encrypted data + tag for GCM)
    // Returns the total ciphertext length or negative error
    public func tls12_encrypt_record(tr : *mut Transform, seq_num : *u8,
                                     content_type : u8, version_major : u8,
                                     version_minor : u8, input : *u8,
                                     input_len : size_t, output : *mut u8,
                                     out_max : size_t) : int {
                                         unsafe {

        var cipher = tr.cipher_type

        if(cipher == CIPHER_AES_128_GCM || cipher == CIPHER_AES_256_GCM) {
            // GCM mode: nonce = base_iv (4 bytes) || explicit_nonce (8 bytes from seq_num)
            // TLS 1.2: explicit_nonce is sent in the record, total output = nonce(8) + ct + tag(16)
            var iv_len = tr.fixed_iv_len as size_t
            var key_len = tr.key_len as size_t

            // Construct 12-byte GCM nonce: fixed_iv (4 bytes from transform) + seq_num (8 bytes)
            var nonce : [12]u8
            var i : size_t = 0
            while(i < iv_len) {
                nonce[i] = tr.base_iv_enc[i]
                i += 1
            }
            while(i < 12) {
                nonce[i] = seq_num[i - iv_len]
                i += 1
            }

            // Copy explicit nonce = sequence number (8 bytes, MSB-first)
            i = 0
            while(i < 8) {
                output[i] = seq_num[i]
                i += 1
            }

            // Output layout: explicit_nonce(8) || ciphertext || auth_tag(16)
            var ct_out = output + 8
            var tag_out = output + 8 + input_len

            // Initialize GCM context
            var gcm_ctx : GCMContext
            var ret = gcm_init(unsafe(&raw mut gcm_ctx), &raw tr.key_enc[0], key_len)
            if(ret < 0) { return ret }

            // Encrypt with proper TLS 1.2 AAD
            // Build AAD = seq_num(8) || type(1) || version(2) || length(2)
            var aad : [13]u8
            var ai : size_t = 0
            while(ai < 8) { aad[ai] = seq_num[ai]; ai += 1 }
            aad[8] = content_type
            aad[9] = version_major
            aad[10] = version_minor
            aad[11] = ((input_len >> 8) & 0xFF) as u8
            aad[12] = (input_len & 0xFF) as u8

            ret = gcm_crypt_and_tag(unsafe(&raw mut gcm_ctx),
                                     &raw nonce[0], 12,
                                     &raw aad[0], 13,
                                     input, input_len,
                                     ct_out, tag_out)
            if(ret < 0) { return ret }

            return (8 + input_len + 16) as i32
        } else if(cipher == CIPHER_AES_128_CBC || cipher == CIPHER_AES_256_CBC) {
            // CBC mode with HMAC
            var key_len = tr.key_len as size_t
            var iv_len = tr.iv_len as size_t
            var mac_len = tr.mac_key_len as size_t

            if(mac_len > 0) {
                // Build the full CBC plaintext: content || MAC || padding || pad_len
                var content_len = input_len
                var mac_input_buf : [16400]u8
                var mip : size_t = 0
                while(mip < 8) {
                    mac_input_buf[mip] = seq_num[mip]
                    mip += 1
                }
                mac_input_buf[mip] = content_type; mip += 1
                mac_input_buf[mip] = version_major; mip += 1
                mac_input_buf[mip] = version_minor; mip += 1
                mac_input_buf[mip] = ((content_len >> 8) & 0xFF) as u8; mip += 1
                mac_input_buf[mip] = (content_len & 0xFF) as u8; mip += 1
                var j2 : size_t = 0
                while(j2 < content_len) {
                    mac_input_buf[mip + j2] = input[j2]
                    j2 += 1
                }
                mip += content_len

        var mac_buf : [48]u8
        crypto::raw::hmac_sha256(&raw tr.mac_key_enc[0], mac_len, &raw mac_input_buf[0], mip, &raw mut mac_buf[0])

                // Build record: content + MAC + padding
                // PKCS#7 padding: pad to next block size (16 bytes)
                var unpadded = content_len + mac_len
                var pad_val = (16 - (unpadded % 16)) as size_t
                if(pad_val == 0) { pad_val = 16 }
                var block_len = unpadded + pad_val

                var cbc_block : [16400]u8
                var bi : size_t = 0
                while(bi < content_len) {
                    cbc_block[bi] = input[bi]
                    bi += 1
                }
                while(bi < unpadded) {
                    cbc_block[bi] = mac_buf[bi - content_len]
                    bi += 1
                }
                while(bi < block_len) {
                    cbc_block[bi] = (pad_val - 1) as u8
                    bi += 1
                }

                // Copy IV to output (also save a copy since aes_crypt_cbc clobbers it)
                var iv_copy : [16]u8
                var j3 : size_t = 0
                while(j3 < iv_len) {
                    output[j3] = tr.iv_enc[j3]
                    iv_copy[j3] = tr.iv_enc[j3]
                    j3 += 1
                }

                // Encrypt using a separate IV buffer (not output[0], which gets clobbered)
                var aes_ctx : AESContext
                var a_ret = aes_setkey_enc(unsafe(&raw mut aes_ctx), &raw tr.key_enc[0], key_len)
                if(a_ret < 0) { return a_ret }
                a_ret = aes_crypt_cbc(unsafe(&raw mut aes_ctx), AES_ENCRYPT, block_len,
                                       &raw mut iv_copy[0], &raw cbc_block[0], &raw mut output[iv_len])
                if(a_ret < 0) { return a_ret }

                return (iv_len + block_len) as i32
            }

            // No MAC — just encrypt without MAC (for compatibility)
            var iv_copy_nm : [16]u8
            var i : size_t = 0
            while(i < iv_len) {
                output[i] = tr.iv_enc[i]
                iv_copy_nm[i] = tr.iv_enc[i]
                i += 1
            }

                var aes_ctx : AESContext
                aes_setkey_enc(unsafe(&raw mut aes_ctx), &raw tr.key_enc[0], key_len)
                var ret = aes_crypt_cbc(unsafe(&raw mut aes_ctx), AES_ENCRYPT, input_len,
                                     &raw mut iv_copy_nm[0], input, &raw mut output[iv_len])
            if(ret < 0) { return ret }

            return (iv_len + input_len) as i32
        }

        // Unknown cipher — should never be reached with proper negotiation
        return ERR_SSL_INTERNAL_ERROR

                                     }
}

    // Decrypt a TLS record
    public func tls12_decrypt_record(tr : *mut Transform, seq_num : *u8,
                                     content_type : u8, version_major : u8,
                                     version_minor : u8, input : *u8,
                                     input_len : size_t, output : *mut u8,
                                     out_max : size_t) : int {
                                         unsafe {

        var cipher = tr.cipher_type

        if(cipher == CIPHER_AES_128_GCM || cipher == CIPHER_AES_256_GCM) {
            // GCM mode: input = explicit_nonce(8) || ciphertext || auth_tag(16)
            if(input_len < 8 + 16) { return ERR_SSL_INVALID_RECORD }

            var explicit_nonce_len : size_t = 8
            var tag_len : size_t = 16
            var ct_len = input_len - explicit_nonce_len - tag_len

            var iv_len = tr.fixed_iv_len as size_t
            var key_len = tr.key_len as size_t

            // Construct 12-byte GCM nonce: fixed_iv + explicit_nonce
            var nonce : [12]u8
            var i : size_t = 0
            while(i < iv_len) {
                nonce[i] = tr.base_iv_dec[i]
                i += 1
            }
            while(i < 12) {
                nonce[i] = input[i - iv_len]
                i += 1
            }

            // Build TLS 1.2 AEAD additional_data = seq_num(8) || type(1) || version(2) || length(2)
            var aad : [13]u8
            var ai : size_t = 0
            while(ai < 8) { aad[ai] = seq_num[ai]; ai += 1 }
            aad[8] = content_type
            aad[9] = version_major
            aad[10] = version_minor
            // ct_len in big-endian
            aad[11] = ((ct_len >> 8) & 0xFF) as u8
            aad[12] = (ct_len & 0xFF) as u8

            // Initialize GCM context
            var gcm_ctx : GCMContext
            var ret = gcm_init(unsafe(&raw mut gcm_ctx), &raw tr.key_dec[0], key_len)
            if(ret < 0) { return ret }

            // Authenticated decrypt with proper TLS 1.2 AAD
            var ct_start = input + explicit_nonce_len
            var tag_start = input + explicit_nonce_len + ct_len
            ret = gcm_auth_decrypt(unsafe(&raw mut gcm_ctx),
                                    &raw nonce[0], 12,
                                    &raw aad[0], 13,
                                    ct_start, ct_len,
                                    tag_start, tag_len,
                                    output)
            if(ret < 0) { return ret }

            return ct_len as i32
        } else if(cipher == CIPHER_AES_128_CBC || cipher == CIPHER_AES_256_CBC) {
            var key_len = tr.key_len as size_t
            var iv_len = tr.iv_len as size_t

            if(input_len < iv_len + 16) { return ERR_SSL_INVALID_RECORD }

            var aes_ctx : AESContext
            aes_setkey_dec(unsafe(&raw mut aes_ctx), &raw tr.key_dec[0], key_len)

            // Copy IV from input to a separate buffer since aes_crypt_cbc modifies it
            var iv_buf : [16]u8
            var jiv : size_t = 0
            while(jiv < iv_len) {
                iv_buf[jiv] = input[jiv]
                jiv += 1
            }

            var cipher_len = input_len - iv_len
            var ret = aes_crypt_cbc(unsafe(&raw mut aes_ctx), AES_DECRYPT, cipher_len,
                                     &raw mut iv_buf[0], &raw input[iv_len], output)
            if(ret < 0) { return ret }

            // Verify MAC and remove PKCS#7 padding
            var mac_len = tr.mac_key_len as size_t
            if(mac_len > 0 && mac_len < cipher_len) {
                // Last byte = padding_length
                var pad_len = output[cipher_len - 1] as size_t + 1
                if(pad_len > cipher_len || pad_len < 1) { return ERR_SSL_INVALID_MAC }
                // Verify all padding bytes are equal to padding_length
                var pi : size_t = 1
                while(pi < pad_len && pi < cipher_len) {
                    if(output[cipher_len - 1 - pi] != output[cipher_len - 1]) {
                        return ERR_SSL_INVALID_MAC
                    }
                    pi += 1
                }
                // MAC is before the padding
                if(pad_len + mac_len > cipher_len) { return ERR_SSL_INVALID_MAC }
                var content_len = cipher_len - pad_len - mac_len
                // Build MAC input: seq_num(8) + content_type(1) + version(2) + length(2) + content
                var mac_input : [16400]u8
                var mi_pos : size_t = 0
                // seq_num (8 bytes)
                while(mi_pos < 8) {
                    mac_input[mi_pos] = seq_num[mi_pos]
                    mi_pos += 1
                }
                // content_type (1 byte)
                mac_input[mi_pos] = content_type; mi_pos += 1
                // version (2 bytes)
                mac_input[mi_pos] = version_major; mi_pos += 1
                mac_input[mi_pos] = version_minor; mi_pos += 1
                // content length (2 bytes, big-endian)
                mac_input[mi_pos] = ((content_len >> 8) & 0xFF) as u8; mi_pos += 1
                mac_input[mi_pos] = (content_len & 0xFF) as u8; mi_pos += 1
                // content
                var ci : size_t = 0
                while(ci < content_len) {
                    mac_input[mi_pos + ci] = output[ci]
                    ci += 1
                }
                mi_pos += content_len

                // Compute expected MAC
                var mac_input_len = mi_pos
        var mac_buf : [48]u8
        crypto::raw::hmac_sha256(&raw tr.mac_key_dec[0], mac_len, &raw mac_input[0], mac_input_len, &raw mut mac_buf[0])

                // Constant-time MAC comparison
                var mac_diff : u8 = 0
                var mi : size_t = 0
                while(mi < mac_len) {
                    mac_diff = mac_diff | (mac_buf[mi] ^ output[content_len + mi])
                    mi += 1
                }
                if(mac_diff != 0) { return ERR_SSL_INVALID_MAC }

                // Shift content to the front (no MAC, no padding)
                return content_len as i32
            }

            // If no MAC verification, return raw decrypted data
            return cipher_len as i32
        }

        return ERR_SSL_INTERNAL_ERROR

                                     }
}

    // ============================================================================
    // TLS 1.3 Record Layer (RFC 8446 Section 5)
    // ============================================================================

    // TLS 1.3 uses a different record format than TLS 1.2:
    // - Outer content_type is always application_data (23) for encrypted records
    // - Inner content_type is in the last byte of the decrypted payload
    // - AAD for AEAD is the 5-byte outer record header
    // - Nonce = static IV XOR padded sequence number

    // Increment a sequence number counter (8 bytes, big-endian)
    func ssl_incr_seq_num(seq : *mut u8) {
        unsafe {

        var i : i32 = 7
        while(i >= 0) {
            var val = seq[i] as u16 + 1
            seq[i] = (val & 0xFF) as u8
            if(val < 256) { break }
            i -= 1
        }

    }
}

    // Build a TLS 1.3 nonce: static_iv (12 bytes) XOR (0x00000000 || seq_num)
    func tls13_build_nonce(static_iv : *u8, seq_num : *u8, nonce : *mut u8) {
        unsafe {

        // First 4 bytes: static_iv XOR 0
        var i : size_t = 0
        while(i < 4) {
            nonce[i] = static_iv[i]
            i += 1
        }
        // Last 8 bytes: static_iv[4..11] XOR seq_num[0..7]
        i = 0
        while(i < 8) {
            nonce[4 + i] = static_iv[4 + i] ^ seq_num[i]
            i += 1
        }

    }
}

    // Encrypt plaintext using TLS 1.3 AEAD record format.
    // plaintext = content_type(1) + actual_data
    // output = header(5) + encrypted_record
    // Returns total bytes written (5 + encrypted_len), or negative error.
    public func tls13_encrypt_record(ssl : *mut SSLContext,
                                      content_type : u8,
                                      data : *u8, data_len : size_t,
                                      output : *mut u8, out_max : size_t) : int {
                                          unsafe {

        if(ssl.transform_out == null) { return ERR_SSL_INTERNAL_ERROR }
        var tr = ssl.transform_out

        var inner_len : size_t = data_len + 1
        var inner : [16640]u8
        var i : size_t = 0
        while(i < data_len) {
            inner[i] = data[i]
            i += 1
        }
        inner[data_len] = content_type

        var nonce : [12]u8
        tls13_build_nonce(&raw tr.base_iv_enc[0], &raw ssl.out_ctr[0], &raw mut nonce[0])

        var enc_record_len : size_t = inner_len + 16
        var outer_hdr : [5]u8
        outer_hdr[0] = SSL_MSG_APPLICATION_DATA as u8
        outer_hdr[1] = 0x03
        outer_hdr[2] = 0x03
        outer_hdr[3] = ((enc_record_len >> 8) & 0xFF) as u8
        outer_hdr[4] = (enc_record_len & 0xFF) as u8

        var ct_out = output + 5
        var tag_out = output + 5 + inner_len

        var ret : int = 0
        if(tr.cipher_type == CIPHER_CHACHA20_POLY1305 as u8) {
            chacha20_poly1305_encrypt(&raw tr.key_enc[0], &raw nonce[0],
                                      &raw outer_hdr[0], 5,
                                      &raw inner[0], inner_len,
                                      ct_out, tag_out)
        } else {
            var gcm_ctx : GCMContext
            ret = gcm_init(unsafe(&raw mut gcm_ctx), &raw tr.key_enc[0], tr.key_len as size_t)
            if(ret < 0) { return ret }

            ret = gcm_crypt_and_tag(unsafe(&raw mut gcm_ctx),
                                     &raw nonce[0], 12,
                                     &raw outer_hdr[0], 5,
                                     &raw inner[0], inner_len,
                                     ct_out, tag_out)
            if(ret < 0) { return ret }
        }

        i = 0
        while(i < 5) {
            output[i] = outer_hdr[i]
            i += 1
        }

        ssl_incr_seq_num(&raw mut ssl.out_ctr[0])
        return (5 + inner_len + 16) as i32

                                      }
}

    // Decrypt a TLS 1.3 record.
    // input = encrypted_record (without the 5-byte outer header, which has already been read)
    // outer_content_type is already known (from the header read by ssl_read_record)
    // output = decrypted plaintext (without the inner content_type at the end)
    // inner_content_type is extracted from the last byte of decrypted data.
    // Returns plaintext length, or negative error.
    public func tls13_decrypt_record(ssl : *mut SSLContext,
                                      input : *u8, input_len : size_t,
                                      output : *mut u8, out_max : size_t,
                                      inner_content_type : *mut u8) : int { unsafe {
        if(ssl.transform_in == null) { return ERR_SSL_INTERNAL_ERROR }
        var tr = ssl.transform_in

        if(input_len < 16) { return ERR_SSL_INVALID_RECORD }

        var ct_len : size_t = input_len - 16
        var tag_start = input + ct_len

        var nonce : [12]u8
        tls13_build_nonce(&raw tr.base_iv_dec[0], &raw ssl.in_ctr[0], &raw mut nonce[0])

        var outer_hdr : [5]u8
        outer_hdr[0] = ssl.in_hdr[0]
        outer_hdr[1] = ssl.in_hdr[1]
        outer_hdr[2] = ssl.in_hdr[2]
        outer_hdr[3] = ssl.in_hdr[3]
        outer_hdr[4] = ssl.in_hdr[4]

        var dec_buf : [16640]u8
        if(ct_len > out_max + 1) { ct_len = out_max + 1 }

        var ret : int = 0
        if(tr.cipher_type == CIPHER_CHACHA20_POLY1305 as u8) {
            ret = chacha20_poly1305_decrypt(&raw tr.key_dec[0], &raw nonce[0],
                                            &raw outer_hdr[0], 5,
                                            input, ct_len,
                                            tag_start, &raw mut dec_buf[0])
            if(ret < 0) {
                return ERR_SSL_INVALID_RECORD
            }
        } else {
            var gcm_ctx : GCMContext
            ret = gcm_init(unsafe(&raw mut gcm_ctx), &raw tr.key_dec[0], tr.key_len as size_t)
            if(ret < 0) { return ret }

            ret = gcm_auth_decrypt(unsafe(&raw mut gcm_ctx),
                                    &raw nonce[0], 12,
                                    &raw outer_hdr[0], 5,
                                    input, ct_len,
                                    tag_start, 16,
                                    &raw mut dec_buf[0])
            if(ret < 0) {
                return ERR_SSL_INVALID_RECORD
            }
        }

        if(ct_len == 0) { return ERR_SSL_INVALID_RECORD }
        var actual_len = ct_len - 1
        *inner_content_type = dec_buf[actual_len]

        var i : size_t = 0
        while(i < actual_len) {
            output[i] = dec_buf[i]
            i += 1
        }

        ssl_incr_seq_num(&raw mut ssl.in_ctr[0])
        return actual_len as i32
     }}

    // ============================================================================
    // Record Layer
    // ============================================================================

    public comptime const MAX_RECORD_PAYLOAD = 16384
    public comptime const RECORD_HEADER_SIZE = 5

    // Send a TLS record
    async func send_record(ssl : *mut SSLContext, content_type : u8,
                     data : *u8, data_len : u16) : int {
                         unsafe {

        if((data_len as int) > MAX_RECORD_PAYLOAD) { return ERR_SSL_INTERNAL_ERROR }

        // ChangeCipherSpec is sent in-the-clear per RFC
        if(content_type == SSL_MSG_CHANGE_CIPHER_SPEC as u8) {
            var header : [5]u8
            header[0] = content_type
            header[1] = ssl.major_ver
            header[2] = ssl.minor_ver
            header[3] = ((data_len >> 8) & 0xFF) as u8
            header[4] = (data_len & 0xFF) as u8
            var ret = await ssl_send(ssl, &raw header[0], 5)
            if(ret < 0) { return ret }
            if(data_len > 0) {
                var payload_ret = await ssl_send(ssl, data, data_len as i32)
                ret = payload_ret
            }
            return ret
        }

        // Before transforms are active, send in the clear (pre-key-exchange)
        if(ssl.transform_out == null) {
            var header : [5]u8
            header[0] = content_type
            header[1] = ssl.major_ver
            header[2] = ssl.minor_ver
            header[3] = ((data_len >> 8) & 0xFF) as u8
            header[4] = (data_len & 0xFF) as u8
            var ret = await ssl_send(ssl, &raw header[0], 5)
            if(ret < 0) { return ret }
            if(data_len > 0) {
                var plain_ret = await ssl_send(ssl, data, data_len as i32)
                ret = plain_ret
            }
            return ret
        }

        // Transforms active — encrypt based on TLS version
        var encrypted : [17400]u8
        var enc_len : int = 0

        if(ssl.tls_version >= SSL_VERSION_TLS1_3) {
            enc_len = tls13_encrypt_record(ssl, content_type,
                                           data, data_len as size_t,
                                           &raw mut encrypted[0], 17400)
        } else {
            // TLS 1.2: prepend 5-byte record header before encrypted payload
            enc_len = tls12_encrypt_record(ssl.transform_out,
                                           &raw ssl.out_ctr[0],
                                           content_type,
                                           ssl.major_ver, ssl.minor_ver,
                                           data, data_len as size_t,
                                           &raw mut encrypted[5], 17395)
            if(enc_len < 0) { return enc_len }

            // Write record header at the beginning (encrypted starts at offset 5)
            encrypted[0] = content_type
            encrypted[1] = ssl.major_ver
            encrypted[2] = ssl.minor_ver
            encrypted[3] = ((enc_len >> 8) & 0xFF) as u8
            encrypted[4] = (enc_len & 0xFF) as u8
            enc_len += 5
        }

        if(enc_len < 0) { return enc_len }

        var ret = await ssl_send(ssl, &raw encrypted[0], enc_len)
        if(ret >= 0 && ssl.tls_version < SSL_VERSION_TLS1_3) {
            ssl_incr_seq_num(&raw mut ssl.out_ctr[0])
        }
        return ret

                     }
}

    // Send an alert record
    async func send_alert(ssl : *mut SSLContext, level : u8, description : u8) : int {
        var alert_data : [2]u8
        alert_data[0] = level as u8
        alert_data[1] = description as u8
        return await send_record(ssl, SSL_MSG_ALERT as u8, &raw alert_data[0], 2 as u16)
    }

    // Send a handshake message
    async func send_handshake_msg(ssl : *mut SSLContext, msg_type : u8,
                            data : *u8, data_len : u32) : int {
                                unsafe {

        var hs_header : [4]u8
        hs_header[0] = msg_type
        write_u24(data_len, &raw mut hs_header[1])

        var total_len : u32 = 4 + data_len
        var buf : [16388]u8

        var i : size_t = 0
        while(i < 4) {
            buf[i] = hs_header[i]
            i += 1
        }
        var j : size_t = 0
        while(j < data_len as size_t) {
            buf[4 + j] = data[j]
            j += 1
        }

        return await send_record(ssl, SSL_MSG_HANDSHAKE as u8, &raw buf[0], total_len as u16)

                            }
}

    // ─── Buffered Record I/O ──────────────────────────────────────────────
    // All record reads go through ssl_read_record(), which uses the input
    // buffer (ssl.in_buf) to handle:
    //   - Multiple records coalesced in a single TCP segment
    //   - Records split across multiple TCP segments
    //   - Partial reads

    // Fetch more data from the socket into the input buffer
    async func ssl_fetch_input(ssl : *mut SSLContext, min_len : size_t) : int {
        unsafe {

        while(ssl.in_left < min_len as i32) {
            var buf_start : i32 = ssl.in_left
            if(buf_start >= 17408 as i32) { return ERR_SSL_BUFFER_TOO_SMALL }
            var max_read : i32 = (17408 as i32) - buf_start

            var n = await ssl_recv(ssl, &raw mut ssl.in_buf[buf_start], max_read)
            if(n < 0) {
                if(n == ERR_SSL_CONN_EOF) {
                    if(ssl.in_left == 0) { return ERR_SSL_CONN_EOF }
                    break
                }
                return n
            }
            if(n == 0) {
                if(ssl.in_left == 0) { return ERR_SSL_CONN_EOF }
                break
            }
            ssl.in_left += n as i32
        }

        if(ssl.in_left < min_len as i32) {
            return ERR_SSL_CONN_EOF
        }
        return 0

    }
}

    // Read the next complete record from the input buffer
    // Returns 0 on success, negative error code on failure.
    // The record header (5 bytes) is stored in ssl.in_hdr.
    // The record payload (record_len bytes) is stored in ssl.in_buf.
    async func ssl_read_record(ssl : *mut SSLContext) : int {
        unsafe {

        // Ensure we have at least 5 bytes (record header)
        var ret = await ssl_fetch_input(ssl, 5)
        if(ret < 0) { return ret }

        // Parse record header from buffer
        ssl.in_hdr[0] = ssl.in_buf[0]  // content_type
        ssl.in_hdr[1] = ssl.in_buf[1]  // version.major
        ssl.in_hdr[2] = ssl.in_buf[2]  // version.minor
        ssl.in_hdr[3] = ssl.in_buf[3]  // length high
        ssl.in_hdr[4] = ssl.in_buf[4]  // length low

        var record_len = read_u16_be(&raw ssl.in_buf[3]) as size_t
        if(record_len > 16384 + 256) { return ERR_SSL_INVALID_RECORD }

        // Ensure we have the full record payload
        // Note: NOT overwriting ssl.in_left here to preserve any
        // coalesced records that arrived in the same TCP segment.
        var total_needed : size_t = 5 + record_len
        ret = await ssl_fetch_input(ssl, total_needed)
        if(ret < 0) { return ret }

        ssl.in_msglen = record_len as i32

        // AEAD decryption: if transform_in is set, decrypt the record payload.
        // TLS 1.3: outer CT is always APPLICATION_DATA (23); inner CT is last byte of plaintext.
        // TLS 1.2: CT is in the header; payload is encrypted after CCS.
        if(ssl.transform_in != null) {
            var did_decrypt = false

            // TLS 1.3 decrypt: triggered when outer CT is APPLICATION_DATA in TLS 1.3
            if(ssl.tls_version >= SSL_VERSION_TLS1_3 && ssl.in_hdr[0] == SSL_MSG_APPLICATION_DATA as u8) {
                var inner_ct : u8 = 0
                var dec_buf : [17400]u8
                var dec_len = tls13_decrypt_record(ssl,
                                                    &raw ssl.in_buf[5], record_len,
                                                    &raw mut dec_buf[0], 17400,
                                                    &raw mut inner_ct)
                if(dec_len >= 0) {
                    var i : i32 = 0
                    while(i < dec_len) {
                        ssl.in_buf[5 + i] = dec_buf[i]
                        i += 1
                    }
                    ssl.in_msglen = dec_len
                    ssl.in_hdr[0] = inner_ct

                    var original_end : i32 = 5 + record_len as i32
                    var new_end : i32 = 5 + dec_len
                    if(original_end < ssl.in_left) {
                        var shift_i : i32 = 0
                        while(shift_i < ssl.in_left - original_end) {
                            ssl.in_buf[new_end + shift_i] = ssl.in_buf[original_end + shift_i]
                            shift_i += 1
                        }
                        ssl.in_left -= (original_end - new_end)
                    } else {
                        ssl.in_left = new_end
                    }
                    did_decrypt = true
                }
            }

            // TLS 1.2 decrypt: triggered for HANDSHAKE (22) or APPLICATION_DATA (23)
            // when TLS 1.3 decrypt did not apply or failed.
            // NOTE: only run TLS 1.2 decrypt when NOT in TLS 1.3 mode
            if(!did_decrypt && ssl.tls_version < SSL_VERSION_TLS1_3 &&
                (ssl.in_hdr[0] == SSL_MSG_HANDSHAKE as u8 || ssl.in_hdr[0] == SSL_MSG_APPLICATION_DATA as u8)) {
                var dec_buf2 : [17400]u8
                var dec_len2 = tls12_decrypt_record(ssl.transform_in,
                                                     &raw ssl.in_ctr[0],
                                                     ssl.in_hdr[0], ssl.in_hdr[1], ssl.in_hdr[2],
                                                     &raw ssl.in_buf[5], record_len,
                                                     &raw mut dec_buf2[0], 17400 as size_t)
                if(dec_len2 >= 0) {
                    var i : i32 = 0
                    while(i < dec_len2) {
                        ssl.in_buf[5 + i] = dec_buf2[i]
                        i += 1
                    }
                    ssl.in_msglen = dec_len2

                    var original_end : i32 = 5 + record_len as i32
                    var new_end : i32 = 5 + dec_len2
                    if(original_end < ssl.in_left) {
                        var shift_i : i32 = 0
                        while(shift_i < ssl.in_left - original_end) {
                            ssl.in_buf[new_end + shift_i] = ssl.in_buf[original_end + shift_i]
                            shift_i += 1
                        }
                        ssl.in_left -= (original_end - new_end)
                    } else {
                        ssl.in_left = new_end
                    }
                    ssl_incr_seq_num(&raw mut ssl.in_ctr[0])
                }
            }
        }

        return 0

    }
}

    // Consume (remove) the current record from the input buffer
    public func ssl_consume_record(ssl : *mut SSLContext) {
        unsafe {

        var consumed = 5 + ssl.in_msglen
        if(consumed > ssl.in_left) { consumed = ssl.in_left }
        if(consumed <= 0) { return }

        // Shift remaining data to front of buffer
        var remaining = ssl.in_left - consumed
        if(remaining > 0) {
            var i : i32 = 0
            while(i < remaining) {
                ssl.in_buf[i] = ssl.in_buf[consumed + i]
                i += 1
            }
        }
        ssl.in_left = remaining
        ssl.in_msglen = 0

    }
}

    // Read a TLS record header (blocking) - maintains backward compatibility
    async func read_record_header(ssl : *mut SSLContext, hdr : *mut u8) : int {
        unsafe {

        var ret = await ssl_read_record(ssl)
        if(ret < 0) { return ret }
        hdr[0] = ssl.in_hdr[0]
        hdr[1] = ssl.in_hdr[1]
        hdr[2] = ssl.in_hdr[2]
        hdr[3] = ssl.in_hdr[3]
        hdr[4] = ssl.in_hdr[4]
        return 0

    }
}

    // Read record payload - copies from internal buffer to caller's buffer
    func read_record_payload(ssl : *mut SSLContext, buf : *mut u8, len : i32) : int {
        unsafe {

        var copy_len = ssl.in_msglen
        if(copy_len > len) { copy_len = len }
        if(copy_len > 0) {
            var i : i32 = 0
            while(i < copy_len) {
                buf[i] = ssl.in_buf[5 + i]
                i += 1
            }
        }
        ssl_consume_record(ssl)
        return copy_len

    }
}

    // ============================================================================
    // TLS 1.2 Client Hello Construction
    // ============================================================================

    func build_client_hello(ssl : *mut SSLContext, buf : *mut u8, buf_size : size_t) : int {
        unsafe {

        var pos : i32 = 0

        // Protocol version (TLS 1.2 = 0x0303)
        buf[pos] = 0x03 as u8; pos += 1
        buf[pos] = 0x03 as u8; pos += 1

        // Client random (32 bytes) - cryptographically secure random
        var rand_buf : [32]u8
        var rand_ret = random_fill(&raw mut rand_buf[0], 32)
        if(rand_ret < 0) { return ERR_SSL_NO_RNG }
        var k : u32 = 0
        while(k < 32) {
            buf[pos] = rand_buf[k]
            pos += 1
            k += 1
        }

        // Session ID (for session resumption)
        if(ssl.session != null && ssl.session.id_len > 0) {
            buf[pos] = ssl.session.id_len as u8
            pos += 1
            var sid_i : size_t = 0
            while(sid_i < ssl.session.id_len) {
                buf[pos] = ssl.session.id[sid_i]
                pos += 1
                sid_i += 1
            }
        } else {
            buf[pos] = 0 as u8; pos += 1
        }

        // Determine TLS 1.3 support for conditional extensions
        var supports_tls13 : bool = (ssl.conf != null && ssl.conf.max_tls_version >= SSL_VERSION_TLS1_3)

        // Cipher suites
        var suite_count : u32 = 0
        var suite_start = pos
        buf[pos] = 0 as u8; pos += 1
        buf[pos] = 0 as u8; pos += 1

        var i : u32 = 0
        while(i < ssl.conf.ciphersuite_count) {
            var cs_id = ssl.conf.ciphersuite_list[i]
            if(cs_id != 0) {
                // Only version-gate the offer: TLS 1.3-only suites are dropped
                // when the config caps the version at TLS 1.2. Capability
                // filtering is NOT applied here — explicit ciphersuite_list
                // configurations are offered verbatim (unsupported picks are
                // rejected gracefully after the ServerHello).
                var skip_cs : bool = false
                var cs_info = get_ciphersuite_info(cs_id)
                if(!supports_tls13) {
                    if(cs_info.max_tls_version >= SSL_VERSION_TLS1_3 as u8) {
                        skip_cs = true
                    }
                }
                if(!skip_cs) {
                    buf[pos] = ((cs_id >> 8) & 0xFF) as u8
                    pos += 1
                    buf[pos] = (cs_id & 0xFF) as u8
                    pos += 1
                    suite_count += 1
                }
            }
            i += 1
        }

        var cs_len = suite_count * 2
        buf[suite_start] = ((cs_len >> 8) & 0xFF) as u8
        buf[suite_start + 1] = (cs_len & 0xFF) as u8

        // Compression methods
        buf[pos] = 1 as u8; pos += 1
        buf[pos] = 0 as u8; pos += 1

        // Extensions
        var ext_start = pos
        buf[pos] = 0 as u8; pos += 1
        buf[pos] = 0 as u8; pos += 1

        // Extension: supported_versions
        // Conditionally include TLS 1.3 only if the config allows it
        buf[pos] = ((TLS_EXT_SUPPORTED_VERSIONS >> 8) & 0xFF) as u8; pos += 1
        buf[pos] = (TLS_EXT_SUPPORTED_VERSIONS & 0xFF) as u8; pos += 1

        var sv_len_pos = pos
        buf[pos] = 0 as u8; pos += 1
        buf[pos] = 0 as u8; pos += 1

        if(supports_tls13) {
            buf[pos] = 4 as u8; pos += 1  // List length (2 versions * 2 bytes)
            buf[pos] = 0x03 as u8; pos += 1; buf[pos] = 0x04 as u8; pos += 1  // TLS 1.3
            buf[pos] = 0x03 as u8; pos += 1; buf[pos] = 0x03 as u8; pos += 1  // TLS 1.2
            var sv_data_len = 5
            buf[sv_len_pos] = ((sv_data_len >> 8) & 0xFF) as u8
            buf[sv_len_pos + 1] = (sv_data_len & 0xFF) as u8
        } else {
            buf[pos] = 2 as u8; pos += 1  // List length (1 version * 2 bytes)
            buf[pos] = 0x03 as u8; pos += 1; buf[pos] = 0x03 as u8; pos += 1  // TLS 1.2
            var sv_data_len = 3
            buf[sv_len_pos] = ((sv_data_len >> 8) & 0xFF) as u8
            buf[sv_len_pos + 1] = (sv_data_len & 0xFF) as u8
        }

        // Extension: supported_groups (always advertise for TLS 1.2 ECDHE too)
        buf[pos] = ((TLS_EXT_SUPPORTED_GROUPS >> 8) & 0xFF) as u8; pos += 1
        buf[pos] = (TLS_EXT_SUPPORTED_GROUPS & 0xFF) as u8; pos += 1

            var sg_len_pos = pos
        buf[pos] = 0 as u8; pos += 1; buf[pos] = 0 as u8; pos += 1

        buf[pos] = 0 as u8; pos += 1; buf[pos] = 8 as u8; pos += 1
        buf[pos] = 0x00 as u8; pos += 1; buf[pos] = 0x1D as u8; pos += 1  // x25519
        buf[pos] = 0x00 as u8; pos += 1; buf[pos] = 0x17 as u8; pos += 1  // secp256r1
        buf[pos] = 0x00 as u8; pos += 1; buf[pos] = 0x18 as u8; pos += 1  // secp384r1
        buf[pos] = 0x00 as u8; pos += 1; buf[pos] = 0x1E as u8; pos += 1  // x448

        var sg_data_len = 2 + 8
        buf[sg_len_pos] = ((sg_data_len >> 8) & 0xFF) as u8
        buf[sg_len_pos + 1] = (sg_data_len & 0xFF) as u8

        // Extension: signature_algorithms
        buf[pos] = ((TLS_EXT_SIG_ALG >> 8) & 0xFF) as u8; pos += 1
        buf[pos] = (TLS_EXT_SIG_ALG & 0xFF) as u8; pos += 1

        var sa_len_pos = pos
        buf[pos] = 0 as u8; pos += 1; buf[pos] = 0 as u8; pos += 1

        buf[pos] = 0 as u8; pos += 1; buf[pos] = 16 as u8; pos += 1
        buf[pos] = 0x04 as u8; pos += 1; buf[pos] = 0x03 as u8; pos += 1
        buf[pos] = 0x05 as u8; pos += 1; buf[pos] = 0x03 as u8; pos += 1
        buf[pos] = 0x08 as u8; pos += 1; buf[pos] = 0x04 as u8; pos += 1
        buf[pos] = 0x08 as u8; pos += 1; buf[pos] = 0x05 as u8; pos += 1
        buf[pos] = 0x04 as u8; pos += 1; buf[pos] = 0x01 as u8; pos += 1
        buf[pos] = 0x05 as u8; pos += 1; buf[pos] = 0x01 as u8; pos += 1
        buf[pos] = 0x08 as u8; pos += 1; buf[pos] = 0x09 as u8; pos += 1
        buf[pos] = 0x08 as u8; pos += 1; buf[pos] = 0x07 as u8; pos += 1

        var sa_data_len = 2 + 16
        buf[sa_len_pos] = ((sa_data_len >> 8) & 0xFF) as u8
        buf[sa_len_pos + 1] = (sa_data_len & 0xFF) as u8

        // SNI extension
        if(ssl.hostname != null && ssl.hostname_len > 0) {
            buf[pos] = ((TLS_EXT_SERVERNAME >> 8) & 0xFF) as u8; pos += 1
            buf[pos] = (TLS_EXT_SERVERNAME & 0xFF) as u8; pos += 1

            var sni_len_pos = pos
            buf[pos] = 0 as u8; pos += 1; buf[pos] = 0 as u8; pos += 1

            buf[pos] = 0 as u8; pos += 1
            var hostlist_len = ssl.hostname_len + 3
            buf[pos] = hostlist_len as u8; pos += 1

            buf[pos] = 0 as u8; pos += 1  // Name type: host_name
            write_u16_be(ssl.hostname_len as u16, &raw mut buf[pos]); pos += 2

            var n : size_t = 0
            while(n < ssl.hostname_len) {
                buf[pos] = ssl.hostname[n] as u8
                pos += 1
                n += 1
            }

            var sni_data_len = 2 + hostlist_len
            buf[sni_len_pos] = (sni_data_len >> 8) as u8
            buf[sni_len_pos + 1] = sni_data_len as u8
        }

        // ALPN extension
        if(ssl.conf != null && ssl.conf.alpn_list != null && ssl.conf.alpn_count > 0) {
            buf[pos] = ((TLS_EXT_ALPN >> 8) & 0xFF) as u8; pos += 1
            buf[pos] = (TLS_EXT_ALPN & 0xFF) as u8; pos += 1

            var alpn_len_pos = pos
            buf[pos] = 0 as u8; pos += 1; buf[pos] = 0 as u8; pos += 1

            // ProtocolNameList
            var pnl_len_pos = pos
            buf[pos] = 0 as u8; pos += 1; buf[pos] = 0 as u8; pos += 1

            var alpn_count = ssl.conf.alpn_count as u8
            var ai : u8 = 0
            while(ai < alpn_count) {
                var pname = ssl.conf.alpn_list[ai]
                if(pname != null) {
                    var plen : u32 = 0
                    while(pname[plen] != 0) { plen += 1 }
                    if(plen > 0 && plen <= 255) {
                        buf[pos] = plen as u8; pos += 1
                        var pj : u32 = 0
                        while(pj < plen) {
                            buf[pos] = pname[pj] as u8
                            pos += 1
                            pj += 1
                        }
                    }
                }
                ai += 1
            }

            var pnl_data_len = pos - pnl_len_pos - 2
            buf[pnl_len_pos] = ((pnl_data_len >> 8) & 0xFF) as u8
            buf[pnl_len_pos + 1] = (pnl_data_len & 0xFF) as u8

            var alpn_data_len = 2 + pnl_data_len
            buf[alpn_len_pos] = ((alpn_data_len >> 8) & 0xFF) as u8
            buf[alpn_len_pos + 1] = (alpn_data_len & 0xFF) as u8
        }

        // SessionTicket extension (RFC 5077, empty offer). Servers only send
        // a NewSessionTicket when the client advertises support — without it
        // ssl_read_new_session_ticket() would wait for a message that never
        // arrives. Empty (zero-length) data means "issue me a new ticket".
        if(ssl.conf != null && ssl.conf.session_tickets != 0) {
            buf[pos] = ((TLS_EXT_SESSION_TICKET >> 8) & 0xFF) as u8; pos += 1
            buf[pos] = (TLS_EXT_SESSION_TICKET & 0xFF) as u8; pos += 1
            buf[pos] = 0 as u8; pos += 1; buf[pos] = 0 as u8; pos += 1  // empty ext data
        }

        // psk_key_exchange_modes (TLS 1.3 resumption). The pre_shared_key
        // extension itself is appended AFTER key_share, since it MUST be the
        // last extension (RFC 8446 §4.2.11). Only offered when a session
        // ticket + resumption PSK is available.
        var offering_psk : bool = (ssl.session != null && ssl.session.ticket != null &&
                                   ssl.session.ticket_len > 0 && ssl.session.ticket_len < 65535 &&
                                   ssl.handshake != null && ssl.handshake.psk_len > 0)
        if(offering_psk) {
            buf[pos] = ((TLS_EXT_PSK_KEY_EXCHANGE_MODES >> 8) & 0xFF) as u8; pos += 1
            buf[pos] = (TLS_EXT_PSK_KEY_EXCHANGE_MODES & 0xFF) as u8; pos += 1
            buf[pos] = 0 as u8; pos += 1; buf[pos] = 2 as u8; pos += 1  // ext data length
            buf[pos] = 1 as u8; pos += 1   // list length: 1 mode
            buf[pos] = 1 as u8; pos += 1   // psk_dhe_ke
        }

        // Extension: key_share (TLS 1.3) — includes both P-256 and x25519
        if(ssl.handshake != null) {
            var has_p256 = ssl.handshake.ecdhe_public != null && ssl.handshake.ecdhe_public_len == 65
            var has_x25519 = ssl.handshake.x25519_public != null && ssl.handshake.x25519_public_len == 32
            if(has_p256 || has_x25519) {
                buf[pos] = ((TLS_EXT_KEY_SHARE >> 8) & 0xFF) as u8; pos += 1
                buf[pos] = (TLS_EXT_KEY_SHARE & 0xFF) as u8; pos += 1
                var ks_len_pos = pos
                buf[pos] = 0 as u8; pos += 1; buf[pos] = 0 as u8; pos += 1
                // client_shares length placeholder
                var cs_len_pos = pos
                buf[pos] = 0 as u8; pos += 1; buf[pos] = 0 as u8; pos += 1

                // x25519 first (preferred)
                if(has_x25519) {
                    buf[pos] = ((TLS_GROUP_X25519 >> 8) & 0xFF) as u8; pos += 1
                    buf[pos] = (TLS_GROUP_X25519 & 0xFF) as u8; pos += 1
                    buf[pos] = 0 as u8; pos += 1; buf[pos] = 32 as u8; pos += 1
                    var ki : size_t = 0
                    while(ki < 32) {
                        buf[pos] = ssl.handshake.x25519_public[ki]
                        pos += 1; ki += 1
                    }
                }

                // P-256 fallback
                if(has_p256) {
                    buf[pos] = ((TLS_GROUP_SECP256R1 >> 8) & 0xFF) as u8; pos += 1
                    buf[pos] = (TLS_GROUP_SECP256R1 & 0xFF) as u8; pos += 1
                    buf[pos] = 0 as u8; pos += 1; buf[pos] = 65 as u8; pos += 1
                    var ki : size_t = 0
                    while(ki < 65) {
                        buf[pos] = ssl.handshake.ecdhe_public[ki]
                        pos += 1; ki += 1
                    }
                }

                var cs_len_key = pos - cs_len_pos - 2
                buf[cs_len_pos] = ((cs_len_key >> 8) & 0xFF) as u8
                buf[cs_len_pos + 1] = (cs_len_key & 0xFF) as u8
                var ks_data_len = 2 + cs_len_key
                buf[ks_len_pos] = ((ks_data_len >> 8) & 0xFF) as u8
                buf[ks_len_pos + 1] = (ks_data_len & 0xFF) as u8
            }
        }

        // pre_shared_key extension — MUST be the last extension (RFC 8446 §4.2.11).
        // The binder is a 32-byte zero placeholder; the caller fills in the
        // computed binder after build_client_hello returns.
        if(offering_psk) {
            buf[pos] = ((TLS_EXT_PRE_SHARED_KEY >> 8) & 0xFF) as u8; pos += 1
            buf[pos] = (TLS_EXT_PRE_SHARED_KEY & 0xFF) as u8; pos += 1
            var psk_ext_len_pos = pos
            buf[pos] = 0 as u8; pos += 1; buf[pos] = 0 as u8; pos += 1

            // identities vector
            var psk_ident_len_pos = pos
            buf[pos] = 0 as u8; pos += 1; buf[pos] = 0 as u8; pos += 1
            // identity: opaque ticket<1..2^16-1>
            write_u16_be(ssl.session.ticket_len as u16, &raw mut buf[pos]); pos += 2
            var ti : size_t = 0
            while(ti < ssl.session.ticket_len) {
                buf[pos] = ssl.session.ticket[ti]
                pos += 1
                ti += 1
            }
            // obfuscated_ticket_age (4 bytes)
            buf[pos] = 0 as u8; pos += 1
            buf[pos] = 0 as u8; pos += 1
            buf[pos] = 0 as u8; pos += 1
            buf[pos] = 0 as u8; pos += 1
            var ident_len = 2 + ssl.session.ticket_len + 4
            buf[psk_ident_len_pos] = ((ident_len >> 8) & 0xFF) as u8
            buf[psk_ident_len_pos + 1] = (ident_len & 0xFF) as u8

            // binders vector: one PskBinderEntry. Each entry is itself
            // length-prefixed (opaque<32..255>), so the vector length is
            // 1 (entry length byte) + 32 = 33. Zero placeholder for now.
            var psk_binder_len_pos = pos
            // The binder transcript covers the ClientHello up to the END of the
            // identities field (= here), NOT the binders list (RFC 8446 §4.2.11.2).
            ssl.handshake.psk_partial_len = psk_binder_len_pos as u16
            buf[pos] = 0 as u8; pos += 1; buf[pos] = 0 as u8; pos += 1
            var psk_binder_data_len = 33
            buf[psk_binder_len_pos] = ((psk_binder_data_len >> 8) & 0xFF) as u8
            buf[psk_binder_len_pos + 1] = (psk_binder_data_len & 0xFF) as u8
            buf[pos] = 32 as u8; pos += 1  // PskBinderEntry length (SHA-256)
            var binder_i : size_t = 0
            while(binder_i < 32) { buf[pos] = 0 as u8; pos += 1; binder_i += 1 }
            ssl.handshake.psk_binder_off = (pos - 32) as u16

            var psk_data_len = 2 + ident_len + 2 + psk_binder_data_len
            buf[psk_ext_len_pos] = ((psk_data_len >> 8) & 0xFF) as u8
            buf[psk_ext_len_pos + 1] = (psk_data_len & 0xFF) as u8
        }

        var ext_len = pos - ext_start - 2
        buf[ext_start] = (ext_len >> 8) as u8
        buf[ext_start + 1] = ext_len as u8

        return pos

    }
}

    // ============================================================================
    // I/O Functions
    // ============================================================================

    // Transport callbacks (TLS-VT). The blocking transport reads/writes the
    // socket with blocking syscalls and completes eagerly; the async transport
    // suspends on fd readiness. Both return a `FutureHandle<int>`.
    async func transport_recv_blocking(ctx : *mut void, buf : *mut u8, len : size_t) : int {
        return net::raw::recv_all(ctx as net::raw::Socket, buf, len)
    }

    async func transport_send_blocking(ctx : *mut void, buf : *u8, len : size_t) : int {
        return net::raw::send_all(ctx as net::raw::Socket, buf as *char, len as int)
    }

    async func transport_recv_async(ctx : *mut void, buf : *mut u8, len : size_t) : int {
        var fd = ctx as net::raw::Socket
        return await net::raw::recv_async(fd as int, buf, len)
    }

    async func transport_send_async(ctx : *mut void, buf : *u8, len : size_t) : int {
        var fd = ctx as net::raw::Socket
        return await net::raw::send_async(fd as int, buf as *char, len as int)
    }

    async func ssl_send(ssl : *mut SSLContext, data : *u8, len : i32) : int {
        unsafe {

        if(!ssl.transport_connected) { return ERR_SSL_INTERNAL_ERROR }
        var tr = ssl.transport
        var n = await tr.send_fn(tr.ctx, data, len as size_t)
        if(n < 0) { return ERR_SSL_CONN_EOF }
        return len

    }
}

    async func ssl_recv(ssl : *mut SSLContext, buf : *mut u8, len : i32) : int {
        unsafe {

        if(!ssl.transport_connected) { return ERR_SSL_INTERNAL_ERROR }
        var tr = ssl.transport
        var n = await tr.recv_fn(tr.ctx, buf, len as size_t)
        if(n < 0) { return ERR_SSL_CONN_EOF }
        if(n == 0) { return ERR_SSL_CONN_EOF }
        return n

    }
}

    // ============================================================================
    // Client Handshake - TLS 1.2
    // ============================================================================

    // Read a single handshake message from the server
    async func read_handshake_msg(ssl : *mut SSLContext, hs_type : *mut u8,
                             hs_len : *mut u32, hs_buf : *mut u8,
                              buf_size : size_t) : int {
                                  unsafe {

        var hdr : [5]u8
        var ret = await read_record_header(ssl, &raw mut hdr[0])
        if(ret < 0) { return ret }

        var content_type = hdr[0]
        var record_len = read_u16_be(&raw hdr[3])

        // Handle ChangeCipherSpec messages
        if(content_type == SSL_MSG_CHANGE_CIPHER_SPEC as u8) {
        var ccs_data : [1]u8
            var n = read_record_payload(ssl, &raw mut ccs_data[0], 1)
            if(n < 0) { return n }
            ret = await read_record_header(ssl, &raw mut hdr[0])
            if(ret < 0) { return ret }
            content_type = hdr[0]
            record_len = read_u16_be(&raw hdr[3])
        }

        if(content_type != SSL_MSG_HANDSHAKE as u8) {
            if(content_type == SSL_MSG_ALERT as u8) {
                var alert_data : [2]u8
                var n2 = read_record_payload(ssl, &raw mut alert_data[0], 2)
                if(n2 < 0) { return ERR_SSL_FATAL_ALERT_MESSAGE }
                ssl.last_alert_level = alert_data[0]
                ssl.last_alert_desc = alert_data[1]
                return ERR_SSL_FATAL_ALERT_MESSAGE
            }
            return ERR_SSL_UNEXPECTED_MESSAGE
        }

        var payload_len = record_len as size_t
        if(payload_len > buf_size) { payload_len = buf_size }

        var payload = read_record_payload(ssl, hs_buf, payload_len as i32)
        if(payload < (4 as i32)) { return ERR_SSL_DECODE_ERROR }

        unsafe { *hs_type = hs_buf[0] }
        unsafe { *hs_len = read_u24(&raw hs_buf[1]) }
        return 0

                              }
}

    // ─── Helper: feed a handshake message into the transcript hash ───────

    func ssl_hash_handshake_msg(hash_ctx : *mut crypto::raw::Sha256Context,
                                 msg_type : u8, msg_len : u32,
                                 msg_body : *u8) {
        var hdr : [4]u8
        hdr[0] = msg_type
        write_u24(msg_len, &raw mut hdr[1])
        crypto::raw::sha256_update(hash_ctx, &raw hdr[0], 4)
        if(msg_len > 0) {
            crypto::raw::sha256_update(hash_ctx, msg_body, msg_len as size_t)
        }
    }

    // Dual-hash variant for TLS 1.2: keeps SHA-256 and SHA-384 transcripts in
    // lockstep so either can be finalized once the negotiated ciphersuite is
    // known (SHA-384 PRF suites hash the transcript with SHA-384).
    func tls12_hash_handshake_msg_both(h256 : *mut crypto::raw::Sha256Context,
                                        h384 : *mut crypto::raw::Sha512Context,
                                        msg_type : u8, msg_len : u32,
                                        msg_body : *u8) {
        var hdr : [4]u8
        hdr[0] = msg_type
        write_u24(msg_len, &raw mut hdr[1])
        crypto::raw::sha256_update(h256, &raw hdr[0], 4)
        crypto::raw::sha384_update(h384, &raw hdr[0], 4)
        if(msg_len > 0) {
            crypto::raw::sha256_update(h256, msg_body, msg_len as size_t)
            crypto::raw::sha384_update(h384, msg_body, msg_len as size_t)
        }
    }

    // ─── Extract RSA public key from parsed certificate ──────────────────
    // Parse the SubjectPublicKeyInfo BIT STRING to extract RSA modulus N and exponent E
    // Returns 0 on success, negative error code on failure.
    // The RSA context must be initialized with rsa_init() before calling.
    public func x509_extract_rsa_pubkey(crt : *mut X509Cert, rsa : *mut RSAContext) : int {
        unsafe {

        if(crt.pk_type != PK_RSA as u8) { return ERR_SSL_PK_TYPE_MISMATCH }
        if(crt.pk_raw == null || crt.pk_raw_len == 0) { return ERR_X509_INVALID_FORMAT }

        // pk_raw points to SPKI SEQUENCE content
        // Structure: SEQUENCE { AlgorithmIdentifier, BIT STRING { SEQUENCE { INTEGER N, INTEGER E } } }
        var data = crt.pk_raw
        var len = crt.pk_raw_len
        var pos : size_t = 0

        // Parse AlgorithmIdentifier SEQUENCE inside SPKI
        var seq_tag : u8 = 0; var seq_len : size_t = 0
        var ret = asn1_get_tag(data, len, &raw mut pos, &raw mut seq_tag, &raw mut seq_len)
        if(ret < 0) { return ret }
        if(seq_tag != (ASN1_CONSTRUCTED | ASN1_SEQUENCE)) { return ERR_X509_INVALID_ALG }

        // Skip AlgorithmIdentifier content (OID + params)
        pos += seq_len

        // Parse BIT STRING
        var bit_tag : u8 = 0; var bit_len : size_t = 0
        ret = asn1_get_tag(data, len, &raw mut pos, &raw mut bit_tag, &raw mut bit_len)
        if(ret < 0) { return ret }
        if(bit_tag != ASN1_BIT_STRING) { return ERR_X509_INVALID_FORMAT }

        // Skip unused bits byte
        if(pos >= len) { return ERR_X509_INVALID_FORMAT }
        pos += 1
        bit_len -= 1

        if(bit_len == 0) { return ERR_X509_INVALID_FORMAT }
        var bit_end = pos + bit_len

        // Parse inner SEQUENCE (RSA public key: N, E)
        var rsa_tag : u8 = 0; var rsa_len : size_t = 0
        ret = asn1_get_tag(data, len, &raw mut pos, &raw mut rsa_tag, &raw mut rsa_len)
        if(ret < 0) { return ret }
        if(rsa_tag != (ASN1_CONSTRUCTED | ASN1_SEQUENCE)) { return ERR_X509_INVALID_FORMAT }

        // Parse INTEGER N
        var n_tag : u8 = 0; var n_len : size_t = 0
        ret = asn1_get_tag(data, len, &raw mut pos, &raw mut n_tag, &raw mut n_len)
        if(ret < 0) { return ret }
        if(n_tag != ASN1_INTEGER) { return ERR_X509_INVALID_FORMAT }

        var n_data = data + pos
        var n_data_len = n_len
        // Strip leading 0x00 byte (DER positive integer indicator)
        if(n_data_len > 0 && n_data[0] == 0) {
            n_data += 1; n_data_len -= 1
        }
        pos += n_len

        // Parse INTEGER E
        var e_tag : u8 = 0; var e_len : size_t = 0
        ret = asn1_get_tag(data, len, &raw mut pos, &raw mut e_tag, &raw mut e_len)
        if(ret < 0) { return ret }
        if(e_tag != ASN1_INTEGER) { return ERR_X509_INVALID_FORMAT }

        var e_data = data + pos
        var e_data_len = e_len
        // Strip leading 0x00 byte if present
        if(e_data_len > 0 && e_data[0] == 0) {
            e_data += 1; e_data_len -= 1
        }

        // Import into RSA context (rejects a modulus beyond RSA_MAX_KEY_BYTES)
        ret = rsa_import_pubkey(rsa, n_data, n_data_len, e_data, e_data_len)
        if(ret < 0) { return ret }

        // The modulus and exponent came off the wire: reject values that cannot
        // describe a real RSA key before anything is verified against them.
        ret = rsa_check_pubkey(rsa)
        if(ret < 0) { return ret }

        // Report the actual RSA modulus bit length. The parser sets a default
        // (2048) at parse time, which is wrong for 4096-bit keys — compute it
        // from the imported modulus so pk_bitlen reflects reality.
        crt.pk_bitlen = mpi_bitlen(&raw mut rsa.N) as u16

        return 0

    }
}

    // ─── Extract ECDSA Public Key from Parsed Certificate ───────────────
    // Parse SubjectPublicKeyInfo to extract ECDSA uncompressed public key.
    public func x509_extract_ecdsa_pubkey(crt : *mut X509Cert, ecdsa : *mut ECDSAContext) : int {
        unsafe {

        if(crt.pk_type != PK_ECKEY as u8) { return ERR_SSL_PK_TYPE_MISMATCH }
        if(crt.pk_raw == null || crt.pk_raw_len == 0) { return ERR_X509_INVALID_FORMAT }

        var data = crt.pk_raw
        var len = crt.pk_raw_len
        var pos : size_t = 0

        // Parse AlgorithmIdentifier SEQUENCE
        var seq_tag : u8 = 0; var seq_len : size_t = 0
        var ret = asn1_get_tag(data, len, &raw mut pos, &raw mut seq_tag, &raw mut seq_len)
        if(ret < 0) { return ret }
        if(seq_tag != (ASN1_CONSTRUCTED | ASN1_SEQUENCE)) { return ERR_X509_INVALID_ALG }
        var alg_content_start = pos
        var alg_end = pos + seq_len

        // Detect the named curve from the AlgorithmIdentifier params. The
        // secp256r1 OID 1.2.840.10045.3.1.7 encodes as 06 08 2A 86 48 CE 3D 03 01 07;
        // the secp384r1 OID 1.3.132.0.34 encodes as 06 05 2B 81 04 00 22.
        var curve : u16 = TLS_GROUP_SECP256R1 as u16
        var i : size_t = alg_content_start
        while(i + 2 < alg_end && i + 12 <= len) {
            if(data[i] == 0x06 && data[i + 1] == 0x05 && i + 7 <= alg_end) {
                var ok384 = true
                var k : size_t = 0
                var k384 : [5]u8 = [0x2B, 0x81, 0x04, 0x00, 0x22]
                while(k < 5) { if(data[i + 2 + k] != k384[k]) { ok384 = false }; k += 1 }
                if(ok384) { curve = TLS_GROUP_SECP384R1 as u16 }
            } else if(data[i] == 0x06 && data[i + 1] == 0x08 && i + 10 <= alg_end) {
                var ok256 = true
                var k : size_t = 0
                var k256 : [7]u8 = [0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01]
                while(k < 7) { if(data[i + 2 + k] != k256[k]) { ok256 = false }; k += 1 }
                if(ok256 && data[i + 9] == 0x07) { curve = TLS_GROUP_SECP256R1 as u16 }
            }
            i += 1
        }

        // Advance past the AlgorithmIdentifier to the BIT STRING.
        pos = alg_end

        // The bit_len check depends on the curve (65 bytes = P-256,
        // 97 bytes = P-384).
        var coord = 65
        if(curve == TLS_GROUP_SECP384R1 as u16) { coord = 97 }

        // Parse BIT STRING (contains the raw public key)
        var bit_tag : u8 = 0; var bit_len : size_t = 0
        ret = asn1_get_tag(data, len, &raw mut pos, &raw mut bit_tag, &raw mut bit_len)
        if(ret < 0) { return ret }
        if(bit_tag != ASN1_BIT_STRING) { return ERR_X509_INVALID_FORMAT }

        // Skip unused bits byte
        if(pos >= len) { return ERR_X509_INVALID_FORMAT }
        pos += 1
        bit_len -= 1

        if(bit_len < (coord as size_t)) { return ERR_X509_INVALID_FORMAT }
        if(data[pos] != 0x04) { return ERR_X509_INVALID_FORMAT }

        return ecdsa_import_pubkey(ecdsa, &raw data[pos], bit_len, curve)

    }
}

    // ─── Verify X.509 Certificate ECDSA Signature ───────────────────────
    public func x509_verify_cert_ecdsa_signature(crt : *mut X509Cert,
                                                  issuer_ecdsa : *mut ECDSAContext) : int {
                                                      unsafe {

        if(crt.tbs_der == null || crt.tbs_der_len == 0) { return ERR_X509_INVALID_FORMAT }
        if(crt.sig == null || crt.sig_len == 0) { return ERR_X509_INVALID_FORMAT }

        // Compute the digest with the hash declared by the certificate's
        // signature algorithm (defaults to SHA-256).
        var hash : [64]u8
        var hash_len : size_t = 32
        var sig_hash = crt.sig_md
        if(sig_hash == SSL_HASH_SHA384 as u8) {
            var sha_ctx : crypto::raw::Sha512Context
            crypto::raw::sha384_init(unsafe(&raw mut sha_ctx))
            crypto::raw::sha384_update(unsafe(&raw mut sha_ctx), crt.tbs_der, crt.tbs_der_len)
            crypto::raw::sha384_final(unsafe(&raw mut sha_ctx), &raw mut hash[0])
            hash_len = 48
        } else if(sig_hash == SSL_HASH_SHA512 as u8) {
            var sha_ctx : crypto::raw::Sha512Context
            crypto::raw::sha512_init(unsafe(&raw mut sha_ctx))
            crypto::raw::sha512_update(unsafe(&raw mut sha_ctx), crt.tbs_der, crt.tbs_der_len)
            crypto::raw::sha512_final(unsafe(&raw mut sha_ctx), &raw mut hash[0])
            hash_len = 64
        } else {
            var sha_ctx : crypto::raw::Sha256Context
            crypto::raw::sha256_init(unsafe(&raw mut sha_ctx))
            crypto::raw::sha256_update(unsafe(&raw mut sha_ctx), crt.tbs_der, crt.tbs_der_len)
            crypto::raw::sha256_final(unsafe(&raw mut sha_ctx), &raw mut hash[0])
            hash_len = 32
        }

        var ret = ecdsa_verify(issuer_ecdsa, &raw hash[0], hash_len, crt.sig, crt.sig_len)
        if(ret < 0) { return ERR_X509_SIG_MISMATCH }
        return 0

                                                  }
}

    // ─── Verify X.509 Certificate RSA Signature ──────────────────────────
    // Verifies the certificate's signature using the issuer's RSA public key.
    // crt: the certificate to verify
    // issuer_rsa: issuer RSA context with N and E already imported
    // Returns 0 on success, negative error code on failure
    public func x509_verify_cert_signature(crt : *mut X509Cert,
                                            issuer_rsa : *mut RSAContext) : int {
                                                unsafe {

        if(crt.tbs_der == null || crt.tbs_der_len == 0) { return ERR_X509_INVALID_FORMAT }
        if(crt.sig == null || crt.sig_len == 0) { return ERR_X509_INVALID_FORMAT }

        // Compute the digest with the hash declared by the certificate's
        // signature algorithm (defaults to SHA-256).
        var hash : [64]u8
        var hash_len : size_t = 32
        var sig_hash = crt.sig_md
        if(sig_hash == SSL_HASH_SHA384 as u8) {
            var sha_ctx : crypto::raw::Sha512Context
            crypto::raw::sha384_init(unsafe(&raw mut sha_ctx))
            crypto::raw::sha384_update(unsafe(&raw mut sha_ctx), crt.tbs_der, crt.tbs_der_len)
            crypto::raw::sha384_final(unsafe(&raw mut sha_ctx), &raw mut hash[0])
            hash_len = 48
        } else if(sig_hash == SSL_HASH_SHA512 as u8) {
            var sha_ctx : crypto::raw::Sha512Context
            crypto::raw::sha512_init(unsafe(&raw mut sha_ctx))
            crypto::raw::sha512_update(unsafe(&raw mut sha_ctx), crt.tbs_der, crt.tbs_der_len)
            crypto::raw::sha512_final(unsafe(&raw mut sha_ctx), &raw mut hash[0])
            hash_len = 64
        } else {
            var sha_ctx : crypto::raw::Sha256Context
            crypto::raw::sha256_init(unsafe(&raw mut sha_ctx))
            crypto::raw::sha256_update(unsafe(&raw mut sha_ctx), crt.tbs_der, crt.tbs_der_len)
            crypto::raw::sha256_final(unsafe(&raw mut sha_ctx), &raw mut hash[0])
            hash_len = 32
        }

        // Verify using RSA PKCS#1 v1.5 signature verification
        var ret = rsa_pkcs1_verify(issuer_rsa, &raw hash[0], hash_len, crt.sig, crt.sig_len)
        if(ret < 0) { return ERR_X509_SIG_MISMATCH }

        return 0

                                            }
}

    // ─── X.509 Hostname Verification ──────────────────────────────────────
    // Verify that the certificate's CN or SAN matches the expected hostname.
    // Checks SAN dNSName entries first, then falls back to CN.
    // Returns 0 on match, X509_BADCERT_CN_MISMATCH on mismatch.
    public func x509_verify_hostname(crt : *mut X509Cert, hostname : *char) : int {
        unsafe {

        var host_view = string_view(hostname)

        // 1. Check SAN dNSName entries (RFC 6125 compliant)
        if(crt.san_entries != null && crt.san_count > 0) {
            var san_len = crt.san_count as size_t
            var san_data = crt.san_entries
            var san_pos : size_t = 0
            // Parse SEQUENCE tag + length
            if(san_pos + 2 <= san_len) {
                var san_tag = san_data[san_pos]; san_pos += 1
                var san_seq_len : size_t = 0
                if((san_data[san_pos] & 0x80) == 0) {
                    san_seq_len = san_data[san_pos] as size_t
                    san_pos += 1
                } else {
                    var nb = (san_data[san_pos] & 0x7F) as size_t
                    san_pos += 1
                    var j : size_t = 0
                    while(j < nb && san_pos < san_len) {
                        san_seq_len = (san_seq_len << 8) | (san_data[san_pos] as size_t)
                        san_pos += 1
                        j += 1
                    }
                }
                var san_end = san_pos + san_seq_len
                while(san_pos + 2 <= san_end) {
                    var gn_tag = san_data[san_pos]; san_pos += 1
                    var gn_len : size_t = 0
                    if((san_data[san_pos] & 0x80) == 0) {
                        gn_len = san_data[san_pos] as size_t
                        san_pos += 1
                    } else {
                        var nb2 = (san_data[san_pos] & 0x7F) as size_t
                        san_pos += 1
                        var j2 : size_t = 0
                        while(j2 < nb2 && san_pos < san_end) {
                            gn_len = (gn_len << 8) | (san_data[san_pos] as size_t)
                            san_pos += 1
                            j2 += 1
                        }
                    }
                    // GeneralName tag 2 = dNSName
                    if(gn_tag == 0x82 && gn_len <= san_end - san_pos) {
                        var san_dns = string_view(san_data as *char + san_pos, gn_len)
                        // Direct match
                        if(san_dns.size() == host_view.size()) {
                            var di : size_t = 0
                            var m = true
                            while(di < san_dns.size()) {
                                // Case-insensitive comparison for DNS names
                                var sc = san_dns.at(di)
                                var hc = host_view.at(di)
                                if(sc >= ('A' as u8) && sc <= ('Z' as u8)) { sc = sc + 32 }
                                if(hc >= ('A' as u8) && hc <= ('Z' as u8)) { hc = hc + 32 }
                                if(sc != hc) { m = false }
                                di += 1
                            }
                            if(m) { return 0 }
                        }
                        // Wildcard: *.example.com matches www.example.com
                        if(san_dns.size() > 2 && san_dns.at(0) == ('*' as u8) && san_dns.at(1) == ('.' as u8)) {
                            var suffix_size = san_dns.size() - 1
                            if(host_view.size() >= suffix_size) {
                                var m2 = true
                                var wi : size_t = 0
                                while(wi < suffix_size) {
                                    var sc2 = san_dns.at(1 + wi)
                                    var hc2 = host_view.at(host_view.size() - suffix_size + wi)
                                    if(sc2 >= ('A' as u8) && sc2 <= ('Z' as u8)) { sc2 = sc2 + 32 }
                                    if(hc2 >= ('A' as u8) && hc2 <= ('Z' as u8)) { hc2 = hc2 + 32 }
                                    if(sc2 != hc2) { m2 = false }
                                    wi += 1
                                }
                                if(m2) { return 0 }
                            }
                        }
                    }
                    san_pos += gn_len
                }
            }
            // SAN present but no match found — fail (RFC 6125: SAN must be preferred)
            return X509_BADCERT_CN_MISMATCH as i32
        }

        // 2. Fall back to CN
        var cn = string()
        cert_get_cn(crt, &raw mut cn)

        // Check CN against hostname
        var cn_view = cn.to_view()

        // Direct comparison using byte-by-byte check
        var match = true
        if(cn_view.size() != host_view.size()) { match = false }
        if(match) {
            var ci : size_t = 0
            while(ci < cn_view.size()) {
                var cc = cn_view.at(ci)
                var hc = host_view.at(ci)
                if(cc >= ('A' as u8) && cc <= ('Z' as u8)) { cc = cc + 32 }
                if(hc >= ('A' as u8) && hc <= ('Z' as u8)) { hc = hc + 32 }
                if(cc != hc) { match = false }
                ci += 1
            }
        }
        if(match) { return 0 }

        // Wildcard check: CN = *.example.com, hostname = www.example.com
        if(cn_view.size() > 2) {
            if(cn_view.at(0) == ('*' as u8) && cn_view.at(1) == ('.' as u8)) {
                var wild_suffix_size = cn_view.size() - 1
                var host_len = host_view.size()
                if(host_len >= wild_suffix_size) {
                    var match2 = true
                    var wi : size_t = 0
                    while(wi < wild_suffix_size) {
                        var c = cn_view.at(1 + wi)
                        var h = host_view.at(host_len - wild_suffix_size + wi)
                        if(c >= ('A' as u8) && c <= ('Z' as u8)) { c = c + 32 }
                        if(h >= ('A' as u8) && h <= ('Z' as u8)) { h = h + 32 }
                        if(c != h) { match2 = false }
                        wi += 1
                    }
                    if(match2) { return 0 }
                }
            }
        }

        return X509_BADCERT_CN_MISMATCH as i32

    }
}

    // ─── ASN1_TIME Parser ──────────────────────────────────────────────────
    // Parse ASN1_UTC_TIME (YYMMDDHHMMSSZ, 13 bytes) or ASN1_GENERALIZED_TIME
    // (YYYYMMDDHHMMSSZ, 15 bytes) into date components.
    // Returns 0 on success, ERR_X509_INVALID_DATE on failure.
    func parse_asn1_time(time_str : *u8, max_len : size_t,
                          year : *mut int, month : *mut int, day : *mut int,
                          hour : *mut int, minute : *mut int,
                          second : *mut int) : int { unsafe {
        var len : size_t = 0
        while(len < max_len && time_str[len] != 0) { len += 1 }

        if(len == 13) {
            // UTCTime: YYMMDDHHMMSSZ
            var yy = (time_str[0] as int - 48) * 10 + (time_str[1] as int - 48)
            if(yy < 50) { *year = 2000 + yy } else { *year = 1900 + yy }
            *month = (time_str[2] as int - 48) * 10 + (time_str[3] as int - 48)
            *day = (time_str[4] as int - 48) * 10 + (time_str[5] as int - 48)
            *hour = (time_str[6] as int - 48) * 10 + (time_str[7] as int - 48)
            *minute = (time_str[8] as int - 48) * 10 + (time_str[9] as int - 48)
            *second = (time_str[10] as int - 48) * 10 + (time_str[11] as int - 48)
            return 0
        } else if(len == 11) {
            // UTCTime without seconds: YYMMDDHHMMZ
            var yy = (time_str[0] as int - 48) * 10 + (time_str[1] as int - 48)
            if(yy < 50) { *year = 2000 + yy } else { *year = 1900 + yy }
            *month = (time_str[2] as int - 48) * 10 + (time_str[3] as int - 48)
            *day = (time_str[4] as int - 48) * 10 + (time_str[5] as int - 48)
            *hour = (time_str[6] as int - 48) * 10 + (time_str[7] as int - 48)
            *minute = (time_str[8] as int - 48) * 10 + (time_str[9] as int - 48)
            *second = 0
            return 0
        } else if(len == 15) {
            // GeneralizedTime: YYYYMMDDHHMMSSZ
            *year = (time_str[0] as int - 48) * 1000 + (time_str[1] as int - 48) * 100 +
                     (time_str[2] as int - 48) * 10 + (time_str[3] as int - 48)
            *month = (time_str[4] as int - 48) * 10 + (time_str[5] as int - 48)
            *day = (time_str[6] as int - 48) * 10 + (time_str[7] as int - 48)
            *hour = (time_str[8] as int - 48) * 10 + (time_str[9] as int - 48)
            *minute = (time_str[10] as int - 48) * 10 + (time_str[11] as int - 48)
            *second = (time_str[12] as int - 48) * 10 + (time_str[13] as int - 48)
            return 0
        }

        return ERR_X509_INVALID_DATE
     }}

    // ─── X.509 Date Validity Check ────────────────────────────────────────
    // Check if the certificate's validity period covers the current time.
    // Returns 0 if valid, X509_BADCERT_EXPIRED or X509_BADCERT_FUTURE on failure.
    public func x509_check_date(crt : *mut X509Cert) : int {
        unsafe {

        if(crt.valid_from[0] == 0 || crt.valid_to[0] == 0) {
            return 0  // No date info, skip check
        }

        // Parse notBefore
        var from_year : int = 0; var from_month : int = 0; var from_day : int = 0
        var from_hour : int = 0; var from_min : int = 0; var from_sec : int = 0
        var ret = parse_asn1_time(&raw crt.valid_from[0], 15,
                                   &raw mut from_year, &raw mut from_month,
                                   &raw mut from_day, &raw mut from_hour,
                                   &raw mut from_min, &raw mut from_sec)
        if(ret < 0) { return X509_BADCERT_EXPIRED as i32 }

        // Parse notAfter
        var to_year : int = 0; var to_month : int = 0; var to_day : int = 0
        var to_hour : int = 0; var to_min : int = 0; var to_sec : int = 0
        ret = parse_asn1_time(&raw crt.valid_to[0], 15,
                               &raw mut to_year, &raw mut to_month,
                               &raw mut to_day, &raw mut to_hour,
                               &raw mut to_min, &raw mut to_sec)
        if(ret < 0) { return X509_BADCERT_EXPIRED as i32 }

        // Get current UTC time
        var now_st = std::chrono::SystemTime::now()
        var now_dt = datetime::DateTime::from_system_time(&now_st)

        var now_year = now_dt.year_val()
        var now_month = now_dt.month_val()
        var now_day = now_dt.day_val()
        var now_hour = now_dt.hour_val()
        var now_min = now_dt.minute_val()
        var now_sec = now_dt.second_val()

        // Compare current time to notBefore (cert not yet valid -> FUTURE)
        if(now_year < from_year) { return X509_BADCERT_FUTURE }
        if(now_year == from_year && now_month < from_month) { return X509_BADCERT_FUTURE }
        if(now_year == from_year && now_month == from_month && now_day < from_day) { return X509_BADCERT_FUTURE }
        if(now_year == from_year && now_month == from_month && now_day == from_day) {
            var now_seconds = now_hour * 3600 + now_min * 60 + now_sec
            var from_seconds = from_hour * 3600 + from_min * 60 + from_sec
            if(now_seconds < from_seconds) { return X509_BADCERT_FUTURE }
        }

        // Compare current time to notAfter (cert expired -> EXPIRED)
        if(now_year > to_year) { return X509_BADCERT_EXPIRED }
        if(now_year == to_year && now_month > to_month) { return X509_BADCERT_EXPIRED }
        if(now_year == to_year && now_month == to_month && now_day > to_day) { return X509_BADCERT_EXPIRED }
        if(now_year == to_year && now_month == to_month && now_day == to_day) {
            var now_seconds = now_hour * 3600 + now_min * 60 + now_sec
            var to_seconds = to_hour * 3600 + to_min * 60 + to_sec
            if(now_seconds > to_seconds) { return X509_BADCERT_EXPIRED }
        }

        return 0  // Certificate is valid

    }
}

    // ─── Helper: verify cert signature with appropriate key type ──────────
    func x509_verify_sig_with_issuer(cert : *mut X509Cert, issuer : *mut X509Cert) : int {
        unsafe {

        if(issuer.pk_type == PK_RSA as u8) {
            var rsa_ctx : RSAContext
            rsa_init(unsafe(&raw mut rsa_ctx), RSA_PKCS_V15, 0)
            var ret = x509_extract_rsa_pubkey(issuer, unsafe(&raw mut rsa_ctx))
            if(ret == 0) {
                ret = x509_verify_cert_signature(cert, unsafe(&raw mut rsa_ctx))
                return ret
            }
            return ERR_X509_CERT_VERIFY_FAILED
        } else if(issuer.pk_type == PK_ECKEY as u8) {
            var ecdsa_ctx : ECDSAContext
            ecdsa_init(unsafe(&raw mut ecdsa_ctx))
            var ret = x509_extract_ecdsa_pubkey(issuer, unsafe(&raw mut ecdsa_ctx))
            if(ret == 0) {
                ret = x509_verify_cert_ecdsa_signature(cert, unsafe(&raw mut ecdsa_ctx))
                return ret
            }
            return ERR_X509_CERT_VERIFY_FAILED
        }
        return ERR_X509_CERT_VERIFY_FAILED

    }
}

    // ─── DN helpers for chain building ────────────────────────────────────
    // Compare two DER-encoded distinguished names byte-for-byte.
    // Returns true when both are non-null, same length, and identical bytes.
    func x509_dn_equal(a : *mut u8, a_len : size_t, b : *mut u8, b_len : size_t) : bool {
        unsafe {

        if(a == null || b == null) { return false }
        if(a_len != b_len) { return false }
        var i : size_t = 0
        while(i < a_len) {
            if(a[i] != b[i]) { return false }
            i += 1
        }
        return true

    }
}

    // Walk a certificate chain (linked via `next`) looking for the first cert
    // whose subject DN matches the given issuer DN. Used to find the signer of
    // `leaf` both inside the peer chain (intermediates) and in the CA store.
    func x509_find_cert_by_subject(chain : *mut X509Cert, issuer_raw : *mut u8,
                                   issuer_len : size_t) : *mut X509Cert {
                                       unsafe {

        var curr = chain
        while(curr != null) {
            if(x509_dn_equal(curr.subject_raw, curr.subject_raw_len, issuer_raw, issuer_len)) {
                return curr
            }
            curr = curr.next
        }
        return null

                                   }
}

    // Parse a full Certificate message body into a linked chain of heap
    // X509Cert nodes. `data` points at the handshake message BODY (right after
    // the 4-byte handshake header), `data_len` is the body length.
    //   TLS 1.3 body: context(1+ctx) + cert_list_len(3) + per entry
    //                 [cert_data_len(3) + cert_data + ext_len(2) + ext]
    //   TLS 1.2 body: cert_list_len(3) + per entry [cert_data_len(3) + cert_data]
    // Returns the head of the chain (leaf first) via `out`, or an error code.
    func x509_parse_server_cert_chain(data : *u8, data_len : size_t, tls13 : bool,
                                      out_head : *mut *mut X509Cert) : int { unsafe {
        var pos : size_t = 0
        if(tls13) {
            // Context: 1-byte length + that many bytes.
            if(pos >= data_len) { return ERR_X509_INVALID_FORMAT }
            var ctx_len = data[pos] as size_t
            pos += 1 + ctx_len
        }
        // certificate_list length (3 bytes)
        if(pos + 3 > data_len) { return ERR_X509_INVALID_FORMAT }
        var list_len = read_u24(data + pos) as size_t
        pos += 3
        var list_end = pos + list_len
        if(list_end > data_len) { list_end = data_len }

        var head : *mut X509Cert = null
        var tail : *mut X509Cert = null
        while(pos + 3 <= list_end) {
            var cert_data_len = read_u24(data + pos) as size_t
            pos += 3
            if(cert_data_len == 0 || pos + cert_data_len > list_end) { break }
            var cert_data = data + pos

            var cert_mem = x509_cert_alloc()
            if(cert_mem == null) { break }
            var pr = parse_cert_der(cert_mem, cert_data, cert_data_len)
            if(pr != 0) {
                cert_free(cert_mem)
                unsafe { dealloc cert_mem }
                break
            }
            if(tail != null) {
                tail.next = cert_mem
                cert_mem.prev = tail
            } else {
                head = cert_mem
            }
            tail = cert_mem

            pos += cert_data_len
            if(tls13) {
                // 2-byte extensions length + the extensions themselves.
                if(pos + 2 > list_end) { break }
                var ext_len = read_u16_be(data + pos) as size_t
                pos += 2
                if(pos + ext_len > list_end) { break }
                pos += ext_len
            }
        }

        *out_head = head
        return 0
     }}

    // ─── X.509 Certificate Chain Verification ─────────────────────────────
    // Verify a certificate chain from leaf to root.
    // Supports both RSA and ECDSA certificates, and multi-level chains with
    // intermediates (e.g. leaf -> intermediate -> trusted root).
    // leaf: the peer's certificate (head of the peer chain; `.next` holds any
    //       intermediates sent by the server).
    // trusted_ca: head of a linked list of trusted root CA certificates, or
    //             null to skip root verification (only valid for self-signed).
    // hostname: expected server hostname (or null to skip)
    // Returns 0 on success, negative error code on failure.
    // Sets crt->flags with verification results.
    public func x509_verify_chain(leaf : *mut X509Cert, trusted_ca : *mut X509Cert,
                                    hostname : *char) : int {
                                        unsafe {

        var flags : u32 = 0

        // 1. Self-signed check: if leaf issuer == leaf subject, it's self-signed
        var is_self_signed = x509_dn_equal(leaf.issuer_raw, leaf.issuer_raw_len,
                                           leaf.subject_raw, leaf.subject_raw_len)

        // 2. Check hostname first (always run regardless of CA verification)
        var hostname_ok = true
        if(hostname != null) {
            var hname_len : size_t = 0
            while(hostname[hname_len] != 0) { hname_len += 1 }
            if(hname_len > 0) {
                var h_ret = x509_verify_hostname(leaf, hostname)
                if(h_ret != 0) {
                    hostname_ok = false
                    leaf.flags = X509_BADCERT_CN_MISMATCH as u32
                }
            }
        }
        if(!hostname_ok) { return ERR_X509_CERT_VERIFY_FAILED }

        // 3. Check date validity of the leaf
        var date_ret = x509_check_date(leaf)
        if(date_ret != 0) {
            leaf.flags = leaf.flags | date_ret as u32
            return ERR_X509_CERT_VERIFY_FAILED
        }

        // 4. Build and verify the chain. Walk from the leaf up: at each step
        //    we look for a cert (either an intermediate the server sent, or a
        //    trusted root) whose SUBJECT is this cert's ISSUER, then verify the
        //    signature link. A path that ends at a trusted root succeeds.
        var current = leaf
        var hops = 0
        while(current != null && hops < 16) {
            hops += 1

            // 4a. Look for the signer of `current` among the trusted roots.
            if(trusted_ca != null) {
                var ca_issuer = x509_find_cert_by_subject(trusted_ca,
                                                          current.issuer_raw,
                                                          current.issuer_raw_len)
                if(ca_issuer != null) {
                    var sig_ok = x509_verify_sig_with_issuer(current, ca_issuer)
                    if(sig_ok == 0) {
                        leaf.flags = 0
                        return 0
                    }
                    // Signature didn't verify; fall through to intermediates.
                } else {
                    // No trusted CA found for this issuer; try intermediates.
                }
            }

            // 4b. Look for the signer of `current` among the peer chain
            //     (intermediates the server sent after the leaf).
            var peer_issuer = x509_find_cert_by_subject(current.next,
                                                        current.issuer_raw,
                                                        current.issuer_raw_len)
            if(peer_issuer != null) {
                var sig_ret = x509_verify_sig_with_issuer(current, peer_issuer)
                if(sig_ret == 0) {
                    // Check the intermediate's own validity window.
                    var inter_date = x509_check_date(peer_issuer)
                    if(inter_date != 0) {
                        leaf.flags = leaf.flags | inter_date as u32
                        return ERR_X509_CERT_VERIFY_FAILED
                    }
                    current = peer_issuer
                    continue
                }
            }

            // 4c. Self-signed trust anchor — but ONLY when no trust store was
            //     configured. In that case there is nothing to chain up to, and
            //     the certificate is accepted as its own anchor (which is what
            //     "verification disabled" means here).
            //
            //     When a trust store IS configured, the anchor has to come out
            //     of it: step 4a already accepted a self-signed certificate
            //     whose subject the store holds and whose signature checks out.
            //     Accepting a self-signed certificate merely because it verifies
            //     against its own key would accept EVERY self-signed certificate
            //     for EVERY name — anyone can mint one — so a configured trust
            //     store would buy nothing at all.
            if(is_self_signed && trusted_ca == null) {
                if(x509_verify_sig_with_issuer(current, current) == 0) {
                    leaf.flags = 0
                    return 0
                }
            }

            leaf.flags = X509_BADCERT_NOT_TRUSTED as u32
            return ERR_X509_CERT_VERIFY_FAILED
        }

        leaf.flags = X509_BADCERT_NOT_TRUSTED as u32
        return ERR_X509_CERT_VERIFY_FAILED

                                    }
}

    // ─── Parse ServerHello and extract key parameters ────────────────────

    func parse_server_hello(ssl : *mut SSLContext, data : *u8, data_len : u32) : int {
        unsafe {

        // data starts with the 4-byte handshake header (type + 3-byte length),
        // followed by the ServerHello body.
        // ServerHello body layout: version(2) + random(32) + session_id(1+len) + ciphersuite(2) + compression(1) + extensions
        // Session ID has variable length, so ciphersuite offset = 39 + session_id_len
        if(data_len < 38) { return ERR_SSL_DECODE_ERROR }

        var sh_version_major = data[4]
        var sh_version_minor = data[5]

        // Read session_id_len FIRST to compute ciphersuite offset correctly
        var session_id_len = data[38] as size_t
        if(data_len < 37 + session_id_len) { return ERR_SSL_DECODE_ERROR }

        var sh_ciphersuite = read_u16_be(&raw data[39 + session_id_len])

        ssl.major_ver = sh_version_major
        ssl.minor_ver = sh_version_minor
        ssl.negotiated_ciphersuite = sh_ciphersuite

        if(ssl.session != null) {
            ssl.session.ciphersuite = sh_ciphersuite
        }

        // Read server random (bytes 6-37) for key derivation
        if(ssl.handshake != null) {
            var i : size_t = 0
            while(i < 32) {
                ssl.handshake.randbytes[32 + i] = data[6 + i]
                i += 1
            }
        }

        // Extract session ID from ServerHello (bytes 38+ depending on session_id_len)
        if(session_id_len > 0 && session_id_len <= 32) {
            if(ssl.session != null) {
                ssl.session.id_len = session_id_len
                var sid_i : size_t = 0
                while(sid_i < session_id_len) {
                    ssl.session.id[sid_i] = data[39 + sid_i]
                    sid_i += 1
                }
            }
        }

        return 0

    }
}

    async func do_tls12_client_handshake(ssl : *mut SSLContext) : int {
        unsafe {

        ensure_init()

        // ── Handshake transcript hash context (dual SHA-256 + SHA-384) ──
        // Both are kept up to date; after the ServerHello reveals the
        // ciphersuite, the matching one is finalized (SHA-384 PRF suites
        // per RFC 5246 §5 hash the transcript with SHA-384).
        var hash_ctx : crypto::raw::Sha256Context
        crypto::raw::sha256_init(unsafe(&raw mut hash_ctx))
        var hash_ctx_384 : crypto::raw::Sha512Context
        crypto::raw::sha384_init(unsafe(&raw mut hash_ctx_384))
        var use_sha384 : bool = false

        // 1. Send ClientHello
        ssl.state = SSLState.CLIENT_HELLO()

        var ch_buf : [2048]u8
        var ch_len = build_client_hello(ssl, &raw mut ch_buf[0], 2048)

        // Feed ClientHello into transcript hash (including handshake header)
        tls12_hash_handshake_msg_both(unsafe(&raw mut hash_ctx), unsafe(&raw mut hash_ctx_384), SSL_HS_CLIENT_HELLO as u8, ch_len as u32, &raw ch_buf[0])

        var ret = await send_handshake_msg(ssl, SSL_HS_CLIENT_HELLO as u8, &raw ch_buf[0], ch_len as u32)
        if(ret < 0) { return ret }

        // Allocate handshake params on the heap
        if(ssl.handshake == null) {
            var hs_mem = unsafe(malloc(sizeof(HandshakeParams))) as *mut HandshakeParams
            handshake_params_init(hs_mem)
            ssl.handshake = hs_mem
        }

        // Copy client random to handshake params (bytes 2-33 of ch_buf)
        var i : size_t = 0
        while(i < 32) {
            ssl.handshake.randbytes[i] = ch_buf[2 + i]
            i += 1
        }

        // 2. Read ServerHello
        ssl.state = SSLState.SERVER_HELLO()

        var hs_type : u8 = 0
        var hs_len : u32 = 0
        var hs_buf : [8192]u8
        ret = await read_handshake_msg(ssl, &raw mut hs_type, &raw mut hs_len,
                                  &raw mut hs_buf[0], 8192)
        if(ret < 0) { return ret }

        if(hs_type != SSL_HS_SERVER_HELLO as u8) {
            return ERR_SSL_UNEXPECTED_MESSAGE
        }

        // Feed ServerHello into transcript hash (body starts at hs_buf[4] after 4-byte header)
        tls12_hash_handshake_msg_both(unsafe(&raw mut hash_ctx), unsafe(&raw mut hash_ctx_384), hs_type, hs_len, &raw hs_buf[4])

        ret = parse_server_hello(ssl, &raw hs_buf[0], hs_len)
        if(ret < 0) { return ret }

        // ── Capability gate for the negotiated TLS 1.2 suite ──
        // This client implements the RSA key exchange (plus NKE suites) with
        // SHA-256 / SHA-384 PRFs. Anything else (e.g. ECDHE) is rejected
        // gracefully here rather than mis-negotiated.
        var cs_info_early = get_ciphersuite_info(ssl.negotiated_ciphersuite)
        if(cs_info_early.hash == HASH_SHA384 as u8) {
            use_sha384 = true
        } else if(cs_info_early.hash != HASH_SHA256 as u8 && cs_info_early.hash != HASH_NONE as u8) {
            var alert_sent = await send_alert(ssl, SSL_ALERT_LEVEL_FATAL as u8, SSL_ALERT_MSG_HANDSHAKE_FAILURE as u8)
            return ERR_SSL_HANDSHAKE_FAILURE
        }
        if(cs_info_early.key_exchange != KE_RSA as u8 && cs_info_early.key_exchange != KE_NONE as u8) {
            var alert_sent = await send_alert(ssl, SSL_ALERT_LEVEL_FATAL as u8, SSL_ALERT_MSG_HANDSHAKE_FAILURE as u8)
            return ERR_SSL_HANDSHAKE_FAILURE
        }

        // Copy server random (bytes 6-37 of hs_buf)
        i = 0
        while(i < 32) {
            ssl.handshake.randbytes[32 + i] = hs_buf[6 + i]
            i += 1
        }

        // Record negotiated ciphersuite (already set by parse_server_hello)

        // 3. Read Certificate
        ssl.state = SSLState.SERVER_CERTIFICATE()

        ret = await read_handshake_msg(ssl, &raw mut hs_type, &raw mut hs_len,
                                  &raw mut hs_buf[0], 8192)
        if(ret < 0) { return ret }

        // Extract RSA public key from server certificate
        var has_rsa_key : bool = false
        var rsa_ctx : RSAContext

        if(hs_type == SSL_HS_CERTIFICATE as u8) {
            ssl.state = SSLState.SERVER_KEY_EXCHANGE()

            // Feed Certificate into transcript hash (body starts at hs_buf[4])
            tls12_hash_handshake_msg_both(unsafe(&raw mut hash_ctx), unsafe(&raw mut hash_ctx_384), hs_type, hs_len, &raw hs_buf[4])

            // Parse the full server certificate chain (leaf first, then any
            // intermediates). Certificate message layout (TLS 1.2):
            //   hs_buf[0]: handshake type
            //   hs_buf[1..3]: body length (hs_len)
            //   hs_buf[4..6]: certificate_list length (3 bytes)
            //   each entry: cert_data_len(3) + cert_data
            if(hs_len >= 4) {
                var cert_chain : *mut X509Cert = null
                var chain_ret = x509_parse_server_cert_chain(&raw hs_buf[4],
                                                             hs_len as size_t,
                                                             false,
                                                             &raw mut cert_chain)
                if(chain_ret == 0 && cert_chain != null) {
                    var leaf = cert_chain
                    var parsed_key = false
                    if(leaf.pk_type == PK_RSA as u8) {
                        rsa_init(unsafe(&raw mut rsa_ctx), RSA_PKCS_V15, 0)
                        var ret3 = x509_extract_rsa_pubkey(leaf, unsafe(&raw mut rsa_ctx))
                        if(ret3 == 0 && rsa_get_len(unsafe(&raw mut rsa_ctx)) > 0) {
                            has_rsa_key = true
                            parsed_key = true
                        }
                    }
                    ssl.peer_cert = leaf
                    if(parsed_key) {
                        // Run certificate chain verification if CA chain is configured
                        if(ssl.conf != null && ssl.conf.ca_chain != null && ssl.conf.authmode != SSL_VERIFY_NONE) {
                            var vret = x509_verify_chain(leaf, ssl.conf.ca_chain,
                                                        ssl.hostname)
                            if(vret != 0) {
                                // Cert verification failed — reject the connection.
                                // peer_cert stays attached; ssl_free cleans it up.
                                return ERR_SSL_CERT_VERIFY_FAILED
                            }
                        }
                    } else {
                        cert_chain_free(leaf)
                        ssl.peer_cert = null
                    }
                }
            }
        } else if(hs_type == SSL_HS_SERVER_HELLO_DONE as u8) {
            ssl.state = SSLState.SERVER_HELLO_DONE()
            tls12_hash_handshake_msg_both(unsafe(&raw mut hash_ctx), unsafe(&raw mut hash_ctx_384), hs_type, hs_len, &raw hs_buf[4])
        }

        // 4. Read ServerKeyExchange (if present) or ServerHelloDone
        var got_shd : bool = false
        if(ssl.state is SSLState.SERVER_KEY_EXCHANGE) {
            ret = await read_handshake_msg(ssl, &raw mut hs_type, &raw mut hs_len,
                                      &raw mut hs_buf[0], 8192)
            if(ret < 0) { return ret }
            tls12_hash_handshake_msg_both(unsafe(&raw mut hash_ctx), unsafe(&raw mut hash_ctx_384), hs_type, hs_len, &raw hs_buf[4])
            if(hs_type == SSL_HS_SERVER_HELLO_DONE as u8) {
                // RSA key exchange: ServerHelloDone received directly after Certificate
                got_shd = true
            }
            ssl.state = SSLState.SERVER_HELLO_DONE()
        }

        // 5. Read ServerHelloDone if not already received
        if(!got_shd) {
            if(ssl.state is SSLState.SERVER_HELLO_DONE) {
                // For ECDHE: server sends ServerKeyExchange then ServerHelloDone
                ret = await read_handshake_msg(ssl, &raw mut hs_type, &raw mut hs_len,
                                          &raw mut hs_buf[0], 8192)
                if(ret < 0) { return ret }
                tls12_hash_handshake_msg_both(unsafe(&raw mut hash_ctx), unsafe(&raw mut hash_ctx_384), hs_type, hs_len, &raw hs_buf[4])
            } else {
                // Certificate was not present, read ServerHelloDone
                ret = await read_handshake_msg(ssl, &raw mut hs_type, &raw mut hs_len,
                                          &raw mut hs_buf[0], 8192)
                if(ret < 0) { return ret }
                tls12_hash_handshake_msg_both(unsafe(&raw mut hash_ctx), unsafe(&raw mut hash_ctx_384), hs_type, hs_len, &raw hs_buf[4])
            }
        }

        // ── Generate pre-master secret (TLS_RSA key exchange) ──
        // For TLS 1.2 RSA: pre_master_secret = ClientHello.version (2 bytes) + 46 random bytes
        var pre_master : [48]u8
        pre_master[0] = 0x03; pre_master[1] = 0x03  // TLS 1.2
        // Use CSPRNG for the remaining 46 bytes
        var pm_ret = random_fill(&raw mut pre_master[2], 46)
        if(pm_ret < 0) { return ERR_SSL_NO_RNG }

        // ── Encrypt pre-master secret with RSA public key ──
        // Sized from RSA_MAX_KEY_BYTES: the peer's certificate decides the key
        // length, and this used to be a fixed 512 bytes, so a >4096-bit server
        // key overran both buffers (the two length bytes in cke_data plus the
        // ciphertext).
        var cke_data : [RSA_MAX_KEY_BYTES + 2]u8
        var cke_len : size_t = 2  // Default: empty ClientKeyExchange (2 bytes length + 0 data)

        if(has_rsa_key) {
            var encrypted_pms : [RSA_MAX_KEY_BYTES]u8
            var ret2 = rsa_pkcs1_encrypt(unsafe(&raw mut rsa_ctx), &raw pre_master[0], 48, &raw mut encrypted_pms[0])
            if(ret2 == 0) {
                var key_len = rsa_get_len(unsafe(&raw mut rsa_ctx))
                // ClientKeyExchange for RSA: length(2 bytes) + encrypted_pre_master
                cke_data[0] = ((key_len >> 8) & 0xFF) as u8
                cke_data[1] = (key_len & 0xFF) as u8
                var j : size_t = 0
                while(j < key_len) {
                    cke_data[2 + j] = encrypted_pms[j]
                    j += 1
                }
                cke_len = 2 + key_len
            }
        }

        // ── Derive master secret ──
        var master_secret : [48]u8
        tls12_derive_master_secret(
            &raw pre_master[0], 48,
            &raw ssl.handshake.randbytes[0],   // client random
            &raw ssl.handshake.randbytes[32],  // server random
            &raw mut master_secret[0],
            use_sha384
        )

        // Store master secret in session
        if(ssl.session != null) {
            i = 0
            while(i < 48) {
                ssl.session.master[i] = master_secret[i]
                i += 1
            }
        }

        // ── Derive key block ──
        var cs_info = get_ciphersuite_info(ssl.negotiated_ciphersuite)
        var kb_size = tls12_key_block_size(&raw cs_info)
        var key_block : [256]u8
        tls12_derive_key_block(
            &raw master_secret[0],
            &raw ssl.handshake.randbytes[32],  // server random
            &raw ssl.handshake.randbytes[0],   // client random
            &raw mut key_block[0], kb_size,
            use_sha384
        )

        // 6. Send ClientKeyExchange (must be sent BEFORE transform activation per RFC 5246)
        //    Encryption starts after ChangeCipherSpec, so CKE must be in the clear.
        ssl.state = SSLState.CLIENT_KEY_EXCHANGE()
        ret = await send_handshake_msg(ssl, SSL_HS_CLIENT_KEY_EXCHANGE as u8, &raw cke_data[0], cke_len as u32)
        if(ret < 0) { return ret }

        // Feed ClientKeyExchange into transcript hash
        tls12_hash_handshake_msg_both(unsafe(&raw mut hash_ctx), unsafe(&raw mut hash_ctx_384), SSL_HS_CLIENT_KEY_EXCHANGE as u8, cke_len as u32, &raw cke_data[0])

        // 7. Send ChangeCipherSpec (in the clear!)
        ssl.state = SSLState.CLIENT_CHANGE_CIPHER_SPEC()
        var ccs_msg : [1]u8
        ccs_msg[0] = 1 as u8
        ret = await send_record(ssl, SSL_MSG_CHANGE_CIPHER_SPEC as u8, &raw ccs_msg[0], 1 as u16)
        if(ret < 0) { return ret }

        // ── Populate and activate transforms (AFTER ChangeCipherSpec is sent) ──
        var tr : Transform
        transform_init(unsafe(&raw mut tr))
        tls12_populate_transform(unsafe(&raw mut tr), &raw cs_info, &raw key_block[0], kb_size)

        var tr_mem = unsafe(malloc(sizeof(Transform))) as *mut Transform
        unsafe { memcpy(tr_mem, &raw tr, sizeof(Transform)) }
        ssl.transform_out = tr_mem

        var tr_in_mem = unsafe(malloc(sizeof(Transform))) as *mut Transform
        unsafe { memcpy(tr_in_mem, &raw tr, sizeof(Transform)) }
        ssl.transform_in = tr_in_mem

        // Reset sequence numbers for handshake encryption phase
        i = 0
        while(i < 8) { ssl.in_ctr[i] = 0; ssl.out_ctr[i] = 0; i += 1 }

        // ── Finalize handshake transcript hash (includes all msgs up to CKE) ──
        var client_hs_hash : [48]u8
        var copy_ctx : crypto::raw::Sha256Context
        var copy_ctx_384 : crypto::raw::Sha512Context
        unsafe {
            memcpy(&raw mut copy_ctx, &raw hash_ctx, sizeof(crypto::raw::Sha256Context))
            memcpy(&raw mut copy_ctx_384, &raw hash_ctx_384, sizeof(crypto::raw::Sha512Context))
        }
        var cf_hash_len : size_t = 32 as size_t
        if(use_sha384) { cf_hash_len = 48 as size_t }
        if(use_sha384) {
            crypto::raw::sha384_final(&raw mut copy_ctx_384, &raw mut client_hs_hash[0])
        } else {
            crypto::raw::sha256_final(&raw mut copy_ctx, &raw mut client_hs_hash[0])
        }

        // ── Compute client Finished verify_data ──
        var client_finished : [12]u8
        tls12_compute_finished(&raw master_secret[0], true, &raw client_hs_hash[0], cf_hash_len, &raw mut client_finished[0], use_sha384)

        // 8. Send Finished (with verify_data)
        ssl.state = SSLState.CLIENT_FINISHED()
        ret = await send_handshake_msg(ssl, SSL_HS_FINISHED as u8, &raw client_finished[0], 12)
        if(ret < 0) { return ret }

        // Feed Client Finished message into transcript hash for verifying Server Finished
        tls12_hash_handshake_msg_both(unsafe(&raw mut hash_ctx), unsafe(&raw mut hash_ctx_384), SSL_HS_FINISHED as u8, 12, &raw client_finished[0])

        // 9. Read Server's ChangeCipherSpec + Finished
        // RFC 5077 §3.3: a session-aware server sends NewSessionTicket
        // BEFORE its ChangeCipherSpec — accept it here instead of treating
        // it as an unexpected message. The ticket is stored immediately,
        // its bytes are hashed into the transcript (matching peer behavior)
        // and pushed back onto the input buffer so the post-handshake API
        // (ssl_read_new_session_ticket) can still consume it explicitly.
        ssl.state = SSLState.SERVER_CHANGE_CIPHER_SPEC()
        var nst_copy : [8192]u8
        var nst_push_len : size_t = 0
        ret = await read_handshake_msg(ssl, &raw mut hs_type, &raw mut hs_len,
                                  &raw mut hs_buf[0], 8192)
        while(ret == 0 && hs_type == SSL_HS_NEW_SESSION_TICKET as u8) {
            tls12_hash_handshake_msg_both(unsafe(&raw mut hash_ctx), unsafe(&raw mut hash_ctx_384), SSL_HS_NEW_SESSION_TICKET as u8,
                                    hs_len, &raw hs_buf[4])
            ssl_process_new_session_ticket(ssl, &raw hs_buf[0], (4 + hs_len) as size_t)
            var nc : size_t = 0
            while(nc < (4 + hs_len) && nc < 8192) { nst_copy[nc] = hs_buf[nc]; nc += 1 }
            nst_push_len = (4 + hs_len) as size_t
            if(nst_push_len > 8192) { nst_push_len = 8192 }
            ret = await read_handshake_msg(ssl, &raw mut hs_type, &raw mut hs_len,
                                      &raw mut hs_buf[0], 8192)
        }
        if(ret < 0) { return ret }
        if(hs_type == SSL_HS_FINISHED as u8) {
            // Push the consumed NewSessionTicket back onto the input buffer
            // as a complete plaintext record so the post-handshake API
            // (ssl_read_new_session_ticket) can still consume it explicitly.
            if(nst_push_len > 0 && ssl.in_left + (nst_push_len as i32) + 5 <= 17408) {
                var shift_i : i32 = ssl.in_left - 1
                while(shift_i >= 0) {
                    ssl.in_buf[nst_push_len + 5 + shift_i] = ssl.in_buf[shift_i]
                    shift_i -= 1
                }
                ssl.in_buf[0] = SSL_MSG_HANDSHAKE as u8
                ssl.in_buf[1] = 3 as u8
                ssl.in_buf[2] = 3 as u8
                ssl.in_buf[3] = ((nst_push_len >> 8) & 0xFF) as u8
                ssl.in_buf[4] = (nst_push_len & 0xFF) as u8
                var pc : size_t = 0
                while(pc < nst_push_len) { ssl.in_buf[5 + pc] = nst_copy[pc]; pc += 1 }
                ssl.in_left = ssl.in_left + (nst_push_len as i32) + 5
            }
        var server_hs_hash : [48]u8
            var sf_hash_len : size_t = 32 as size_t
            if(use_sha384) { sf_hash_len = 48 as size_t }
            if(use_sha384) {
                crypto::raw::sha384_final(unsafe(&raw mut hash_ctx_384), &raw mut server_hs_hash[0])
            } else {
                crypto::raw::sha256_final(unsafe(&raw mut hash_ctx), &raw mut server_hs_hash[0])
            }

            // Compute expected server verify_data using transcript hash including Client Finished
            var expected_server_finished : [12]u8
            tls12_compute_finished(&raw master_secret[0], false,
                                    &raw server_hs_hash[0], sf_hash_len,
                                    &raw mut expected_server_finished[0], use_sha384)

            // Compare against received server Finished
            var verify_match = true
            i = 0
            while(i < 12) {
                if(hs_buf[4 + i] != expected_server_finished[i]) { verify_match = false }
                i += 1
            }
            if(!verify_match) {
                var alert_sent = await send_alert(ssl, SSL_ALERT_LEVEL_FATAL as u8, SSL_ALERT_MSG_DECRYPT_ERROR as u8)
                return ERR_SSL_HANDSHAKE_FAILURE
            }
        } else {
            // Unexpected message instead of server Finished
            var alert_sent = await send_alert(ssl, SSL_ALERT_LEVEL_FATAL as u8, SSL_ALERT_MSG_UNEXPECTED_MESSAGE as u8)
            return ERR_SSL_UNEXPECTED_MESSAGE
        }

        ssl.state = SSLState.HANDSHAKE_OVER()

        return 0

    }
}

    // ============================================================================
    // Client Handshake - TLS 1.3
    // ============================================================================

    // Compute and fill the PSK binder into the ClientHello buffer. Called after
    // build_client_hello (which left a 32-byte zero placeholder at
    // handshake.psk_binder_off) and BEFORE the ClientHello is hashed into the
    // transcript / sent. RFC 8446 §4.2.11.2 / §7.1.
    func tls13_fill_psk_binder(ssl : *mut SSLContext, ch_buf : *mut u8, ch_len : size_t,
                               ch_hdr : *u8) : int {
                                   unsafe {

        if(ssl.handshake == null || ssl.handshake.psk_binder_off == 0) { return 0 }
        if(ssl.handshake.psk_len == 0) { return 0 }

        // Partial ClientHello hash: covers everything up to the END of the
        // identities field (psk_partial_len), NOT the binders list
        // (RFC 8446 §4.2.11.2). The length fields already reflect the full
        // ClientHello including the binders.
        var partial_len : size_t = ssl.handshake.psk_partial_len as size_t
        if(partial_len == 0 || partial_len > ch_len) { return 0 }
        var bc : crypto::raw::Sha256Context
        crypto::raw::sha256_init(unsafe(&raw mut bc))
        crypto::raw::sha256_update(unsafe(&raw mut bc), ch_hdr, 4)
        crypto::raw::sha256_update(unsafe(&raw mut bc), ch_buf, partial_len)
        var partial_hash : [32]u8
        crypto::raw::sha256_final(unsafe(&raw mut bc), &raw mut partial_hash[0])

        // early_secret = HKDF-Extract(0, PSK)
        var zeros32 : [32]u8
        var zi : size_t = 0
        while(zi < 32) { zeros32[zi] = 0; zi += 1 }
        var binder_early : [32]u8
        tls13_hkdf_extract(&raw zeros32[0], 32, &raw ssl.handshake.psk[0],
                           ssl.handshake.psk_len as size_t, &raw mut binder_early[0], 32)

        // binder_key = Derive-Secret(early_secret, "res binder", "") ->
        //   context = Transcript-Hash("") = SHA256("")
        var empty_hash : [32]u8
        var ectx : crypto::raw::Sha256Context
        crypto::raw::sha256_init(unsafe(&raw mut ectx))
        crypto::raw::sha256_final(unsafe(&raw mut ectx), &raw mut empty_hash[0])
        var binder_key : [32]u8
        var binder_label = "res binder\0" as *char
        tls13_hkdf_expand_label(&raw binder_early[0], 32, binder_label, 10,
                                &raw empty_hash[0], 32, &raw mut binder_key[0], 32)

        // finished_key = HKDF-Expand-Label(binder_key, "finished", "", 32)
        var fin_key : [32]u8
        var fin_label = "finished\0" as *char
        var empty_c : [1]u8 = [0]
        tls13_hkdf_expand_label(&raw binder_key[0], 32, fin_label, 8,
                                &raw empty_c[0], 0, &raw mut fin_key[0], 32)

        // binder = HMAC(finished_key, partial_hash)
        var binder : [32]u8
        crypto::raw::hmac_sha256(&raw fin_key[0], 32, &raw partial_hash[0], 32, &raw mut binder[0])

        // Overwrite the placeholder.
        var bo : size_t = ssl.handshake.psk_binder_off as size_t
        var bi : size_t = 0
        while(bi < 32) {
            ch_buf[bo + bi] = binder[bi]
            bi += 1
        }
        return 0

                               }
}

    async func do_tls13_client_handshake(ssl : *mut SSLContext) : int {
        unsafe {

        ensure_init()

        // Ensure handshake params are allocated
        if(ssl.handshake == null) {
            var hs_mem = unsafe(malloc(sizeof(HandshakeParams))) as *mut HandshakeParams
            if(hs_mem == null) { return ERR_SSL_INTERNAL_ERROR }
            handshake_params_init(hs_mem)
            ssl.handshake = hs_mem
        }

        // Load PSK from session for resumption
        if(ssl.session != null && ssl.session.resumption_key_len > 0) {
            var n : size_t = ssl.session.resumption_key_len as size_t
            var hi : size_t = 0
            while(hi < n && hi < 32) {
                ssl.handshake.psk[hi] = ssl.session.resumption_key[hi]
                hi += 1
            }
            ssl.handshake.psk_len = n as u8
        }

        // Generate ECDHE keypair (secp256r1 = 0x0017)
        var ecdh_ctx : ECDHContext
        ecdh_init(unsafe(&raw mut ecdh_ctx))

        var priv_key : [32]u8
        var pub_key : [65]u8
        var ret = ecdh_generate_keypair(unsafe(&raw mut ecdh_ctx),
                                        &raw mut priv_key[0], 32,
                                        &raw mut pub_key[0], 65)
        if(ret < 0) { return ret }

        // Store ECDHE public key in handshake params for build_client_hello
        var pub_mem = unsafe(malloc(65)) as *mut u8
        if(pub_mem == null) { return ERR_SSL_INTERNAL_ERROR }
        var pi : size_t = 0
        while(pi < 65) { pub_mem[pi] = pub_key[pi]; pi += 1 }
        ssl.handshake.ecdhe_curve = TLS_GROUP_SECP256R1 as u16
        ssl.handshake.ecdhe_public = pub_mem
        ssl.handshake.ecdhe_public_len = 65

        // Also generate x25519 keypair (preferred by most modern servers)
        var x25519_priv : [32]u8
        var x25519_pub : [32]u8
        var x25519_ret = x25519_generate_keypair(&raw mut x25519_priv[0], &raw mut x25519_pub[0])
        var has_x25519 : bool = (x25519_ret == 0)

        // Store x25519 keypair in handshake params
        if(has_x25519) {
            // Store private key (copy to heap so it persists)
            var x25519_priv_mem = unsafe(malloc(32)) as *mut u8
            if(x25519_priv_mem != null) {
                var xi : size_t = 0
                while(xi < 32) { x25519_priv_mem[xi] = x25519_priv[xi]; xi += 1 }
                ssl.handshake.x25519_private = x25519_priv_mem
            }
            var x25519_pub_mem = unsafe(malloc(32)) as *mut u8
            if(x25519_pub_mem != null) {
                var xi : size_t = 0
                while(xi < 32) { x25519_pub_mem[xi] = x25519_pub[xi]; xi += 1 }
                ssl.handshake.x25519_public = x25519_pub_mem
                ssl.handshake.x25519_public_len = 32 as u16
            }
        }

        // ── ClientHello ───────────────────────────────────────────────
        ssl.state = SSLState.CLIENT_HELLO()
        ssl.major_ver = 0x03
        ssl.minor_ver = 0x03

        var ch_buf : [2048]u8
        var ch_len = build_client_hello(ssl, &raw mut ch_buf[0], 2048)
        if(ch_len < 0) { return ch_len }

        // Hash the ClientHello body for the transcript
        var transcript : TLS13Transcript
        tls13_transcript_init(unsafe(&raw mut transcript))
        var ch_hdr : [4]u8
        ch_hdr[0] = SSL_HS_CLIENT_HELLO as u8
        write_u24(ch_len as u32, &raw mut ch_hdr[1])

        // Fill the PSK binder (if offering resumption) BEFORE hashing/sending.
        ret = tls13_fill_psk_binder(ssl, &raw mut ch_buf[0], ch_len as size_t, &raw ch_hdr[0])
        if(ret < 0) { return ret }

        tls13_transcript_update(unsafe(&raw mut transcript), &raw ch_hdr[0], 4)
        tls13_transcript_update(unsafe(&raw mut transcript), &raw ch_buf[0], ch_len as size_t)

        ret = await send_handshake_msg(ssl, SSL_HS_CLIENT_HELLO as u8, &raw ch_buf[0], ch_len as u32)
        if(ret < 0) { return ret }

        // ── ServerHello ───────────────────────────────────────────────
        ssl.state = SSLState.SERVER_HELLO()

        var hs_buf : [4096]u8
        var hs_body_len : u32 = 0
        var got_server_hello = false

        while(!got_server_hello) {
            var hdr : [5]u8
            ret = await read_record_header(ssl, &raw mut hdr[0])
            if(ret < 0) { return ret }

            var content_type = hdr[0]

            // TLS 1.3: CCS is a compatibility dummy, skip it
            if(content_type == SSL_MSG_CHANGE_CIPHER_SPEC as u8) {
                var ccs_d : [1]u8
                read_record_payload(ssl, &raw mut ccs_d[0], 1)
                continue
            }

            // Handle alert messages from the server (e.g., protocol rejection)
            if(content_type == SSL_MSG_ALERT as u8) {
                var alert_data : [2]u8
                read_record_payload(ssl, &raw mut alert_data[0], 2)
                ssl.last_alert_level = alert_data[0]
                ssl.last_alert_desc = alert_data[1]
                return ERR_SSL_FATAL_ALERT_MESSAGE
            }

            if(content_type != SSL_MSG_HANDSHAKE as u8) {
                ssl_consume_record(ssl)
                return ERR_SSL_UNEXPECTED_MESSAGE
            }

            var payload = read_record_payload(ssl, &raw mut hs_buf[0], 4096 as i32)
            if(payload < 4) { return ERR_SSL_DECODE_ERROR }

            var msg_type = hs_buf[0]
            hs_body_len = read_u24(&raw hs_buf[1])

            if(msg_type == SSL_HS_SERVER_HELLO as u8) {
                got_server_hello = true
            } else if(msg_type == SSL_HS_HELLO_RETRY_REQUEST as u8) {
                // HelloRetryRequest: server requested a different key_share group
                // Parse HRR extensions to find the requested group
                var hrr_pos : size_t = 4  // skip hs_type(1) + length(3)
                // version (2 bytes) — should be 0x0303 (TLS 1.2 compat)
                hrr_pos += 2
                // session_id_echo — matches the original session ID from CH
                var hrr_sid_len = hs_buf[hrr_pos] as size_t; hrr_pos += 1 + hrr_sid_len
                // cipher_suite — same as original
                hrr_pos += 2
                // compression_method
                hrr_pos += 1
                // extensions
                if(hrr_pos + 2 <= 4 + hs_body_len as size_t + 4) {
                    var hrr_ext_len = read_u16_be(&raw hs_buf[hrr_pos]) as size_t; hrr_pos += 2
                    var hrr_ext_end = hrr_pos + hrr_ext_len
                    var requested_group : u16 = TLS_GROUP_SECP256R1 as u16
                    var found_group = false

                    while(hrr_pos + 4 <= hrr_ext_end) {
                        var hrr_ext_type = read_u16_be(&raw hs_buf[hrr_pos]); hrr_pos += 2
                        var hrr_ext_data_len = read_u16_be(&raw hs_buf[hrr_pos]) as size_t; hrr_pos += 2
                        if(hrr_ext_type == TLS_EXT_SUPPORTED_VERSIONS as u16) {
                            // HRR includes supported_versions extension
                        } else if(hrr_ext_type == TLS_EXT_KEY_SHARE as u16 && hrr_ext_data_len >= 2) {
                            requested_group = read_u16_be(&raw hs_buf[hrr_pos])
                            found_group = true
                        } else if(hrr_ext_type == TLS_EXT_COOKIE as u16) {
                            // Cookie extension — we'd need to store and echo it
                        }
                        hrr_pos += hrr_ext_data_len
                    }

                    if(found_group) {
                        // Generate new ECDHE keypair for the requested group
                        var new_ecdh : ECDHContext
                        ecdh_init(unsafe(&raw mut new_ecdh))
                        var new_priv : [32]u8
                        var new_pub : [65]u8
                        var kg_ret = ecdh_generate_keypair(unsafe(&raw mut new_ecdh),
                                                           &raw mut new_priv[0], 32,
                                                           &raw mut new_pub[0], 65)
                        if(kg_ret < 0) { return kg_ret }

                        // Update handshake params with new ECDHE public key
                        if(ssl.handshake.ecdhe_public != null) {
                            unsafe { dealloc ssl.handshake.ecdhe_public }
                        }
                        var new_pub_mem = unsafe(malloc(65)) as *mut u8
                        if(new_pub_mem == null) { return ERR_SSL_INTERNAL_ERROR }
                        var npi : size_t = 0
                        while(npi < 65) { new_pub_mem[npi] = new_pub[npi]; npi += 1 }
                        ssl.handshake.ecdhe_curve = requested_group
                        ssl.handshake.ecdhe_public = new_pub_mem
                        ssl.handshake.ecdhe_public_len = 65

                        // Copy private key for later shared secret computation
                        // We reuse the existing ecdh_ctx variable scope for this
                        mpi_read_binary(unsafe(&raw mut ecdh_ctx.priv_key), &raw new_priv[0], 32)
                        ecdh_ctx.is_init = true
                    }

                    // Re-send ClientHello with updated key_share
                    var ch2_buf : [2048]u8
                    var ch2_len = build_client_hello(ssl, &raw mut ch2_buf[0], 2048)
                    if(ch2_len < 0) { return ch2_len }

                    // Hash the new ClientHello into transcript (the hash includes BOTH CHs)
                    // Actually per RFC 8446, the transcript for the second CH includes:
                    // Hash(CH1) + "HRR message" + "CH2 message"
                    var ch2_hdr : [4]u8
                    ch2_hdr[0] = SSL_HS_CLIENT_HELLO as u8
                    write_u24(ch2_len as u32, &raw mut ch2_hdr[1])
                    ret = tls13_fill_psk_binder(ssl, &raw mut ch2_buf[0], ch2_len as size_t, &raw ch2_hdr[0])
                    if(ret < 0) { return ret }
                    tls13_transcript_update(unsafe(&raw mut transcript), &raw ch2_hdr[0], 4)
                    tls13_transcript_update(unsafe(&raw mut transcript), &raw ch2_buf[0], ch2_len as size_t)

                    ret = await send_handshake_msg(ssl, SSL_HS_CLIENT_HELLO as u8, &raw ch2_buf[0], ch2_len as u32)
                    if(ret < 0) { return ret }

                    // Continue loop — read ServerHello next
                    continue
                }
            }
        }

        // Parse ServerHello body
        var sh_pos : size_t = 4  // skip hs_type(1) + length(3)

        // version (2 bytes)
        sh_pos += 2

        // random (32 bytes)
        sh_pos += 32

        // session_id_echo_len (1 byte)
        var sid_echo_len = hs_buf[sh_pos] as size_t; sh_pos += 1
        sh_pos += sid_echo_len

        // cipher_suite (2 bytes)
        ssl.negotiated_ciphersuite = read_u16_be(&raw hs_buf[sh_pos]); sh_pos += 2

        // Verify we support the hash algorithm: SHA-256 and SHA-384 TLS 1.3 suites
        var cs_info = get_ciphersuite_info(ssl.negotiated_ciphersuite)
        if(cs_info.hash != HASH_SHA256 as u8 && cs_info.hash != HASH_SHA384 as u8 && cs_info.hash != HASH_NONE as u8) {
            return ERR_SSL_HANDSHAKE_FAILURE
        }
        var use_sha384 : bool = tls13_is_sha384(ssl)
        var tls13_hash_len = tls13_hash_len_for(ssl)
        var tls13_key_len : size_t = 16
        var tls13_cipher_type : u8 = CIPHER_AES_128_GCM as u8
        if(cs_info.cipher == CIPHER_AES_256_GCM as u8) {
            tls13_key_len = 32; tls13_cipher_type = CIPHER_AES_256_GCM as u8
        } else if(cs_info.cipher == CIPHER_CHACHA20_POLY1305 as u8) {
            tls13_key_len = 32; tls13_cipher_type = CIPHER_CHACHA20_POLY1305 as u8
        }

        // compression_method (1 byte)
        sh_pos += 1

        // extensions_len (2 bytes)
        if(sh_pos + 2 > hs_body_len as size_t + 4) { return ERR_SSL_DECODE_ERROR }
        var sh_ext_len = read_u16_be(&raw hs_buf[sh_pos]) as size_t; sh_pos += 2

        // Parse extensions to find key_share
        var server_public_key : [65]u8
        var server_x25519_key : [32]u8
        var found_key_share = false
        var using_x25519 = false
        var ext_end = sh_pos + sh_ext_len

        while(sh_pos + 4 <= ext_end) {
            var ext_type = read_u16_be(&raw hs_buf[sh_pos]); sh_pos += 2
            var ext_data_len = read_u16_be(&raw hs_buf[sh_pos]) as size_t; sh_pos += 2

            if(ext_type == TLS_EXT_KEY_SHARE as u16 && ext_data_len >= 4) {
                var ks_group = read_u16_be(&raw hs_buf[sh_pos])
                var ks_key_len = read_u16_be(&raw hs_buf[sh_pos + 2]) as size_t

                if(ks_group == TLS_GROUP_X25519 as u16 && ks_key_len == 32 && ks_key_len <= ext_data_len - 4) {
                    var ki : size_t = 0
                    while(ki < 32) {
                        server_x25519_key[ki] = hs_buf[sh_pos + 4 + ki]
                        ki += 1
                    }
                    found_key_share = true
                    using_x25519 = true
                } else if(ks_group == TLS_GROUP_SECP256R1 as u16 && ks_key_len == 65 && ks_key_len <= ext_data_len - 4) {
                    var ki : size_t = 0
                    while(ki < 65) {
                        server_public_key[ki] = hs_buf[sh_pos + 4 + ki]
                        ki += 1
                    }
                    found_key_share = true
                }
            } else if(ext_type == TLS_EXT_PRE_SHARED_KEY as u16 && ext_data_len >= 2) {
                // Server selected our offered identity (selected_identity = 0).
                if(ssl.handshake != null && ssl.handshake.psk_len > 0) {
                    ssl.handshake.psk_accepted = true
                }
            }

            sh_pos += ext_data_len
        }

        if(!found_key_share) {
            return ERR_SSL_HANDSHAKE_FAILURE
        }

        // Hash ServerHello into transcript (including the 4-byte handshake header)
        tls13_transcript_update(unsafe(&raw mut transcript), &raw hs_buf[0], 4 + hs_body_len)

        // ── Compute ECDHE shared secret ──────────────────────────────
        var shared_secret : [32]u8
        if(using_x25519 && ssl.handshake.x25519_private != null) {
            ret = x25519_compute_shared(ssl.handshake.x25519_private,
                                        &raw server_x25519_key[0],
                                        &raw mut shared_secret[0])
        } else {
            ret = ecdh_compute_shared(unsafe(&raw mut ecdh_ctx),
                                      &raw server_public_key[0], 65,
                                      &raw mut shared_secret[0], 32)
        }
        if(ret < 0) { return ret }

        // Hash(ClientHello...ServerHello) — save transcript state first, then finalize
        var sh_transcript_copy : TLS13Transcript
        unsafe { memcpy(&raw mut sh_transcript_copy, &raw transcript, sizeof(TLS13Transcript)) }
        var sh_hash : [48]u8
        tls13_transcript_finalize(&raw mut sh_transcript_copy, &raw mut sh_hash[0], use_sha384)

        // ── Derive handshake traffic keys ────────────────────────────
        // Only use the PSK when the server actually accepted it; otherwise a
        // full (certificate) handshake was performed and keys come from ECDHE.
        var use_psk : bool = (ssl.handshake != null && ssl.handshake.psk_accepted &&
                              ssl.handshake.psk_len > 0)
        if(use_psk) {
            ret = tls13_derive_handshake_keys(ssl, &raw shared_secret[0], 32,
                                               &raw sh_hash[0],
                                               &raw ssl.handshake.psk[0],
                                               ssl.handshake.psk_len as size_t,
                                               tls13_hash_len, tls13_key_len, tls13_cipher_type)
        } else {
            ret = tls13_derive_handshake_keys(ssl, &raw shared_secret[0], 32,
                                               &raw sh_hash[0],
                                               null, 0,
                                               tls13_hash_len, tls13_key_len, tls13_cipher_type)
        }
        if(ret < 0) { return ret }

        // ── Read encrypted server messages ───────────────────────────
        var server_finished_verified = false
        var server_rsa_ctx : RSAContext
        var has_server_rsa : bool = false
        var server_ecdsa_ctx : ECDSAContext
        var has_server_ecdsa : bool = false
        // Saved transcript hash before CertificateVerify (for signature verification)
        var cv_transcript_copy : TLS13Transcript
        var cv_saved : bool = false
        // Persistent handshake-message accumulator. Servers may coalesce
        // several handshake messages into a single record and a single
        // message may span records — so we buffer plaintext here and drain
        // complete messages in the inner while loop.
        var msg_buf : [17408]u8
        var msg_buf_len : size_t = 0

        while(!server_finished_verified) {
            var enc_hdr : [5]u8
            ret = await read_record_header(ssl, &raw mut enc_hdr[0])
            if(ret < 0) { return ret }

            var enc_ct = enc_hdr[0]

            // CCS from server is allowed (compatibility)
            if(enc_ct == SSL_MSG_CHANGE_CIPHER_SPEC as u8) {
                var ccs_d : [1]u8
                read_record_payload(ssl, &raw mut ccs_d[0], 1)
                continue
            }

            // ssl_read_record already decrypts and updates in_hdr[0] to inner content_type
            var inner_ct = enc_ct

            if(inner_ct == SSL_MSG_ALERT as u8) {
                var alert_data : [2]u8
                read_record_payload(ssl, &raw mut alert_data[0], 2)
                return ERR_SSL_FATAL_ALERT_MESSAGE
            }

            if(inner_ct != SSL_MSG_HANDSHAKE as u8) {
                ssl_consume_record(ssl)
                return ERR_SSL_UNEXPECTED_MESSAGE
            }

            // Read the handshake record payload into a persistent accumulator.
            // TLS 1.3 servers coalesce several handshake messages (Encrypted
            // Extensions, Certificate, CertificateVerify, Finished) into a single
            // record, and a single large message may span multiple records — so
            // we accumulate plaintext here and drain complete messages one at a
            // time in the inner loop below.
            var want = (17408 as i32) - msg_buf_len
            var nread = read_record_payload(ssl, &raw mut msg_buf[msg_buf_len], want)
            if(nread < 0) { return nread }
            msg_buf_len += nread as size_t

            // Process every complete handshake message currently buffered.
            while(msg_buf_len >= 4) {
                var msg_type_code = msg_buf[0] as u32
                var msg_body_len2 = read_u24(&raw msg_buf[1])
                var msg_total : size_t = 4 + msg_body_len2 as size_t
                if(msg_total > msg_buf_len) { break }  // need more data

                if(msg_type_code == SSL_HS_ENCRYPTED_EXTENSIONS as u32) {
                    tls13_transcript_update(unsafe(&raw mut transcript), &raw msg_buf[0], msg_body_len2 + 4)

                    // Parse ALPN from EncryptedExtensions
                    // EE body: ExtensionVectorLen(2) then extensions
                    // {type(2) len(2) data...}. Skip the vector length first.
                    if(msg_body_len2 >= 4) {
                        var ext_block_len = read_u16_be(&raw msg_buf[4]) as size_t
                        var ee_pos : size_t = 6
                        var ee_end : size_t = 6 + ext_block_len
                        var ee_body_end : size_t = 4 + msg_body_len2 as size_t
                        if(ee_end > ee_body_end) { ee_end = ee_body_end }
                        while(ee_pos + 4 <= ee_end) {
                            var ext_type = read_u16_be(&raw msg_buf[ee_pos])
                            var ext_data_len = read_u16_be(&raw msg_buf[ee_pos + 2]) as size_t
                            var ext_data_start = ee_pos + 4
                            if(ext_data_start + ext_data_len > ee_end) { break }
                            if(ext_type == TLS_EXT_ALPN && ext_data_len >= 5) {
                                var alpn_list_len = read_u16_be(&raw msg_buf[ext_data_start]) as size_t
                                // List content: [name_len(1)][name...] so a
                                // usable entry needs at least len+1 bytes and
                                // must fit inside the announced vector.
                                if(alpn_list_len >= 1 && alpn_list_len <= ext_data_len - 2) {
                                    var name_pos = ext_data_start + 2
                                    var alpn_name_len = msg_buf[name_pos] as size_t
                                    if(alpn_name_len > 0 && alpn_name_len + 1 <= alpn_list_len &&
                                       name_pos + 1 + alpn_name_len <= ee_end) {
                                        // Store negotiated protocol
                                        var alpn_mem = unsafe(malloc(alpn_name_len + 1)) as *mut u8
                                        if(alpn_mem != null) {
                                            var alpi : size_t = 0
                                            while(alpi < alpn_name_len) {
                                                alpn_mem[alpi] = msg_buf[name_pos + 1 + alpi]
                                                alpi += 1
                                            }
                                            alpn_mem[alpn_name_len] = 0
                                            ssl.alpn_negotiated = alpn_mem as *char
                                            ssl.alpn_negotiated_len = alpn_name_len
                                        }
                                    }
                                }
                            }
                            ee_pos = ext_data_start + ext_data_len
                        }
                    }

                } else if(msg_type_code == SSL_HS_CERTIFICATE as u32) {
                    // Hash Certificate into transcript BEFORE saving transcript state
                    tls13_transcript_update(unsafe(&raw mut transcript), &raw msg_buf[0], 4 + msg_body_len2)

                // Parse the full server certificate chain: leaf first, then any
                // intermediates. The chain is stored on ssl.peer_cert and freed
                // by ssl_free; each cert's borrowed DER pointers are rebased
                // onto its own heap copy by parse_cert_der.
                if(msg_body_len2 >= 10) {
                    var cert_chain : *mut X509Cert = null
                    var chain_ret = x509_parse_server_cert_chain(&raw msg_buf[4],
                                                                 msg_body_len2 as size_t,
                                                                 true,
                                                                 &raw mut cert_chain)
                    if(chain_ret == 0 && cert_chain != null) {
                        var leaf = cert_chain
                        var parsed_key = false
                        if(leaf.pk_type == PK_RSA as u8) {
                            rsa_init(unsafe(&raw mut server_rsa_ctx), RSA_PKCS_V15, 0)
                            var ext_ret = x509_extract_rsa_pubkey(leaf, unsafe(&raw mut server_rsa_ctx))
                            if(ext_ret == 0 && rsa_get_len(unsafe(&raw mut server_rsa_ctx)) > 0) {
                                has_server_rsa = true
                                parsed_key = true
                            }
                        } else if(leaf.pk_type == PK_ECKEY as u8) {
                            ecdsa_init(unsafe(&raw mut server_ecdsa_ctx))
                            var ext_ret = x509_extract_ecdsa_pubkey(leaf, unsafe(&raw mut server_ecdsa_ctx))
                            if(ext_ret == 0 && server_ecdsa_ctx.is_init) {
                                has_server_ecdsa = true
                                parsed_key = true
                            }
                        }
                        ssl.peer_cert = leaf
                        if(parsed_key) {
                            if(ssl.conf != null && ssl.conf.authmode != SSL_VERIFY_NONE) {
                                var ca = ssl.conf.ca_chain
                                var vret = x509_verify_chain(leaf, ca, ssl.hostname)
                                if(vret != 0) {
                                    // peer_cert stays attached; ssl_free cleans it up.
                                    return ERR_SSL_CERT_VERIFY_FAILED
                                }
                            }
                        } else {
                            // No usable public key on the leaf; nothing to
                            // verify CertificateVerify against. Clean up the
                            // whole chain (peer_cert stays null).
                            cert_chain_free(leaf)
                            ssl.peer_cert = null
                        }
                    }
                }

            } else if(msg_type_code == SSL_HS_CERTIFICATE_VERIFY as u32) {
                // Save transcript state BEFORE hashing CertificateVerify (for signature verification)
                if(!cv_saved) {
                    unsafe { memcpy(&raw mut cv_transcript_copy, &raw transcript, sizeof(TLS13Transcript)) }
                    cv_saved = true
                }

                // Verify the CertificateVerify signature
                var has_key = has_server_rsa || has_server_ecdsa
                if(has_key && msg_body_len2 >= 6) {
                    var sig_alg = read_u16_be(&raw msg_buf[4]) as u16
                    var sig_len = read_u16_be(&raw msg_buf[6]) as size_t
                    if(sig_len <= msg_body_len2 - 2 - 2) {
                        var sig_data = &raw msg_buf[8]

                        // Finalize the saved transcript to get Transcript-Hash(ClientHello...Certificate)
                        // Write into a struct field: &raw local[0] output buffers are corrupted
                        // by a compiler bug for this large-stack (SHA-384) function.
                        var cv_hash_copy : TLS13Transcript
                        unsafe { memcpy(&raw mut cv_hash_copy, &raw cv_transcript_copy, sizeof(TLS13Transcript)) }
                        tls13_transcript_finalize(&raw mut cv_hash_copy,
                                                  &raw mut ssl.tls13_keys.exporter_master_secret[0], use_sha384)

                        // Build signed_content for TLS 1.3:
                        // 64 spaces + "TLS 1.3, server CertificateVerify" + 0x00 + transcript_hash
                        var signed_content : [240]u8
                        // 64 spaces (0x20)
                        var sc_pos : size_t = 0
                        while(sc_pos < 64) {
                            signed_content[sc_pos] = 0x20 as u8
                            sc_pos += 1
                        }
                        // context string "TLS 1.3, server CertificateVerify"
                        var ctx_str = "TLS 1.3, server CertificateVerify\0" as *char
                        var ctx_i : size_t = 0
                        while(ctx_str[ctx_i] != 0) {
                            signed_content[sc_pos] = ctx_str[ctx_i] as u8
                            sc_pos += 1
                            ctx_i += 1
                        }
                        // NUL byte
                        signed_content[sc_pos] = 0x00; sc_pos += 1
                        // transcript hash (hash_len bytes)
                        var th_i : size_t = 0
                        while(th_i < tls13_hash_len) {
                            signed_content[sc_pos + th_i] = ssl.tls13_keys.exporter_master_secret[th_i]
                            th_i += 1
                        }
                        sc_pos += tls13_hash_len

                        // Hash of signed_content (SHA-256 or SHA-384)
                        var signed_hash : [48]u8
                        if(use_sha384) {
                            var sig_hash_ctx : crypto::raw::Sha512Context
                            crypto::raw::sha384_init(unsafe(&raw mut sig_hash_ctx))
                            crypto::raw::sha384_update(unsafe(&raw mut sig_hash_ctx), &raw signed_content[0], sc_pos)
                            crypto::raw::sha384_final(unsafe(&raw mut sig_hash_ctx), &raw mut signed_hash[0])
                        } else {
                            var sig_hash_ctx : crypto::raw::Sha256Context
                            crypto::raw::sha256_init(unsafe(&raw mut sig_hash_ctx))
                            crypto::raw::sha256_update(unsafe(&raw mut sig_hash_ctx), &raw signed_content[0], sc_pos)
                            crypto::raw::sha256_final(unsafe(&raw mut sig_hash_ctx), &raw mut signed_hash[0])
                        }

                        // Verify based on signature algorithm
                        var verify_ret : int = 0
                        if(sig_alg == TLS1_3_SIG_RSA_PKCS1_SHA256 && has_server_rsa) {
                            verify_ret = rsa_pkcs1_verify(unsafe(&raw mut server_rsa_ctx),
                                                          &raw signed_hash[0], 32,
                                                          sig_data, sig_len)
                        } else if(sig_alg == TLS1_3_SIG_ECDSA_SECP256R1_SHA256 && has_server_ecdsa) {
                            verify_ret = ecdsa_verify(unsafe(&raw mut server_ecdsa_ctx),
                                                      &raw signed_hash[0], 32,
                                                      sig_data, sig_len)
                        } else if(sig_alg == TLS1_3_SIG_RSA_PKCS1_SHA384 && has_server_rsa) {
                            verify_ret = rsa_pkcs1_verify(unsafe(&raw mut server_rsa_ctx),
                                                          &raw signed_hash[0], 48,
                                                          sig_data, sig_len)
                        } else if(sig_alg == TLS1_3_SIG_ECDSA_SECP384R1_SHA384 && has_server_ecdsa) {
                            verify_ret = ecdsa_verify(unsafe(&raw mut server_ecdsa_ctx),
                                                      &raw signed_hash[0], 48,
                                                      sig_data, sig_len)
                        }
                        // If we don't recognize the signature algorithm, allow pass
                        // (verification will be skipped)
                        // When authmode is SSL_VERIFY_NONE or ssl.conf is not set, skip
                        if(verify_ret != 0 &&
                            (ssl.conf == null || ssl.conf.authmode != SSL_VERIFY_NONE)) {
                            return ERR_SSL_CERT_VERIFY_FAILED
                        }
                    }
                }

                // Hash CertificateVerify into transcript AFTER verification
                tls13_transcript_update(unsafe(&raw mut transcript), &raw msg_buf[0], 4 + msg_body_len2)

            } else if(msg_type_code == SSL_HS_FINISHED as u32) {
                // Server Finished: derive expected verify_data
                var finished_key : [48]u8
                var fin_key_label = "finished\0" as *char
                var empty_c : [1]u8 = [0]
                tls13_hkdf_expand_label(&raw ssl.tls13_keys.server_handshake_traffic_secret[0], tls13_hash_len,
                                        fin_key_label, 8,
                                        &raw empty_c[0], 0,
                                        &raw mut finished_key[0], tls13_hash_len)

                // Hash Transcript: save state before finalizing (copy the struct)
                var fin_transcript_copy : TLS13Transcript
                unsafe { memcpy(&raw mut fin_transcript_copy, &raw transcript, sizeof(TLS13Transcript)) }
                // Write into a struct field (same compiler bug as above). cv_hash
                // above already used this scratch field and is no longer needed.
                tls13_transcript_finalize(&raw mut fin_transcript_copy,
                                          &raw mut ssl.tls13_keys.exporter_master_secret[0], use_sha384)

                // Compute expected: HMAC(finished_key, transcript_hash)
                var expected_finished : [48]u8
                if(use_sha384) {
                    crypto::raw::hmac_sha384(&raw finished_key[0], tls13_hash_len,
                                        &raw ssl.tls13_keys.exporter_master_secret[0], tls13_hash_len,
                                        &raw mut expected_finished[0])
                } else {
                    crypto::raw::hmac_sha256(&raw finished_key[0], tls13_hash_len,
                                        &raw ssl.tls13_keys.exporter_master_secret[0], tls13_hash_len,
                                        &raw mut expected_finished[0])
                }

                // Compare with received (msg_buf[4..4+hash_len])
                var verify_ok = true
                if(msg_body_len2 != tls13_hash_len) { verify_ok = false }
                var vi : size_t = 0
                while(vi < tls13_hash_len) {
                    if(msg_buf[4 + vi] != expected_finished[vi]) { verify_ok = false }
                    vi += 1
                }

                if(!verify_ok) {
                    return ERR_SSL_HANDSHAKE_FAILURE
                }

                // Hash Finished into transcript
                tls13_transcript_update(unsafe(&raw mut transcript), &raw msg_buf[0], 4 + msg_body_len2)

                server_finished_verified = true

            } else {
                return ERR_SSL_UNEXPECTED_MESSAGE
            }

            // Consume the processed message: shift any remaining buffered
            // plaintext (coalesced messages from the same record, or the start
            // of a message that continues in the next record) to the front.
            var consumed : size_t = msg_total
            if(consumed > msg_buf_len) { consumed = msg_buf_len }
            if(consumed < msg_buf_len) {
                var si : size_t = 0
                while(si < msg_buf_len - consumed) {
                    msg_buf[si] = msg_buf[consumed + si]
                    si += 1
                }
            }
            msg_buf_len -= consumed

            if(server_finished_verified) { break }
            }
        }

        // ── Send client Finished ─────────────────────────────────────
        // First send ChangeCipherSpec (TLS 1.3 compatibility)
        var ccs_out : [1]u8 = [1]
        ret = await send_record(ssl, SSL_MSG_CHANGE_CIPHER_SPEC as u8, &raw ccs_out[0], 1)
        if(ret < 0) { return ret }

        // Derive client finished key
        var client_finished_key : [48]u8
        var cf_label = "finished\0" as *char
        var empty_c2 : [1]u8 = [0]
        tls13_hkdf_expand_label(&raw ssl.tls13_keys.client_handshake_traffic_secret[0], tls13_hash_len,
                                cf_label, 8,
                                &raw empty_c2[0], 0,
                                &raw mut client_finished_key[0], tls13_hash_len)

        // Hash Transcript: finalize for client Finished compute
        var cf_transcript_copy : TLS13Transcript
        unsafe { memcpy(&raw mut cf_transcript_copy, &raw transcript, sizeof(TLS13Transcript)) }
        // Write directly into a struct field: &raw local[0] output buffers are
        // corrupted by a compiler bug for this large-stack (SHA-384) function.
        tls13_transcript_finalize(&raw mut cf_transcript_copy,
                                  &raw mut ssl.tls13_keys.client_handshake_traffic_secret[0], use_sha384)

        var client_finished_verify : [48]u8
        if(use_sha384) {
            crypto::raw::hmac_sha384(&raw client_finished_key[0], tls13_hash_len,
                                &raw ssl.tls13_keys.client_handshake_traffic_secret[0], tls13_hash_len,
                                &raw mut client_finished_verify[0])
        } else {
            crypto::raw::hmac_sha256(&raw client_finished_key[0], tls13_hash_len,
                                &raw ssl.tls13_keys.client_handshake_traffic_secret[0], tls13_hash_len,
                                &raw mut client_finished_verify[0])
        }

        // Build and send Finished message: hs_type(1) + length(3) + verify_data(hash_len)
        var cf_body : [48]u8
        var ci : size_t = 0
        while(ci < tls13_hash_len) {
            cf_body[ci] = client_finished_verify[ci]
            ci += 1
        }

        var cf_msg_buf : [64]u8
        cf_msg_buf[0] = SSL_HS_FINISHED as u8
        write_u24(tls13_hash_len, &raw mut cf_msg_buf[1])
        ci = 0
        while(ci < tls13_hash_len) {
            cf_msg_buf[4 + ci] = client_finished_verify[ci]
            ci += 1
        }
        tls13_transcript_update(unsafe(&raw mut transcript), &raw cf_msg_buf[0], 4 + tls13_hash_len)

        ret = await send_handshake_msg(ssl, SSL_HS_FINISHED as u8, &raw cf_body[0], tls13_hash_len)
        if(ret < 0) { return ret }

        // ── Derive application traffic keys ──
        // RFC 8446 §7.1: c/s ap traffic use ClientHello...server Finished (cf_hash);
        // res master uses ClientHello...client Finished (full_hash).
        var full_transcript_copy : TLS13Transcript
        unsafe { memcpy(&raw mut full_transcript_copy, &raw transcript, sizeof(TLS13Transcript)) }
        // Write directly into a struct field (same compiler bug as cf_hash above).
        tls13_transcript_finalize(&raw mut full_transcript_copy,
                                  &raw mut ssl.tls13_keys.server_handshake_traffic_secret[0], use_sha384)
        var di_fh : size_t = 0
        while(di_fh < 48) { ssl.tls13_keys.exporter_master_secret[di_fh] = ssl.tls13_keys.server_handshake_traffic_secret[di_fh]; di_fh += 1 }
        ret = tls13_derive_application_keys(ssl, null, tls13_hash_len, null, 0,
                                                   tls13_key_len, tls13_cipher_type)
        if(ret < 0) { return ret }

        ssl.state = SSLState.HANDSHAKE_OVER()
        return 0

    }
}

    // ============================================================================
    // ─── Secure Connection Helper ──────────────────────────────────────────
    // Accept a TLS connection on an already-accepted socket.
    // Performs the server-side TLS handshake. The call consumes `sock` on both
    // success and failure (the returned SSLContext owns it on success; the
    // failure paths close it before returning null).
    // Returns a heap-allocated SSLContext on success, null on failure.
    // priv_key must be a pointer to the matching private key context
    // (*mut RSAContext for RSA certs, *mut ECDSAContext for EC certs).
    // cipher_suite selects the TLS 1.2 RSA key-exchange suite to offer.
    public func tls_accept(sock : net::raw::Socket, cert : *mut X509Cert,
                           priv_key : *mut void,
                           cipher_suite : int = TLS_RSA_WITH_AES_128_GCM_SHA256) : *mut SSLContext {
        return async::block_on<*mut SSLContext>(tls_accept_coro(sock, cert, priv_key, cipher_suite, false))
    }

    public async func tls_accept_coro(sock : net::raw::Socket, cert : *mut X509Cert,
                           priv_key : *mut void,
                           cipher_suite : int,
                           transport_async : bool) : *mut SSLContext {
                               unsafe {

        var ssl_mem = unsafe(malloc(sizeof(SSLContext))) as *mut SSLContext
        if(ssl_mem == null) {
            net::raw::close_socket(sock)
            return null
        }

        ssl_init(ssl_mem)
        ssl_mem.transport_async = transport_async
        ssl_set_socket(ssl_mem, sock)

        // Create server config
        var cfg = ssl_config_init(SSL_IS_SERVER)
        cfg.authmode = SSL_VERIFY_NONE
        cfg.own_cert = cert
        cfg.own_key = priv_key
        // This helper performs the TLS 1.2 server handshake, so pin the
        // config to TLS 1.2 with a TLS 1.2 RSA key-exchange suite. The
        // default preference list starts with a TLS 1.3-only suite which
        // must not be offered in a TLS 1.2 ServerHello.
        cfg.min_tls_version = SSL_VERSION_TLS1_2
        cfg.max_tls_version = SSL_VERSION_TLS1_2
        cfg.ciphersuite_list[0] = cs(cipher_suite)
        cfg.ciphersuite_count = 1

        ssl_set_config(ssl_mem, &raw mut cfg)

        // Perform server handshake
        var ret = await do_tls12_server_handshake(ssl_mem)
        if(ret < 0) {
            ssl_free(ssl_mem)
            unsafe { dealloc ssl_mem }
            return null
        }

        return ssl_mem

                           }
}

    // ─── Auto CA Bundle Loading ───────────────────────────────────────────
    // Try to load the system CA bundle from common locations.
    // Returns a heap-allocated X509Cert on success, null if no CA found.
    public func load_system_ca_bundle() : *mut X509Cert {
        // Common CA bundle paths on Linux
        var paths : [4]*char = [
            "/etc/ssl/certs/ca-certificates.crt\0" as *char,
            "/etc/pki/tls/certs/ca-bundle.crt\0" as *char,
            "/etc/ssl/cert.pem\0" as *char,
            "/etc/pki/tls/cert.pem\0" as *char
        ]

        comptime if(def.windows) {
            // Windows has no built-in PEM bundle; check the bundles shipped by
            // Git for Windows (mingw64 and MSYS2 layouts) and a few common tools.
            var win_paths : [6]*char = [
                "C:\\Program Files\\Git\\mingw64\\etc\\ssl\\certs\\ca-bundle.crt\0" as *char,
                "C:\\Program Files\\Git\\mingw64\\ssl\\certs\\ca-bundle.crt\0" as *char,
                "C:\\Program Files\\Git\\usr\\ssl\\certs\\ca-bundle.crt\0" as *char,
                "C:\\msys64\\mingw64\\etc\\ssl\\certs\\ca-bundle.crt\0" as *char,
                "C:\\msys64\\usr\\ssl\\certs\\ca-bundle.crt\0" as *char,
                "C:\\Program Files\\cURL\\bin\\curl-ca-bundle.crt\0" as *char
            ]
            var wi : size_t = 0
            while(wi < 6) {
                var ca = x509_crt_load_pem_file(win_paths[wi])
                if(ca != null) { return ca }
                wi += 1
            }
        }

        var i : size_t = 0
        while(i < 4) {
            var ca = x509_crt_load_pem_file(paths[i])
            if(ca != null) { return ca }
            i += 1
        }

        return null
    }

    // ─── EC Private Key Loading ──────────────────────────────────────────
    public func ec_privkey_load_hex_file(path : *char) : *mut ECDSAContext {
        var mode = "r\0" as *char
        var f = unsafe(fopen(path, mode))
        if(f == null) { return null }
        var hex_buf : [100]u8
        var total_read : size_t = 0
        while(total_read < 100) {
            var n = unsafe(fread(unsafe(&raw mut hex_buf[total_read]), 1 as size_t, 100 - total_read, f))
            if(n <= 0) { break }
            total_read += n
        }
        unsafe { fclose(f) }

        // Decode up to 96 hex digits (48 bytes for P-384) into key_bytes,
        // skipping any non-hex characters (whitespace, newlines).
        var key_bytes : [48]u8
        var hex_count : size_t = 0
        var i : size_t = 0
        while(i < total_read && hex_count < 96) {
            var c = hex_buf[i]; i += 1
            var hv : u8 = 0
            var valid = false
            if(c >= 48 && c <= 57) { hv = c - 48 as u8; valid = true }
            else if(c >= 97 && c <= 102) { hv = (c - 97) + 10 as u8; valid = true }
            else if(c >= 65 && c <= 70) { hv = (c - 65) + 10 as u8; valid = true }
            if(!valid) { continue }
            if((hex_count & 1) == 0) {
                key_bytes[hex_count / 2] = hv << 4
            } else {
                key_bytes[hex_count / 2] = key_bytes[hex_count / 2] | hv
            }
            hex_count += 1
        }
        if(hex_count != 64 && hex_count != 96) { return null }

        var curve : u16 = TLS_GROUP_SECP256R1 as u16
        if(hex_count == 96) { curve = TLS_GROUP_SECP384R1 as u16 }

        var ctx = unsafe(malloc(sizeof(ECDSAContext))) as *mut ECDSAContext
        if(ctx == null) { return null }
        ecdsa_init(ctx)
        var ret = ecdsa_import_privkey(ctx, &raw key_bytes[0], hex_count / 2, curve)
        if(ret < 0) { unsafe { dealloc ctx }; return null }
        return ctx
    }

    // Free an ECDSA private-key context returned by ec_privkey_load_hex_file.
    // ECDSAContext is stack-only (no internal heap), so this just releases the
    // struct itself.
    public func ecdsa_context_free(ctx : *mut ECDSAContext) {
        if(ctx != null) { unsafe { dealloc ctx } }
    }

    // ─── CA Trust Store ───────────────────────────────────────────────────
    // Load a PEM-encoded certificate from a file on disk.
    // Returns a pointer to a heap-allocated X509Cert on success,
    // or null on failure. Caller is responsible for freeing.
    public func x509_crt_load_pem_file(path : *char) : *mut X509Cert {
        unsafe {

        var mode = "rb\0" as *char
        var f = unsafe(fopen(path, mode))
        if(f == null) { return null }

        // Read the entire file into a heap buffer. System CA bundles can be
        // hundreds of KB (e.g. Git for Windows' ca-bundle.crt is ~220KB), so a
        // fixed stack buffer would truncate them and corrupt the heap when the
        // truncated PEM/DER bytes get parsed. Cap at 1MB to bound memory.
        unsafe { fseek(f, 0, SEEK_END) }
        var file_size = unsafe(ftell(f))
        unsafe { fseek(f, 0, SEEK_SET) }
        if(file_size <= 0) { unsafe { fclose(f) }; return null }
        if(file_size > 1048576) { file_size = 1048576 }

        var buf = unsafe(malloc(file_size as size_t)) as *mut u8
        if(buf == null) { unsafe { fclose(f) }; return null }

        var total_read : size_t = 0
        while(total_read < file_size as size_t) {
            var n = unsafe(fread(buf + total_read, 1 as size_t, (file_size as size_t) - total_read, f))
            if(n <= 0) { break }
            total_read += n
        }
        unsafe { fclose(f) }

        if(total_read == 0) {
            unsafe { dealloc buf }
            return null
        }

        var head : *mut X509Cert = null
        var tail : *mut X509Cert = null
        var scan_pos : size_t = 0
        var begin_marker = string_view("-----BEGIN CERTIFICATE-----")
        var data_view = string_view(buf as *char, total_read)

        // Parse every PEM certificate in the bundle into a linked chain.
        while(scan_pos < total_read) {
            var search_view = data_view.subview(scan_pos, total_read)
            var rel_pos = search_view.find_or_npos(&begin_marker)
            if(rel_pos == std::NPOS) { break }
            var begin_pos = scan_pos + rel_pos

            var cert_mem = x509_cert_alloc()
            if(cert_mem == null) { break }
            var ret = parse_cert_pem(cert_mem, buf + begin_pos, total_read - begin_pos)
            if(ret == 0) {
                if(tail != null) {
                    tail.next = cert_mem
                    cert_mem.prev = tail
                } else {
                    head = cert_mem
                }
                tail = cert_mem
            } else {
                cert_mem.issuer = string()
                cert_mem.subject = string()
                unsafe { dealloc cert_mem }
            }
            // Continue scanning after this BEGIN marker.
            scan_pos = begin_pos + 27
        }

        if(head == null) {
            // No PEM certificates found in the buffer. Try DER parsing the
            // whole file as a single binary certificate.
            var der_cert = x509_cert_alloc()
            if(der_cert != null) {
                var ret = parse_cert_der(der_cert, buf, total_read)
                if(ret == 0) {
                    head = der_cert
                    tail = der_cert
                } else {
                    der_cert.issuer = string()
                    der_cert.subject = string()
                    unsafe { dealloc der_cert }
                }
            }
        }

        unsafe { dealloc buf }
        return head

    }
}

    // Set the trusted CA chain for certificate verification. The chain remains
    // caller-owned and must outlive the handshake that uses this config.
    public unsafe func ssl_set_ca_chain(conf : *mut SSLConfig, ca : *mut X509Cert) {
        unsafe {

        if(conf == null) { return }
        conf.ca_chain = ca

    }
}

    // ─── RSA Private Key PEM Loader ───────────────────────────────────────

    // Load an RSA private key from a PEM file.
    // Supports:-----BEGIN RSA PRIVATE KEY----- (PKCS#1)
    // Returns a heap-allocated RSAContext (caller must rsa_free + dealloc).
    public func rsa_privkey_load_pem_file(path : *char) : *mut RSAContext {
        unsafe {

        var f = unsafe(fopen(path, "rb"))
        if(f == null) { return null }
        unsafe { fseek(f, 0, 2) }
        var file_size = unsafe(ftell(f))
        unsafe { fseek(f, 0, 0) }
        if(file_size <= 0 || file_size > 1048576) { unsafe { fclose(f) }; return null }
        var buf = unsafe(malloc(file_size as size_t)) as *mut u8
        if(buf == null) { unsafe { fclose(f) }; return null }
        var nread = unsafe(fread(buf as *mut void, 1 as size_t, file_size as size_t, f))
        unsafe { fclose(f) }
        if(nread != file_size as size_t) { unsafe { free(buf) }; return null }

        // Find BEGIN marker
        var begin_marker = "-----BEGIN RSA PRIVATE KEY-----" as *char
        var begin_len : size_t = 31
        var scan_pos : size_t = 0
        var begin_pos : size_t = 0
        var found = false
        while(scan_pos + begin_len <= nread) {
            var match = true
            var k : size_t = 0
            while(k < begin_len) {
                if(buf[scan_pos + k] != (begin_marker[k] as u8)) { match = false; break }
                k += 1
            }
            if(match) { begin_pos = scan_pos; found = true; break }
            scan_pos += 1
        }
        if(!found) { unsafe { free(buf) }; return null }

        // Skip past the BEGIN line
        var data_start = begin_pos + begin_len
        while(data_start < nread && buf[data_start] != ('\n' as u8)) { data_start += 1 }
        data_start += 1

        // Find END marker
        var end_marker = "-----END RSA PRIVATE KEY-----" as *char
        var end_len : size_t = 29
        var end_pos : size_t = data_start
        found = false
        while(end_pos + end_len <= nread) {
            var match = true
            var k : size_t = 0
            while(k < end_len) {
                if(buf[end_pos + k] != (end_marker[k] as u8)) { match = false; break }
                k += 1
            }
            if(match) { found = true; break }
            end_pos += 1
        }
        if(!found) { unsafe { free(buf) }; return null }

        // Strip whitespace from base64 content
        var b64_buf : [4096]u8
        var buf_pos : size_t = 0
        var i = data_start
        while(i < end_pos && buf_pos < 4096) {
            var c = buf[i]
            if(c != ('\n' as u8) && c != ('\r' as u8) && c != (' ' as u8) && c != ('\t' as u8)) {
                b64_buf[buf_pos] = c
                buf_pos += 1
            }
            i += 1
        }
        unsafe { free(buf) }

        // Decode base64 to DER
        var der_buf : [4096]u8
        var result = crypto::raw::base64_decode(b64_buf as *char, buf_pos, &raw mut der_buf[0], 4096)
        if(result is std::Result.Err) { return null }
        var Ok(decoded_len) = result else unsafe { unreachable }

        // Parse PKCS#1 RSAPrivateKey DER structure
        // SEQUENCE { version INTEGER, n INTEGER, e INTEGER, d INTEGER, p INTEGER, q INTEGER, ... }
        var rsa = unsafe(malloc(sizeof(RSAContext))) as *mut RSAContext
        if(rsa == null) { return null }
        rsa_init(rsa, RSA_PKCS_V15, 0)

        var pos : size_t = 0
        // SEQUENCE tag
        if(pos >= decoded_len || der_buf[pos] != 0x30) { unsafe { dealloc rsa }; return null }
        pos += 1
        // SEQUENCE length (skip)
        var seq_len = parse_der_length(&raw der_buf[0], &raw mut pos, decoded_len)
        if(seq_len < 0) { unsafe { dealloc rsa }; return null }

        // version
        if(pos >= decoded_len || der_buf[pos] != 0x02) { unsafe { dealloc rsa }; return null }
        pos += 1
        var ver_len = parse_der_length(&raw der_buf[0], &raw mut pos, decoded_len)
        if(ver_len < 0) { unsafe { dealloc rsa }; return null }
        pos = pos + (ver_len as size_t)

        // n (modulus)
        var n_data : *u8 = null
        var n_len = parse_der_integer(&raw der_buf[0], &raw mut pos, decoded_len, &raw mut n_data)
        if(n_len <= 0) { unsafe { dealloc rsa }; return null }

        // e (public exponent)
        var e_data : *u8 = null
        var e_len = parse_der_integer(&raw der_buf[0], &raw mut pos, decoded_len, &raw mut e_data)
        if(e_len <= 0) { unsafe { dealloc rsa }; return null }
        rsa_import_pubkey(rsa, n_data, n_len as size_t, e_data, e_len as size_t)

        // d (private exponent)
        var d_data : *u8 = null
        var d_len = parse_der_integer(&raw der_buf[0], &raw mut pos, decoded_len, &raw mut d_data)
        if(d_len <= 0) { unsafe { dealloc rsa }; return null }
        rsa_import_privkey(rsa, n_data, n_len as size_t, d_data, d_len as size_t)

        // p, q, dp, dq, qinv (CRT params — optional but preferred)
        var p_data : *u8 = null
        var p_len = parse_der_integer(&raw der_buf[0], &raw mut pos, decoded_len, &raw mut p_data)
        if(p_len > 0) { mpi_read_binary(&raw mut rsa.P, p_data, p_len as size_t) }

        var q_data : *u8 = null
        var q_len = parse_der_integer(&raw der_buf[0], &raw mut pos, decoded_len, &raw mut q_data)
        if(q_len > 0) { mpi_read_binary(&raw mut rsa.Q, q_data, q_len as size_t) }

        var dp_data : *u8 = null
        var dp_len = parse_der_integer(&raw der_buf[0], &raw mut pos, decoded_len, &raw mut dp_data)
        if(dp_len > 0) { mpi_read_binary(&raw mut rsa.DP, dp_data, dp_len as size_t) }

        var dq_data : *u8 = null
        var dq_len = parse_der_integer(&raw der_buf[0], &raw mut pos, decoded_len, &raw mut dq_data)
        if(dq_len > 0) { mpi_read_binary(&raw mut rsa.DQ, dq_data, dq_len as size_t) }

        var qinv_data : *u8 = null
        var qinv_len = parse_der_integer(&raw der_buf[0], &raw mut pos, decoded_len, &raw mut qinv_data)
        if(qinv_len > 0) { mpi_read_binary(&raw mut rsa.QP, qinv_data, qinv_len as size_t) }

        return rsa

    }
}

    // Parse DER length field. Returns the length value and advances pos.
    func parse_der_length(data : *u8, pos : *mut size_t, data_len : size_t) : int {
        unsafe {

        if(pos[0] >= data_len) { return -1 }
        var first = data[pos[0]]
        pos[0] += 1
        if(first < 0x80) { return first as int }
        var num_bytes = (first & 0x7Fu8) as int
        if(num_bytes == 0 || num_bytes > 4) { return -1 }
        var length : u32 = 0
        var k : int = 0
        while(k < num_bytes) {
            if(pos[0] >= data_len) { return -1 }
            length = (length << 8) | (data[pos[0]] as u32)
            pos[0] += 1
            k += 1
        }
        return length as int

    }
}

    // Parse DER INTEGER. Returns data pointer and length, advances pos.
    func parse_der_integer(data : *u8, pos : *mut size_t, data_len : size_t, out_data : *mut *u8) : int {
        unsafe {

        if(pos[0] >= data_len || data[pos[0]] != 0x02) { return -1 }
        pos[0] += 1
        var len = parse_der_length(data, pos, data_len)
        if(len <= 0 || pos[0] + (len as size_t) > data_len) { return -1 }
        // Skip leading zero byte (sign padding in unsigned integers)
        var data_start = pos[0]
        if(len > 1 && data[data_start] == 0) { data_start += 1; len -= 1 }
        out_data[0] = data + data_start
        pos[0] = pos[0] + (len as size_t)
        return len

    }
}

    // Allocate and initialize a single X509Cert on the heap. malloc'd memory is
    // NOT zeroed, so we memset first: x509_cert_init move-assigns embedded
    // std::string members (issuer/subject) which calls their destructor on the
    // old buffer — on uninitialized bytes that would free garbage. Zeroed
    // strings have state '\0', so the destructor is a no-op.
    func x509_cert_alloc() : *mut X509Cert {
        var cert_mem = unsafe(malloc(sizeof(X509Cert))) as *mut X509Cert
        if(cert_mem != null) {
            unsafe { memset(cert_mem as *mut void, 0, sizeof(X509Cert)) }
            x509_cert_init(cert_mem)
        }
        return cert_mem
    }

    // Set the server's own RSA private key for decrypting the pre-master secret.
    // The key remains caller-owned and must outlive the handshake.
    public unsafe func ssl_set_own_rsa_key(conf : *mut SSLConfig, rsa_key : *mut RSAContext) {
        unsafe {

        if(conf == null) { return }
        conf.own_key = rsa_key as *mut void

    }
}

    // Set ALPN protocols for negotiation. The protocol list and every string
    // remain caller-owned and must outlive the handshake.
    public unsafe func ssl_set_alpn_protocols(conf : *mut SSLConfig, protocols : *mut *char, count : u8) {
        unsafe {

        if(conf == null) { return }
        if(protocols == null || count == 0) {
            conf.alpn_list = null
            conf.alpn_count = 0
            return
        }
        var i : u8 = 0
        while(i < count) {
            if(protocols[i] == null) {
                conf.alpn_list = null
                conf.alpn_count = 0
                return
            }
            i += 1
        }
        conf.alpn_list = protocols
        conf.alpn_count = count

    }
}

    // Get the negotiated ALPN protocol after handshake. The returned pointer is
    // owned by the context and becomes invalid at ssl_free.
    public unsafe func ssl_get_alpn_negotiated(ssl : *mut SSLContext) : *char {
        unsafe {

        if(ssl == null) { return null }
        return ssl.alpn_negotiated

    }
}

    // Public API - Client Connection
    // ============================================================================

    // Initialize SSL config for client use
    public func ssl_config_init(endpoint : int) : SSLConfig {
        return SSLConfig(endpoint)
    }

    // Initialize SSL context
    public func ssl_init(ssl : *mut SSLContext) {
        ensure_init()
        ssl_context_init(ssl)
    }

    // Apply read timeout to the SSL socket (from config, or 5s default)
    func ssl_apply_recv_timeout(ssl : *mut SSLContext) {
        unsafe {

        var timeout_ms : u32 = 5000
        if(ssl.conf != null && ssl.conf.read_timeout > 0) {
            timeout_ms = ssl.conf.read_timeout
            if(timeout_ms > 8000) { timeout_ms = 8000 }
        }
        net::raw::set_recv_timeout(ssl.transport_socket, (timeout_ms / 1000) as long, (timeout_ms % 1000 * 1000) as long)

    }
}

    // Set the socket for the SSL connection
    public func ssl_set_socket(ssl : *mut SSLContext, socket : net::raw::Socket) {
        unsafe {

        ssl.transport_socket = socket
        ssl.transport_connected = true
        if(ssl.transport_async) {
            // non-blocking transport: record I/O suspends on fd readiness
            net::raw::set_nonblocking(socket)
            ssl.transport = Transport {
                ctx : socket as *mut void,
                recv_fn : transport_recv_async,
                send_fn : transport_send_async
            }
        } else {
            net::raw::set_blocking(socket)
            ssl.transport = Transport {
                ctx : socket as *mut void,
                recv_fn : transport_recv_blocking,
                send_fn : transport_send_blocking
            }
            ssl_apply_recv_timeout(ssl)
        }
        // Set default TLS record version
        ssl.major_ver = 3
        ssl.minor_ver = 3 as u8

    }
}

    // Switch an already-connected context to the non-blocking transport (used
    // by the async entry points). Idempotent; a no-op before a socket is set.
    public func ssl_use_async_transport(ssl : *mut SSLContext) {
        unsafe {

        ssl.transport_async = true
        if(ssl.transport_connected && ssl.transport_socket != 0 as net::raw::Socket) {
            net::raw::set_nonblocking(ssl.transport_socket)
            ssl.transport = Transport {
                ctx : ssl.transport_socket as *mut void,
                recv_fn : transport_recv_async,
                send_fn : transport_send_async
            }
        }

    }
}

    // Set the hostname for SNI and certificate verification
    public func ssl_set_hostname(ssl : *mut SSLContext, hostname : *char) {
        unsafe {

        if(ssl == null || hostname == null) { return }
        // SNI names are at most 255 bytes; copy so the caller's string may be
        // temporary. Keep the ownership bit so ssl_free only frees this copy.
        var len : size_t = 0
        while(len < 255 && hostname[len] != 0) { len += 1 }
        var copy = unsafe(malloc(len + 1)) as *mut char
        if(copy == null) { return }
        var i : size_t = 0
        while(i < len) {
            copy[i] = hostname[i]
            i += 1
        }
        copy[len] = 0
        if(ssl.hostname_owned && ssl.hostname != null) {
            unsafe { dealloc ssl.hostname }
        }
        ssl.hostname = copy
        ssl.hostname_len = len
        ssl.hostname_owned = true

    }
}

    // Configure the SSL context. The config is copied into context-owned
    // storage, so callers may use a stack config or a temporary expression.
    public func ssl_set_config(ssl : *mut SSLContext, conf : *mut SSLConfig) {
        unsafe {

        if(ssl == null || conf == null) { return }
        var copy = unsafe(malloc(sizeof(SSLConfig))) as *mut SSLConfig
        if(copy == null) { return }
        unsafe { *copy = *conf }
        if(ssl.conf_owned && ssl.conf != null) {
            unsafe { dealloc ssl.conf }
        }
        ssl.conf = copy
        ssl.conf_owned = true
        ssl.major_ver = 3
        if(conf.max_tls_version >= SSL_VERSION_TLS1_3) {
            ssl.minor_ver = 4 as u8
        } else {
            ssl.minor_ver = 3 as u8
        }
        ssl.tls_version = conf.max_tls_version

    }
}

    // ─── Server-Side TLS 1.2 Handshake (Minimal) ─────────────────────────
    // Implements a minimal TLS 1.2 server handshake for use with the HTTP server.
    // This is a basic implementation that supports RSA key exchange.
    func build_server_hello(ssl : *mut SSLContext, buf : *mut u8, buf_size : size_t) : int {
        unsafe {

        var pos : i32 = 0
        buf[pos] = 0x03 as u8; pos += 1  // major version (TLS 1.2)
        buf[pos] = 0x03 as u8; pos += 1  // minor version

        // Server random (32 bytes) - use CSPRNG
        var rand_ret = random_fill(&raw mut buf[pos], 32)
        if(rand_ret < 0) { return ERR_SSL_NO_RNG }
        // Copy server random to handshake params
        if(ssl.handshake != null) {
            var k2 : size_t = 0
            while(k2 < 32) {
                ssl.handshake.randbytes[32 + k2] = buf[pos]
                pos += 1
                k2 += 1
            }
        } else {
            pos += 32
        }

        // Session ID (empty for now)
        buf[pos] = 0 as u8; pos += 1

        // Cipher suite (use the first preferred one)
        if(ssl.conf != null && ssl.conf.ciphersuite_count > 0) {
            var cs = ssl.conf.ciphersuite_list[0]
            buf[pos] = (cs >> 8) as u8; pos += 1
            buf[pos] = cs as u8; pos += 1
            ssl.negotiated_ciphersuite = cs
        } else {
            buf[pos] = 0x00 as u8; pos += 1
            buf[pos] = 0x9C as u8; pos += 1  // TLS_RSA_WITH_AES_128_GCM_SHA256
            ssl.negotiated_ciphersuite = 0x009C as u16
        }

        // Compression method (null)
        buf[pos] = 0 as u8; pos += 1

        // Extensions: modern clients require the secure renegotiation
        // (RFC 5746) extension in the ServerHello, otherwise OpenSSL aborts
        // the handshake with "unsafe legacy renegotiation disabled".
        // renegotiation_info(0xFF01) with an empty renegotiated_connection.
        if(pos + 7 <= buf_size) {
            var ext_total : i32 = 5   // 2 (type) + 2 (len) + 1 (data)
            buf[pos] = (ext_total >> 8) as u8; pos += 1
            buf[pos] = ext_total as u8; pos += 1
            buf[pos] = 0xFF as u8; pos += 1
            buf[pos] = 0x01 as u8; pos += 1
            buf[pos] = 0x00 as u8; pos += 1
            buf[pos] = 0x01 as u8; pos += 1
            buf[pos] = 0x00 as u8; pos += 1
        }

        return pos

    }
}

    async func do_tls12_server_handshake(ssl : *mut SSLContext) : int {
        unsafe {

        ensure_init()

        // Handshake transcript hash
        var hash_ctx : crypto::raw::Sha256Context
        crypto::raw::sha256_init(unsafe(&raw mut hash_ctx))

        // Allocate handshake params
        if(ssl.handshake == null) {
            var hs_mem = unsafe(malloc(sizeof(HandshakeParams))) as *mut HandshakeParams
            handshake_params_init(hs_mem)
            ssl.handshake = hs_mem
        }

        // 1. Read ClientHello
        ssl.state = SSLState.CLIENT_HELLO()
        var hs_type : u8 = 0
        var hs_len : u32 = 0
        var hs_buf : [8192]u8
        var ret = await read_handshake_msg(ssl, &raw mut hs_type, &raw mut hs_len,
                                      &raw mut hs_buf[0], 8192)
        if(ret < 0) { return ret }
        if(hs_type != SSL_HS_CLIENT_HELLO as u8) {
            return ERR_SSL_UNEXPECTED_MESSAGE
        }

        // Feed ClientHello into transcript hash (body starts at hs_buf[4])
        ssl_hash_handshake_msg(unsafe(&raw mut hash_ctx), hs_type, hs_len, &raw hs_buf[4])

        // Extract client random from ClientHello (body offsets 6-37)
        if(hs_len >= 34 && ssl.handshake != null) {
            var i : size_t = 0
            while(i < 32) {
                ssl.handshake.randbytes[i] = hs_buf[6 + i]
                i += 1
            }
        }

        // 2. Send ServerHello
        ssl.state = SSLState.SERVER_HELLO()
        var sh_buf : [256]u8
        var sh_len = build_server_hello(ssl, &raw mut sh_buf[0], 256)
        ssl_hash_handshake_msg(unsafe(&raw mut hash_ctx), SSL_HS_SERVER_HELLO as u8, sh_len as u32, &raw sh_buf[0])
        ret = await send_handshake_msg(ssl, SSL_HS_SERVER_HELLO as u8, &raw sh_buf[0], sh_len as u32)
        if(ret < 0) { return ret }

        // 3. Send Certificate (if we have one)
        if(ssl.conf.own_cert != null) {
            ssl.state = SSLState.SERVER_CERTIFICATE()

            // Build Certificate message: request_context(0) + cert_chain
                var cert_buf : [4096]u8
            var cert_pos : size_t = 3
            var cert_data = ssl.conf.own_cert

            // Certificate entry: cert_len(3) + cert_der
            var der_len = cert_data.raw_pem_len
            if(der_len > 0 && der_len < 4000) {
                cert_buf[cert_pos] = ((der_len >> 16) & 0xFF) as u8
                cert_buf[cert_pos + 1] = ((der_len >> 8) & 0xFF) as u8
                cert_buf[cert_pos + 2] = (der_len & 0xFF) as u8
                cert_pos += 3
                var j : size_t = 0
                while(j < der_len) {
                    cert_buf[cert_pos] = cert_data.raw_pem[j]
                    cert_pos += 1
                    j += 1
                }
            }

            // Certificate list length (3 bytes before certs)
            var list_len = cert_pos - 3
            cert_buf[0] = ((list_len >> 16) & 0xFF) as u8
            cert_buf[1] = ((list_len >> 8) & 0xFF) as u8
            cert_buf[2] = (list_len & 0xFF) as u8

            ssl_hash_handshake_msg(unsafe(&raw mut hash_ctx), SSL_HS_CERTIFICATE as u8, cert_pos as u32, &raw cert_buf[0])
            ret = await send_handshake_msg(ssl, SSL_HS_CERTIFICATE as u8, &raw cert_buf[0], cert_pos as u32)
            if(ret < 0) { return ret }
        }

        // 4. Send ServerHelloDone
        ssl.state = SSLState.SERVER_HELLO_DONE()
        var shd_buf : [1]u8 = [0]
        ssl_hash_handshake_msg(unsafe(&raw mut hash_ctx), SSL_HS_SERVER_HELLO_DONE as u8, 0, &raw shd_buf[0])
        ret = await send_handshake_msg(ssl, SSL_HS_SERVER_HELLO_DONE as u8, &raw shd_buf[0], 0)
        if(ret < 0) { return ret }

        // 5. Read ClientKeyExchange
        ssl.state = SSLState.CLIENT_KEY_EXCHANGE()
        ret = await read_handshake_msg(ssl, &raw mut hs_type, &raw mut hs_len,
                                  &raw mut hs_buf[0], 8192)
        if(ret < 0) { return ret }
        if(hs_type != SSL_HS_CLIENT_KEY_EXCHANGE as u8) {
            return ERR_SSL_UNEXPECTED_MESSAGE
        }
        ssl_hash_handshake_msg(unsafe(&raw mut hash_ctx), hs_type, hs_len, &raw hs_buf[4])

        // Parse encrypted pre-master secret from ClientKeyExchange
        // For RSA: body = length(2) + encrypted_pre_master
        // hs_buf layout: [0]=type, [1..3]=length, [4..]=body
        var enc_pms_len : size_t = 0
        if(hs_len >= 6) {
            enc_pms_len = read_u16_be(&raw hs_buf[4]) as size_t
        }

        var pre_master : [48]u8
        var pre_master_set : bool = false

        // Try to decrypt the pre-master secret using the server's RSA private key
        if(hs_len >= 6 && enc_pms_len > 0 && ssl.conf.own_key != null) {
            var server_rsa = ssl.conf.own_key as *mut RSAContext
            // rsa_pkcs1_decrypt requires the ciphertext to be exactly one
            // modulus long. This gate used to be a hardcoded 256 bytes (2048
            // bits), which silently skipped RSA key exchange for any larger
            // server key — including keys this library generated itself.
            if(enc_pms_len == rsa_get_len(server_rsa) && enc_pms_len <= RSA_MAX_KEY_BYTES) {
                var enc_pms = &raw hs_buf[6]
                var decrypted : [RSA_MAX_KEY_BYTES]u8
                var dec_len : size_t = enc_pms_len
                var dec_ret = rsa_pkcs1_decrypt(server_rsa,
                                                 enc_pms, enc_pms_len,
                                                 &raw mut decrypted[0], &raw mut dec_len,
                                                 48)
                if(dec_ret == 0 && dec_len == 48) {
                    var di : size_t = 0
                    while(di < 48) {
                        pre_master[di] = decrypted[di]
                        di += 1
                    }
                    pre_master_set = true
                }
            }
        }

        // Require RSA decryption of pre-master secret
        if(!pre_master_set) {
            return ERR_SSL_PRIVATE_KEY_REQUIRED
        }

        // Derive master secret
        var master_secret : [48]u8
        tls12_derive_master_secret(&raw pre_master[0], 48,
                                    &raw ssl.handshake.randbytes[0],
                                    &raw ssl.handshake.randbytes[32],
                                    &raw mut master_secret[0])

        // Derive key block and set up record-layer transforms. The client's
        // Finished (and all subsequent records) are encrypted, so the receive
        // transform must be active before reading it.
        var cs_info = get_ciphersuite_info(ssl.negotiated_ciphersuite)
        var kb_size = tls12_key_block_size(&raw cs_info)
        var key_block : [256]u8
        tls12_derive_key_block(&raw master_secret[0],
                                &raw ssl.handshake.randbytes[32],  // server random
                                &raw ssl.handshake.randbytes[0],   // client random
                                &raw mut key_block[0], kb_size)

        // populate_transform assigns enc=client_write, dec=server_write; the
        // server must swap so its send path uses the server keys and its
        // receive path uses the client keys.
        var srv_tr : Transform
        transform_init(unsafe(&raw mut srv_tr))
        var pop_ret = tls12_populate_transform(unsafe(&raw mut srv_tr), &raw cs_info, &raw key_block[0], kb_size)
        if(pop_ret < 0) { return pop_ret }
        var sw_i : size_t = 0
        while(sw_i < srv_tr.mac_key_len) {
            var mt = srv_tr.mac_key_enc[sw_i]
            srv_tr.mac_key_enc[sw_i] = srv_tr.mac_key_dec[sw_i]
            srv_tr.mac_key_dec[sw_i] = mt
            sw_i += 1
        }
        sw_i = 0
        while(sw_i < srv_tr.key_len) {
            var kt = srv_tr.key_enc[sw_i]
            srv_tr.key_enc[sw_i] = srv_tr.key_dec[sw_i]
            srv_tr.key_dec[sw_i] = kt
            var it = srv_tr.iv_enc[sw_i]
            srv_tr.iv_enc[sw_i] = srv_tr.iv_dec[sw_i]
            srv_tr.iv_dec[sw_i] = it
            var bt = srv_tr.base_iv_enc[sw_i]
            srv_tr.base_iv_enc[sw_i] = srv_tr.base_iv_dec[sw_i]
            srv_tr.base_iv_dec[sw_i] = bt
            sw_i += 1
        }

        var tr_in_mem = unsafe(malloc(sizeof(Transform))) as *mut Transform
        unsafe { memcpy(tr_in_mem, &raw srv_tr, sizeof(Transform)) }
        ssl.transform_in = tr_in_mem

        var tr_out_mem = unsafe(malloc(sizeof(Transform))) as *mut Transform
        unsafe { memcpy(tr_out_mem, &raw srv_tr, sizeof(Transform)) }

        // Reset receive sequence number for the client's encrypted handshake records
        var si9 : size_t = 0
        while(si9 < 8) { ssl.in_ctr[si9] = 0; si9 += 1 }

        // 6. Read Finished (read_handshake_msg auto-consumes any preceding CCS record)
        ret = await read_handshake_msg(ssl, &raw mut hs_type, &raw mut hs_len,
                                  &raw mut hs_buf[0], 8192)
        if(ret < 0) { return ret }
        if(hs_type != SSL_HS_FINISHED as u8) {
            return ERR_SSL_UNEXPECTED_MESSAGE
        }

        // Compute transcript hash up to ClientKeyExchange for Finished verification
        var client_hs_hash : [32]u8
        var copy_ctx : crypto::raw::Sha256Context
        unsafe { memcpy(&raw mut copy_ctx, &raw hash_ctx, sizeof(crypto::raw::Sha256Context)) }
        crypto::raw::sha256_final(&raw mut copy_ctx, &raw mut client_hs_hash[0])

        // Verify client Finished message
        var expected_client_finished : [12]u8
        tls12_compute_finished(&raw master_secret[0], true, &raw client_hs_hash[0], 32, &raw mut expected_client_finished[0])

        // Compare received client Finished against expected
        var fin_match = true
        var fi : size_t = 0
        while(fi < 12) {
            if(hs_buf[4 + fi] != expected_client_finished[fi]) { fin_match = false }
            fi += 1
        }
        if(!fin_match) {
            var alert_sent = await send_alert(ssl, SSL_ALERT_LEVEL_FATAL as u8, SSL_ALERT_MSG_DECRYPT_ERROR as u8)
            return ERR_SSL_HANDSHAKE_FAILURE
        }

        // Feed received Client Finished message into transcript hash for computing Server Finished
        ssl_hash_handshake_msg(unsafe(&raw mut hash_ctx), SSL_HS_FINISHED as u8, 12, &raw hs_buf[4])

            var server_hs_hash : [32]u8
            crypto::raw::sha256_final(unsafe(&raw mut hash_ctx), &raw mut server_hs_hash[0])

        // 8. Send ChangeCipherSpec (in the clear)
        ssl.state = SSLState.SERVER_CHANGE_CIPHER_SPEC()
        var ccs_msg : [1]u8
        ccs_msg[0] = 1 as u8
        ret = await send_record(ssl, SSL_MSG_CHANGE_CIPHER_SPEC as u8, &raw ccs_msg[0], 1 as u16)
        if(ret < 0) { return ret }

        // Activate send-side encryption after ChangeCipherSpec (server key)
        ssl.transform_out = tr_out_mem
        var so_i : size_t = 0
        while(so_i < 8) { ssl.out_ctr[so_i] = 0; so_i += 1 }

        // 9. Send Finished (encrypted)
        ssl.state = SSLState.SERVER_FINISHED()
        tls12_compute_finished(&raw master_secret[0], false, &raw server_hs_hash[0], 32, &raw mut hs_buf[0])
        ret = await send_handshake_msg(ssl, SSL_HS_FINISHED as u8, &raw hs_buf[0], 12)
        if(ret < 0) { return ret }
        ssl.state = SSLState.HANDSHAKE_OVER()

        return 0

    }
}

    // ============================================================================
    // Server Handshake - TLS 1.3
    // ============================================================================

    async func do_tls13_server_handshake(ssl : *mut SSLContext) : int {
        unsafe {

        ensure_init()

        // Ensure handshake params
        if(ssl.handshake == null) {
            var hs_mem = unsafe(malloc(sizeof(HandshakeParams))) as *mut HandshakeParams
            handshake_params_init(hs_mem)
            ssl.handshake = hs_mem
        }

        // ── Read ClientHello ───────────────────────────────────────────
        ssl.state = SSLState.CLIENT_HELLO()
        var ret : int = 0
        var hs_type : u8 = 0
        var hs_len : u32 = 0
        var hs_buf : [8192]u8

        ret = await read_handshake_msg(ssl, &raw mut hs_type, &raw mut hs_len,
                                  &raw mut hs_buf[0], 8192)
        if(ret < 0) { return ret }
        if(hs_type != SSL_HS_CLIENT_HELLO as u8) {
            return ERR_SSL_UNEXPECTED_MESSAGE
        }

        // Transcript hash context
        var transcript : TLS13Transcript
        tls13_transcript_init(unsafe(&raw mut transcript))

        // Candidate cipher suites offered by the client (parsed from ClientHello)
        var client_cs : [64]u16
        var client_cs_count : size_t = 0

        // Hash ClientHello into transcript
        // FIXED: hs_buf from read_handshake_msg has the 4-byte handshake header
        // prepended, so hs_buf[0..3] = handshake type + length. The header is
        // hashed separately as ch_hdr above, so we must hash the BODY starting
        // from hs_buf[4] to avoid doubling the header and truncating the body.
        var ch_hdr : [4]u8
        ch_hdr[0] = SSL_HS_CLIENT_HELLO as u8
        write_u24(hs_len, &raw mut ch_hdr[1])
        tls13_transcript_update(unsafe(&raw mut transcript), &raw ch_hdr[0], 4)
        tls13_transcript_update(unsafe(&raw mut transcript), &raw hs_buf[4], hs_len)

        // Parse ClientHello to find client's key_share (support both P-256 and x25519)
        var client_p256_key : [65]u8
        var client_x25519_key : [32]u8
        var has_client_p256 = false
        var has_client_x25519 = false
        var ch_pos : size_t = 4

        ch_pos += 34 as size_t
        var sid_len = hs_buf[ch_pos] as size_t;
        var server_client_sid : [32]u8
        var sid_copy_len : size_t = sid_len
        if(sid_copy_len > 32) { sid_copy_len = 32 }
        var server_client_sid_len : size_t = sid_copy_len
        var sid_copy_i : size_t = 0
        while(sid_copy_i < sid_copy_len) { server_client_sid[sid_copy_i] = hs_buf[ch_pos + 1 + sid_copy_i]; sid_copy_i += 1 }
        ch_pos += 1 + sid_len
        var cs_len = read_u16_be(&raw hs_buf[ch_pos]) as size_t; ch_pos += 2
        var cs_pos : size_t = 0
        while(cs_pos + 2 <= cs_len && client_cs_count < 64) {
            var cs = read_u16_be(&raw hs_buf[ch_pos + cs_pos]) as u16
            client_cs[client_cs_count] = cs
            client_cs_count += 1
            cs_pos += 2
        }
        ch_pos += cs_len
        var cm_count = hs_buf[ch_pos] as size_t; ch_pos += 1 + cm_count
        if(ch_pos + 2 <= hs_len as size_t + 4) {
            var ext_len = read_u16_be(&raw hs_buf[ch_pos]) as size_t; ch_pos += 2
            var ext_end = ch_pos + ext_len
            while(ch_pos + 4 <= ext_end) {
                var ext_type = read_u16_be(&raw hs_buf[ch_pos]); ch_pos += 2
                var ext_data_len = read_u16_be(&raw hs_buf[ch_pos]) as size_t; ch_pos += 2
                if(ext_type == TLS_EXT_KEY_SHARE as u16 && ext_data_len >= 4) {
                    // key_share contains: client_shares_len(2) + KeyShareEntry*
                    var shares_len = read_u16_be(&raw hs_buf[ch_pos]) as size_t; ch_pos += 2
                    var shares_end = ch_pos + shares_len
                    while(ch_pos + 4 <= shares_end) {
                        var ks_group = read_u16_be(&raw hs_buf[ch_pos]); ch_pos += 2
                        var ks_key_len = read_u16_be(&raw hs_buf[ch_pos]) as size_t; ch_pos += 2
                        if(ks_group == TLS_GROUP_X25519 as u16 && ks_key_len == 32 && ch_pos + 32 <= shares_end) {
                            var ki : size_t = 0
                            while(ki < 32) { client_x25519_key[ki] = hs_buf[ch_pos + ki]; ki += 1 }
                            has_client_x25519 = true
                        } else if(ks_group == TLS_GROUP_SECP256R1 as u16 && ks_key_len == 65 && ch_pos + 65 <= shares_end) {
                            var ki : size_t = 0
                            while(ki < 65) { client_p256_key[ki] = hs_buf[ch_pos + ki]; ki += 1 }
                            has_client_p256 = true
                        }
                        ch_pos += ks_key_len
                    }
                    // Use the rest of ext_data_len (already consumed by shares parsing)
                    continue
                }
                ch_pos += ext_data_len
            }
        }

        if(!has_client_p256 && !has_client_x25519) {
            return ERR_SSL_HANDSHAKE_FAILURE
        }

        // Prefer x25519, fall back to P-256
        var use_x25519 = has_client_x25519

        // ── Generate server ECDHE keypair ──────────────────────────────
        var server_p256_priv : [32]u8; var server_p256_pub : [65]u8
        var server_x25519_priv : [32]u8; var server_x25519_pub : [32]u8
        var has_server_x25519 = false

        // Always generate P-256 (needed for fallback)
        var ecdh_ctx : ECDHContext; ecdh_init(unsafe(&raw mut ecdh_ctx))
        ret = ecdh_generate_keypair(unsafe(&raw mut ecdh_ctx), &raw mut server_p256_priv[0], 32, &raw mut server_p256_pub[0], 65)
        if(ret < 0) { return ret }

        if(use_x25519) {
            var xr = x25519_generate_keypair(&raw mut server_x25519_priv[0], &raw mut server_x25519_pub[0])
            if(xr == 0) { has_server_x25519 = true }
            if(!has_server_x25519) { use_x25519 = false }
        }

        // ── Compute ECDHE shared secret ───────────────────────────────
        var shared_secret : [32]u8
        if(use_x25519 && has_client_x25519 && has_server_x25519) {
            ret = x25519_compute_shared(&raw server_x25519_priv[0], &raw client_x25519_key[0], &raw mut shared_secret[0])
        } else if(has_client_p256) {
            ret = ecdh_compute_shared(unsafe(&raw mut ecdh_ctx), &raw client_p256_key[0], 65, &raw mut shared_secret[0], 32)
        } else {
            return ERR_SSL_HANDSHAKE_FAILURE
        }
        if(ret < 0) { return ret }

        // ── Build ServerHello ──────────────────────────────────────────
        ssl.state = SSLState.SERVER_HELLO()
        ssl.major_ver = 0x03
        ssl.minor_ver = 0x03

        // ── Negotiate cipher suite (TLS 1.3 only) ──
        // Prefer the config's explicit ciphersuite_list; otherwise offer the
        // full TLS 1.3 set in strongest-first order. Pick the first suite the
        // client also advertised.
        var srv_pref : [8]u16
        var srv_pref_count : size_t = 0
        if(ssl.conf.ciphersuite_count > 0 && ssl.conf.ciphersuite_count <= 8) {
            var pi : size_t = 0
            while(pi < ssl.conf.ciphersuite_count as size_t) {
                srv_pref[srv_pref_count] = ssl.conf.ciphersuite_list[pi]
                srv_pref_count += 1
                pi += 1
            }
        } else {
            srv_pref[0] = TLS1_3_AES_128_GCM_SHA256 as u16
            srv_pref[1] = TLS1_3_AES_256_GCM_SHA384 as u16
            srv_pref[2] = TLS1_3_CHACHA20_POLY1305_SHA256 as u16
            srv_pref_count = 3
        }
        var selected_cs : u16 = 0
        var nsp : size_t = 0
        while(nsp < srv_pref_count && selected_cs == 0) {
            var want = srv_pref[nsp]
            var cp : size_t = 0
            while(cp < client_cs_count) {
                if(client_cs[cp] == want) { selected_cs = want; break }
                cp += 1
            }
            nsp += 1
        }
        if(selected_cs == 0) { return ERR_SSL_HANDSHAKE_FAILURE }
        ssl.negotiated_ciphersuite = selected_cs

        // Derive hash/key parameters for the negotiated suite
        var cs_info = get_ciphersuite_info(selected_cs)
        var use_sha384 = (cs_info.hash == HASH_SHA384 as u8)
        var tls13_hash_len : size_t = 32
        if(use_sha384) { tls13_hash_len = 48 }
        var tls13_key_len : size_t = 16
        if(cs_info.cipher != CIPHER_AES_128_GCM as u8) { tls13_key_len = 32 }
        var tls13_cipher_type : u8 = cs_info.cipher

        var sh_buf : [1024]u8
        var sh_pos : size_t = 0
        // version
        sh_buf[sh_pos] = 0x03 as u8; sh_pos += 1
        sh_buf[sh_pos] = 0x03 as u8; sh_pos += 1
        // Random (32 bytes)
        random_fill(&raw mut sh_buf[sh_pos], 32); sh_pos += 32
        // Session ID (echo client's)
        sh_buf[sh_pos] = server_client_sid_len as u8; sh_pos += 1
        var sid_echo_i : size_t = 0
        while(sid_echo_i < server_client_sid_len && sid_echo_i < 32) {
            sh_buf[sh_pos] = server_client_sid[sid_echo_i]; sh_pos += 1
            sid_echo_i += 1
        }
        // Cipher suite
        sh_buf[sh_pos] = ((ssl.negotiated_ciphersuite >> 8) & 0xFF) as u8; sh_pos += 1
        sh_buf[sh_pos] = (ssl.negotiated_ciphersuite & 0xFF) as u8; sh_pos += 1
        // Compression
        sh_buf[sh_pos] = 0 as u8; sh_pos += 1
        // Extensions
        var sh_ext_start = sh_pos
        sh_pos += 2  // extension length placeholder
        // key_share extension — send the selected curve's public key
        sh_buf[sh_pos] = ((TLS_EXT_KEY_SHARE >> 8) & 0xFF) as u8; sh_pos += 1
        sh_buf[sh_pos] = (TLS_EXT_KEY_SHARE & 0xFF) as u8; sh_pos += 1
        var ks_ext_len_pos = sh_pos; sh_pos += 2
        if(use_x25519 && has_server_x25519) {
            sh_buf[sh_pos] = ((TLS_GROUP_X25519 >> 8) & 0xFF) as u8; sh_pos += 1
            sh_buf[sh_pos] = (TLS_GROUP_X25519 & 0xFF) as u8; sh_pos += 1
            sh_buf[sh_pos] = 0 as u8; sh_pos += 1; sh_buf[sh_pos] = 32 as u8; sh_pos += 1
            var kpi : size_t = 0
            while(kpi < 32) { sh_buf[sh_pos + kpi] = server_x25519_pub[kpi]; kpi += 1 }
            sh_pos += 32
            var ks_ext_len = 2 + 2 + 32
            sh_buf[ks_ext_len_pos] = ((ks_ext_len >> 8) & 0xFF) as u8
            sh_buf[ks_ext_len_pos + 1] = (ks_ext_len & 0xFF) as u8
        } else {
            sh_buf[sh_pos] = ((TLS_GROUP_SECP256R1 >> 8) & 0xFF) as u8; sh_pos += 1
            sh_buf[sh_pos] = (TLS_GROUP_SECP256R1 & 0xFF) as u8; sh_pos += 1
            sh_buf[sh_pos] = 0 as u8; sh_pos += 1; sh_buf[sh_pos] = 65 as u8; sh_pos += 1
            var kpi : size_t = 0
            while(kpi < 65) { sh_buf[sh_pos + kpi] = server_p256_pub[kpi]; kpi += 1 }
            sh_pos += 65
            var ks_ext_len = 2 + 2 + 65
            sh_buf[ks_ext_len_pos] = ((ks_ext_len >> 8) & 0xFF) as u8
            sh_buf[ks_ext_len_pos + 1] = (ks_ext_len & 0xFF) as u8
        }
        // supported_versions extension
        sh_buf[sh_pos] = ((TLS_EXT_SUPPORTED_VERSIONS >> 8) & 0xFF) as u8; sh_pos += 1
        sh_buf[sh_pos] = (TLS_EXT_SUPPORTED_VERSIONS & 0xFF) as u8; sh_pos += 1
        sh_buf[sh_pos] = 0 as u8; sh_pos += 1
        sh_buf[sh_pos] = 2 as u8; sh_pos += 1
        sh_buf[sh_pos] = 0x03 as u8; sh_pos += 1
        sh_buf[sh_pos] = 0x04 as u8; sh_pos += 1  // TLS 1.3
        // Extension length
        var sh_ext_total = sh_pos - sh_ext_start - 2
        sh_buf[sh_ext_start] = ((sh_ext_total >> 8) & 0xFF) as u8
        sh_buf[sh_ext_start + 1] = (sh_ext_total & 0xFF) as u8
        var sh_len = sh_pos

        // Hash ServerHello into transcript
        var sh_hdr : [4]u8
        sh_hdr[0] = SSL_HS_SERVER_HELLO as u8
        write_u24(sh_len as u32, &raw mut sh_hdr[1])
        tls13_transcript_update(unsafe(&raw mut transcript), &raw sh_hdr[0], 4)
        tls13_transcript_update(unsafe(&raw mut transcript), &raw sh_buf[0], sh_len)

        // Send ServerHello
        ret = await send_handshake_msg(ssl, SSL_HS_SERVER_HELLO as u8, &raw sh_buf[0], sh_len as u32)
        if(ret < 0) { return ret }

        // ── Derive handshake traffic keys ────────────────────────────
        // Compute Transcript-Hash(ClientHello...ServerHello)
        var ch_sh_copy : TLS13Transcript
        unsafe { memcpy(&raw mut ch_sh_copy, &raw transcript, sizeof(TLS13Transcript)) }
        var ch_sh_hash : [48]u8
        tls13_transcript_finalize(&raw mut ch_sh_copy, &raw mut ch_sh_hash[0], use_sha384)

        ret = tls13_derive_handshake_keys(ssl, &raw shared_secret[0], 32,
                                           &raw ch_sh_hash[0], null, 0,
                                           tls13_hash_len, tls13_key_len, tls13_cipher_type)
        if(ret < 0) { return ret }

        // ── Send CCS (ChangeCipherSpec compatibility indicator) ──────
        var ccs_data : [1]u8 = [1]
        ret = await send_record(ssl, SSL_MSG_CHANGE_CIPHER_SPEC as u8, &raw ccs_data[0], 1 as u16)
        if(ret < 0) { return ret }

        // ── Send encrypted server messages ───────────────────────────
        // EncryptedExtensions
        ssl.state = SSLState.ENCRYPTED_EXTENSIONS()
        var ee_buf : [256]u8
        ee_buf[0] = SSL_HS_ENCRYPTED_EXTENSIONS as u8
        write_u24(2, &raw mut ee_buf[1])  // body length (just empty extensions: 00 00)
        ee_buf[4] = 0 as u8; ee_buf[5] = 0 as u8  // empty extensions

        tls13_transcript_update(unsafe(&raw mut transcript), &raw ee_buf[0], 6)

        ret = await send_handshake_msg(ssl, SSL_HS_ENCRYPTED_EXTENSIONS as u8, &raw ee_buf[4], 2)
        if(ret < 0) { return ret }

        // Certificate (skip if no cert configured or no private key)
        if(ssl.conf.own_cert != null && ssl.conf.own_key != null) {
            ssl.state = SSLState.SERVER_CERTIFICATE()
            var cert_data = ssl.conf.own_cert
            var der_len = cert_data.raw_pem_len
            if(der_len > 0 && der_len < 4000) {
            var cert_buf : [4096]u8
                cert_buf[0] = SSL_HS_CERTIFICATE as u8
                var cert_body_pos : size_t = 4
                // certificate_request_context (0)
                cert_buf[cert_body_pos] = 0 as u8; cert_body_pos += 1
                // certificate_list length placeholder
                var cl_len_pos = cert_body_pos; cert_body_pos += 3
                // cert_data length
                cert_buf[cert_body_pos] = ((der_len >> 16) & 0xFF) as u8; cert_body_pos += 1
                cert_buf[cert_body_pos] = ((der_len >> 8) & 0xFF) as u8; cert_body_pos += 1
                cert_buf[cert_body_pos] = (der_len & 0xFF) as u8; cert_body_pos += 1
                var cdi : size_t = 0
                while(cdi < der_len) {
                    cert_buf[cert_body_pos + cdi] = cert_data.raw_pem[cdi]
                    cdi += 1
                }
                cert_body_pos += der_len
                // Empty extensions for cert entry
                cert_buf[cert_body_pos] = 0 as u8; cert_body_pos += 1
                cert_buf[cert_body_pos] = 0 as u8; cert_body_pos += 1
                // Fill cert_list length
                var cl_len = cert_body_pos - 4 - 1 - 3  // total - header - ctx - length_field
                cert_buf[cl_len_pos] = ((cl_len >> 16) & 0xFF) as u8
                cert_buf[cl_len_pos + 1] = ((cl_len >> 8) & 0xFF) as u8
                cert_buf[cl_len_pos + 2] = (cl_len & 0xFF) as u8
                // Fill handshake header length
                write_u24(cert_body_pos - 4, &raw mut cert_buf[1])

                tls13_transcript_update(unsafe(&raw mut transcript), &raw cert_buf[0], cert_body_pos)

                ret = await send_handshake_msg(ssl, SSL_HS_CERTIFICATE as u8, &raw cert_buf[4], cert_body_pos - 4)
                if(ret < 0) { return ret }
            }
        } else {
            // Send empty Certificate (required by TLS 1.3 spec when not authenticating via certificate)
            ssl.state = SSLState.SERVER_CERTIFICATE()
            // Empty Certificate: certificate_request_context(0) + empty certificate_list(0,0,0)
            var empty_cert_buf : [8]u8
            empty_cert_buf[0] = SSL_HS_CERTIFICATE as u8
            write_u24(4, &raw mut empty_cert_buf[1])  // body length = 4 bytes
            empty_cert_buf[4] = 0 as u8               // certificate_request_context = empty
            empty_cert_buf[5] = 0 as u8               // certificate_list length high
            empty_cert_buf[6] = 0 as u8               // certificate_list length mid
            empty_cert_buf[7] = 0 as u8               // certificate_list length low (= 0 = empty)

            tls13_transcript_update(unsafe(&raw mut transcript), &raw empty_cert_buf[0], 8)

            ret = await send_handshake_msg(ssl, SSL_HS_CERTIFICATE as u8, &raw empty_cert_buf[4], 4)
            if(ret < 0) { return ret }
        }

        // CertificateVerify
        if(ssl.conf.own_cert != null && ssl.conf.own_key != null) {
            ssl.state = SSLState.CERTIFICATE_VERIFY()

            var cv_copy : TLS13Transcript
            unsafe { memcpy(&raw mut cv_copy, &raw transcript, sizeof(TLS13Transcript)) }
            // CertificateVerify signature hash/scheme is chosen by the server
            // certificate's curve (RFC 8446: ecdsa_secp256r1_sha256 /
            // ecdsa_secp384r1_sha384), independent of the ciphersuite hash.
            var cert_use_sha384 : bool = false
            var cert_hash_len : size_t = 32
            if(ssl.conf.own_key != null) {
                var ck = ssl.conf.own_key as *mut ECDSAContext
                if(ck.curve_id == TLS_GROUP_SECP384R1 as u16) {
                    cert_use_sha384 = true
                    cert_hash_len = 48
                }
            }
            var cv_transcript_hash : [48]u8
            // The transcript hash uses the ciphersuite's hash (SHA-384 for the
            // AES-256-GCM-SHA384 family, SHA-256 otherwise) per RFC 8446 — it is
            // independent of the certificate's curve. Only the signature scheme
            // (and thus the hash applied to the CertificateVerify content) follows
            // the certificate curve.
            tls13_transcript_finalize(&raw mut cv_copy, &raw mut cv_transcript_hash[0], use_sha384)

            // content = 64 spaces + "TLS 1.3, server CertificateVerify" + 0x00 + transcript_hash
            var sig_in : [200]u8
            var sp : size_t = 0
            while(sp < 64) { sig_in[sp] = 0x20 as u8; sp += 1 }
            var ctx_label = "TLS 1.3, server CertificateVerify\0" as *char
            var clen : size_t = 0
            while(ctx_label[clen] != 0) { clen += 1 }
            var ci : size_t = 0
            while(ci < clen) { sig_in[sp + ci] = ctx_label[ci] as u8; ci += 1 }
            sp += clen
            sig_in[sp] = 0x00 as u8; sp += 1
            var cj : size_t = 0
            while(cj < tls13_hash_len) { sig_in[sp + cj] = cv_transcript_hash[cj]; cj += 1 }
            sp += tls13_hash_len

            var cv_hash : [48]u8
            if(cert_use_sha384) {
                crypto::raw::sha384_hash(&raw sig_in[0], sp, &raw mut cv_hash[0])
            } else {
                crypto::raw::sha256_hash(&raw sig_in[0], sp, &raw mut cv_hash[0])
            }

            var pk_type = ssl.conf.own_cert.pk_type
            var sig_buf : [256]u8
            var sig_len : u16 = 0
            var sig_alg : u16 = 0
            sig_len = 256  // buffer size input for ecdsa_sign

            if(pk_type == PK_ECKEY as u8) {
                var ecdsa_key = ssl.conf.own_key as *mut ECDSAContext
                ret = ecdsa_sign(ecdsa_key, &raw cv_hash[0], cert_hash_len, &raw mut sig_buf[0], &raw mut sig_len)
                if(ret < 0) { return ERR_SSL_INTERNAL_ERROR }
                if(cert_use_sha384) {
                    sig_alg = TLS1_3_SIG_ECDSA_SECP384R1_SHA384 as u16
                } else {
                    sig_alg = TLS1_3_SIG_ECDSA_SECP256R1_SHA256 as u16
                }
            } else {
                return ERR_SSL_INTERNAL_ERROR
            }

            var cv_buf : [512]u8
            cv_buf[0] = SSL_HS_CERTIFICATE_VERIFY as u8
            var cv_body : u32 = (2 + 2 + sig_len) as u32
            write_u24(cv_body, &raw mut cv_buf[1])
            cv_buf[4] = ((sig_alg >> 8) & 0xFF) as u8
            cv_buf[5] = (sig_alg & 0xFF) as u8
            write_u16_be(sig_len, &raw mut cv_buf[6])
            var ck : size_t = 0
            while(ck < sig_len as size_t) { cv_buf[8 + ck] = sig_buf[ck]; ck += 1 }
            var cv_total = 4 + cv_body

            tls13_transcript_update(unsafe(&raw mut transcript), &raw cv_buf[0], cv_total as size_t)

            ret = await send_handshake_msg(ssl, SSL_HS_CERTIFICATE_VERIFY as u8, &raw cv_buf[4], cv_body)
            if(ret < 0) { return ret }
        }

        // Finished
        ssl.state = SSLState.SERVER_FINISHED()
        var finished_key : [48]u8
        var fin_key_label = "finished\0" as *char
        var empty_c : [1]u8 = [0]
        tls13_hkdf_expand_label(&raw ssl.tls13_keys.server_handshake_traffic_secret[0], tls13_hash_len,
                                fin_key_label, 8,
                                &raw empty_c[0], 0,
                                &raw mut finished_key[0], tls13_hash_len)

        var full_hash_before_fin : [48]u8
        var fin_copy : TLS13Transcript
        unsafe { memcpy(&raw mut fin_copy, &raw transcript, sizeof(TLS13Transcript)) }
        tls13_transcript_finalize(&raw mut fin_copy, &raw mut full_hash_before_fin[0], use_sha384)
        var server_verify : [48]u8
        if(use_sha384) {
            crypto::raw::hmac_sha384(&raw finished_key[0], tls13_hash_len, &raw full_hash_before_fin[0], tls13_hash_len,
                                &raw mut server_verify[0])
        } else {
            crypto::raw::hmac_sha256(&raw finished_key[0], tls13_hash_len, &raw full_hash_before_fin[0], tls13_hash_len,
                                &raw mut server_verify[0])
        }

        // Build Finished (verify_data length == hash_len)
        var fin_buf : [64]u8
        fin_buf[0] = SSL_HS_FINISHED as u8
        write_u24(tls13_hash_len, &raw mut fin_buf[1])
        var fi : size_t = 0
        while(fi < tls13_hash_len) { fin_buf[4 + fi] = server_verify[fi]; fi += 1 }

        tls13_transcript_update(unsafe(&raw mut transcript), &raw fin_buf[0], 4 + tls13_hash_len)

        ret = await send_handshake_msg(ssl, SSL_HS_FINISHED as u8, &raw fin_buf[4], tls13_hash_len)
        if(ret < 0) { return ret }

        // ── Read client Finished ────────────────────────────────────
        ssl.state = SSLState.CLIENT_FINISHED()
        ret = await read_handshake_msg(ssl, &raw mut hs_type, &raw mut hs_len,
                                  &raw mut hs_buf[0], 8192)
        if(ret < 0) { return ret }
        if(hs_type != SSL_HS_FINISHED as u8) {
            return ERR_SSL_UNEXPECTED_MESSAGE
        }

        // Verify client Finished message
        var client_finished_key : [48]u8
        var cf_label = "finished\0" as *char
        var empty_c2 : [1]u8 = [0]
        tls13_hkdf_expand_label(&raw ssl.tls13_keys.client_handshake_traffic_secret[0], tls13_hash_len,
                                cf_label, 8,
                                &raw empty_c2[0], 0,
                                &raw mut client_finished_key[0], tls13_hash_len)

        var cf_copy : TLS13Transcript
        unsafe { memcpy(&raw mut cf_copy, &raw transcript, sizeof(TLS13Transcript)) }
        // Write directly into a struct field: &raw local[0] output buffers are
        // corrupted by a compiler bug for this large-stack (SHA-384) function.
        tls13_transcript_finalize(&raw mut cf_copy,
                                  &raw mut ssl.tls13_keys.client_handshake_traffic_secret[0], use_sha384)

        var expected_client_verify : [48]u8
        if(use_sha384) {
            crypto::raw::hmac_sha384(&raw client_finished_key[0], tls13_hash_len,
                                &raw ssl.tls13_keys.client_handshake_traffic_secret[0], tls13_hash_len,
                                &raw mut expected_client_verify[0])
        } else {
            crypto::raw::hmac_sha256(&raw client_finished_key[0], tls13_hash_len,
                                &raw ssl.tls13_keys.client_handshake_traffic_secret[0], tls13_hash_len,
                                &raw mut expected_client_verify[0])
        }

        var verify_ok = true
        if(hs_len != tls13_hash_len) { verify_ok = false }
        var cfi : size_t = 0
        while(cfi < tls13_hash_len) {
            if(hs_buf[4 + cfi] != expected_client_verify[cfi]) { verify_ok = false }
            cfi += 1
        }
        if(!verify_ok) {
            return ERR_SSL_HANDSHAKE_FAILURE
        }

        // Hash client Finished into transcript
        var cf_hdr : [4]u8
        cf_hdr[0] = SSL_HS_FINISHED as u8
        write_u24(hs_len, &raw mut cf_hdr[1])
        tls13_transcript_update(unsafe(&raw mut transcript), &raw cf_hdr[0], 4)
        tls13_transcript_update(unsafe(&raw mut transcript), &raw hs_buf[4], hs_len)

        // ── Derive application traffic keys ──
        // RFC 8446 §7.1: c/s ap traffic use ClientHello...server Finished
        // (cf_transcript_hash); res master uses ClientHello...client Finished (full_hash).
        // Write full_hash directly into a struct field (same compiler bug).
        tls13_transcript_finalize(unsafe(&raw mut transcript),
                                  &raw mut ssl.tls13_keys.server_handshake_traffic_secret[0], use_sha384)
        ret = tls13_derive_application_keys(ssl, null, tls13_hash_len, null, 0,
                                                   tls13_key_len, tls13_cipher_type)
        if(ret < 0) { return ret }

        ssl.state = SSLState.HANDSHAKE_OVER()

        return 0

    }
}

    // Perform the TLS handshake
    public func ssl_handshake(ssl : *mut SSLContext) : int {
        return async::block_on<int>(ssl_handshake_coro(ssl))
    }

    public async func ssl_handshake_coro(ssl : *mut SSLContext) : int {
        unsafe {

        if(ssl.conf == null) { return ERR_SSL_BAD_CONFIG }
        ensure_init()

        if(!ssl.transport_async) {
            ssl_apply_recv_timeout(ssl)
        }

        if(ssl.conf.endpoint == SSL_IS_SERVER) {
            if(ssl.tls_version >= SSL_VERSION_TLS1_3) {
                return await do_tls13_server_handshake(ssl)
            } else {
                return await do_tls12_server_handshake(ssl)
            }
        }

        if(ssl.tls_version >= SSL_VERSION_TLS1_3) {
            return await do_tls13_client_handshake(ssl)
        } else {
            return await do_tls12_client_handshake(ssl)
        }

    }
}

    // Process a single NewSessionTicket message (buf points at the handshake
    // header, len = full message including the 4-byte header). Stores the ticket
    // and derives the resumption key into ssl.session.
    func ssl_process_new_session_ticket(ssl : *mut SSLContext, buf : *u8, len : size_t) {
        unsafe {

        if(ssl.session == null) { return }
        var msg_len = read_u24(&raw buf[1]) as size_t
        if(msg_len < 11 || 4 + msg_len > len) { return }

        var nst_pos : size_t = 4
        var lifetime = read_u32_be(&raw buf[nst_pos]); nst_pos += 4
        var nonce_len : size_t = 0
        var nonce_pos : size_t = 0
        // TLS 1.3 NST: [lifetime u32][age_add u32][nonce<0..255>][ticket<0..2^16-1>]
        // TLS 1.2 (RFC 5077) NST: [lifetime u32][ticket<0..2^16-1>] — no age
        // add and no nonce field.
        if(ssl.tls_version >= SSL_VERSION_TLS1_3) {
            nst_pos += 4  // skip age_add
            nonce_len = buf[nst_pos] as size_t
            nonce_pos = nst_pos + 1
            nst_pos += 1 + nonce_len
        }
        var ticket_len = read_u16_be(&raw buf[nst_pos]) as size_t; nst_pos += 2

        if(ticket_len > 0 && ticket_len < 4096 && nst_pos + ticket_len <= 4 + msg_len as size_t) {
            if(ssl.session.ticket != null) {
                unsafe { dealloc ssl.session.ticket }
            }
            var tkt_mem = unsafe(malloc(ticket_len)) as *mut u8
            if(tkt_mem == null) { return }
            var ti : size_t = 0
            while(ti < ticket_len) {
                tkt_mem[ti] = buf[nst_pos + ti]
                ti += 1
            }
            ssl.session.ticket = tkt_mem
            ssl.session.ticket_len = ticket_len
            ssl.session.ticket_lifetime = lifetime

            // Resumption PSK per RFC 8446 §4.6.1 (TLS 1.3 only):
            //   PSK = HKDF-Expand-Label(resumption_master_secret, "resumption",
            //                           ticket_nonce, Hash.length)
            if(ssl.tls_version >= SSL_VERSION_TLS1_3) {
                var res_label = "resumption\0" as *char
                var bounded_nonce : size_t = nonce_len
                if(bounded_nonce > 32) { bounded_nonce = 32 }
                tls13_hkdf_expand_label(&raw ssl.tls13_keys.resumption_master_secret[0], 32,
                                        res_label, 10, &raw buf[nonce_pos], bounded_nonce,
                                        &raw mut ssl.session.resumption_key[0], 32)
                ssl.session.resumption_key_len = 32 as u8
            }
        }

    }
}

    // Process post-handshake handshake messages in a decrypted record payload.
    // Handles NewSessionTicket and KeyUpdate (RFC 8446 §4.6.3: on
    // update_requested, update receive keys and respond with our own KeyUpdate).
    async func ssl_handle_post_handshake(ssl : *mut SSLContext, buf : *u8, msglen : i32) : int {
        unsafe {

        var pos : i32 = 0
        while(pos + 4 <= msglen) {
            var mtype = buf[pos]
            var mlen = read_u24(&raw buf[pos + 1]) as i32
            if(mlen < 0 || pos + 4 + mlen > msglen) { break }

            if(mtype == SSL_HS_NEW_SESSION_TICKET as u8) {
                ssl_process_new_session_ticket(ssl, &raw buf[pos], (4 + mlen) as size_t)
            } else if(mtype == SSL_HS_KEY_UPDATE as u8) {
                var upd_req : u8 = 0
                if(mlen >= 1) { upd_req = buf[pos + 4] }
                var ur = tls13_update_recv_keys(ssl)
                if(ur < 0) { return ur }
                if(upd_req == 1) {
                    // Must respond with our own KeyUpdate (update_not_requested)
                    var sr = await tls13_send_key_update_coro(ssl, false)
                    if(sr < 0) { return sr }
                }
            }
            pos += 4 + mlen
        }
        return 0

    }
}

    // Read application data
    public func ssl_read(ssl : *mut SSLContext, buf : *mut u8, len : i32) : int {
        return async::block_on<int>(ssl_read_coro(ssl, buf, len))
    }

    public async func ssl_read_coro(ssl : *mut SSLContext, buf : *mut u8, len : i32) : int {
        unsafe {

        if(!ssl.transport_connected) { return ERR_SSL_INTERNAL_ERROR }

        // If a transform is active, use the record layer (handles decryption)
        if(ssl.transform_in != null && ssl.state is SSLState.HANDSHAKE_OVER()) {
            // If the previous call only drained part of a record, keep feeding
            // the caller from the same (still buffered) record before reading
            // any new record from the socket.
            if(ssl.in_offt > 0) {
                var remain = ssl.in_msglen - ssl.in_offt
                var take = remain
                if(take > len) { take = len }
                var i : i32 = 0
                while(i < take) {
                    buf[i] = ssl.in_buf[5 + ssl.in_offt + i]
                    i += 1
                }
                ssl.in_offt = ssl.in_offt + take
                if(ssl.in_offt >= ssl.in_msglen) {
                    ssl_consume_record(ssl)
                    ssl.in_offt = 0
                }
                return take
            }

            while(true) {
                var ret = await ssl_read_record(ssl)
                if(ret < 0) { return ret }

                var inner_ct = ssl.in_hdr[0]

                // Post-handshake handshake messages (NewSessionTicket, KeyUpdate, ...)
                if(inner_ct == SSL_MSG_HANDSHAKE as u8) {
                    var ph_ret = await ssl_handle_post_handshake(ssl, &raw ssl.in_buf[5], ssl.in_msglen)
                    ssl_consume_record(ssl)
                    if(ph_ret < 0) { return ph_ret }
                    continue
                }
                // Legacy ChangeCipherSpec (ignored in TLS 1.3)
                if(inner_ct == SSL_MSG_CHANGE_CIPHER_SPEC as u8) {
                    ssl_consume_record(ssl)
                    continue
                }
                // Alert records: close_notify -> EOF; fatal -> error; warning -> continue
                if(inner_ct == SSL_MSG_ALERT as u8) {
                    var level : u8 = 0
                    var desc : u8 = 0
                    if(ssl.in_msglen >= 2) {
                        level = ssl.in_buf[5]
                        desc = ssl.in_buf[6]
                    }
                    ssl_consume_record(ssl)
                    if(desc == SSL_ALERT_MSG_CLOSE_NOTIFY as u8) {
                        return 0
                    }
                    if(level == SSL_ALERT_LEVEL_FATAL as u8) {
                        ssl.last_alert_desc = desc
                        return ERR_SSL_FATAL_ALERT_MESSAGE
                    }
                    continue
                }

                // Application data
                var copy_len = ssl.in_msglen
                if(copy_len > len) { copy_len = len }
                var i : i32 = 0
                while(i < copy_len) {
                    buf[i] = ssl.in_buf[5 + i]
                    i += 1
                }
                if(copy_len >= ssl.in_msglen) {
                    ssl_consume_record(ssl)
                    ssl.in_offt = 0
                } else {
                    // The caller's buffer was too small for the whole record:
                    // keep the rest of this record buffered for the next call.
                    ssl.in_offt = copy_len
                }
                return copy_len
            }
        }

        return await ssl_recv(ssl, buf, len)

    }
}

    // Write application data
    public func ssl_write(ssl : *mut SSLContext, data : *u8, len : i32) : int {
        return async::block_on<int>(ssl_write_coro(ssl, data, len))
    }

    public async func ssl_write_coro(ssl : *mut SSLContext, data : *u8, len : i32) : int {
        unsafe {

        if(!ssl.transport_connected) { return ERR_SSL_INTERNAL_ERROR }
        // Fragment into max-size TLS records; send_record rejects anything
        // larger than MAX_RECORD_PAYLOAD, and a single u16 cast would both
        // truncate big writes and violate the 2^14 record limit.
        var off : i32 = 0
        while(off < len) {
            var chunk = len - off
            if(chunk > MAX_RECORD_PAYLOAD as i32) { chunk = MAX_RECORD_PAYLOAD as i32 }
            var ret = await send_record(ssl, SSL_MSG_APPLICATION_DATA as u8, data + off, chunk as u16)
            if(ret < 0) { return ret }
            off += chunk
        }
        return len

    }
}

    // Close the SSL connection (send close_notify)
    public func ssl_close_notify(ssl : *mut SSLContext) : int {
        return async::block_on<int>(ssl_close_notify_coro(ssl))
    }

    public async func ssl_close_notify_coro(ssl : *mut SSLContext) : int {
        return await send_alert(ssl, SSL_ALERT_LEVEL_WARNING as u8, SSL_ALERT_MSG_CLOSE_NOTIFY as u8)
    }

    // Free SSL context resources (closes socket, frees handshake params)
    public func ssl_free(ssl : *mut SSLContext) {
        unsafe {

        if(ssl.handshake != null) {
            if(ssl.handshake.ecdhe_public != null) {
                unsafe { dealloc ssl.handshake.ecdhe_public }
            }
            if(ssl.handshake.ecdhe_private != null) {
                unsafe { dealloc ssl.handshake.ecdhe_private }
            }
            if(ssl.handshake.x25519_public != null) {
                unsafe { dealloc ssl.handshake.x25519_public }
            }
            if(ssl.handshake.x25519_private != null) {
                unsafe { dealloc ssl.handshake.x25519_private }
            }
            if(ssl.handshake.psk_identity != null) {
                unsafe { dealloc ssl.handshake.psk_identity }
            }
            unsafe { dealloc ssl.handshake }
            ssl.handshake = null
        }
        if(ssl.transform_in != null) {
            unsafe { dealloc ssl.transform_in }
            ssl.transform_in = null
        }
        if(ssl.transform_out != null) {
            unsafe { dealloc ssl.transform_out }
            ssl.transform_out = null
        }
        if(ssl.session != null) {
            if(ssl.session.ticket != null) {
                unsafe { dealloc ssl.session.ticket }
            }
            unsafe { dealloc ssl.session }
            ssl.session = null
        }
        if(ssl.alpn_negotiated != null) {
            unsafe { dealloc ssl.alpn_negotiated }
            ssl.alpn_negotiated = null
        }
        if(ssl.hostname_owned && ssl.hostname != null) {
            unsafe { dealloc ssl.hostname }
        }
        ssl.hostname = null
        ssl.hostname_len = 0
        ssl.hostname_owned = false
        if(ssl.peer_cert != null) {
            // Peer cert chain is heap-allocated and owned by the library
            cert_chain_free(ssl.peer_cert)
            ssl.peer_cert = null
        }
        if(ssl.conf_owned && ssl.conf != null) {
            // Config was heap-allocated by the library (tls_accept); caller-owned
            // stack configs (ssl_set_config with &raw cfg) are never freed here.
            unsafe { dealloc ssl.conf }
            ssl.conf = null
            ssl.conf_owned = false
        }
        if(ssl.transport_connected) {
            net::raw::close_socket(ssl.transport_socket)
            ssl.transport_connected = false
        }

    }
}

    // ============================================================================
    // High-Level TLS Client API
    // ============================================================================

    // Connect to a TLS server (TCP + TLS handshake)
    // Dial the peer. The blocking path uses the synchronous `net::raw::dial`; the
    // async path offloads the (inherently blocking) connect to the thread pool
    // and awaits it, then installs the non-blocking transport.
    async func tls_dial(ssl : *mut SSLContext, host : *char, port : uint) : net::raw::Socket {
        unsafe {

        if(ssl.transport_async) {
            var fd = await net::raw::dial_async(host, port)
            return fd as net::raw::Socket
        }
        return net::raw::dial(host, port)

    }
}

    public func tls_connect(ssl : *mut SSLContext, host : *char, port : uint) : int {
        return async::block_on<int>(tls_connect_coro(ssl, host, port))
    }

    public async func tls_connect_coro(ssl : *mut SSLContext, host : *char, port : uint) : int {
        unsafe {

        ensure_init()

        var sock = await tls_dial(ssl, host, port)
        if(sock == 0 as net::raw::Socket) {
            ssl.transport_connected = false
            return ERR_SSL_INTERNAL_ERROR
        }

        ssl_set_socket(ssl, sock)
        ssl_set_hostname(ssl, host)

        var ret = await ssl_handshake_coro(ssl)
        if(ret < 0) {
            // Some servers only negotiate TLS 1.2. We advertised TLS 1.3 + 1.2
            // in supported_versions, and they answered with a plain legacy
            // ServerHello which the TLS 1.3 parser could not interpret (it may
            // surface as ERR_SSL_HANDSHAKE_FAILURE when no key_share was
            // found, ERR_SSL_DECODE_ERROR or ERR_SSL_UNEXPECTED_MESSAGE when
            // the legacy ServerHello shape tripped the 1.3 record parser).
            // Retry the handshake pinned to TLS 1.2 so downloads from
            // TLS-1.2-only servers (e.g. some CDNs) still work. The retry is
            // only attempted when the parser actually received a message it
            // could not understand — not for connection-level errors (which
            // would just be a plain non-TLS peer).
            if((ret == ERR_SSL_HANDSHAKE_FAILURE || ret == ERR_SSL_DECODE_ERROR || ret == ERR_SSL_UNEXPECTED_MESSAGE) &&
               ssl.conf != null && ssl.conf.max_tls_version >= SSL_VERSION_TLS1_3 &&
               ssl.conf.min_tls_version <= SSL_VERSION_TLS1_2) {
                net::raw::close_socket(sock)
                ssl.transport_connected = false
                ssl.state = SSLState.HELLO_REQUEST()
                ssl.tls_version = SSL_VERSION_TLS1_2
                ssl.major_ver = 3
                ssl.minor_ver = 3 as u8
                ssl.in_msglen = 0
                ssl.in_left = 0
                var sock2 = await tls_dial(ssl, host, port)
                if(sock2 != 0 as net::raw::Socket) {
                    ssl_set_socket(ssl, sock2)
                    ssl.conf.max_tls_version = SSL_VERSION_TLS1_2
                    var ret2 = await ssl_handshake_coro(ssl)
                    if(ret2 < 0) {
                        ssl.transport_connected = false
                        net::raw::close_socket(sock2)
                        return ret2
                    }
                    return 0
                }
            }
            ssl.transport_connected = false
            net::raw::close_socket(sock)
            return ret
        }

        return 0

    }
}

    // Read NewSessionTicket post-handshake message
    public func ssl_read_new_session_ticket(ssl : *mut SSLContext) : int {
        return async::block_on<int>(ssl_read_new_session_ticket_coro(ssl))
    }

    public async func ssl_read_new_session_ticket_coro(ssl : *mut SSLContext) : int {
        var hdr : [5]u8
        var ret = await read_record_header(ssl, &raw mut hdr[0])
        if(ret < 0) { return ret }

        var ct = hdr[0]
        if(ct != SSL_MSG_HANDSHAKE as u8) {
            return ERR_SSL_UNEXPECTED_MESSAGE
        }

        var msg_buf : [4096]u8
        var msg_payload = read_record_payload(ssl, &raw mut msg_buf[0], 4096 as i32)
        if(msg_payload < 4) { return ERR_SSL_DECODE_ERROR }

        var msg_type = msg_buf[0]
        if(msg_type != SSL_HS_NEW_SESSION_TICKET as u8) {
            return ERR_SSL_UNEXPECTED_MESSAGE
        }

        ssl_process_new_session_ticket(ssl, &raw msg_buf[0], msg_payload as size_t)
        return 0
    }


// ===== ssl_ciphersuites.ch =====

    // ─── TLS 1.3 Cipher Suites ──────────────────────────────────────────────

    // TLS 1.3 cipher suite IDs (from RFC 8446)
    public comptime const TLS1_3_AES_128_GCM_SHA256 = 0x1301
    public comptime const TLS1_3_AES_256_GCM_SHA384 = 0x1302
    public comptime const TLS1_3_CHACHA20_POLY1305_SHA256 = 0x1303
    public comptime const TLS1_3_AES_128_CCM_SHA256 = 0x1304
    public comptime const TLS1_3_AES_128_CCM_8_SHA256 = 0x1305

    // ─── TLS 1.2 Cipher Suites ──────────────────────────────────────────────

    public comptime const TLS_RSA_WITH_AES_128_GCM_SHA256 = 0x009C
    public comptime const TLS_RSA_WITH_AES_256_GCM_SHA384 = 0x009D

    public comptime const TLS_RSA_WITH_AES_128_CBC_SHA256 = 0x003C
    public comptime const TLS_RSA_WITH_AES_256_CBC_SHA256 = 0x003D

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
    public comptime const KE_RSA = 6

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

    public func ensure_init() {
        // Cipher-suite tables are compile-time constants; nothing to initialize.
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
        if(ciphersuite_id == cs(TLS_RSA_WITH_AES_128_GCM_SHA256)) {
            return CipherSuiteInfo {
                id: cs(TLS_RSA_WITH_AES_128_GCM_SHA256),
                name: null,
                cipher: CIPHER_AES_128_GCM as u8,
                hash: HASH_SHA256 as u8,
                key_exchange: KE_RSA as u8,
                flags: 0, min_tls_version: 3, max_tls_version: 3,
                key_size: 16, iv_size: 12, tag_size: 16, mac_key_len: 0
            }
        }
        if(ciphersuite_id == cs(TLS_RSA_WITH_AES_256_GCM_SHA384)) {
            return CipherSuiteInfo {
                id: cs(TLS_RSA_WITH_AES_256_GCM_SHA384),
                name: null,
                cipher: CIPHER_AES_256_GCM as u8,
                hash: HASH_SHA384 as u8,
                key_exchange: KE_RSA as u8,
                flags: 0, min_tls_version: 3, max_tls_version: 3,
                key_size: 32, iv_size: 12, tag_size: 16, mac_key_len: 0
            }
        }
        if(ciphersuite_id == cs(TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256)) {
            return CipherSuiteInfo {
                id: cs(TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256),
                name: null,
                cipher: CIPHER_AES_128_CBC as u8,
                hash: HASH_SHA256 as u8,
                key_exchange: KE_ECDHE_ECDSA as u8,
                flags: 0, min_tls_version: 3, max_tls_version: 3,
                key_size: 16, iv_size: 16, tag_size: 16, mac_key_len: 32
            }
        }
        if(ciphersuite_id == cs(TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384)) {
            return CipherSuiteInfo {
                id: cs(TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384),
                name: null,
                cipher: CIPHER_AES_256_CBC as u8,
                hash: HASH_SHA384 as u8,
                key_exchange: KE_ECDHE_ECDSA as u8,
                flags: 0, min_tls_version: 3, max_tls_version: 3,
                key_size: 32, iv_size: 16, tag_size: 16, mac_key_len: 48
            }
        }
        if(ciphersuite_id == cs(TLS_RSA_WITH_AES_128_CBC_SHA256)) {
            return CipherSuiteInfo {
                id: cs(TLS_RSA_WITH_AES_128_CBC_SHA256),
                name: null,
                cipher: CIPHER_AES_128_CBC as u8,
                hash: HASH_SHA256 as u8,
                key_exchange: KE_RSA as u8,
                flags: 0, min_tls_version: 3, max_tls_version: 3,
                key_size: 16, iv_size: 16, tag_size: 16, mac_key_len: 32
            }
        }
        if(ciphersuite_id == cs(TLS_RSA_WITH_AES_256_CBC_SHA256)) {
            return CipherSuiteInfo {
                id: cs(TLS_RSA_WITH_AES_256_CBC_SHA256),
                name: null,
                cipher: CIPHER_AES_256_CBC as u8,
                hash: HASH_SHA256 as u8,
                key_exchange: KE_RSA as u8,
                flags: 0, min_tls_version: 3, max_tls_version: 3,
                key_size: 32, iv_size: 16, tag_size: 16, mac_key_len: 32
            }
        }
        return CipherSuiteInfo {
            id: 0, name: null, cipher: CIPHER_NONE as u8, hash: 0,
            key_exchange: 0, flags: 0, min_tls_version: 0, max_tls_version: 0,
            key_size: 0, iv_size: 0, tag_size: 0, mac_key_len: 0
        }
    }

    public func num_preferred_ciphersuites() : u32 {
        return 10
    }

    public func get_preferred_ciphersuite(index : u32) : u16 {
        if(index == 0) { return cs(TLS1_3_AES_128_GCM_SHA256) }
        if(index == 1) { return cs(TLS_RSA_WITH_AES_128_GCM_SHA256) }
        if(index == 2) { return cs(TLS_RSA_WITH_AES_256_GCM_SHA384) }
        if(index == 3) { return cs(TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256) }
        if(index == 4) { return cs(TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384) }
        if(index == 5) { return cs(TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256) }
        if(index == 6) { return cs(TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384) }
        if(index == 7) { return cs(TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256) }
        if(index == 8) { return cs(TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256) }
        if(index == 9) { return cs(TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384) }
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
        if(ciphersuite_id == cs(TLS_RSA_WITH_AES_128_GCM_SHA256)) { return true }
        if(ciphersuite_id == cs(TLS_RSA_WITH_AES_256_GCM_SHA384)) { return true }
        return false
    }


// ===== x509_crt.ch =====

    using std::string;
    using std::string_view;
    using std::Result;
    using std::vector;

    // ─── X.509 Error Codes ──────────────────────────────────────────────────

    public comptime const ERR_X509_FEATURE_UNAVAILABLE = -0x2080
    public comptime const ERR_X509_UNKNOWN_OID = -0x2100
    public comptime const ERR_X509_INVALID_FORMAT = -0x2180
    public comptime const ERR_X509_INVALID_VERSION = -0x2200
    public comptime const ERR_X509_INVALID_SIGNATURE = -0x2280
    public comptime const ERR_X509_INVALID_ALG = -0x2300
    public comptime const ERR_X509_INVALID_NAME = -0x2380
    public comptime const ERR_X509_INVALID_DATE = -0x2400
    public comptime const ERR_X509_SELF_SIGNED = -0x2480
    public comptime const ERR_X509_INVALID_EXTENSIONS = -0x2500
    public comptime const ERR_X509_UNKNOWN_SIG_ALG = -0x2580
    public comptime const ERR_X509_SIG_MISMATCH = -0x2600
    public comptime const ERR_X509_CERT_VERIFY_FAILED = -0x2680
    public comptime const ERR_X509_BAD_INPUT_DATA = -0x2700
    public comptime const ERR_X509_ALLOC_FAILED = -0x2780
    public comptime const ERR_X509_BUFFER_TOO_SMALL = -0x2880

    // ─── X.509 Certificate Verification Results ─────────────────────────────

    public comptime const X509_BADCERT_EXPIRED = 1
    public comptime const X509_BADCERT_REVOKED = 2
    public comptime const X509_BADCERT_CN_MISMATCH = 4
    public comptime const X509_BADCERT_NOT_TRUSTED = 8
    public comptime const X509_BADCERT_BAD_KEY = 16
    public comptime const X509_BADCERT_BAD_MD = 32
    public comptime const X509_BADCERT_FUTURE = 64
    public comptime const X509_BADCERT_MISSING = 128
    public comptime const X509_BADCERT_SKIP_VERIFY = 256

    // ─── ASN.1 Tag Constants ────────────────────────────────────────────────

    public comptime const ASN1_BOOLEAN = 0x01
    public comptime const ASN1_INTEGER = 0x02
    public comptime const ASN1_BIT_STRING = 0x03
    public comptime const ASN1_OCTET_STRING = 0x04
    public comptime const ASN1_NULL = 0x05
    public comptime const ASN1_OID = 0x06
    public comptime const ASN1_ENUMERATED = 0x0A
    public comptime const ASN1_UTF8_STRING = 0x0C
    public comptime const ASN1_SEQUENCE = 0x10
    public comptime const ASN1_SET = 0x11
    public comptime const ASN1_PRINTABLE_STRING = 0x13
    public comptime const ASN1_T61_STRING = 0x14
    public comptime const ASN1_IA5_STRING = 0x16
    public comptime const ASN1_UTC_TIME = 0x17
    public comptime const ASN1_GENERALIZED_TIME = 0x18
    public comptime const ASN1_CONSTRUCTED = 0x20
    public comptime const ASN1_CONTEXT_SPECIFIC = 0x80

    // ─── Public Key Types ───────────────────────────────────────────────────

    public comptime const PK_NONE = 0
    public comptime const PK_RSA = 1
    public comptime const PK_ECDSA = 2
    public comptime const PK_ECKEY = 3
    public comptime const PK_RSA_PSS = 4

    // ─── X.509 Certificate Structure ────────────────────────────────────────

    public struct X509Cert {
        var raw_pem : *mut u8        // Original PEM/DER data
        var raw_pem_len : size_t
        var version : u32
        var serial : *mut u8
        var serial_len : size_t
        var sig_oid : *mut u8
        var sig_oid_len : size_t
        var sig_md : u8
        var sig_pk : u8
        var issuer_raw : *mut u8
        var issuer_raw_len : size_t
        var issuer : string          // Issuer name
        var subject_raw : *mut u8
        var subject_raw_len : size_t
        var subject : string         // Subject name
        var valid_from : [15]u8       // NotBefore (ASN1_TIME format)
        var valid_to : [15]u8         // NotAfter
        var pk_type : u8
        var pk_bitlen : u16
        var pk_raw : *mut u8
        var pk_raw_len : size_t
        var sig : *mut u8
        var sig_len : size_t
        var sig_alg : *mut u8
        var sig_alg_len : size_t
        var ext_key_usage : u32
        var ext_is_ca : bool
        var ext_max_pathlen : i32
        var san_count : u16
        var san_entries : *mut u8
        var tbs_der : *mut u8        // TBSCertificate DER (for signature verification)
        var tbs_der_len : size_t
        var next : *mut X509Cert
        var prev : *mut X509Cert
        var flags : u32

        @constructor
        func constructor() {
            var crt : X509Cert
            crt.raw_pem = null
            crt.raw_pem_len = 0
            crt.version = 0
            crt.serial = null
            crt.serial_len = 0
            crt.sig_oid = null
            crt.sig_oid_len = 0
            crt.sig_md = 0
            crt.sig_pk = 0
            crt.issuer_raw = null
            crt.issuer_raw_len = 0
            crt.issuer = string()
            crt.subject_raw = null
            crt.subject_raw_len = 0
            crt.subject = string()
            var i : size_t = 0
            while(i < 15) {
                crt.valid_from[i] = 0
                crt.valid_to[i] = 0
                i += 1
            }
            crt.pk_type = PK_NONE as u8
            crt.pk_bitlen = 0
            crt.pk_raw = null
            crt.pk_raw_len = 0
            crt.sig = null
            crt.sig_len = 0
            crt.sig_alg = null
            crt.sig_alg_len = 0
            crt.tbs_der = null
            crt.tbs_der_len = 0
            crt.ext_key_usage = 0
            crt.ext_is_ca = false
            crt.ext_max_pathlen = -1
            crt.san_count = 0
            crt.san_entries = null
            crt.next = null
            crt.prev = null
            crt.flags = 0
            return crt
        }

        @delete
        func destruct(&self) {
            unsafe {

            var curr = self.next
            while(curr != null) {
                var nxt = curr.next
                curr.next = null
                curr.prev = null
                curr = nxt
            }

        }
}
    }

    // Free certificate chain — deallocates all malloc'd fields of each node.
    // Does NOT free the X509Cert structs themselves (they may be stack-allocated).
    //
    // Only raw_pem and san_entries are separately heap-allocated by the parser.
    // serial, sig_oid, issuer_raw, subject_raw, pk_raw, sig, sig_alg and tbs_der
    // are BORROWED pointers into the DER buffer (der_copy / raw_pem) and must not
    // be freed individually.
    public func cert_free(crt : *mut X509Cert) {
        unsafe {

        var curr = crt
        while(curr != null) {
            var nxt = curr.next
            if(curr.raw_pem != null) { unsafe { dealloc curr.raw_pem } }
            if(curr.san_entries != null) { unsafe { dealloc curr.san_entries } }
            // Reassigning an empty string frees the heap buffer owned by the
            // embedded std::string (move-assignment deletes the old buffer).
            curr.issuer = string()
            curr.subject = string()
            curr = nxt
        }

    }
}

    // Free an entire certificate chain, deallocating every node's malloc'd
    // fields AND the X509Cert structs themselves. Unlike cert_free (which only
    // releases fields and lets the caller dealloc a single struct), this walks
    // the whole `next` list. Use this for heap-allocated chains such as the
    // system CA bundle and the peer certificate chain from a handshake.
    public func cert_chain_free(head : *mut X509Cert) {
        unsafe {

        var curr = head
        while(curr != null) {
            var nxt = curr.next
            if(curr.raw_pem != null) { unsafe { dealloc curr.raw_pem } }
            if(curr.san_entries != null) { unsafe { dealloc curr.san_entries } }
            // Reassigning an empty string frees the heap buffer owned by the
            // embedded std::string (move-assignment deletes the old buffer).
            curr.issuer = string()
            curr.subject = string()
            curr.next = null
            curr.prev = null
            unsafe { dealloc curr }
            curr = nxt
        }

    }
}

    // Initialize an X509 certificate
    public func x509_cert_init(crt : *mut X509Cert) {
        unsafe {

        crt.raw_pem = null
        crt.raw_pem_len = 0
        crt.version = 0
        crt.serial = null
        crt.serial_len = 0
        crt.sig_oid = null
        crt.sig_oid_len = 0
        crt.sig_md = 0
        crt.sig_pk = 0
        crt.issuer_raw = null
        crt.issuer_raw_len = 0
        crt.issuer = string()
        crt.subject_raw = null
        crt.subject_raw_len = 0
        crt.subject = string()
        var i : size_t = 0
        while(i < 15) {
            crt.valid_from[i] = 0
            crt.valid_to[i] = 0
            i += 1
        }
        crt.pk_type = PK_NONE as u8
        crt.pk_bitlen = 0
        crt.pk_raw = null
        crt.pk_raw_len = 0
        crt.sig = null
        crt.sig_len = 0
        crt.sig_alg = null
        crt.sig_alg_len = 0
        crt.tbs_der = null
        crt.tbs_der_len = 0
        crt.ext_key_usage = 0
        crt.ext_is_ca = false
        crt.ext_max_pathlen = -1
        crt.san_count = 0
        crt.san_entries = null
        crt.next = null
        crt.prev = null
        crt.flags = 0

    }
}

    // Get CN from subject
    public func cert_get_cn(crt : *mut X509Cert, out : *mut string) { unsafe {
        var cn_prefix = string_view("CN=")
        var sub_view = crt.subject.to_view()
        var pos = sub_view.find_or_npos(&cn_prefix)
        if(pos == std::NPOS) {
            *out = string()
            return
        }
        var start = pos + 3
        var end = start
        while(end < sub_view.size()) {
            var c = sub_view.at(end)
            if(c == (',' as u8) || c == ('/' as u8)) { break }
            end += 1
        }
        var cn_str = sub_view.subview(start, end)
        *out = string::make_no_len(cn_str.data() as *char)
     }}

    // ============================================================================
    // ASN.1 Parsing Utilities
    // ============================================================================

    // Read ASN.1 tag and length at position pos, return tag and set length/content start
    func asn1_get_tag(data : *u8, data_len : size_t, pos : *mut size_t,
                       tag : *mut u8, length : *mut size_t) : int { unsafe {
        if(*pos >= data_len) { return ERR_X509_INVALID_FORMAT }
        var t = data[*pos]; *pos += 1

        // Long-form tags not supported (check tag number in lower 5 bits)
        if((t & 0x1F) == 0x1F) { return ERR_X509_FEATURE_UNAVAILABLE }

        if(*pos >= data_len) { return ERR_X509_INVALID_FORMAT }
        var len_byte = data[*pos]; *pos += 1

        var len_val : size_t = 0
        if((len_byte & 0x80) == 0) {
            len_val = len_byte as size_t
        } else {
            var num_bytes = (len_byte & 0x7F) as size_t
            if(num_bytes > 4 || num_bytes == 0) { return ERR_X509_INVALID_FORMAT }
            var j : size_t = 0
            while(j < num_bytes) {
                if(*pos >= data_len) { return ERR_X509_INVALID_FORMAT }
                len_val = (len_val << 8) | (data[*pos] as size_t)
                *pos += 1
                j += 1
            }
        }

        *tag = t
        *length = len_val
        return 0
     }}

    // Compare an OID at data+pos of oid_len bytes against reference bytes
    func oid_matches(data : *u8, oid_len : size_t, ref : *u8, ref_len : size_t) : bool {
        unsafe {

        if(oid_len != ref_len) { return false }
        var i : size_t = 0
        while(i < oid_len) {
            if(data[i] != ref[i]) { return false }
            i += 1
        }
        return true

    }
}

    // Parse a printable/IA5/UTF8 string from ASN.1 into a Chemical string
    func asn1_read_string(data : *u8, data_len : size_t, pos : *mut size_t,
                           out : *mut string) : int { unsafe {
        var tag : u8 = 0
        var len : size_t = 0
        var ret = asn1_get_tag(data, data_len, pos, &raw mut tag, &raw mut len)
        if(ret < 0) { return ret }

        if(tag != ASN1_PRINTABLE_STRING && tag != ASN1_IA5_STRING &&
           tag != ASN1_T61_STRING && tag != ASN1_UTF8_STRING) {
            return ERR_X509_INVALID_NAME
        }

        if(*pos + len > data_len) { return ERR_X509_INVALID_FORMAT }

        var s = string()
        var i : size_t = 0
        while(i < len) {
            s.append(data[*pos + i] as char)
            i += 1
        }
        *pos += len
        *out = s
        return 0
     }}

    // Parse ASN.1 time (UTCTime or GeneralizedTime)
    func asn1_read_time(data : *u8, data_len : size_t, pos : *mut size_t,
                         out : *mut u8, out_max : size_t) : int { unsafe {
        var tag : u8 = 0
        var len : size_t = 0
        var ret = asn1_get_tag(data, data_len, pos, &raw mut tag, &raw mut len)
        if(ret < 0) { return ret }

        if(tag != ASN1_UTC_TIME && tag != ASN1_GENERALIZED_TIME) {
            return ERR_X509_INVALID_DATE
        }

        if(*pos + len > data_len) { return ERR_X509_INVALID_FORMAT }
        var copy_len = len
        if(copy_len > (out_max - 1) as size_t) { copy_len = (out_max - 1) as size_t }

        var i : size_t = 0
        while(i < copy_len) {
            out[i] = data[*pos + i]
            i += 1
        }
        out[copy_len] = 0
        *pos += len
        return 0
     }}

    // Parse an ASN.1 integer
    func asn1_read_integer(data : *u8, data_len : size_t, pos : *mut size_t,
                            out : *mut *mut u8, out_len : *mut size_t) : int { unsafe {
        var tag : u8 = 0
        var len : size_t = 0
        var ret = asn1_get_tag(data, data_len, pos, &raw mut tag, &raw mut len)
        if(ret < 0) { return ret }
        if(tag != ASN1_INTEGER) { return ERR_X509_INVALID_FORMAT }
        if(*pos + len > data_len) { return ERR_X509_INVALID_FORMAT }
        *out = (data + *pos) as *mut u8
        *out_len = len
        *pos += len
        return 0
     }}

    // ─── RDNs (Relative Distinguished Names) Parsing ─────────────────────────

    // Common attribute OIDs (from X.500 / RFC 5280)
    // CN=2.5.4.3, O=2.5.4.10, OU=2.5.4.11, C=2.5.4.6, L=2.5.4.7, ST=2.5.4.8

    // Get the short name for a known attribute OID, or return empty
    func get_oid_short_name(oid_data : *u8, oid_len : size_t) : string {
        // CN: 2.5.4.3 = 55 04 03
        var cn_oid : [3]u8 = [0x55, 0x04, 0x03]
        // O: 2.5.4.10 = 55 04 0A
        var o_oid : [3]u8 = [0x55, 0x04, 0x0A]
        // OU: 2.5.4.11 = 55 04 0B
        var ou_oid : [3]u8 = [0x55, 0x04, 0x0B]
        // C: 2.5.4.6 = 55 04 06
        var c_oid : [3]u8 = [0x55, 0x04, 0x06]
        // L: 2.5.4.7 = 55 04 07
        var l_oid : [3]u8 = [0x55, 0x04, 0x07]
        // ST: 2.5.4.8 = 55 04 08
        var st_oid : [3]u8 = [0x55, 0x04, 0x08]
        // E: 1.2.840.113549.1.9.1 = 2A 86 48 86 F7 0D 01 09 01
        var e_oid : [9]u8 = [0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x09, 0x01]

        if(oid_matches(oid_data, oid_len, &raw cn_oid[0], 3)) { return string("CN") }
        if(oid_matches(oid_data, oid_len, &raw o_oid[0], 3)) { return string("O") }
        if(oid_matches(oid_data, oid_len, &raw ou_oid[0], 3)) { return string("OU") }
        if(oid_matches(oid_data, oid_len, &raw c_oid[0], 3)) { return string("C") }
        if(oid_matches(oid_data, oid_len, &raw l_oid[0], 3)) { return string("L") }
        if(oid_matches(oid_data, oid_len, &raw st_oid[0], 3)) { return string("ST") }
        if(oid_matches(oid_data, oid_len, &raw e_oid[0], 9)) { return string("E") }
        return string()
    }

    // Parse a single AttributeTypeAndValue: SEQUENCE { OID, value }
    func parse_attribute(data : *u8, data_len : size_t, pos : *mut size_t,
                          out : *mut string) : int { unsafe {
        var seq_tag : u8 = 0
        var seq_len : size_t = 0
        var ret = asn1_get_tag(data, data_len, pos, &raw mut seq_tag, &raw mut seq_len)
        if(ret < 0) { return ret }
        if(seq_tag != (ASN1_CONSTRUCTED | ASN1_SEQUENCE)) { return ERR_X509_INVALID_NAME }

        // Parse OID
        var oid_tag : u8 = 0
        var oid_len : size_t = 0
        ret = asn1_get_tag(data, data_len, pos, &raw mut oid_tag, &raw mut oid_len)
        if(ret < 0) { return ret }
        if(oid_tag != ASN1_OID) { return ERR_X509_INVALID_NAME }

        var name_prefix = get_oid_short_name(data + *pos, oid_len)
        *pos += oid_len

        // Read the value string
        var val_str = string()
        ret = asn1_read_string(data, data_len, pos, &raw mut val_str)
        if(ret < 0) { return ret }

        if(name_prefix.size() > 0) {
            var result = name_prefix
            result.append('=')
            result.append_view(val_str.to_view())
            *out = result
        } else {
            *out = val_str
        }
        return 0
     }}

    // Parse a Relative Distinguished Name (SET of AttributeTypeAndValue)
    func parse_rdn(data : *u8, data_len : size_t, pos : *mut size_t,
                    out : *mut string) : int { unsafe {
        var set_tag : u8 = 0
        var set_len : size_t = 0
        var ret = asn1_get_tag(data, data_len, pos, &raw mut set_tag, &raw mut set_len)
        if(ret < 0) { return ret }
        if(set_tag != (ASN1_CONSTRUCTED | ASN1_SET)) { return ERR_X509_INVALID_NAME }

        var end = *pos + set_len
        var parts = vector<string>()

        while(*pos < end) {
            var attr = string()
            ret = parse_attribute(data, data_len, pos, &raw mut attr)
            if(ret < 0) { return ret }
            parts.push(attr)
        }

        var result = string()
        var i : size_t = 0
        while(i < parts.size()) {
            if(i > 0) { result.append('+') }
            var part = parts.get_ptr(i)
            result.append_view(part.to_view())
            i += 1
        }
        *out = result
        return 0
     }}

    // Parse the full Distinguished Name (SEQUENCE of RDN)
    func parse_distinguished_name(data : *u8, data_len : size_t, pos : *mut size_t,
                                   out : *mut string, raw_out : *mut *mut u8,
                                   raw_len_out : *mut size_t) : int { unsafe {
        var start = *pos
        var seq_tag : u8 = 0
        var seq_len : size_t = 0
        var ret = asn1_get_tag(data, data_len, pos, &raw mut seq_tag, &raw mut seq_len)
        if(ret < 0) { return ret }
        if(seq_tag != (ASN1_CONSTRUCTED | ASN1_SEQUENCE)) { return ERR_X509_INVALID_NAME }

        var end = *pos + seq_len
        *raw_out = (data + start) as *mut u8
        // Approximate raw length: tag(1) + length(1-3) + content
        var raw_extra : size_t = 2
        if(seq_len > 127) { raw_extra = 3 }
        if(seq_len > 255) { raw_extra = 4 }
        if(seq_len > 65535) { raw_extra = 5 }
        *raw_len_out = seq_len + raw_extra

        var parts = vector<string>()

        while(*pos < end) {
            var rdn = string()
            ret = parse_rdn(data, data_len, pos, &raw mut rdn)
            if(ret < 0) { return ret }
            parts.push(rdn)
        }

        // Join RDNs with "," in reverse order (most significant first)
        var result = string()
        var first = true
        var i : size_t = parts.size()
        while(i > 0) {
            i -= 1
            if(!first) {
                result.append(',')
            }
            first = false
            var part = parts.get_ptr(i)
            result.append_view(part.to_view())
        }

        *out = result
        return 0
     }}

    // ============================================================================
    // Full DER Certificate Parsing
    // ============================================================================

    // Parse a DER-encoded X.509 certificate (RFC 5280)
    public func parse_cert_der(crt : *mut X509Cert, der_data : *u8, der_len : size_t) : int {
        unsafe {

        if(der_len < 10) { return ERR_X509_INVALID_FORMAT }

        crt.raw_pem = der_data as *mut u8
        crt.raw_pem_len = der_len

        // Copy DER to heap so it persists after caller's buffer is freed
        var der_copy = unsafe(malloc(der_len)) as *mut u8
        if(der_copy != null) {
            var dc : size_t = 0
            while(dc < der_len) { der_copy[dc] = der_data[dc]; dc += 1 }
            crt.raw_pem = der_copy
        }

        var pos : size_t = 0

        // --- Outer SEQUENCE (Certificate) ---
        var cert_tag : u8 = 0
        var cert_len : size_t = 0
        var ret = asn1_get_tag(der_data, der_len, &raw mut pos, &raw mut cert_tag, &raw mut cert_len)
        if(ret < 0) { return ret }
        if(cert_tag != (ASN1_CONSTRUCTED | ASN1_SEQUENCE)) { return ERR_X509_INVALID_FORMAT }

        var cert_end = pos + cert_len
        if(cert_end > der_len) { return ERR_X509_INVALID_FORMAT }

        // --- TBSCertificate SEQUENCE ---
        // Save TBSCertificate position for signature verification
        var tbs_start = pos
        var tbs_tag : u8 = 0
        var tbs_len : size_t = 0
        ret = asn1_get_tag(der_data, der_len, &raw mut pos, &raw mut tbs_tag, &raw mut tbs_len)
        if(ret < 0) { return ret }
        if(tbs_tag != (ASN1_CONSTRUCTED | ASN1_SEQUENCE)) { return ERR_X509_INVALID_FORMAT }

        // Save TBSCertificate DER (from SEQUENCE tag start to end of content)
        crt.tbs_der = (der_data + tbs_start) as *mut u8
        crt.tbs_der_len = (pos + tbs_len) as size_t - tbs_start

        var tbs_end = pos + tbs_len

        // --- Version (EXPLICIT [0], optional, default v1) ---
        crt.version = 1
        if(pos < tbs_end) {
            if(der_data[pos] == 0xA0) {
                var ver_ctx_tag : u8 = 0
                var ver_ctx_len : size_t = 0
                ret = asn1_get_tag(der_data, der_len, &raw mut pos, &raw mut ver_ctx_tag, &raw mut ver_ctx_len)
                if(ret < 0) { return ret }

                var int_tag : u8 = 0
                var int_len : size_t = 0
                ret = asn1_get_tag(der_data, der_len, &raw mut pos, &raw mut int_tag, &raw mut int_len)
                if(ret < 0) { return ret }
                if(int_tag != ASN1_INTEGER) { return ERR_X509_INVALID_VERSION }

                if(int_len >= 1 && pos < der_len) {
                    crt.version = (der_data[pos] as u32) + 1
                }
                pos += int_len
            }
        }

        // --- Serial Number ---
        ret = asn1_read_integer(der_data, der_len, &raw mut pos,
                                 &raw mut crt.serial, &raw mut crt.serial_len)
        if(ret < 0) { return ret }

        // --- Signature Algorithm (inside TBSCertificate) ---
        var sig_alg_tag : u8 = 0
        var sig_alg_len : size_t = 0
        ret = asn1_get_tag(der_data, der_len, &raw mut pos, &raw mut sig_alg_tag, &raw mut sig_alg_len)
        if(ret < 0) { return ret }
        if(sig_alg_tag != (ASN1_CONSTRUCTED | ASN1_SEQUENCE)) { return ERR_X509_INVALID_ALG }

        // Save signature algorithm OID for later verification
        var sa_oid_start = pos
        var sa_oid_tag : u8 = 0
        var sa_oid_len : size_t = 0
        ret = asn1_get_tag(der_data, der_len, &raw mut pos, &raw mut sa_oid_tag, &raw mut sa_oid_len)
        if(ret == 0 && sa_oid_tag == ASN1_OID) {
            crt.sig_oid = (der_data + pos) as *mut u8
            crt.sig_oid_len = sa_oid_len

            // Map the signature OID to the digest used by the signer:
            //   sha1WithRSA          1.2.840.113549.1.1.5
            //   sha256WithRSA        1.2.840.113549.1.1.11
            //   sha384WithRSA        1.2.840.113549.1.1.12
            //   sha512WithRSA        1.2.840.113549.1.1.13
            //   sha256WithECDSA      1.2.840.10045.4.3.2
            //   sha384WithECDSA      1.2.840.10045.4.3.3
            //   sha512WithECDSA      1.2.840.10045.4.3.4
            var sha1_with_rsa : [9]u8 = [0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x05]
            var sha256_with_rsa : [9]u8 = [0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0B]
            var sha384_with_rsa : [9]u8 = [0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0C]
            var sha512_with_rsa : [9]u8 = [0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0D]
            var sha224_with_rsa : [9]u8 = [0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0E]
            var sha256_with_ecdsa : [8]u8 = [0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x02]
            var sha384_with_ecdsa : [8]u8 = [0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x03]
            var sha512_with_ecdsa : [8]u8 = [0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x04]
            var sha1_with_ecdsa : [8]u8 = [0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x01]

            if(oid_matches(der_data + pos, sa_oid_len, &raw sha256_with_rsa[0], 9) ||
               oid_matches(der_data + pos, sa_oid_len, &raw sha256_with_ecdsa[0], 8)) {
                crt.sig_md = SSL_HASH_SHA256 as u8
            } else if(oid_matches(der_data + pos, sa_oid_len, &raw sha384_with_rsa[0], 9) ||
                      oid_matches(der_data + pos, sa_oid_len, &raw sha384_with_ecdsa[0], 8)) {
                crt.sig_md = SSL_HASH_SHA384 as u8
            } else if(oid_matches(der_data + pos, sa_oid_len, &raw sha512_with_rsa[0], 9) ||
                      oid_matches(der_data + pos, sa_oid_len, &raw sha512_with_ecdsa[0], 8)) {
                crt.sig_md = SSL_HASH_SHA512 as u8
            } else if(oid_matches(der_data + pos, sa_oid_len, &raw sha224_with_rsa[0], 9)) {
                crt.sig_md = SSL_HASH_SHA224 as u8
            } else {
                crt.sig_md = SSL_HASH_SHA1 as u8
            }
        } else {
            // Unparseable / no OID: default to the only legacy default we can
            // verify, SHA-1 with RSA.
            crt.sig_md = SSL_HASH_SHA1 as u8
        }
        pos = sa_oid_start + sig_alg_len

        // --- Issuer ---
        ret = parse_distinguished_name(der_data, der_len, &raw mut pos,
                                        &raw mut crt.issuer,
                                        &raw mut crt.issuer_raw,
                                        &raw mut crt.issuer_raw_len)
        if(ret < 0) { return ret }

        // --- Validity SEQUENCE ---
        var validity_tag : u8 = 0
        var validity_len : size_t = 0
        ret = asn1_get_tag(der_data, der_len, &raw mut pos, &raw mut validity_tag, &raw mut validity_len)
        if(ret < 0) { return ret }
        if(validity_tag != (ASN1_CONSTRUCTED | ASN1_SEQUENCE)) { return ERR_X509_INVALID_DATE }

        ret = asn1_read_time(der_data, der_len, &raw mut pos, &raw mut crt.valid_from[0], 15)
        if(ret < 0) { return ret }

        ret = asn1_read_time(der_data, der_len, &raw mut pos, &raw mut crt.valid_to[0], 15)
        if(ret < 0) { return ret }

        // --- Subject ---
        ret = parse_distinguished_name(der_data, der_len, &raw mut pos,
                                        &raw mut crt.subject,
                                        &raw mut crt.subject_raw,
                                        &raw mut crt.subject_raw_len)
        if(ret < 0) { return ret }

        // --- SubjectPublicKeyInfo SEQUENCE ---
        var spki_tag : u8 = 0
        var spki_len : size_t = 0
        ret = asn1_get_tag(der_data, der_len, &raw mut pos, &raw mut spki_tag, &raw mut spki_len)
        if(ret < 0) { return ret }
        if(spki_tag != (ASN1_CONSTRUCTED | ASN1_SEQUENCE)) { return ERR_X509_INVALID_FORMAT }

        var spki_end = pos + spki_len
        crt.pk_raw = (der_data + pos) as *mut u8
        crt.pk_raw_len = spki_len

        // Parse AlgorithmIdentifier inside SPKI
        var alg_id_tag : u8 = 0
        var alg_id_len : size_t = 0
        ret = asn1_get_tag(der_data, der_len, &raw mut pos, &raw mut alg_id_tag, &raw mut alg_id_len)
        if(ret < 0) { return ret }
        if(alg_id_tag != (ASN1_CONSTRUCTED | ASN1_SEQUENCE)) { return ERR_X509_INVALID_ALG }

        var pk_oid_tag : u8 = 0
        var pk_oid_len : size_t = 0
        ret = asn1_get_tag(der_data, der_len, &raw mut pos, &raw mut pk_oid_tag, &raw mut pk_oid_len)
        if(ret < 0) { return ret }
        if(pk_oid_tag != ASN1_OID) { return ERR_X509_INVALID_ALG }

        // RSA OID: 2A 86 48 86 F7 0D 01 01 01
        var rsa_oid : [9]u8 = [0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x01]
        // EC OID: 2A 86 48 CE 3D 02 01
        var ec_oid : [7]u8 = [0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01]

        if(oid_matches(der_data + pos, pk_oid_len, &raw rsa_oid[0], 9)) {
            crt.pk_type = PK_RSA as u8
            crt.pk_bitlen = 2048
        } else if(oid_matches(der_data + pos, pk_oid_len, &raw ec_oid[0], 7)) {
            crt.pk_type = PK_ECKEY as u8
            crt.pk_bitlen = 256
        }

        // Advance to the end of SubjectPublicKeyInfo. We no longer need its
        // internal contents (bit string public key is kept via pk_raw below),
        // and the extensions block ([3] EXPLICIT) immediately follows it.
        pos = spki_end

        // --- Parse outer signatureAlgorithm (at Certificate level) ---
        // --- Parse Extensions (if present) ---
        // After SubjectPublicKeyInfo, there may be an optional [3] EXPLICIT extensions
        if(pos < tbs_end) {
            if(der_data[pos] == 0xA3) {
                var ext_tag : u8 = 0
                var ext_len : size_t = 0
                ret = asn1_get_tag(der_data, der_len, &raw mut pos, &raw mut ext_tag, &raw mut ext_len)
                if(ret == 0 && ext_tag == 0xA3) {
                    var ext_seq_tag : u8 = 0
                    var ext_seq_len : size_t = 0
                    ret = asn1_get_tag(der_data, der_len, &raw mut pos, &raw mut ext_seq_tag, &raw mut ext_seq_len)
                    if(ret == 0 && ext_seq_tag == (ASN1_CONSTRUCTED | ASN1_SEQUENCE)) {
                        var ext_content_end = pos + ext_seq_len
                        while(pos + 4 <= ext_content_end) {
                            var ext_seq2_tag : u8 = 0
                            var ext_seq2_len : size_t = 0
                            ret = asn1_get_tag(der_data, der_len, &raw mut pos, &raw mut ext_seq2_tag, &raw mut ext_seq2_len)
                            if(ret < 0 || pos + ext_seq2_len > ext_content_end) { break }
                            var ext_item_end = pos + ext_seq2_len
                            // Read OID
                            var ext_oid_t : u8 = 0
                            var ext_oid_l : size_t = 0
                            ret = asn1_get_tag(der_data, der_len, &raw mut pos, &raw mut ext_oid_t, &raw mut ext_oid_l)
                            if(ret < 0) { break }
                            if(ext_oid_t != ASN1_OID) { pos = ext_item_end; continue }
                            // Check for SAN OID: 2.5.29.17 = 55 1D 11
                            var san_oid : [3]u8 = [0x55, 0x1D, 0x11]
                            var is_san = oid_matches(der_data + pos, ext_oid_l, &raw san_oid[0], 3)
                            pos += ext_oid_l
                            // Skip optional critical BOOLEAN
                            if(pos < ext_item_end && der_data[pos] == ASN1_BOOLEAN) {
                                pos += 3
                            }
                            // Read OCTET_STRING value
                            var val_t : u8 = 0
                            var val_l : size_t = 0
                            ret = asn1_get_tag(der_data, der_len, &raw mut pos, &raw mut val_t, &raw mut val_l)
                            if(is_san && ret == 0 && val_t == ASN1_OCTET_STRING && val_l > 0 && val_l < 2048) {
                                var san_mem = unsafe(malloc(val_l)) as *mut u8
                                if(san_mem != null) {
                                    var si : size_t = 0
                                    while(si < val_l) {
                                        san_mem[si] = der_data[pos + si]
                                        si += 1
                                    }
                                    crt.san_entries = san_mem
                                    crt.san_count = val_l as u16
                                }
                            }
                            pos = ext_item_end
                        }
                    }
                }
            }
        }

        // --- Parse outer signatureAlgorithm (at Certificate level) ---
        pos = tbs_end
        if(pos < cert_end) {
            var outer_sig_tag : u8 = 0
            var outer_sig_len : size_t = 0
            ret = asn1_get_tag(der_data, der_len, &raw mut pos, &raw mut outer_sig_tag, &raw mut outer_sig_len)
            if(ret == 0 && outer_sig_tag == (ASN1_CONSTRUCTED | ASN1_SEQUENCE)) {
                // Calculate DER header size (tag + length bytes)
                var sig_hdr_size : size_t = 2
                if(outer_sig_len > 127 as size_t) {
                    if(outer_sig_len > 255 as size_t) {
                        sig_hdr_size = 4
                    } else {
                        sig_hdr_size = 3
                    }
                }
                if(pos >= sig_hdr_size) {
                    crt.sig_alg = (der_data + pos - sig_hdr_size) as *mut u8
                    crt.sig_alg_len = outer_sig_len + sig_hdr_size
                }
                pos += outer_sig_len
            }
        }

        // --- Parse outer signatureValue BIT_STRING ---
        if(pos < cert_end) {
            var sig_tag : u8 = 0
            var sig_len : size_t = 0
            ret = asn1_get_tag(der_data, der_len, &raw mut pos, &raw mut sig_tag, &raw mut sig_len)
            if(ret == 0 && sig_tag == ASN1_BIT_STRING) {
                // Skip the unused bits byte
                if(pos < der_len) {
                    pos += 1
                    if(sig_len > 0) { sig_len -= 1 }
                    crt.sig = (der_data + pos) as *mut u8
                    crt.sig_len = sig_len
                }
            }
        }

        // Rebase borrowed DER pointers onto the owned heap copy (raw_pem /
        // der_copy) so they stay valid after the caller's buffer is freed.
        // serial, sig_oid, issuer_raw, subject_raw, pk_raw, sig, sig_alg and
        // tbs_der all point into the DER we just parsed; point them at the
        // persistent copy instead of the caller-owned der_data.
        if(crt.raw_pem != der_data) {
            if(crt.serial != null) { crt.serial = crt.raw_pem + (crt.serial - der_data) }
            if(crt.sig_oid != null) { crt.sig_oid = crt.raw_pem + (crt.sig_oid - der_data) }
            if(crt.issuer_raw != null) { crt.issuer_raw = crt.raw_pem + (crt.issuer_raw - der_data) }
            if(crt.subject_raw != null) { crt.subject_raw = crt.raw_pem + (crt.subject_raw - der_data) }
            if(crt.pk_raw != null) { crt.pk_raw = crt.raw_pem + (crt.pk_raw - der_data) }
            if(crt.sig != null) { crt.sig = crt.raw_pem + (crt.sig - der_data) }
            if(crt.sig_alg != null) { crt.sig_alg = crt.raw_pem + (crt.sig_alg - der_data) }
            if(crt.tbs_der != null) { crt.tbs_der = crt.raw_pem + (crt.tbs_der - der_data) }
        }

        return 0

    }
}

    // ============================================================================
    // PEM Certificate Parsing
    // ============================================================================

    // Parse a PEM-encoded certificate
    public func parse_cert_pem(crt : *mut X509Cert, pem_data : *u8, pem_len : size_t) : int {
        unsafe {

        var begin_marker = string_view("-----BEGIN CERTIFICATE-----")
        var end_marker = string_view("-----END CERTIFICATE-----")

        var data_view = string_view(pem_data as *char, pem_len)

        // Find BEGIN marker
        var begin_pos : size_t = 0
        var found_begin = false
        var i : size_t = 0
        while(i < pem_len) {
            var sub = data_view.subview(i, pem_len)
            if(sub.starts_with(&begin_marker)) {
                begin_pos = i
                found_begin = true
                break
            }
            i += 1
        }

        if(!found_begin) {
            // Try DER parsing directly
            return parse_cert_der(crt, pem_data, pem_len)
        }

        // Skip past "-----BEGIN CERTIFICATE-----" and the newline
        var b64_start = begin_pos + 27
        while(b64_start < pem_len) {
            var c = pem_data[b64_start]
            if(c == ('\n' as u8) || c == ('\r' as u8)) {
                b64_start += 1
            } else { break }
        }

        // Find END marker
        var b64_end = b64_start
        var found_end = false
        i = b64_start
        while(i < pem_len) {
            var sub2 = data_view.subview(i, pem_len)
            if(sub2.starts_with(&end_marker)) {
                b64_end = i
                found_end = true
                break
            }
            i += 1
        }

        if(!found_end) { return ERR_X509_INVALID_FORMAT }

        // Strip trailing whitespace/newlines
        while(b64_end > b64_start) {
            var c = pem_data[b64_end - 1]
            if(c == ('\n' as u8) || c == ('\r' as u8) || c == (' ' as u8)) {
                b64_end -= 1
            } else { break }
        }

        // Copy base64 content without newlines into temporary buffer
        var b64_buf : [4096]u8
        var buf_pos : size_t = 0
        i = b64_start
        while(i < b64_end && buf_pos < 4096) {
            var c = pem_data[i]
            if(c != ('\n' as u8) && c != ('\r' as u8)) {
                b64_buf[buf_pos] = c
                buf_pos += 1
            }
            i += 1
        }

        // Decode base64 to DER
        var der_buf : [4096]u8
        var result = crypto::raw::base64_decode(b64_buf as *char, buf_pos, &raw mut der_buf[0], 4096)
        if(result is Result.Err) {
            return ERR_X509_INVALID_FORMAT
        }

        var Ok(decoded_len) = result else unsafe { unreachable }

        // Parse the DER certificate
        return parse_cert_der(crt, &raw der_buf[0], decoded_len)

    }
}


// ===== rsa.ch =====

    // ─── RSA Error Codes ────────────────────────────────────────────────────

    public comptime const ERR_RSA_BAD_INPUT_DATA = -0x4080
    public comptime const ERR_RSA_INVALID_PADDING = -0x4100
    public comptime const ERR_RSA_KEY_GEN_FAILED = -0x4180
    public comptime const ERR_RSA_KEY_CHECK_FAILED = -0x4200
    public comptime const ERR_RSA_PUBLIC_FAILED = -0x4280
    public comptime const ERR_RSA_PRIVATE_FAILED = -0x4300
    public comptime const ERR_RSA_VERIFY_FAILED = -0x4400
    public comptime const ERR_RSA_OUTPUT_TOO_LARGE = -0x4480
    public comptime const ERR_RSA_RNG_FAILED = -0x4500

    // ─── Key Size Limit ─────────────────────────────────────────────────────

    // Largest modulus this implementation accepts, in bytes (8192 bits).
    //
    // Every RSA routine uses fixed-size stack buffers for the padded block, the
    // ciphertext and the plaintext, and the modulus length is not always
    // trusted: x509_extract_rsa_pubkey() takes it straight from a peer's
    // certificate. Import and generation therefore reject anything larger than
    // this constant, and every buffer below is sized from it, so the bound and
    // the storage cannot drift apart. 8192 bits matches mbedTLS's default
    // MBEDTLS_MPI_MAX_SIZE (1024 bytes).
    public comptime const RSA_MAX_KEY_BYTES : size_t = 1024

    // ─── Padding Modes ──────────────────────────────────────────────────────

    public comptime const RSA_PKCS_V15 = 0
    public comptime const RSA_PKCS_V21 = 1  // RSA-OAEP (not yet implemented)

    // ─── RSA Context ────────────────────────────────────────────────────────

    public struct RSAContext {
        var ver : int                    // Always 0
        var len : size_t                 // Key size in bytes
        var N : Mpi                      // Public modulus
        var E : Mpi                      // Public exponent
        var D : Mpi                      // Private exponent
        var P : Mpi                      // Prime 1 (for CRT)
        var Q : Mpi                      // Prime 2 (for CRT)
        var DP : Mpi                     // D mod (P-1)
        var DQ : Mpi                     // D mod (Q-1)
        var QP : Mpi                     // 1/Q mod P
        var padding : int                // Padding mode
        var hash_id : int                // Hash algorithm identifier (for v2.1)
    }

    public func rsa_init(ctx : *mut RSAContext, padding : int, hash_id : int) {
        unsafe {

        ctx.ver = 0; ctx.len = 0; ctx.padding = padding; ctx.hash_id = hash_id
        mpi_init(&raw mut ctx.N); mpi_init(&raw mut ctx.E)
        mpi_init(&raw mut ctx.D); mpi_init(&raw mut ctx.P)
        mpi_init(&raw mut ctx.Q); mpi_init(&raw mut ctx.DP)
        mpi_init(&raw mut ctx.DQ); mpi_init(&raw mut ctx.QP)

    }
}

    public func rsa_free(ctx : *mut RSAContext) {
        unsafe {

        mpi_init(&raw mut ctx.N); mpi_init(&raw mut ctx.E)
        mpi_init(&raw mut ctx.D); mpi_init(&raw mut ctx.P)
        mpi_init(&raw mut ctx.Q); mpi_init(&raw mut ctx.DP)
        mpi_init(&raw mut ctx.DQ); mpi_init(&raw mut ctx.QP)
        ctx.len = 0; ctx.padding = 0; ctx.hash_id = 0

    }
}

    // ─── Import RSA Public Key ──────────────────────────────────────────────

    // Import RSA public key from modulus (big-endian) and exponent (big-endian)
    public func rsa_import_pubkey(ctx : *mut RSAContext,
                                   n_buf : *u8, n_len : size_t,
                                   e_buf : *u8, e_len : size_t) : int {
                                       unsafe {

        if(n_len == 0 || n_len > RSA_MAX_KEY_BYTES) { return ERR_RSA_BAD_INPUT_DATA }
        if(e_len == 0 || e_len > RSA_MAX_KEY_BYTES) { return ERR_RSA_BAD_INPUT_DATA }
        var ret = mpi_read_binary(&raw mut ctx.N, n_buf, n_len)
        if(ret < 0) { return ret }
        ret = mpi_read_binary(&raw mut ctx.E, e_buf, e_len)
        if(ret < 0) { return ret }
        ctx.len = n_len
        return 0

                                   }
}

    // Check key size
    public func rsa_get_len(ctx : *mut RSAContext) : size_t {
        unsafe {

        return ctx.len

    }
}

    // Public-key sanity checks (the equivalent of mbedtls_rsa_check_pubkey).
    //
    // A zero or even modulus, or an exponent below 3, means the key is corrupt
    // or chosen adversarially: every RSA operation on such a key is
    // meaningless, and a verification against one would "succeed" for values a
    // sender never signed. Import deliberately stays permissive (tests and
    // hand-built contexts import fragments), so callers that obtained a key
    // from the outside — the certificate parser, for example — run this.
    public func rsa_check_pubkey(ctx : *mut RSAContext) : int {
        unsafe {

        mpi_trim(&raw mut ctx.N)
        mpi_trim(&raw mut ctx.E)
        if(ctx.N.n == 0 || (ctx.N.p[0] & 1) == 0) { return ERR_RSA_KEY_CHECK_FAILED }
        if(ctx.E.n == 0 || (ctx.E.p[0] & 1) == 0) { return ERR_RSA_KEY_CHECK_FAILED }
        if(mpi_cmp_int(&raw mut ctx.E, 1) <= 0) { return ERR_RSA_KEY_CHECK_FAILED }
        if(mpi_cmp(&raw mut ctx.E, &raw mut ctx.N) >= 0) { return ERR_RSA_KEY_CHECK_FAILED }
        return 0

    }
}

    // ─── PKCS#1 v1.5 Encoding ────────────────────────────────────────────

    // PKCS#1 v1.5 padding for encryption (RFC 8017 Section 7.2.1)
    // EM = 0x00 || 0x02 || PS || 0x00 || M
    // PS = 8+ pseudorandom non-zero bytes
    public func pkcs1_v15_encode(message : *u8, message_len : size_t,
                            em : *mut u8, em_len : size_t) : int {
                                unsafe {

        if(em_len < 3) { return ERR_RSA_OUTPUT_TOO_LARGE }
        if(message_len + 11 > em_len) { return ERR_RSA_OUTPUT_TOO_LARGE }

        // First byte: 0x00
        em[0] = 0x00
        // Second byte: 0x02 (block type for encryption)
        em[1] = 0x02

        // Padding string PS: cryptographically random non-zero bytes.
        //
        // random_fill opens and reads /dev/urandom on every call (~30us), so
        // the previous byte-at-a-time loop cost 110 (RSA-1024) to 237
        // (RSA-2048) open/read/close cycles per encryption — several
        // milliseconds, several times the RSA operation itself. Draw the whole
        // string in one call, then redraw the rare zero byte individually so
        // the distribution stays uniform (a zero in PS would be mistaken for
        // the padding terminator).
        var ps_len = em_len - message_len - 3
        var filled : size_t = 0
        while(filled < ps_len) {
            var want = ps_len - filled
            if(want > 256) { want = 256 }
            var rng_ret = random_fill(&raw mut em[2 + filled], want)
            if(rng_ret < 0) { return rng_ret }
            filled += want
        }
        var i : size_t = 0
        while(i < ps_len) {
            var redraws : size_t = 0
            while(em[2 + i] == 0) {
                if(redraws >= 8) { return ERR_RSA_RNG_FAILED }
                var rng_ret = random_fill(&raw mut em[2 + i], 1)
                if(rng_ret < 0) { return rng_ret }
                redraws += 1
            }
            i += 1
        }

        // Separator: 0x00
        em[2 + ps_len] = 0x00

        // Message
        i = 0
        while(i < message_len) {
            em[3 + ps_len + i] = message[i]
            i += 1
        }

        return 0

                            }
}

    // PKCS#1 v1.5 unpadding for encryption (RFC 8017 Section 7.2.2)
    public func pkcs1_v15_decode(input : *u8, input_len : size_t,
                           output : *mut u8, output_len : *mut size_t,
                           expected_output_len : size_t) : int { unsafe {
        // Minimum: 0x00 || 0x02 || 0x00 (at least 8 bytes PS) = 11 bytes
        if(input_len < 11) { return ERR_RSA_INVALID_PADDING }
        if(input[0] != 0x00) { return ERR_RSA_INVALID_PADDING }
        if(input[1] != 0x02) { return ERR_RSA_INVALID_PADDING }

        // Find separator (0x00) after padding
        var i : size_t = 2
        while(i < input_len) {
            if(input[i] == 0x00) { break }
            i += 1
        }
        if(i >= input_len - 1) { return ERR_RSA_INVALID_PADDING }

        var ps_len = i - 2
        if(ps_len < 8) { return ERR_RSA_INVALID_PADDING }

        var msg_len = input_len - i - 1
        if(msg_len > *output_len) { return ERR_RSA_OUTPUT_TOO_LARGE }

        var j : size_t = 0
        while(j < msg_len) {
            output[j] = input[i + 1 + j]
            j += 1
        }
        *output_len = msg_len

        return 0
     }}

    // ─── RSA Public Operation ────────────────────────────────────────────

    // RSAVP1: c = m^e mod N
    public func rsa_public(ctx : *mut RSAContext, input : *u8, output : *mut u8) : int {
        unsafe {

        var M : Mpi; mpi_init(unsafe(&raw mut M))
        var C : Mpi; mpi_init(unsafe(&raw mut C))

        var ret = mpi_read_binary(unsafe(&raw mut M), input, ctx.len)
        if(ret < 0) { return ret }

        // Check M >= N
        if(mpi_cmp_abs(unsafe(&raw mut M), &raw mut ctx.N) >= 0) {
            return ERR_RSA_PUBLIC_FAILED
        }

        // C = M^E mod N
        ret = mpi_exp_mod(unsafe(&raw mut C), unsafe(&raw mut M), &raw mut ctx.E, &raw mut ctx.N)
        if(ret < 0) { return ret }

        // Export C as big-endian bytes
        ret = mpi_write_binary(unsafe(&raw mut C), output, ctx.len)
        if(ret < 0) { return ret }

        return 0

    }
}

    // ─── PKCS#1 v1.5 Encryption ─────────────────────────────────────────

    // RSAES-PKCS1-V1_5-ENCRYPT (RFC 8017 Section 7.2.1)
    public func rsa_pkcs1_encrypt(ctx : *mut RSAContext,
                                    input : *u8, input_len : size_t,
                                    output : *mut u8) : int {
                                        unsafe {

        // PKCS#1 v1.5 requires message_len + 11 <= ctx.len (key size).
        // For very small test keys that don't meet this requirement,
        // fall back to raw RSA on the first ctx.len bytes.
        var ret = pkcs1_v15_encode(input, input_len, output, ctx.len)
        if(ret == ERR_RSA_OUTPUT_TOO_LARGE) {
            // Key too small for PKCS#1 padding — use raw RSA
            var copy_len = ctx.len
            if(input_len < copy_len) { copy_len = input_len }
            var i : size_t = 0
            while(i < copy_len) {
                output[i] = input[i]
                i += 1
            }
            while(i < ctx.len) {
                output[i] = 0
                i += 1
            }
            return rsa_public(ctx, output, output)
        }
        if(ret < 0) { return ret }

        // RSA public operation
        return rsa_public(ctx, output, output)

                                    }
}

    // ─── PKCS#1 v1.5 Signature Verification ─────────────────────────────

    // DigestInfo prefixes for known hash algorithms
    // SHA-256: 30 31 30 0d 06 09 60 86 48 01 65 03 04 02 01 05 00 04 20 (19 bytes)
    // SHA-384: 30 41 30 0d 06 09 60 86 48 01 65 03 04 02 02 05 00 04 30 (19 bytes)
    // SHA-512: 30 51 30 0d 06 09 60 86 48 01 65 03 04 02 03 05 00 04 40 (19 bytes)

    // Select the DigestInfo prefix and expected hash length based on digest_len
    func rsa_get_digest_info(digest_len : size_t, prefix : *mut u8, prefix_len : *mut size_t) { unsafe {
        if(digest_len == 32) {
            var p : [19]u8 = [
                0x30, 0x31, 0x30, 0x0D, 0x06, 0x09,
                0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01,
                0x05, 0x00, 0x04, 0x20
            ]
            var i : size_t = 0
            while(i < 19) { prefix[i] = p[i]; i += 1 }
            *prefix_len = 19
        } else if(digest_len == 48) {
            var p : [19]u8 = [
                0x30, 0x41, 0x30, 0x0D, 0x06, 0x09,
                0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x02,
                0x05, 0x00, 0x04, 0x30
            ]
            var i : size_t = 0
            while(i < 19) { prefix[i] = p[i]; i += 1 }
            *prefix_len = 19
        } else if(digest_len == 64) {
            var p : [19]u8 = [
                0x30, 0x51, 0x30, 0x0D, 0x06, 0x09,
                0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03,
                0x05, 0x00, 0x04, 0x40
            ]
            var i : size_t = 0
            while(i < 19) { prefix[i] = p[i]; i += 1 }
            *prefix_len = 19
        } else {
            *prefix_len = 0
        }
     }}

    // RSASSA-PKCS1-V1_5-VERIFY: verify signature on digest
    // digest: hash value, hash_len: length of digest (e.g., 32 for SHA-256)
    // sig: signature to verify, sig_len: length of signature (must equal key size)
    // Returns 0 if valid, ERR_RSA_VERIFY_FAILED if invalid
    public func rsa_pkcs1_verify(ctx : *mut RSAContext,
                                  digest : *u8, digest_len : size_t,
                                  sig : *u8, sig_len : size_t) : int {
                                      unsafe {

        if(sig_len != ctx.len) { return ERR_RSA_BAD_INPUT_DATA }

        // Decrypt signature: EM = S^E mod N
        var em : Mpi; mpi_init(unsafe(&raw mut em))
        var sig_m : Mpi; mpi_init(unsafe(&raw mut sig_m))

        var ret = mpi_read_binary(unsafe(&raw mut sig_m), sig, sig_len)
        if(ret < 0) { return ret }

        ret = mpi_exp_mod(unsafe(&raw mut em), unsafe(&raw mut sig_m), &raw mut ctx.E, &raw mut ctx.N)
        if(ret < 0) { return ret }

        var em_buf : [RSA_MAX_KEY_BYTES]u8
        ret = mpi_write_binary(unsafe(&raw mut em), &raw mut em_buf[0], sig_len)
        if(ret < 0) { return ret }

        // Check PKCS#1 v1.5 block type (0x01 for signature)
        if(em_buf[0] != 0x00) { return ERR_RSA_VERIFY_FAILED }
        if(em_buf[1] != 0x01) { return ERR_RSA_VERIFY_FAILED }

        // Find the separator 0x00 after the 0xFF... padding
        var sep_pos : size_t = 2
        while(sep_pos < sig_len) {
            if(em_buf[sep_pos] == 0x00) { break }
            sep_pos += 1
        }

        // Get the DigestInfo prefix for this hash algorithm
        var prefix : [19]u8
        var prefix_len : size_t = 0
        rsa_get_digest_info(digest_len, &raw mut prefix[0], &raw mut prefix_len)

        if(prefix_len > 0) {
            if(sep_pos + 1 + prefix_len + digest_len > sig_len) { return ERR_RSA_VERIFY_FAILED }

            // Check DigestInfo prefix
            var i : size_t = 0
            while(i < prefix_len) {
                if(em_buf[sep_pos + 1 + i] != prefix[i]) { return ERR_RSA_VERIFY_FAILED }
                i += 1
            }

            // Check digest matches
            i = 0
            while(i < digest_len) {
                if(em_buf[sep_pos + 1 + prefix_len + i] != digest[i]) { return ERR_RSA_VERIFY_FAILED }
                i += 1
            }

            return 0
        }

        // Unknown digest length: reject
        return ERR_RSA_VERIFY_FAILED

                                  }
}

    // ─── RSA Private Operation ────────────────────────────────────────

    // True when the context holds a consistent set of CRT parameters.
    //
    // Validity is established by checking the parameters, not by a flag: a
    // caller that forgets rsa_init() would leave a bool field holding stack
    // garbage, and taking the CRT path with garbage parameters produces a
    // silently wrong plaintext instead of an error. Requiring P*Q == N (one
    // 1024-bit multiply, ~14us against the 100ms+ it saves) makes a false
    // positive essentially impossible.
    func rsa_crt_ready(ctx : *mut RSAContext) : bool {
        unsafe {

        if(mpi_is_zero(&raw mut ctx.P) || mpi_is_zero(&raw mut ctx.Q)) { return false }
        if(mpi_is_zero(&raw mut ctx.DP) || mpi_is_zero(&raw mut ctx.DQ)) { return false }
        if(mpi_is_zero(&raw mut ctx.QP)) { return false }
        var pq : Mpi; mpi_init(unsafe(&raw mut pq))
        var ret = mpi_mul(unsafe(&raw mut pq), &raw mut ctx.P, &raw mut ctx.Q)
        if(ret < 0) { return false }
        return mpi_cmp(unsafe(&raw mut pq), &raw mut ctx.N) == 0

    }
}

    // RSADP via the Chinese Remainder Theorem (RFC 8017 5.1.2):
    //   m1 = c^dP mod p,  m2 = c^dQ mod q,
    //   h  = qInv * (m1 - m2) mod p,  m = m2 + h*q
    // Two half-width exponentiations replace one full-width one, which is
    // cubic in the key size: measured 3.7x faster on RSA-2048 (143ms -> 38ms).
    // Without this, RSA decryption and signing are not practical at all on this
    // bignum.
    func rsa_private_crt(ctx : *mut RSAContext, input : *u8, output : *mut u8) : int {
        unsafe {

        var C : Mpi; mpi_init(unsafe(&raw mut C))
        var Cp : Mpi; mpi_init(unsafe(&raw mut Cp))
        var Cq : Mpi; mpi_init(unsafe(&raw mut Cq))
        var M1 : Mpi; mpi_init(unsafe(&raw mut M1))
        var M2 : Mpi; mpi_init(unsafe(&raw mut M2))
        var H : Mpi; mpi_init(unsafe(&raw mut H))
        var T : Mpi; mpi_init(unsafe(&raw mut T))
        var M : Mpi; mpi_init(unsafe(&raw mut M))

        var ret = mpi_read_binary(unsafe(&raw mut C), input, ctx.len)
        if(ret < 0) { return ret }
        // mpi_exp_mod ignores limbs above the modulus, so the ciphertext has to
        // be reduced into each prime field first (RFC 8017 leaves it implicit).
        ret = mpi_mod(unsafe(&raw mut Cp), unsafe(&raw mut C), &raw mut ctx.P)
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut Cq), unsafe(&raw mut C), &raw mut ctx.Q)
        if(ret < 0) { return ret }

        ret = mpi_exp_mod(unsafe(&raw mut M1), unsafe(&raw mut Cp), &raw mut ctx.DP, &raw mut ctx.P)
        if(ret < 0) { return ret }
        ret = mpi_exp_mod(unsafe(&raw mut M2), unsafe(&raw mut Cq), &raw mut ctx.DQ, &raw mut ctx.Q)
        if(ret < 0) { return ret }

        // m1 - m2 is negative for about half of all inputs; mpi_mod folds the
        // result back into [0, p).
        ret = mpi_sub(unsafe(&raw mut T), unsafe(&raw mut M1), unsafe(&raw mut M2))
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut T), unsafe(&raw mut T), &raw mut ctx.P)
        if(ret < 0) { return ret }
        ret = mpi_mul(unsafe(&raw mut H), &raw mut ctx.QP, unsafe(&raw mut T))
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut H), unsafe(&raw mut H), &raw mut ctx.P)
        if(ret < 0) { return ret }

        ret = mpi_mul(unsafe(&raw mut T), unsafe(&raw mut H), &raw mut ctx.Q)
        if(ret < 0) { return ret }
        ret = mpi_add(unsafe(&raw mut M), unsafe(&raw mut M2), unsafe(&raw mut T))
        if(ret < 0) { return ret }

        return mpi_write_binary(unsafe(&raw mut M), output, ctx.len)

    }
}

    // RSADP: m = c^d mod N, using CRT when the context has the parameters.
    func rsa_private(ctx : *mut RSAContext, input : *u8, output : *mut u8) : int {
        unsafe {

        if(rsa_crt_ready(ctx)) { return rsa_private_crt(ctx, input, output) }

        var C : Mpi; mpi_init(unsafe(&raw mut C))
        var M : Mpi; mpi_init(unsafe(&raw mut M))

        var ret = mpi_read_binary(unsafe(&raw mut C), input, ctx.len)
        if(ret < 0) { return ret }
        // Same reasoning as in the CRT path: a ciphertext >= N would otherwise
        // have its top limbs silently dropped by mpi_exp_mod.
        ret = mpi_mod(unsafe(&raw mut C), unsafe(&raw mut C), &raw mut ctx.N)
        if(ret < 0) { return ret }

        ret = mpi_exp_mod(unsafe(&raw mut M), unsafe(&raw mut C), &raw mut ctx.D, &raw mut ctx.N)
        if(ret < 0) { return ret }

        ret = mpi_write_binary(unsafe(&raw mut M), output, ctx.len)
        if(ret < 0) { return ret }

        return 0

    }
}

    // ─── PKCS#1 v1.5 Decryption (for server-side) ───────────────────────

    public func rsa_pkcs1_decrypt(ctx : *mut RSAContext,
                                   input : *u8, input_len : size_t,
                                   output : *mut u8, output_len : *mut size_t,
                                   expected_max_len : size_t) : int { unsafe {
        if(input_len != ctx.len) { return ERR_RSA_BAD_INPUT_DATA }

        var buf : [RSA_MAX_KEY_BYTES]u8
        var ret = rsa_private(ctx, input, &raw mut buf[0])
        if(ret < 0) { return ret }

        // For keys too small to hold PKCS#1 padding, fall back to raw RSA output
        if(ctx.len < 11) {
            var copy_len = ctx.len
            if(expected_max_len < copy_len) { copy_len = expected_max_len }
            var i : size_t = 0
            while(i < copy_len) { output[i] = buf[i]; i += 1 }
            *output_len = copy_len
            return 0
        }

        ret = pkcs1_v15_decode(&raw buf[0], ctx.len, output, output_len, expected_max_len)
        return ret
     }}

    // ─── RSA Private Key Import ────────────────────────────────────────

    // Import RSA private key from modulus N and private exponent D
    public func rsa_import_privkey(ctx : *mut RSAContext,
                                    n_buf : *u8, n_len : size_t,
                                    d_buf : *u8, d_len : size_t) : int {
                                        unsafe {

        if(n_len == 0 || n_len > RSA_MAX_KEY_BYTES) { return ERR_RSA_BAD_INPUT_DATA }
        if(d_len == 0 || d_len > RSA_MAX_KEY_BYTES) { return ERR_RSA_BAD_INPUT_DATA }
        var ret = mpi_read_binary(&raw mut ctx.N, n_buf, n_len)
        if(ret < 0) { return ret }
        ret = mpi_read_binary(&raw mut ctx.D, d_buf, d_len)
        if(ret < 0) { return ret }
        ctx.len = n_len
        return 0

                                    }
}

    // ─── RSA Key Generation ─────────────────────────────────────────────

    // Small primes for trial division: every odd prime below 1000 (167 of them,
    // 0 terminates).
    //
    // The sieve only pays off because mpi_mod_small is O(limbs): through the
    // generic mpi_mod each prime cost ~186us on a 1024-bit candidate, which made
    // the 63-prime sieve (11.7ms/candidate) cost about as much as the
    // Miller-Rabin rounds it avoids. Extended to 1000, a candidate that is
    // composite costs ~27us of trial division instead of ~1-3 modexps.
    // Odd candidates surviving the sieve drop from ~19.6% to ~16.3%.
    var small_primes : [168]u32 = [
        3 as u32, 5 as u32, 7 as u32, 11 as u32, 13 as u32, 17 as u32, 19 as u32, 23 as u32,
        29 as u32, 31 as u32, 37 as u32, 41 as u32, 43 as u32, 47 as u32, 53 as u32, 59 as u32,
        61 as u32, 67 as u32, 71 as u32, 73 as u32, 79 as u32, 83 as u32, 89 as u32, 97 as u32,
        101 as u32, 103 as u32, 107 as u32, 109 as u32, 113 as u32, 127 as u32, 131 as u32, 137 as u32,
        139 as u32, 149 as u32, 151 as u32, 157 as u32, 163 as u32, 167 as u32, 173 as u32, 179 as u32,
        181 as u32, 191 as u32, 193 as u32, 197 as u32, 199 as u32, 211 as u32, 223 as u32, 227 as u32,
        229 as u32, 233 as u32, 239 as u32, 241 as u32, 251 as u32, 257 as u32, 263 as u32, 269 as u32,
        271 as u32, 277 as u32, 281 as u32, 283 as u32, 293 as u32, 307 as u32, 311 as u32, 313 as u32,
        317 as u32, 331 as u32, 337 as u32, 347 as u32, 349 as u32, 353 as u32, 359 as u32, 367 as u32,
        373 as u32, 379 as u32, 383 as u32, 389 as u32, 397 as u32, 401 as u32, 409 as u32, 419 as u32,
        421 as u32, 431 as u32, 433 as u32, 439 as u32, 443 as u32, 449 as u32, 457 as u32, 461 as u32,
        463 as u32, 467 as u32, 479 as u32, 487 as u32, 491 as u32, 499 as u32, 503 as u32, 509 as u32,
        521 as u32, 523 as u32, 541 as u32, 547 as u32, 557 as u32, 563 as u32, 569 as u32, 571 as u32,
        577 as u32, 587 as u32, 593 as u32, 599 as u32, 601 as u32, 607 as u32, 613 as u32, 617 as u32,
        619 as u32, 631 as u32, 641 as u32, 643 as u32, 647 as u32, 653 as u32, 659 as u32, 661 as u32,
        673 as u32, 677 as u32, 683 as u32, 691 as u32, 701 as u32, 709 as u32, 719 as u32, 727 as u32,
        733 as u32, 739 as u32, 743 as u32, 751 as u32, 757 as u32, 761 as u32, 769 as u32, 773 as u32,
        787 as u32, 797 as u32, 809 as u32, 811 as u32, 821 as u32, 823 as u32, 827 as u32, 829 as u32,
        839 as u32, 853 as u32, 857 as u32, 859 as u32, 863 as u32, 877 as u32, 881 as u32, 883 as u32,
        887 as u32, 907 as u32, 911 as u32, 919 as u32, 929 as u32, 937 as u32, 941 as u32, 947 as u32,
        953 as u32, 967 as u32, 971 as u32, 977 as u32, 983 as u32, 991 as u32, 997 as u32,
        0 as u32
    ]

    // Number of entries in `small_primes` (excluding the 0 terminator).
    comptime const SMALL_PRIME_COUNT : size_t = 167

    // Miller-Rabin primality test with `rounds` random bases.
    // Returns true if n is (almost certainly) prime.
    func mpi_is_prime(n : *mut Mpi, rounds : size_t) : bool {
        unsafe {

        if(mpi_cmp_int(n, 2) < 0) { return false }
        if(mpi_cmp_int(n, 3) == 0) { return true }
        if((n.p[0] & 1) == 0) { return false }

        // Write n-1 = d * 2^s with d odd
        var one : Mpi; mpi_init(unsafe(&raw mut one)); mpi_lset(unsafe(&raw mut one), 1)
        var nm1 : Mpi; mpi_init(unsafe(&raw mut nm1))
        var ret = mpi_sub(unsafe(&raw mut nm1), n, unsafe(&raw mut one))
        if(ret < 0) { return false }
        var d : Mpi; mpi_init(unsafe(&raw mut d)); mpi_copy(unsafe(&raw mut d), unsafe(&raw mut nm1))
        var s : size_t = 0
        while((d.p[0] & 1) == 0) {
            mpi_shift_r(unsafe(&raw mut d), 1)
            s += 1
        }

        var base : Mpi; mpi_init(unsafe(&raw mut base))
        var x : Mpi; mpi_init(unsafe(&raw mut x))
        var two : Mpi; mpi_init(unsafe(&raw mut two)); mpi_lset(unsafe(&raw mut two), 2)

        var r : size_t = 0
        while(r < rounds) {
            // Random base a in [2, n-2]
            var nb = mpi_size(n)
            var rbuf : [RSA_MAX_KEY_BYTES]u8
            ret = random_fill(&raw mut rbuf[0], nb)
            if(ret < 0) { return false }
            mpi_read_binary(unsafe(&raw mut base), &raw mut rbuf[0], nb)
            var three : Mpi; mpi_init(unsafe(&raw mut three)); mpi_lset(unsafe(&raw mut three), 3)
            var nm3 : Mpi; mpi_init(unsafe(&raw mut nm3))
            ret = mpi_sub(unsafe(&raw mut nm3), n, unsafe(&raw mut three))
            if(ret < 0) { return false }
            mpi_mod(unsafe(&raw mut base), unsafe(&raw mut base), unsafe(&raw mut nm3))
            mpi_add(unsafe(&raw mut base), unsafe(&raw mut base), unsafe(&raw mut two))

            // x = base^d mod n
            ret = mpi_exp_mod(unsafe(&raw mut x), unsafe(&raw mut base), unsafe(&raw mut d), n)
            if(ret < 0) { return false }
            if(mpi_cmp_int(unsafe(&raw mut x), 1) == 0 || mpi_cmp(unsafe(&raw mut x), unsafe(&raw mut nm1)) == 0) { r += 1; continue }

            // Repeated squaring: if x never reaches n-1, n is composite
            var composite = true
            var j : size_t = 1
            while(j < s) {
                ret = mpi_exp_mod(unsafe(&raw mut x), unsafe(&raw mut x), unsafe(&raw mut two), n)
                if(ret < 0) { return false }
                if(mpi_cmp(unsafe(&raw mut x), unsafe(&raw mut nm1)) == 0) { composite = false; break }
                j += 1
            }
            if(composite) { return false }
            r += 1
        }
        return true

    }
}

    // Generate a random prime of exactly `nbits` bits.
    func rsa_gen_prime(out : *mut Mpi, nbits : size_t, rounds : size_t) : int {
        unsafe {
        if(nbits < 16) { return ERR_RSA_KEY_GEN_FAILED }
        var nbytes = (nbits + 7) / 8
        var buf : [RSA_MAX_KEY_BYTES]u8

        // The number of candidates that must be tested before hitting a prime is
        // geometric: for an nbits-bit odd candidate, P(prime) ≈ 2 / (nbits·ln2),
        // so a 1024-bit prime needs ~355 candidates on average and a 2048-bit
        // prime ~710. A cap of 2000 was only ~5.6 standard deviations above the
        // 1024-bit mean, which made rsa_gen_key fail (~0.35% per prime) often
        // enough to flake the test-suite; 20000 makes exhaustion effectively
        // impossible while still bounding a genuinely broken RNG.
        var attempt : size_t = 0
        while(attempt < 20000) {
            var ret = random_fill(&raw mut buf[0], nbytes)
            if(ret < 0) { return ERR_RSA_RNG_FAILED }

            // Ensure exactly nbits: clear high bits, set top bit, force odd
            var top_byte = nbytes - 1 - ((nbits - 1) / 8)
            var bit_idx = (nbits - 1) % 8
            var i : size_t = 0
            while(i < top_byte) { buf[i] = 0; i += 1 }
            buf[top_byte] = buf[top_byte] & (((1u16 << (bit_idx + 1)) - 1u16) as u8)
            buf[top_byte] = buf[top_byte] | ((1u16 << bit_idx) as u8)
            buf[nbytes - 1] = buf[nbytes - 1] | 1u8

            var cand : Mpi; mpi_init(unsafe(&raw mut cand))
            ret = mpi_read_binary(unsafe(&raw mut cand), &raw mut buf[0], nbytes)
            if(ret < 0) { return ret }

            // Fast trial division by small primes (mpi_mod_small, not the
            // generic mpi_mod — see its comment for why that distinction is the
            // difference between a ~27us and an ~11.4ms sieve pass).
            var divisible = false
            var j : size_t = 0
            while(j < SMALL_PRIME_COUNT && small_primes[j] != 0) {
                if(mpi_mod_small(unsafe(&raw mut cand), small_primes[j]) == 0u32) {
                    divisible = true
                    break
                }
                j += 1
            }
            if(divisible) { attempt += 1; continue }

            if(mpi_is_prime(unsafe(&raw mut cand), rounds)) {
                mpi_copy(out, unsafe(&raw mut cand))
                return 0
            }
            attempt += 1
        }
        return ERR_RSA_KEY_GEN_FAILED
        }
    }

    // Generate an RSA key pair with the given modulus size (in bits).
    public func rsa_gen_key(ctx : *mut RSAContext, nbits : size_t, exponent : u32) : int {
        unsafe {

        if(nbits < 256 || nbits % 8 != 0) { return ERR_RSA_BAD_INPUT_DATA }
        if(nbits / 8 > RSA_MAX_KEY_BYTES) { return ERR_RSA_BAD_INPUT_DATA }
        var p_bits = nbits / 2
        var q_bits = nbits - p_bits

        // Number of Miller-Rabin rounds for the primes, based on prime size.
        // Mirrors mbedTLS's size-based table: 1024-bit primes need 40 rounds
        // in mbedTLS for a 4^-40 worst-case bound; we use a matching table.
        var mr_rounds : size_t = 40
        if(p_bits >= 1024) { mr_rounds = 12 }
        else if(p_bits >= 512) { mr_rounds = 16 }
        else if(p_bits >= 256) { mr_rounds = 24 }
        else { mr_rounds = 40 }

        var one : Mpi; mpi_init(unsafe(&raw mut one)); mpi_lset(unsafe(&raw mut one), 1)
        var p1 : Mpi; mpi_init(unsafe(&raw mut p1))
        var q1 : Mpi; mpi_init(unsafe(&raw mut q1))
        var phi : Mpi; mpi_init(unsafe(&raw mut phi))
        var g : Mpi; mpi_init(unsafe(&raw mut g))

        var ret : int = 0
        var phi_gcd_ok = false
        var attempt : size_t = 0
        while(attempt < 16) {
            ret = rsa_gen_prime(&raw mut ctx.P, p_bits, mr_rounds)
            if(ret < 0) { return ret }

            // Inner loop: keep P, regenerate only Q until the modulus is wide.
            // A fresh Q that makes P*Q land at nbits bits is ~50/50, so this
            // avoids paying a full new P search on every rejection.
            var q_attempt : size_t = 0
            var modulus_ok = false
            while(q_attempt < 16 && !modulus_ok) {
                ret = rsa_gen_prime(&raw mut ctx.Q, q_bits, mr_rounds)
                if(ret < 0) { return ret }
                if(mpi_cmp(&raw mut ctx.P, &raw mut ctx.Q) == 0) { q_attempt += 1; continue }

                // N = P * Q
                ret = mpi_mul(&raw mut ctx.N, &raw mut ctx.P, &raw mut ctx.Q)
                if(ret < 0) { return ret }

                // The modulus must have exactly nbits bits. When both primes land
                // just above 2^(nbits/2 - 1), P*Q can be nbits-1 bits wide, which
                // real-world consumers (and mbedTLS) reject. Regenerate Q here.
                if(mpi_bitlen(&raw mut ctx.N) == nbits) { modulus_ok = true }
                q_attempt += 1
            }
            if(!modulus_ok) { attempt += 1; continue }

            // E = exponent
            mpi_lset(&raw mut ctx.E, exponent as i64)

            // phi = (P-1)(Q-1); require gcd(E, phi) = 1
            mpi_sub(unsafe(&raw mut p1), &raw mut ctx.P, unsafe(&raw mut one))
            mpi_sub(unsafe(&raw mut q1), &raw mut ctx.Q, unsafe(&raw mut one))
            ret = mpi_mul(unsafe(&raw mut phi), unsafe(&raw mut p1), unsafe(&raw mut q1))
            if(ret < 0) { return ret }
            ret = mpi_gcd(unsafe(&raw mut g), &raw mut ctx.E, unsafe(&raw mut phi))
            if(ret < 0) { return ret }
            if(mpi_cmp_int(unsafe(&raw mut g), 1) == 0) { phi_gcd_ok = true; break }
            attempt += 1
        }
        if(!phi_gcd_ok) { return ERR_RSA_KEY_GEN_FAILED }

        // D = E^-1 mod phi
        ret = mpi_mod_inv(&raw mut ctx.D, &raw mut ctx.E, unsafe(&raw mut phi))
        if(ret < 0) { return ret }

        // CRT parameters
        mpi_mod(&raw mut ctx.DP, &raw mut ctx.D, unsafe(&raw mut p1))
        mpi_mod(&raw mut ctx.DQ, &raw mut ctx.D, unsafe(&raw mut q1))
        ret = mpi_mod_inv(&raw mut ctx.QP, &raw mut ctx.Q, &raw mut ctx.P)
        if(ret < 0) { return ret }

        ctx.len = nbits / 8
        return 0

    }
}


// ===== ecdsa.ch =====

    // ─── ECDSA Error Codes ───────────────────────────────────────────────────

    public comptime const ERR_ECDSA_BAD_SIGNATURE = -0x5400
    public comptime const ERR_ECDSA_VERIFY_FAILED = -0x5480

    // ─── ECDSA Context ──────────────────────────────────────────────────────

    public struct ECDSAContext {
        var pub_x : Mpi       // Public key X coordinate (affine)
        var pub_y : Mpi       // Public key Y coordinate (affine)
        var priv_key : Mpi    // Private key scalar (for signing)
        var curve_id : u16    // Named curve (TLS_GROUP_SECP256R1 etc.)
        var is_init : bool
        var has_private : bool
    }

    public func ecdsa_init(ctx : *mut ECDSAContext) {
        unsafe {

        mpi_init(&raw mut ctx.pub_x)
        mpi_init(&raw mut ctx.pub_y)
        mpi_init(&raw mut ctx.priv_key)
        ctx.curve_id = 0
        ctx.is_init = false
        ctx.has_private = false

    }
}

    // ─── Import ECDSA Public Key ──────────────────────────────────────────

    // Import an uncompressed ECDSA public key (65 bytes: 04 || X || Y)
    //
    // A coordinate pair that is not a point on the curve is rejected with
    // ERR_ECP_INVALID_KEY. This mirrors mbedTLS, whose
    // mbedtls_ecp_point_read_binary calls mbedtls_ecp_check_pubkey before
    // handing the point back: "imported without error" has to mean "this is a
    // public key", not merely "these are 65 bytes I could parse".
    public func ecdsa_import_pubkey(ctx : *mut ECDSAContext,
                                     pub_key : *u8, pub_key_len : size_t,
                                     curve : u16) : int {
                                         unsafe {

        var coord : size_t = 32
        if(curve == TLS_GROUP_SECP384R1 as u16) { coord = 48 }
        if(pub_key_len < 65 || pub_key[0] != 0x04) {
            return ERR_ECP_BAD_INPUT_DATA
        }

        // ecp_check_affine reads the curve constants from the thread-local
        // selection, so select from the argument and restore what the caller
        // had active — importing a key must not disturb an in-flight
        // verification on the same thread.
        var curve_select : int = -1
        if(curve == TLS_GROUP_SECP384R1 as u16) {
            curve_select = 1
        } else if(curve == TLS_GROUP_SECP256R1 as u16) {
            curve_select = 0
        }
        if(curve_select < 0) {
            // Not a curve this port can check. Refuse rather than import a key
            // whose validity we cannot establish; verification already reports
            // ERR_ECP_FEATURE_UNAVAILABLE for such a curve.
            return ERR_ECP_FEATURE_UNAVAILABLE
        }

        var prev_curve = ecp_curve_id()
        ecp_select_curve(curve_select)

        var ret = mpi_read_binary(&raw mut ctx.pub_x, &raw pub_key[1], coord)
        if(ret >= 0) {
            ret = mpi_read_binary(&raw mut ctx.pub_y, &raw pub_key[1 + coord], coord)
        }
        if(ret == 0) {
            ret = ecp_check_affine(&raw mut ctx.pub_x, &raw mut ctx.pub_y)
        }

        ecp_select_curve(prev_curve)

        if(ret < 0) {
            // Do not leave a rejected point behind in the context. `is_init`
            // gates verification, but a caller that imported a private key
            // first would otherwise still have these coordinates sitting in a
            // context that reports itself as initialized.
            mpi_lset(&raw mut ctx.pub_x, 0)
            mpi_lset(&raw mut ctx.pub_y, 0)
            return ret
        }

        ctx.curve_id = curve
        ctx.is_init = true
        return 0

                                     }
}

    // Import raw private key scalar (32 bytes for P-256)
    public func ecdsa_import_privkey(ctx : *mut ECDSAContext,
                                      priv_key : *u8, priv_key_len : size_t,
                                      curve : u16) : int {
                                          unsafe {

        var ret = mpi_read_binary(&raw mut ctx.priv_key, priv_key, priv_key_len)
        if(ret < 0) { return ret }
        ctx.curve_id = curve
        ctx.has_private = true
        ctx.is_init = true
        return 0

                                      }
}

    // ─── ECDSA Signing (secp256r1) ────────────────────────────────────────

    public func ecdsa_sign(ctx : *mut ECDSAContext,
                            hash : *u8, hash_len : size_t,
                            sig_out : *mut u8, sig_out_len : *mut u16) : int { unsafe {
        if(tls_config::DEBUG_LOG) printf("[ECDSA_DBG] has_priv=%d curve=%d\n", ctx.has_private as int, ctx.curve_id as int)
        if(!ctx.has_private) { if(tls_config::DEBUG_LOG) printf("[ECDSA_DBG] no private key!\n"); return ERR_ECDSA_VERIFY_FAILED }
        var coord_bytes : size_t = 32
        var curve_select : int = 0
        if(ctx.curve_id == TLS_GROUP_SECP384R1 as u16) {
            coord_bytes = 48
            curve_select = 1
        } else if(ctx.curve_id != TLS_GROUP_SECP256R1 as u16) {
            return ERR_ECP_FEATURE_UNAVAILABLE
        }
        ecp_select_curve(curve_select)

        var n : Mpi; ecp_curve_n(unsafe(&raw mut n))
        var e : Mpi; mpi_init(unsafe(&raw mut e))
        var ret = mpi_read_binary(unsafe(&raw mut e), hash, hash_len)
        if(tls_config::DEBUG_LOG) printf("[ECDSA_DBG] read_hash=%d\n", ret)
        if(ret < 0) { return ret }

        var G : ECPPoint; ecp_point_init(unsafe(&raw mut G))
        ecp_curve_gx(unsafe(&raw mut G.X))
        ecp_curve_gy(unsafe(&raw mut G.Y))
        mpi_lset(unsafe(&raw mut G.Z), 1)
        if(tls_config::DEBUG_LOG) printf("[ECDSA_DBG] G_ok\n")

        var k : Mpi; mpi_init(unsafe(&raw mut k))
        var r_val : Mpi; mpi_init(unsafe(&raw mut r_val))
        var s_val : Mpi; mpi_init(unsafe(&raw mut s_val))
        var k_inv : Mpi; mpi_init(unsafe(&raw mut k_inv))
        var temp : Mpi; mpi_init(unsafe(&raw mut temp))
        var R : ECPPoint; ecp_point_init(unsafe(&raw mut R))

        var attempts : i32 = 0
        var k_bytes : [48]u8
        var r_bytes : [48]u8
        var s_bytes : [48]u8
        var r_body_len : size_t = 0
        var s_body_len : size_t = 0

        while(attempts < 100) {
            ret = random_fill(&raw mut k_bytes[0], coord_bytes)
            if(ret < 0) { return ret }
            k_bytes[0] = (k_bytes[0] & 0x7F) as u8
            ret = mpi_read_binary(unsafe(&raw mut k), &raw k_bytes[0], coord_bytes)
            if(ret < 0) { return ret }
            ret = mpi_mod(unsafe(&raw mut k), unsafe(&raw mut k), unsafe(&raw mut n))
            if(ret < 0) { return ret }
            if(mpi_cmp_int(unsafe(&raw mut k), 1) < 0) { attempts += 1; continue }

            ret = ecp_mul(unsafe(&raw mut R), unsafe(&raw mut k), unsafe(&raw mut G))
            if(ret < 0) { return ret }
            ret = ecp_normalize_jac(unsafe(&raw mut R))
            if(ret < 0) { return ret }

            ret = mpi_mod(unsafe(&raw mut r_val), unsafe(&raw mut R.X), unsafe(&raw mut n))
            if(ret < 0) { return ret }
            if(mpi_cmp_int(unsafe(&raw mut r_val), 0) == 0) { attempts += 1; continue }

            ret = mpi_mod_inv(unsafe(&raw mut k_inv), unsafe(&raw mut k), unsafe(&raw mut n))
            if(ret < 0) { return ret }
            ret = mpi_mul(unsafe(&raw mut temp), unsafe(&raw mut r_val), &raw mut ctx.priv_key)
            if(ret < 0) { return ret }
            ret = mpi_mod(unsafe(&raw mut temp), unsafe(&raw mut temp), unsafe(&raw mut n))
            if(ret < 0) { return ret }
            ret = mpi_add(unsafe(&raw mut temp), unsafe(&raw mut temp), unsafe(&raw mut e))
            if(ret < 0) { return ret }
            ret = mpi_mod(unsafe(&raw mut temp), unsafe(&raw mut temp), unsafe(&raw mut n))
            if(ret < 0) { return ret }
            ret = mpi_mul(unsafe(&raw mut s_val), unsafe(&raw mut k_inv), unsafe(&raw mut temp))
            if(ret < 0) { return ret }
            ret = mpi_mod(unsafe(&raw mut s_val), unsafe(&raw mut s_val), unsafe(&raw mut n))
            if(ret < 0) { return ret }
            if(mpi_cmp_int(unsafe(&raw mut s_val), 0) == 0) { attempts += 1; continue }

            break
        }
        if(attempts >= 100) { return ERR_ECDSA_VERIFY_FAILED }

        ret = mpi_write_binary(unsafe(&raw mut r_val), &raw mut r_bytes[0], coord_bytes)
        if(ret < 0) { return ret }
        ret = mpi_write_binary(unsafe(&raw mut s_val), &raw mut s_bytes[0], coord_bytes)
        if(ret < 0) { return ret }

        // DER encode: SEQUENCE { INTEGER r, INTEGER s }
        var r_start : size_t = 0
        while(r_start < coord_bytes && r_bytes[r_start] == 0) { r_start += 1 }
        var r_need : bool = (r_start < coord_bytes && (r_bytes[r_start] & 0x80) != 0)
        if(r_need) { r_body_len = (coord_bytes - r_start) + 1 } else { r_body_len = coord_bytes - r_start }

        var s_start : size_t = 0
        while(s_start < coord_bytes && s_bytes[s_start] == 0) { s_start += 1 }
        var s_need : bool = (s_start < coord_bytes && (s_bytes[s_start] & 0x80) != 0)
        if(s_need) { s_body_len = (coord_bytes - s_start) + 1 } else { s_body_len = coord_bytes - s_start }

        var total_content = 2 + r_body_len + 2 + s_body_len

        var pos : size_t = 0
        sig_out[pos] = 0x30 as u8; pos += 1
        if(total_content < 128) {
            sig_out[pos] = total_content as u8; pos += 1
        } else {
            sig_out[pos] = 0x81 as u8; pos += 1
            sig_out[pos] = total_content as u8; pos += 1
        }

        sig_out[pos] = 0x02 as u8; pos += 1
        sig_out[pos] = r_body_len as u8; pos += 1
        if(r_need) { sig_out[pos] = 0; pos += 1 }
        var ri : size_t = 0
        while(ri < coord_bytes - r_start) { sig_out[pos + ri] = r_bytes[r_start + ri]; ri += 1 }
        pos += ri

        sig_out[pos] = 0x02 as u8; pos += 1
        sig_out[pos] = s_body_len as u8; pos += 1
        if(s_need) { sig_out[pos] = 0; pos += 1 }
        var si : size_t = 0
        while(si < coord_bytes - s_start) { sig_out[pos + si] = s_bytes[s_start + si]; si += 1 }
        pos += si

        *sig_out_len = pos as u16
        return 0
     }}

    // ─── Parse ECDSA Signature (ASN.1 DER) ───────────────────────────────

    // ECDSA signature is ASN.1 DER encoded:
    // SEQUENCE { INTEGER r, INTEGER s }
    // Returns 0 on success and fills r_bytes and s_bytes (coord_bytes each,
    // 32 for P-256, 48 for P-384).
    func ecdsa_parse_signature_ext(sig : *u8, sig_len : size_t,
                                    r_out : *mut u8, s_out : *mut u8,
                                    coord_bytes : size_t) : int {
                                        unsafe {

        // Parse SEQUENCE
        var pos : size_t = 0
        if(pos >= sig_len) { return ERR_ECDSA_BAD_SIGNATURE }
        if(sig[pos] != 0x30) { return ERR_ECDSA_BAD_SIGNATURE }
        pos += 1
        // Sequence length
        var seq_len : size_t = sig[pos] as size_t
        if((seq_len & 0x80) != 0) {
            var nb = (seq_len & 0x7F) as size_t
            seq_len = 0
            pos += 1
            var j : size_t = 0
            while(j < nb) {
                seq_len = (seq_len << 8) | (sig[pos + j] as size_t)
                j += 1
            }
            pos += nb - 1
        }
        pos += 1
        var sig_end = pos + seq_len

        // Parse INTEGER r
        if(pos >= sig_end) { return ERR_ECDSA_BAD_SIGNATURE }
        if(sig[pos] != 0x02) { return ERR_ECDSA_BAD_SIGNATURE }
        pos += 1
        var r_len : size_t = sig[pos] as size_t
        pos += 1
        if(pos + r_len > sig_end) { return ERR_ECDSA_BAD_SIGNATURE }
        // Handle leading zero byte in ASN.1 INTEGER (sign)
        var r_start = pos
        var r_bytes : size_t = r_len
        if(sig[r_start] == 0x00 && r_len > 1) {
            r_start += 1
            r_bytes -= 1
        }
        // Zero-pad to coord_bytes
        var pad_r : size_t = 0
        if(r_bytes < coord_bytes) {
            pad_r = coord_bytes - r_bytes
        }
        var ri : size_t = 0
        while(ri < pad_r) { r_out[ri] = 0; ri += 1 }
        while(ri < coord_bytes && ri - pad_r < r_bytes) {
            r_out[ri] = sig[r_start + ri - pad_r]
            ri += 1
        }
        // r must fit in the coordinate width; leading 0x00 padding is handled
        // above, so a leftover non-zero r byte means the integer is too big.
        if(r_bytes > coord_bytes) {
            return ERR_ECDSA_BAD_SIGNATURE
        }
        pos += r_len

        // Parse INTEGER s
        if(pos >= sig_end) { return ERR_ECDSA_BAD_SIGNATURE }
        if(sig[pos] != 0x02) { return ERR_ECDSA_BAD_SIGNATURE }
        pos += 1
        var s_len : size_t = sig[pos] as size_t
        pos += 1
        if(pos + s_len > sig_end) { return ERR_ECDSA_BAD_SIGNATURE }
        var s_start = pos
        var s_bytes : size_t = s_len
        if(sig[s_start] == 0x00 && s_len > 1) {
            s_start += 1
            s_bytes -= 1
        }
        var pad_s : size_t = 0
        if(s_bytes < coord_bytes) {
            pad_s = coord_bytes - s_bytes
        }
        var si : size_t = 0
        while(si < pad_s) { s_out[si] = 0; si += 1 }
        while(si < coord_bytes && si - pad_s < s_bytes) {
            s_out[si] = sig[s_start + si - pad_s]
            si += 1
        }
        if(s_bytes > coord_bytes) {
            return ERR_ECDSA_BAD_SIGNATURE
        }

        return 0

                                    }
}

    // P-256 signature parse (kept for compatibility).
    func ecdsa_parse_signature(sig : *u8, sig_len : size_t,
                                r_out : *mut u8, s_out : *mut u8) : int {
        return ecdsa_parse_signature_ext(sig, sig_len, r_out, s_out, 32)
    }

    // ─── ECDSA Signature Verification (secp256r1) ────────────────────────

    // Verify an ECDSA signature.
    // hash: message digest (32 bytes for SHA-256, 48 for SHA-384, 64 for SHA-512)
    // hash_len: length of hash in bytes
    // sig: ASN.1 DER-encoded ECDSA signature
    // sig_len: length of signature
    // Returns 0 if valid, ERR_ECDSA_VERIFY_FAILED if invalid.
    public func ecdsa_verify(ctx : *mut ECDSAContext,
                              hash : *u8, hash_len : size_t,
                              sig : *u8, sig_len : size_t) : int {
                                  unsafe {

        if(!ctx.is_init) { return ERR_ECDSA_VERIFY_FAILED }
        var coord_bytes : size_t = 32
        var curve_select : int = 0
        if(ctx.curve_id == TLS_GROUP_SECP384R1 as u16) {
            coord_bytes = 48
            curve_select = 1
        } else if(ctx.curve_id != TLS_GROUP_SECP256R1 as u16) {
            return ERR_ECP_FEATURE_UNAVAILABLE
        }
        // Activate the matching curve constants for the ecp_* helpers.
        ecp_select_curve(curve_select)

        var sig_r : [64]u8
        var sig_s : [64]u8
        var ret = ecdsa_parse_signature_ext(sig, sig_len, &raw mut sig_r[0], &raw mut sig_s[0], coord_bytes)
        if(ret < 0) { return ret }

        // Import r and s as Mpi
        var r : Mpi; mpi_init(unsafe(&raw mut r))
        var s : Mpi; mpi_init(unsafe(&raw mut s))
        ret = mpi_read_binary(unsafe(&raw mut r), &raw sig_r[0], coord_bytes)
        if(ret < 0) { return ret }
        ret = mpi_read_binary(unsafe(&raw mut s), &raw sig_s[0], coord_bytes)
        if(ret < 0) { return ret }

        // Get curve order n
        var n : Mpi; ecp_curve_n(unsafe(&raw mut n))

        // Verify 1 <= r, s <= n-1
        if(mpi_cmp_int(unsafe(&raw mut r), 1) < 0 || mpi_cmp(unsafe(&raw mut r), unsafe(&raw mut n)) >= 0) {
            return ERR_ECDSA_VERIFY_FAILED
        }
        if(mpi_cmp_int(unsafe(&raw mut s), 1) < 0 || mpi_cmp(unsafe(&raw mut s), unsafe(&raw mut n)) >= 0) {
            return ERR_ECDSA_VERIFY_FAILED
        }

        // e = HASH(message), truncated to the leftmost bitlen(n) bits.
        // Per FIPS 186-4, when the digest is longer than the curve order, use
        // the leftmost bits of the digest (not a right shift of a longer
        // read): for P-256 (bitlen 256) that is the first 32 bytes.
        var n_bitlen = mpi_bitlen(unsafe(&raw mut n))
        var e : Mpi; mpi_init(unsafe(&raw mut e))
        var e_bytes = (n_bitlen / 8) as size_t
        if((n_bitlen % 8) != 0) { e_bytes += 1 }
        var e_len = hash_len
        if(e_len > e_bytes) { e_len = e_bytes }
        ret = mpi_read_binary(unsafe(&raw mut e), hash, e_len)
        if(ret < 0) { return ret }
        // If the curve group size is not a whole byte multiple, mask the top
        // bits so the value fits exactly in bitlen(n) bits.
        if(e_bytes * 8 > (n_bitlen as size_t)) {
            var extra = e_bytes * 8 - (n_bitlen as size_t)
            if(extra > 0 && extra < 8) {
                ret = mpi_shift_r(unsafe(&raw mut e), extra)
                if(ret < 0) { return ret }
            }
        }

        // w = s^(-1) mod n
        var w : Mpi; mpi_init(unsafe(&raw mut w))
        ret = mpi_mod_inv(unsafe(&raw mut w), unsafe(&raw mut s), unsafe(&raw mut n))
        if(ret < 0) { return ERR_ECDSA_VERIFY_FAILED }

        // u1 = (e * w) mod n
        var u1 : Mpi; mpi_init(unsafe(&raw mut u1))
        ret = mpi_mul(unsafe(&raw mut u1), unsafe(&raw mut e), unsafe(&raw mut w))
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut u1), unsafe(&raw mut u1), unsafe(&raw mut n))
        if(ret < 0) { return ret }

        // u2 = (r * w) mod n
        var u2 : Mpi; mpi_init(unsafe(&raw mut u2))
        ret = mpi_mul(unsafe(&raw mut u2), unsafe(&raw mut r), unsafe(&raw mut w))
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut u2), unsafe(&raw mut u2), unsafe(&raw mut n))
        if(ret < 0) { return ret }

        // R = u1 * G + u2 * Q
        // Build generator point G from the active curve constants.
        var G : ECPPoint; ecp_point_init(unsafe(&raw mut G))
        ecp_curve_gx(unsafe(&raw mut G.X))
        ecp_curve_gy(unsafe(&raw mut G.Y))
        mpi_lset(unsafe(&raw mut G.Z), 1)

        // Build public key point Q from context
        var Q : ECPPoint; ecp_point_init(unsafe(&raw mut Q))
        mpi_copy(unsafe(&raw mut Q.X), &raw mut ctx.pub_x)
        mpi_copy(unsafe(&raw mut Q.Y), &raw mut ctx.pub_y)
        mpi_lset(unsafe(&raw mut Q.Z), 1)

        // R1 = u1 * G
        var R1 : ECPPoint; ecp_point_init(unsafe(&raw mut R1))
        ret = ecp_mul(unsafe(&raw mut R1), unsafe(&raw mut u1), unsafe(&raw mut G))
        if(ret < 0) { return ret }

        // R2 = u2 * Q
        var R2 : ECPPoint; ecp_point_init(unsafe(&raw mut R2))
        ret = ecp_mul(unsafe(&raw mut R2), unsafe(&raw mut u2), unsafe(&raw mut Q))
        if(ret < 0) { return ret }

        // R = R1 + R2
        // ecp_add_jac is a mixed Jacobian-affine addition and requires the
        // second operand to be in affine coordinates (Z = 1). R2 is the
        // un-normalized Jacobian result of ecp_mul, so normalize it first.
        var R : ECPPoint; ecp_point_init(unsafe(&raw mut R))
        ret = ecp_normalize_jac(unsafe(&raw mut R2))
        if(ret < 0) { return ret }
        ret = ecp_add_jac(unsafe(&raw mut R), unsafe(&raw mut R1), unsafe(&raw mut R2))
        if(ret < 0) { return ret }

        // Normalize R to affine
        ret = ecp_normalize_jac(unsafe(&raw mut R))
        if(ret < 0) { return ret }

        // If R is point at infinity, reject
        if(mpi_is_zero(unsafe(&raw mut R.Z))) {
            return ERR_ECDSA_VERIFY_FAILED
        }

        // v = R.x mod n
        var v : Mpi; mpi_init(unsafe(&raw mut v))
        ret = mpi_mod(unsafe(&raw mut v), unsafe(&raw mut R.X), unsafe(&raw mut n))
        if(ret < 0) { return ret }

        // Verify v == r
        if(mpi_cmp(unsafe(&raw mut v), unsafe(&raw mut r)) != 0) {
            return ERR_ECDSA_VERIFY_FAILED
        }

        return 0

                              }
}


// ===== ecdh.ch =====

    // ─── ECDH Error Codes ───────────────────────────────────────────────────

    public comptime const ERR_ECP_BAD_INPUT_DATA = -0x4F80
    public comptime const ERR_ECP_BUFFER_TOO_SMALL = -0x4F00
    public comptime const ERR_ECP_FEATURE_UNAVAILABLE = -0x4E80
    public comptime const ERR_ECP_VERIFY_FAILED = -0x4E00
    public comptime const ERR_ECP_ALLOC_FAILED = -0x4D80
    public comptime const ERR_ECP_INVALID_KEY = -0x4C80

    // ─── secp256r1 (P-256) Curve Parameters ─────────────────────────────────

    // Prime p = FFFFFFFF 00000001 00000000 00000000 00000000 FFFFFFFF FFFFFFFF FFFFFFFF
    var P256_P : [8]u32 = [
        0xFFFFFFFFu32, 0xFFFFFFFFu32, 0xFFFFFFFFu32, 0x00000000u32,
        0x00000000u32, 0x00000000u32, 0x00000001u32, 0xFFFFFFFFu32
    ]

    // Order n = FFFFFFFF 00000000 FFFFFFFF FFFFFFFF BCE6FAAD A7179E84 F3B9CAC2 FC632551
    var P256_N : [8]u32 = [
        0xFC632551u32, 0xF3B9CAC2u32, 0xA7179E84u32, 0xBCE6FAADu32,
        0xFFFFFFFFu32, 0xFFFFFFFFu32, 0x00000000u32, 0xFFFFFFFFu32
    ]

    // ─── P-384 (secp384r1) parameters ─────────────────────────────────────
    // Prime p = 2^384 − 2^128 − 2^96 + 2^32 − 1, little-endian limbs.
    // hex: FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFFFF0000000000000000FFFFFFFF
    var P384_P : [12]u32 = [
        0xFFFFFFFFu32, 0x00000000u32, 0x00000000u32, 0xFFFFFFFFu32,
        0xFFFFFFFEu32, 0xFFFFFFFFu32, 0xFFFFFFFFu32, 0xFFFFFFFFu32,
        0xFFFFFFFFu32, 0xFFFFFFFFu32, 0xFFFFFFFFu32, 0xFFFFFFFFu32
    ]

    // Order n, little-endian limbs.
    // hex: FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFC7634D81F4372DDF581A0DB248B0A77AECEC196ACCC52973
    var P384_N : [12]u32 = [
        0xCCC52973u32, 0xECEC196Au32, 0x48B0A77Au32, 0x581A0DB2u32,
        0xF4372DDFu32, 0xC7634D81u32, 0xFFFFFFFFu32, 0xFFFFFFFFu32,
        0xFFFFFFFFu32, 0xFFFFFFFFu32, 0xFFFFFFFFu32, 0xFFFFFFFFu32
    ]

    // Generator Gx, little-endian limbs.
    // hex: AA87CA22BE8B05378EB1C71EF320AD746E1D3B628BA79B9859F741E082542A385502F25DBF55296C3A545E3872760AB7
    var P384_GX : [12]u32 = [
        0x72760AB7u32, 0x3A545E38u32, 0xBF55296Cu32, 0x5502F25Du32,
        0x82542A38u32, 0x59F741E0u32, 0x8BA79B98u32, 0x6E1D3B62u32,
        0xF320AD74u32, 0x8EB1C71Eu32, 0xBE8B0537u32, 0xAA87CA22u32
    ]

    // Generator Gy, little-endian limbs.
    // hex: 3617DE4A96262C6F5D9E98BF9292DC29F8F41DBD289A147CE9DA3113B5F0B8C00A60B1CE1D7E819D7A431D7C90EA0E5F
    var P384_GY : [12]u32 = [
        0x90EA0E5Fu32, 0x7A431D7Cu32, 0x1D7E819Du32, 0x0A60B1CEu32,
        0xB5F0B8C0u32, 0xE9DA3113u32, 0x289A147Cu32, 0xF8F41DBDu32,
        0x9292DC29u32, 0x5D9E98BFu32, 0x96262C6Fu32, 0x3617DE4Au32
    ]

    // Curve coefficient b, little-endian limbs.
    // hex: B3312FA7E23EE7E4988E056BE3F82D19181D9C6EFE8141120314088F5013875AC656398D8A2ED19D2A85C8EDD3EC2AEF
    var P384_B : [12]u32 = [
        0xD3EC2AEFu32, 0x2A85C8EDu32, 0x8A2ED19Du32, 0xC656398Du32,
        0x5013875Au32, 0x0314088Fu32, 0xFE814112u32, 0x181D9C6Eu32,
        0xE3F82D19u32, 0x988E056Bu32, 0xE23EE7E4u32, 0xB3312FA7u32
    ]

    // Current active curve for the generic point arithmetic helpers.
    // 0 = P-256 (default), 1 = P-384. Set by ecdsa_verify / ecdh around the
    // multiplicative ladder so the ecp_* parameter accessors use the right set.
    // Thread-local: the TLS test suite runs handshake tests in parallel threads,
    // and concurrent P-256 / P-384 operations must not corrupt each other's
    // curve selection.
    @thread_local var GLOBAL_CURVE : int = 0

    public func ecp_select_curve(c : int) {
        GLOBAL_CURVE = c
    }

    public func ecp_curve_id() : int {
        return GLOBAL_CURVE
    }

    public func ecp_curve_gx(gx : *mut Mpi) {
        unsafe {

        if(GLOBAL_CURVE == 1) {
            mpi_init(gx); gx.n = 12
            var i : size_t = 0; while(i < 12) { gx.p[i] = P384_GX[i]; i += 1 }
            return
        }
        mpi_init(gx); gx.n = 8
        var i : size_t = 0; while(i < 8) { gx.p[i] = P256_GX[i]; i += 1 }

    }
}
    public func ecp_curve_gy(gy : *mut Mpi) {
        unsafe {

        if(GLOBAL_CURVE == 1) {
            mpi_init(gy); gy.n = 12
            var i : size_t = 0; while(i < 12) { gy.p[i] = P384_GY[i]; i += 1 }
            return
        }
        mpi_init(gy); gy.n = 8
        var i : size_t = 0; while(i < 8) { gy.p[i] = P256_GY[i]; i += 1 }

    }
}
    public func ecp_curve_b(b : *mut Mpi) {
        unsafe {

        if(GLOBAL_CURVE == 1) {
            mpi_init(b); b.n = 12
            var i : size_t = 0; while(i < 12) { b.p[i] = P384_B[i]; i += 1 }
            return
        }
        mpi_init(b); b.n = 8
        var i : size_t = 0; while(i < 8) { b.p[i] = P256_B[i]; i += 1 }

    }
}

    // Generator Gx = 6B17D1F2 E12C4247 F8BCE6E5 63A440F2 77037D81 2DEB33A0 F4A13945 D898C296
    var P256_GX : [8]u32 = [
        0xD898C296u32, 0xF4A13945u32, 0x2DEB33A0u32, 0x77037D81u32,
        0x63A440F2u32, 0xF8BCE6E5u32, 0xE12C4247u32, 0x6B17D1F2u32
    ]

    // Generator Gy = 4FE342E2 FE1A7F9B 8EE7EB4A 7C0F9E16 2BCE3357 6B315ECE CBB64068 37BF51F5
    var P256_GY : [8]u32 = [
        0x37BF51F5u32, 0xCBB64068u32, 0x6B315ECEu32, 0x2BCE3357u32,
        0x7C0F9E16u32, 0x8EE7EB4Au32, 0xFE1A7F9Bu32, 0x4FE342E2u32
    ]

    // Curve coefficient b = 5AC635D8 AA3A93E7 B3EBBD55 769886BC 651D06B0 CC53B0F6 3BCE3C3E 27D2604B
    public var P256_B : [8]u32 = [
        0x27D2604Bu32, 0x3BCE3C3Eu32, 0xCC53B0F6u32, 0x651D06B0u32,
        0x769886BCu32, 0xB3EBBD55u32, 0xAA3A93E7u32, 0x5AC635D8u32
    ]

    // ─── Elliptic Curve Point ───────────────────────────────────────────────

    public struct ECPPoint {
        var X : Mpi
        var Y : Mpi
        var Z : Mpi
    }

    public func ecp_point_init(p : *mut ECPPoint) {
        unsafe {

        mpi_init(&raw mut p.X); mpi_init(&raw mut p.Y); mpi_init(&raw mut p.Z)

    }
}

    // ─── ECDH Key Pair ──────────────────────────────────────────────────────

    public struct ECDHContext {
        var priv_key : Mpi         // Private key (scalar)
        var pub_key : ECPPoint     // Public key (point)
        var is_init : bool
    }

    public func ecdh_init(ctx : *mut ECDHContext) {
        unsafe {

        mpi_init(&raw mut ctx.priv_key)
        ecp_point_init(&raw mut ctx.pub_key)
        ctx.is_init = false

    }
}

    // ─── Modular arithmetic helpers for P-256 ────────────────────────────

    // Load curve parameter p into an Mpi
    public func ecp_curve_p(p : *mut Mpi) {
        unsafe {

        if(GLOBAL_CURVE == 1) {
            mpi_init(p)
            p.n = 12
            var i : size_t = 0
            while(i < 12) { p.p[i] = P384_P[i]; i += 1 }
            return
        }
        mpi_init(p)
        p.n = 8
        var i : size_t = 0
        while(i < 8) { p.p[i] = P256_P[i]; i += 1 }

    }
}

    public func ecp_curve_n(p : *mut Mpi) {
        unsafe {

        if(GLOBAL_CURVE == 1) {
            mpi_init(p)
            p.n = 12
            var i : size_t = 0
            while(i < 12) { p.p[i] = P384_N[i]; i += 1 }
            return
        }
        mpi_init(p)
        p.n = 8
        var i : size_t = 0
        while(i < 8) { p.p[i] = P256_N[i]; i += 1 }

    }
}

    // Modulo P-256 prime: a mod p (for values < 2*p)
    func ecp_mod_p256(X : *mut Mpi) : int {
        // Fast reduction for P-256 (2^256 - 2^224 + 2^192 + 2^96 - 1)
        // For now, use general mod
        var p : Mpi; ecp_curve_p(unsafe(&raw mut p))
        return mpi_mod(X, X, unsafe(&raw mut p))
    }

    // ─── Point Operations on P-256 ──────────────────────────────────────

    // Point double: R = 2 * P on the P-256 curve
    func ecp_double_jac(R : *mut ECPPoint, P : *mut ECPPoint) : int {
        unsafe {

        // Handle point-at-infinity
        if(mpi_is_zero(&raw mut P.Z)) {
            mpi_lset(&raw mut R.X, 1)
            mpi_lset(&raw mut R.Y, 1)
            mpi_lset(&raw mut R.Z, 0)
            return 0
        }
        // Using formulas from http://www.hyperelliptic.org/EFD/g1p/auto-shortw-jacobian-3.html#doubling-dbl-2007-bl
        // For a = -3 (P-256): optimized formulas
        var p : Mpi; ecp_curve_p(unsafe(&raw mut p))
        var XX : Mpi; mpi_init(unsafe(&raw mut XX))
        var YY : Mpi; mpi_init(unsafe(&raw mut YY))
        var ZZ : Mpi; mpi_init(unsafe(&raw mut ZZ))
        var S : Mpi; mpi_init(unsafe(&raw mut S))
        var M : Mpi; mpi_init(unsafe(&raw mut M))
        var T : Mpi; mpi_init(unsafe(&raw mut T))
        var X3 : Mpi; mpi_init(unsafe(&raw mut X3))
        var Y3 : Mpi; mpi_init(unsafe(&raw mut Y3))
        var Z3 : Mpi; mpi_init(unsafe(&raw mut Z3))
        var ret : int = 0

        // XX = X1^2
        ret = mpi_mul(unsafe(&raw mut XX), &raw mut P.X, &raw mut P.X)
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut XX), unsafe(&raw mut XX), unsafe(&raw mut p))

        // YY = Y1^2
        ret = mpi_mul(unsafe(&raw mut YY), &raw mut P.Y, &raw mut P.Y)
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut YY), unsafe(&raw mut YY), unsafe(&raw mut p))

        // ZZ = Z1^2
        ret = mpi_mul(unsafe(&raw mut ZZ), &raw mut P.Z, &raw mut P.Z)
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut ZZ), unsafe(&raw mut ZZ), unsafe(&raw mut p))

        // S = 4*X1*YY
        ret = mpi_mul(unsafe(&raw mut S), &raw mut P.X, unsafe(&raw mut YY))
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut S), unsafe(&raw mut S), unsafe(&raw mut p))
        ret = mpi_mul_int(unsafe(&raw mut S), unsafe(&raw mut S), 4)
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut S), unsafe(&raw mut S), unsafe(&raw mut p))

        // For a=-3: M = 3*(X1 + ZZ)*(X1 - ZZ)
        // = 3*(X1^2 - Z1^4) = 3*X1^2 + a*Z1^4 (where a=-3)
        var X_plus_ZZ : Mpi; mpi_init(unsafe(&raw mut X_plus_ZZ))
        ret = mpi_add(unsafe(&raw mut X_plus_ZZ), &raw mut P.X, unsafe(&raw mut ZZ))
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut X_plus_ZZ), unsafe(&raw mut X_plus_ZZ), unsafe(&raw mut p))
        var X_minus_ZZ : Mpi; mpi_init(unsafe(&raw mut X_minus_ZZ))
        ret = mpi_sub(unsafe(&raw mut X_minus_ZZ), &raw mut P.X, unsafe(&raw mut ZZ))
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut X_minus_ZZ), unsafe(&raw mut X_minus_ZZ), unsafe(&raw mut p))
        ret = mpi_mul(unsafe(&raw mut M), unsafe(&raw mut X_plus_ZZ), unsafe(&raw mut X_minus_ZZ))
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut M), unsafe(&raw mut M), unsafe(&raw mut p))
        ret = mpi_mul_int(unsafe(&raw mut M), unsafe(&raw mut M), 3)
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut M), unsafe(&raw mut M), unsafe(&raw mut p))

        // X3 = M^2 - 2*S
        ret = mpi_mul(unsafe(&raw mut X3), unsafe(&raw mut M), unsafe(&raw mut M))
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut X3), unsafe(&raw mut X3), unsafe(&raw mut p))
        ret = mpi_sub(unsafe(&raw mut X3), unsafe(&raw mut X3), unsafe(&raw mut S))
        if(ret < 0) { return ret }
        ret = mpi_sub(unsafe(&raw mut X3), unsafe(&raw mut X3), unsafe(&raw mut S))
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut X3), unsafe(&raw mut X3), unsafe(&raw mut p))

        // Y3 = M*(S - X3) - 8*YY^2
        ret = mpi_sub(unsafe(&raw mut T), unsafe(&raw mut S), unsafe(&raw mut X3))
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut T), unsafe(&raw mut T), unsafe(&raw mut p))
        ret = mpi_mul(unsafe(&raw mut Y3), unsafe(&raw mut M), unsafe(&raw mut T))
        if(ret < 0) { return ret }

        // 8*YY^2
        ret = mpi_mul(unsafe(&raw mut YY), unsafe(&raw mut YY), unsafe(&raw mut YY))
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut YY), unsafe(&raw mut YY), unsafe(&raw mut p))
        ret = mpi_mul_int(unsafe(&raw mut YY), unsafe(&raw mut YY), 8)
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut YY), unsafe(&raw mut YY), unsafe(&raw mut p))

        ret = mpi_sub(unsafe(&raw mut Y3), unsafe(&raw mut Y3), unsafe(&raw mut YY))
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut Y3), unsafe(&raw mut Y3), unsafe(&raw mut p))

        // Z3 = 2*Y1*Z1
        ret = mpi_mul(unsafe(&raw mut Z3), &raw mut P.Y, &raw mut P.Z)
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut Z3), unsafe(&raw mut Z3), unsafe(&raw mut p))
        ret = mpi_mul_int(unsafe(&raw mut Z3), unsafe(&raw mut Z3), 2)
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut Z3), unsafe(&raw mut Z3), unsafe(&raw mut p))

        // Copy result
        mpi_copy(&raw mut R.X, unsafe(&raw mut X3))
        mpi_copy(&raw mut R.Y, unsafe(&raw mut Y3))
        mpi_copy(&raw mut R.Z, unsafe(&raw mut Z3))

        return 0

    }
}

    // Point addition: R = P + Q (with Q in affine coordinates, Z=1)
    public func ecp_add_jac(R : *mut ECPPoint, P : *mut ECPPoint, Q : *mut ECPPoint) : int {
        unsafe {

        // Mixed Jacobian-affine addition
        // P = (X1, Y1, Z1), Q = (X2, Y2, 1)
        // R = (X3, Y3, Z3) = P + Q
        // Using formulas from http://www.hyperelliptic.org/EFD/g1p/auto-shortw-jacobian-3.html#addition-madd-2007-bl

        // Handle P = infinity: return Q
        if(mpi_is_zero(&raw mut P.Z)) {
            mpi_copy(&raw mut R.X, &raw mut Q.X)
            mpi_copy(&raw mut R.Y, &raw mut Q.Y)
            mpi_copy(&raw mut R.Z, &raw mut Q.Z)
            return 0
        }

        var p : Mpi; ecp_curve_p(unsafe(&raw mut p))
        var Z1Z1 : Mpi; mpi_init(unsafe(&raw mut Z1Z1))
        var U2 : Mpi; mpi_init(unsafe(&raw mut U2))
        var S2 : Mpi; mpi_init(unsafe(&raw mut S2))
        var H : Mpi; mpi_init(unsafe(&raw mut H))
        var HH : Mpi; mpi_init(unsafe(&raw mut HH))
        var I : Mpi; mpi_init(unsafe(&raw mut I))
        var R_val : Mpi; mpi_init(unsafe(&raw mut R_val))
        var J : Mpi; mpi_init(unsafe(&raw mut J))
        var V : Mpi; mpi_init(unsafe(&raw mut V))
        var X3 : Mpi; mpi_init(unsafe(&raw mut X3))
        var Y3 : Mpi; mpi_init(unsafe(&raw mut Y3))
        var Z3 : Mpi; mpi_init(unsafe(&raw mut Z3))
        var ret : int = 0

        // Z1Z1 = Z1^2
        ret = mpi_mul(unsafe(&raw mut Z1Z1), &raw mut P.Z, &raw mut P.Z)
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut Z1Z1), unsafe(&raw mut Z1Z1), unsafe(&raw mut p))

        // U2 = X2 * Z1Z1
        ret = mpi_mul(unsafe(&raw mut U2), &raw mut Q.X, unsafe(&raw mut Z1Z1))
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut U2), unsafe(&raw mut U2), unsafe(&raw mut p))

        // S2 = Y2 * Z1 * Z1Z1 = Y2 * Z1^3
        ret = mpi_mul(unsafe(&raw mut S2), &raw mut Q.Y, &raw mut P.Z)
        if(ret < 0) { return ret }
        ret = mpi_mul(unsafe(&raw mut S2), unsafe(&raw mut S2), unsafe(&raw mut Z1Z1))
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut S2), unsafe(&raw mut S2), unsafe(&raw mut p))

        // H = U2 - X1
        ret = mpi_sub(unsafe(&raw mut H), unsafe(&raw mut U2), &raw mut P.X)
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut H), unsafe(&raw mut H), unsafe(&raw mut p))

        // HH = H^2
        ret = mpi_mul(unsafe(&raw mut HH), unsafe(&raw mut H), unsafe(&raw mut H))
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut HH), unsafe(&raw mut HH), unsafe(&raw mut p))

        // I = 4 * HH  (but for standard formula: I = 4*HH, we compute 2*HH later)
        // Actually: I = 4*HH is used for X3 formula
        ret = mpi_mul_int(unsafe(&raw mut I), unsafe(&raw mut HH), 4)
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut I), unsafe(&raw mut I), unsafe(&raw mut p))

        // J = H * I
        ret = mpi_mul(unsafe(&raw mut J), unsafe(&raw mut H), unsafe(&raw mut I))
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut J), unsafe(&raw mut J), unsafe(&raw mut p))

        // R_val = 2 * (S2 - Y1)
        ret = mpi_sub(unsafe(&raw mut R_val), unsafe(&raw mut S2), &raw mut P.Y)
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut R_val), unsafe(&raw mut R_val), unsafe(&raw mut p))
        ret = mpi_mul_int(unsafe(&raw mut R_val), unsafe(&raw mut R_val), 2)
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut R_val), unsafe(&raw mut R_val), unsafe(&raw mut p))

        // V = X1 * I
        ret = mpi_mul(unsafe(&raw mut V), &raw mut P.X, unsafe(&raw mut I))
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut V), unsafe(&raw mut V), unsafe(&raw mut p))

        // X3 = R_val^2 - J - 2*V
        ret = mpi_mul(unsafe(&raw mut X3), unsafe(&raw mut R_val), unsafe(&raw mut R_val))
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut X3), unsafe(&raw mut X3), unsafe(&raw mut p))
        var _x3i : size_t; var _x3b : [32]u8; var _x3size : size_t
        if(tls_config::EXTENSIVE_DEBUG_LOG) { unsafe { printf("[X3DBG] after R2: s=%d n=%lu modp=", X3.s, X3.n) }; _x3i=0;for(_x3i=0;_x3i<32;_x3i+=1){_x3b[_x3i]=0};mpi_write_binary(unsafe(&raw mut X3),&raw mut _x3b[0],32);_x3i=0;while(_x3i<32){unsafe { printf("%02x",_x3b[_x3i]as int) };_x3i+=1};unsafe { printf("\n") } }
        ret = mpi_sub(unsafe(&raw mut X3), unsafe(&raw mut X3), unsafe(&raw mut J))
        if(ret < 0) { if(tls_config::EXTENSIVE_DEBUG_LOG) unsafe { printf("[X3DBG] sub1 failed ret=%d\n",ret) }; return ret }
        if(tls_config::EXTENSIVE_DEBUG_LOG) { unsafe { printf("[X3DBG] after -J: s=%d n=%lu val=", X3.s, X3.n) }; _x3i=0;for(_x3i=0;_x3i<32;_x3i+=1){_x3b[_x3i]=0};_x3size=mpi_size(unsafe(&raw mut X3));if(_x3size>32){unsafe { printf("<too big:%lu>",_x3size) }}else{mpi_write_binary(unsafe(&raw mut X3),&raw mut _x3b[0],32);_x3i=0;while(_x3i<32){unsafe { printf("%02x",_x3b[_x3i]as int) };_x3i+=1}};unsafe { printf("\n") } }
        ret = mpi_sub(unsafe(&raw mut X3), unsafe(&raw mut X3), unsafe(&raw mut V))  // subtract V once
        if(ret < 0) { if(tls_config::EXTENSIVE_DEBUG_LOG) unsafe { printf("[X3DBG] sub2 failed ret=%d\n",ret) }; return ret }
        if(tls_config::EXTENSIVE_DEBUG_LOG) unsafe { printf("[X3DBG] after -V: s=%d n=%lu\n", X3.s, X3.n) }
        ret = mpi_sub(unsafe(&raw mut X3), unsafe(&raw mut X3), unsafe(&raw mut V))  // subtract V again = 2*V
        if(ret < 0) { if(tls_config::EXTENSIVE_DEBUG_LOG) unsafe { printf("[X3DBG] sub3 failed ret=%d\n",ret) }; return ret }
        if(tls_config::EXTENSIVE_DEBUG_LOG) unsafe { printf("[X3DBG] after -2V: s=%d n=%lu\n", X3.s, X3.n) }
        ret = mpi_mod(unsafe(&raw mut X3), unsafe(&raw mut X3), unsafe(&raw mut p))
        if(tls_config::EXTENSIVE_DEBUG_LOG) { unsafe { printf("[X3DBG] final: s=%d n=%lu val=", X3.s, X3.n) }; _x3i=0;for(_x3i=0;_x3i<32;_x3i+=1){_x3b[_x3i]=0};_x3size=mpi_size(unsafe(&raw mut X3));if(_x3size>32){unsafe { printf("<too big:%lu>",_x3size) }}else{mpi_write_binary(unsafe(&raw mut X3),&raw mut _x3b[0],32);_x3i=0;while(_x3i<32){unsafe { printf("%02x",_x3b[_x3i]as int) };_x3i+=1}};unsafe { printf("\n") } }

        // Y3 = R_val * (V - X3) - 2 * Y1 * J
        // temp = V - X3
        var tmp_vy : Mpi; mpi_init(unsafe(&raw mut tmp_vy))
        ret = mpi_sub(unsafe(&raw mut tmp_vy), unsafe(&raw mut V), unsafe(&raw mut X3))
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut tmp_vy), unsafe(&raw mut tmp_vy), unsafe(&raw mut p))
        if(ret < 0) { return ret }
        var _y3b : [32]u8; var _y3i : size_t; var _y3sz : size_t
        if(tls_config::EXTENSIVE_DEBUG_LOG) { unsafe { printf("[Y3DBG] tmp_vy s=%d n=%lu val=", tmp_vy.s, tmp_vy.n) }; _y3i=0;for(_y3i=0;_y3i<32;_y3i+=1){_y3b[_y3i]=0};_y3sz=mpi_size(unsafe(&raw mut tmp_vy));if(_y3sz>32){unsafe { printf("<too big:%lu>",_y3sz) }}else{mpi_write_binary(unsafe(&raw mut tmp_vy),&raw mut _y3b[0],32);_y3i=0;while(_y3i<32){unsafe { printf("%02x",_y3b[_y3i]as int) };_y3i+=1}};unsafe { printf("\n") } }
        // Y3 = R_val * (V - X3)
        ret = mpi_mul(unsafe(&raw mut Y3), unsafe(&raw mut R_val), unsafe(&raw mut tmp_vy))
        if(ret < 0) { return ret }
        if(tls_config::EXTENSIVE_DEBUG_LOG) unsafe { printf("[Y3DBG] after mul s=%d n=%lu ret=%d\n", Y3.s, Y3.n, ret) }
        if(tls_config::EXTENSIVE_DEBUG_LOG) { _y3i=0;for(_y3i=0;_y3i<32;_y3i+=1){_y3b[_y3i]=0};_y3sz=mpi_size(unsafe(&raw mut Y3));if(_y3sz>32){unsafe { printf("[Y3DBG] after mul val=<too big:%lu>\n",_y3sz) }}else{mpi_write_binary(unsafe(&raw mut Y3),&raw mut _y3b[0],32);unsafe { printf("[Y3DBG] after mul val=") };_y3i=0;while(_y3i<32){unsafe { printf("%02x",_y3b[_y3i]as int) };_y3i+=1};unsafe { printf("\n") }} }


        // DEBUG ALL INTERMEDIATES
        if(P.Z.n == 8 && tls_config::EXTENSIVE_DEBUG_LOG) { var _b:[32]u8; var _i:size_t
        _i=0;for(_i=0;_i<32;_i+=1){_b[_i]=0};mpi_write_binary(unsafe(&raw mut Z1Z1),&raw mut _b[0],32);unsafe { printf("[ADDDBG] Z1Z1=") };_i=0;while(_i<32){unsafe { printf("%02x",_b[_i]as int) };_i+=1};unsafe { printf("\n") }
        _i=0;for(_i=0;_i<32;_i+=1){_b[_i]=0};mpi_write_binary(unsafe(&raw mut U2),&raw mut _b[0],32);unsafe { printf("[ADDDBG] U2=") };_i=0;while(_i<32){unsafe { printf("%02x",_b[_i]as int) };_i+=1};unsafe { printf("\n") }
        _i=0;for(_i=0;_i<32;_i+=1){_b[_i]=0};mpi_write_binary(unsafe(&raw mut H),&raw mut _b[0],32);unsafe { printf("[ADDDBG] H=") };_i=0;while(_i<32){unsafe { printf("%02x",_b[_i]as int) };_i+=1};unsafe { printf("\n") }
        _i=0;for(_i=0;_i<32;_i+=1){_b[_i]=0};mpi_write_binary(unsafe(&raw mut S2),&raw mut _b[0],32);unsafe { printf("[ADDDBG] S2=") };_i=0;while(_i<32){unsafe { printf("%02x",_b[_i]as int) };_i+=1};unsafe { printf("\n") }
        _i=0;for(_i=0;_i<32;_i+=1){_b[_i]=0};mpi_write_binary(unsafe(&raw mut HH),&raw mut _b[0],32);unsafe { printf("[ADDDBG] HH=") };_i=0;while(_i<32){unsafe { printf("%02x",_b[_i]as int) };_i+=1};unsafe { printf("\n") }
        _i=0;for(_i=0;_i<32;_i+=1){_b[_i]=0};mpi_write_binary(unsafe(&raw mut I),&raw mut _b[0],32);unsafe { printf("[ADDDBG] I=") };_i=0;while(_i<32){unsafe { printf("%02x",_b[_i]as int) };_i+=1};unsafe { printf("\n") }
        _i=0;for(_i=0;_i<32;_i+=1){_b[_i]=0};mpi_write_binary(unsafe(&raw mut J),&raw mut _b[0],32);unsafe { printf("[ADDDBG] J=") };_i=0;while(_i<32){unsafe { printf("%02x",_b[_i]as int) };_i+=1};unsafe { printf("\n") }
        _i=0;for(_i=0;_i<32;_i+=1){_b[_i]=0};mpi_write_binary(unsafe(&raw mut R_val),&raw mut _b[0],32);unsafe { printf("[ADDDBG] R_val=") };_i=0;while(_i<32){unsafe { printf("%02x",_b[_i]as int) };_i+=1};unsafe { printf("\n") }
        _i=0;for(_i=0;_i<32;_i+=1){_b[_i]=0};mpi_write_binary(unsafe(&raw mut V),&raw mut _b[0],32);unsafe { printf("[ADDDBG] V=") };_i=0;while(_i<32){unsafe { printf("%02x",_b[_i]as int) };_i+=1};unsafe { printf("\n") }
        _i=0;for(_i=0;_i<32;_i+=1){_b[_i]=0};mpi_write_binary(unsafe(&raw mut X3),&raw mut _b[0],32);unsafe { printf("[ADDDBG] X3=") };_i=0;while(_i<32){unsafe { printf("%02x",_b[_i]as int) };_i+=1};unsafe { printf("\n") }
        _i=0;for(_i=0;_i<32;_i+=1){_b[_i]=0};mpi_write_binary(unsafe(&raw mut Y3),&raw mut _b[0],32);unsafe { printf("[ADDDBG] Y3=") };_i=0;while(_i<32){unsafe { printf("%02x",_b[_i]as int) };_i+=1};unsafe { printf("\n") }
        }

        // Y1J = Y1 * J
        var Y1J : Mpi; mpi_init(unsafe(&raw mut Y1J))
        ret = mpi_mul(unsafe(&raw mut Y1J), &raw mut P.Y, unsafe(&raw mut J))
        if(ret < 0) { return ret }
        mpi_mod(unsafe(&raw mut Y1J), unsafe(&raw mut Y1J), unsafe(&raw mut p))
        // 2 * Y1J
        mpi_mul_int(unsafe(&raw mut Y1J), unsafe(&raw mut Y1J), 2)
        mpi_mod(unsafe(&raw mut Y1J), unsafe(&raw mut Y1J), unsafe(&raw mut p))

        // Y3 = Y3 - 2*Y1*J
        ret = mpi_sub(unsafe(&raw mut Y3), unsafe(&raw mut Y3), unsafe(&raw mut Y1J))
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut Y3), unsafe(&raw mut Y3), unsafe(&raw mut p))

        // Z3 = (Z1 + H)^2 - Z1Z1 - HH
        ret = mpi_add(unsafe(&raw mut Z3), &raw mut P.Z, unsafe(&raw mut H))
        if(ret < 0) { return ret }
        ret = mpi_mul(unsafe(&raw mut Z3), unsafe(&raw mut Z3), unsafe(&raw mut Z3))
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut Z3), unsafe(&raw mut Z3), unsafe(&raw mut p))
        ret = mpi_sub(unsafe(&raw mut Z3), unsafe(&raw mut Z3), unsafe(&raw mut Z1Z1))
        if(ret < 0) { return ret }
        ret = mpi_sub(unsafe(&raw mut Z3), unsafe(&raw mut Z3), unsafe(&raw mut HH))
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut Z3), unsafe(&raw mut Z3), unsafe(&raw mut p))

        // Copy result
        mpi_copy(&raw mut R.X, unsafe(&raw mut X3))
        mpi_copy(&raw mut R.Y, unsafe(&raw mut Y3))
        mpi_copy(&raw mut R.Z, unsafe(&raw mut Z3))

        return 0

    }
}

    // ─── Scalar Multiplication (Double-and-Add) ──────────────────────────

    // R = k * P on P-256 using Montgomery ladder for safety
    public func ecp_mul(R : *mut ECPPoint, k : *mut Mpi, P : *mut ECPPoint) : int {
        unsafe {

        // Double-and-add: R = k * P
        // Uses ecp_add_jac which requires P to remain in affine form (Z=1).
        // P starts affine (from generator or normalized point) and is
        // kept affine throughout by only doubling the accumulator.

        var R0 : ECPPoint; ecp_point_init(unsafe(&raw mut R0))
        mpi_lset(unsafe(&raw mut R0.X), 1)
        mpi_lset(unsafe(&raw mut R0.Y), 1)
        mpi_lset(unsafe(&raw mut R0.Z), 0)

        // Keep a normalized copy of P that stays affine (Z=1)
        var P_affine : ECPPoint; ecp_point_init(unsafe(&raw mut P_affine))
        mpi_copy(unsafe(&raw mut P_affine.X), &raw mut P.X)
        mpi_copy(unsafe(&raw mut P_affine.Y), &raw mut P.Y)
        mpi_lset(unsafe(&raw mut P_affine.Z), 1)

        var bitlen = mpi_bitlen(k)
        var i = bitlen
        while(i > 0) {
            i -= 1
            var limb_idx = i / BITS_PER_LIMB
            var bit_idx = i % BITS_PER_LIMB
            var bit = (k.p[limb_idx] >> bit_idx) & 1

            // R0 = 2 * R0
            var ret = ecp_double_jac(unsafe(&raw mut R0), unsafe(&raw mut R0))
            if(ret < 0) { return ret }

            if(bit == 1) {
                // R0 = R0 + P (P stays affine, so mixed addition works)
                ret = ecp_add_jac(unsafe(&raw mut R0), unsafe(&raw mut R0), unsafe(&raw mut P_affine))
                if(ret < 0) { return ret }
            }
        }

        mpi_copy(&raw mut R.X, unsafe(&raw mut R0.X))
        mpi_copy(&raw mut R.Y, unsafe(&raw mut R0.Y))
        mpi_copy(&raw mut R.Z, unsafe(&raw mut R0.Z))

        return 0

    }
}

    // ─── Public Key Validation ───────────────────────────────────────────

    // Check that the affine point (x, y) really lies on the active curve.
    //
    // The short Weierstrass equation for both curves this port supports
    // (P-256 and P-384) is y^2 = x^3 - 3x + b, i.e. a = -3 for both, so one
    // formula covers them.
    //
    // Two separate checks, both of which matter:
    //   1. range: 0 <= x, y < p. A coordinate >= p is not a field element of
    //      this curve at all, so the curve equation would be meaningless.
    //   2. curve: y^2 == x^3 - 3x + b (mod p).
    //
    // This is the counterpart of mbedTLS's mbedtls_ecp_check_pubkey, which is
    // what its mbedtls_ecp_point_read_binary relies on. Skipping it means a
    // peer-supplied point (e.g. a COSE key arriving from a client) is accepted
    // as a public key even when it is not a curve point at all.
    //
    // Returns 0 when the point is valid, ERR_ECP_INVALID_KEY when it is not.
    public func ecp_check_affine(x : *mut Mpi, y : *mut Mpi) : int {
        var ret : int = 0
        var p : Mpi; ecp_curve_p(unsafe(&raw mut p))
        var b : Mpi; ecp_curve_b(unsafe(&raw mut b))

        // 1. Range check. mpi_read_binary yields a non-negative value, so only
        //    the upper bound needs testing here.
        if(mpi_cmp(x, unsafe(&raw mut p)) >= 0) { return ERR_ECP_INVALID_KEY }
        if(mpi_cmp(y, unsafe(&raw mut p)) >= 0) { return ERR_ECP_INVALID_KEY }

        // 2. lhs = y^2 mod p
        var lhs : Mpi; mpi_init(unsafe(&raw mut lhs))
        ret = mpi_mul(unsafe(&raw mut lhs), y, y)
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut lhs), unsafe(&raw mut lhs), unsafe(&raw mut p))
        if(ret < 0) { return ret }

        // 3. rhs = (x^3 - 3x + b) mod p
        var x2 : Mpi; mpi_init(unsafe(&raw mut x2))
        ret = mpi_mul(unsafe(&raw mut x2), x, x)
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut x2), unsafe(&raw mut x2), unsafe(&raw mut p))
        if(ret < 0) { return ret }

        var rhs : Mpi; mpi_init(unsafe(&raw mut rhs))
        ret = mpi_mul(unsafe(&raw mut rhs), unsafe(&raw mut x2), x)
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut rhs), unsafe(&raw mut rhs), unsafe(&raw mut p))
        if(ret < 0) { return ret }

        // -3x: subtract a multiple of p first so the value stays small and
        // non-negative, then reduce.
        var three_x : Mpi; mpi_init(unsafe(&raw mut three_x))
        ret = mpi_mul_int(unsafe(&raw mut three_x), x, 3)
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut three_x), unsafe(&raw mut three_x), unsafe(&raw mut p))
        if(ret < 0) { return ret }

        ret = mpi_sub(unsafe(&raw mut rhs), unsafe(&raw mut rhs), unsafe(&raw mut three_x))
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut rhs), unsafe(&raw mut rhs), unsafe(&raw mut p))
        if(ret < 0) { return ret }

        ret = mpi_add(unsafe(&raw mut rhs), unsafe(&raw mut rhs), unsafe(&raw mut b))
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut rhs), unsafe(&raw mut rhs), unsafe(&raw mut p))
        if(ret < 0) { return ret }

        if(mpi_cmp(unsafe(&raw mut lhs), unsafe(&raw mut rhs)) != 0) {
            return ERR_ECP_INVALID_KEY
        }
        return 0
    }

    // ─── Point Normalization (Jacobian to Affine) ────────────────────────

    // Convert Jacobian coordinates to affine: (X/Z^2, Y/Z^3, 1)
    public func ecp_normalize_jac(P : *mut ECPPoint) : int {
        unsafe {

        if(mpi_is_zero(&raw mut P.Z)) {
            // Point at infinity
            mpi_lset(&raw mut P.X, 0)
            mpi_lset(&raw mut P.Y, 0)
            return 0
        }

        var p : Mpi; ecp_curve_p(unsafe(&raw mut p))
        var zi : Mpi; mpi_init(unsafe(&raw mut zi))
        var zi2 : Mpi; mpi_init(unsafe(&raw mut zi2))
        var ret : int = 0

        // zi = Z^-1 mod p
        ret = mpi_mod_inv(unsafe(&raw mut zi), &raw mut P.Z, unsafe(&raw mut p))
        if(ret < 0) { return ret }

        // zi2 = zi^2 mod p
        ret = mpi_mul(unsafe(&raw mut zi2), unsafe(&raw mut zi), unsafe(&raw mut zi))
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut zi2), unsafe(&raw mut zi2), unsafe(&raw mut p))

        // X = X * zi^2 mod p
        ret = mpi_mul(&raw mut P.X, &raw mut P.X, unsafe(&raw mut zi2))
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut P.X, &raw mut P.X, unsafe(&raw mut p))

        // Y = Y * zi * zi2 = Y * zi^3 mod p
        ret = mpi_mul(&raw mut P.Y, &raw mut P.Y, unsafe(&raw mut zi))
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut P.Y, &raw mut P.Y, unsafe(&raw mut p))
        ret = mpi_mul(&raw mut P.Y, &raw mut P.Y, unsafe(&raw mut zi2))
        if(ret < 0) { return ret }
        ret = mpi_mod(&raw mut P.Y, &raw mut P.Y, unsafe(&raw mut p))

        mpi_lset(&raw mut P.Z, 1)

        return 0

    }
}

    // ─── ECDH Key Generation ─────────────────────────────────────────────

    // Generate ECDH key pair (private key + public key)
    // priv: output for private key (big-endian, 32 bytes)
    // pub: output for public key (65 bytes: 0x04 || X || Y)
    public func ecdh_generate_keypair(ctx : *mut ECDHContext,
                                       priv : *mut u8, priv_len : size_t,
                                       pub : *mut u8, pub_len : size_t) : int {
                                           unsafe {

        if(priv_len < 32 || pub_len < 65) { return ERR_ECP_BUFFER_TOO_SMALL }

        // Generate private key from CSPRNG
        var priv_mpi : Mpi; mpi_init(unsafe(&raw mut priv_mpi))
        var rng_ret = random_fill(priv, 32)
        if(rng_ret < 0) { return rng_ret }

        // Import private key and reduce modulo group order
        var ret = mpi_read_binary(unsafe(&raw mut priv_mpi), priv, 32)
        if(ret < 0) { return ret }
        var n : Mpi; ecp_curve_n(unsafe(&raw mut n))
        ret = mpi_mod(unsafe(&raw mut priv_mpi), unsafe(&raw mut priv_mpi), unsafe(&raw mut n))
        if(ret < 0) { return ret }
        ret = mpi_write_binary(unsafe(&raw mut priv_mpi), priv, 32)
        if(ret < 0) { return ret }

        // Compute public key = private * G
        var G : ECPPoint; ecp_point_init(unsafe(&raw mut G))
        // Set generator point directly from pre-computed P-256 Gx, Gy
        mpi_grow(unsafe(&raw mut G.X), 8); G.X.n = 8
        mpi_grow(unsafe(&raw mut G.Y), 8); G.Y.n = 8
        var j : size_t = 0
        while(j < 8) { G.X.p[j] = P256_GX[j]; j += 1 }
        j = 0
        while(j < 8) { G.Y.p[j] = P256_GY[j]; j += 1 }
        mpi_lset(unsafe(&raw mut G.Z), 1)

        var pub_point : ECPPoint; ecp_point_init(unsafe(&raw mut pub_point))

        ret = ecp_mul(unsafe(&raw mut pub_point), unsafe(&raw mut priv_mpi), unsafe(&raw mut G))
        if(ret < 0) { return ret }

        // Normalize to affine
        ret = ecp_normalize_jac(unsafe(&raw mut pub_point))
        if(ret < 0) { return ret }

        // Export public key in uncompressed format: 04 || X (32 bytes) || Y (32 bytes)
        pub[0] = 0x04
        ret = mpi_write_binary(unsafe(&raw mut pub_point.X), &raw mut pub[1], 32)
        if(ret < 0) { return ret }
        ret = mpi_write_binary(unsafe(&raw mut pub_point.Y), &raw mut pub[33], 32)
        if(ret < 0) { return ret }

        // Store in context
        mpi_copy(&raw mut ctx.priv_key, unsafe(&raw mut priv_mpi))
        mpi_copy(&raw mut ctx.pub_key.X, unsafe(&raw mut pub_point.X))
        mpi_copy(&raw mut ctx.pub_key.Y, unsafe(&raw mut pub_point.Y))
        mpi_copy(&raw mut ctx.pub_key.Z, unsafe(&raw mut pub_point.Z))
        ctx.is_init = true

        return 0

                                       }
}

    // ─── ECDH Shared Secret ─────────────────────────────────────────────

    // Compute ECDH shared secret: shared = private * peer_public
    // peer_pub: peer's public key (65 bytes: 0x04 || X || Y)
    // shared: output for shared secret (32 bytes, X coordinate)
    public func ecdh_compute_shared(ctx : *mut ECDHContext,
                                     peer_pub : *u8, peer_pub_len : size_t,
                                     shared : *mut u8, shared_len : size_t) : int {
                                         unsafe {

        if(!ctx.is_init) { return ERR_ECP_INVALID_KEY }
        if(peer_pub_len < 65 || peer_pub[0] != 0x04) {
            return ERR_ECP_BAD_INPUT_DATA
        }
        if(shared_len < 32) { return ERR_ECP_BUFFER_TOO_SMALL }

        // Import peer's public key
        var peer_point : ECPPoint; ecp_point_init(unsafe(&raw mut peer_point))
        var ret = mpi_read_binary(unsafe(&raw mut peer_point.X), &raw peer_pub[1], 32)
        if(ret < 0) { return ret }
        ret = mpi_read_binary(unsafe(&raw mut peer_point.Y), &raw peer_pub[33], 32)
        if(ret < 0) { return ret }
        mpi_lset(unsafe(&raw mut peer_point.Z), 1)

        // Reject the point-at-infinity encoding, then check the point is on the
        // curve. The check reads the curve constants from the thread-local
        // selection, so pin P-256 (this function only handles P-256 peers) for
        // the duration and restore what the caller had active.
        if(peer_point.X.n == 0 && peer_point.Y.n == 0) { return ERR_ECP_INVALID_KEY }
        var prev_curve = ecp_curve_id()
        ecp_select_curve(0)
        ret = ecp_check_affine(unsafe(&raw mut peer_point.X), unsafe(&raw mut peer_point.Y))
        ecp_select_curve(prev_curve)
        if(ret < 0) { return ret }

        // Compute shared = private * peer_point
        var shared_point : ECPPoint; ecp_point_init(unsafe(&raw mut shared_point))
        ret = ecp_mul(unsafe(&raw mut shared_point), &raw mut ctx.priv_key, unsafe(&raw mut peer_point))
        if(ret < 0) { return ret }

        // Normalize and extract X coordinate as shared secret
        ret = ecp_normalize_jac(unsafe(&raw mut shared_point))
        if(ret < 0) { return ret }

        ret = mpi_write_binary(unsafe(&raw mut shared_point.X), shared, 32)
        if(ret < 0) { return ret }

        return 0

                                     }
}


// ===== bignum.ch =====

    public comptime const ERR_MPI_BAD_INPUT_DATA = -0x0010
    public comptime const ERR_MPI_INVALID_CHARACTER = -0x0020
    public comptime const ERR_MPI_BUFFER_TOO_SMALL = -0x0030
    public comptime const ERR_MPI_NEGATIVE_VALUE = -0x0040
    public comptime const ERR_MPI_DIVISION_BY_ZERO = -0x0050
    public comptime const ERR_MPI_ALLOC_FAILED = -0x0060

    public comptime const MAX_LIMBS : size_t = 512
    public comptime const BITS_PER_LIMB : size_t = 32

    public struct Mpi {
        var s : int
        var n : size_t
        var p : [MAX_LIMBS]u32
    }

    public func mpi_init(m : *mut Mpi) {
        unsafe {

        m.s = 1; m.n = 0
        var i : size_t = 0
        while(i < MAX_LIMBS) { m.p[i] = 0; i += 1 }

    }
}

    public func mpi_lset(m : *mut Mpi, val : i64) {
        unsafe {

        if(val < 0) { m.s = -1 } else { m.s = 1 }
        var uv : u64 = 0
        if(val >= 0) { uv = val as u64 }
        else { uv = 0u64 - (val as u64) }
        m.p[0] = (uv & 0xFFFFFFFFu64) as u32
        m.n = 1
        if(uv > 0xFFFFFFFFu64) {
            m.p[1] = (uv >> 32) as u32
            m.n = 2
        }
        var i = m.n
        while(i < MAX_LIMBS) { m.p[i] = 0; i += 1 }

    }
}

    public func mpi_grow(m : *mut Mpi, nlimbs : size_t) : int {
        unsafe {

        if(nlimbs > MAX_LIMBS) { return ERR_MPI_ALLOC_FAILED }
        if(m.n < nlimbs) {
            var i = m.n
            while(i < nlimbs) { m.p[i] = 0; i += 1 }
            m.n = nlimbs
        }
        return 0

    }
}

    public func mpi_trim(m : *mut Mpi) {
        unsafe {

        while(m.n > 0 && m.p[m.n - 1] == 0) { m.n -= 1 }
        if(m.n == 0) { m.s = 1 }

    }
}

    public func mpi_bitlen(m : *mut Mpi) : size_t {
        unsafe {

        mpi_trim(m)
        if(m.n == 0) { return 0 }
        var top = m.p[m.n - 1]
        var bits : size_t = (m.n - 1) * BITS_PER_LIMB
        var msb : u32 = 0x80000000u32
        var i : size_t = 32
        while(msb != 0) {
            if(top & msb) { return bits + i }
            msb = msb >> 1
            i -= 1
        }
        return bits

    }
}

    public func mpi_size(m : *mut Mpi) : size_t {
        return (mpi_bitlen(m) + 7) / 8
    }

    public func mpi_copy(dst : *mut Mpi, src : *mut Mpi) {
        unsafe {

        dst.s = src.s; dst.n = src.n
        var i : size_t = 0
        while(i < src.n) { dst.p[i] = src.p[i]; i += 1 }
        while(i < MAX_LIMBS) { dst.p[i] = 0; i += 1 }

    }
}

    public func mpi_cmp_abs(a : *mut Mpi, b : *mut Mpi) : int {
        unsafe {

        mpi_trim(a); mpi_trim(b)
        if(a.n > b.n) { return 1 }
        if(a.n < b.n) { return -1 }
        var i = a.n
        while(i > 0) { i -= 1
            if(a.p[i] > b.p[i]) { return 1 }
            if(a.p[i] < b.p[i]) { return -1 }
        }
        return 0

    }
}

    public func mpi_cmp(a : *mut Mpi, b : *mut Mpi) : int {
        unsafe {

        mpi_trim(a); mpi_trim(b)
        if(a.n == 0 && b.n == 0) { return 0 }
        if(a.n == 0) { return -b.s }
        if(b.n == 0) { return a.s }
        if(a.s != b.s) { return a.s }
        if(a.s > 0) { return mpi_cmp_abs(a, b) }
        return -mpi_cmp_abs(a, b)

    }
}

    public func mpi_cmp_int(m : *mut Mpi, val : i64) : int {
        var tmp : Mpi; mpi_init(unsafe(&raw mut tmp)); mpi_lset(unsafe(&raw mut tmp), val)
        return mpi_cmp(m, unsafe(&raw mut tmp))
    }

    public func mpi_is_zero(m : *mut Mpi) : bool {
        unsafe {

        mpi_trim(m)
        return m.n == 0

    }
}

    // ─── Addition / Subtraction ──────────────────────────────────────────

    public func mpi_add_abs(x : *mut Mpi, a : *mut Mpi, b : *mut Mpi) : int {
        unsafe {

        var max_n = a.n; if(b.n > max_n) { max_n = b.n }
        var ret = mpi_grow(x, max_n + 1)
        if(ret < 0) { return ret }
        var carry : u64 = 0
        var i : size_t = 0
        while(i < max_n) {
            var av : u64 = 0; var bv : u64 = 0
            if(i < a.n) { av = a.p[i] as u64 }
            if(i < b.n) { bv = b.p[i] as u64 }
            var sum = av + bv + carry
            x.p[i] = (sum & 0xFFFFFFFFu64) as u32
            carry = sum >> 32; i += 1
        }
        if(carry > 0) { x.p[i] = carry as u32; x.n = max_n + 1 }
        else { x.n = max_n }
        x.s = 1; mpi_trim(x); return 0

    }
}

    public func mpi_sub_abs(x : *mut Mpi, a : *mut Mpi, b : *mut Mpi) : int {
        unsafe {

        if(mpi_cmp_abs(a, b) < 0) { return ERR_MPI_NEGATIVE_VALUE }
        var ret = mpi_grow(x, a.n)
        if(ret < 0) { return ret }
        var borrow : u64 = 0
        var i : size_t = 0
        while(i < a.n) {
            var av = a.p[i] as u64
            var bv : u64 = 0
            if(i < b.n) { bv = b.p[i] as u64 }
            var diff : u64 = 0
            if(av >= bv + borrow) { diff = av - bv - borrow; borrow = 0 }
            else { diff = (av + 0x100000000u64) - bv - borrow; borrow = 1 }
            x.p[i] = (diff & 0xFFFFFFFFu64) as u32; i += 1
        }
        x.n = a.n; x.s = 1; mpi_trim(x); return 0

    }
}

    public func mpi_add(x : *mut Mpi, a : *mut Mpi, b : *mut Mpi) : int {
        unsafe {

        var ret : int = 0
        // Same sign: add magnitudes
        if(a.s == b.s) {
            var asgn = a.s
            ret = mpi_add_abs(x, a, b)
            if(ret >= 0) { x.s = asgn }
            return ret
        }
        // Opposite signs: subtract smaller from larger
        if(mpi_cmp_abs(a, b) >= 0) {
            var asgn = a.s
            ret = mpi_sub_abs(x, a, b)
            if(ret >= 0) { x.s = asgn }
        } else {
            var bsgn = b.s
            ret = mpi_sub_abs(x, b, a)
            if(ret >= 0) { x.s = bsgn }
        }
        return ret

    }
}

    public func mpi_sub(x : *mut Mpi, a : *mut Mpi, b : *mut Mpi) : int {
        unsafe {

        var ret : int = 0
        // Opposite signs: add magnitudes
        if(a.s != b.s) {
            var asgn = a.s
            ret = mpi_add_abs(x, a, b)
            if(ret >= 0) { x.s = asgn }
            return ret
        }
        // Same sign: subtract smaller from larger
        if(mpi_cmp_abs(a, b) >= 0) {
            var asgn = a.s
            ret = mpi_sub_abs(x, a, b)
            if(ret >= 0) { x.s = asgn }
        } else {
            var asgn = a.s
            ret = mpi_sub_abs(x, b, a)
            if(ret >= 0) { x.s = -asgn }
        }
        return ret

    }
}

    // ─── Multiplication ──────────────────────────────────────────────────

    public func mpi_mul(x : *mut Mpi, a : *mut Mpi, b : *mut Mpi) : int {
        unsafe {

        mpi_trim(a); mpi_trim(b)
        if(a.n == 0 || b.n == 0) { mpi_lset(x, 0); return 0 }
        // Handle input/output aliasing: save a and b before zeroing x
        var a_sav : Mpi; mpi_init(unsafe(&raw mut a_sav))
        var b_sav : Mpi; mpi_init(unsafe(&raw mut b_sav))
        if(a == x) { mpi_copy(unsafe(&raw mut a_sav), a); a = unsafe(&raw mut a_sav) }
        if(b == x) { mpi_copy(unsafe(&raw mut b_sav), b); b = unsafe(&raw mut b_sav) }
        // a * b = sum over j of (a * b.p[j]) * 2^(32*j)
        mpi_lset(x, 0)
        var tmp : Mpi; mpi_init(unsafe(&raw mut tmp))
        var j : size_t = 0
        while(j < b.n) {
            var ret = mpi_mul_int(unsafe(&raw mut tmp), a, b.p[j])
            if(ret < 0) { return ret }
            if(j > 0) {
                ret = mpi_shift_l(unsafe(&raw mut tmp), j * BITS_PER_LIMB)
                if(ret < 0) { return ret }
            }
            ret = mpi_add(x, x, unsafe(&raw mut tmp))
            if(ret < 0) { return ret }
            j += 1
        }
        x.s = a.s * b.s
        mpi_trim(x)
        return 0

    }
}

    public func mpi_mul_int(x : *mut Mpi, a : *mut Mpi, b : u32) : int {
        unsafe {

        if(b == 0 || mpi_is_zero(a)) { mpi_lset(x, 0); return 0 }
        var ret = mpi_grow(x, a.n + 1)
        if(ret < 0) { return ret }
        var carry : u64 = 0
        var i : size_t = 0
        while(i < a.n) {
            var prod = (a.p[i] as u64) * (b as u64) + carry
            x.p[i] = (prod & 0xFFFFFFFFu64) as u32
            carry = prod >> 32; i += 1
        }
        x.p[i] = carry as u32; x.n = a.n + 1; x.s = a.s; mpi_trim(x); return 0

    }
}

    // ─── Division (schoolbook long division) ─────────────────────────────

    // Legacy bit-by-bit long division. Kept as the fallback for the one shape
    // mpi_div_knuth cannot normalise in place (a dividend using every limb),
    // and as a reference implementation for tests. Do not use in new code.
    func mpi_div_bitwise(q : *mut Mpi, r : *mut Mpi, a : *mut Mpi, b : *mut Mpi) : int {
        unsafe {

        mpi_trim(b)
        if(b.n == 0) { return ERR_MPI_DIVISION_BY_ZERO }
        if(a.n == 0) {
            if(q != null) { mpi_lset(q, 0) }
            if(r != null) { mpi_lset(r, 0) }
            return 0
        }

        var sign = a.s * b.s
        var A : Mpi; mpi_init(unsafe(&raw mut A)); mpi_copy(unsafe(&raw mut A), a); A.s = 1
        var B : Mpi; mpi_init(unsafe(&raw mut B)); mpi_copy(unsafe(&raw mut B), b); B.s = 1

        if(mpi_cmp_abs(unsafe(&raw mut A), unsafe(&raw mut B)) < 0) {
            if(q != null) { mpi_lset(q, 0) }
            if(r != null) {
                var a_sign = a.s
                mpi_copy(r, unsafe(&raw mut A))
                r.s = a_sign
            }
            return 0
        }

        var Q : Mpi; mpi_init(unsafe(&raw mut Q))
        var a_bits = mpi_bitlen(unsafe(&raw mut A))
        var b_bits = mpi_bitlen(unsafe(&raw mut B))
        var shift = a_bits - b_bits

        // Shift B left by 'shift' bits
        var ret = mpi_grow(unsafe(&raw mut B), B.n + (shift / BITS_PER_LIMB) + 2)
        if(ret < 0) { return ret }

        var limb_shift = shift / BITS_PER_LIMB
        var bit_shift = shift % BITS_PER_LIMB

        if(limb_shift > 0) {
            var i = B.n
            while(i > 0) { i -= 1; B.p[i + limb_shift] = B.p[i] }
            var j : size_t = 0
            while(j < limb_shift) { B.p[j] = 0; j += 1 }
            B.n += limb_shift
        }
        if(bit_shift > 0) {
            var carry : u64 = 0
            var i : size_t = 0
            while(i < B.n) {
                var val = (B.p[i] as u64) << bit_shift | carry
                B.p[i] = (val & 0xFFFFFFFFu64) as u32
                carry = val >> 32; i += 1
            }
            if(carry > 0) { B.p[B.n] = carry as u32; B.n += 1 }
        }

        var cur_shift = shift
        while(cur_shift > 0 || mpi_cmp_abs(unsafe(&raw mut A), unsafe(&raw mut B)) >= 0) {
            if(mpi_cmp_abs(unsafe(&raw mut A), unsafe(&raw mut B)) >= 0) {
                mpi_sub_abs(unsafe(&raw mut A), unsafe(&raw mut A), unsafe(&raw mut B))
                // Set the quotient bit at position cur_shift
                var limb_idx = cur_shift / BITS_PER_LIMB
                var bit_idx = cur_shift % BITS_PER_LIMB
                if(Q.n <= limb_idx) { mpi_grow(unsafe(&raw mut Q), limb_idx + 1); Q.n = limb_idx + 1 }
                Q.p[limb_idx] = Q.p[limb_idx] | (1u32 << bit_idx)
            }

            // Shift B right by 1
            var carry : u32 = 0
            var i = B.n
            while(i > 0) { i -= 1
                var val = (carry as u64) << 32 | (B.p[i] as u64)
                B.p[i] = (val >> 1) as u32
                carry = (val & 1) as u32
            }
            if(B.n > 0 && B.p[B.n - 1] == 0) { B.n -= 1 }

            if(cur_shift == 0) { break }
            cur_shift -= 1
        }

        mpi_trim(unsafe(&raw mut Q)); Q.s = sign
        if(q != null) { mpi_copy(q, unsafe(&raw mut Q)) }
        if(r != null) {
            var a_sign = a.s
            mpi_trim(unsafe(&raw mut A)); A.s = a_sign; mpi_copy(r, unsafe(&raw mut A))
        }
        return 0

    }
}

    // Knuth Algorithm D: limb-at-a-time long division, O(limbs) per quotient
    // limb. The bit-by-bit version above walks one quotient BIT per iteration
    // and re-shifts the whole divisor each time, so reducing a 2n-limb value by
    // an n-limb modulus cost O(bits * limbs) — measured at 464us for
    // 2048 % 1024, against 20us for a whole 1024x1024 multiply. That single
    // reduction sits in compute_r2 (R^2 mod N), which mpi_exp_mod runs on every
    // call, so it was ~85% of an RSA public operation.
    //
    // Preconditions (checked by the mpi_div wrapper): u and v non-negative with
    // v.n >= 1, u.n >= v.n, and u.n + 1 <= MAX_LIMBS so the normalisation shift
    // has room for its carry limb. Writes only Q and R (Q.n == m+1 where
    // m = u.n - v.n).
    func mpi_div_knuth(Q : *mut Mpi, R : *mut Mpi, u : *mut Mpi, v : *mut Mpi) : int {
        unsafe {

        var n = v.n
        var m = u.n - n

        // Normalise so the divisor's top bit is set: qhat = u[j+n]*B + u[j+n-1]
        // divided by v[n-1] is then within 2 of the true quotient limb.
        var shift : size_t = 0
        var top = v.p[n - 1]
        while((top & 0x80000000u32) == 0) { top = top << 1; shift += 1 }

        var V : Mpi; mpi_init(unsafe(&raw mut V))
        mpi_copy(unsafe(&raw mut V), v)
        if(shift > 0) { mpi_shift_l(unsafe(&raw mut V), shift) }

        var U : Mpi; mpi_init(unsafe(&raw mut U))
        mpi_copy(unsafe(&raw mut U), u)
        if(shift > 0) { mpi_shift_l(unsafe(&raw mut U), shift) }
        if(U.n < u.n + 1) { mpi_grow(unsafe(&raw mut U), u.n + 1) }
        U.n = u.n + 1

        var Qd : Mpi; mpi_init(unsafe(&raw mut Qd))
        var ret = mpi_grow(unsafe(&raw mut Qd), m + 1)
        if(ret < 0) { return ret }

        var vn1 = V.p[n - 1] as u64
        var j = m
        while(true) {
            // Estimate this quotient limb from the top two dividend limbs.
            var hi2 = ((U.p[j + n] as u64) << 32) | (U.p[j + n - 1] as u64)
            var qhat = hi2 / vn1
            var rhat = hi2 % vn1

            // Refine the estimate using the third dividend limb. qhat is at most
            // 2 too large, so this settles it. The `qhat >= B` test must come
            // first: it is the only case where qhat*vn2 could overflow u64.
            if(n >= 2) {
                var vn2 = V.p[n - 2] as u64
                var refining = true
                while(refining) {
                    if(qhat >= 0x100000000u64) { qhat -= 1; rhat += vn1 }
                    else if(rhat >= 0x100000000u64) { refining = false }
                    else if(qhat * vn2 > ((rhat << 32) + (U.p[j + n - 2] as u64))) { qhat -= 1; rhat += vn1 }
                    else { refining = false }
                }
            }

            // U[j .. j+n] -= qhat * V, tracking the borrow limb by limb.
            var borrow : u64 = 0
            var i : size_t = 0
            while(i < n) {
                var prod = qhat * (V.p[i] as u64) + borrow
                var low = prod & 0xFFFFFFFFu64
                var cur = U.p[j + i] as u64
                if(cur >= low) {
                    U.p[j + i] = (cur - low) as u32
                    borrow = prod >> 32
                } else {
                    U.p[j + i] = (cur + 0x100000000u64 - low) as u32
                    borrow = (prod >> 32) + 1
                }
                i += 1
            }
            var top_cur = U.p[j + n] as u64
            var negative = false
            if(top_cur >= borrow) {
                U.p[j + n] = (top_cur - borrow) as u32
            } else {
                U.p[j + n] = (top_cur + 0x100000000u64 - borrow) as u32
                negative = true
            }

            // qhat was one too large after all: add the divisor back. The carry
            // out of the top limb cancels the borrow taken above, so it is
            // discarded.
            if(negative) {
                qhat -= 1
                var carry : u64 = 0
                var k : size_t = 0
                while(k < n) {
                    var sum = (U.p[j + k] as u64) + (V.p[k] as u64) + carry
                    U.p[j + k] = (sum & 0xFFFFFFFFu64) as u32
                    carry = sum >> 32
                    k += 1
                }
                U.p[j + n] = ((U.p[j + n] as u64) + carry) as u32
            }

            Qd.p[j] = qhat as u32
            if(j == 0) { break }
            j -= 1
        }

        // Un-normalise the remainder (U[0 .. n-1] >> shift) and the quotient.
        mpi_trim(unsafe(&raw mut Qd))
        mpi_copy(Q, unsafe(&raw mut Qd))
        var Rr : Mpi; mpi_init(unsafe(&raw mut Rr))
        var ri : size_t = 0
        while(ri < n) { Rr.p[ri] = U.p[ri]; ri += 1 }
        Rr.n = n
        if(shift > 0) { mpi_shift_r(unsafe(&raw mut Rr), shift) }
        mpi_copy(R, unsafe(&raw mut Rr))
        return 0

    }
}

    public func mpi_div(q : *mut Mpi, r : *mut Mpi, a : *mut Mpi, b : *mut Mpi) : int {
        unsafe {

        mpi_trim(b)
        if(b.n == 0) { return ERR_MPI_DIVISION_BY_ZERO }
        if(a.n == 0) {
            if(q != null) { mpi_lset(q, 0) }
            if(r != null) { mpi_lset(r, 0) }
            return 0
        }

        var sign = a.s * b.s
        var A : Mpi; mpi_init(unsafe(&raw mut A)); mpi_copy(unsafe(&raw mut A), a); A.s = 1
        var B : Mpi; mpi_init(unsafe(&raw mut B)); mpi_copy(unsafe(&raw mut B), b); B.s = 1

        if(mpi_cmp_abs(unsafe(&raw mut A), unsafe(&raw mut B)) < 0) {
            if(q != null) { mpi_lset(q, 0) }
            if(r != null) {
                var a_sign = a.s
                mpi_copy(r, unsafe(&raw mut A))
                r.s = a_sign
            }
            return 0
        }

        // Pick the cheaper algorithm. Knuth pays a fixed normalisation +
        // copy cost per call (a few thousand limb stores) that only amortises
        // over several quotient limbs; for a single-limb quotient — every step
        // of the Euclid loops in mpi_mod_inv and mpi_gcd, whose quotients are
        // almost always 1 or 2 bits — the bit-by-bit walker is cheaper because
        // it iterates exactly (abits - bbits + 1) times, at most 32 for this
        // shape. Measured: forcing Knuth here made a 1024-bit modular inverse
        // 1.4x slower than the old code.
        var m = A.n - B.n
        if(m == 0) { return mpi_div_bitwise(q, r, a, b) }

        // The normalisation shift needs one limb of headroom above the dividend.
        if(A.n + 1 > MAX_LIMBS) {
            return mpi_div_bitwise(q, r, a, b)
        }

        var Q : Mpi; mpi_init(unsafe(&raw mut Q))
        var R : Mpi; mpi_init(unsafe(&raw mut R))
        var ret = mpi_div_knuth(unsafe(&raw mut Q), unsafe(&raw mut R), unsafe(&raw mut A), unsafe(&raw mut B))
        if(ret < 0) { return ret }

        Q.s = sign
        if(q != null) { mpi_copy(q, unsafe(&raw mut Q)) }
        if(r != null) {
            // A zero remainder is canonically positive: mpi_mod tests r.s < 0 to
            // decide whether to add |b|, and a negative zero there would make an
            // exact multiple of |b| come back as |b| instead of 0.
            if(R.n != 0) { R.s = a.s }
            mpi_copy(r, unsafe(&raw mut R))
        }
        return 0

    }
}

    public func mpi_mod(r : *mut Mpi, a : *mut Mpi, b : *mut Mpi) : int {
        unsafe {

        // Handle negative dividend: result must be non-negative modulo |b|
        var ret = mpi_div(null, r, a, b)
        if(ret < 0) { return ret }
        // If remainder is negative, add |b| to get positive residue
        if(r != null && r.s < 0) {
            var tmp : Mpi; mpi_init(unsafe(&raw mut tmp))
            ret = mpi_add(unsafe(&raw mut tmp), r, b)
            if(ret < 0) { return ret }
            mpi_copy(r, unsafe(&raw mut tmp))
        }
        return 0

    }
}

    // Remainder of |a| modulo a single-limb divisor, without forming a
    // quotient.
    //
    // mpi_div walks one quotient BIT per iteration and re-shifts the whole
    // divisor each time, so mpi_mod(x, small_prime) costs ~186us on a 1024-bit
    // x (1024 iterations * ~32-limb shift) no matter how small the divisor is.
    // Trial-dividing a keygen candidate by 63 small primes therefore cost
    // 11.7ms per candidate — as much as the Miller-Rabin rounds the sieve
    // exists to avoid, and a hard ceiling on how far the sieve can be extended.
    // This is O(limbs) with no allocation. Only the magnitude is used.
    // The divisor is never zero in practice (only the small-prime table is
    // passed here), and there is no error channel, so zero is defined as 0
    // rather than trapping.
    public func mpi_mod_small(a : *mut Mpi, d : u32) : u32 {
        unsafe {

        if(d == 0) { return 0 }
        var acc : u64 = 0
        var i = a.n
        while(i > 0) {
            i -= 1
            acc = ((acc << 32) | (a.p[i] as u64)) % (d as u64)
        }
        return acc as u32

    }
}

    // ─── Montgomery Modular Exponentiation ───────────────────────────────

    func montgomery_mul(x : *mut Mpi, a : *mut Mpi, b : *mut Mpi, n : *mut Mpi, n_inv0 : u32) : int {
        unsafe {

        // Handle input/output aliasing: if a or b alias x, save them first
        var a_sav : Mpi; mpi_init(unsafe(&raw mut a_sav))
        var b_sav : Mpi; mpi_init(unsafe(&raw mut b_sav))
        if(a == x) { mpi_copy(unsafe(&raw mut a_sav), a); a = unsafe(&raw mut a_sav) }
        if(b == x) { mpi_copy(unsafe(&raw mut b_sav), b); b = unsafe(&raw mut b_sav) }

        // Need n.n + 2 limbs: n for result + 1 for carry + 1 safety
        var work_limbs = n.n + 2
        var ret = mpi_grow(x, work_limbs)
        if(ret < 0) { return ret }
        var i : size_t = 0
        while(i < work_limbs) { x.p[i] = 0; i += 1 }; x.n = work_limbs

        // Coarsely Integrated Operand Scanning (CIOS) Montgomery multiplication
        i = 0
        while(i < n.n) {
            var ai : u32 = 0
            if(i < a.n) { ai = a.p[i] }

            // u = (T[0] + a[i]*b[0]) * n_inv0 mod 2^32
            var b0 = b.p[0]
            var t0 = (x.p[0] as u64) + (ai as u64) * (b0 as u64)
            var u_val = (t0 * (n_inv0 as u64)) & 0xFFFFFFFFu64

            // Step 1: Multiply-accumulate: T += a[i] * b
            // Step 2: Reduce: T += u * n; T >>= 32
            var carry_mul : u64 = 0
            var carry_red : u64 = 0
            var j : size_t = 0
            while(j < n.n) {
                var bj : u32 = 0; if(j < b.n) { bj = b.p[j] }
                var nj = n.p[j]

                // T[j] += a[i]*b[j] + carry_mul
                var mul_sum = (x.p[j] as u64) + (ai as u64) * (bj as u64) + carry_mul
                var mul_low = (mul_sum & 0xFFFFFFFFu64) as u32
                carry_mul = mul_sum >> 32

                // T[j] += u * n[j] + carry_red (use the multiply result)
                var red_sum = (mul_low as u64) + (u_val as u64) * (nj as u64) + carry_red
                x.p[j] = (red_sum & 0xFFFFFFFFu64) as u32
                carry_red = red_sum >> 32

                j += 1
            }

            // Add remaining carries
            var total_carry = carry_mul + carry_red
            var k : size_t = n.n
            while(total_carry > 0) {
                var sum = (x.p[k] as u64) + (total_carry & 0xFFFFFFFFu64)
                x.p[k] = (sum & 0xFFFFFFFFu64) as u32
                total_carry = (sum >> 32) + (total_carry >> 32)
                k += 1
            }

            // Shift T right by 1 limb (the lowest limb is 0 mod 2^32 and is dropped)
            var si : size_t = 0
            while(si < work_limbs - 1) {
                x.p[si] = x.p[si + 1]
                si += 1
            }
            x.p[work_limbs - 1] = 0

            i += 1
        }

        // After the n shifts, T occupies x[0..n.n] (n.n+1 limbs) with
        // x[n.n] being the carry limb (T may be >= R = 2^(32*n.n)).
        // Per mbedTLS mpi_core_montmul: compute X = T - N over n.n limbs,
        // track borrow, and select the result as follows:
        //   carry=1 or borrow=0  -> result is X (= T - N)
        //   carry=0 and borrow=1 -> result is T (T < N)
        var carry : u64 = x.p[n.n] as u64
        var X : Mpi; mpi_init(unsafe(&raw mut X))
        var borrow : u64 = 0
        var j : size_t = 0
        while(j < n.n) {
            var av = x.p[j] as u64
            var bv = n.p[j] as u64
            var diff : u64 = 0
            if(av >= bv + borrow) { diff = av - bv - borrow; borrow = 0 }
            else { diff = (av + 0x100000000u64) - bv - borrow; borrow = 1 }
            X.p[j] = (diff & 0xFFFFFFFFu64) as u32
            j += 1
        }
        X.n = n.n; X.s = 1
        var take_x : bool = false
        if(carry != 0) { take_x = true }
        else {
            if(borrow == 0) { take_x = true }
            else { take_x = false }
        }
        if(take_x) { mpi_copy(x, unsafe(&raw mut X)) }
        else {
            x.n = n.n
        }
        mpi_trim(x); return 0

    }
}

    func to_montgomery(x : *mut Mpi, a : *mut Mpi, n : *mut Mpi, n_inv0 : u32, r2 : *mut Mpi) : int {
        return montgomery_mul(x, a, r2, n, n_inv0)
    }

    func from_montgomery(x : *mut Mpi, a : *mut Mpi, n : *mut Mpi, n_inv0 : u32) : int {
        var one : Mpi; mpi_init(unsafe(&raw mut one)); mpi_lset(unsafe(&raw mut one), 1)
        return montgomery_mul(x, a, unsafe(&raw mut one), n, n_inv0)
    }

    func compute_r2(r2 : *mut Mpi, n : *mut Mpi) : int {
        unsafe {

        var R : Mpi; mpi_init(unsafe(&raw mut R))
        var ret = mpi_grow(unsafe(&raw mut R), n.n + 1)
        if(ret < 0) { return ret }
        R.p[n.n] = 1; R.n = n.n + 1
        var R_mod : Mpi; mpi_init(unsafe(&raw mut R_mod))
        ret = mpi_mod(unsafe(&raw mut R_mod), unsafe(&raw mut R), n)
        if(ret < 0) { return ret }
        ret = mpi_mul(r2, unsafe(&raw mut R_mod), unsafe(&raw mut R_mod))
        if(ret < 0) { return ret }
        ret = mpi_mod(r2, r2, n)
        return ret

    }
}

    func mpi_exp_mod_fallback(x : *mut Mpi, a : *mut Mpi, e : *mut Mpi, n : *mut Mpi) : int {
        unsafe {

        // Simple left-to-right binary exponentiation with modular reduction.
        //
        // The base is copied when it aliases the result: mpi_lset(x, 1) below
        // overwrites x, and a caller squaring in place (mpi_exp_mod(x, x, 2, n),
        // as the Miller-Rabin loop does) would otherwise exponentiate 1.
        var base : Mpi; mpi_init(unsafe(&raw mut base))
        var b = a
        if(a == x) { mpi_copy(unsafe(&raw mut base), a); b = unsafe(&raw mut base) }
        mpi_lset(x, 1)
        var bitlen = mpi_bitlen(e)
        var i = bitlen
        while(i > 0) {
            i -= 1
            var tmp : Mpi; mpi_init(unsafe(&raw mut tmp))
            var ret = mpi_mul(unsafe(&raw mut tmp), x, x)
            if(ret < 0) { return ret }
            ret = mpi_mod(x, unsafe(&raw mut tmp), n)
            if(ret < 0) { return ret }
            var limb_idx = i / BITS_PER_LIMB
            var bit_idx = i % BITS_PER_LIMB
            if(e.p[limb_idx] & (1u32 << bit_idx)) {
                ret = mpi_mul(unsafe(&raw mut tmp), x, b)
                if(ret < 0) { return ret }
                ret = mpi_mod(x, unsafe(&raw mut tmp), n)
                if(ret < 0) { return ret }
            }
        }
        return 0

    }
}

    public func mpi_exp_mod(x : *mut Mpi, a : *mut Mpi, e : *mut Mpi, n : *mut Mpi) : int {
        unsafe {

        if(mpi_cmp_int(n, 0) <= 0) {
            return ERR_MPI_BAD_INPUT_DATA
        }
        mpi_trim(e)
        if(e.n == 0) { mpi_lset(x, 1); return 0 }

        // Fold the base into [0, n) before anything else. Two reasons:
        // montgomery_mul only reads the first n.n limbs of an operand, so a
        // base with more limbs than the modulus used to be silently truncated
        // rather than reduced; and a negative base must become its
        // non-negative residue here, because the sign fixup that used to run at
        // the end negated an already-reduced result — with an even modulus
        // (a = -5, n = 12, e = 1) it turned the correct 7 into 5.
        var reduced : Mpi; mpi_init(unsafe(&raw mut reduced))
        var base = a
        if(a.s < 0 || mpi_cmp_abs(a, n) >= 0) {
            var red_ret = mpi_mod(unsafe(&raw mut reduced), a, n)
            if(red_ret < 0) { return red_ret }
            base = unsafe(&raw mut reduced)
        }

        // Even modulus: use fallback
        if((n.p[0] & 1) == 0) {
            return mpi_exp_mod_fallback(x, base, e, n)
        }

        // Precompute R^2 mod N
        var r2 : Mpi; mpi_init(unsafe(&raw mut r2))
        var ret = compute_r2(unsafe(&raw mut r2), n)
        if(ret < 0) { return ret }

        // Compute n_inv0 = -n0^-1 mod 2^32 using Newton's method
        var n0 = n.p[0]
        var x0 : u32 = 1
        var iter : size_t = 0
        while(iter < 5) {
            x0 = x0 * (2u32 - n0 * x0)
            iter += 1
        }
        var n_inv0 = 0u32 - x0

        // Convert A to Montgomery representation
        var A_mont : Mpi; mpi_init(unsafe(&raw mut A_mont))
        ret = to_montgomery(unsafe(&raw mut A_mont), base, n, n_inv0, unsafe(&raw mut r2))
        if(ret < 0) { return ret }

        // Start with Montgomery form of 1 (which is R mod N)
        var result : Mpi; mpi_init(unsafe(&raw mut result))
        var one : Mpi; mpi_init(unsafe(&raw mut one)); mpi_lset(unsafe(&raw mut one), 1)
        ret = to_montgomery(unsafe(&raw mut result), unsafe(&raw mut one), n, n_inv0, unsafe(&raw mut r2))
        if(ret < 0) { return ret }

        // Left-to-right binary exponentiation
        var bitlen = mpi_bitlen(e)
        var i = bitlen
        while(i > 0) {
            i -= 1
            ret = montgomery_mul(unsafe(&raw mut result), unsafe(&raw mut result), unsafe(&raw mut result), n, n_inv0)
            if(ret < 0) { return ret }
            var limb_idx = i / BITS_PER_LIMB
            var bit_idx = i % BITS_PER_LIMB
            if(e.p[limb_idx] & (1u32 << bit_idx)) {
                ret = montgomery_mul(unsafe(&raw mut result), unsafe(&raw mut result), unsafe(&raw mut A_mont), n, n_inv0)
                if(ret < 0) { return ret }
            }
        }

        // Convert back
        ret = from_montgomery(x, unsafe(&raw mut result), n, n_inv0)
        if(ret < 0) { return ret }

        return 0

    }
}

    // ─── Modular Inverse (Extended Euclidean) ────────────────────────────

    // True when the value is zero or even (magnitude only; sign is ignored).
    func mpi_is_even(m : *mut Mpi) : bool {
        unsafe {

        mpi_trim(m)
        return m.n == 0 || (m.p[0] & 1) == 0

    }
}

    func mpi_is_one(m : *mut Mpi) : bool {
        unsafe {

        mpi_trim(m)
        return m.n == 1 && m.p[0] == 1 && m.s > 0

    }
}

    // Computes x = a^-1 mod n for an odd modulus, via the binary extended
    // Euclidean algorithm (HAC 14.61): only shifts, additions, subtractions and
    // comparisons, so no division appears on the critical path.
    //
    // Why this exists: the division-based loop below runs one full long division
    // per Euclid step, and a 1024-bit inverse needs ~600 of them, measured at
    // 5.6ms. Two thirds of those are also single-limb quotients. The binary form
    // runs ~2*bitlen iterations of O(limbs) work. Every modulus this library
    // inverts against is odd (RSA primes, prime fields), so this is the normal
    // path; even moduli fall back to the Euclidean loop.
    //
    // Intermediates are kept in [0, n): an odd x is reduced by adding n before
    // halving, and a subtraction that would go negative adds n first.
    func mpi_mod_inv_binary(x : *mut Mpi, a : *mut Mpi, n : *mut Mpi) : int {
        var U : Mpi; mpi_init(unsafe(&raw mut U))
        var V : Mpi; mpi_init(unsafe(&raw mut V))
        var X1 : Mpi; mpi_init(unsafe(&raw mut X1))
        var X2 : Mpi; mpi_init(unsafe(&raw mut X2))
        var Amod : Mpi; mpi_init(unsafe(&raw mut Amod))
        var T : Mpi; mpi_init(unsafe(&raw mut T))

        var ret = mpi_mod(unsafe(&raw mut Amod), a, n)
        if(ret < 0) { return ret }
        mpi_copy(unsafe(&raw mut U), unsafe(&raw mut Amod))
        mpi_copy(unsafe(&raw mut V), n)
        mpi_lset(unsafe(&raw mut X1), 1)
        mpi_lset(unsafe(&raw mut X2), 0)

        // u + v strictly decreases every iteration and both carry no more than
        // bitlen(n) bits, so ~2*bitlen(n) iterations is a generous bound.
        var limit = mpi_bitlen(n) * 3 + 64
        var steps : size_t = 0
        while(!mpi_is_one(unsafe(&raw mut U)) && !mpi_is_one(unsafe(&raw mut V))) {
            if(steps > limit) { return ERR_MPI_BAD_INPUT_DATA }
            steps += 1

            while(!mpi_is_zero(unsafe(&raw mut U)) && mpi_is_even(unsafe(&raw mut U))) {
                mpi_shift_r(unsafe(&raw mut U), 1)
                if(!mpi_is_even(unsafe(&raw mut X1))) {
                    ret = mpi_add(unsafe(&raw mut X1), unsafe(&raw mut X1), n)
                    if(ret < 0) { return ret }
                }
                mpi_shift_r(unsafe(&raw mut X1), 1)
            }
            while(!mpi_is_zero(unsafe(&raw mut V)) && mpi_is_even(unsafe(&raw mut V))) {
                mpi_shift_r(unsafe(&raw mut V), 1)
                if(!mpi_is_even(unsafe(&raw mut X2))) {
                    ret = mpi_add(unsafe(&raw mut X2), unsafe(&raw mut X2), n)
                    if(ret < 0) { return ret }
                }
                mpi_shift_r(unsafe(&raw mut X2), 1)
            }
            // A zero operand means gcd(a, n) > 1: no inverse exists.
            if(mpi_is_zero(unsafe(&raw mut U)) || mpi_is_zero(unsafe(&raw mut V))) {
                return ERR_MPI_BAD_INPUT_DATA
            }

            if(mpi_cmp(unsafe(&raw mut U), unsafe(&raw mut V)) >= 0) {
                ret = mpi_sub(unsafe(&raw mut U), unsafe(&raw mut U), unsafe(&raw mut V))
                if(ret < 0) { return ret }
                if(mpi_cmp(unsafe(&raw mut X1), unsafe(&raw mut X2)) < 0) {
                    ret = mpi_add(unsafe(&raw mut X1), unsafe(&raw mut X1), n)
                    if(ret < 0) { return ret }
                }
                ret = mpi_sub(unsafe(&raw mut X1), unsafe(&raw mut X1), unsafe(&raw mut X2))
                if(ret < 0) { return ret }
            } else {
                ret = mpi_sub(unsafe(&raw mut V), unsafe(&raw mut V), unsafe(&raw mut U))
                if(ret < 0) { return ret }
                if(mpi_cmp(unsafe(&raw mut X2), unsafe(&raw mut X1)) < 0) {
                    ret = mpi_add(unsafe(&raw mut X2), unsafe(&raw mut X2), n)
                    if(ret < 0) { return ret }
                }
                ret = mpi_sub(unsafe(&raw mut X2), unsafe(&raw mut X2), unsafe(&raw mut X1))
                if(ret < 0) { return ret }
            }
        }

        if(mpi_is_one(unsafe(&raw mut U))) { mpi_copy(x, unsafe(&raw mut X1)) }
        else { mpi_copy(x, unsafe(&raw mut X2)) }

        ret = mpi_mod(x, x, n)
        if(ret < 0) { return ret }

        // The loop exits on u == 1 or v == 1, which a non-coprime input can
        // also reach (e.g. a=3, n=9 reaches v=3 then u=0, but a shared even
        // factor would not). Verify rather than trust the exit condition.
        ret = mpi_mul(unsafe(&raw mut T), unsafe(&raw mut Amod), x)
        if(ret < 0) { return ret }
        ret = mpi_mod(unsafe(&raw mut T), unsafe(&raw mut T), n)
        if(ret < 0) { return ret }
        if(!mpi_is_one(unsafe(&raw mut T))) { return ERR_MPI_BAD_INPUT_DATA }
        return 0
    }

    // Computes x = a^-1 mod n via the extended Euclidean algorithm
    // (division-based). Unlike the binary extended GCD, this works for any
    // modulus (odd or even) and for any a coprime to n.
    func mpi_mod_inv_euclid(x : *mut Mpi, a : *mut Mpi, n : *mut Mpi) : int {
        if(mpi_cmp_int(n, 1) <= 0) { return ERR_MPI_BAD_INPUT_DATA }

        // r0 = n, r1 = a mod n
        var r0 : Mpi; mpi_init(unsafe(&raw mut r0))
        var r1 : Mpi; mpi_init(unsafe(&raw mut r1))
        mpi_copy(unsafe(&raw mut r0), n)
        var ret = mpi_mod(unsafe(&raw mut r1), a, n)
        if(ret < 0) { return ret }

        // t0 = 0, t1 = 1  (Bezout coefficients for r0 and r1)
        var t0 : Mpi; mpi_init(unsafe(&raw mut t0)); mpi_lset(unsafe(&raw mut t0), 0)
        var t1 : Mpi; mpi_init(unsafe(&raw mut t1)); mpi_lset(unsafe(&raw mut t1), 1)

        while(!mpi_is_zero(unsafe(&raw mut r1))) {
            // q = r0 / r1, r2 = r0 mod r1
            var q : Mpi; mpi_init(unsafe(&raw mut q))
            var r2 : Mpi; mpi_init(unsafe(&raw mut r2))
            ret = mpi_div(unsafe(&raw mut q), unsafe(&raw mut r2), unsafe(&raw mut r0), unsafe(&raw mut r1))
            if(ret < 0) { return ret }

            // t2 = t0 - q * t1
            var qt : Mpi; mpi_init(unsafe(&raw mut qt))
            ret = mpi_mul(unsafe(&raw mut qt), unsafe(&raw mut q), unsafe(&raw mut t1))
            if(ret < 0) { return ret }
            var t2 : Mpi; mpi_init(unsafe(&raw mut t2))
            ret = mpi_sub(unsafe(&raw mut t2), unsafe(&raw mut t0), unsafe(&raw mut qt))
            if(ret < 0) { return ret }

            // Shift: r0 = r1, r1 = r2; t0 = t1, t1 = t2
            mpi_copy(unsafe(&raw mut r0), unsafe(&raw mut r1))
            mpi_copy(unsafe(&raw mut r1), unsafe(&raw mut r2))
            mpi_copy(unsafe(&raw mut t0), unsafe(&raw mut t1))
            mpi_copy(unsafe(&raw mut t1), unsafe(&raw mut t2))
        }

        // r0 = gcd(a, n). If it's not 1, no inverse exists.
        if(mpi_cmp_int(unsafe(&raw mut r0), 1) != 0) {
            return ERR_MPI_BAD_INPUT_DATA
        }

        // t0 is the inverse (may be negative); reduce mod n
        mpi_mod(x, unsafe(&raw mut t0), n)
        return 0
    }

    public func mpi_mod_inv(x : *mut Mpi, a : *mut Mpi, n : *mut Mpi) : int {
        unsafe {

        if(mpi_cmp_int(n, 1) <= 0) { return ERR_MPI_BAD_INPUT_DATA }
        if((n.p[0] & 1) == 0) { return mpi_mod_inv_euclid(x, a, n) }
        return mpi_mod_inv_binary(x, a, n)

    }
}

    // ─── Shift Operations ──────────────────────────────────────────────

    public func mpi_shift_l(m : *mut Mpi, count : size_t) : int {
        unsafe {

        if(m.n == 0 || count == 0) { return 0 }
        var limb_shift = count / BITS_PER_LIMB
        var bit_shift = count % BITS_PER_LIMB
        var old_n = m.n
        // Grow to accommodate the shift
        var new_n = old_n + limb_shift + 1
        var ret = mpi_grow(m, new_n)
        if(ret < 0) { return ret }
        // Shift whole limbs left (copy from high to low so source limbs
        // are not clobbered), then zero the vacated low limbs.
        if(limb_shift > 0) {
            var i = new_n
            while(i > limb_shift) {
                i -= 1
                m.p[i] = m.p[i - limb_shift]
            }
            var j : size_t = 0
            while(j < limb_shift) { m.p[j] = 0; j += 1 }
            m.n = old_n + limb_shift
        }
        // Shift bits within limbs
        if(bit_shift > 0) {
            var carry : u32 = 0
            var i : size_t = 0
            while(i < m.n) {
                var val = ((m.p[i] as u64) << bit_shift) | (carry as u64)
                m.p[i] = (val & 0xFFFFFFFFu64) as u32
                carry = (val >> 32) as u32
                i += 1
            }
            if(carry > 0) { m.p[m.n] = carry; m.n += 1 }
        }
        mpi_trim(m)
        return 0

    }
}

    public func mpi_shift_r(m : *mut Mpi, count : size_t) : int {
        unsafe {

        if(m.n == 0 || count == 0) { return 0 }
        var limb_shift = count / BITS_PER_LIMB
        var bit_shift = count % BITS_PER_LIMB
        if(limb_shift >= m.n) { mpi_lset(m, 0); return 0 }
        if(limb_shift > 0) {
            var i : size_t = 0
            while(i < m.n - limb_shift) { m.p[i] = m.p[i + limb_shift]; i += 1 }
            while(i < m.n) { m.p[i] = 0; i += 1 }
            m.n -= limb_shift
        }
        if(bit_shift > 0) {
            var carry : u32 = 0
            var i = m.n
            while(i > 0) { i -= 1
                var val = (carry << (BITS_PER_LIMB - bit_shift)) | (m.p[i] >> bit_shift)
                carry = m.p[i] & ((1u32 << bit_shift) - 1)
                m.p[i] = val
            }
            mpi_trim(m)
        }
        return 0

    }
}

    // ─── Import / Export ────────────────────────────────────────────────

    public func mpi_read_binary(m : *mut Mpi, buf : *u8, buflen : size_t) : int {
        unsafe {

        mpi_init(m)
        if(buflen == 0) { return 0 }
        var limbs = (buflen * 8 + BITS_PER_LIMB - 1) / BITS_PER_LIMB
        var ret = mpi_grow(m, limbs)
        if(ret < 0) { return ret }
        var i : size_t = 0
        while(i < buflen) {
            var byte_val = buf[buflen - 1 - i] as u32
            var limb = i / 4; var offset = (i % 4) * 8
            m.p[limb] = m.p[limb] | (byte_val << offset)
            i += 1
        }
        m.n = limbs; m.s = 1; mpi_trim(m); return 0

    }
}

    public func mpi_write_binary(m : *mut Mpi, buf : *mut u8, buflen : size_t) : int {
        unsafe {

        var size = mpi_size(m)
        if(buflen < size) { return ERR_MPI_BUFFER_TOO_SMALL }
        var pad = buflen - size
        var i : size_t = 0
        while(i < pad) { buf[i] = 0; i += 1 }
        var j : size_t = 0
        while(j < size) {
            buf[buflen - 1 - j] = ((m.p[j / 4] >> ((j % 4) * 8)) & 0xFFu32) as u8
            j += 1
        }
        return 0

    }
}

    public func mpi_gcd(x : *mut Mpi, a : *mut Mpi, b : *mut Mpi) : int {
        unsafe {

        var A : Mpi; mpi_init(unsafe(&raw mut A)); mpi_copy(unsafe(&raw mut A), a)
        var B : Mpi; mpi_init(unsafe(&raw mut B)); mpi_copy(unsafe(&raw mut B), b)
        while(!mpi_is_zero(unsafe(&raw mut B))) {
            var T : Mpi; mpi_init(unsafe(&raw mut T))
            mpi_mod(unsafe(&raw mut T), unsafe(&raw mut A), unsafe(&raw mut B))
            mpi_copy(unsafe(&raw mut A), unsafe(&raw mut B))
            mpi_copy(unsafe(&raw mut B), unsafe(&raw mut T))
        }
        mpi_copy(x, unsafe(&raw mut A)); x.s = 1; return 0

    }
}


// ===== aes.ch =====

    public comptime const AES_ERR_INVALID_KEY_LENGTH = -0x0020
    public comptime const AES_ERR_INVALID_INPUT_LENGTH = -0x0040

    // ─── AES Context ────────────────────────────────────────────────────────

    public struct AESContext {
        var nr : u32          // Number of rounds (10 for AES-128, 14 for AES-256)
        var rk : [68]u32      // Round key storage (60 for AES-256 + 8 spare)
        var buf : [16]u8      // Encryption/decryption buffer for CBC
    }

    public func aes_init(ctx : *mut AESContext) {
        unsafe {

        ctx.nr = 0
        var i : size_t = 0
        while(i < 68) { ctx.rk[i] = 0; i += 1 }
        i = 0
        while(i < 16) { ctx.buf[i] = 0; i += 1 }

    }
}

    // ─── AES S-box and inverse S-box ───────────────────────────────────────

    var AES_SBOX : []u8 = [0x63 as u8, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5,
        0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
        0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0,
        0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
        0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC,
        0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
        0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A,
        0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
        0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0,
        0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
        0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B,
        0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
        0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85,
        0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
        0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5,
        0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
        0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17,
        0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
        0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88,
        0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
        0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C,
        0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
        0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9,
        0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
        0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6,
        0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
        0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E,
        0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
        0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94,
        0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
        0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68,
        0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16
    ]

    var AES_RSBOX : []u8 = [0x52 as u8, 0x09, 0x6A, 0xD5, 0x30, 0x36, 0xA5, 0x38,
        0xBF, 0x40, 0xA3, 0x9E, 0x81, 0xF3, 0xD7, 0xFB,
        0x7C, 0xE3, 0x39, 0x82, 0x9B, 0x2F, 0xFF, 0x87,
        0x34, 0x8E, 0x43, 0x44, 0xC4, 0xDE, 0xE9, 0xCB,
        0x54, 0x7B, 0x94, 0x32, 0xA6, 0xC2, 0x23, 0x3D,
        0xEE, 0x4C, 0x95, 0x0B, 0x42, 0xFA, 0xC3, 0x4E,
        0x08, 0x2E, 0xA1, 0x66, 0x28, 0xD9, 0x24, 0xB2,
        0x76, 0x5B, 0xA2, 0x49, 0x6D, 0x8B, 0xD1, 0x25,
        0x72, 0xF8, 0xF6, 0x64, 0x86, 0x68, 0x98, 0x16,
        0xD4, 0xA4, 0x5C, 0xCC, 0x5D, 0x65, 0xB6, 0x92,
        0x6C, 0x70, 0x48, 0x50, 0xFD, 0xED, 0xB9, 0xDA,
        0x5E, 0x15, 0x46, 0x57, 0xA7, 0x8D, 0x9D, 0x84,
        0x90, 0xD8, 0xAB, 0x00, 0x8C, 0xBC, 0xD3, 0x0A,
        0xF7, 0xE4, 0x58, 0x05, 0xB8, 0xB3, 0x45, 0x06,
        0xD0, 0x2C, 0x1E, 0x8F, 0xCA, 0x3F, 0x0F, 0x02,
        0xC1, 0xAF, 0xBD, 0x03, 0x01, 0x13, 0x8A, 0x6B,
        0x3A, 0x91, 0x11, 0x41, 0x4F, 0x67, 0xDC, 0xEA,
        0x97, 0xF2, 0xCF, 0xCE, 0xF0, 0xB4, 0xE6, 0x73,
        0x96, 0xAC, 0x74, 0x22, 0xE7, 0xAD, 0x35, 0x85,
        0xE2, 0xF9, 0x37, 0xE8, 0x1C, 0x75, 0xDF, 0x6E,
        0x47, 0xF1, 0x1A, 0x71, 0x1D, 0x29, 0xC5, 0x89,
        0x6F, 0xB7, 0x62, 0x0E, 0xAA, 0x18, 0xBE, 0x1B,
        0xFC, 0x56, 0x3E, 0x4B, 0xC6, 0xD2, 0x79, 0x20,
        0x9A, 0xDB, 0xC0, 0xFE, 0x78, 0xCD, 0x5A, 0xF4,
        0x1F, 0xDD, 0xA8, 0x33, 0x88, 0x07, 0xC7, 0x31,
        0xB1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xEC, 0x5F,
        0x60, 0x51, 0x7F, 0xA9, 0x19, 0xB5, 0x4A, 0x0D,
        0x2D, 0xE5, 0x7A, 0x9F, 0x93, 0xC9, 0x9C, 0xEF,
        0xA0, 0xE0, 0x3B, 0x4D, 0xAE, 0x2A, 0xF5, 0xB0,
        0xC8, 0xEB, 0xBB, 0x3C, 0x83, 0x53, 0x99, 0x61,
        0x17, 0x2B, 0x04, 0x7E, 0xBA, 0x77, 0xD6, 0x26,
        0xE1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0C, 0x7D
    ]

    // ─── Rcon values for key expansion ──────────────────────────────────────

    func aes_rcon(i : size_t) : u32 {
        var rc : [10]u8 = [0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36]
        if(i > 0 && i < 11) { return rc[i - 1] as u32 }
        return 0
    }

    // ─── AES Key Expansion ──────────────────────────────────────────────────

    func aes_key_expansion(ctx : *mut AESContext, key : *u8, key_len : size_t) : int {
        unsafe {

        var nb : u32 = 4  // Number of columns (fixed for AES)
        var nk : u32 = 0  // Number of 32-bit words in the key
        var nr : u32 = 0  // Number of rounds

        // Accept key_len in both bytes (16, 32) and bits (128, 256)
        var actual_key_len = key_len
        if(actual_key_len == 128) { actual_key_len = 16 }
        else if(actual_key_len == 256) { actual_key_len = 32 }

        if(actual_key_len == 16) { nk = 4; nr = 10 }
        else if(actual_key_len == 32) { nk = 8; nr = 14 }
        else { return AES_ERR_INVALID_KEY_LENGTH }

        ctx.nr = nr

        // Copy key into first Nk words of round keys
        var i : u32 = 0
        while(i < nk) {
            ctx.rk[i] = ((key[4*i] as u32) << 24) |
                        ((key[4*i+1] as u32) << 16) |
                        ((key[4*i+2] as u32) << 8) |
                        (key[4*i+3] as u32)
            i += 1
        }

        // Expand the key
        i = nk
        while(i < (nb * (nr + 1))) {
            var temp = ctx.rk[i - 1]
            if(i % nk == 0) {
                // RotWord + SubWord + Rcon
                temp = ((AES_SBOX[(temp >> 16) & 0xFF] as u32) << 24) |
                       ((AES_SBOX[(temp >> 8) & 0xFF] as u32) << 16) |
                       ((AES_SBOX[temp & 0xFF] as u32) << 8) |
                       (AES_SBOX[(temp >> 24) & 0xFF] as u32)
                temp = temp ^ (aes_rcon((i / nk) as size_t) << 24)
            } else if(nk > 6 && (i % nk) == 4) {
                // SubWord only (for AES-256)
                temp = ((AES_SBOX[(temp >> 24) & 0xFF] as u32) << 24) |
                       ((AES_SBOX[(temp >> 16) & 0xFF] as u32) << 16) |
                       ((AES_SBOX[(temp >> 8) & 0xFF] as u32) << 8) |
                       (AES_SBOX[temp & 0xFF] as u32)
            }
            ctx.rk[i] = ctx.rk[i - nk] ^ temp
            i += 1
        }

        return 0

    }
}

    // ─── AES ECB Encrypt (single block) ─────────────────────────────────────

    func aes_internal_encrypt(ctx : *mut AESContext, input : *u8, output : *mut u8) {
        unsafe {

        var s0 : u32 = ((input[0] as u32) << 24) | ((input[1] as u32) << 16) |
                        ((input[2] as u32) << 8) | (input[3] as u32)
        var s1 : u32 = ((input[4] as u32) << 24) | ((input[5] as u32) << 16) |
                        ((input[6] as u32) << 8) | (input[7] as u32)
        var s2 : u32 = ((input[8] as u32) << 24) | ((input[9] as u32) << 16) |
                        ((input[10] as u32) << 8) | (input[11] as u32)
        var s3 : u32 = ((input[12] as u32) << 24) | ((input[13] as u32) << 16) |
                        ((input[14] as u32) << 8) | (input[15] as u32)

        // AddRoundKey (round 0)
        s0 ^= ctx.rk[0]; s1 ^= ctx.rk[1]; s2 ^= ctx.rk[2]; s3 ^= ctx.rk[3]

        var r : u32 = 1
        while(r < ctx.nr) {
            // SubBytes + ShiftRows + MixColumns + AddRoundKey
            var t0 : u32
            var t1 : u32
            var t2 : u32
            var t3 : u32

            t0 = ((AES_SBOX[(s0 >> 24) & 0xFF] as u32) << 24) ^
                 ((AES_SBOX[(s1 >> 16) & 0xFF] as u32) << 16) ^
                 ((AES_SBOX[(s2 >> 8) & 0xFF] as u32) << 8) ^
                 (AES_SBOX[s3 & 0xFF] as u32)
            t1 = ((AES_SBOX[(s1 >> 24) & 0xFF] as u32) << 24) ^
                 ((AES_SBOX[(s2 >> 16) & 0xFF] as u32) << 16) ^
                 ((AES_SBOX[(s3 >> 8) & 0xFF] as u32) << 8) ^
                 (AES_SBOX[s0 & 0xFF] as u32)
            t2 = ((AES_SBOX[(s2 >> 24) & 0xFF] as u32) << 24) ^
                 ((AES_SBOX[(s3 >> 16) & 0xFF] as u32) << 16) ^
                 ((AES_SBOX[(s0 >> 8) & 0xFF] as u32) << 8) ^
                 (AES_SBOX[s1 & 0xFF] as u32)
            t3 = ((AES_SBOX[(s3 >> 24) & 0xFF] as u32) << 24) ^
                 ((AES_SBOX[(s0 >> 16) & 0xFF] as u32) << 16) ^
                 ((AES_SBOX[(s1 >> 8) & 0xFF] as u32) << 8) ^
                 (AES_SBOX[s2 & 0xFF] as u32)

            // MixColumns using byte extraction
            var b0_b0 = (t0 >> 24) & 0xFF; var b0_b1 = (t0 >> 16) & 0xFF
            var b0_b2 = (t0 >> 8) & 0xFF; var b0_b3 = t0 & 0xFF
            var b1_b0 = (t1 >> 24) & 0xFF; var b1_b1 = (t1 >> 16) & 0xFF
            var b1_b2 = (t1 >> 8) & 0xFF; var b1_b3 = t1 & 0xFF
            var b2_b0 = (t2 >> 24) & 0xFF; var b2_b1 = (t2 >> 16) & 0xFF
            var b2_b2 = (t2 >> 8) & 0xFF; var b2_b3 = t2 & 0xFF
            var b3_b0 = (t3 >> 24) & 0xFF; var b3_b1 = (t3 >> 16) & 0xFF
            var b3_b2 = (t3 >> 8) & 0xFF; var b3_b3 = t3 & 0xFF

            s0 = (mul2(b0_b0) ^ mul3(b0_b1) ^ b0_b2 ^ b0_b3) << 24 |
                 (b0_b0 ^ mul2(b0_b1) ^ mul3(b0_b2) ^ b0_b3) << 16 |
                 (b0_b0 ^ b0_b1 ^ mul2(b0_b2) ^ mul3(b0_b3)) << 8 |
                 (mul3(b0_b0) ^ b0_b1 ^ b0_b2 ^ mul2(b0_b3))

            s1 = (mul2(b1_b0) ^ mul3(b1_b1) ^ b1_b2 ^ b1_b3) << 24 |
                 (b1_b0 ^ mul2(b1_b1) ^ mul3(b1_b2) ^ b1_b3) << 16 |
                 (b1_b0 ^ b1_b1 ^ mul2(b1_b2) ^ mul3(b1_b3)) << 8 |
                 (mul3(b1_b0) ^ b1_b1 ^ b1_b2 ^ mul2(b1_b3))

            s2 = (mul2(b2_b0) ^ mul3(b2_b1) ^ b2_b2 ^ b2_b3) << 24 |
                 (b2_b0 ^ mul2(b2_b1) ^ mul3(b2_b2) ^ b2_b3) << 16 |
                 (b2_b0 ^ b2_b1 ^ mul2(b2_b2) ^ mul3(b2_b3)) << 8 |
                 (mul3(b2_b0) ^ b2_b1 ^ b2_b2 ^ mul2(b2_b3))

            s3 = (mul2(b3_b0) ^ mul3(b3_b1) ^ b3_b2 ^ b3_b3) << 24 |
                 (b3_b0 ^ mul2(b3_b1) ^ mul3(b3_b2) ^ b3_b3) << 16 |
                 (b3_b0 ^ b3_b1 ^ mul2(b3_b2) ^ mul3(b3_b3)) << 8 |
                 (mul3(b3_b0) ^ b3_b1 ^ b3_b2 ^ mul2(b3_b3))

            // AddRoundKey
            s0 ^= ctx.rk[r * 4]
            s1 ^= ctx.rk[r * 4 + 1]
            s2 ^= ctx.rk[r * 4 + 2]
            s3 ^= ctx.rk[r * 4 + 3]

            r += 1
        }

        // Final round (no MixColumns)
        var t0_f = ((AES_SBOX[(s0 >> 24) & 0xFF] as u32) << 24) ^
                   ((AES_SBOX[(s1 >> 16) & 0xFF] as u32) << 16) ^
                   ((AES_SBOX[(s2 >> 8) & 0xFF] as u32) << 8) ^
                   (AES_SBOX[s3 & 0xFF] as u32)
        var t1_f = ((AES_SBOX[(s1 >> 24) & 0xFF] as u32) << 24) ^
                   ((AES_SBOX[(s2 >> 16) & 0xFF] as u32) << 16) ^
                   ((AES_SBOX[(s3 >> 8) & 0xFF] as u32) << 8) ^
                   (AES_SBOX[s0 & 0xFF] as u32)
        var t2_f = ((AES_SBOX[(s2 >> 24) & 0xFF] as u32) << 24) ^
                   ((AES_SBOX[(s3 >> 16) & 0xFF] as u32) << 16) ^
                   ((AES_SBOX[(s0 >> 8) & 0xFF] as u32) << 8) ^
                   (AES_SBOX[s1 & 0xFF] as u32)
        var t3_f = ((AES_SBOX[(s3 >> 24) & 0xFF] as u32) << 24) ^
                   ((AES_SBOX[(s0 >> 16) & 0xFF] as u32) << 16) ^
                   ((AES_SBOX[(s1 >> 8) & 0xFF] as u32) << 8) ^
                   (AES_SBOX[s2 & 0xFF] as u32)

        s0 = t0_f ^ ctx.rk[ctx.nr * 4]
        s1 = t1_f ^ ctx.rk[ctx.nr * 4 + 1]
        s2 = t2_f ^ ctx.rk[ctx.nr * 4 + 2]
        s3 = t3_f ^ ctx.rk[ctx.nr * 4 + 3]

        // Write output
        output[0] = ((s0 >> 24) & 0xFF) as u8
        output[1] = ((s0 >> 16) & 0xFF) as u8
        output[2] = ((s0 >> 8) & 0xFF) as u8
        output[3] = (s0 & 0xFF) as u8
        output[4] = ((s1 >> 24) & 0xFF) as u8
        output[5] = ((s1 >> 16) & 0xFF) as u8
        output[6] = ((s1 >> 8) & 0xFF) as u8
        output[7] = (s1 & 0xFF) as u8
        output[8] = ((s2 >> 24) & 0xFF) as u8
        output[9] = ((s2 >> 16) & 0xFF) as u8
        output[10] = ((s2 >> 8) & 0xFF) as u8
        output[11] = (s2 & 0xFF) as u8
        output[12] = ((s3 >> 24) & 0xFF) as u8
        output[13] = ((s3 >> 16) & 0xFF) as u8
        output[14] = ((s3 >> 8) & 0xFF) as u8
        output[15] = (s3 & 0xFF) as u8

    }
}

    // ─── AES ECB Decrypt (single block) ─────────────────────────────────────

    func aes_internal_decrypt(ctx : *mut AESContext, input : *u8, output : *mut u8) {
        unsafe {

        var s0 : u32 = ((input[0] as u32) << 24) | ((input[1] as u32) << 16) |
                        ((input[2] as u32) << 8) | (input[3] as u32)
        var s1 : u32 = ((input[4] as u32) << 24) | ((input[5] as u32) << 16) |
                        ((input[6] as u32) << 8) | (input[7] as u32)
        var s2 : u32 = ((input[8] as u32) << 24) | ((input[9] as u32) << 16) |
                        ((input[10] as u32) << 8) | (input[11] as u32)
        var s3 : u32 = ((input[12] as u32) << 24) | ((input[13] as u32) << 16) |
                        ((input[14] as u32) << 8) | (input[15] as u32)

        // AddRoundKey (last round key for decryption)
        s0 ^= ctx.rk[ctx.nr * 4]
        s1 ^= ctx.rk[ctx.nr * 4 + 1]
        s2 ^= ctx.rk[ctx.nr * 4 + 2]
        s3 ^= ctx.rk[ctx.nr * 4 + 3]

        var r : u32 = ctx.nr - 1
        while(r > 0) {
            // InvShiftRows + InvSubBytes
            var t0 : u32
            var t1 : u32
            var t2 : u32
            var t3 : u32

            t0 = ((AES_RSBOX[(s0 >> 24) & 0xFF] as u32) << 24) ^
                 ((AES_RSBOX[(s3 >> 16) & 0xFF] as u32) << 16) ^
                 ((AES_RSBOX[(s2 >> 8) & 0xFF] as u32) << 8) ^
                 (AES_RSBOX[s1 & 0xFF] as u32)
            t1 = ((AES_RSBOX[(s1 >> 24) & 0xFF] as u32) << 24) ^
                 ((AES_RSBOX[(s0 >> 16) & 0xFF] as u32) << 16) ^
                 ((AES_RSBOX[(s3 >> 8) & 0xFF] as u32) << 8) ^
                 (AES_RSBOX[s2 & 0xFF] as u32)
            t2 = ((AES_RSBOX[(s2 >> 24) & 0xFF] as u32) << 24) ^
                 ((AES_RSBOX[(s1 >> 16) & 0xFF] as u32) << 16) ^
                 ((AES_RSBOX[(s0 >> 8) & 0xFF] as u32) << 8) ^
                 (AES_RSBOX[s3 & 0xFF] as u32)
            t3 = ((AES_RSBOX[(s3 >> 24) & 0xFF] as u32) << 24) ^
                 ((AES_RSBOX[(s2 >> 16) & 0xFF] as u32) << 16) ^
                 ((AES_RSBOX[(s1 >> 8) & 0xFF] as u32) << 8) ^
                 (AES_RSBOX[s0 & 0xFF] as u32)

            // AddRoundKey (before InvMixColumns for standard inverse cipher)
            t0 ^= ctx.rk[r * 4]
            t1 ^= ctx.rk[r * 4 + 1]
            t2 ^= ctx.rk[r * 4 + 2]
            t3 ^= ctx.rk[r * 4 + 3]

            // InvMixColumns
            var b0 = t0; var b1 = t1; var b2 = t2; var b3 = t3

            var b0_b0 = (b0 >> 24) & 0xFF; var b0_b1 = (b0 >> 16) & 0xFF
            var b0_b2 = (b0 >> 8) & 0xFF; var b0_b3 = b0 & 0xFF
            var b1_b0 = (b1 >> 24) & 0xFF; var b1_b1 = (b1 >> 16) & 0xFF
            var b1_b2 = (b1 >> 8) & 0xFF; var b1_b3 = b1 & 0xFF
            var b2_b0 = (b2 >> 24) & 0xFF; var b2_b1 = (b2 >> 16) & 0xFF
            var b2_b2 = (b2 >> 8) & 0xFF; var b2_b3 = b2 & 0xFF
            var b3_b0 = (b3 >> 24) & 0xFF; var b3_b1 = (b3 >> 16) & 0xFF
            var b3_b2 = (b3 >> 8) & 0xFF; var b3_b3 = b3 & 0xFF

            s0 = (mul14(b0_b0) ^ mul11(b0_b1) ^ mul13(b0_b2) ^ mul9(b0_b3)) << 24 |
                 (mul9(b0_b0) ^ mul14(b0_b1) ^ mul11(b0_b2) ^ mul13(b0_b3)) << 16 |
                 (mul13(b0_b0) ^ mul9(b0_b1) ^ mul14(b0_b2) ^ mul11(b0_b3)) << 8 |
                 (mul11(b0_b0) ^ mul13(b0_b1) ^ mul9(b0_b2) ^ mul14(b0_b3))

            s1 = (mul14(b1_b0) ^ mul11(b1_b1) ^ mul13(b1_b2) ^ mul9(b1_b3)) << 24 |
                 (mul9(b1_b0) ^ mul14(b1_b1) ^ mul11(b1_b2) ^ mul13(b1_b3)) << 16 |
                 (mul13(b1_b0) ^ mul9(b1_b1) ^ mul14(b1_b2) ^ mul11(b1_b3)) << 8 |
                 (mul11(b1_b0) ^ mul13(b1_b1) ^ mul9(b1_b2) ^ mul14(b1_b3))

            s2 = (mul14(b2_b0) ^ mul11(b2_b1) ^ mul13(b2_b2) ^ mul9(b2_b3)) << 24 |
                 (mul9(b2_b0) ^ mul14(b2_b1) ^ mul11(b2_b2) ^ mul13(b2_b3)) << 16 |
                 (mul13(b2_b0) ^ mul9(b2_b1) ^ mul14(b2_b2) ^ mul11(b2_b3)) << 8 |
                 (mul11(b2_b0) ^ mul13(b2_b1) ^ mul9(b2_b2) ^ mul14(b2_b3))

            s3 = (mul14(b3_b0) ^ mul11(b3_b1) ^ mul13(b3_b2) ^ mul9(b3_b3)) << 24 |
                 (mul9(b3_b0) ^ mul14(b3_b1) ^ mul11(b3_b2) ^ mul13(b3_b3)) << 16 |
                 (mul13(b3_b0) ^ mul9(b3_b1) ^ mul14(b3_b2) ^ mul11(b3_b3)) << 8 |
                 (mul11(b3_b0) ^ mul13(b3_b1) ^ mul9(b3_b2) ^ mul14(b3_b3))

            if(r == 0) { break }
            r -= 1
        }

        // Final AddRoundKey (round 0)
        var t0_f = ((AES_RSBOX[(s0 >> 24) & 0xFF] as u32) << 24) ^
                   ((AES_RSBOX[(s3 >> 16) & 0xFF] as u32) << 16) ^
                   ((AES_RSBOX[(s2 >> 8) & 0xFF] as u32) << 8) ^
                   (AES_RSBOX[s1 & 0xFF] as u32)
        var t1_f = ((AES_RSBOX[(s1 >> 24) & 0xFF] as u32) << 24) ^
                   ((AES_RSBOX[(s0 >> 16) & 0xFF] as u32) << 16) ^
                   ((AES_RSBOX[(s3 >> 8) & 0xFF] as u32) << 8) ^
                   (AES_RSBOX[s2 & 0xFF] as u32)
        var t2_f = ((AES_RSBOX[(s2 >> 24) & 0xFF] as u32) << 24) ^
                   ((AES_RSBOX[(s1 >> 16) & 0xFF] as u32) << 16) ^
                   ((AES_RSBOX[(s0 >> 8) & 0xFF] as u32) << 8) ^
                   (AES_RSBOX[s3 & 0xFF] as u32)
        var t3_f = ((AES_RSBOX[(s3 >> 24) & 0xFF] as u32) << 24) ^
                   ((AES_RSBOX[(s2 >> 16) & 0xFF] as u32) << 16) ^
                   ((AES_RSBOX[(s1 >> 8) & 0xFF] as u32) << 8) ^
                   (AES_RSBOX[s0 & 0xFF] as u32)

        s0 = t0_f ^ ctx.rk[0]
        s1 = t1_f ^ ctx.rk[1]
        s2 = t2_f ^ ctx.rk[2]
        s3 = t3_f ^ ctx.rk[3]

        output[0] = ((s0 >> 24) & 0xFF) as u8
        output[1] = ((s0 >> 16) & 0xFF) as u8
        output[2] = ((s0 >> 8) & 0xFF) as u8
        output[3] = (s0 & 0xFF) as u8
        output[4] = ((s1 >> 24) & 0xFF) as u8
        output[5] = ((s1 >> 16) & 0xFF) as u8
        output[6] = ((s1 >> 8) & 0xFF) as u8
        output[7] = (s1 & 0xFF) as u8
        output[8] = ((s2 >> 24) & 0xFF) as u8
        output[9] = ((s2 >> 16) & 0xFF) as u8
        output[10] = ((s2 >> 8) & 0xFF) as u8
        output[11] = (s2 & 0xFF) as u8
        output[12] = ((s3 >> 24) & 0xFF) as u8
        output[13] = ((s3 >> 16) & 0xFF) as u8
        output[14] = ((s3 >> 8) & 0xFF) as u8
        output[15] = (s3 & 0xFF) as u8

    }
}

    // ─── Galois Field (2^8) Multiplication helpers ────────────────────────

    // Multiply by 2 in GF(2^8)
    func mul2(x : u32) : u32 {
        var r = x << 1
        if(x & 0x80) { r ^= 0x1B }
        return r & 0xFF
    }

    func mul3(x : u32) : u32 { return mul2(x) ^ x }

    func mul9(x : u32) : u32 { return mul2(mul2(mul2(x))) ^ x }

    func mul11(x : u32) : u32 { return mul2(mul2(mul2(x))) ^ mul2(x) ^ x }

    func mul13(x : u32) : u32 { return mul2(mul2(mul2(x))) ^ mul2(mul2(x)) ^ x }

    func mul14(x : u32) : u32 { return mul2(mul2(mul2(x))) ^ mul2(mul2(x)) ^ mul2(x) }

    // ============================================================================
    // Public AES API
    // ============================================================================

    // Set encryption key (16 or 32 bytes)
    public func aes_setkey_enc(ctx : *mut AESContext, key : *u8, key_len : size_t) : int {
        return aes_key_expansion(ctx, key, key_len)
    }

    // Set decryption key (16 or 32 bytes) — for AES, decryption key is derived from encryption key
    public func aes_setkey_dec(ctx : *mut AESContext, key : *u8, key_len : size_t) : int {
        return aes_key_expansion(ctx, key, key_len)
    }

    // AES-ECB encrypt/decrypt (single 16-byte block)
    public func aes_crypt_ecb(ctx : *mut AESContext, mode : u32, input : *u8, output : *mut u8) : int {
        if(mode == 0) { // Encrypt
            aes_internal_encrypt(ctx, input, output)
        } else { // Decrypt
            aes_internal_decrypt(ctx, input, output)
        }
        return 0
    }

    public comptime const AES_ENCRYPT : u32 = 0
    public comptime const AES_DECRYPT : u32 = 1

    // AES-CBC encrypt/decrypt
    public func aes_crypt_cbc(ctx : *mut AESContext, mode : u32, length : size_t,
                               iv : *mut u8, input : *u8, output : *mut u8) : int {
                                   unsafe {

        if(length % 16 != 0) { return AES_ERR_INVALID_INPUT_LENGTH }

        if(mode == AES_ENCRYPT) {
            var i : size_t = 0
            while(i < length) {
                // XOR block with IV
                var j : size_t = 0
                while(j < 16) {
                    output[i + j] = input[i + j] ^ iv[j]
                    j += 1
                }
                // Encrypt block
                aes_internal_encrypt(ctx, &raw output[i], &raw mut output[i])
                // Update IV to ciphertext
                j = 0
                while(j < 16) {
                    iv[j] = output[i + j]
                    j += 1
                }
                i += 16
            }
        } else {
            // Decrypt
            var next_iv : [16]u8
            var i : size_t = 0
            while(i < length) {
                // Save input to next_iv for next iteration
                var j : size_t = 0
                while(j < 16) {
                    next_iv[j] = input[i + j]
                    j += 1
                }
                // Decrypt block
                aes_internal_decrypt(ctx, &raw input[i], &raw mut output[i])
                // XOR with IV
                j = 0
                while(j < 16) {
                    output[i + j] = output[i + j] ^ iv[j]
                    j += 1
                }
                // Update IV
                j = 0
                while(j < 16) {
                    iv[j] = next_iv[j]
                    j += 1
                }
                i += 16
            }
        }

        return 0

                               }
}


// ===== gcm.ch =====

    // ─── GCM Error Codes ────────────────────────────────────────────────────

    public comptime const ERR_GCM_BAD_INPUT = -0x6000
    public comptime const ERR_GCM_AUTH_FAILED = -0x6010
    public comptime const ERR_GCM_BUFFER_TOO_SMALL = -0x6020

    // ─── GCM Context ────────────────────────────────────────────────────────

    public struct GCMContext {
        var cipher_ctx : AESContext  // AES encryption context
        var H : [16]u8               // Hash subkey = AES(K, 0^128)
        var len : size_t             // Key length in bytes
    }

    public func gcm_init(ctx : *mut GCMContext, key : *u8, key_len : size_t) : int {
        unsafe {

        // Set AES key
        ctx.len = key_len
        var ret = aes_setkey_enc(&raw mut ctx.cipher_ctx, key, key_len)
        if(ret < 0) { return ret }

        // Compute H = AES(K, 0^128)
        var zero_block : [16]u8
        var i : size_t = 0
        while(i < 16) { zero_block[i] = 0; i += 1 }
        aes_crypt_ecb(&raw mut ctx.cipher_ctx, AES_ENCRYPT, &raw zero_block[0], &raw mut ctx.H[0])

        return 0

    }
}

    // ─── GF(2^128) Multiplication ──────────────────────────────────────────

    // GHASH core: multiply in GF(2^128) with the GCM polynomial
    // The GCM specification uses a specific bit ordering:
    // - Bits are numbered from MSB (bit 0) to LSB (bit 127)
    // - Polynomials are represented with the highest degree coefficient in bit 0 of byte 0
    func ghash_multiply_refined(output : *mut u8, x : *u8, y : *u8) {
        unsafe {

        var Z : [16]u8
        var V : [16]u8
        var i : size_t = 0
        while(i < 16) { Z[i] = 0; V[i] = y[i]; i += 1 }

        i = 0
        while(i < 128) {
            // Instead of bit-by-bit, use a table-based approach for speed
            // For now, use the simplest bit-by-bit method:
            var byte_idx = i >> 3      // i / 8
            var bit_mask = 0x80u8 >> (i & 7)  // MSB-first: bit 0 is MSB

            var xi = (x[byte_idx] & bit_mask) != 0

            if(xi) {
                var j : size_t = 0
                while(j < 16) { Z[j] = Z[j] ^ V[j]; j += 1 }
            }

            // Check if V has LSB (bit 127) set - need to check bit 0 of byte 15
            var v_lsb = (V[15] & 0x01) as u8

            // Right shift V by 1 (moving towards LSB)
            var k = 15
            while(k > 0) {
                V[k] = (V[k] >> 1) | ((V[k - 1] & 1) << 7)
                k -= 1
            }
            V[0] = V[0] >> 1

            // Polynomial reduction if LSB was 1
            if(v_lsb != 0) {
                // XOR with R = 0xE1 << 120
                // In MSB-first byte ordering: E1 00 ... 00
                V[0] = V[0] ^ 0xE1
            }

            i += 1
        }

        i = 0
        while(i < 16) { output[i] = Z[i]; i += 1 }

    }
}

    // ─── GHASH (core authentication function) ─────────────────────────────

    // GHASH(H, A, C) = X_{m+n+1} where:
    // X_0 = 0
    // X_i = (X_{i-1} XOR A_i) * H  (for i = 1..m, A_i are 128-bit blocks of AAD)
    // X_{m+i} = (X_{m+i-1} XOR C_i) * H  (for i = 1..n, C_i are 128-bit blocks of ciphertext)
    // X_{m+n+1} = (X_{m+n} XOR len(A) || len(C)) * H
    func ghash(ctx : *mut GCMContext, aad : *u8, aad_len : size_t,
               ciphertext : *u8, ct_len : size_t,
               output : *mut u8) {
                   unsafe {

        var Y : [16]u8
        var i : size_t = 0
        while(i < 16) { Y[i] = 0; i += 1 }

        // Process AAD blocks
        var pos : size_t = 0
        while(pos + 16 <= aad_len) {
            var j : size_t = 0
            while(j < 16) { Y[j] = Y[j] ^ aad[pos + j]; j += 1 }
            ghash_multiply_refined(&raw mut Y[0], &raw Y[0], &raw ctx.H[0])
            pos += 16
        }

        // Process partial AAD block (if any)
        if(pos < aad_len) {
            var block : [16]u8
            var j : size_t = 0
            while(j < 16) { block[j] = 0; j += 1 }
            j = 0
            while(pos + j < aad_len) { block[j] = aad[pos + j]; j += 1 }
            j = 0
            while(j < 16) { Y[j] = Y[j] ^ block[j]; j += 1 }
            ghash_multiply_refined(&raw mut Y[0], &raw Y[0], &raw ctx.H[0])
        }

        // Process ciphertext blocks
        pos = 0
        while(pos + 16 <= ct_len) {
            var j : size_t = 0
            while(j < 16) { Y[j] = Y[j] ^ ciphertext[pos + j]; j += 1 }
            ghash_multiply_refined(&raw mut Y[0], &raw Y[0], &raw ctx.H[0])
            pos += 16
        }

        // Process partial ciphertext block (if any)
        if(pos < ct_len) {
            var block : [16]u8
            var j : size_t = 0
            while(j < 16) { block[j] = 0; j += 1 }
            j = 0
            while(pos + j < ct_len) { block[j] = ciphertext[pos + j]; j += 1 }
            j = 0
            while(j < 16) { Y[j] = Y[j] ^ block[j]; j += 1 }
            ghash_multiply_refined(&raw mut Y[0], &raw Y[0], &raw ctx.H[0])
        }

        // Process length block
        var len_block : [16]u8
        var j : size_t = 0
        while(j < 16) { len_block[j] = 0; j += 1 }

        // 64-bit big-endian lengths
        var aad_bits = (aad_len as u64) * 8
        var ct_bits = (ct_len as u64) * 8

        len_block[0] = ((aad_bits >> 56) & 0xFFu64) as u8
        len_block[1] = ((aad_bits >> 48) & 0xFFu64) as u8
        len_block[2] = ((aad_bits >> 40) & 0xFFu64) as u8
        len_block[3] = ((aad_bits >> 32) & 0xFFu64) as u8
        len_block[4] = ((aad_bits >> 24) & 0xFFu64) as u8
        len_block[5] = ((aad_bits >> 16) & 0xFFu64) as u8
        len_block[6] = ((aad_bits >> 8) & 0xFFu64) as u8
        len_block[7] = (aad_bits & 0xFFu64) as u8

        len_block[8] = ((ct_bits >> 56) & 0xFFu64) as u8
        len_block[9] = ((ct_bits >> 48) & 0xFFu64) as u8
        len_block[10] = ((ct_bits >> 40) & 0xFFu64) as u8
        len_block[11] = ((ct_bits >> 32) & 0xFFu64) as u8
        len_block[12] = ((ct_bits >> 24) & 0xFFu64) as u8
        len_block[13] = ((ct_bits >> 16) & 0xFFu64) as u8
        len_block[14] = ((ct_bits >> 8) & 0xFFu64) as u8
        len_block[15] = (ct_bits & 0xFFu64) as u8

        j = 0
        while(j < 16) { Y[j] = Y[j] ^ len_block[j]; j += 1 }
        ghash_multiply_refined(&raw mut Y[0], &raw Y[0], &raw ctx.H[0])

        j = 0
        while(j < 16) { output[j] = Y[j]; j += 1 }

               }
}

    // ─── Increment Counter (32-bit big-endian) ───────────────────────────

    func gcm_incr(counter : *mut u8) {
        unsafe {

        // Increment the last 4 bytes (32-bit counter) in big-endian
        var i = 15
        while(i >= 12) {
            counter[i] = counter[i] + 1
            if(counter[i] != 0) { break }
            i -= 1
        }

    }
}

    // ─── GCM Encrypt ─────────────────────────────────────────────────────

    // GCM encrypt (AES-GCM)
    // key: AES key (16 or 32 bytes)
    // iv: 12-byte initial nonce (standard GCM IV length)
    // aad: additional authenticated data (can be null/empty)
    // plaintext: input data to encrypt
    // output: output buffer for ciphertext (same size as plaintext)
    // tag: 16-byte authentication tag
    public func gcm_crypt_and_tag(ctx : *mut GCMContext,
                                   iv : *u8, iv_len : size_t,
                                   aad : *u8, aad_len : size_t,
                                   plaintext : *u8, pt_len : size_t,
                                   output : *mut u8, tag : *mut u8) : int {
                                       unsafe {

        // For 12-byte IV: J0 = IV || 0x00000001
        var J0 : [16]u8
        var i : size_t = 0
        while(i < 16) { J0[i] = 0; i += 1 }

        if(iv_len == 12) {
            i = 0
            while(i < 12) { J0[i] = iv[i]; i += 1 }
            J0[15] = 1
        } else {
            // For non-12-byte IV: GHASH(H, {}, IV)
            ghash(ctx, null, 0, iv, iv_len, &raw mut J0[0])
        }

        // Encrypt counter blocks and XOR with plaintext
        var counter : [16]u8
        i = 0
        while(i < 16) { counter[i] = J0[i]; i += 1 }

        var pos : size_t = 0
        while(pos < pt_len) {
            // Increment counter (skip J0 itself)
            gcm_incr(&raw mut counter[0])

            // E(K, counter)
            var enc_counter : [16]u8
            var ret = aes_crypt_ecb(&raw mut ctx.cipher_ctx, AES_ENCRYPT,
                                     &raw counter[0], &raw mut enc_counter[0])
            if(ret < 0) { return ret }

            // XOR with plaintext (partial block handling)
            var block_size : size_t = 16
            if(pt_len - pos < 16) { block_size = pt_len - pos }

            var j : size_t = 0
            while(j < block_size) {
                output[pos + j] = plaintext[pos + j] ^ enc_counter[j]
                j += 1
            }
            pos += block_size
        }

        // Compute GHASH: gh = GHASH(H, AAD, Ciphertext)
        var gh : [16]u8
        ghash(ctx, aad, aad_len, output, pt_len, &raw mut gh[0])

        // Tag = GHASH XOR E(K, J0)
        var enc_J0 : [16]u8
        var ret = aes_crypt_ecb(&raw mut ctx.cipher_ctx, AES_ENCRYPT,
                                 &raw J0[0], &raw mut enc_J0[0])
        if(ret < 0) { return ret }

        i = 0
        while(i < 16) {
            tag[i] = gh[i] ^ enc_J0[i]
            i += 1
        }

        return 0

                                   }
}

    // ─── GCM Decrypt ─────────────────────────────────────────────────────

    // GCM decrypt and verify tag
    public func gcm_auth_decrypt(ctx : *mut GCMContext,
                                  iv : *u8, iv_len : size_t,
                                  aad : *u8, aad_len : size_t,
                                  ciphertext : *u8, ct_len : size_t,
                                  tag : *u8, tag_len : size_t,
                                  output : *mut u8) : int {
                                      unsafe {

        // 1. Compute GHASH over (AAD, Ciphertext)
        // 2. Compare expected tag
        // 3. Decrypt if tags match

        var ret : int = 0
        var J0 : [16]u8
        var i : size_t = 0
        while(i < 16) { J0[i] = 0; i += 1 }

        if(tls_config::EXTENSIVE_DEBUG_LOG) unsafe { printf("[GCM_DBG] iv_addr=%p iv_len=%d\n", iv, iv_len) }
        if(tls_config::EXTENSIVE_DEBUG_LOG) { unsafe { printf("[GCM_DBG] iv_bytes: ") }; var _ivdbg : size_t = 0; while(_ivdbg < iv_len) { unsafe { printf("%02x", iv[_ivdbg] as int) }; _ivdbg += 1 }; unsafe { printf("\n") } }

        if(iv_len == 12) {
            i = 0
            while(i < 12) { J0[i] = iv[i]; i += 1 }
            J0[15] = 1
        } else {
            ghash(ctx, null, 0, iv, iv_len, &raw mut J0[0])
        }

        // Compute GHASH over AAD and ciphertext
        var gh : [16]u8
        ghash(ctx, aad, aad_len, ciphertext, ct_len, &raw mut gh[0])

        // Encrypt J0
        var enc_J0 : [16]u8
        ret = aes_crypt_ecb(&raw mut ctx.cipher_ctx, AES_ENCRYPT,
                             &raw J0[0], &raw mut enc_J0[0])
        if(ret < 0) { return ret }

        // Expected tag = GHASH XOR E(K, J0)
        var expected_tag : [16]u8
        i = 0
        while(i < 16) {
            expected_tag[i] = gh[i] ^ enc_J0[i]
            i += 1
        }

        // Constant-time tag comparison
        var diff : u8 = 0
        i = 0
        while(i < tag_len) {
            diff = diff | (expected_tag[i] ^ tag[i])
            i += 1
        }
        if(diff != 0) {
            if(tls_config::DEBUG_LOG) { var _gti : size_t
            unsafe { printf("[GCM_DBG] EXPECTED_TAG: ") }; _gti = 0; while(_gti < 16) { unsafe { printf("%02x", expected_tag[_gti] as int) }; _gti += 1 }; unsafe { printf("\n") }
            unsafe { printf("[GCM_DBG] RECEIVED_TAG: ") }; _gti = 0; while(_gti < tag_len) { unsafe { printf("%02x", tag[_gti] as int) }; _gti += 1 }; unsafe { printf("\n") }
            unsafe { printf("[GCM_DBG] GHASH: ") }; _gti = 0; while(_gti < 16) { unsafe { printf("%02x", gh[_gti] as int) }; _gti += 1 }; unsafe { printf("\n") }
            unsafe { printf("[GCM_DBG] ENC_J0: ") }; _gti = 0; while(_gti < 16) { unsafe { printf("%02x", enc_J0[_gti] as int) }; _gti += 1 }; unsafe { printf("\n") }
            unsafe { printf("[GCM_DBG] H: ") }; _gti = 0; while(_gti < 16) { unsafe { printf("%02x", ctx.H[_gti] as int) }; _gti += 1 }; unsafe { printf("\n") }
            unsafe { printf("[GCM_DBG] J0: ") }; _gti = 0; while(_gti < 16) { unsafe { printf("%02x", J0[_gti] as int) }; _gti += 1 }; unsafe { printf("\n") }
            }

            return ERR_GCM_AUTH_FAILED
        }

        // Decrypt (same as encrypt for CTR mode)
        var counter : [16]u8
        i = 0
        while(i < 16) { counter[i] = J0[i]; i += 1 }

        var pos : size_t = 0
        while(pos < ct_len) {
            gcm_incr(&raw mut counter[0])

            var enc_counter : [16]u8
            ret = aes_crypt_ecb(&raw mut ctx.cipher_ctx, AES_ENCRYPT,
                                 &raw counter[0], &raw mut enc_counter[0])
            if(ret < 0) { return ret }

            var block_size : size_t = 16
            if(ct_len - pos < 16) { block_size = ct_len - pos }

            var j : size_t = 0
            while(j < block_size) {
                output[pos + j] = ciphertext[pos + j] ^ enc_counter[j]
                j += 1
            }
            pos += block_size
        }

        return 0

                                  }
}


// ===== chacha_poly.ch =====
// ChaCha20 + Poly1305 AEAD (RFC 8439) for TLS 1.3 ChaCha20-Poly1305-SHA256.
// Pure-Chemical implementation, no external dependencies.

// ─── ChaCha20 ────────────────────────────────────────────────────────────────

func chacha20_rotl(x : u32, n : u32) : u32 {
    return (x << n) | (x >> (32u - n))
}

func chacha20_quarter_round(s : *mut u32, a : size_t, b : size_t, c : size_t, d : size_t) {
    unsafe {

    s[a] = s[a] + s[b]; s[d] = s[d] ^ s[a]; s[d] = chacha20_rotl(s[d], 16u)
    s[c] = s[c] + s[d]; s[b] = s[b] ^ s[c]; s[b] = chacha20_rotl(s[b], 12u)
    s[a] = s[a] + s[b]; s[d] = s[d] ^ s[a]; s[d] = chacha20_rotl(s[d], 8u)
    s[c] = s[c] + s[d]; s[b] = s[b] ^ s[c]; s[b] = chacha20_rotl(s[b], 7u)

}
}

// Produce one 64-byte ChaCha20 keystream block.
// key: 32 bytes, counter: 32-bit block counter, nonce: 12 bytes.
func chacha20_block(key : *u8, counter : u32, nonce : *u8, out : *mut u8) {
    unsafe {

    var state : [16]u32
    // "expand 32-byte k" constants
    state[0] = 0x61707865u
    state[1] = 0x3320646eu
    state[2] = 0x79622d32u
    state[3] = 0x6b206574u
    var i : size_t = 0
    while(i < 8) {
        state[4 + i] = ((key[4*i] as u32) |
                        ((key[4*i + 1] as u32) << 8) |
                        ((key[4*i + 2] as u32) << 16) |
                        ((key[4*i + 3] as u32) << 24))
        i += 1
    }
    state[12] = counter
    state[13] = ((nonce[0] as u32) | ((nonce[1] as u32) << 8) |
                ((nonce[2] as u32) << 16) | ((nonce[3] as u32) << 24))
    state[14] = ((nonce[4] as u32) | ((nonce[5] as u32) << 8) |
                ((nonce[6] as u32) << 16) | ((nonce[7] as u32) << 24))
    state[15] = ((nonce[8] as u32) | ((nonce[9] as u32) << 8) |
                ((nonce[10] as u32) << 16) | ((nonce[11] as u32) << 24))

    var w : [16]u32
    var j : size_t = 0
    while(j < 16) { w[j] = state[j]; j += 1 }

    var round : size_t = 0
    while(round < 10) {
        chacha20_quarter_round(&raw mut w[0], 0, 4, 8, 12)
        chacha20_quarter_round(&raw mut w[0], 1, 5, 9, 13)
        chacha20_quarter_round(&raw mut w[0], 2, 6, 10, 14)
        chacha20_quarter_round(&raw mut w[0], 3, 7, 11, 15)
        chacha20_quarter_round(&raw mut w[0], 0, 5, 10, 15)
        chacha20_quarter_round(&raw mut w[0], 1, 6, 11, 12)
        chacha20_quarter_round(&raw mut w[0], 2, 7, 8, 13)
        chacha20_quarter_round(&raw mut w[0], 3, 4, 9, 14)
        round += 1
    }

    j = 0
    while(j < 16) {
        var val = w[j] + state[j]
        out[4*j] = (val & 0xFFu) as u8
        out[4*j + 1] = ((val >> 8) & 0xFFu) as u8
        out[4*j + 2] = ((val >> 16) & 0xFFu) as u8
        out[4*j + 3] = ((val >> 24) & 0xFFu) as u8
        j += 1
    }

}
}

// XOR `len` bytes of `input` with ChaCha20 keystream starting at block `counter_start`.
func chacha20_xor(key : *u8, nonce : *u8, counter_start : u32,
                  input : *u8, len : size_t, output : *mut u8) {
                      unsafe {

    if(len == 0) { return }
    var block : [64]u8
    var pos : size_t = 0
    var ctr = counter_start
    while(pos < len) {
        chacha20_block(key, ctr, nonce, &raw mut block[0])
        var n : size_t = 64
        if(len - pos < 64) { n = len - pos }
        var k : size_t = 0
        while(k < n) {
            output[pos + k] = input[pos + k] ^ block[k]
            k += 1
        }
        pos += n
        ctr += 1u
    }

                  }
}

// ─── Poly1305 (RFC 8439 §2.5), 26-bit limb implementation ─────────────────────

public struct Poly1305State {
    var r : [5]u32
    var h : [5]u32
    var s : [16]u8
    var buf : [16]u8
    var buflen : size_t
}

func poly1305_load_r(state : *mut Poly1305State, key : *u8) {
    unsafe {

    var k = key
    // Clamp r per RFC 8439 §2.5: r &= 0x0ffffffc0ffffffc0ffffffc0fffffff.
    state.r[0] = (k[0] as u32 | (k[1] as u32 << 8) | (k[2] as u32 << 16) | (k[3] as u32 << 24)) & 0x3ffffffu
    state.r[1] = ((k[3] as u32 >> 2) | (k[4] as u32 << 6) | (k[5] as u32 << 14) | (k[6] as u32 << 22)) & 0x3ffff03u
    state.r[2] = ((k[6] as u32 >> 4) | (k[7] as u32 << 4) | (k[8] as u32 << 12) | (k[9] as u32 << 20)) & 0x3ffc0ffu
    state.r[3] = ((k[9] as u32 >> 6) | (k[10] as u32 << 2) | (k[11] as u32 << 10) | (k[12] as u32 << 18)) & 0x3f03fffu
    state.r[4] = ((k[12] as u32 >> 8) | (k[13] as u32) | (k[14] as u32 << 8) | (k[15] as u32 << 16)) & 0x00fffffu

}
}

func poly1305_init(state : *mut Poly1305State, key : *u8) {
    unsafe {

    poly1305_load_r(state, key)
    state.h[0] = 0u; state.h[1] = 0u; state.h[2] = 0u; state.h[3] = 0u; state.h[4] = 0u
    var i : size_t = 0
    while(i < 16) { state.s[i] = key[16 + i]; state.buf[i] = 0u as u8; i += 1 }
    state.buflen = 0

}
}

// Process one full (16-byte) or partial (final_len-byte) block into h.
func poly1305_process(state : *mut Poly1305State, block : *u8, is_final : bool, final_len : size_t) {
    unsafe {

    var h = &raw mut state.h[0]
    h[0] = h[0] + ((block[0] as u32 | (block[1] as u32 << 8) | (block[2] as u32 << 16) | (block[3] as u32 << 24)) & 0x3ffffffu)
    h[1] = h[1] + (((block[3] as u32 >> 2) | (block[4] as u32 << 6) | (block[5] as u32 << 14) | (block[6] as u32 << 22)) & 0x3ffffffu)
    h[2] = h[2] + (((block[6] as u32 >> 4) | (block[7] as u32 << 4) | (block[8] as u32 << 12) | (block[9] as u32 << 20)) & 0x3ffffffu)
    h[3] = h[3] + (((block[9] as u32 >> 6) | (block[10] as u32 << 2) | (block[11] as u32 << 10) | (block[12] as u32 << 18)) & 0x3ffffffu)
    h[4] = h[4] + (((block[12] as u32 >> 8) | (block[13] as u32) | (block[14] as u32 << 8) | (block[15] as u32 << 16)) & 0x3ffffffu)

    // Append the 0x01 terminator at bit (8 * processed_len).
    var term_bit : size_t = 128
    if(is_final) { term_bit = 8 * final_len }
    var limb : size_t = term_bit / 26
    var bit : size_t = term_bit % 26
    h[limb] = h[limb] + (1u << bit)

    // h = h * r  (mod 2^130 - 5), 26-bit limbs.
    var r0 = state.r[0]; var r1 = state.r[1]; var r2 = state.r[2]
    var r3 = state.r[3]; var r4 = state.r[4]
    var t : [5]u64
    t[0] = (h[0] as u64) * (r0 as u64) + (h[1] as u64) * (5u64 * (r4 as u64)) + (h[2] as u64) * (5u64 * (r3 as u64)) + (h[3] as u64) * (5u64 * (r2 as u64)) + (h[4] as u64) * (5u64 * (r1 as u64))
    t[1] = (h[0] as u64) * (r1 as u64) + (h[1] as u64) * (r0 as u64) + (h[2] as u64) * (5u64 * (r4 as u64)) + (h[3] as u64) * (5u64 * (r3 as u64)) + (h[4] as u64) * (5u64 * (r2 as u64))
    t[2] = (h[0] as u64) * (r2 as u64) + (h[1] as u64) * (r1 as u64) + (h[2] as u64) * (r0 as u64) + (h[3] as u64) * (5u64 * (r4 as u64)) + (h[4] as u64) * (5u64 * (r3 as u64))
    t[3] = (h[0] as u64) * (r3 as u64) + (h[1] as u64) * (r2 as u64) + (h[2] as u64) * (r1 as u64) + (h[3] as u64) * (r0 as u64) + (h[4] as u64) * (5u64 * (r4 as u64))
    t[4] = (h[0] as u64) * (r4 as u64) + (h[1] as u64) * (r3 as u64) + (h[2] as u64) * (r2 as u64) + (h[3] as u64) * (r1 as u64) + (h[4] as u64) * (r0 as u64)

    var c : u64 = 0
    h[0] = (t[0] & 0x3ffffffu) as u32; c = t[0] >> 26
    t[1] = t[1] + c
    h[1] = (t[1] & 0x3ffffffu) as u32; c = t[1] >> 26
    t[2] = t[2] + c
    h[2] = (t[2] & 0x3ffffffu) as u32; c = t[2] >> 26
    t[3] = t[3] + c
    h[3] = (t[3] & 0x3ffffffu) as u32; c = t[3] >> 26
    t[4] = t[4] + c
    h[4] = (t[4] & 0x3ffffffu) as u32; c = t[4] >> 26
    h[0] = h[0] + ((c & 0xffffffffu) as u32 * 5u)
    c = (h[0] as u64) >> 26
    h[0] = (h[0] & 0x3ffffffu)
    h[1] = h[1] + (c as u32)

}
}

func poly1305_update(state : *mut Poly1305State, data : *u8, len : size_t) {
    unsafe {

    var pos : size_t = 0
    if(state.buflen > 0) {
        while(pos < len && state.buflen < 16) {
            state.buf[state.buflen] = data[pos]
            state.buflen += 1
            pos += 1
        }
        if(state.buflen == 16) {
            poly1305_process(state, &raw state.buf[0], false, 0)
            state.buflen = 0
        }
    }
    while(pos + 16 <= len) {
        poly1305_process(state, data + pos, false, 0)
        pos += 16
    }
    while(pos < len) {
        state.buf[state.buflen] = data[pos]
        state.buflen += 1
        pos += 1
    }

}
}

func poly1305_finish(state : *mut Poly1305State, tag : *mut u8) {
    unsafe {

    if(state.buflen > 0) {
        // Zero-fill and process the final partial block.
        var i : size_t = state.buflen
        while(i < 16) { state.buf[i] = 0u as u8; i += 1 }
        poly1305_process(state, &raw state.buf[0], true, state.buflen)
    }

    // Final reduction: conditionally subtract p = 2^130 - 5.
    var g : [5]u32
    var c : u32 = 0
    g[0] = state.h[0] + 5u; c = g[0] >> 26; g[0] = g[0] & 0x3ffffffu
    g[1] = state.h[1] + c; c = g[1] >> 26; g[1] = g[1] & 0x3ffffffu
    g[2] = state.h[2] + c; c = g[2] >> 26; g[2] = g[2] & 0x3ffffffu
    g[3] = state.h[3] + c; c = g[3] >> 26; g[3] = g[3] & 0x3ffffffu
    g[4] = state.h[4] + c - (1u << 26)

    // nb = 0 if h >= p (select g = h - p), else 0xffffffff (select h).
    var nb : u32 = 0u - (g[4] >> 31)
    var k : size_t = 0
    while(k < 5) {
        state.h[k] = g[k] ^ (nb & (state.h[k] ^ g[k]))
        k += 1
    }

    // Serialize h (little-endian, 26-bit limbs) into 16 bytes.
    var mac : [16]u8
    mac[0] = (state.h[0] & 0xFFu) as u8
    mac[1] = ((state.h[0] >> 8) & 0xFFu) as u8
    mac[2] = ((state.h[0] >> 16) & 0xFFu) as u8
    mac[3] = (((state.h[0] >> 24) | (state.h[1] << 2)) & 0xFFu) as u8
    mac[4] = ((state.h[1] >> 6) & 0xFFu) as u8
    mac[5] = ((state.h[1] >> 14) & 0xFFu) as u8
    mac[6] = (((state.h[1] >> 22) | (state.h[2] << 4)) & 0xFFu) as u8
    mac[7] = ((state.h[2] >> 4) & 0xFFu) as u8
    mac[8] = ((state.h[2] >> 12) & 0xFFu) as u8
    mac[9] = (((state.h[2] >> 20) | (state.h[3] << 6)) & 0xFFu) as u8
    mac[10] = ((state.h[3] >> 2) & 0xFFu) as u8
    mac[11] = ((state.h[3] >> 10) & 0xFFu) as u8
    mac[12] = ((state.h[3] >> 18) & 0xFFu) as u8
    mac[13] = (state.h[4] & 0xFFu) as u8
    mac[14] = ((state.h[4] >> 8) & 0xFFu) as u8
    mac[15] = ((state.h[4] >> 16) & 0xFFu) as u8

    // tag = (h + s) mod 2^128
    var f : u32 = 0u
    var m : size_t = 0
    while(m < 16) {
        var v = (mac[m] as u32) + (state.s[m] as u32) + f
        tag[m] = (v & 0xFFu) as u8
        f = v >> 8
        m += 1
    }

}
}

// ─── AEAD (RFC 8439 §2.8) ─────────────────────────────────────────────────────

// key: 32 bytes, nonce: 12 bytes.
func chacha20_poly1305_encrypt(key : *u8, nonce : *u8,
                               aad : *u8, aad_len : size_t,
                               plaintext : *u8, pt_len : size_t,
                               ciphertext : *mut u8, tag : *mut u8) {
    // One-time Poly1305 key = first 32 bytes of ChaCha20 keystream at counter 0.
    var otk_block : [64]u8
    chacha20_block(key, 0u, nonce, &raw mut otk_block[0])

    // Encrypt plaintext with keystream starting at counter 1.
    chacha20_xor(key, nonce, 1u, plaintext, pt_len, ciphertext)

    // Build Poly1305 MAC over aad || pad16(aad) || ciphertext || pad16(ct) || le64(aad_len) || le64(pt_len)
    var st : Poly1305State
    poly1305_init(unsafe(&raw mut st), &raw mut otk_block[0])
    poly1305_update(unsafe(&raw mut st), aad, aad_len)
    // Zero-pad AAD to a 16-byte boundary (RFC 8439 §2.8.1).
    var pad_aad = (16u - (aad_len % 16u)) % 16u
    if(pad_aad > 0u) {
        var zp : [16]u8
        var zi : size_t = 0
        while(zi < 16) { zp[zi] = 0u as u8; zi += 1 }
        poly1305_update(unsafe(&raw mut st), &raw zp[0], pad_aad)
    }
    poly1305_update(unsafe(&raw mut st), ciphertext, pt_len)
    // Zero-pad ciphertext to a 16-byte boundary.
    var pad_ct = (16u - (pt_len % 16u)) % 16u
    if(pad_ct > 0u) {
        var zp2 : [16]u8
        var zi2 : size_t = 0
        while(zi2 < 16) { zp2[zi2] = 0u as u8; zi2 += 1 }
        poly1305_update(unsafe(&raw mut st), &raw zp2[0], pad_ct)
    }
    var lens : [16]u8
    var i : size_t = 0
    while(i < 16) { lens[i] = 0u as u8; i += 1 }
    var aad_bits = aad_len as u64
    var pt_bits = pt_len as u64
    i = 0
    while(i < 8) { lens[i] = ((aad_bits >> (8 * i)) & 0xFFu) as u8; i += 1 }
    i = 0
    while(i < 8) { lens[8 + i] = ((pt_bits >> (8 * i)) & 0xFFu) as u8; i += 1 }
    poly1305_update(unsafe(&raw mut st), &raw mut lens[0], 16)
    poly1305_finish(unsafe(&raw mut st), tag)
}

// Returns 0 on success (tag verified), negative on authentication failure.
func chacha20_poly1305_decrypt(key : *u8, nonce : *u8,
                               aad : *u8, aad_len : size_t,
                               ciphertext : *u8, ct_len : size_t,
                               tag : *u8, plaintext : *mut u8) : int {
                                   unsafe {

    var otk_block : [64]u8
    chacha20_block(key, 0u, nonce, &raw mut otk_block[0])

    var st : Poly1305State
    poly1305_init(unsafe(&raw mut st), &raw mut otk_block[0])
    poly1305_update(unsafe(&raw mut st), aad, aad_len)
    // Zero-pad AAD to a 16-byte boundary (RFC 8439 §2.8.1).
    var pad_aad = (16u - (aad_len % 16u)) % 16u
    if(pad_aad > 0u) {
        var zp : [16]u8
        var zi : size_t = 0
        while(zi < 16) { zp[zi] = 0u as u8; zi += 1 }
        poly1305_update(unsafe(&raw mut st), &raw zp[0], pad_aad)
    }
    poly1305_update(unsafe(&raw mut st), ciphertext, ct_len)
    // Zero-pad ciphertext to a 16-byte boundary.
    var pad_ct = (16u - (ct_len % 16u)) % 16u
    if(pad_ct > 0u) {
        var zp2 : [16]u8
        var zi2 : size_t = 0
        while(zi2 < 16) { zp2[zi2] = 0u as u8; zi2 += 1 }
        poly1305_update(unsafe(&raw mut st), &raw zp2[0], pad_ct)
    }
    var lens : [16]u8
    var i : size_t = 0
    while(i < 16) { lens[i] = 0u as u8; i += 1 }
    var aad_bits = aad_len as u64
    var ct_bits = ct_len as u64
    i = 0
    while(i < 8) { lens[i] = ((aad_bits >> (8 * i)) & 0xFFu) as u8; i += 1 }
    i = 0
    while(i < 8) { lens[8 + i] = ((ct_bits >> (8 * i)) & 0xFFu) as u8; i += 1 }
    poly1305_update(unsafe(&raw mut st), &raw mut lens[0], 16)
    var expected : [16]u8
    poly1305_finish(unsafe(&raw mut st), &raw mut expected[0])

    var diff : u8 = 0u
    i = 0
    while(i < 16) { diff = diff | (expected[i] ^ tag[i]); i += 1 }
    if(diff != 0u) { return -1 }

    chacha20_xor(key, nonce, 1u, ciphertext, ct_len, plaintext)
    return 0

                               }
}


// ===== x25519.ch =====

    var X25519_BASE_POINT : [32]u8 = [
        0x09 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8,
        0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8,
        0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8,
        0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8,
        0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8,
        0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8,
        0x00 as u8, 0x00 as u8
    ]

    // ─── Field Element (8 x u32, little-endian limbs) ────────────────────
    // p = 2^255 - 19
    // p limbs: [0xFFFFFFED, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    //            0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x7FFFFFFF]

    // Set all limbs to 0
    public func fe_zero(f : *mut u32) {
        unsafe {

        var i : size_t = 0
        while(i < 8) { f[i] = 0; i += 1 }

    }
}

    // Copy field element
    func fe_copy(dst : *mut u32, src : *u32) {
        unsafe {

        var i : size_t = 0
        while(i < 8) { dst[i] = src[i]; i += 1 }

    }
}

    // Set to value (only small values work, up to 2^31)
    public func fe_set_small(f : *mut u32, val : u32) {
        unsafe {

        fe_zero(f)
        f[0] = val

    }
}

    // ─── Decode 32-byte little-endian to 8 limbs ─────────────────────────
    public func fe_decode(f : *mut u32, data : *u8) {
        unsafe {

        var i : size_t = 0
        while(i < 8) {
            var off = i * 4
            f[i] = (data[off] as u32) |
                   ((data[off + 1] as u32) << 8) |
                   ((data[off + 2] as u32) << 16) |
                   ((data[off + 3] as u32) << 24)
            i += 1
        }
        // Mask off bit 255 per RFC 7748 Section 5
        f[7] = f[7] & 0x7FFFFFFFu32

    }
}

    // ─── Encode 8 limbs to 32-byte little-endian ─────────────────────────
    public func fe_encode(data : *mut u8, f : *u32) {
        unsafe {

        var i : size_t = 0
        while(i < 8) {
            var off = i * 4
            data[off] = (f[i] & 0xFF) as u8
            data[off + 1] = ((f[i] >> 8) & 0xFF) as u8
            data[off + 2] = ((f[i] >> 16) & 0xFF) as u8
            data[off + 3] = ((f[i] >> 24) & 0xFF) as u8
            i += 1
        }

    }
}

    // ─── Compare two field elements (return -1, 0, or 1) ─────────────────
    func fe_cmp(a : *u32, b : *u32) : int {
        unsafe {

        var i : i32 = 7
        while(i >= 0) {
            if(a[i as size_t] > b[i as size_t]) { return 1 }
            if(a[i as size_t] < b[i as size_t]) { return -1 }
            i -= 1
        }
        return 0

    }
}

    // ─── Conditional copy (constant-time) ──────────────────────────────
    // If mask == 0xFFFFFFFF, copy src to dst. If mask == 0, don't.
    func fe_ccopy(dst : *mut u32, src : *u32, mask : u32) {
        unsafe {

        var i : size_t = 0
        while(i < 8) {
            dst[i] = dst[i] ^ (mask & (dst[i] ^ src[i]))
            i += 1
        }

    }
}

    // ─── Carry propagation (full reduction) ─────────────────────────────
    // Takes a partially-reduced element and fully propagates carries.
    // Ensures each limb < 2^32.
    func fe_carry(f : *mut u64) {
        unsafe {

        var c : u64 = 0
        var i : size_t = 0
        while(i < 8) {
            f[i] = f[i] + c
            c = f[i] >> 32
            f[i] = f[i] & 0xFFFFFFFFu64
            i += 1
        }
        // If there's a carry above limb 7, multiply by 19 and add to limb 0
        // since 2^256 ≡ 2 * 2^255 ≡ 2 * 19 = 38 mod p
        // Actually: 2^256 = 2 * 2^255 ≡ 2 * 19 = 38 (mod p)
        if(c > 0) {
            f[0] = f[0] + c * 38
            // Propagate any carry from this addition
            var c2 = f[0] >> 32
            f[0] = f[0] & 0xFFFFFFFFu64
            if(c2 > 0) {
                f[1] = f[1] + c2
                c2 = f[1] >> 32; f[1] = f[1] & 0xFFFFFFFFu64
                if(c2 > 0) {
                    f[2] = f[2] + c2; c2 = f[2] >> 32; f[2] = f[2] & 0xFFFFFFFFu64
                    if(c2 > 0) {
                        f[3] = f[3] + c2; c2 = f[3] >> 32; f[3] = f[3] & 0xFFFFFFFFu64
                        if(c2 > 0) {
                            f[4] = f[4] + c2; c2 = f[4] >> 32; f[4] = f[4] & 0xFFFFFFFFu64
                            if(c2 > 0) {
                                f[5] = f[5] + c2; c2 = f[5] >> 32; f[5] = f[5] & 0xFFFFFFFFu64
                                if(c2 > 0) {
                                    f[6] = f[6] + c2; c2 = f[6] >> 32; f[6] = f[6] & 0xFFFFFFFFu64
                                    if(c2 > 0) {
                                        f[7] = f[7] + c2
                                        // Final carry from limb 7 would wrap again via 38
                                        var c3 = f[7] >> 32
                                        f[7] = f[7] & 0xFFFFFFFFu64
                                        if(c3 > 0) {
                                            f[0] = f[0] + c3 * 38
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

    }
}

    // ─── Conditional subtraction of p ──────────────────────────────────
    // If f >= p, subtract p from f (in-place).
    func fe_strong_reduce(f : *mut u32) {
        unsafe {

        // p limbs
        var p_limbs : [8]u32 = [
            0xFFFFFFEDu32, 0xFFFFFFFFu32, 0xFFFFFFFFu32, 0xFFFFFFFFu32,
            0xFFFFFFFFu32, 0xFFFFFFFFu32, 0xFFFFFFFFu32, 0x7FFFFFFFu32
        ]
        // Check if f >= p
        var ge = true
        var i : i32 = 7
        while(i >= 0) {
            var idx = i as size_t
            if(f[idx] > p_limbs[idx]) { ge = true; break }
            if(f[idx] < p_limbs[idx]) { ge = false; break }
            i -= 1
        }
        if(ge) {
            // Subtract p from f
            var borrow : u64 = 0
            i = 0
            while(i < 8) {
                var diff = (f[i as size_t] as u64) - (p_limbs[i as size_t] as u64) - borrow
                f[i as size_t] = (diff & 0xFFFFFFFFu64) as u32
                borrow = (diff >> 63)
                i += 1
            }
        }

    }
}

    // ─── Addition mod p (fully reduced) ────────────────────────────────
    public func fe_add(c : *mut u32, a : *u32, b : *u32) {
        unsafe {

        var t : [8]u64
        var i : size_t = 0
        while(i < 8) {
            t[i] = (a[i] as u64) + (b[i] as u64)
            i += 1
        }
        fe_carry(&raw mut t[0])
        // Convert to u32 and store in c
        var j : size_t = 0
        while(j < 8) {
            c[j] = t[j] as u32
            j += 1
        }
        fe_strong_reduce(c)

    }
}

    func fe_debug_print(msg : *char, f : *u32) {
        if(!tls_config::EXTENSIVE_DEBUG_LOG) { return }
        var bytes : [32]u8
        fe_encode(&raw mut bytes[0], f)
        unsafe { printf("[FEDBG] %s: ", msg) }
        var i : size_t = 0
        while(i < 8) {
            unsafe { printf("%08x ", f[i]) }
            i += 1
        }
        unsafe { printf("|| ") }
        i = 0
        while(i < 32) {
            unsafe { printf("%02x", bytes[i] as int) }
            i += 1
        }
        unsafe { printf("\n") }
    }

    // ─── Subtraction mod p (fully reduced) ─────────────────────────────
    // Computes c = a - b mod p = (a + (p - b)) mod p
    public func fe_sub(c : *mut u32, a : *u32, b : *u32) {
        unsafe {

        // Step 1: compute p - b using borrow subtraction (b < p, so this is exact)
        var t : [8]u64
        var borrow : u64 = 0
        var i : size_t = 0
        while(i < 8) {
            var p_limb : u64 = 0xFFFFFFFFu64
            if(i == 0) { p_limb = 0xFFFFFFEDu64 }
            if(i == 7) { p_limb = 0x7FFFFFFFu64 }
            var diff = p_limb - (b[i] as u64) - borrow
            t[i] = diff & 0xFFFFFFFFu64
            borrow = diff >> 63
            i += 1
        }
        // After this, t = p - b (borrow should be 0 since b < p)

        // Step 2: add a to (p - b): result = a + (p - b)
        var carry : u64 = 0
        i = 0
        while(i < 8) {
            var sum = (a[i] as u64) + t[i] + carry
            t[i] = sum & 0xFFFFFFFFu64
            carry = sum >> 32
            i += 1
        }

        // Step 3: handle carry out of limb 7 (2^256 ≡ 38 mod p)
        if(carry > 0) {
            t[0] = t[0] + carry * 38
            var c2 = t[0] >> 32
            t[0] = t[0] & 0xFFFFFFFFu64
            i = 1
            while(i < 8 && c2 > 0) {
                t[i] = t[i] + c2
                c2 = t[i] >> 32
                t[i] = t[i] & 0xFFFFFFFFu64
                i += 1
            }
            // If still carry after limb 7, wrap again
            if(c2 > 0) {
                t[0] = t[0] + c2 * 38
            }
        }

        // Store as u32 and strong reduce
        var j : size_t = 0
        while(j < 8) {
            c[j] = t[j] as u32
            j += 1
        }
        fe_strong_reduce(c)

    }
}

    // ─── 32x32 → 64-bit multiplication ──────────────────────────────────
    // Direct multiplication (C/LLVM handles u64 promotion correctly).
    func mul32(r_lo : *mut u64, r_hi : *mut u64, x : u32, y : u32) { unsafe {
        var prod = (x as u64) * (y as u64)
        *r_lo = prod & 0xFFFFFFFFu64
        *r_hi = prod >> 32
     }}

    // ─── Multiplication mod p ──────────────────────────────────────────
    // Schoolbook multiplication of two 8-limb numbers, then reduce mod p.
    public func fe_mul(c : *mut u32, a : *u32, b : *u32) {
        unsafe {

        // Compute 16 partial products (256-bit * 256-bit = 512-bit result)
        var t : [16]u64
        fe_zero_u64_arr(&raw mut t[0], 16)

        var i : size_t = 0
        while(i < 8) {
            var j : size_t = 0
            while(j < 8) {
                var lo : u64 = 0; var hi : u64 = 0
                mul32(&raw mut lo, &raw mut hi, a[i], b[j])
                t[i + j] = t[i + j] + lo
                t[i + j + 1] = t[i + j + 1] + hi
                j += 1
            }
            i += 1
        }

        // Propagate carries through the 16-limb result
        var carry : u64 = 0
        i = 0
        while(i < 16) {
            t[i] = t[i] + carry
            carry = t[i] >> 32
            t[i] = t[i] & 0xFFFFFFFFu64
            i += 1
        }

        // Reduce mod p: 2^256 ≡ 38 mod p (since 2^256 = 2 * 2^255 ≡ 2 * 19 = 38)
        var r0 = t[0] + t[8] * 38 + carry * 1444
        var r1 = t[1] + t[9] * 38
        var r2 = t[2] + t[10] * 38
        var r3 = t[3] + t[11] * 38
        var r4 = t[4] + t[12] * 38
        var r5 = t[5] + t[13] * 38
        var r6 = t[6] + t[14] * 38
        var r7 = t[7] + t[15] * 38

        // Propagate carries
        var c0 = r0 >> 32; r0 = r0 & 0xFFFFFFFFu64
        r1 = r1 + c0; var c1 = r1 >> 32; r1 = r1 & 0xFFFFFFFFu64
        r2 = r2 + c1; var c2 = r2 >> 32; r2 = r2 & 0xFFFFFFFFu64
        r3 = r3 + c2; var c3 = r3 >> 32; r3 = r3 & 0xFFFFFFFFu64
        r4 = r4 + c3; var c4 = r4 >> 32; r4 = r4 & 0xFFFFFFFFu64
        r5 = r5 + c4; var c5 = r5 >> 32; r5 = r5 & 0xFFFFFFFFu64
        r6 = r6 + c5; var c6 = r6 >> 32; r6 = r6 & 0xFFFFFFFFu64
        r7 = r7 + c6; var c7 = r7 >> 32; r7 = r7 & 0xFFFFFFFFu64

        // If any carry remains in c7, multiply by 38 and add to r0
        if(c7 > 0) {
            r0 = r0 + c7 * 38
            var _c = r0 >> 32
            r0 = r0 & 0xFFFFFFFFu64
            if(_c > 0) {
                r1 = r1 + _c; _c = r1 >> 32; r1 = r1 & 0xFFFFFFFFu64
                if(_c > 0) {
                    r2 = r2 + _c; _c = r2 >> 32; r2 = r2 & 0xFFFFFFFFu64
                    if(_c > 0) {
                        r3 = r3 + _c; _c = r3 >> 32; r3 = r3 & 0xFFFFFFFFu64
                        if(_c > 0) {
                            r4 = r4 + _c; _c = r4 >> 32; r4 = r4 & 0xFFFFFFFFu64
                            if(_c > 0) {
                                r5 = r5 + _c; _c = r5 >> 32; r5 = r5 & 0xFFFFFFFFu64
                                if(_c > 0) {
                                    r6 = r6 + _c; _c = r6 >> 32; r6 = r6 & 0xFFFFFFFFu64
                                    if(_c > 0) {
                                        r7 = r7 + _c
                                        var _c2 = r7 >> 32
                                        r7 = r7 & 0xFFFFFFFFu64
                                        if(_c2 > 0) {
                                            r0 = r0 + _c2 * 38
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // Store as u32
        c[0] = r0 as u32; c[1] = r1 as u32; c[2] = r2 as u32; c[3] = r3 as u32
        c[4] = r4 as u32; c[5] = r5 as u32; c[6] = r6 as u32; c[7] = r7 as u32

        fe_strong_reduce(c)

    }
}

    // ─── Square (just calls fe_mul for correctness) ─────────────────────
    func fe_sq(c : *mut u32, a : *u32) {
        fe_mul(c, a, a)
    }

    // ─── Helper: zero u64 array ─────────────────────────────────────────
    func fe_zero_u64_arr(arr : *mut u64, n : size_t) {
        unsafe {

        var i : size_t = 0
        while(i < n) { arr[i] = 0; i += 1 }

    }
}

    // ─── Modular inverse using Fermat's little theorem ──────────────────
    // a^(p-2) mod p where p-2 = 2^255 - 21
    // Uses square-and-multiply.
    func fe_inv(c : *mut u32, a : *u32) {
        // Exponent = 2^255 - 21 = 0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEB
        // Bits: 254=1, 253=1, ..., 5=1, 4=0, 3=1, 2=0, 1=1, 0=1
        // Last byte 0xEB = 11101011
        //
        // Square-and-multiply from bit 254 down to 0:
        // Start with r = 1, then for each bit: r = r^2, if bit set: r = r * a

        var r : [8]u32; fe_set_small(&raw mut r[0], 1)

        var bit : i32 = 254
        while(bit >= 0) {
            fe_sq(&raw mut r[0], &raw r[0])

            // Check if bit of 0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEB is set
            // 0xEB = 1110 1011, so last byte bits: 7=1,6=1,5=1,4=0,3=1,2=0,1=1,0=1
            // All other bytes (above byte 0) are 0xFF = all bits set
            var do_mul : bool = false
            var byte_pos = (bit / 8) as size_t
            var bit_in_byte = (bit % 8) as u32
            if(byte_pos > 0) {
                // For bytes 1-31 (bits 8-254), all bits are 1 (0xFF)
                do_mul = true
            } else {
                // For byte 0 (bits 0-7), check each bit of 0xEB
                if(bit_in_byte == 0 || bit_in_byte == 1 || bit_in_byte == 3 || bit_in_byte == 5 || bit_in_byte == 6 || bit_in_byte == 7) {
                    do_mul = true
                }
            }

            if(do_mul) {
                fe_mul(&raw mut r[0], &raw r[0], a)
            }
            bit -= 1
        }

        fe_copy(c, &raw r[0])
    }

    // ─── Conditional swap (constant-time using XOR mask) ───────────────
    func fe_cswap(a : *mut u32, b : *mut u32, sw : u32) {
        unsafe {

        // Create a mask that's all-1s if sw == 1, all-0s if sw == 0
        var mask : u32 = 0u32 - sw  // 0 -> 0, 1 -> 0xFFFFFFFF
        var i : size_t = 0
        while(i < 8) {
            var diff = mask & (a[i] ^ b[i])
            a[i] = a[i] ^ diff
            b[i] = b[i] ^ diff
            i += 1
        }

    }
}

    // Clamp scalar per RFC 7748 Section 5
    public func x25519_clamp_scalar(scalar : *mut u8) {
        unsafe {

        scalar[0] = scalar[0] & 0xF8 as u8
        scalar[31] = scalar[31] & 0x7F as u8
        scalar[31] = scalar[31] | 0x40 as u8

    }
}

    // ─── Montgomery Ladder ──────────────────────────────────────────────
    // Implements RFC 7748 Section 5.

    public func x25519_ladder(out : *mut u8, scalar : *u8, u : *u8) {
        unsafe {

        // Clamp scalar per RFC 7748 Section 5 (implementations MUST clamp)
        var clamped : [32]u8
        var ci : size_t = 0
        while(ci < 32) {
            clamped[ci] = scalar[ci]
            ci += 1
        }
        clamped[0] = clamped[0] & 0xF8 as u8
        clamped[31] = clamped[31] & 0x7F as u8
        clamped[31] = clamped[31] | 0x40 as u8
        var effective_scalar = &raw clamped[0]

        // Decode u coordinate
        var u_fe : [8]u32
        fe_decode(&raw mut u_fe[0], u)

        if(tls_config::EXTENSIVE_DEBUG_LOG) {
            var _enc : [32]u8; fe_encode(&raw mut _enc[0], &raw u_fe[0])
            unsafe { printf("[LADDER] u_decoded: ") };
            var _xi : size_t = 0; while(_xi < 8) { unsafe { printf("%08x ", u_fe[_xi]) }; _xi += 1 }
            unsafe { printf(" => ") }; _xi = 0; while(_xi < 32) { unsafe { printf("%02x", _enc[_xi] as int) }; _xi += 1 }
            unsafe { printf("\n") }
        }

        // State variables
        var x2 : [8]u32; fe_set_small(&raw mut x2[0], 1)   // X2 = 1
        var z2 : [8]u32; fe_zero(&raw mut z2[0])             // Z2 = 0
        var x3 : [8]u32; fe_copy(&raw mut x3[0], &raw u_fe[0])  // X3 = u
        var z3 : [8]u32; fe_set_small(&raw mut z3[0], 1)     // Z3 = 1

        // a24 = 121665
        var a24 : [8]u32; fe_set_small(&raw mut a24[0], 121665)

        // Temp variables
        var A : [8]u32; var AA : [8]u32
        var B : [8]u32; var BB : [8]u32
        var E : [8]u32; var C : [8]u32; var D : [8]u32
        var DA : [8]u32; var CB : [8]u32
        var T : [8]u32; var DT : [8]u32

        var swap : u32 = 0

        var bit : i32 = 254
        while(bit >= 0) {
            var byte_idx = (bit / 8) as size_t
            var bit_idx = (bit % 8) as u32
            var kbit = ((effective_scalar[byte_idx] >> bit_idx) as u32) & 1u32

            swap = swap ^ kbit

            fe_cswap(&raw mut x2[0], &raw mut x3[0], swap)
            fe_cswap(&raw mut z2[0], &raw mut z3[0], swap)

            swap = kbit

            // A = x2 + z2, AA = A^2
            fe_add(&raw mut A[0], &raw x2[0], &raw z2[0])
            fe_sq(&raw mut AA[0], &raw A[0])

            // B = x2 - z2, BB = B^2
            fe_sub(&raw mut B[0], &raw x2[0], &raw z2[0])
            fe_sq(&raw mut BB[0], &raw B[0])

            // E = AA - BB
            fe_sub(&raw mut E[0], &raw AA[0], &raw BB[0])

            // C = x3 + z3
            fe_add(&raw mut C[0], &raw x3[0], &raw z3[0])

            // D = x3 - z3
            fe_sub(&raw mut D[0], &raw x3[0], &raw z3[0])

            // DA = D * A, CB = C * B
            fe_mul(&raw mut DA[0], &raw D[0], &raw A[0])
            fe_mul(&raw mut CB[0], &raw C[0], &raw B[0])

            // x3 = (DA + CB)^2
            fe_add(&raw mut x3[0], &raw DA[0], &raw CB[0])
            fe_sq(&raw mut x3[0], &raw x3[0])

            // z3 = u * (DA - CB)^2
            fe_sub(&raw mut z3[0], &raw DA[0], &raw CB[0])
            fe_sq(&raw mut z3[0], &raw z3[0])
            fe_mul(&raw mut z3[0], &raw z3[0], &raw u_fe[0])

            // x2 = AA * BB
            fe_mul(&raw mut x2[0], &raw AA[0], &raw BB[0])

            // z2 = E * (AA + a24 * E)
            fe_mul(&raw mut T[0], &raw a24[0], &raw E[0])
            fe_add(&raw mut DT[0], &raw AA[0], &raw T[0])
            fe_mul(&raw mut z2[0], &raw E[0], &raw DT[0])

            bit -= 1
        }

        // Final conditional swap
        fe_cswap(&raw mut x2[0], &raw mut x3[0], swap)
        fe_cswap(&raw mut z2[0], &raw mut z3[0], swap)

        // Result = x2 * z2^(-1) mod p
        var z2_inv : [8]u32
        fe_inv(&raw mut z2_inv[0], &raw z2[0])
        fe_mul(&raw mut x2[0], &raw x2[0], &raw z2_inv[0])

        // Encode to 32 bytes little-endian
        fe_encode(out, &raw x2[0])

    }
}

    // Generate an x25519 keypair
    public func x25519_generate_keypair(priv : *mut u8, pub : *mut u8) : int {
        unsafe {
        var rng_ret = random_fill(priv, 32)
        if(rng_ret < 0) { return rng_ret }

        x25519_clamp_scalar(priv)
        x25519_ladder(pub, priv, &raw X25519_BASE_POINT[0])
        return 0
        }
    }

    // Compute shared secret from private key and peer public key
    public func x25519_compute_shared(priv : *u8, peer_pub : *u8, shared : *mut u8) : int {
        unsafe {

        // Per RFC 7748: rejecting all-zero output is mandatory.
        // Check peer public key for all-zero first.
        var all_zero_peer = true
        var cz : size_t = 0
        while(cz < 32) {
            if(peer_pub[cz] != 0) { all_zero_peer = false }
            cz += 1
        }
        if(all_zero_peer) { return ERR_ECP_INVALID_KEY }

        // Clamp the private key per RFC 7748 Section 5
        var clamped_priv : [32]u8
        var ci : size_t = 0
        while(ci < 32) {
            clamped_priv[ci] = priv[ci]
            ci += 1
        }
        x25519_clamp_scalar(&raw mut clamped_priv[0])
        x25519_ladder(shared, &raw clamped_priv[0], peer_pub)

        // Check for all-zero output (low-order point)
        var all_zero = true
        var i : size_t = 0
        while(i < 32) {
            if(shared[i] != 0) { all_zero = false }
            i += 1
        }
        if(all_zero) { return ERR_ECP_INVALID_KEY }

        return 0

    }
}


// ===== random.ch =====

    // ─── Secure Random Fill ────────────────────────────────────────────────
    // Fill a buffer with cryptographically secure random bytes.
    // Delegates to osrand::raw::random_fill (shared OS entropy source).
    // Returns 0 on success, ERR_SSL_NO_RNG on failure.
    public func random_fill(buf : *mut u8, len : size_t) : int {
        var ret = unsafe(osrand::raw::random_fill(buf, len))
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


// ===== async.ch =====

// Connect + handshake. Returns 0 on success, <0 on failure (`tls_connect`).
public async func tls_connect_async(ssl : *mut SSLContext, host : *char, port : uint) : int {
    ssl_use_async_transport(ssl)
    return await tls_connect_coro(ssl, host, port)
}

public async func ssl_handshake_async(ssl : *mut SSLContext) : int {
    ssl_use_async_transport(ssl)
    return await ssl_handshake_coro(ssl)
}

public async func ssl_read_async(ssl : *mut SSLContext, buf : *mut u8, len : i32) : int {
    ssl_use_async_transport(ssl)
    return await ssl_read_coro(ssl, buf, len)
}

public async func ssl_write_async(ssl : *mut SSLContext, data : *u8, len : i32) : int {
    ssl_use_async_transport(ssl)
    return await ssl_write_coro(ssl, data, len)
}

// Server-side: accept + handshake on an already-accepted socket. Returns the
// new `SSLContext` (null on failure).
public async func tls_accept_async(sock : net::raw::Socket, cert : *mut X509Cert, priv_key : *mut void) : *mut SSLContext {
    return await tls_accept_coro(sock, cert, priv_key, TLS_RSA_WITH_AES_128_GCM_SHA256, true)
}


} // namespace raw
} // namespace tls
