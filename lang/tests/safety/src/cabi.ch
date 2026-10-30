// C ABI by-value aggregate tests (S2.5). The linked C file defines the real
// callees; the internal MIR ABI passes aggregates by pointer, so these calls
// exercise the emitter's by-value coercion/memory/sret paths on both backends.

@extern struct CAbiPair { var a : int; var b : int }
@extern struct CAbiBig { var a : int; var b : int; var c : int; var d : int; var e : int }
@extern struct CAbiFloats { var a : float; var b : float }
@extern struct CAbiMixed { var a : int; var b : float }
@extern struct CAbiThree { var a : int; var b : int; var c : int }
@extern union CAbiUnion { var i : int; var f : float }
@packed @extern struct CAbiPacked { var tag : u8; var value : int; var tail : u8 }
@repr("transparent") @extern struct CAbiTransparent { var value : int }

@extern func cabi_pair_add(p : CAbiPair) : CAbiPair
@extern func cabi_pair_sum(p : CAbiPair) : int
@extern func cabi_pair_sum_extra(p : CAbiPair, extra : int) : int
@extern func cabi_two_pairs(a : CAbiPair, b : CAbiPair) : int
@extern func cabi_big_add(x : CAbiBig) : CAbiBig
@extern func cabi_floats_add(x : CAbiFloats) : CAbiFloats
@extern func cabi_mixed_add(x : CAbiMixed) : CAbiMixed
@extern func cabi_three_add(x : CAbiThree) : CAbiThree
@extern func cabi_union_read(u : CAbiUnion) : int
@extern func cabi_packed_sum(p : CAbiPacked) : int
@extern func cabi_transparent_read(x : CAbiTransparent) : int

@test
public func CABI_by_value_aggregates(env : &mut TestEnv) {
    var p = CAbiPair { a : 1, b : 2 }
    var q = unsafe(cabi_pair_add(p))
    var s = unsafe(cabi_pair_sum(q))
    var e = unsafe(cabi_pair_sum_extra(p, 4))
    var two = unsafe(cabi_two_pairs(p, q))
    var big = CAbiBig { a : 1, b : 2, c : 3, d : 4, e : 5 }
    var b = unsafe(cabi_big_add(big))
    var fl = CAbiFloats { a : 1.5f, b : 2.5f }
    var fr = unsafe(cabi_floats_add(fl))
    var mx = CAbiMixed { a : 1, b : 2.5f }
    var mr = unsafe(cabi_mixed_add(mx))
    var th = CAbiThree { a : 1, b : 2, c : 3 }
    var tr = unsafe(cabi_three_add(th))
    var u = CAbiUnion { i : 7 }
    var ur = unsafe(cabi_union_read(u))
    var pk = CAbiPacked { tag : 3, value : 4, tail : 5 }
    var ps = unsafe(cabi_packed_sum(pk))
    var tv = CAbiTransparent { value : 9 }
    var tx = unsafe(cabi_transparent_read(tv))
    if(q.a != 2 || q.b != 4) { env.error("small struct return mismatch") }
    if(s != 6 || e != 7) { env.error("small struct parameter mismatch") }
    if(two != 5) { env.error("two small struct parameters mismatch") }
    if(b.a != 2 || b.b != 4 || b.c != 6 || b.d != 8 || b.e != 10) {
        env.error("memory-class struct mismatch")
    }
    if(fr.a != 2.5f || fr.b != 4.5f) { env.error("SSE struct mismatch") }
    if(mr.a != 2 || mr.b != 4.5f) { env.error("mixed struct mismatch") }
    if(tr.a != 2 || tr.b != 4 || tr.c != 6) { env.error("two-eightbyte struct mismatch") }
    if(ur != 7) { env.error("union by-value mismatch") }
    if(ps != 12) { env.error("packed struct C ABI mismatch") }
    if(tx != 9) { env.error("transparent struct C ABI mismatch") }
}
