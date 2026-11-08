using std::Option;

// Phase 2 no-panic sample: only explicitly annotated, non-panicking core
// operations. Full generic/async/interface effect propagation is P3.1.
func no_panic_add(a : u32, b : u32) : Option<u32> [no_panic] {
    var out : u64 = a as u64 + b as u64
    if(out > 4294967295 as u64) {
        return Option.None<u32>()
    }
    return Option.Some<u32>(out as u32)
}

func no_panic_bounds(len : size_t, i : size_t) : bool [no_panic] {
    return i < len
}

@test
func test_no_panic_sample(env : &mut TestEnv) {
    var a = no_panic_add(1u32, 2u32)
    if(a is Option.None) { env.error("no_panic_add in range") }
    var b = no_panic_add(4294967295u32, 1u32)
    if(b is Option.Some) { env.error("no_panic_add overflow") }
    if(no_panic_bounds(0u, 0u)) { env.error("no_panic_bounds empty") }
}
