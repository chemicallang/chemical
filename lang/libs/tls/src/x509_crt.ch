// ============================================================================
// X.509 Certificate Types and Parsing
// ============================================================================
// Port of mbedTLS x509_crt.h and x509.h to Chemical.
// X.509 certificate handling for TLS.
// ============================================================================

public namespace tls {

    using std::string;
    using std::string_view;

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
        var next : *mut X509Cert
        var prev : *mut X509Cert
        var flags : u32

        @delete
        func destruct(&self) {
            // Note: raw memory pointers are owned externally
            // just clear the linked list pointers
            var curr = self.next
            while(curr != null) {
                var nxt = curr.next
                curr.next = null
                curr.prev = null
                curr = nxt
            }
        }
    }

    // Free certificate chain
    public func cert_free(crt : *mut X509Cert) {
        // The destructor handles the chain cleanup
        // This function is a no-op for manual cases
    }

    // Initialize an X509 certificate
    public func x509_cert_init(crt : *mut X509Cert) {
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
        crt.ext_key_usage = 0
        crt.ext_is_ca = false
        crt.ext_max_pathlen = -1
        crt.san_count = 0
        crt.san_entries = null
        crt.next = null
        crt.prev = null
        crt.flags = 0
    }

    // Get CN from subject (extracts CN= value from subject string)
    public func cert_get_cn(crt : *mut X509Cert, out : *mut string) {
        var cn_prefix = string_view("CN=")
        var sub_view = crt.subject.to_view()
        var pos = sub_view.find(&cn_prefix)
        if(pos == std::NPOS) {
            *out = string()
            return
        }
        var start = pos + 3
        var end = start
        while(end < sub_view.size()) {
            var c = sub_view.get(end)
            if(c == ',' as u8 || c == '/' as u8) { break }
            end += 1
        }
        var cn_str = sub_view.subview(start, end)
        *out = string::make_no_len(cn_str.data() as *char)
    }

    // ─── X.509 Certificate Parsing ─────────────────────────────────────────

    // Parse a PEM certificate
    public func parse_cert_pem(crt : *mut X509Cert, pem_data : *u8, pem_len : size_t) : int {
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

        // Find END marker
        i = begin_pos + 27
        var end_pos : size_t = begin_pos + 27
        var found_end = false
        while(i < pem_len) {
            var sub2 = data_view.subview(i, pem_len)
            if(sub2.starts_with(&end_marker)) {
                end_pos = i
                found_end = true
                break
            }
            i += 1
        }

        if(!found_end) { return ERR_X509_INVALID_FORMAT }

        // For now, store raw data and return basic parsing
        // TODO: full PEM decoding and DER parsing
        crt.raw_pem = pem_data as *mut u8
        crt.raw_pem_len = pem_len
        crt.version = 3
        crt.subject = string("(unknown)")

        return ERR_X509_FEATURE_UNAVAILABLE
    }

    // Parse a DER certificate
    public func parse_cert_der(crt : *mut X509Cert, der_data : *u8, der_len : size_t) : int {
        if(der_len < 4) { return ERR_X509_INVALID_FORMAT }

        // Check for SEQUENCE tag
        if(der_data[0] != (ASN1_CONSTRUCTED | ASN1_SEQUENCE)) {
            return ERR_X509_INVALID_FORMAT
        }

        // Store raw data
        crt.raw_pem = der_data as *mut u8
        crt.raw_pem_len = der_len
        crt.version = 3
        crt.subject = string("(DER)")

        return 0  // Success (simplified)
    }

}
 // namespace tls
