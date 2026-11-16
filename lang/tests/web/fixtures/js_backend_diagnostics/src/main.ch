// JS backend — diagnostics fixture (expected to FAIL to compile for JS).
//
// Inline assembly has no JavaScript equivalent. The safety pass requires it in an
// `unsafe` block, so it reaches the backend; the backend must then reject it at
// build time with the function name *and* the source position, for every
// offending site, so this is never discovered as a browser console error.

func uses_asm_a(x : int) : int {
    var v : int = x
    unsafe { asm("nop") }
    return v
}

func uses_asm_b(x : int) : int {
    var v : int = x + 1
    unsafe { asm("nop") }
    return v
}

public func main() : int {
    unsafe {
        printf("%d\n", uses_asm_a(1) + uses_asm_b(2))
    }
    return 0
}
