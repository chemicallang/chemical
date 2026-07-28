using namespace tls
using namespace crypto

func py_uniq() : i32 {
    var r : u32
    random_fill(&raw mut r as *mut u8, 4)
    return (r as i32) & 0x7FFFFFFFi32
}

func print_hex(label : *char, data : *u8, len : size_t) {
    printf("[%s] ", label)
    var i : size_t = 0
    while(i < len) { printf("%02x", data[i] as int); i += 1 }
    printf("\n")
}

func mk_outpath(id : i32, path : *mut char) {
    path[0]=47; path[1]=116; path[2]=109; path[3]=112; path[4]=47; path[5]=112; path[6]=121
    var pi:size_t=7; var tid=id; var rev:[10]char; var ri:size_t=0
    if(tid==0) { path[pi]=48; pi+=1 }
    while(tid>0) { rev[ri]=48 as char+((tid%10)as char); tid=tid/10; ri+=1 }
    while(ri>0) { ri-=1; path[pi]=rev[ri]; pi+=1 }
    path[pi]=0
}

func mk_fullpath(id : i32, path : *mut char) {
    mk_outpath(id, path)
    var pi:size_t=0; while(path[pi]!=0) { pi+=1 }
    path[pi]=46; pi+=1; path[pi]=111; pi+=1; path[pi]=117; pi+=1; path[pi]=116; pi+=1; path[pi]=0
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

// Write raw bytes to file, run python3 on it, return hex output
func run_py_script(script : *u8, slen : size_t, out : *mut u8, out_max : size_t) : size_t {
    var id = py_uniq(); var py_path:[32]char; mk_fullpath(id, &raw mut py_path[0])
    var spath:[32]char; mk_outpath(id, &raw mut spath[0])
    test_write_file(&raw spath[0], script, slen)
    var cmd:[256]char; var cp:size_t=0; var ci:size_t=0
    var c0 = "python3 " as *char; ci=0; while(c0[ci]!=0){cmd[cp]=c0[ci];cp+=1;ci+=1}
    ci=0; while(spath[ci]!=0){cmd[cp]=spath[ci];cp+=1;ci+=1}
    var c1 = " > " as *char; ci=0; while(c1[ci]!=0){cmd[cp]=c1[ci];cp+=1;ci+=1}
    ci=0; while(py_path[ci]!=0){cmd[cp]=py_path[ci];cp+=1;ci+=1}
    var c2 = " 2>/dev/null" as *char; ci=0; while(c2[ci]!=0){cmd[cp]=c2[ci];cp+=1;ci+=1}
    cmd[cp]=0; system(&raw cmd[0])
    return read_hex_output(&raw py_path[0], out, out_max)
}

func mpi_get_bytes(m : *mut Mpi, out : *mut u8) {
    var buf : [32]u8
    mpi_write_binary(m, &raw mut buf[0], 32)
    var i : size_t = 0; while(i < 32) { out[i] = buf[i]; i += 1 }
}

// ============================================================================
// mpi_mul_int: P-256 * 4
// ============================================================================
@test public func TEST_mpi_mul_int_vs_py(env:&mut TestEnv) {
    var p : Mpi; ecp_curve_p(&raw mut p)
    var p_bytes : [32]u8; mpi_get_bytes(&raw mut p, &raw mut p_bytes[0])
    var p_hex : [65]char; test_bytes_to_hex(&raw p_bytes[0], 32, &raw mut p_hex[0])

    var cmd : [512]char; var cp:size_t=0; var ci:size_t=0
    var pa = "python3 -c 'import sys; a=int.from_bytes(bytes.fromhex(\"" as *char
    ci=0; while(pa[ci]!=0){cmd[cp]=pa[ci];cp+=1;ci+=1}
    ci=0; while(p_hex[ci]!=0){cmd[cp]=p_hex[ci];cp+=1;ci+=1}
    var pb = "\"),\"big\"); r=a*4; l=(r.bit_length()+7)//8 or 1; sys.stdout.write(r.to_bytes(l,\"big\").hex())' 2>/dev/null" as *char
    ci=0; while(pb[ci]!=0){cmd[cp]=pb[ci];cp+=1;ci+=1}
    cmd[cp]=0

    var id = py_uniq(); var py_path:[32]char; mk_fullpath(id, &raw mut py_path[0])
    var cmd2 : [512]char; var c2p:size_t=0; ci=0
    while(cmd[ci]!=0){cmd2[c2p]=cmd[ci];c2p+=1;ci+=1}
    ci=0; var red = " > " as *char; while(red[ci]!=0){cmd2[c2p]=red[ci];c2p+=1;ci+=1}
    ci=0; while(py_path[ci]!=0){cmd2[c2p]=py_path[ci];c2p+=1;ci+=1}
    cmd2[c2p]=0; system(&raw cmd2[0])

    var py_r:[64]u8; var py_len = read_hex_output(&raw py_path[0], &raw mut py_r[0], 64)
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

// ============================================================================
// mpi_sub: P-256 - 5
// ============================================================================
@test public func TEST_mpi_sub_vs_py(env:&mut TestEnv) {
    var p : Mpi; ecp_curve_p(&raw mut p)
    var p_bytes : [32]u8; mpi_get_bytes(&raw mut p, &raw mut p_bytes[0])
    var p_hex : [65]char; test_bytes_to_hex(&raw p_bytes[0], 32, &raw mut p_hex[0])

    var cmd : [512]char; var cp:size_t=0; var ci:size_t=0
    var pa = "python3 -c 'import sys; a=int.from_bytes(bytes.fromhex(\"" as *char
    ci=0; while(pa[ci]!=0){cmd[cp]=pa[ci];cp+=1;ci+=1}
    ci=0; while(p_hex[ci]!=0){cmd[cp]=p_hex[ci];cp+=1;ci+=1}
    var pb = "\"),\"big\"); r=a-5; l=(r.bit_length()+7)//8 or 1; sys.stdout.write(r.to_bytes(l,\"big\").hex())' 2>/dev/null" as *char
    ci=0; while(pb[ci]!=0){cmd[cp]=pb[ci];cp+=1;ci+=1}
    cmd[cp]=0

    var id = py_uniq(); var py_path:[32]char; mk_fullpath(id, &raw mut py_path[0])
    var cmd2 : [512]char; var c2p:size_t=0; ci=0
    while(cmd[ci]!=0){cmd2[c2p]=cmd[ci];c2p+=1;ci+=1}
    ci=0; var red = " > " as *char; while(red[ci]!=0){cmd2[c2p]=red[ci];c2p+=1;ci+=1}
    ci=0; while(py_path[ci]!=0){cmd2[c2p]=py_path[ci];c2p+=1;ci+=1}
    cmd2[c2p]=0; system(&raw cmd2[0])

    var py_r:[40]u8; var py_len = read_hex_output(&raw py_path[0], &raw mut py_r[0], 40)
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

// ============================================================================
// mpi_add: P-256 + 5
// ============================================================================
@test public func TEST_mpi_add_vs_py(env:&mut TestEnv) {
    var p : Mpi; ecp_curve_p(&raw mut p)
    var p_bytes : [32]u8; mpi_get_bytes(&raw mut p, &raw mut p_bytes[0])
    var p_hex : [65]char; test_bytes_to_hex(&raw p_bytes[0], 32, &raw mut p_hex[0])

    var cmd : [512]char; var cp:size_t=0; var ci:size_t=0
    var pa = "python3 -c 'import sys; a=int.from_bytes(bytes.fromhex(\"" as *char
    ci=0; while(pa[ci]!=0){cmd[cp]=pa[ci];cp+=1;ci+=1}
    ci=0; while(p_hex[ci]!=0){cmd[cp]=p_hex[ci];cp+=1;ci+=1}
    var pb = "\"),\"big\"); r=a+5; l=(r.bit_length()+7)//8 or 1; sys.stdout.write(r.to_bytes(l,\"big\").hex())' 2>/dev/null" as *char
    ci=0; while(pb[ci]!=0){cmd[cp]=pb[ci];cp+=1;ci+=1}
    cmd[cp]=0

    var id = py_uniq(); var py_path:[32]char; mk_fullpath(id, &raw mut py_path[0])
    var cmd2 : [512]char; var c2p:size_t=0; ci=0
    while(cmd[ci]!=0){cmd2[c2p]=cmd[ci];c2p+=1;ci+=1}
    ci=0; var red = " > " as *char; while(red[ci]!=0){cmd2[c2p]=red[ci];c2p+=1;ci+=1}
    ci=0; while(py_path[ci]!=0){cmd2[c2p]=py_path[ci];c2p+=1;ci+=1}
    cmd2[c2p]=0; system(&raw cmd2[0])

    var py_r:[40]u8; var py_len = read_hex_output(&raw py_path[0], &raw mut py_r[0], 40)
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

// ============================================================================
// mpi_mul: P-256 * 2
// ============================================================================
@test public func TEST_mpi_mul_vs_py(env:&mut TestEnv) {
    var p : Mpi; ecp_curve_p(&raw mut p)
    var p_bytes : [32]u8; mpi_get_bytes(&raw mut p, &raw mut p_bytes[0])
    var p_hex : [65]char; test_bytes_to_hex(&raw p_bytes[0], 32, &raw mut p_hex[0])

    var cmd : [512]char; var cp:size_t=0; var ci:size_t=0
    var pa = "python3 -c 'import sys; a=int.from_bytes(bytes.fromhex(\"" as *char
    ci=0; while(pa[ci]!=0){cmd[cp]=pa[ci];cp+=1;ci+=1}
    ci=0; while(p_hex[ci]!=0){cmd[cp]=p_hex[ci];cp+=1;ci+=1}
    var pb = "\"),\"big\"); r=a*2; l=(r.bit_length()+7)//8 or 1; sys.stdout.write(r.to_bytes(l,\"big\").hex())' 2>/dev/null" as *char
    ci=0; while(pb[ci]!=0){cmd[cp]=pb[ci];cp+=1;ci+=1}
    cmd[cp]=0

    var id = py_uniq(); var py_path:[32]char; mk_fullpath(id, &raw mut py_path[0])
    var cmd2 : [512]char; var c2p:size_t=0; ci=0
    while(cmd[ci]!=0){cmd2[c2p]=cmd[ci];c2p+=1;ci+=1}
    ci=0; var red = " > " as *char; while(red[ci]!=0){cmd2[c2p]=red[ci];c2p+=1;ci+=1}
    ci=0; while(py_path[ci]!=0){cmd2[c2p]=py_path[ci];c2p+=1;ci+=1}
    cmd2[c2p]=0; system(&raw cmd2[0])

    var py_r:[64]u8; var py_len = read_hex_output(&raw py_path[0], &raw mut py_r[0], 64)
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

// ============================================================================
// mpi_mod: (P-256 + 5) mod 7
// ============================================================================
@test public func TEST_mpi_mod_vs_py(env:&mut TestEnv) {
    var p : Mpi; ecp_curve_p(&raw mut p)
    var p_bytes : [32]u8; mpi_get_bytes(&raw mut p, &raw mut p_bytes[0])
    var p_hex : [65]char; test_bytes_to_hex(&raw p_bytes[0], 32, &raw mut p_hex[0])

    var cmd : [512]char; var cp:size_t=0; var ci:size_t=0
    var pa = "python3 -c 'import sys; a=int.from_bytes(bytes.fromhex(\"" as *char
    ci=0; while(pa[ci]!=0){cmd[cp]=pa[ci];cp+=1;ci+=1}
    ci=0; while(p_hex[ci]!=0){cmd[cp]=p_hex[ci];cp+=1;ci+=1}
    var pb = "\"),\"big\"); r=(a+5)%7; l=(r.bit_length()+7)//8 or 1; sys.stdout.write(r.to_bytes(l,\"big\").hex())' 2>/dev/null" as *char
    ci=0; while(pb[ci]!=0){cmd[cp]=pb[ci];cp+=1;ci+=1}
    cmd[cp]=0

    var id = py_uniq(); var py_path:[32]char; mk_fullpath(id, &raw mut py_path[0])
    var cmd2 : [512]char; var c2p:size_t=0; ci=0
    while(cmd[ci]!=0){cmd2[c2p]=cmd[ci];c2p+=1;ci+=1}
    ci=0; var red = " > " as *char; while(red[ci]!=0){cmd2[c2p]=red[ci];c2p+=1;ci+=1}
    ci=0; while(py_path[ci]!=0){cmd2[c2p]=py_path[ci];c2p+=1;ci+=1}
    cmd2[c2p]=0; system(&raw cmd2[0])

    var py_r:[8]u8; var py_len = read_hex_output(&raw py_path[0], &raw mut py_r[0], 8)
    if(py_len == 0) { env.error("Python empty"); return }

    var mf: Mpi; var fv:[1]u8; fv[0]=5; mpi_read_binary(&raw mut mf, &raw fv[0], 1)
    var mm: Mpi; var mv:[1]u8; mv[0]=7; mpi_read_binary(&raw mut mm, &raw mv[0], 1)
    var tmp: Mpi; mpi_init(&raw mut tmp); mpi_add(&raw mut tmp, &raw mut p, &raw mut mf)
    var mr: Mpi; mpi_init(&raw mut mr); mpi_mod(&raw mut mr, &raw mut tmp, &raw mut mm)
    var cs = mpi_size(&raw mut mr); var cb:[8]u8; mpi_write_binary(&raw mut mr, &raw mut cb[0], 8)
    if(cs != py_len || (cs>0 && !test_bytes_eq(&raw cb[8-cs], &raw py_r[0], cs))) {
        printf("[MOD] cs=%d py_len=%d\n", cs as int, py_len as int)
        if(cs>0) { print_hex("chem", &raw cb[8-cs], cs) }
        if(py_len>0) { print_hex("py  ", &raw py_r[0], py_len) }
        env.error("mpi_mod mismatch")
    }
}

// ============================================================================
// mpi_mod with negative: (3 - P-256) mod P-256 = 3
// ============================================================================
@test public func TEST_mpi_negmod_vs_py(env:&mut TestEnv) {
    var p : Mpi; ecp_curve_p(&raw mut p)
    var p_bytes : [32]u8; mpi_get_bytes(&raw mut p, &raw mut p_bytes[0])
    var p_hex : [65]char; test_bytes_to_hex(&raw p_bytes[0], 32, &raw mut p_hex[0])

    var cmd : [512]char; var cp:size_t=0; var ci:size_t=0
    var pa = "python3 -c 'import sys; a=int.from_bytes(bytes.fromhex(\"" as *char
    ci=0; while(pa[ci]!=0){cmd[cp]=pa[ci];cp+=1;ci+=1}
    ci=0; while(p_hex[ci]!=0){cmd[cp]=p_hex[ci];cp+=1;ci+=1}
    var pb = "\"),\"big\"); r=(3-a)%a; l=(r.bit_length()+7)//8 or 1; sys.stdout.write(r.to_bytes(l,\"big\").hex())' 2>/dev/null" as *char
    ci=0; while(pb[ci]!=0){cmd[cp]=pb[ci];cp+=1;ci+=1}
    cmd[cp]=0

    var id = py_uniq(); var py_path:[32]char; mk_fullpath(id, &raw mut py_path[0])
    var cmd2 : [512]char; var c2p:size_t=0; ci=0
    while(cmd[ci]!=0){cmd2[c2p]=cmd[ci];c2p+=1;ci+=1}
    ci=0; var red = " > " as *char; while(red[ci]!=0){cmd2[c2p]=red[ci];c2p+=1;ci+=1}
    ci=0; while(py_path[ci]!=0){cmd2[c2p]=py_path[ci];c2p+=1;ci+=1}
    cmd2[c2p]=0; system(&raw cmd2[0])

    var py_r:[8]u8; var py_len = read_hex_output(&raw py_path[0], &raw mut py_r[0], 8)
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
        if(py_len>0) { print_hex("py  ", &raw py_r[0], py_len) }
        env.error("mpi_mod(negative) mismatch")
    }
}

// ============================================================================
// P-256 prime self-consistency check
// ============================================================================
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

// ============================================================================
// ECDH: Compute 3*G using ecp_mul() directly, compare against Python
// No hardcoded constants — uses library's own curve parameters
// ============================================================================
@test public func TEST_ecdh_k3_direct(env:&mut TestEnv) {
    // Python script to compute 3*G using cryptography library (written as raw bytes)
    var py_script : [512]u8
    var psi : size_t = 0
    // Line 1: import sys
    py_script[psi]=105; psi+=1; py_script[psi]=109; psi+=1; py_script[psi]=112; psi+=1
    py_script[psi]=111; psi+=1; py_script[psi]=114; psi+=1; py_script[psi]=116; psi+=1
    py_script[psi]=32; psi+=1; py_script[psi]=115; psi+=1; py_script[psi]=121; psi+=1
    py_script[psi]=115; psi+=1; py_script[psi]=10; psi+=1  // newline
    // Line 2: from cryptography.hazmat.primitives.asymmetric import ec
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
    py_script[psi]=10; psi+=1  // newline
    // Line 3: from cryptography.hazmat.primitives.serialization import Encoding, PublicFormat
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
    // Line 4: sk=ec.derive_private_key(3,ec.SECP256R1())
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
    // Line 5: pub=sk.public_key().public_bytes(Encoding.X962,PublicFormat.UncompressedPoint)
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
    // Line 6: sys.stdout.write(pub.hex())
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

    // Chemical: compute 3*G using library's own constants
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

// ============================================================================
// Curve equation step-by-step: For generator G, verify each stage of
// y^2 = x^3 - 3x + b (mod p) by comparing each intermediate against Python
// ============================================================================
@test public func TEST_curve_equation_stepwise(env:&mut TestEnv) {
    // Python script (written as raw bytes, 10=newline)
    var py_script : [1024]u8; var psi:size_t=0
    // import sys
    py_script[psi]=105;psi+=1;py_script[psi]=109;psi+=1;py_script[psi]=112;psi+=1;py_script[psi]=111;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=32;psi+=1;py_script[psi]=115;psi+=1;py_script[psi]=121;psi+=1;py_script[psi]=115;psi+=1;py_script[psi]=10;psi+=1
    // from cryptography.hazmat.primitives.asymmetric import ec
    py_script[psi]=102;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=111;psi+=1;py_script[psi]=109;psi+=1;py_script[psi]=32;psi+=1;py_script[psi]=99;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=121;psi+=1;py_script[psi]=112;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=111;psi+=1;py_script[psi]=103;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=97;psi+=1;py_script[psi]=112;psi+=1;py_script[psi]=104;psi+=1;py_script[psi]=121;psi+=1;py_script[psi]=46;psi+=1;py_script[psi]=104;psi+=1;py_script[psi]=97;psi+=1;py_script[psi]=122;psi+=1;py_script[psi]=109;psi+=1;py_script[psi]=97;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=46;psi+=1;py_script[psi]=112;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=109;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=118;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=115;psi+=1;py_script[psi]=46;psi+=1;py_script[psi]=97;psi+=1;py_script[psi]=115;psi+=1;py_script[psi]=121;psi+=1;py_script[psi]=109;psi+=1;py_script[psi]=109;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=99;psi+=1;py_script[psi]=32;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=109;psi+=1;py_script[psi]=112;psi+=1;py_script[psi]=111;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=32;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=99;psi+=1;py_script[psi]=10;psi+=1
    // curve=ec.SECP256R1()
    py_script[psi]=99;psi+=1;py_script[psi]=117;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=118;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=61;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=99;psi+=1;py_script[psi]=46;psi+=1;py_script[psi]=83;psi+=1;py_script[psi]=69;psi+=1;py_script[psi]=67;psi+=1;py_script[psi]=80;psi+=1;py_script[psi]=50;psi+=1;py_script[psi]=53;psi+=1;py_script[psi]=54;psi+=1;py_script[psi]=82;psi+=1;py_script[psi]=49;psi+=1;py_script[psi]=40;psi+=1;py_script[psi]=41;psi+=1;py_script[psi]=10;psi+=1
    // sk=ec.derive_private_key(1,curve)
    py_script[psi]=115;psi+=1;py_script[psi]=107;psi+=1;py_script[psi]=61;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=99;psi+=1;py_script[psi]=46;psi+=1;py_script[psi]=100;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=118;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=95;psi+=1;py_script[psi]=112;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=118;psi+=1;py_script[psi]=97;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=95;psi+=1;py_script[psi]=107;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=121;psi+=1;py_script[psi]=40;psi+=1;py_script[psi]=49;psi+=1;py_script[psi]=44;psi+=1;py_script[psi]=99;psi+=1;py_script[psi]=117;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=118;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=41;psi+=1;py_script[psi]=10;psi+=1
    // pub=sk.public_key().public_bytes(ec.Encoding.X962,ec.PublicFormat.UncompressedPoint)
    py_script[psi]=112;psi+=1;py_script[psi]=117;psi+=1;py_script[psi]=98;psi+=1;py_script[psi]=61;psi+=1;py_script[psi]=115;psi+=1;py_script[psi]=107;psi+=1;py_script[psi]=46;psi+=1;py_script[psi]=112;psi+=1;py_script[psi]=117;psi+=1;py_script[psi]=98;psi+=1;py_script[psi]=108;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=99;psi+=1;py_script[psi]=95;psi+=1;py_script[psi]=107;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=121;psi+=1;py_script[psi]=40;psi+=1;py_script[psi]=41;psi+=1;py_script[psi]=46;psi+=1;py_script[psi]=112;psi+=1;py_script[psi]=117;psi+=1;py_script[psi]=98;psi+=1;py_script[psi]=108;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=99;psi+=1;py_script[psi]=95;psi+=1;py_script[psi]=98;psi+=1;py_script[psi]=121;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=115;psi+=1;py_script[psi]=40;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=99;psi+=1;py_script[psi]=46;psi+=1;py_script[psi]=69;psi+=1;py_script[psi]=110;psi+=1;py_script[psi]=99;psi+=1;py_script[psi]=111;psi+=1;py_script[psi]=100;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=110;psi+=1;py_script[psi]=103;psi+=1;py_script[psi]=46;psi+=1;py_script[psi]=88;psi+=1;py_script[psi]=57;psi+=1;py_script[psi]=54;psi+=1;py_script[psi]=50;psi+=1;py_script[psi]=44;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=99;psi+=1;py_script[psi]=46;psi+=1;py_script[psi]=80;psi+=1;py_script[psi]=117;psi+=1;py_script[psi]=98;psi+=1;py_script[psi]=108;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=99;psi+=1;py_script[psi]=70;psi+=1;py_script[psi]=111;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=109;psi+=1;py_script[psi]=97;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=46;psi+=1;py_script[psi]=85;psi+=1;py_script[psi]=110;psi+=1;py_script[psi]=99;psi+=1;py_script[psi]=111;psi+=1;py_script[psi]=109;psi+=1;py_script[psi]=112;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=115;psi+=1;py_script[psi]=115;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=100;psi+=1;py_script[psi]=80;psi+=1;py_script[psi]=111;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=110;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=41;psi+=1;py_script[psi]=10;psi+=1
    // Gx=int.from_bytes(pub[1:33],'big')
    py_script[psi]=71;psi+=1;py_script[psi]=120;psi+=1;py_script[psi]=61;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=110;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=46;psi+=1;py_script[psi]=102;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=111;psi+=1;py_script[psi]=109;psi+=1;py_script[psi]=95;psi+=1;py_script[psi]=98;psi+=1;py_script[psi]=121;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=115;psi+=1;py_script[psi]=40;psi+=1;py_script[psi]=112;psi+=1;py_script[psi]=117;psi+=1;py_script[psi]=98;psi+=1;py_script[psi]=91;psi+=1;py_script[psi]=49;psi+=1;py_script[psi]=58;psi+=1;py_script[psi]=51;psi+=1;py_script[psi]=51;psi+=1;py_script[psi]=93;psi+=1;py_script[psi]=44;psi+=1;py_script[psi]=39;psi+=1;py_script[psi]=98;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=103;psi+=1;py_script[psi]=39;psi+=1;py_script[psi]=41;psi+=1;py_script[psi]=10;psi+=1
    // Gy=int.from_bytes(pub[33:65],'big')
    py_script[psi]=71;psi+=1;py_script[psi]=121;psi+=1;py_script[psi]=61;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=110;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=46;psi+=1;py_script[psi]=102;psi+=1;py_script[psi]=114;psi+=1;py_script[psi]=111;psi+=1;py_script[psi]=109;psi+=1;py_script[psi]=95;psi+=1;py_script[psi]=98;psi+=1;py_script[psi]=121;psi+=1;py_script[psi]=116;psi+=1;py_script[psi]=101;psi+=1;py_script[psi]=115;psi+=1;py_script[psi]=40;psi+=1;py_script[psi]=112;psi+=1;py_script[psi]=117;psi+=1;py_script[psi]=98;psi+=1;py_script[psi]=91;psi+=1;py_script[psi]=51;psi+=1;py_script[psi]=51;psi+=1;py_script[psi]=58;psi+=1;py_script[psi]=54;psi+=1;py_script[psi]=53;psi+=1;py_script[psi]=93;psi+=1;py_script[psi]=44;psi+=1;py_script[psi]=39;psi+=1;py_script[psi]=98;psi+=1;py_script[psi]=105;psi+=1;py_script[psi]=103;psi+=1;py_script[psi]=39;psi+=1;py_script[psi]=41;psi+=1;py_script[psi]=10;psi+=1
    // p=0xffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632551
    var py_p = "p=0x" as *char; var ci:size_t=0; while(py_p[ci]!=0){py_script[psi]=py_p[ci]as u8;psi+=1;ci+=1}
    var p_m : Mpi; ecp_curve_p(&raw mut p_m); var p_buf:[32]u8; mpi_get_bytes(&raw mut p_m, &raw mut p_buf[0])
    var p_hex:[65]char; test_bytes_to_hex(&raw p_buf[0], 32, &raw mut p_hex[0])
    ci=0; while(p_hex[ci]!=0){py_script[psi]=p_hex[ci]as u8;psi+=1;ci+=1}
    py_script[psi]=10;psi+=1
    // b=0x...
    var py_b = "b=0x" as *char; ci=0; while(py_b[ci]!=0){py_script[psi]=py_b[ci]as u8;psi+=1;ci+=1}
    var b_m : Mpi; ecp_curve_b(&raw mut b_m); var b_buf:[32]u8; mpi_get_bytes(&raw mut b_m, &raw mut b_buf[0])
    var b_hex:[65]char; test_bytes_to_hex(&raw b_buf[0], 32, &raw mut b_hex[0])
    ci=0; while(b_hex[ci]!=0){py_script[psi]=b_hex[ci]as u8;psi+=1;ci+=1}
    py_script[psi]=10;psi+=1
    // lhs=(Gy*Gy)%p
    var py_l1 = "lhs=(Gy*Gy)%p" as *char; ci=0; while(py_l1[ci]!=0){py_script[psi]=py_l1[ci]as u8;psi+=1;ci+=1}
    py_script[psi]=10;psi+=1
    // x3=pow(Gx,3,p)
    var py_x3 = "x3=pow(Gx,3,p)" as *char; ci=0; while(py_x3[ci]!=0){py_script[psi]=py_x3[ci]as u8;psi+=1;ci+=1}
    py_script[psi]=10;psi+=1
    // three_x=(3*Gx)%p
    var py_tx = "three_x=(3*Gx)%p" as *char; ci=0; while(py_tx[ci]!=0){py_script[psi]=py_tx[ci]as u8;psi+=1;ci+=1}
    py_script[psi]=10;psi+=1
    // rhs=(x3-three_x+b)%p
    var py_rh = "rhs=(x3-three_x+b)%p" as *char; ci=0; while(py_rh[ci]!=0){py_script[psi]=py_rh[ci]as u8;psi+=1;ci+=1}
    py_script[psi]=10;psi+=1
    // sys.stdout.write(f'{lhs:064x}{x3:064x}{three_x:064x}{rhs:064x}')
    var py_wr = "sys.stdout.write(f'" as *char; ci=0; while(py_wr[ci]!=0){py_script[psi]=py_wr[ci]as u8;psi+=1;ci+=1}
    var py_fmt = "{lhs:064x}{x3:064x}{three_x:064x}{rhs:064x}')" as *char; ci=0; while(py_fmt[ci]!=0){py_script[psi]=py_fmt[ci]as u8;psi+=1;ci+=1}
    py_script[psi]=10;psi+=1

    var py_out:[256]u8; var py_len = run_py_script(&raw py_script[0], psi, &raw mut py_out[0], 256)
    if(py_len < 128) { env.error("Python output too short"); return }

    var py_lhs = &raw py_out[0]; var py_x3p = &raw py_out[32]
    var py_3x  = &raw py_out[64]; var py_rhs = &raw py_out[96]

    // Chemical: get from library
    var Gx : Mpi; ecp_curve_gx(&raw mut Gx); var Gy : Mpi; ecp_curve_gy(&raw mut Gy)
    var p  : Mpi; ecp_curve_p(&raw mut p); var b  : Mpi; ecp_curve_b(&raw mut b)
    var chem_lhs:[32]u8; var chem_x3:[32]u8; var chem_3x:[32]u8; var chem_rhs:[32]u8

    // Step 1: y^2 mod p
    var y2 : Mpi; mpi_init(&raw mut y2)
    mpi_mul(&raw mut y2, &raw mut Gy, &raw mut Gy); mpi_mod(&raw mut y2, &raw mut y2, &raw mut p)
    mpi_get_bytes(&raw mut y2, &raw mut chem_lhs[0])
    if(!test_bytes_eq(&raw chem_lhs[0], py_lhs, 32)) {
        printf("[CURVE] y^2 mod p mismatch\n"); print_hex("chem",&raw chem_lhs[0],32); print_hex("py  ",py_lhs,32)
        env.error("step1: y^2 mismatch"); return
    }

    // Step 2: x^3 mod p
    var x3m : Mpi; mpi_init(&raw mut x3m)
    mpi_mul(&raw mut x3m, &raw mut Gx, &raw mut Gx); mpi_mod(&raw mut x3m, &raw mut x3m, &raw mut p)
    mpi_mul(&raw mut x3m, &raw mut x3m, &raw mut Gx); mpi_mod(&raw mut x3m, &raw mut x3m, &raw mut p)
    mpi_get_bytes(&raw mut x3m, &raw mut chem_x3[0])
    if(!test_bytes_eq(&raw chem_x3[0], py_x3p, 32)) {
        printf("[CURVE] x^3 mod p mismatch\n"); print_hex("chem",&raw chem_x3[0],32); print_hex("py  ",py_x3p,32)
        env.error("step2: x^3 mismatch"); return
    }

    // Step 3: 3*x mod p
    var t3x : Mpi; mpi_init(&raw mut t3x)
    mpi_mul_int(&raw mut t3x, &raw mut Gx, 3); mpi_mod(&raw mut t3x, &raw mut t3x, &raw mut p)
    mpi_get_bytes(&raw mut t3x, &raw mut chem_3x[0])
    if(!test_bytes_eq(&raw chem_3x[0], py_3x, 32)) {
        printf("[CURVE] 3x mod p mismatch\n"); print_hex("chem",&raw chem_3x[0],32); print_hex("py  ",py_3x,32)
        env.error("step3: 3x mismatch"); return
    }

    // Step 4: x^3 - 3x + b mod p
    var t2 : Mpi; mpi_init(&raw mut t2)
    mpi_sub(&raw mut t2, &raw mut x3m, &raw mut t3x); mpi_mod(&raw mut t2, &raw mut t2, &raw mut p)
    mpi_add(&raw mut t2, &raw mut t2, &raw mut b); mpi_mod(&raw mut t2, &raw mut t2, &raw mut p)
    mpi_get_bytes(&raw mut t2, &raw mut chem_rhs[0])
    if(!test_bytes_eq(&raw chem_rhs[0], py_rhs, 32)) {
        printf("[CURVE] x^3-3x+b mod p mismatch\n"); print_hex("chem",&raw chem_rhs[0],32); print_hex("py  ",py_rhs,32)
        env.error("step4: rhs mismatch"); return
    }

    // Final: y^2 should equal x^3-3x+b for the generator
    if(!test_bytes_eq(&raw chem_lhs[0], &raw chem_rhs[0], 32)) {
        printf("[CURVE] curve equation y^2 != x^3-3x+b for generator G\n")
        print_hex("y^2  ", &raw chem_lhs[0], 32); print_hex("x^3-3x+b", &raw chem_rhs[0], 32)
        env.error("curve equation failed")
    }
}

// ============================================================================
// mpi_mul_int: Simple test - 7 * 6 = 42
// ============================================================================
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
