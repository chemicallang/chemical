import "../test.ch"
import "@std/std.ch"

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
        free(x);
        return result;
    })

    test("new works with native types and const", () => {
        const x = new int;
        *x = 13;
        var result = *x == 13;
        free(x);
        return result;
    })

    test("new works with pointer types", () => {
        var x = new *int;
        var y = 13;
        *x = &y
        const ptr = *x;
        var result = *ptr == 13;
        free(x);
        return result;
    })

    test("new works with struct values", () => {
        var x = new PointNew12 {
            a : 10,
            b : 20
        };
        var result = x.a == 10 && x.b == 20;
        free(x);
        return result;
    })

    test("new works with struct values and const", () => {
        const x = new PointNew12 {
            a : 10,
            b : 20
        };
        var result = x.a == 10 && x.b == 20;
        free(x);
        return result;
    })

    test("new works with access chains", () => {
        var x = new PointNew13(20, 13)
        var result = x.a == 20 && x.b == 13;
        free(x);
        return result;
    })

    test("placement new works with struct values", () => {
        var ptr = malloc(#sizeof(PointNew12));
        var x = new (ptr) PointNew12 {
            a : 87,
            b : 33
        };
        var result = x.a == 87 && x.b == 33;
        free(ptr);
        return result;
    })

    test("placement new works with access chains", () => {
        var ptr = malloc(#sizeof(PointNew12));
        var x = new (ptr) PointNew13(20, 13)
        var result = x.a == 20 && x.b == 13;
        free(ptr);
        return result;
    })

    test("placement new works without a variable", () => {
        var ptr = malloc(#sizeof(PointNew12)) as *mut PointNew12
        new (ptr) PointNew12 { a : 12, b : 43 }
        var result = ptr.a == 12 && ptr.b == 43
        free(ptr)
        return result;
    })

}