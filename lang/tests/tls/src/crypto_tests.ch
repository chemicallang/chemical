using namespace tls
using namespace crypto
using std::string
using std::string_view
using std::vector

func print_hex(label : *char, data : *u8, len : size_t) {
    printf("[%s] ", label)
    var i : size_t = 0
    while(i < len) { printf("%02x", data[i] as int); i += 1 }
    printf("\n")
}

func read_hex_output(filepath : *char, out : *mut u8, out_max : size_t) : size_t {
    var buf : [512]u8
    var n = test_read_file(filepath, &raw mut buf[0], 512)
    var out_len : size_t = 0
    while(out_len*2 + 1 < n && buf[out_len*2] != 10 && buf[out_len*2] != 0 && out_len < out_max) {
        out[out_len] = test_hex_pair_byte(buf[out_len*2] as char, buf[out_len*2+1] as char)
        out_len += 1
    }
    return out_len
}

// Write script to temp dir, run it with python, read hex output
func run_py_script(script : *u8, slen : size_t, out : *mut u8, out_max : size_t) : size_t {
    var rnd : u32
    tls::random_fill(&raw mut rnd as *mut u8, 4)
    var name = string("py_")
    name.append_uinteger((rnd as ubigint) & 0xFFFFFFFu)
    var py_out = test_python_run_script(script, slen, name.to_view())
    // Parse entire output as hex
    var written : size_t = 0
    var pos : size_t = 0
    while(pos + 1 < py_out.size() && written < out_max) {
        var hi = py_out.get(pos) as char
        var lo = py_out.get(pos + 1) as char
        if(hi == 10 as char || hi == 13 as char || hi == 0 as char) { break } else {}
        out[written] = test_hex_pair_byte(hi, lo)
        written += 1
        pos += 2
    }
    return written
}

func mpi_get_bytes(m : *mut Mpi, out : *mut u8) {
    var buf : [32]u8
    mpi_write_binary(m, &raw mut buf[0], 32)
    var i : size_t = 0; while(i < 32) { out[i] = buf[i]; i += 1 }
}

// === mpi_mul_int: P-256 * 4 ===
@test public func TEST_mpi_mul_int_vs_py(env:&mut TestEnv) {
    var p : Mpi; ecp_curve_p(&raw mut p)
    var p_bytes : [32]u8; mpi_get_bytes(&raw mut p, &raw mut p_bytes[0])
    var p_hex : [65]char; test_bytes_to_hex(&raw p_bytes[0], 32, &raw mut p_hex[0])

    var script : [512]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "import sys; a=int.from_bytes(bytes.fromhex('" as *char
    si=0; while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    si=0; while(p_hex[si]!=0){script[sp]=p_hex[si] as u8; sp+=1; si+=1}
    var tail = "'),'big'); r=a*4; l=(r.bit_length()+7)//8 or 1; sys.stdout.write(r.to_bytes(l,'big').hex())" as *char
    si=0; while(tail[si]!=0){script[sp]=tail[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("mpi_mul_int"))

    var py_r:[64]u8; var py_len : size_t = 0
    var pi : size_t = 0
    while(pi + 1 < py_out.size() && py_len < 64) {
        var hi = py_out.get(pi) as char; var lo = py_out.get(pi+1) as char
        if(hi == 10 as char || hi == 13 as char || hi == 0 as char) { break } else {}
        py_r[py_len] = test_hex_pair_byte(hi, lo); py_len += 1; pi += 2
    }
    if(py_len == 0) { env.error("Python output empty"); return }

    var mr: Mpi; mpi_init(&raw mut mr)
    var ret = mpi_mul_int(&raw mut mr, &raw mut p, 4)
    var cs = mpi_size(&raw mut mr); var cb:[64]u8; mpi_write_binary(&raw mut mr, &raw mut cb[0], 64)
    if(ret < 0) { env.error("mpi_mul_int error"); return }
    if(cs != py_len || !test_bytes_eq(&raw cb[64-cs], &raw py_r[0], cs)) {
        printf("[MULINT] cs=%d py_len=%d\n", cs as int, py_len as int)
        print_hex("chem", &raw cb[64-cs], cs); print_hex("py  ", &raw py_r[0], py_len)
        env.error("mpi_mul_int mismatch")
    }
}

// === mpi_sub: P-256 - 5 ===
@test public func TEST_mpi_sub_vs_py(env:&mut TestEnv) {
    var p : Mpi; ecp_curve_p(&raw mut p)
    var p_bytes : [32]u8; mpi_get_bytes(&raw mut p, &raw mut p_bytes[0])
    var p_hex : [65]char; test_bytes_to_hex(&raw p_bytes[0], 32, &raw mut p_hex[0])

    var script : [512]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "import sys; a=int.from_bytes(bytes.fromhex('" as *char
    si=0; while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    si=0; while(p_hex[si]!=0){script[sp]=p_hex[si] as u8; sp+=1; si+=1}
    var tail = "'),'big'); r=a-5; l=(r.bit_length()+7)//8 or 1; sys.stdout.write(r.to_bytes(l,'big').hex())" as *char
    si=0; while(tail[si]!=0){script[sp]=tail[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("mpi_sub"))

    var py_r:[40]u8; var py_len : size_t = 0
    var pi : size_t = 0
    while(pi + 1 < py_out.size() && py_len < 40) {
        var hi = py_out.get(pi) as char; var lo = py_out.get(pi+1) as char
        if(hi == 10 as char || hi == 13 as char || hi == 0 as char) { break } else {}
        py_r[py_len] = test_hex_pair_byte(hi, lo); py_len += 1; pi += 2
    }
    if(py_len == 0) { env.error("Python empty"); return }

    var mf: Mpi; var fv:[1]u8; fv[0]=5; mpi_read_binary(&raw mut mf, &raw fv[0], 1)
    var mr: Mpi; mpi_init(&raw mut mr)
    var ret = mpi_sub(&raw mut mr, &raw mut p, &raw mut mf)
    var cs = mpi_size(&raw mut mr); var cb:[40]u8; mpi_write_binary(&raw mut mr, &raw mut cb[0], 40)
    if(ret < 0) { env.error("mpi_sub error"); return }
    if(cs != py_len || !test_bytes_eq(&raw cb[40-cs], &raw py_r[0], cs)) {
        printf("[SUB] cs=%d py_len=%d\n", cs as int, py_len as int)
        print_hex("chem", &raw cb[40-cs], cs); print_hex("py  ", &raw py_r[0], py_len)
        env.error("mpi_sub mismatch")
    }
}

// === mpi_add: P-256 + 5 ===
@test public func TEST_mpi_add_vs_py(env:&mut TestEnv) {
    var p : Mpi; ecp_curve_p(&raw mut p)
    var p_bytes : [32]u8; mpi_get_bytes(&raw mut p, &raw mut p_bytes[0])
    var p_hex : [65]char; test_bytes_to_hex(&raw p_bytes[0], 32, &raw mut p_hex[0])

    var script : [512]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "import sys; a=int.from_bytes(bytes.fromhex('" as *char
    si=0; while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    si=0; while(p_hex[si]!=0){script[sp]=p_hex[si] as u8; sp+=1; si+=1}
    var tail = "'),'big'); r=a+5; l=(r.bit_length()+7)//8 or 1; sys.stdout.write(r.to_bytes(l,'big').hex())" as *char
    si=0; while(tail[si]!=0){script[sp]=tail[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("mpi_add"))

    var py_r:[40]u8; var py_len : size_t = 0
    var pi : size_t = 0
    while(pi + 1 < py_out.size() && py_len < 40) {
        var hi = py_out.get(pi) as char; var lo = py_out.get(pi+1) as char
        if(hi == 10 as char || hi == 13 as char || hi == 0 as char) { break } else {}
        py_r[py_len] = test_hex_pair_byte(hi, lo); py_len += 1; pi += 2
    }
    if(py_len == 0) { env.error("Python empty"); return }

    var mf: Mpi; var fv:[1]u8; fv[0]=5; mpi_read_binary(&raw mut mf, &raw fv[0], 1)
    var mr: Mpi; mpi_init(&raw mut mr)
    var ret = mpi_add(&raw mut mr, &raw mut p, &raw mut mf)
    var cs = mpi_size(&raw mut mr); var cb:[40]u8; mpi_write_binary(&raw mut mr, &raw mut cb[0], 40)
    if(ret < 0) { env.error("mpi_add error"); return }
    if(cs != py_len || !test_bytes_eq(&raw cb[40-cs], &raw py_r[0], cs)) {
        printf("[ADD] cs=%d py_len=%d\n", cs as int, py_len as int)
        print_hex("chem", &raw cb[40-cs], cs); print_hex("py  ", &raw py_r[0], py_len)
        env.error("mpi_add mismatch")
    }
}

// === mpi_mul: P-256 * 2 ===
@test public func TEST_mpi_mul_vs_py(env:&mut TestEnv) {
    var p : Mpi; ecp_curve_p(&raw mut p)
    var p_bytes : [32]u8; mpi_get_bytes(&raw mut p, &raw mut p_bytes[0])
    var p_hex : [65]char; test_bytes_to_hex(&raw p_bytes[0], 32, &raw mut p_hex[0])

    var script : [512]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "import sys; a=int.from_bytes(bytes.fromhex('" as *char
    si=0; while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    si=0; while(p_hex[si]!=0){script[sp]=p_hex[si] as u8; sp+=1; si+=1}
    var tail = "'),'big'); r=a*2; l=(r.bit_length()+7)//8 or 1; sys.stdout.write(r.to_bytes(l,'big').hex())" as *char
    si=0; while(tail[si]!=0){script[sp]=tail[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("mpi_mul"))

    var py_r:[64]u8; var py_len : size_t = 0
    var pi : size_t = 0
    while(pi + 1 < py_out.size() && py_len < 64) {
        var hi = py_out.get(pi) as char; var lo = py_out.get(pi+1) as char
        if(hi == 10 as char || hi == 13 as char || hi == 0 as char) { break } else {}
        py_r[py_len] = test_hex_pair_byte(hi, lo); py_len += 1; pi += 2
    }
    if(py_len == 0) { env.error("Python empty"); return }

    var mtwo: Mpi; var tv:[1]u8; tv[0]=2; mpi_read_binary(&raw mut mtwo, &raw tv[0], 1)
    var mr: Mpi; mpi_init(&raw mut mr)
    var ret = mpi_mul(&raw mut mr, &raw mut p, &raw mut mtwo)
    var cs = mpi_size(&raw mut mr); var cb:[64]u8; mpi_write_binary(&raw mut mr, &raw mut cb[0], 64)
    if(ret < 0) { env.error("mpi_mul error"); return }
    if(cs != py_len || !test_bytes_eq(&raw cb[64-cs], &raw py_r[0], cs)) {
        printf("[MUL] cs=%d py_len=%d\n", cs as int, py_len as int)
        print_hex("chem", &raw cb[64-cs], cs); print_hex("py  ", &raw py_r[0], py_len)
        env.error("mpi_mul mismatch")
    }
}

// === mpi_mod: (P-256 + 5) mod 7 ===
@test public func TEST_mpi_mod_vs_py(env:&mut TestEnv) {
    var p : Mpi; ecp_curve_p(&raw mut p)
    var p_bytes : [32]u8; mpi_get_bytes(&raw mut p, &raw mut p_bytes[0])
    var p_hex : [65]char; test_bytes_to_hex(&raw p_bytes[0], 32, &raw mut p_hex[0])

    var script : [512]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "import sys; a=int.from_bytes(bytes.fromhex('" as *char
    si=0; while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    si=0; while(p_hex[si]!=0){script[sp]=p_hex[si] as u8; sp+=1; si+=1}
    var tail = "'),'big'); r=(a+5)%7; l=(r.bit_length()+7)//8 or 1; sys.stdout.write(r.to_bytes(l,'big').hex())" as *char
    si=0; while(tail[si]!=0){script[sp]=tail[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("mpi_mod"))

    var py_r:[8]u8; var py_len : size_t = 0
    var pi : size_t = 0
    while(pi + 1 < py_out.size() && py_len < 8) {
        var hi = py_out.get(pi) as char; var lo = py_out.get(pi+1) as char
        if(hi == 10 as char || hi == 13 as char || hi == 0 as char) { break } else {}
        py_r[py_len] = test_hex_pair_byte(hi, lo); py_len += 1; pi += 2
    }
    if(py_len == 0) { env.error("Python empty"); return }

    var mf: Mpi; var fv:[1]u8; fv[0]=5; mpi_read_binary(&raw mut mf, &raw fv[0], 1)
    var mm: Mpi; var mv:[1]u8; mv[0]=7; mpi_read_binary(&raw mut mm, &raw mv[0], 1)
    var tmp: Mpi; mpi_init(&raw mut tmp); mpi_add(&raw mut tmp, &raw mut p, &raw mut mf)
    var mr: Mpi; mpi_init(&raw mut mr); mpi_mod(&raw mut mr, &raw mut tmp, &raw mut mm)
    var cs = mpi_size(&raw mut mr); var cb:[8]u8; mpi_write_binary(&raw mut mr, &raw mut cb[0], 8)
    if(cs != py_len || (cs>0 && !test_bytes_eq(&raw cb[8-cs], &raw py_r[0], cs))) {
        printf("[MOD] cs=%d py_len=%d\n", cs as int, py_len as int)
        if(cs>0) { print_hex("chem", &raw cb[8-cs], cs) }
        if(py_len>0) { printf("[py] "); pi=0; while(pi<py_len){printf("%02x",py_r[pi]as int);pi+=1}; printf("\n") }
        env.error("mpi_mod mismatch")
    }
}

// === mpi_mod with negative: (3 - P-256) mod P-256 = 3 ===
@test public func TEST_mpi_negmod_vs_py(env:&mut TestEnv) {
    var p : Mpi; ecp_curve_p(&raw mut p)
    var p_bytes : [32]u8; mpi_get_bytes(&raw mut p, &raw mut p_bytes[0])
    var p_hex : [65]char; test_bytes_to_hex(&raw p_bytes[0], 32, &raw mut p_hex[0])

    var script : [512]u8; var sp : size_t = 0; var si : size_t = 0
    var hdr = "import sys; a=int.from_bytes(bytes.fromhex('" as *char
    si=0; while(hdr[si]!=0){script[sp]=hdr[si] as u8; sp+=1; si+=1}
    si=0; while(p_hex[si]!=0){script[sp]=p_hex[si] as u8; sp+=1; si+=1}
    var tail = "'),'big'); r=(3-a)%a; l=(r.bit_length()+7)//8 or 1; sys.stdout.write(r.to_bytes(l,'big').hex())" as *char
    si=0; while(tail[si]!=0){script[sp]=tail[si] as u8; sp+=1; si+=1}

    var py_out = test_python_run_script(&raw script[0], sp, string_view("mpi_negmod"))

    var py_r:[8]u8; var py_len : size_t = 0
    var pi : size_t = 0
    while(pi + 1 < py_out.size() && py_len < 8) {
        var hi = py_out.get(pi) as char; var lo = py_out.get(pi+1) as char
        if(hi == 10 as char || hi == 13 as char || hi == 0 as char) { break } else {}
        py_r[py_len] = test_hex_pair_byte(hi, lo); py_len += 1; pi += 2
    }
    if(py_len == 0) { env.error("Python empty"); return }

    var mthree: Mpi; var tv:[1]u8; tv[0]=3; mpi_read_binary(&raw mut mthree, &raw tv[0], 1)
    var tmp: Mpi; mpi_init(&raw mut tmp); var ret = mpi_sub(&raw mut tmp, &raw mut mthree, &raw mut p)
    if(ret < 0) { env.error("mpi_sub failed"); return }
    var mr: Mpi; mpi_init(&raw mut mr); ret = mpi_mod(&raw mut mr, &raw mut tmp, &raw mut p)
    if(ret < 0) { env.error("mpi_mod failed"); return }
    var cs = mpi_size(&raw mut mr); var cb:[8]u8; mpi_write_binary(&raw mut mr, &raw mut cb[0], 8)
    if(cs != py_len || (cs>0 && !test_bytes_eq(&raw cb[8-cs], &raw py_r[0], cs))) {
        printf("[NEGMOD] cs=%d py_len=%d\n", cs as int, py_len as int)
        if(cs>0) { print_hex("chem", &raw cb[8-cs], cs) }
        if(py_len>0) { printf("[py] "); pi=0; while(pi<py_len){printf("%02x",py_r[pi]as int);pi+=1}; printf("\n") }
        env.error("mpi_mod(negative) mismatch")
    }
}

// === P-256 prime self-consistency check ===
@test public func TEST_p256_prime(env:&mut TestEnv) {
    var p : Mpi; ecp_curve_p(&raw mut p)
    var expected : Mpi; ecp_curve_p(&raw mut expected)
    var buf1:[32]u8; var buf2:[32]u8
    mpi_write_binary(&raw mut p, &raw mut buf1[0], 32)
    mpi_write_binary(&raw mut expected, &raw mut buf2[0], 32)
    if(!test_bytes_eq(&raw buf1[0], &raw buf2[0], 32)) {
        print_hex("got", &raw buf1[0], 32)
        env.error("P-256 prime mismatch")
    }
}

// === 2*G test: check if doubling alone is correct ===
@test public func TEST_ecdh_k2_direct(env:&mut TestEnv) {
    var G : ECPPoint; ecp_point_init(&raw mut G)
    ecp_curve_gx(&raw mut G.X); ecp_curve_gy(&raw mut G.Y); mpi_lset(&raw mut G.Z, 1)
    var k : Mpi; mpi_init(&raw mut k); mpi_lset(&raw mut k, 2)
    var R : ECPPoint; ecp_point_init(&raw mut R)
    var ret = ecp_mul(&raw mut R, &raw mut k, &raw mut G)
    if(ret < 0) { printf("[K2] ecp_mul error=%d\n", ret); env.error("ecp_mul failed"); return }
    ret = ecp_normalize_jac(&raw mut R)
    if(ret < 0) { printf("[K2] normalize error=%d\n", ret); env.error("normalize failed"); return }
    var chem_x : [32]u8; ret = mpi_write_binary(&raw mut R.X, &raw mut chem_x[0], 32)
    if(ret < 0) { printf("[K2] write error=%d\n", ret); env.error("write failed"); return }

    // Python: compute 2*G
    var py_script : [512]u8; var psi:size_t=0
    py_script[psi]=105;psi+=1;py_script[psi]=109;psi+=1;py_script[psi]=112;psi+=1;py_script[psi]=111;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=32;psi+=1;py_script[psi]=115;psi+=1;py_script[psi]=121;psi+=1;py_script[psi]=115;psi+=1;py_script[psi]=10;psi+=1
    py_script[psi]=102;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=111;psi+=1;py_script[psi]=109;psi+=1;py_script[psi]=32;psi+=1;py_script[psi]=99;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=121;psi+=1;py_script[psi]=112;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=111;psi+=1;py_script[psi]=103;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=97;psi+=1;py_script[psi]=112;psi+=1;py_script[psi]=104;psi+=1;py_script[psi]=121;psi+=1;py_script[psi]=46;psi+=1;py_script[psi]=104;psi+=1;py_script[psi]=97;psi+=1;py_script[psi]=122;psi+=1;py_script[psi]=109;psi+=1;py_script[psi]=97;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=46;psi+=1;py_script[psi]=112;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=109;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=118;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=115;psi+=1;py_script[psi]=46;psi+=1;py_script[psi]=97;psi+=1;py_script[psi]=115;psi+=1;py_script[psi]=121;psi+=1;py_script[psi]=109;psi+=1;py_script[psi]=109;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=99;psi+=1;py_script[psi]=32;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=109;psi+=1;py_script[psi]=112;psi+=1;py_script[psi]=111;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=32;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=99;psi+=1;py_script[psi]=10;psi+=1
    py_script[psi]=102;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=111;psi+=1;py_script[psi]=109;psi+=1;py_script[psi]=32;psi+=1;py_script[psi]=99;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=121;psi+=1;py_script[psi]=112;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=111;psi+=1;py_script[psi]=103;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=97;psi+=1;py_script[psi]=112;psi+=1;py_script[psi]=104;psi+=1;py_script[psi]=121;psi+=1;py_script[psi]=46;psi+=1;py_script[psi]=104;psi+=1;py_script[psi]=97;psi+=1;py_script[psi]=122;psi+=1;py_script[psi]=109;psi+=1;py_script[psi]=97;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=46;psi+=1;py_script[psi]=112;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=109;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=118;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=115;psi+=1;py_script[psi]=46;psi+=1;py_script[psi]=115;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=97;psi+=1;py_script[psi]=108;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=122;psi+=1;py_script[psi]=97;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=111;psi+=1;py_script[psi]=110;psi+=1;py_script[psi]=32;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=109;psi+=1;py_script[psi]=112;psi+=1;py_script[psi]=111;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=32;psi+=1;py_script[psi]=69;psi+=1;py_script[psi]=110;psi+=1;py_script[psi]=99;psi+=1;py_script[psi]=111;psi+=1;py_script[psi]=100;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=110;psi+=1;py_script[psi]=103;psi+=1;py_script[psi]=44;psi+=1;py_script[psi]=80;psi+=1;py_script[psi]=117;psi+=1;py_script[psi]=98;psi+=1;py_script[psi]=108;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=99;psi+=1;py_script[psi]=70;psi+=1;py_script[psi]=111;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=109;psi+=1;py_script[psi]=97;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=10;psi+=1
    py_script[psi]=115;psi+=1;py_script[psi]=107;psi+=1;py_script[psi]=61;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=99;psi+=1;py_script[psi]=46;psi+=1;py_script[psi]=100;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=118;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=95;psi+=1;py_script[psi]=112;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=118;psi+=1;py_script[psi]=97;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=95;psi+=1;py_script[psi]=107;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=121;psi+=1;py_script[psi]=40;psi+=1;py_script[psi]=50;psi+=1;py_script[psi]=44;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=99;psi+=1;py_script[psi]=46;psi+=1;py_script[psi]=83;psi+=1;py_script[psi]=69;psi+=1;py_script[psi]=67;psi+=1;py_script[psi]=80;psi+=1;py_script[psi]=50;psi+=1;py_script[psi]=53;psi+=1;py_script[psi]=54;psi+=1;py_script[psi]=82;psi+=1;py_script[psi]=49;psi+=1;py_script[psi]=40;psi+=1;py_script[psi]=41;psi+=1;py_script[psi]=41;psi+=1;py_script[psi]=10;psi+=1
    py_script[psi]=112;psi+=1;py_script[psi]=117;psi+=1;py_script[psi]=98;psi+=1;py_script[psi]=61;psi+=1;py_script[psi]=115;psi+=1;py_script[psi]=107;psi+=1;py_script[psi]=46;psi+=1;py_script[psi]=112;psi+=1;py_script[psi]=117;psi+=1;py_script[psi]=98;psi+=1;py_script[psi]=108;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=99;psi+=1;py_script[psi]=95;psi+=1;py_script[psi]=107;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=121;psi+=1;py_script[psi]=40;psi+=1;py_script[psi]=41;psi+=1;py_script[psi]=46;psi+=1;py_script[psi]=112;psi+=1;py_script[psi]=117;psi+=1;py_script[psi]=98;psi+=1;py_script[psi]=108;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=99;psi+=1;py_script[psi]=95;psi+=1;py_script[psi]=98;psi+=1;py_script[psi]=121;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=115;psi+=1;py_script[psi]=40;psi+=1;py_script[psi]=69;psi+=1;py_script[psi]=110;psi+=1;py_script[psi]=99;psi+=1;py_script[psi]=111;psi+=1;py_script[psi]=100;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=110;psi+=1;py_script[psi]=103;psi+=1;py_script[psi]=46;psi+=1;py_script[psi]=88;psi+=1;py_script[psi]=57;psi+=1;py_script[psi]=54;psi+=1;py_script[psi]=50;psi+=1;py_script[psi]=44;psi+=1;py_script[psi]=80;psi+=1;py_script[psi]=117;psi+=1;py_script[psi]=98;psi+=1;py_script[psi]=108;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=99;psi+=1;py_script[psi]=70;psi+=1;py_script[psi]=111;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=109;psi+=1;py_script[psi]=97;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=46;psi+=1;py_script[psi]=85;psi+=1;py_script[psi]=110;psi+=1;py_script[psi]=99;psi+=1;py_script[psi]=111;psi+=1;py_script[psi]=109;psi+=1;py_script[psi]=112;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=115;psi+=1;py_script[psi]=115;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=100;psi+=1;py_script[psi]=80;psi+=1;py_script[psi]=111;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=110;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=41;psi+=1;py_script[psi]=10;psi+=1
    py_script[psi]=115;psi+=1;py_script[psi]=121;psi+=1;py_script[psi]=115;psi+=1;py_script[psi]=46;psi+=1;py_script[psi]=115;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=100;psi+=1;py_script[psi]=111;psi+=1;py_script[psi]=117;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=46;psi+=1;py_script[psi]=119;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=40;psi+=1;py_script[psi]=112;psi+=1;py_script[psi]=117;psi+=1;py_script[psi]=98;psi+=1;py_script[psi]=46;psi+=1;py_script[psi]=104;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=120;psi+=1;py_script[psi]=40;psi+=1;py_script[psi]=41;psi+=1;py_script[psi]=41;psi+=1;py_script[psi]=10;psi+=1

    var py_pub:[65]u8; var py_len = run_py_script(&raw py_script[0], psi, &raw mut py_pub[0], 65)
    if(py_len < 65) { printf("[K2] Python output too short: %d\n", py_len as int); env.error("python failed"); return }

    printf("[K2] chem=%064x py=%064x\n", chem_x[0] as int, py_pub[1] as int)
    if(!test_bytes_eq(&raw chem_x[0], &raw py_pub[1], 32)) {
        printf("[K2] X mismatch\n")
        print_hex("chem", &raw chem_x[0], 32); print_hex("py  ", &raw py_pub[1], 32)
        env.error("k2 mismatch")
    }
}

// === 2G + G using mixed Jacobian-affine addition ===
@test public func TEST_ecdh_add(env:&mut TestEnv) {
    var G : ECPPoint; ecp_point_init(&raw mut G)
    ecp_curve_gx(&raw mut G.X); ecp_curve_gy(&raw mut G.Y); mpi_lset(&raw mut G.Z, 1)

    var k2 : Mpi; mpi_init(&raw mut k2); mpi_lset(&raw mut k2, 2)
    var R2 : ECPPoint; ecp_point_init(&raw mut R2)
    var ret = ecp_mul(&raw mut R2, &raw mut k2, &raw mut G)
    if(ret < 0) { env.error("ecp_mul k=2"); return }

    var P_affine : ECPPoint; ecp_point_init(&raw mut P_affine)
    ecp_curve_gx(&raw mut P_affine.X); ecp_curve_gy(&raw mut P_affine.Y); mpi_lset(&raw mut P_affine.Z, 1)
    var R3_add : ECPPoint; ecp_point_init(&raw mut R3_add)
    ret = ecp_add_jac(&raw mut R3_add, &raw mut R2, &raw mut P_affine)
    if(ret < 0) { env.error("ecp_add_jac failed"); return }
    ret = ecp_normalize_jac(&raw mut R3_add)
    if(ret < 0) { env.error("normalize failed"); return }
    var add_x : [32]u8; mpi_write_binary(&raw mut R3_add.X, &raw mut add_x[0], 32)

    var k3 : Mpi; mpi_init(&raw mut k3); mpi_lset(&raw mut k3, 3)
    var R3_mul : ECPPoint; ecp_point_init(&raw mut R3_mul)
    ret = ecp_mul(&raw mut R3_mul, &raw mut k3, &raw mut G)
    if(ret < 0) { env.error("ecp_mul k=3"); return }
    ret = ecp_normalize_jac(&raw mut R3_mul)
    if(ret < 0) { env.error("normalize mul"); return }
    var mul_x : [32]u8; mpi_write_binary(&raw mut R3_mul.X, &raw mut mul_x[0], 32)

    if(!test_bytes_eq(&raw add_x[0], &raw mul_x[0], 32)) {
        printf("[ADD] add="); var _adi:size_t=0; while(_adi<32){printf("%02x",add_x[_adi]as int);_adi+=1}; printf("\n")
        printf("[ADD] mul="); _adi=0; while(_adi<32){printf("%02x",mul_x[_adi]as int);_adi+=1}; printf("\n")
        env.error("add != mul")
    }
}

// === ECDH: 3*G computed with ecp_mul, compared against Python ===
@test public func TEST_ecdh_k3_direct(env:&mut TestEnv) {
    var py_script : [512]u8; var psi : size_t = 0
    py_script[psi]=105; psi+=1; py_script[psi]=109; psi+=1; py_script[psi]=112; psi+=1
    py_script[psi]=111; psi+=1; py_script[psi]=114; psi+=1; py_script[psi]=116; psi+=1
    py_script[psi]=32; psi+=1; py_script[psi]=115; psi+=1; py_script[psi]=121; psi+=1
    py_script[psi]=115; psi+=1; py_script[psi]=10; psi+=1
    py_script[psi]=102; psi+=1; py_script[psi]=114; psi+=1; py_script[psi]=111; psi+=1
    py_script[psi]=109; psi+=1; py_script[psi]=32; psi+=1
    py_script[psi]=99; psi+=1; py_script[psi]=114; psi+=1; py_script[psi]=121; psi+=1
    py_script[psi]=112; psi+=1; py_script[psi]=116; psi+=1; py_script[psi]=111; psi+=1
    py_script[psi]=103; psi+=1; py_script[psi]=114; psi+=1; py_script[psi]=97; psi+=1
    py_script[psi]=112; psi+=1; py_script[psi]=104; psi+=1; py_script[psi]=121; psi+=1
    py_script[psi]=46; psi+=1; py_script[psi]=104; psi+=1; py_script[psi]=97; psi+=1
    py_script[psi]=122; psi+=1; py_script[psi]=109; psi+=1; py_script[psi]=97; psi+=1
    py_script[psi]=116; psi+=1; py_script[psi]=46; psi+=1; py_script[psi]=112; psi+=1
    py_script[psi]=114; psi+=1; py_script[psi]=105; psi+=1; py_script[psi]=109; psi+=1
    py_script[psi]=105; psi+=1; py_script[psi]=116; psi+=1; py_script[psi]=105; psi+=1
    py_script[psi]=118; psi+=1; py_script[psi]=101; psi+=1; py_script[psi]=115; psi+=1
    py_script[psi]=46; psi+=1; py_script[psi]=97; psi+=1; py_script[psi]=115; psi+=1
    py_script[psi]=121; psi+=1; py_script[psi]=109; psi+=1; py_script[psi]=109; psi+=1
    py_script[psi]=101; psi+=1; py_script[psi]=116; psi+=1; py_script[psi]=114; psi+=1
    py_script[psi]=105; psi+=1; py_script[psi]=99; psi+=1; py_script[psi]=32; psi+=1
    py_script[psi]=105; psi+=1; py_script[psi]=109; psi+=1; py_script[psi]=112; psi+=1
    py_script[psi]=111; psi+=1; py_script[psi]=114; psi+=1; py_script[psi]=116; psi+=1
    py_script[psi]=32; psi+=1; py_script[psi]=101; psi+=1; py_script[psi]=99; psi+=1
    py_script[psi]=10; psi+=1
    py_script[psi]=102; psi+=1; py_script[psi]=114; psi+=1; py_script[psi]=111; psi+=1
    py_script[psi]=109; psi+=1; py_script[psi]=32; psi+=1
    py_script[psi]=99; psi+=1; py_script[psi]=114; psi+=1; py_script[psi]=121; psi+=1
    py_script[psi]=112; psi+=1; py_script[psi]=116; psi+=1; py_script[psi]=111; psi+=1
    py_script[psi]=103; psi+=1; py_script[psi]=114; psi+=1; py_script[psi]=97; psi+=1
    py_script[psi]=112; psi+=1; py_script[psi]=104; psi+=1; py_script[psi]=121; psi+=1
    py_script[psi]=46; psi+=1; py_script[psi]=104; psi+=1; py_script[psi]=97; psi+=1
    py_script[psi]=122; psi+=1; py_script[psi]=109; psi+=1; py_script[psi]=97; psi+=1
    py_script[psi]=116; psi+=1; py_script[psi]=46; psi+=1; py_script[psi]=112; psi+=1
    py_script[psi]=114; psi+=1; py_script[psi]=105; psi+=1; py_script[psi]=109; psi+=1
    py_script[psi]=105; psi+=1; py_script[psi]=116; psi+=1; py_script[psi]=105; psi+=1
    py_script[psi]=118; psi+=1; py_script[psi]=101; psi+=1; py_script[psi]=115; psi+=1
    py_script[psi]=46; psi+=1; py_script[psi]=115; psi+=1; py_script[psi]=101; psi+=1
    py_script[psi]=114; psi+=1; py_script[psi]=105; psi+=1; py_script[psi]=97; psi+=1
    py_script[psi]=108; psi+=1; py_script[psi]=105; psi+=1; py_script[psi]=122; psi+=1
    py_script[psi]=97; psi+=1; py_script[psi]=116; psi+=1; py_script[psi]=105; psi+=1
    py_script[psi]=111; psi+=1; py_script[psi]=110; psi+=1; py_script[psi]=32; psi+=1
    py_script[psi]=105; psi+=1; py_script[psi]=109; psi+=1; py_script[psi]=112; psi+=1
    py_script[psi]=111; psi+=1; py_script[psi]=114; psi+=1; py_script[psi]=116; psi+=1
    py_script[psi]=32; psi+=1; py_script[psi]=69; psi+=1; py_script[psi]=110; psi+=1
    py_script[psi]=99; psi+=1; py_script[psi]=111; psi+=1; py_script[psi]=100; psi+=1
    py_script[psi]=105; psi+=1; py_script[psi]=110; psi+=1; py_script[psi]=103; psi+=1
    py_script[psi]=44; psi+=1; py_script[psi]=32; psi+=1; py_script[psi]=80; psi+=1
    py_script[psi]=117; psi+=1; py_script[psi]=98; psi+=1; py_script[psi]=108; psi+=1
    py_script[psi]=105; psi+=1; py_script[psi]=99; psi+=1; py_script[psi]=70; psi+=1
    py_script[psi]=111; psi+=1; py_script[psi]=114; psi+=1; py_script[psi]=109; psi+=1
    py_script[psi]=97; psi+=1; py_script[psi]=116; psi+=1; py_script[psi]=10; psi+=1
    py_script[psi]=115; psi+=1; py_script[psi]=107; psi+=1; py_script[psi]=61; psi+=1
    py_script[psi]=101; psi+=1; py_script[psi]=99; psi+=1; py_script[psi]=46; psi+=1
    py_script[psi]=100; psi+=1; py_script[psi]=101; psi+=1; py_script[psi]=114; psi+=1
    py_script[psi]=105; psi+=1; py_script[psi]=118; psi+=1; py_script[psi]=101; psi+=1
    py_script[psi]=95; psi+=1; py_script[psi]=112; psi+=1; py_script[psi]=114; psi+=1
    py_script[psi]=105; psi+=1; py_script[psi]=118; psi+=1; py_script[psi]=97; psi+=1
    py_script[psi]=116; psi+=1; py_script[psi]=101; psi+=1; py_script[psi]=95; psi+=1
    py_script[psi]=107; psi+=1; py_script[psi]=101; psi+=1; py_script[psi]=121; psi+=1
    py_script[psi]=40; psi+=1; py_script[psi]=51; psi+=1; py_script[psi]=44; psi+=1
    py_script[psi]=101; psi+=1; py_script[psi]=99; psi+=1; py_script[psi]=46; psi+=1
    py_script[psi]=83; psi+=1; py_script[psi]=69; psi+=1; py_script[psi]=67; psi+=1
    py_script[psi]=80; psi+=1; py_script[psi]=50; psi+=1; py_script[psi]=53; psi+=1
    py_script[psi]=54; psi+=1; py_script[psi]=82; psi+=1; py_script[psi]=49; psi+=1
    py_script[psi]=40; psi+=1; py_script[psi]=41; psi+=1; py_script[psi]=41; psi+=1
    py_script[psi]=10; psi+=1
    py_script[psi]=112; psi+=1; py_script[psi]=117; psi+=1; py_script[psi]=98; psi+=1
    py_script[psi]=61; psi+=1; py_script[psi]=115; psi+=1; py_script[psi]=107; psi+=1
    py_script[psi]=46; psi+=1; py_script[psi]=112; psi+=1; py_script[psi]=117; psi+=1
    py_script[psi]=98; psi+=1; py_script[psi]=108; psi+=1; py_script[psi]=105; psi+=1
    py_script[psi]=99; psi+=1; py_script[psi]=95; psi+=1; py_script[psi]=107; psi+=1
    py_script[psi]=101; psi+=1; py_script[psi]=121; psi+=1; py_script[psi]=40; psi+=1
    py_script[psi]=41; psi+=1; py_script[psi]=46; psi+=1; py_script[psi]=112; psi+=1
    py_script[psi]=117; psi+=1; py_script[psi]=98; psi+=1; py_script[psi]=108; psi+=1
    py_script[psi]=105; psi+=1; py_script[psi]=99; psi+=1; py_script[psi]=95; psi+=1
    py_script[psi]=98; psi+=1; py_script[psi]=121; psi+=1; py_script[psi]=116; psi+=1
    py_script[psi]=101; psi+=1; py_script[psi]=115; psi+=1; py_script[psi]=40; psi+=1
    py_script[psi]=69; psi+=1; py_script[psi]=110; psi+=1; py_script[psi]=99; psi+=1
    py_script[psi]=111; psi+=1; py_script[psi]=100; psi+=1; py_script[psi]=105; psi+=1
    py_script[psi]=110; psi+=1; py_script[psi]=103; psi+=1; py_script[psi]=46; psi+=1
    py_script[psi]=88; psi+=1; py_script[psi]=57; psi+=1; py_script[psi]=54; psi+=1
    py_script[psi]=50; psi+=1; py_script[psi]=44; psi+=1; py_script[psi]=80; psi+=1
    py_script[psi]=117; psi+=1; py_script[psi]=98; psi+=1; py_script[psi]=108; psi+=1
    py_script[psi]=105; psi+=1; py_script[psi]=99; psi+=1; py_script[psi]=70; psi+=1
    py_script[psi]=111; psi+=1; py_script[psi]=114; psi+=1; py_script[psi]=109; psi+=1
    py_script[psi]=97; psi+=1; py_script[psi]=116; psi+=1; py_script[psi]=46; psi+=1
    py_script[psi]=85; psi+=1; py_script[psi]=110; psi+=1; py_script[psi]=99; psi+=1
    py_script[psi]=111; psi+=1; py_script[psi]=109; psi+=1; py_script[psi]=112; psi+=1
    py_script[psi]=114; psi+=1; py_script[psi]=101; psi+=1; py_script[psi]=115; psi+=1
    py_script[psi]=115; psi+=1; py_script[psi]=101; psi+=1; py_script[psi]=100; psi+=1
    py_script[psi]=80; psi+=1; py_script[psi]=111; psi+=1; py_script[psi]=105; psi+=1
    py_script[psi]=110; psi+=1; py_script[psi]=116; psi+=1; py_script[psi]=41; psi+=1
    py_script[psi]=10; psi+=1
    py_script[psi]=115; psi+=1; py_script[psi]=121; psi+=1; py_script[psi]=115; psi+=1
    py_script[psi]=46; psi+=1; py_script[psi]=115; psi+=1; py_script[psi]=116; psi+=1
    py_script[psi]=100; psi+=1; py_script[psi]=111; psi+=1; py_script[psi]=117; psi+=1
    py_script[psi]=116; psi+=1; py_script[psi]=46; psi+=1; py_script[psi]=119; psi+=1
    py_script[psi]=114; psi+=1; py_script[psi]=105; psi+=1; py_script[psi]=116; psi+=1
    py_script[psi]=101; psi+=1; py_script[psi]=40; psi+=1; py_script[psi]=112; psi+=1
    py_script[psi]=117; psi+=1; py_script[psi]=98; psi+=1; py_script[psi]=46; psi+=1
    py_script[psi]=104; psi+=1; py_script[psi]=101; psi+=1; py_script[psi]=120; psi+=1
    py_script[psi]=40; psi+=1; py_script[psi]=41; psi+=1; py_script[psi]=41; psi+=1
    py_script[psi]=10; psi+=1

    var py_pub:[65]u8; var py_len = run_py_script(&raw py_script[0], psi, &raw mut py_pub[0], 65)
    if(py_len < 65) { printf("[K3] Python output too short: %d\n", py_len as int); env.error("python failed"); return }

    var G : ECPPoint; ecp_point_init(&raw mut G)
    ecp_curve_gx(&raw mut G.X); ecp_curve_gy(&raw mut G.Y); mpi_lset(&raw mut G.Z, 1)
    var k : Mpi; mpi_init(&raw mut k); mpi_lset(&raw mut k, 3)
    var R : ECPPoint; ecp_point_init(&raw mut R)
    var ret = ecp_mul(&raw mut R, &raw mut k, &raw mut G)
    if(ret < 0) { printf("[K3] ecp_mul error=%d\n", ret); env.error("ecp_mul failed"); return }
    ret = ecp_normalize_jac(&raw mut R)
    if(ret < 0) { printf("[K3] normalize error=%d\n", ret); env.error("normalize failed"); return }
    var chem_x : [32]u8; ret = mpi_write_binary(&raw mut R.X, &raw mut chem_x[0], 32)
    if(ret < 0) { printf("[K3] write error=%d\n", ret); env.error("write failed"); return }
    if(!test_bytes_eq(&raw chem_x[0], &raw py_pub[1], 32)) {
        printf("[K3] X mismatch\n")
        print_hex("chem", &raw chem_x[0], 32); print_hex("py  ", &raw py_pub[1], 32)
        env.error("k3 mismatch")
    }
}

// === Curve equation: G on curve ===
@test public func TEST_curve_equation_stepwise(env:&mut TestEnv) {
    var G : ECPPoint; ecp_point_init(&raw mut G)
    ecp_curve_gx(&raw mut G.X); ecp_curve_gy(&raw mut G.Y); mpi_lset(&raw mut G.Z, 1)
    var p : Mpi; ecp_curve_p(&raw mut p)
    var b_m : Mpi; ecp_curve_b(&raw mut b_m)
    var lhs : Mpi; mpi_init(&raw mut lhs)
    mpi_mul(&raw mut lhs, &raw mut G.Y, &raw mut G.Y); mpi_mod(&raw mut lhs, &raw mut lhs, &raw mut p)
    var x_sq : Mpi; mpi_init(&raw mut x_sq)
    mpi_mul(&raw mut x_sq, &raw mut G.X, &raw mut G.X); mpi_mod(&raw mut x_sq, &raw mut x_sq, &raw mut p)
    var x_cu : Mpi; mpi_init(&raw mut x_cu)
    mpi_mul(&raw mut x_cu, &raw mut x_sq, &raw mut G.X); mpi_mod(&raw mut x_cu, &raw mut x_cu, &raw mut p)
    var three_x : Mpi; mpi_init(&raw mut three_x)
    mpi_mul_int(&raw mut three_x, &raw mut G.X, 3); mpi_mod(&raw mut three_x, &raw mut three_x, &raw mut p)
    var rhs : Mpi; mpi_init(&raw mut rhs)
    mpi_sub(&raw mut rhs, &raw mut x_cu, &raw mut three_x); mpi_mod(&raw mut rhs, &raw mut rhs, &raw mut p)
    mpi_add(&raw mut rhs, &raw mut rhs, &raw mut b_m); mpi_mod(&raw mut rhs, &raw mut rhs, &raw mut p)
    if(mpi_cmp(&raw mut lhs, &raw mut rhs) != 0) {
        printf("[CURVE] G NOT on curve!\n")
        env.error("G not on curve")
    }
}

@test public func TEST_mpi_mul_simple(env:&mut TestEnv) {
    var a:[32]u8; a[31]=7
    var ma:Mpi; mpi_read_binary(&raw mut ma, &raw a[0], 32)
    var mr:Mpi; mpi_init(&raw mut mr)
    var ret = mpi_mul_int(&raw mut mr, &raw mut ma, 6)
    var cs=mpi_size(&raw mut mr); var cb:[8]u8; mpi_write_binary(&raw mut mr, &raw mut cb[0], 8)
    if(ret<0 || cs!=1 || cb[7]!=42){
        printf("[MUL_SIMP] ret=%d cs=%d cb[7]=%d\n", ret, cs as int, cb[7] as int)
        env.error("mpi_mul_int(7,6) failed")
    }
}
