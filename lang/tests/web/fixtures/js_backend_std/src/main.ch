// JS backend — standard library through the emitted MIR (Phase 1).
//
// The ABI spike encoded these operations by hand; this program lets the *emitter*
// encode them from std's own MIR. It exercises SSO and heap strings (malloc /
// realloc), a generic `vector<T>` of a struct (monomorphized in std), struct
// field addressing, and a variant's tag + payload.
//
// Output must be byte-identical under the native and JS backends.

variant Shape {
    Circle(r : int)
    Square(s : int)
}

struct Point {
    var x : int
    var y : int
}

public func main() : int {
    unsafe {
        // std::string: empty -> SSO -> heap, then read it back
        var s = std::string()
        s.append_view("hello")
        printf("sso=%s|%zu\n", s.data(), s.size())

        var i : int = 0
        while(i < 40) {
            s.append('x')
            i = i + 1
        }
        printf("heap=%s|%zu\n", s.data(), s.size())

        // vector<Point>: growth, element addressing, field loads
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

        // variant: tag + payload, dispatched on the tag
        var sh = Shape.Circle(7)
        switch(sh) {
            Circle(r) => { printf("shape=circle|%d\n", r) }
            Square(w) => { printf("shape=square|%d\n", w) }
        }
    }
    return 0
}
