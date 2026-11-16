// JS backend ABI spike (lang/docs/js-backend-abi.md §9).
//
// The program exercises exactly the MIR features the decision rests on: a plain
// struct, a vector of structs, a real std::string through both the SSO and the
// heap path, and a variant with a tag and two cases. Its output must be
// byte-identical when executed by the C backend and by the JS ABI spike
// (scripts/js_abi_spike). If the JS side cannot reproduce these numbers, the
// emulated-linear-memory ABI is wrong and the emitter must not be written.

variant Shape {
    Circle(r : int)
    Square(s : int)
}

struct Point {
    var x : int
    var y : int
}

public func main() : int { unsafe {
    // 1. Layout facts: the JS side has to derive the same sizes from its own
    //    type table, which is what proves the layout model is shared.
    printf("point_size=%zu\n", sizeof(Point))
    printf("vec_size=%zu\n", sizeof(std::vector<Point>))
    printf("string_size=%zu\n", sizeof(std::string))

    // 2. std::string, SSO path (the constant lives inside the struct).
    var sso = std::string()
    sso.append_view("hello")
    printf("sso=%s|%zu\n", sso.data(), sso.size())

    // 3. std::string, heap path (> SSO capacity, so the union switches state and
    //    the buffer comes from malloc/free on the emulated heap).
    var heap = std::string()
    var i : int = 0
    while(i < 40) {
        heap.append('x')
        i = i + 1
    }
    printf("heap=%s|%zu\n", heap.data(), heap.size())

    // 4. vector<Point>: push_back growth, element addressing, field loads.
    var pts = std::vector<Point>()
    pts.push_back(Point { x = 1, y = 2 })
    pts.push_back(Point { x = 3, y = 4 })
    var sum : int = 0
    var j : uint = 0
    while(j < pts.size()) {
        const p = pts.get_ptr(j)
        sum = sum + p.x + p.y
        j = j + 1
    }
    printf("vec=%zu|%d\n", pts.size(), sum)

    // 5. Variant: tag + payload, plus the dispatch that proves the tag is right.
    var sh = Shape.Circle(7)
    switch(sh) {
        Circle(r) => { printf("shape=circle|%d\n", r) }
        Square(w) => { printf("shape=square|%d\n", w) }
    }

    return 0
} }
