import "../test.ch"

namespace cool {
    struct Pair2 {
        var a : int
        var b : int
    }
    func pair2_sum_call(p : Pair2*) : int {
        return pair2_sum(p);
    }
    func pair2_sum(p : Pair2*) : int {
        return p.a + p.b;
    }
}

namespace cool {
    func pair2_ext_sum(p : Pair2*) : int {
        return pair2_sum(p);
    }
    func pair2_indirect_mul(p : Pair2*) : int {
        return pair2_mul(p);
    }
    func pair2_mul(p : Pair2*) : int {
        return p.a * p.b;
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
    test("test that namespace functions can call each other", () => {
        var p = cool::Pair2 {
            a : 1,
            b : 2
        }
        return cool::pair2_sum(&p) == 3;
    })
    test("test that namespace extended functions work - 1", () => {
        var p = cool::Pair2 {
            a : 1,
            b : 2
        }
        return cool::pair2_mul(&p) == 2;
    })
    test("test that namespace extended functions work - 2", () => {
        var p = cool::Pair2 {
            a : 1,
            b : 2
        }
        return cool::pair2_indirect_mul(&p) == 2;
    })
    test("test that namespace extended functions work - 3", () => {
        var p = cool::Pair2 {
            a : 1,
            b : 2
        }
        return cool::pair2_ext_sum(&p) == 3;
    })
}