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

func get_moved_clear_i(c : ClearObj) : int {
    return c.i;
}

func test_moves() {

    // TESTING WHETHER NEW OWNER OF MOVED OBJECTS ARE VALID

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
}