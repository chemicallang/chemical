// P0.1a regression proof: a named function assigned to `std::function`.
//
// Before P0.1a:
//   TCCCompiler: C translation error "cannot convert 'static void (int, int)'
//                to 'struct std_stddefault_function_instance'"
//   Compiler:    built, but `f(1, 2)` passed garbage and printed g=16777217
//
// After P0.1a both backends wrap the named function in a non-capturing
// callable instance (a capturing-ABI thunk + `make2`) and this probe prints
// `g=3` / `named_fn PASS`. Keep it as a proof; the regression tests live in
// `lang/tests/src/basic/lambda/named_function.ch`.

var g : int = 0

func inc(a : int, b : int) {
    unsafe { g = g + a + b }
}

public func main() : int {
    var f : std::function<(a : int, b : int) => void> = inc
    f(1, 2)
    unsafe {
        printf("g=%d\n", g)
        if(g == 3) { println("named_fn PASS"); return 0 }
    }
    println("named_fn FAIL")
    return 1
}
