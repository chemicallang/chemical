import "/test.ch"

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

struct Derived32 : Base89 {

    var f : Field88

}

struct Derived78 : Base89 {

    var f : Field88

    @make
    func make() {
        // manually generated default constructor
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
}