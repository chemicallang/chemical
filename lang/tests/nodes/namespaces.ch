import "../test.ch"

namespace cool {
    struct Pair2 {
        var a : int
        var b : int
    }
    func pair2_sum_call(p : *Pair2) : int {
        return pair2_sum(p);
    }
    func pair2_sum(p : *Pair2) : int {
        return p.a + p.b;
    }
    typealias kinda_int = int
}

namespace cool {
    func pair2_ext_sum(p : *Pair2) : int {
        return pair2_sum(p);
    }
    func pair2_indirect_mul(p : *Pair2) : int {
        return pair2_mul(p);
    }
    func pair2_mul(p : *Pair2) : int {
        return p.a * p.b;
    }
}

namespace closed_bro {
    func bring_me_in() : int {
        return 11;
    }
}

namespace all_closed {
    func check_im_closed() : int {
        return 22;
    }
}

using closed_bro::bring_me_in;
using namespace all_closed;

func test_namespaces() {
    test("test that namespace structs work", () => {
        var p = cool::Pair2 {
            a : 1,
            b : 2
        }
        return p.a == 1 && p.b == 2;
    })
    test("test that namespace structs work without values", () => {
        var p : cool::Pair2
        p.a = 1;
        p.b = 2;
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
    test("test typealias from namespace works", () => {
        var t : cool::kinda_int
        t = 5;
        return t == 5;
    })
    test("test using statement can bring identifier into current scope - 2", () => {
        return check_im_closed() == 22;
    })
    test("test using statement can bring identifier into current scope - 2", () => {
        return bring_me_in() == 11;
    })
}