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

// Phase 3 effects: `[no_alloc]`, `[no_unsafe]`, `[no_blocking]`, `[pure]` and
// the transitive callee rule. These are all plain arithmetic so the checker can
// prove every restriction.
func no_alloc_sum(a : int, b : int) : int [no_alloc] {
    return a + b
}

func no_alloc_sum_caller(a : int, b : int) : int [no_alloc] {
    return no_alloc_sum(a, b)
}

func no_unsafe_ge(a : int, b : int) : bool [no_unsafe] {
    return a >= b
}

func no_blocking_id(v : int) : int [no_blocking] {
    return v
}

func pure_mul(a : int, b : int) : int [pure] {
    return a * b
}

func <T> generic_ident(v : T) : T [no_alloc] {
    return v
}

func generic_caller(v : int) : int [no_alloc] {
    return generic_ident<int>(v)
}

@test
func test_no_panic_sample(env : &mut TestEnv) {
    var a = no_panic_add(1u32, 2u32)
    if(a is Option.None) { env.error("no_panic_add in range") }
    var b = no_panic_add(4294967295u32, 1u32)
    if(b is Option.Some) { env.error("no_panic_add overflow") }
    if(no_panic_bounds(0u, 0u)) { env.error("no_panic_bounds empty") }
}

@test
func test_function_effects_sample(env : &mut TestEnv) {
    if(no_alloc_sum_caller(20, 22) != 42) { env.error("no_alloc_sum_caller") }
    if(!no_unsafe_ge(2, 1)) { env.error("no_unsafe_ge") }
    if(no_blocking_id(7) != 7) { env.error("no_blocking_id") }
    if(pure_mul(6, 7) != 42) { env.error("pure_mul") }
    if(generic_caller(42) != 42) { env.error("generic effect propagation") }
}
