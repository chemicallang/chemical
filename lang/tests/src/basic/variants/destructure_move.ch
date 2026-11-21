using std::Option;
using std::string;
using std::vector;

// A pattern-bound variant payload (`var Some(id) = o`) is an alias into `o`, not a
// value that owns a drop flag of its own: whatever takes the payload must clear the
// *variant's* flag. Before aggregate initializers handled that, a payload moved into
// a struct field was freed twice — once by the field, once by the variant at scope
// exit ("free(): double free detected in tcache").
struct DestructureBox {
    var id : string
}

func destructure_long_text() : string {
    var s = string()
    s.append_view("0123456789abcdefghijklmnopqrstuvwxyz-0123456789abcdef")
    return s
}

func destructure_size_of(v : string) : size_t {
    return v.size()
}

func test_destructure_move() {
    test("a destructured payload moved into a struct field stays intact", () => {
        var o = Option.Some<string>(destructure_long_text())
        var Some(id) = o else unsafe { unreachable }
        var box = DestructureBox { id : id }
        return box.id.size() == 53u
    })

    test("a destructured payload passed by value keeps its bytes", () => {
        var o = Option.Some<string>(destructure_long_text())
        var Some(id) = o else unsafe { unreachable }
        return destructure_size_of(id) == 53u
    })

    test("a destructured payload pushed into a vector keeps its bytes", () => {
        var o = Option.Some<string>(destructure_long_text())
        var Some(id) = o else unsafe { unreachable }
        var v = vector<string>()
        v.push(id)
        var first = v.get_ptr(0u)
        return v.size() == 1u && unsafe(first.size()) == 53u
    })

    test("destructured payloads moved into structs survive allocator churn", () => {
        var idx = 0
        while(idx < 200) {
            var o = Option.Some<string>(destructure_long_text())
            var Some(id) = o else unsafe { unreachable }
            var box = DestructureBox { id : id }
            if(box.id.size() != 53u) { return false }
            idx += 1
        }
        return true
    })
}
