// ecdsa_point_validation.ch — P-256 public keys must be rejected when the
// point is not on the curve.
//
// THE GAP THIS PINS
//
// `ecdsa_import_pubkey` used to parse the uncompressed point form (0x04 || X ||
// Y) without checking that the result is a point on the curve. Upstream
// mbedTLS's `mbedtls_ecp_point_read_binary` does, and answers
// `MBEDTLS_ERR_ECP_INVALID_KEY` for an off-curve point, so this was a gap in
// the port rather than a deliberate relaxation.
//
// It was found while building WebAuthn support in Wiqis' account service, where
// it matters more than it usually would: a COSE key arrives FROM THE CLIENT at
// registration time, so "imported without error" must never be read as "this is
// a valid public key".
//
// FIXED: `ecp_check_affine` in `lang/libs/tls/src/ecdh.ch` now does the range
// and curve-equation checks, and `ecdsa_import_pubkey` calls it (pinning the
// curve from its `curve` argument for the duration). These tests were written
// to fail before that landed and are now the regression guard for it.
//
// Python's `cryptography` is the oracle, in both directions — the same library
// the rest of this suite already cross-checks ECDSA against. It generates a real
// key (so we are not asserting a rejection for a point that might legitimately
// be valid), and it is asked to confirm that the tampered point really is
// off-curve before the port is asked about it.
//
// Style note: everything is inlined rather than factored into helpers, because
// Chemical will not pass a fixed-size array to a `&mut [N]u8` parameter. That is
// why the rest of this suite is as repetitive as it is.

using namespace tls
using std::string_view

@test
public func INT_ecdsa_import_pubkey_rejects_off_curve_point(env : &mut TestEnv) {
    // A real key, plus one bit flipped in Y.
    var script : [2048]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.asymmetric import ec\nkey=ec.generate_private_key(ec.SECP256R1())\nn=key.public_key().public_numbers()\nprint('PX='+format(n.x,'064x'))\nprint('PY='+format(n.y,'064x'))\nprint('BY='+format(n.y ^ 1,'064x'))\n" as *char
    si=0; while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var py_out = test_python_run_script(&raw script[0], sp, string_view("ecdsa_ptval"))
    var px : [64]u8; var py : [64]u8; var by : [64]u8
    if(test_parse_py_hex_label(&raw mut py_out, string_view("PX="), &raw mut px[0], 32)!=32){env.error("px");return}else{}
    if(test_parse_py_hex_label(&raw mut py_out, string_view("PY="), &raw mut py[0], 32)!=32){env.error("py");return}else{}
    if(test_parse_py_hex_label(&raw mut py_out, string_view("BY="), &raw mut by[0], 32)!=32){env.error("by");return}else{}

    // Ask python whether that point is genuinely invalid, so a failure below is
    // the port's doing and not a bad test vector.
    var px_hex : [65]char; test_bytes_to_hex(&raw px[0], 32, &raw mut px_hex[0])
    var by_hex : [65]char; test_bytes_to_hex(&raw by[0], 32, &raw mut by_hex[0])
    var script2 : [1024]u8; var sp2 : size_t = 0; var si2 : size_t = 0
    var hdr2 = "from cryptography.hazmat.primitives.asymmetric import ec\ntry:\n ec.EllipticCurvePublicNumbers(int.from_bytes(bytes.fromhex('" as *char
    si2=0; while(hdr2[si2]!=0){script2[sp2]=hdr2[si2] as u8; sp2+=1; si2+=1}
    si2=0; while(px_hex[si2]!=0){script2[sp2]=px_hex[si2] as u8; sp2+=1; si2+=1}
    var l2 = "'),'big'),int.from_bytes(bytes.fromhex('" as *char; si2=0
    while(l2[si2]!=0){script2[sp2]=l2[si2] as u8; sp2+=1; si2+=1}
    si2=0; while(by_hex[si2]!=0){script2[sp2]=by_hex[si2] as u8; sp2+=1; si2+=1}
    l2 = "'),'big'),ec.SECP256R1()).public_key()\n print('OFF=1')\nexcept Exception:\n print('OFF=0')\n" as *char; si2=0
    while(l2[si2]!=0){script2[sp2]=l2[si2] as u8; sp2+=1; si2+=1}
    var py_out2 = test_python_run_script(&raw script2[0], sp2, string_view("ecdsa_ptval2"))
    // Expect "OFF=0": python refused the point.
    var python_said_invalid = false
    var k : size_t = 0
    while(k + 5u <= py_out2.size()) {
        if(py_out2.get(k)==79 && py_out2.get(k+1u)==70 && py_out2.get(k+2u)==70 && py_out2.get(k+3u)==61 && py_out2.get(k+4u)==48) {
            python_said_invalid = true
        }
        k += 1u
    }
    if(!python_said_invalid) {
        env.error("python accepted the tampered point - the test vector is not invalid after all")
        return
    }

    var point : [65]u8
    point[0]=4; var i:size_t=0
    while(i<32){point[1+i]=px[i];point[33+i]=by[i];i+=1}
    var ctx : ECDSAContext; ecdsa_init(unsafe(&raw mut ctx))
    var ret = ecdsa_import_pubkey(unsafe(&raw mut ctx), &raw point[0], 65, TLS_GROUP_SECP256R1 as u16)
    if(ret == 0) {
        // THE BUG: an off-curve point was accepted.
        env.error("ecdsa_import_pubkey accepted a point that is NOT on the curve (python confirms it is invalid)")
    }
}

@test
public func INT_ecdsa_off_curve_key_verifies_nothing(env : &mut TestEnv) {
    // The property that protects a caller today, and which must survive the fix:
    // even though the importer lets an off-curve point through, it must not
    // verify a real signature. This passes now and should keep passing.
    var script : [2048]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "from cryptography.hazmat.primitives.asymmetric import ec\nkey=ec.generate_private_key(ec.SECP256R1())\nn=key.public_key().public_numbers()\nprint('PX='+format(n.x,'064x'))\nprint('PY='+format(n.y ^ 1,'064x'))\n" as *char
    si=0; while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    var py_out = test_python_run_script(&raw script[0], sp, string_view("ecdsa_offkey"))
    var px : [64]u8; var by : [64]u8
    if(test_parse_py_hex_label(&raw mut py_out, string_view("PX="), &raw mut px[0], 32)!=32){env.error("px");return}else{}
    if(test_parse_py_hex_label(&raw mut py_out, string_view("PY="), &raw mut by[0], 32)!=32){env.error("py");return}else{}

    // A genuine signature from python, over a random digest, with its own key.
    var hash : [32]u8; test_random_bytes(&raw mut hash[0], 32)
    var h_hex : [65]char; test_bytes_to_hex(&raw hash[0], 32, &raw mut h_hex[0])
    var script2 : [2048]u8; var sp2 : size_t = 0; var si2 : size_t = 0
    var hdr2 = "from cryptography.hazmat.primitives.asymmetric import ec,utils\nfrom cryptography.hazmat.primitives import hashes\nsk=ec.generate_private_key(ec.SECP256R1())\nn=sk.public_key().public_numbers()\nsig=sk.sign(bytes.fromhex('" as *char
    si2=0; while(hdr2[si2]!=0){script2[sp2]=hdr2[si2] as u8; sp2+=1; si2+=1}
    si2=0; while(h_hex[si2]!=0){script2[sp2]=h_hex[si2] as u8; sp2+=1; si2+=1}
    var l2 = "'),ec.ECDSA(utils.Prehashed(hashes.SHA256())))\nprint('N2X='+format(n.x,'064x'))\nprint('N2Y='+format(n.y,'064x'))\nprint('SIG='+sig.hex())\n" as *char; si2=0
    while(l2[si2]!=0){script2[sp2]=l2[si2] as u8; sp2+=1; si2+=1}
    var py_out2 = test_python_run_script(&raw script2[0], sp2, string_view("ecdsa_offsig"))
    var n2x : [64]u8; var n2y : [64]u8; var sig : [128]u8
    if(test_parse_py_hex_label(&raw mut py_out2, string_view("N2X="), &raw mut n2x[0], 32)!=32){env.error("n2x");return}else{}
    if(test_parse_py_hex_label(&raw mut py_out2, string_view("N2Y="), &raw mut n2y[0], 32)!=32){env.error("n2y");return}else{}
    var sig_len = test_parse_py_hex_label(&raw mut py_out2, string_view("SIG="), &raw mut sig[0], 128)
    if(sig_len==0){env.error("sig");return}else{}

    // Sanity: the signature DOES verify under the key that made it, so a
    // failure below is about the off-curve key and not a broken vector.
    var good : [65]u8
    good[0]=4; var i:size_t=0
    while(i<32){good[1+i]=n2x[i];good[33+i]=n2y[i];i+=1}
    var good_ctx : ECDSAContext; ecdsa_init(unsafe(&raw mut good_ctx))
    if(ecdsa_import_pubkey(unsafe(&raw mut good_ctx), &raw good[0], 65, TLS_GROUP_SECP256R1 as u16) != 0) {
        env.error("could not import the on-curve key python gave us")
        return
    }
    if(ecdsa_verify(unsafe(&raw mut good_ctx), &raw hash[0], 32, &raw sig[0], sig_len) < 0) {
        env.error("a genuine python signature did not verify under its own key")
        return
    }

    // Now the same signature under the off-curve point.
    var point : [65]u8
    point[0]=4; var j:size_t=0
    while(j<32){point[1+j]=px[j];point[33+j]=by[j];j+=1}
    var ctx : ECDSAContext; ecdsa_init(unsafe(&raw mut ctx))
    if(ecdsa_import_pubkey(unsafe(&raw mut ctx), &raw point[0], 65, TLS_GROUP_SECP256R1 as u16) == 0) {
        if(ecdsa_verify(unsafe(&raw mut ctx), &raw hash[0], 32, &raw sig[0], sig_len) == 0) {
            env.error("an off-curve key VERIFIED a real signature - that is a serious bug")
        }
    }
}

