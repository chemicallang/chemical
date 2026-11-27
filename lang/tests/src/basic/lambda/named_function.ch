// Regression: a named function converted to `std::function` / `std::typed_function`
// / `std::Fn` must be wrapped in the non-capturing callable instance (a
// capturing-ABI thunk with a null environment), not cast to the struct.
//
// P0.1a: this used to fail on TCC (invalid C conversion) and pass garbage on
// LLVM (`fn(void* data, ...)` called a `void(int,int)` symbol).

using std::Result;

var named_fn_side : int = 0

func named_fn_add(a : int, b : int) : int {
    return a + b
}

func named_fn_zero(a : int, b : int) : int {
    return 0
}

func named_fn_side_effect(a : int, b : int) {
    unsafe { named_fn_side = named_fn_side + a + b }
}

func named_fn_seven() : int {
    return 7
}

func named_fn_make() : std::function<(a : int, b : int) => int> {
    return named_fn_add
}

struct NamedFnPair {
    var a : int
    var b : int
}

struct NamedFnHolder {
    var f : std::function<(a : int, b : int) => int>
}

func named_fn_pair(a : int, b : int) : NamedFnPair {
    return NamedFnPair { a : a, b : b }
}

func <T> named_fn_generic_identity(v : T) : T {
    return v
}

@test
public func named_function_to_std_function_return(env : &mut TestEnv) {
    var f : std::function<(a : int, b : int) => int> = named_fn_add
    if(f(20, 22) != 42) {
        env.error("named function through std::function returned the wrong value")
    }
}

@test
public func named_function_to_std_function_void(env : &mut TestEnv) {
    unsafe { named_fn_side = 0 }
    var f : std::function<(a : int, b : int) => void> = named_fn_side_effect
    f(1, 2)
    unsafe {
        if(named_fn_side != 3) {
            env.error("named void function through std::function was not called")
        }
    }
}

@test
public func named_function_zero_parameters(env : &mut TestEnv) {
    var f : std::function<() => int> = named_fn_seven
    if(f() != 7) {
        env.error("named zero-parameter function through std::function failed")
    }
}

@test
public func named_function_assignment_and_return(env : &mut TestEnv) {
    var f : std::function<(a : int, b : int) => int> = named_fn_zero
    f = named_fn_add
    if(f(1, 2) != 3) {
        env.error("assigning a named function to std::function failed")
    }
    var made = named_fn_make()
    if(made(3, 4) != 7) {
        env.error("returning a named function as std::function failed")
    }
}

@test
public func named_function_typed_and_fn_aliases(env : &mut TestEnv) {
    var tf : std::typed_function<(a : int, b : int) => int, std::SendStaticCapture> = named_fn_add
    if(tf(1, 2) != 3) {
        env.error("named function through std::typed_function failed")
    }
    var ff : std::Fn<(a : int, b : int) => int> = named_fn_add
    if(ff(3, 4) != 7) {
        env.error("named function through std::Fn failed")
    }
}

@test
public func named_function_aggregate_return(env : &mut TestEnv) {
    var f : std::function<(a : int, b : int) => NamedFnPair> = named_fn_pair
    var out = f(3, 4)
    if(out.a != 3 || out.b != 4) {
        env.error("named function with aggregate return through std::function failed")
    }
}

@test
public func named_function_generic_instantiation(env : &mut TestEnv) {
    var f : std::function<(v : int) => int> = named_fn_generic_identity<int>
    if(f(42) != 42) {
        env.error("generic named function through std::function failed")
    }
}

@test
public func named_function_struct_field(env : &mut TestEnv) {
    var holder = NamedFnHolder { f : named_fn_add }
    if(holder.f(20, 22) != 42) {
        env.error("named function stored in a std::function field failed")
    }
}

@test
public func capturing_lambda_path_still_works(env : &mut TestEnv) {
    var base = 10
    var add_base : std::function<(x : int) => int> = |base|(x : int) : int => {
        return x + base
    }
    if(add_base(5) != 15) {
        env.error("capturing lambda regression: by-value capture failed")
    }
    var twice : std::function<(x : int) => int> = ||(x : int) : int => {
        return x * 2
    }
    if(twice(21) != 42) {
        env.error("non-capturing lambda regression failed")
    }
}
