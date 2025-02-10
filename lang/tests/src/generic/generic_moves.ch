import "/test.ch"

func test_generic_moves() {

    // TESTING WHETHER NON MOVABLE OBJECTS ARE COPIED WHEN PASSED AROUND IN GENERICS

    // TODO THESE TESTS ARE NOT WORKING NOT_WORKING
    // test("non movable objects can be passed to functions", () => {
    //     var n = NonMovableObj { a : 110, b : 220 }
    //     return test_sent_gen(n, (n) => {
    //         return n.a == 110 && n.b == 220
    //     });
    // })

    // test("non movable object can be assigned to another variable", () => {
    //     var a = NonMovableObj { a : 560, b : 343 }
    //     return test_non_movable_init(a, (b) => {
    //         return b.a == 560 && b.b == 343
    //     })
    // })

    // test("non movable object can be re assigned to another variable", () => {
    //     var a = NonMovableObj { a : 560, b : 343 }
    //     var b = NonMovableObj { a : 33, b : 66 };
    //     return test_non_movable_reassignment(a, b, (a) => {
    //         return a.a == 33 && a.b == 66
    //     })
    // })

    // test("non movable object can be stored in struct by ref", () => {
    //     var a = NonMovableObj { a : 560, b : 343 }
    //     var n = NonMCon { n : ret_sent_gen(a) }
    //     return n.n.a == 560 && n.n.b == 343 && a.a == 560 && a.b == 343
    // })

    // test("non movable object can be stored in array by ref", () => {
    //     var a = NonMovableObj { a : 556, b : 766 }
    //     var n = { ret_sent_gen(a) }
    //     return n[0].a == 556 && n[0].b == 766 && a.a == 556 && a.b == 766;
    // })

    // test("non movable objects can be stored in variants by ref", () => {
    //     var a = NonMovableObj { a : 323, b : 124 }
    //     var n = NonMovableOpt.Some(ret_sent_gen(a))
    //     switch(n) {
    //         Some(n) => {
    //             return n.a == 323 && n.b == 124
    //         }
    //         None() => {
    //             return false;
    //         }
    //     }
    // })
    //

}