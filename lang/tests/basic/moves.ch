import "../test.ch"

var clear_called = 0;
var delete_called = 0;

struct ClearObj {

    var i : int

    @clear
    func clear() {
        clear_called++;
    }

    @delete
    func delete() {
        delete_called++;
    }

}

variant OptClear {
    Some(c : ClearObj)
    None()
}

struct ClearObjCon {
    var c : ClearObj
}

func moved_param_not_cleared(c : ClearObj) {
    var d = c;
    // the function param 'c' has been moved into 'd' and destructor won't be called on 'c' but on 'd'
    // clear won't be called on 'c' as well, because it can't be used
}

func take_clear_obj(c : ClearObj) {

}

func get_moved_clear_i(c : ClearObj) : int {
    return c.i;
}

struct NonMovableObj {
    var a : int
    var b : int
}

struct NonMCon {
    var n : NonMovableObj
}

func test_non_movable_obj(n : NonMovableObj) : bool {
    return n.a == 110 && n.b == 220
}

func test_moves() {

    // TESTING WHETHER NON MOVABLE OBJ ARE VALID WHEN MOVED AROUND

    test("non movable objects can be passed to functions", () => {
        var n = NonMovableObj { a : 110, b : 220 }
        return test_non_movable_obj(n);
    })

    test("non movable object can be assigned to another variable", () => {
        var a = NonMovableObj { a : 560, b : 343 }
        var b = a;
        return b.a == 560 && b.b == 343 && a.a == 560 && a.b == 343
    })

    test("non movable object can be re assigned to another variable", () => {
        var a = NonMovableObj { a : 560, b : 343 }
        var b = NonMovableObj { a : 33, b : 66 };
        b = a
        return b.a == 560 && b.b == 343 && a.a == 560 && a.b == 343
    })

    test("non movable object can be stored in struct by ref", () => {
        var a = NonMovableObj { a : 560, b : 343 }
        var n = NonMCon { n : a }
        return n.n.a == 560 && n.n.b == 343 && a.a == 560 && a.b == 343
    })

    test("non movable object can be stored in array by ref", () => {
        var a = NonMovableObj { a : 556, b : 766 }
        var n = { a }
        return n[0].a == 556 && n[0].b == 766 && a.a == 556 && a.b == 766;
    })

    // TESTING WHETHER NEW OWNERS OF MOVED OBJECTS ARE VALID

    test("moved into var init is valid", () => {
        var obj = ClearObj { i : 343 }
        var d = obj
        return d.i == 343
    })
    test("moved into assignment is valid", () => {
        var obj = ClearObj { i : 556 }
        var d = ClearObj { i : 23 }
        d = obj
        return d.i == 556
    })
    test("moved into struct member is valid", () => {
        var obj = ClearObj { i : 322 }
        var con = ClearObjCon { c : obj }
        return con.c.i == 322;
    })
    test("moved into struct member using assignment is valid", () => {
        var obj = ClearObj { i : 775 }
        var con = ClearObjCon { c : ClearObj { i : 544 } }
        con.c = obj;
        return con.c.i == 775;
    })
    test("moved into array value is valid", () => {
        var obj = ClearObj { i : 323 }
        var con = { obj }
        return con[0].i == 323;
    })
    test("moved into array value using assignment is valid", () => {
        var obj = ClearObj { i : 656 }
        var con = { ClearObj { i : 776 } }
        con[0] = obj
        return con[0].i == 656;
    })
    test("moved into function param is valid", () => {
        var obj = ClearObj { i : 987 }
        const i = get_moved_clear_i(obj);
        return i == 987;
    })
    test("moved into variant call is valid", () => {
        var obj = ClearObj { i : 645 }
        var opt : OptClear = OptClear.Some(obj);
        switch(opt) {
            OptClear.Some(c) => {
                return c.i == 645
            }
            OptClear.None() => {
                return false;
            }
        }
    })

    // TESTING CLEAR FUNCTION CALLS ON MOVES

    clear_called = 0;
    delete_called = 0;
    test("object moved from var init, clear function is not called", () => {
        if(true) {
            var obj = ClearObj { i : 432 }
            var other = obj
            if(other.i != 432) return false;
        }
        // clear is not called, because obj won't be destructed, but delete is called, because other is destructed
        return clear_called == 0 && delete_called == 1;
    })
    clear_called = 0;
    delete_called = 0;
    test("function param object moved, clear function is not called", () => {
        moved_param_not_cleared(ClearObj { i : 543 });
        return clear_called == 0 && delete_called == 1;
    })
    clear_called = 0;
    delete_called = 0;
    test("movable member of struct, delete function is called on previous value on assignment", () => {
        var con = ClearObjCon { c : ClearObj { i : 655 } }
        con.c = ClearObj { i : 543 }
        return clear_called == 0 && delete_called == 1;
    })
    clear_called = 0;
    delete_called = 0;
    test("movable member of struct, delete function is not called on previous moved value on assignment", () => {
        var con = ClearObjCon { c : ClearObj { i : 655 } }
        take_clear_obj(con.c);
        con.c = ClearObj { i : 543 }
        return clear_called == 1 && delete_called == 1; // called once, inside the take_clear_obj, but not due to assignment
    })
    clear_called = 0;
    delete_called = 0;
    test("clear function is called on previous value when moving into other struct", () => {
        if(true) {
            var con = ClearObjCon { c : ClearObj { i : 453 } }
            var con2 = ClearObjCon { c : con.c }
        }
        return clear_called == 1 && delete_called == 2;
    })
    clear_called = 0;
    delete_called = 0;
    test("clear function is called on previous value when moving into array", () => {
        if(true) {
            var con = ClearObjCon { c : ClearObj { i : 453 } }
            var con2 = { con.c }
        }
        return clear_called == 1 && delete_called == 2;
    })
    clear_called = 0;
    delete_called = 0;
    test("clear function is called on previous value when moving into var init", () => {
        if(true) {
            var con = ClearObjCon { c : ClearObj { i : 453 } }
            var con2 = con.c
        }
        return clear_called == 1 && delete_called == 2;
    })
}