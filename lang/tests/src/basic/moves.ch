var copy_called = 0;
var delete_called = 0;

struct ClearObj {

    var i : int

    @delete
    func delete(&self) {
        delete_called++;
    }

}

struct MoveObj {

    var i : int

    @delete
    func delete(&self) {
        delete_called++;
    }

}

struct ImpCopyObj {

    var i : int

    @implicit
    @copy
    func copy(&mut self, other : &self) {
        i = other.i
        copy_called++;
    }

    @delete
    func delete(&self) {
        delete_called++;
    }

}

struct CopyObj {

    var i : int

    @copy
    func copy(&mut self, other : &self) {
        i = other.i
    }

}

struct CopyObjCon {
    var c : CopyObj
}

func give_copy_obj_i(c : CopyObj) : int {
    return c.i;
}

struct ImpCopyObjCon {
    var i : ImpCopyObj
}

variant OptCopy {
    Some(i : ImpCopyObj)
    None()
}

variant OptMove {
    Some(m : MoveObj)
    None()
}

struct MoveObjCon {
    var m : MoveObj
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

func moved_param_not_moved(m : MoveObj) {
    var d = m;
}

func take_clear_obj(c : ClearObj) {

}

func take_move_obj(m : MoveObj) {

}

func move_obj_conditionally(m : MoveObj, move : bool) {
    if(move) {
        take_move_obj(m)
    }
}

func get_moved_clear_i(c : ClearObj) : int {
    return c.i;
}

func get_moved_move_i(m : MoveObj) : int {
    return m.i;
}

func get_moved_copy_i(m : ImpCopyObj) : int {
    return m.i;
}

struct NonMovableObj {
    var a : int
    var b : int
}

variant NonMovableOpt {
    Some(n : NonMovableObj)
    None()
}

struct NonMCon {
    var n : NonMovableObj
}

func test_non_movable_obj(n : NonMovableObj) : bool {
    return n.a == 110 && n.b == 220
}

func <T> test_sent_gen(n : T, test_lamb : (value : &T) => bool) : bool {
    return test_lamb(n)
}

func <T> ret_sent_gen(a : T) : T {
    return a;
}

func <T> test_non_movable_init(n : T, test_lamb : (value : &T) => bool) : bool {
    var x = n;
    return test_lamb(x)
}

func <T> test_non_movable_reassignment(value : T, value2 : T, test_lamb : (a : &T) => bool) : bool {
    var a = value
    a = value2
    return test_lamb(a)
}

struct CopyableObj {
    var a : int
    var b : int
}

func change_copyable_movable_obj(n : CopyableObj) {
    n.a = 89
    n.b = 83
}

func test_moves() {

    // TESTING WHETHER NON MOVABLE OBJ ARE COPIED WHEN PASSED AROUND

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
        var n = [ a ]
        return n[0].a == 556 && n[0].b == 766 && a.a == 556 && a.b == 766;
    })

    test("non movable objects can be stored in variants by ref", () => {
        var a = NonMovableObj { a : 323, b : 124 }
        var n = NonMovableOpt.Some(a)
        switch(n) {
            Some(n) => {
                return n.a == 323 && n.b == 124
            }
            None() => {
                return false;
            }
        }
    })

    test("copyable objects aren't modified when passed to functions directly", () => {
        var n = CopyableObj { a : 323, b : 124 }
        change_copyable_movable_obj(n);
        return n.a == 323 && n.b == 124;
    })

    // TESTING WHETHER NEW OWNERS OF MOVED (cleared) OBJECTS ARE VALID

    test("moved into var init is valid - (cleared)", () => {
        var obj = ClearObj { i : 343 }
        var d = obj
        return d.i == 343
    })
    test("moved into assignment is valid - (cleared)", () => {
        var obj = ClearObj { i : 556 }
        var d = ClearObj { i : 23 }
        d = obj
        return d.i == 556
    })
    test("moved into struct member is valid - (cleared)", () => {
        var obj = ClearObj { i : 322 }
        var con = ClearObjCon { c : obj }
        return con.c.i == 322;
    })
    test("moved into struct member using assignment is valid - (cleared)", () => {
        var obj = ClearObj { i : 775 }
        var con = ClearObjCon { c : ClearObj { i : 544 } }
        con.c = obj;
        return con.c.i == 775;
    })
    test("moved into array value is valid - (cleared)", () => {
        var obj = ClearObj { i : 323 }
        var con = [ obj ]
        return con[0].i == 323;
    })
    test("moved into array value using assignment is valid - (cleared)", () => {
        var obj = ClearObj { i : 656 }
        var con = [ ClearObj { i : 776 } ]
        con[0] = obj
        return con[0].i == 656;
    })
    test("moved into function param is valid - (cleared)", () => {
        var obj = ClearObj { i : 987 }
        const i = get_moved_clear_i(obj);
        return i == 987;
    })
    test("moved into variant call is valid - (cleared)", () => {
        var obj = ClearObj { i : 645 }
        var opt : OptClear = OptClear.Some(obj);
        switch(opt) {
            Some(c) => {
                return c.i == 645
            }
            None() => {
                return false;
            }
        }
    })

    // TESTING WHETHER NEW OWNERS OF MOVED (movable) OBJECTS ARE VALID

    test("moved into var init is valid - (movable)", () => {
        var obj = MoveObj { i : 343 }
        var d = obj
        return d.i == 343
    })
    test("moved into assignment is valid - (movable)", () => {
        var obj = MoveObj { i : 556 }
        var d = MoveObj { i : 23 }
        d = obj
        return d.i == 556
    })
    test("moved into struct member is valid - (movable)", () => {
        var obj = MoveObj { i : 322 }
        var con = MoveObjCon { m : obj }
        return con.m.i == 322;
    })
    test("moved into struct member using assignment is valid - (movable)", () => {
        var obj = MoveObj { i : 775 }
        var con = MoveObjCon { m : MoveObj { i : 544 } }
        con.m = obj;
        return con.m.i == 775;
    })
    test("moved into array value is valid - (movable)", () => {
        var obj = MoveObj { i : 323 }
        var con = [ obj ]
        return con[0].i == 323;
    })
    test("moved into array value using assignment is valid - (movable)", () => {
        var obj = MoveObj { i : 656 }
        var con = [ MoveObj { i : 776 } ]
        con[0] = obj
        return con[0].i == 656;
    })
    test("moved into function param is valid - (movable)", () => {
        var obj = MoveObj { i : 987 }
        const i = get_moved_move_i(obj);
        return i == 987;
    })
    test("moved into variant call is valid - (movable)", () => {
        var obj = MoveObj { i : 645 }
        var opt : OptMove = OptMove.Some(obj);
        switch(opt) {
            Some(m) => {
                return m.i == 645
            }
            None() => {
                return false;
            }
        }
    })

    // TESTING WHETHER NEW OWNERS OF MOVED (copied) OBJECTS ARE VALID

    test("moved into var init is valid - (copied)", () => {
        var obj = ImpCopyObj { i : 343 }
        var d = obj
        return d.i == 343
    })
    test("moved into assignment is valid - (copied)", () => {
        var obj = ImpCopyObj { i : 556 }
        var d = ImpCopyObj { i : 23 }
        d = obj
        return d.i == 556
    })
    test("moved into struct member is valid - (copied)", () => {
        var obj = ImpCopyObj { i : 322 }
        var con = ImpCopyObjCon { i : obj }
        return con.i.i == 322;
    })
    test("moved into struct member using assignment is valid - (copied)", () => {
        var obj = ImpCopyObj { i : 775 }
        var con = ImpCopyObjCon { i : ImpCopyObj { i : 544 } }
        con.i = obj;
        return con.i.i == 775;
    })
    test("moved into array value is valid - (copied)", () => {
        var obj = ImpCopyObj { i : 323 }
        var con = [ obj ]
        return con[0].i == 323;
    })
    test("moved into array value using assignment is valid - (copied)", () => {
        var obj = ImpCopyObj { i : 656 }
        var con = [ ImpCopyObj { i : 776 } ]
        con[0] = obj
        return con[0].i == 656;
    })
    test("moved into function param is valid - (copied)", () => {
        var obj = ImpCopyObj { i : 987 }
        const i = get_moved_copy_i(obj);
        return i == 987;
    })
    test("moved into variant call is valid - (copied)", () => {
        var obj = ImpCopyObj { i : 645 }
        var opt : OptCopy = OptCopy.Some(obj);
        switch(opt) {
            Some(i) => {
                return i.i == 645
            }
            None() => {
                return false;
            }
        }
    })

    // TESTING CLEAR FUNCTION CALLS ON MOVES

    delete_called = 0;
    test("object moved from var init, delete is called once on new var init", () => {
        if(true) {
            var obj = ClearObj { i : 432 }
            var other = obj
            if(other.i != 432) return false;
        }
        return delete_called == 1;
    })
    delete_called = 0;
    test("function param object moved, delete is called once - 1", () => {
        moved_param_not_cleared(ClearObj { i : 543 });
        return delete_called == 1;
    })
    delete_called = 0;
    test("function param object moved, delete is called once - 2", () => {
        var c = ClearObj { i : 543 }
        moved_param_not_cleared(c);
        return delete_called == 1;
    })
    delete_called = 0;
    test("movable member of struct, delete function is called on previous value on assignment", () => {
        var con = ClearObjCon { c : ClearObj { i : 655 } }
        con.c = ClearObj { i : 543 }
        return delete_called == 1;
    })
    delete_called = 0;
    /** TODO: test moves a member of struct without initializing
     * test("movable member of struct, delete function is not called on previous moved value on assignment", () => {
     *     var con = ClearObjCon { c : ClearObj { i : 655 } }
     *     take_clear_obj(con.c);
     *     con.c = ClearObj { i : 543 }
     *     return delete_called == 1; // called once, inside the take_clear_obj, but not due to assignment
     * })
     */
    /** TODO: test moves a member of struct without initializing
     * delete_called = 0;
     * test("clear function is called on previous value when moving into other struct", () => {
     *     if(true) {
     *         var con = ClearObjCon { c : ClearObj { i : 453 } }
     *         var con2 = ClearObjCon { c : con.c }
     *     }
     *     return delete_called == 2;
     * })
     */
    /** TODO: test moves a member of struct without initializing
     * delete_called = 0;
     * test("clear function is called on previous value when moving into array", () => {
     *     if(true) {
     *         var con = ClearObjCon { c : ClearObj { i : 453 } }
     *         var con2 = [ con.c ]
     *     }
     *     return delete_called == 2;
     * })
     */
    /** TODO: test moves a member of struct without initializing
     * delete_called = 0;
     * test("clear function is called on previous value when moving into var init", () => {
     *     if(true) {
     *         var con = ClearObjCon { c : ClearObj { i : 453 } }
     *         var con2 = con.c
     *     }
     *     return delete_called == 2;
     * })
     */
    // TESTING MOVE FUNCTIONS FROM HERE

    delete_called = 0;
    test("delete is called once, when moving from var init", () => {
        if(true) {
            var a = MoveObj { i : 32 }
            var b = a
        }
        return delete_called == 1;
    })

    delete_called = 0;
    test("delete is called once, when moving using assignment", () => {
        var result : int = 0;
        if(true) {
            var a = MoveObj { i : 32 }
            var b = MoveObj { i : 33 }
            a = b
            result = a.i
        }
        // how this works, b is mem copied into a, after a has been destroyed and a is destroyed at the end of scope again
        // b doesn't need to be move called into a, since b will never be accessed again, a becomes the only owner
        return delete_called == 2 && result == 33;
    })

    delete_called = 0;
    test("delete is called, if object is not moved", () => {
        var a = MoveObj { i : 32 }
        move_obj_conditionally(a, false)
        return delete_called == 1;
    })

    delete_called = 0;
    test("delete is called once, if object is moved", () => {
        var a = MoveObj { i : 32 }
        move_obj_conditionally(a, true)
        return delete_called == 1;
    })

    /**
     * delete_called = 0;
     * test("delete function is called, when moving struct member that would leave other one empty", () => {
     *     var result : int = 7373
     *     if(true) {
     *         var a = MoveObjCon { m : MoveObj { i : 32 } }
     *         var b = MoveObjCon { m : MoveObj { i : 33 } }
     *         a.m = b.m
     *         b.m = MoveObj { i : 34 }
     *         result = a.m.i
     *     }
     *     // first a.m is destructed, b.m is moved into a.m (using move constructor, self = a.m, other = b.m)
     *     // then a is destructed, and b is destructed, that's three destructors called, a single move
     *     return delete_called == 3 && result == 33;
     * })
     */

    delete_called = 0;
    test("function param object moved, move function is not called", () => {
        moved_param_not_moved(MoveObj { i : 543 });
        return delete_called == 1;
    })
    delete_called = 0;
    test("function param object moved, move function is not called", () => {
        var m = MoveObj { i : 543 };
        moved_param_not_moved(m);
        return delete_called == 1;
    })
    delete_called = 0;
    test("movable member of struct, delete function is called on previous value on assignment - 2", () => {
        var con = MoveObjCon { m : MoveObj { i : 655 } }
        con.m = MoveObj { i : 543 }
        return delete_called == 1;
    })
    /** TODO: test moves a member of struct without initializing
     * delete_called = 0;
     * test("movable member of struct, delete function is not called on previous moved value on assignment - 2", () => {
     *     var con = MoveObjCon { m : MoveObj { i : 655 } }
     *     take_move_obj(con.m);
     *     con.m = MoveObj { i : 543 }
     *     return delete_called == 1; // called once, inside the take_move_obj, but not due to assignment
     * })
     */
    /** TODO: test moves a member of struct without initializing
     * delete_called = 0;
     * test("move function is called on previous value when moving struct member into other struct", () => {
     *     if(true) {
     *         var con = MoveObjCon { m : MoveObj { i : 453 } }
     *         var con2 = MoveObjCon { m : con.m }
     *     }
     *     return delete_called == 2;
     * })
     */
    /** TODO: test moves a member of struct without initializing
     * delete_called = 0;
     * test("move function is called on previous value when moving struct member into array", () => {
     *     if(true) {
     *         var con = MoveObjCon { m : MoveObj { i : 453 } }
     *         var con2 = [ con.m ]
     *         // TODO this should result in an error, that con object has uninitialized fields before destruction
     *     }
     *     return delete_called == 2;
     * })
     */
    /** TODO: test moves a member of struct without initializing
     * delete_called = 0;
     * test("move function is called when moving struct member into var init", () => {
     *     if(true) {
     *         var con = MoveObjCon { m : MoveObj { i : 453 } }
     *         var con2 = con.m
     *     }
     *     return delete_called == 2;
     * })
     */

    // TESTING EXPLICIT COPY FUNCTION CALLS FROM HERE

    test("explicit copy function calls work", () => {
        var c = CopyObj { i : 45 }
        var d = c.copy();
        return d.i == 45;
    })
    test("explicit copy function calls work in function arguments", () => {
        var c = CopyObj { i : 43 }
        return give_copy_obj_i(c.copy()) == 43;
    })
    test("explicit copy function calls work in struct value", () => {
        var c = CopyObj { i : 46 }
        var d = CopyObjCon { c : c.copy() }
        return d.c.i == 46;
    })
    test("explicit copy function calls work in array value", () => {
        var c = CopyObj { i : 46 }
        var d = [ c.copy() ]
        return d[0].i == 46;
    })
    test("explicit copy function calls work in assignment", () => {
        var c = CopyObj { i : 46 }
        var d = CopyObj { i : 44 }
        c = d.copy()
        return c.i == 44;
    })

}