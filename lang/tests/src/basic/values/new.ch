
struct PointNew12 {
    var a : int
    var b : int
}

struct PointNew13 {

    var a : int
    var b : int

    @make
    func make(x : int, y : int) {
        a = x;
        b = y;
    }

}

namespace MultiIdStrTyCon {
    struct PointMultiId {
        var a : int
        var b : int
    }
}

variant OptIntPlNew {
    Some(value : int)
    None()
}

func test_new() {

    // test new
    // 1 - with different type
    // 2 - with value struct / chain
    // 3 - with placement new struct / chain
    // 4 - and changing const and var

    test("new works with native types", () => {
        var x = new int;
        *x = 13;
        var result = *x == 13;
        dealloc x;
        return result;
    })

    test("new works with native types and const", () => {
        const x = new int;
        *x = 13;
        var result = *x == 13;
        dealloc x;
        return result;
    })

    test("new works with pointer types", () => {
        var x = new *int;
        var y = 13;
        *x = &y
        const ptr = *x;
        var result = *ptr == 13;
        dealloc x;
        return result;
    })

    test("new works with single identifier struct types", () => {
        var x = new PointNew13
        x.a = 234
        x.b = 111
        return x.a == 234 && x.b == 111
    })

    test("new works with multiple identifier struct types", () => {
        var x = new MultiIdStrTyCon::PointMultiId
        x.a = 821
        x.b = 2834
        return x.a == 821 && x.b == 2834
    })

    test("new works with struct values", () => {
        var x = new PointNew12 {
            a : 10,
            b : 20
        };
        var result = x.a == 10 && x.b == 20;
        dealloc x;
        return result;
    })

    test("new works with struct values and const", () => {
        const x = new PointNew12 {
            a : 10,
            b : 20
        };
        var result = x.a == 10 && x.b == 20;
        dealloc x;
        return result;
    })

    test("new works with access chains", () => {
        var x = new PointNew13(20, 13)
        var result = x.a == 20 && x.b == 13;
        dealloc x;
        return result;
    })

    test("placement new works with struct values", () => {
        var ptr = malloc(sizeof(PointNew12));
        var x = new (ptr) PointNew12 {
            a : 87,
            b : 33
        };
        var result = x.a == 87 && x.b == 33;
        dealloc ptr;
        return result;
    })

    test("placement new works with access chains", () => {
        var ptr = malloc(sizeof(PointNew12));
        var x = new (ptr) PointNew13(20, 13)
        var result = x.a == 20 && x.b == 13;
        dealloc ptr;
        return result;
    })

    test("placement new works without a variable", () => {
        var ptr = malloc(sizeof(PointNew12)) as *mut PointNew12
        new (ptr) PointNew12 { a : 12, b : 43 }
        var result = ptr.a == 12 && ptr.b == 43
        dealloc ptr
        return result;
    })

    test("placement new works with variant calls", () => {
        var ptr = new OptIntPlNew
        new (ptr) OptIntPlNew.Some(763)
        var Some(value) = *ptr else return false
        var result = value
        dealloc ptr
        return value == 763
    })

}