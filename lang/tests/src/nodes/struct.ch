func sum_this_please(a : int, b : int) : int {
    return a + b;
}

@direct_init
struct Pair {

    var a : int
    var b : int

    @constructor
    func pair(c : int) {
        init {
            a(c / 2);
            b(a);
        }
    }

    func get_pair_sum(&self) : int {
        return a + b;
    }

    func check_direct_call(&self) : int {
        return self.get_pair_sum();
    }

    func check_indirect_call(&self) : int {
        return get_pair_sum();
    }

    func check_outside_call(&self) : int {
        return sum_this_please(a, b);
    }

}

struct IntPair {
    var pair : Pair
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
    var value : *DeeplyNested3
}

func give_pair() : Pair {
    return Pair { a : 99, b : 98 }
}

func test_pair(p : Pair) : bool {
    return p.a == 99 && p.b == 98
}

struct ImpPair {

    var data : int

    @constructor
    @implicit
    func makexv(d : int) {
        init {
            data(d)
        }
    }

}

var implicit_constructor_called_or_not = false

struct ImpConstructorCallCheck {
    @implicit
    @make
    func make(d : int) {
        implicit_constructor_called_or_not = true;
    }
}

func take_imp_cons_cc(thing : ImpConstructorCallCheck) : bool {
    return true
}

func check_implicit(p : ImpPair) : bool {
    return p.data == 55;
}

struct ImpContainer {
    var imp : ImpPair
}

func give_implicit_return() : ImpPair {
    return 65;
}

func give_explicit_return() : ImpPair {
    return ImpPair(65)
}

// ----------------- Code Gen TEST Start ----------------------

// this test is that a type referencing a struct below it
// which uses the type directly should succeed
type IndirectFnRef = (r : *mut IndirectFnStructPtr) => void

// this test is that a type referencing a struct below it
// which uses the type directly should succeed
struct IndirectFnStructPtr {
    var thing : IndirectFnRef
}

// ----------------- Code Gen TEST End ----------------------

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
    test("functions present outside struct can be called inside struct", () => {
        var p = Pair {
            a : 10,
            b : 10
        }
        return p.check_outside_call() == 20;
    })
    test("struct constructor can be called", () => {
        var p = Pair(4)
        return p.a == 2 && p.b == 2;
    })
    test("direct struct values can be passed as args", () => {
        const p = Pair { a : 99, b : 98 }
        return test_pair(p);
    })
    test("const function calls returning struct values can be passed as args", () => {
        const p = give_pair();
        return test_pair(p);
    })
    test("const struct values can be passed as args", () => {
        return test_pair(Pair { a : 99, b : 98 })
    })
    test("function calls that return struct, can be passed as args", () => {
        return test_pair(give_pair())
    })
    test("implicit constructors work in function parameters - 1", () => {
        return check_implicit(55);
    })
    test("implicit constructors work in function parameters - 2", () => {
        return check_implicit(ImpPair(55));
    })
    test("implicit constructors inside structs work - 1", () => {
        var p = ImpContainer {
            imp : 55
        }
        return p.imp.data == 55;
    })
    test("implicit constructors inside structs work - 2", () => {
        var p = ImpContainer {
            imp : ImpPair(55)
        }
        return p.imp.data == 55;
    })
    test("implicit constructors inside array values work - 1", () => {
        var arr : ImpPair[1] = [ 55 ]
        return arr[0].data == 55;
    })
    test("implicit constructors inside array values work - 2", () => {
        var arr : ImpPair[1] = [ ImpPair(55) ]
        return arr[0].data == 55;
    })
    test("implicit constructors in return work - 1", () => {
        const p = give_implicit_return();
        return p.data == 65;
    })
    test("implicit constructors in return work - 2", () => {
        const p = give_explicit_return();
        return p.data == 65;
    })
    test("contained pair can be initialized properly", () => {
        var p = IntPair {
            pair : Pair {
                a : 14,
                b : 12
            }
        };
        return p.pair.get_pair_sum() == 26
    })
    test("inline struct types work as well - 1", () => {
        var point : struct Point { var a : int; var b : int; } = struct {
            a : 13,
            b : 6
        }
        return point.a + point.b == 19
    })
    test("inline struct types work as well - 2", () => {
        var point : struct Point { var a : int; var b : int; } = Point {
            a : 11,
            b : 3
        }
        return point.a + point.b == 14
    })
    test("inline struct types work as well - 3", () => {
        var point : struct { var a : int; var b : int; } = struct {
            a : 32,
            b : 22
        }
        return point.a + point.b == 54
    })
    test("implicit constructors are not called before the statement - 1", () => {
        implicit_constructor_called_or_not = false
        var call = false
        if(call && take_imp_cons_cc(333)) {
            // do nothing here
        }
        return implicit_constructor_called_or_not == false
    })
    test("implicit constructors are not called before the statement - 2", () => {
        implicit_constructor_called_or_not = false
        var call = true
        if(call && take_imp_cons_cc(333)) {
            // do nothing here
        }
        return implicit_constructor_called_or_not == true
    })
    test_inheritance();
}