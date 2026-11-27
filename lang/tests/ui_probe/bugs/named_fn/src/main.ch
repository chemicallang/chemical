// P0.1 reduced compiler bug: a named function assigned to `std::function`.
//
// Expected today:
//   TCCCompiler: C translation error "cannot convert 'static void (int, int)'
//                to 'struct std_stddefault_function_instance'"
//   Compiler:    builds, but `f(1, 2)` passes garbage and prints g=16777217
//
// Workaround for the UI library: construct the callback with explicit lambda
// syntax (`||(a:int,b:int) => { ... }`) until this is fixed.
//
// This file is intentionally a failing probe; run-probes.sh does not gate on it.

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
