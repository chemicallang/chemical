struct Base89 {

    var i : int

    @make
    func make() {
        i = 10
    }

}

struct Field88 {

    var j : int

    @make
    func make() {
        j = 20
    }

}

struct constructable_obj_comptime {

    var data : *char
    var size : ubigint

    @implicit
    @constructor
    comptime func make(value : %literal_string) {
        return intrinsics::wrap(constructor(value, intrinsics::size(value)))
    }

    @constructor
    func constructor(value : *char, length : size_t) {
        data = null;
        size = length;
    }

    @constructor
    func empty_make() {
        data = null
        size = 0
    }

}

struct comptime_constructor_field_container {
    var c : constructable_obj_comptime
}

struct Derived32 : Base89 {

    var f : Field88

}

struct Derived78 : Base89 {

    var f : Field88

    var k : int = 98

    @make
    func make() {
        // manually generated default constructor
        k = 30
    }

}

interface UselessInterface {
    func give(&self) : int
}

struct ComposedInitializableStruct {
    var i : int
    @make
    func make() {
        i = 8372534
    }
}

struct UselessImplInitialization : UselessInterface {

    var first : ComposedInitializableStruct

    @make
    func make() {

    }

    @override
    func give(&self) : int {
        return first.i;
    }

}

func test_constructors() {
    test("automatically generated default constructor makes a call to the inherited default constructor", () => {
        var d = Derived32()
        return d.i == 10
    })
    test("automatically generated default constructor makes a call to the field's default constructor", () => {
        var d = Derived32()
        return d.f.j == 20
    })
    test("manual default constructor makes a call to the inherited default constructor", () => {
        var d = Derived78()
        return d.i == 10
    })
    test("manual default constructor makes a call to the field's default constructor", () => {
        var d = Derived78()
        return d.f.j == 20
    })
    test("manual default constructor also runs the code inside", () => {
        var d = Derived78()
        return d.k == 30
    })
    test("constructor initializes the field inside the struct value - 1", () => {
        var c = comptime_constructor_field_container {
            c : constructable_obj_comptime("")
        }
        return c.c.data == null && c.c.size == 0
    })
    test("constructor initializes the field inside the struct value - 2", () => {
        var c = comptime_constructor_field_container {
            c : constructable_obj_comptime("abc")
        }
        return c.c.data == null && c.c.size == 3
    })
    test("constructor initializes the field inside the struct value - 3", () => {
        var c = comptime_constructor_field_container {
            c : ""
        }
        return c.c.data == null && c.c.size == 0
    })
    test("constructor initializes the field inside the struct value - 4", () => {
        var c = comptime_constructor_field_container {
            c : "abc"
        }
        return c.c.data == null && c.c.size == 3
    })
    test("interface can exist as the first in inheritance list", () => {
        var m = UselessImplInitialization()
        return m.give() == 8372534;
    })
}