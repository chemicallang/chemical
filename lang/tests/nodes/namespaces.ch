import "../test.ch"

namespace cool {
    struct Pair2 {
        var a : int
        var b : int
    }
    func pair2_sum(p : Pair2*) {
        return p.a + p.b;
    }
}

func test_namespaces() {
    test("test that namespace structs work", () => {
        var p = cool::Pair2 {
            a : 1,
            b : 2
        }
        return p.a == 1 && p.b == 2;
    })
    test("test that namespace functions work", () => {
        var p = cool::Pair2 {
            a : 1,
            b : 2
        }
        return cool::pair2_sum(&p);
    })
}