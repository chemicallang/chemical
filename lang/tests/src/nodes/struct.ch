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

variant PairVarCon {
    Some(pair : Pair)
    None()
}

func create_pair() : Pair {
    return Pair {
        a : 33,
        b : 55
    }
}

func create_pair_no_type() : Pair {
    return { a : 34, b : 56 }
}

func create_pair_as_variable() : Pair {
    var p = Pair {
        a : 44,
        b : 66
    }
    return p;
}

func create_pair_as_variable_no_type() : Pair {
    var p : Pair = {
        a : 45,
        b : 67
    }
    return p;
}

func take_struct_no_type(p : Pair) : int {
    return p.a + p.b;
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

// tests that early return in constructors works
struct EarlyReturnConstructor {
    var i : int
    @make
    func make(early : bool) {
        if(early) {
            i = 2;
            return;
        }
        i = 33;
    }
}

struct DefValueTest {
    var a : int
    var b : int
    var c : int = 30
}

func create_def_value_test() : DefValueTest {
    return DefValueTest {
        a : 10,
        b : 20
    }
}

// TDIS = Test Def Initialized Struct
struct TDISA {
    var a : int = 124
}

struct TDISB : TDISA {
    var b : int = 453
}

struct TDISC : TDISB {
    var c : int
    @make
    func make() {
        c = 938
    }
}

struct TDISD : TDISC {
    var d : int = 89
}

struct TDISF : TDISD {}

func test_no_type_structs() {
    test("can return created struct values without types", () => {
        var pair = create_pair_no_type();
        return pair.a == 34 && pair.b == 56;
    })
    test("can return a newly created struct without type that is referenced", () => {
        var pair = create_pair_as_variable_no_type();
        return pair.a == 45 && pair.b == 67;
    })
    test("can send structs without type to function calls", () => {
        return take_struct_no_type({
            a : 50,
            b : 4
        }) == 54
    })
    test("can store structs without type in other structs", () => {
        var p = IntPair { pair : { a : 87, b : 32 } }
        return p.pair.a == 87 && p.pair.b == 32
    })
    test("can store structs without type in arrays", () => {
        var p : []Pair = [ { a : 45, b : 76 } ]
        return p[0].a == 45 && p[0].b == 76
    })
    test("can store structs without type in variants", () => {
        var v = PairVarCon.Some({
            a : 87, b : 99
        })
        switch(v) {
            Some(pair) => {
                return pair.a == 87 && pair.b == 99;
            }
            None => {
                return false;
            }
        }
    })
    test("can store structs without type in assignment", () => {
        var p = Pair { a : 10, b : 20 }
        p = { a : 66, b : 77 }
        return p.a == 66 && p.b == 77
    })
}

// ------------ existence test begins here -----------------

// this test verifies that constructor of a generic struct can be called
// in a generic container and generated as well

struct existence_gen_container_21345<T> {
    var a : T = 99
}

func <T> give_existence_gen_container_2343() : existence_gen_container_21345<T> {
    return existence_gen_container_21345<T>()
}

func gen_existence_gen_container_43534() {
    give_existence_gen_container_2343<int>()
}

// ------------ existence test ends here ----------------

interface GrandpaInterface {
    func give(&self) : int
}

func call_grandpa_interface_func(a : dyn GrandpaInterface) : int {
    return a.give()
}

interface ParentInterface : GrandpaInterface {}

struct ChildStructImplementsGrandpa : ParentInterface {
    var a : int
    var b : int
    @override
    func give(&self) : int {
        return a + b;
    }
}

struct PlaceboOverrideGrandparentChild : ParentInterface {
    @override
    func give(&self) : int {
        return 383
    }
}

struct DefaultConstructedPoint {
    var a : int
    var b : int
    @make
    func make() {
        a = 73
        b = 953
    }
}

struct DefaultConstructedPointContainer {
    var x : int
    var y : int
    var p : DefaultConstructedPoint
}


struct contained_def_initialized_7326 {
    var c : int = 877
}

struct container_def_init_82773 {
    var c : contained_def_initialized_7326
}

struct builder_pattern {
    var i : int
    func inc(&mut self) : &mut builder_pattern {
        i++;
        return self;
    }
    func dec(&mut self) : *mut builder_pattern {
        i--;
        return &mut self;
    }
}

func test_structs() {
    test("builder pattern works - 1", () => {
        var b = builder_pattern { i : 13 }
        b.inc().dec().inc()
        return b.i == 14
    })
    test("builder pattern works - 2", () => {
        var b = builder_pattern { i : 13 }
        b.inc().inc().dec().dec()
        return b.i == 13
    })
    test("builder pattern works - 3", () => {
        var b = builder_pattern { i : 13 }
        b.dec().inc().dec()
        return b.i == 12
    })
    test_no_type_structs();
    test_structs_aliases();
    test("default values work, when returning from function", () => {
        var d = create_def_value_test();
        return d.c == 30;
    })
    test("can return a newly created struct", () => {
        var pair = create_pair();
        return pair.a == 33 && pair.b == 55;
    })
    test("can create structs with strings as keys", () => {
        var p = Pair { "a" : 323, "b" : 873 }
        return p.a == 323 && p.b == 873
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
        var arr : [1]ImpPair = [ 55 ]
        return arr[0].data == 55;
    })
    test("implicit constructors inside array values work - 2", () => {
        var arr : [1]ImpPair = [ ImpPair(55) ]
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
    test("early return in constructors works - 1", () => {
        var early = EarlyReturnConstructor(true)
        return early.i == 2;
    })
    test("early return in constructors works - 2", () => {
        var early = EarlyReturnConstructor(false)
        return early.i == 33;
    })
    test("inherited default initialized structs work", () => {
        var f = TDISF {}
        return f.a == 124 && f.b == 453 && f.c == 938 && f.d == 89
    })
    test("grandparent interface can be implemented by a child struct - 1", () => {
        var s = ChildStructImplementsGrandpa { a : 23, b : 37 }
        return s.give() == 60
    })
    test("grandparent interface can be implemented by a child struct - 2", () => {
        var s = ChildStructImplementsGrandpa { a : 34, b : 3 }
        return call_grandpa_interface_func(dyn<GrandpaInterface>(s)) == 37
    })
    test("grandparent interface can be implemented by a child struct - 3", () => {
        var s = PlaceboOverrideGrandparentChild { }
        return call_grandpa_interface_func(dyn<GrandpaInterface>(s)) == 383
    })
    test("default constructable struct is properly initialized after specified values", () => {
        var c = DefaultConstructedPointContainer { x : 921, y : 2347 }
        return c.x == 921 && c.y == 2347 && c.p.a == 73 && c.p.b == 953
    })
    test("translation for automatic initialization of contained struct works", () => {
        var c = container_def_init_82773 {}
        return c.c.c == 877
    })

}

type PairAlias = Pair

func create_pair_alias() : PairAlias {
    return PairAlias { a : 20, b : 40 }
}

func create_pair_alias_inferred() : PairAlias {
    return { a : 22, b : 44 }
}

func take_my_pair_alias(p : PairAlias) : bool {
    return p.a == 21 && p.b == 45
}

func test_structs_aliases() {
    test("returning struct alias works", () => {
        var pairAlias = create_pair_alias();
        return pairAlias.a == 20 && pairAlias.b == 40;
    })
    test("return struct alias with inferred type works", () => {
        var pairAlias = create_pair_alias_inferred();
        return pairAlias.a == 22 && pairAlias.b == 44;
    })
    test("taking struct alias as parameters works", () => {
        var pairAlias = PairAlias { a : 21, b : 45 }
        return take_my_pair_alias(pairAlias);
    })
}