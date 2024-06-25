import "../test.ch"

struct Pair {

    var a : int
    var b : int

    func get_pair_sum(&self) : int {
        return a + b;
    }

    func check_direct_call(&self) : int {
        return self.get_pair_sum();
    }

    func check_indirect_call(&self) : int {
        return get_pair_sum();
    }

}

func create_pair() : Pair {
    return Pair {
        a : 33,
        b : 55
    }
}

func create_pair_as_variable() : Pair {
    var p = Pair {
        a : 44,
        b : 66
    }
    return p;
}

struct DeeplyNested3 {
    var value : int
}

struct DeeplyNested2 {
    var nested : DeeplyNested3
}

struct DeeplyNested1 {
    var nested : DeeplyNested2
}

struct DeeplyNested {
    var nested : DeeplyNested1
}

struct PointerStr {
    var value : DeeplyNested3*
}

func test_structs() {
    test("can return a newly created struct", () => {
        var pair = create_pair();
        return pair.a == 33 && pair.b == 55;
    })
    test("can return a newly created struct that is referenced", () => {
        var pair = create_pair_as_variable();
        return pair.a == 44 && pair.b == 66;
    })
    test("deeply nested struct works", () => {
        var n = DeeplyNested {
            nested : DeeplyNested1 {
                nested : DeeplyNested2 {
                    nested : DeeplyNested3 {
                        value : 55
                    }
                }
            }
        }
        return n.nested.nested.nested.value == 55;
    })
    test("pointer to struct inside a struct allows access", () => {
        var nested = DeeplyNested3 {
            value : 55
        }
        var str = PointerStr {
            value : &nested
        }
        return str.value.value == 55;
    })
    test("struct members can be accessed without using self. or this.", () => {
        var p = Pair {
            a : 10,
            b : 10
        }
        return p.get_pair_sum() == 20;
    })
    test("struct functions can be accessed using self. or this.", () => {
        var p = Pair {
            a : 10,
            b : 10
        }
        return p.check_direct_call() == 20;
    })
    test("struct functions can be accessed without using self. or this.", () => {
        var p = Pair {
            a : 10,
            b : 10
        }
        return p.check_indirect_call() == 20;
    })
}