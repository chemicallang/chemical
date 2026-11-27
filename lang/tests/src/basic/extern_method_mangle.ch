// P0.2b: `@extern` methods keep their owner prefix; top-level `@extern` stays
// bare. The C definitions in lang/tests/ext/file.c prove the symbols are
// distinct and reachable.
@extern public func ext_p02b_bare(x : int) : int

struct P02bAlpha {
    @extern public func text(&self, x : int) : int
}

struct P02bBeta {
    @extern public func text(&self, x : int) : int
}

struct P02bExplicit {
    @extern @no_mangle public func p02b_explicit_bare(&self, x : int) : int
}

@test
public func extern_method_names_are_owner_mangled(env : &mut TestEnv) {
    var a = P02bAlpha { }
    var b = P02bBeta { }
    var e = P02bExplicit { }
    var bare = 0
    var av = 0
    var bv = 0
    var ev = 0
    unsafe {
        bare = ext_p02b_bare(1)
        av = a.text(1)
        bv = b.text(1)
        ev = e.p02b_explicit_bare(1)
    }
    if(bare != 31) { env.error("top-level @extern did not stay bare") }
    if(av != 11) { env.error("first method did not reach its C function") }
    if(bv != 21) { env.error("second method did not reach its C function") }
    if(ev != 41) { env.error("@no_mangle on a method did not preserve the bare name") }
}
