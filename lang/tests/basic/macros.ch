import "../test.ch"

struct SizeOfStrT1 {
    var p1 : int;
    var p2 : int
    var p3 : int
}

func test_sizeof() {
    test("test sizeof int", () => {
        var i = #sizeof { int }
        return i == 4;
    })
    test("test sizeof long", () => {
        var is64 = #target:is64bit {}
        var i = #sizeof { long }
        if(is64) {
            return i == 8;
        } else {
            return i == 4;
        }
    })
    test("test sizeof struct", () => {
        var i = #sizeof { SizeOfStrT1 }
        return i == 12;
    })
}

func test_macros() {
    test_sizeof();
    test("test evaluation macro works", () => {
        var evaluated = #eval { 2 + 2 };
        return evaluated == 4;
    });
}