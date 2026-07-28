using namespace tls
using namespace crypto

// Simple test: mpi_mul with 8-limb numbers, compare against Python via system()
@test public func TEST_mpi_mul_256(env:&mut TestEnv) {
    // a = 7, b = 6 (simple test first, then large)
    var a:[32]u8; a[31]=7
    var b:[32]u8; b[31]=6
    // expected: 7*6=42
    var ma:Mpi; mpi_read_binary(&raw mut ma, &raw a[0], 32)
    var mb:Mpi; mpi_read_binary(&raw mut mb, &raw b[0], 32)
    var mr:Mpi; mpi_init(&raw mut mr)
    mpi_mul(&raw mut mr, &raw mut ma, &raw mut mb)
    var cs=mpi_size(&raw mut mr)
    var cb:[8]u8; mpi_write_binary(&raw mut mr, &raw mut cb[0], 8)
    // The result should be 42 = 0x2A at the last byte
    if(cs!=1 || cb[7]!=42){
        printf("[MUL_SIMPLE] cs=%d cb[7]=%d\n", cs as int, cb[7] as int)
        env.error("mpi_mul 7*6 failed")
    }else{}
}

@test public func TEST_mpi_mul_8limb(env:&mut TestEnv) {
    // Use P-256 prime as a and a small number as b
    var p:[32]u8 = [0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
                    0xBC,0xE6,0xFA,0xAD,0xA7,0x17,0x9E,0x84,0xF3,0xB9,0xCA,0xC2,0xFC,0x63,0x25,0x51]
    var b:[1]u8; b[0]=2
    var ph:[65]char; test_bytes_to_hex(&raw p[0], 32, &raw mut ph[0])
    var id=py_uniq(); var outfile:[32]char
    outfile[0]=47; outfile[1]=116; outfile[2]=109; outfile[3]=112; outfile[4]=47; outfile[5]=112; outfile[6]=121
    var ofi:size_t=7; var tid=id
    if(tid==0){outfile[ofi]=48; ofi+=1}else{}
    var rev:[10]char; var ri:size_t=0
    while(tid>0){rev[ri]=48 as char+((tid%10)as char); tid=tid/10; ri+=1}
    while(ri>0){ri-=1; outfile[ofi]=rev[ri]; ofi+=1}
    outfile[ofi]=46; outfile[ofi+1]=111; outfile[ofi+2]=117; outfile[ofi+3]=116; outfile[ofi+4]=0

    var cmd:[512]char; var cp:size_t=0
    var c0="python3 -c 'a=int.from_bytes(bytes.fromhex(\"\0" as *char; var si:size_t=0
    while(c0[si]!=0){cmd[cp]=c0[si]; cp+=1; si+=1}
    si=0; while(ph[si]!=0){cmd[cp]=ph[si]; cp+=1; si+=1}
    var c1="\"),\"big\");r=a*2;l=(r.bit_length()+7)//8 or 1;print(r.to_bytes(l,\"big\").hex())' > \0" as *char; si=0
    while(c1[si]!=0){cmd[cp]=c1[si]; cp+=1; si+=1}
    si=0; while(outfile[si]!=0){cmd[cp]=outfile[si]; cp+=1; si+=1}
    cmd[cp]=32; cp+=1; var c2="2>/dev/null\0" as *char; si=0
    while(c2[si]!=0){cmd[cp]=c2[si]; cp+=1; si+=1}
    cmd[cp]=0
    system(&raw cmd[0])

    var outbuf:[256]u8
    var n = test_read_file(&raw outfile[0], &raw mut outbuf[0] as *mut u8, 256)

    // Chemical: p * 2
    var ma2:Mpi; mpi_read_binary(&raw mut ma2, &raw p[0], 32)
    var mb2:Mpi; mpi_read_binary(&raw mut mb2, &raw b[0], 1)
    var mr2:Mpi; mpi_init(&raw mut mr2)
    mpi_mul(&raw mut mr2, &raw mut ma2, &raw mut mb2)
    var cs2=mpi_size(&raw mut mr2); var cb2:[64]u8; mpi_write_binary(&raw mut mr2, &raw mut cb2[0], 64)

    // Parse python output (hex string without label prefix)
    var py_len:size_t=0; var py_r:[64]u8
    while(py_len*2 < n && outbuf[py_len*2]!=10 && outbuf[py_len*2]!=0){
        py_r[py_len]=test_hex_pair_byte(outbuf[py_len*2] as char, outbuf[py_len*2+1] as char); py_len+=1}

    if(cs2!=py_len||!test_bytes_eq(&raw cb2[64-cs2],&raw py_r[0],cs2)){
        printf("[MUL8L] cs=%d py_len=%d cb[%d]=%02x py[0]=%02x\n",cs2 as int,py_len as int,(64-cs2)as int,cb2[64-cs2] as int,py_r[0] as int)
        env.error("mpi_mul 8-limb mismatch")
    }else{}
}

var _py_id : i32 = 0
func py_uniq() : i32 { _py_id = _py_id + 1; return _py_id }

@test public func TEST_mpi_mod_8limb(env:&mut TestEnv) {
    // (p + 5) mod 3 vs Python
    var p:[32]u8 = [0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
                    0xBC,0xE6,0xFA,0xAD,0xA7,0x17,0x9E,0x84,0xF3,0xB9,0xCA,0xC2,0xFC,0x63,0x25,0x51]
    var ph:[65]char; test_bytes_to_hex(&raw p[0], 32, &raw mut ph[0])
    var m:[1]u8; m[0]=7
    var id=py_uniq()
    var outfile:[32]char; outfile[0]=47; outfile[1]=116; outfile[2]=109; outfile[3]=112; outfile[4]=47; outfile[5]=112; outfile[6]=121
    var ofi:size_t=7
    var tid=id; if(tid==0){outfile[ofi]=48; ofi+=1}else{}
    var rev:[10]char; var ri:size_t=0
    while(tid>0){rev[ri]=48 as char+((tid%10)as char); tid=tid/10; ri+=1}
    while(ri>0){ri-=1; outfile[ofi]=rev[ri]; ofi+=1}
    outfile[ofi]=46; outfile[ofi+1]=111; outfile[ofi+2]=117; outfile[ofi+3]=116; outfile[ofi+4]=0

    var cmd:[512]char; var cp:size_t=0
    var c0="python3 -c 'a=int.from_bytes(bytes.fromhex(\"\0" as *char; var si:size_t=0
    while(c0[si]!=0){cmd[cp]=c0[si]; cp+=1; si+=1}
    si=0; while(ph[si]!=0){cmd[cp]=ph[si]; cp+=1; si+=1}
    var c1="\"),\"big\");m=7;r=(a+5)%m;l=(r.bit_length()+7)//8 or 1;print(r.to_bytes(l,\"big\").hex())' > \0" as *char; si=0
    while(c1[si]!=0){cmd[cp]=c1[si]; cp+=1; si+=1}
    si=0; while(outfile[si]!=0){cmd[cp]=outfile[si]; cp+=1; si+=1}
    cmd[cp]=32; cp+=1; var c2="2>/dev/null\0" as *char; si=0
    while(c2[si]!=0){cmd[cp]=c2[si]; cp+=1; si+=1}
    cmd[cp]=0
    system(&raw cmd[0])
    var outbuf:[256]u8
    var n = test_read_file(&raw outfile[0], &raw mut outbuf[0] as *mut u8, 256)
    var py_r:[4]u8; var py_len:size_t=0
    while(py_len*2<n&&outbuf[py_len*2]!=10&&outbuf[py_len*2]!=0){
        py_r[py_len]=test_hex_pair_byte(outbuf[py_len*2]as char,outbuf[py_len*2+1]as char); py_len+=1}

    // Chemical: (p + 5) mod 3
    var five:[1]u8;five[0]=5
    var ma:Mpi;mpi_read_binary(&raw mut ma,&raw p[0],32)
    var mf:Mpi;mpi_read_binary(&raw mut mf,&raw five[0],1)
    var tmp:Mpi;mpi_init(&raw mut tmp);mpi_add(&raw mut tmp,&raw mut ma,&raw mut mf)
    var mm:Mpi;mpi_read_binary(&raw mut mm,&raw m[0],1)
    var mr:Mpi;mpi_init(&raw mut mr);mpi_mod(&raw mut mr,&raw mut tmp,&raw mut mm)
    var cs=mpi_size(&raw mut mr);var cb:[4]u8;mpi_write_binary(&raw mut mr,&raw mut cb[0],4)
    if(cs!=py_len || !test_bytes_eq(&raw cb[4-cs],&raw py_r[0],cs)){
        env.error("mpi_mod mismatch")
    }
}

@test public func TEST_mpi_sub_8limb(env:&mut TestEnv) {
    // (p - 5) vs Python
    var p:[32]u8 = [0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
                    0xBC,0xE6,0xFA,0xAD,0xA7,0x17,0x9E,0x84,0xF3,0xB9,0xCA,0xC2,0xFC,0x63,0x25,0x51]
    var ph:[65]char; test_bytes_to_hex(&raw p[0], 32, &raw mut ph[0])
    var id=py_uniq(); var outfile:[32]char
    outfile[0]=47; outfile[1]=116; outfile[2]=109; outfile[3]=112; outfile[4]=47; outfile[5]=112; outfile[6]=121
    var ofi:size_t=7; var tid=id
    if(tid==0){outfile[ofi]=48; ofi+=1}else{}
    var rev:[10]char; var ri:size_t=0
    while(tid>0){rev[ri]=48 as char+((tid%10)as char); tid=tid/10; ri+=1}
    while(ri>0){ri-=1; outfile[ofi]=rev[ri]; ofi+=1}
    outfile[ofi]=46; outfile[ofi+1]=111; outfile[ofi+2]=117; outfile[ofi+3]=116; outfile[ofi+4]=0

    var cmd:[512]char; var cp:size_t=0
    var c0="python3 -c 'a=int.from_bytes(bytes.fromhex(\"\0" as *char; var si:size_t=0
    while(c0[si]!=0){cmd[cp]=c0[si]; cp+=1; si+=1}
    si=0; while(ph[si]!=0){cmd[cp]=ph[si]; cp+=1; si+=1}
    var c1="\"),\"big\");r=a-5;l=(r.bit_length()+7)//8 or 1;print(r.to_bytes(l,\"big\").hex())' > \0" as *char; si=0
    while(c1[si]!=0){cmd[cp]=c1[si]; cp+=1; si+=1}
    si=0; while(outfile[si]!=0){cmd[cp]=outfile[si]; cp+=1; si+=1}
    cmd[cp]=32; cp+=1; var c2="2>/dev/null\0" as *char; si=0
    while(c2[si]!=0){cmd[cp]=c2[si]; cp+=1; si+=1}
    cmd[cp]=0
    system(&raw cmd[0])
    var outbuf:[256]u8
    var n = test_read_file(&raw outfile[0], &raw mut outbuf[0] as *mut u8, 256)
    var py_r:[40]u8; var py_len:size_t=0
    while(py_len*2<n&&outbuf[py_len*2]!=10&&outbuf[py_len*2]!=0){
        py_r[py_len]=test_hex_pair_byte(outbuf[py_len*2]as char,outbuf[py_len*2+1]as char); py_len+=1}

    // Chemical: p - 5
    var five:[1]u8;five[0]=5
    var ma:Mpi;mpi_read_binary(&raw mut ma,&raw p[0],32)
    var mf:Mpi;mpi_read_binary(&raw mut mf,&raw five[0],1)
    var mr:Mpi;mpi_init(&raw mut mr);mpi_sub(&raw mut mr,&raw mut ma,&raw mut mf)
    var cs=mpi_size(&raw mut mr);var cb:[40]u8;mpi_write_binary(&raw mut mr,&raw mut cb[0],40)
    if(cs!=py_len||!test_bytes_eq(&raw cb[40-cs],&raw py_r[0],cs)){
        printf("[SUB8L] cs=%d py_len=%d cb=%d py=%d\n",cs as int,py_len as int,cb[40-cs]as int,py_r[0]as int)
        env.error("mpi_sub 8-limb mismatch")
    }else{}
}

@test public func TEST_mpi_mul_int_8limb(env:&mut TestEnv) {
    var a:[32]u8 = [0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
                    0xBC,0xE6,0xFA,0xAD,0xA7,0x17,0x9E,0x84,0xF3,0xB9,0xCA,0xC2,0xFC,0x63,0x25,0x51]
    var ah:[65]char; test_bytes_to_hex(&raw a[0], 32, &raw mut ah[0])
    var id=py_uniq(); var outfile:[32]char
    outfile[0]=47; outfile[1]=116; outfile[2]=109; outfile[3]=112; outfile[4]=47; outfile[5]=112; outfile[6]=121
    var ofi:size_t=7; var tid=id
    if(tid==0){outfile[ofi]=48; ofi+=1}else{}
    var rev:[10]char; var ri:size_t=0
    while(tid>0){rev[ri]=48 as char+((tid%10)as char); tid=tid/10; ri+=1}
    while(ri>0){ri-=1; outfile[ofi]=rev[ri]; ofi+=1}
    outfile[ofi]=46; outfile[ofi+1]=111; outfile[ofi+2]=117; outfile[ofi+3]=116; outfile[ofi+4]=0
    var cmd:[512]char; var cp:size_t=0
    var c0="python3 -c 'a=int.from_bytes(bytes.fromhex(\"\0" as *char; var si:size_t=0
    while(c0[si]!=0){cmd[cp]=c0[si]; cp+=1; si+=1}
    si=0; while(ah[si]!=0){cmd[cp]=ah[si]; cp+=1; si+=1}
    var c1="\"),\"big\");r=a*4;l=(r.bit_length()+7)//8 or 1;print(r.to_bytes(l,\"big\").hex())' > \0" as *char; si=0
    while(c1[si]!=0){cmd[cp]=c1[si]; cp+=1; si+=1}
    si=0; while(outfile[si]!=0){cmd[cp]=outfile[si]; cp+=1; si+=1}
    cmd[cp]=32; cp+=1; var c2="2>/dev/null\0" as *char; si=0
    while(c2[si]!=0){cmd[cp]=c2[si]; cp+=1; si+=1}
    cmd[cp]=0; system(&raw cmd[0])
    var outbuf:[256]u8
    var n=test_read_file(&raw outfile[0],&raw mut outbuf[0]as *mut u8,256)
    var py_r:[64]u8; var py_len:size_t=0
    while(py_len*2<n&&outbuf[py_len*2]!=10&&outbuf[py_len*2]!=0){py_r[py_len]=test_hex_pair_byte(outbuf[py_len*2]as char,outbuf[py_len*2+1]as char); py_len+=1}

    var ma:Mpi; mpi_read_binary(&raw mut ma,&raw a[0],32)
    var mr:Mpi; mpi_init(&raw mut mr)
    var ret = mpi_mul_int(&raw mut mr,&raw mut ma,4)
    var cs=mpi_size(&raw mut mr); var cb:[64]u8; mpi_write_binary(&raw mut mr,&raw mut cb[0],64)
    if(ret<0||cs!=py_len||!test_bytes_eq(&raw cb[64-cs],&raw py_r[0],cs)){
        env.error("mpi_mul_int mismatch")
    }else{}
}

@test public func TEST_mpi_sub_mod_8limb(env:&mut TestEnv) {
    // (3 - p) mod p = 3 (since 3 - p is negative, result = 3)
    var p:[32]u8 = [0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
                    0xBC,0xE6,0xFA,0xAD,0xA7,0x17,0x9E,0x84,0xF3,0xB9,0xCA,0xC2,0xFC,0x63,0x25,0x51]
    var three:[1]u8; three[0]=3
    var ma:Mpi; mpi_read_binary(&raw mut ma,&raw three[0],1)
    var mp:Mpi; mpi_read_binary(&raw mut mp,&raw p[0],32)
    var mr:Mpi; mpi_init(&raw mut mr)
    // Chemical: (3 - p) mod p
    var tmp:Mpi; mpi_init(&raw mut tmp)
    var ret=mpi_sub(&raw mut tmp,&raw mut ma,&raw mut mp)  // tmp = 3 - p (negative)
    if(ret<0){env.error("sub failed");return}else{}
    ret=mpi_mod(&raw mut mr,&raw mut tmp,&raw mut mp)  // mr = (3-p) mod p = 3
    if(ret<0){env.error("mod failed");return}else{}
    var cs=mpi_size(&raw mut mr); var cb:[4]u8; mpi_write_binary(&raw mut mr,&raw mut cb[0],4)
    if(cs!=1 || cb[3]!=3){
        printf("[SUBMOD] cs=%d cb[3]=%d\n",cs as int, cb[3] as int)
        env.error("mpi_sub+mod mismatch")
    }else{}
}

@test public func TEST_p256_prime(env:&mut TestEnv) {
    // Verify P-256 prime is correct: FFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551
    var p: Mpi; ecp_curve_p(&raw mut p)
    var expected:[32]u8 = [
        0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xBC,0xE6,0xFA,0xAD,0xA7,0x17,0x9E,0x84,0xF3,0xB9,0xCA,0xC2,0xFC,0x63,0x25,0x51
    ]
    var buf:[32]u8
    mpi_write_binary(&raw mut p, &raw mut buf[0], 32)
    if(!test_bytes_eq(&raw buf[0], &raw expected[0], 32)){
        env.error("P-256 prime mismatch")
    }
}

@test public func TEST_ecdh_k3_vs_py(env:&mut TestEnv) {
    var sc:[512]u8; var sp:size_t=0
    var h="#!/usr/bin/python3\nfrom cryptography.hazmat.primitives.asymmetric import ec\nfrom cryptography.hazmat.primitives.serialization import Encoding,PublicFormat\nsk=ec.derive_private_key(3,ec.SECP256R1())\npub=sk.public_key().public_bytes(Encoding.X962,PublicFormat.UncompressedPoint)\nprint('PUB='+pub.hex())\n\0" as *char; var si:size_t=0
    while(h[si]!=0){sc[sp]=h[si]as u8; sp+=1; si+=1}
    var id=py_uniq(); var outfile:[32]char
    outfile[0]=47; outfile[1]=116; outfile[2]=109; outfile[3]=112; outfile[4]=47; outfile[5]=112; outfile[6]=121
    var ofi:size_t=7; var tid=id
    if(tid==0){outfile[ofi]=48; ofi+=1}else{}
    var rev:[10]char; var ri:size_t=0
    while(tid>0){rev[ri]=48 as char+((tid%10)as char); tid=tid/10; ri+=1}
    while(ri>0){ri-=1; outfile[ofi]=rev[ri]; ofi+=1}
    outfile[ofi]=46; outfile[ofi+1]=111; outfile[ofi+2]=117; outfile[ofi+3]=116; outfile[ofi+4]=0
    test_write_file(&raw outfile[0], &raw sc[0], sp)
    var cmd:[128]char; var cp:size_t=0
    var c0="python3 \0" as *char; si=0
    while(c0[si]!=0){cmd[cp]=c0[si]; cp+=1; si+=1}
    si=0; while(outfile[si]!=0){cmd[cp]=outfile[si]; cp+=1; si+=1}
    cmd[cp]=32; cp+=1; cmd[cp]=62; cp+=1; cmd[cp]=32; cp+=1
    si=0; while(outfile[si]!=0){cmd[cp]=outfile[si]; cp+=1; si+=1}
    cmd[cp]=46; cp+=1; cmd[cp]=111; cp+=1; cmd[cp]=117; cp+=1; cmd[cp]=116; cp+=1
    cmd[cp]=32; cp+=1; var c1="2>/dev/null\0" as *char; si=0
    while(c1[si]!=0){cmd[cp]=c1[si]; cp+=1; si+=1}
    cmd[cp]=0
    system(&raw cmd[0])
    var opath:[32]char; var oi:size_t=0
    while(outfile[oi]!=0){opath[oi]=outfile[oi]; oi+=1}
    opath[oi-1]=111; opath[oi]=117; opath[oi+1]=116; opath[oi+2]=0
    var outbuf:[256]u8
    var n=test_read_file(&raw opath[0], &raw mut outbuf[0]as *mut u8, 256)
    var py_pub:[65]u8; var py_len:size_t=0
    while(py_len*2<n&&outbuf[py_len*2]!=10&&outbuf[py_len*2]!=0&&py_len<65){py_pub[py_len]=test_hex_pair_byte(outbuf[py_len*2]as char,outbuf[py_len*2+1]as char); py_len+=1}

    var k3:[32]u8; var ki:size_t=0; while(ki<31){k3[ki]=0; ki+=1}; k3[31]=3
    var G:[65]u8; G[0]=0x04
    var Gx=[0x6B,0x17,0xD1,0xF2,0xE1,0x2C,0x42,0x47,0xF8,0xBC,0xE6,0xE5,0x63,0xA4,0x40,0xF2,0x77,0x03,0x7D,0x81,0x2D,0xEB,0x33,0xA0,0xF4,0xA1,0x39,0x45,0xD8,0x98,0xC2,0x96]
    var Gy=[0x4F,0xE3,0x42,0xE2,0xFE,0x1A,0x7F,0x9B,0x8E,0xE7,0xEB,0x4A,0x7C,0x0F,0x9E,0x16,0x2B,0xCE,0x33,0x57,0x6B,0x31,0x5E,0xCE,0xCB,0xB6,0x40,0x68,0x37,0xBF,0x51,0xF5]
    var gi:size_t=0; while(gi<32){G[1+gi]=Gx[gi]as u8; gi+=1}
    gi=0; while(gi<32){G[33+gi]=Gy[gi]as u8; gi+=1}
    var ctx:ECDHContext; ecdh_init(&raw mut ctx)
    var ret=mpi_read_binary(&raw mut ctx.priv_key, &raw k3[0], 32)
    if(ret<0){env.error("read k3");return}else{}
    ctx.is_init=true
    var shared:[32]u8
    ret=ecdh_compute_shared(&raw mut ctx, &raw G[0], 65, &raw mut shared[0], 32)
    if(ret<0){env.error("cs k3");return}else{}
    if(!test_bytes_eq(&raw shared[0], &raw py_pub[1], 32)){env.error("k3 mismatch")}else{}
}
