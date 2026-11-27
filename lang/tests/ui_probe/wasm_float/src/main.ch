// P0.1: scalar float/double math on wasm32 (no SIMD), checked via integer
// conversions so the freestanding printf subset (%d only) can report it.
public func main() : int {
    var a : float = 0.5f
    var b : float = 3.25f
    var c : float = a * b + 1.5f
    var d : double = 0.1 + 0.2
    var e : double = d * 3.0
    var ci = (c * 100.0f) as int
    var di = (e * 1000.0) as int
    unsafe {
        printf("P0.1 wasm_float ci=%d di=%d\n", ci, di)
    }
    if(ci == 312 && di == 900) {
        println("P0.1 wasm_float PASS")
        return 0
    }
    println("P0.1 wasm_float FAIL")
    return 1
}
