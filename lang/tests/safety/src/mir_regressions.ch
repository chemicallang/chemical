using std::Option;

// P3.3/P3.4 regression corpus for previously documented compiler crashes.
// These shapes must compile and run on both backends.

public variant VariantSelf {
    Has(v : int)
    Empty()

    // A by-value `self` receiver on a variant used to segfault the compiler.
    func by_value(self) : int {
        switch(self) {
            Has(v) => { return v }
            Empty() => { return 1 }
        }
    }
}

public variant GenericVariantSelf<T> {
    Has(v : T)
    Empty()

    func has_value(self) : bool {
        switch(self) {
            Has(v) => { return true }
            Empty() => { return false }
        }
    }
}

struct GenericVariantParam<T> {
    var value : T

    // A parameter whose type is a generic variant instantiation on a generic
    // struct method used to lower to `void*` in the C backend.
    func set_option(&mut self, v : Option<T>) {
        if(v is Option.Some) {
            var Some(x) = v else unsafe { unreachable }
            self.value = x
        }
    }
}

@test
func variant_by_value_self_runs(env : &mut TestEnv) {
    var q = VariantSelf.Has(42)
    if(q.by_value() != 42) { env.error("variant by-value self") }
    var e = VariantSelf.Empty()
    if(e.by_value() != 1) { env.error("variant by-value self empty") }

    var g = GenericVariantSelf.Has<int>(7)
    if(!g.has_value()) { env.error("generic variant by-value self") }
}

@test
func generic_variant_parameter_runs(env : &mut TestEnv) {
    var w = GenericVariantParam<int>{ value: 0 }
    w.set_option(Option.Some<int>(42))
    if(w.value != 42) { env.error("generic variant parameter") }
    var e = GenericVariantParam<int>{ value: 9 }
    e.set_option(Option.None<int>())
    if(e.value != 9) { env.error("generic variant parameter empty") }
}
