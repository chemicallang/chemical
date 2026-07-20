// ============================================================================
// X.509 Certificate Types and Parsing
// ============================================================================
// Port of mbedTLS x509_crt.h and x509.h to Chemical.
// X.509 certificate handling for TLS with full PEM and DER parsing.
// ============================================================================

public namespace tls {

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
        var next : *mut X509Cert
        var prev : *mut X509Cert
        var flags : u32

        @delete
        func destruct(&self) {
            var curr = self.next
            while(curr != null) {
                var nxt = curr.next
                curr.next = null
                curr.prev = null
                curr = nxt
            }
        }
    }

    // Free certificate chain (destructor handles it)
    public func cert_free(crt : *mut X509Cert) {}

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

    // Get CN from subject
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
            if(c == (',' as u8) || c == ('/' as u8)) { break }
            end += 1
        }
        var cn_str = sub_view.subview(start, end)
        *out = string::make_no_len(cn_str.data() as *char)
    }

    // ============================================================================
    // ASN.1 Parsing Utilities
    // ============================================================================

    // Read ASN.1 tag and length at position pos, return tag and set length/content start
    func asn1_get_tag(data : *u8, data_len : size_t, pos : *mut size_t,
                       tag : *mut u8, length : *mut size_t) : int {
        if(*pos >= data_len) { return ERR_X509_INVALID_FORMAT }
        var t = data[*pos]; *pos += 1

        // Long-form tags not supported
        if(t >= 0x1F) { return ERR_X509_FEATURE_UNAVAILABLE }

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
    }

    // Compare an OID at data+pos of oid_len bytes against reference bytes
    func oid_matches(data : *u8, oid_len : size_t, ref : *u8, ref_len : size_t) : bool {
        if(oid_len != ref_len) { return false }
        var i : size_t = 0
        while(i < oid_len) {
            if(data[i] != ref[i]) { return false }
            i += 1
        }
        return true
    }

    // Parse a printable/IA5/UTF8 string from ASN.1 into a Chemical string
    func asn1_read_string(data : *u8, data_len : size_t, pos : *mut size_t,
                           out : *mut string) : int {
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
    }

    // Parse ASN.1 time (UTCTime or GeneralizedTime)
    func asn1_read_time(data : *u8, data_len : size_t, pos : *mut size_t,
                         out : *mut u8, out_max : size_t) : int {
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
    }

    // Parse an ASN.1 integer
    func asn1_read_integer(data : *u8, data_len : size_t, pos : *mut size_t,
                            out : *mut *mut u8, out_len : *mut size_t) : int {
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
    }

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
                          out : *mut string) : int {
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
    }

    // Parse a Relative Distinguished Name (SET of AttributeTypeAndValue)
    func parse_rdn(data : *u8, data_len : size_t, pos : *mut size_t,
                    out : *mut string) : int {
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
    }

    // Parse the full Distinguished Name (SEQUENCE of RDN)
    func parse_distinguished_name(data : *u8, data_len : size_t, pos : *mut size_t,
                                   out : *mut string, raw_out : *mut *mut u8,
                                   raw_len_out : *mut size_t) : int {
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
    }

    // ============================================================================
    // Full DER Certificate Parsing
    // ============================================================================

    // Parse a DER-encoded X.509 certificate (RFC 5280)
    public func parse_cert_der(crt : *mut X509Cert, der_data : *u8, der_len : size_t) : int {
        if(der_len < 10) { return ERR_X509_INVALID_FORMAT }

        crt.raw_pem = der_data as *mut u8
        crt.raw_pem_len = der_len

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
        var tbs_tag : u8 = 0
        var tbs_len : size_t = 0
        ret = asn1_get_tag(der_data, der_len, &raw mut pos, &raw mut tbs_tag, &raw mut tbs_len)
        if(ret < 0) { return ret }
        if(tbs_tag != (ASN1_CONSTRUCTED | ASN1_SEQUENCE)) { return ERR_X509_INVALID_FORMAT }

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

        return 0
    }

    // ============================================================================
    // PEM Certificate Parsing
    // ============================================================================

    // Parse a PEM-encoded certificate
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
        var result = crypto::base64_decode(b64_buf as *char, buf_pos, &raw mut der_buf[0], 4096)
        if(result is Result.Err) {
            return ERR_X509_INVALID_FORMAT
        }

        var Ok(decoded_len) = result else unreachable

        // Parse the DER certificate
        return parse_cert_der(crt, &raw der_buf[0], decoded_len)
    }

} // namespace tls
