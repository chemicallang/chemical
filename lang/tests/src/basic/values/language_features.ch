// Core-language coverage for features that the MIR backend still lowers via the
// legacy C visitor fallback (see the `[MIR] lowering failed` log). These are
// language-level constructs, so they belong in the main compiled suite: if a
// construct regresses, these fail instead of silently falling back.

// --- if / loop / switch value expressions ---

func lf_if_value(cond : bool) : int {
    var r = if(cond) 10 else 20
    return r
}

func lf_loop_value(n : int) : int {
    var i = 0
    var r = loop {
        if(i >= n) { break i }
        i = i + 1
    }
    return r
}

func lf_switch_value(x : int) : int {
    var r = switch(x) {
        0 => 100
        1 => 200
        default => 300
    }
    return r
}

// --- pattern matching ---

variant LfShape {
    Circle(radius : int)
    Rect(w : int, h : int)
}

func lf_area(s : LfShape) : int {
    switch(s) {
        Circle(r) => {
            return r * r * 3
        }
        Rect(w, h) => {
            return w * h
        }
    }
}

func lf_is_circle(s : LfShape) : bool {
    return s is LfShape.Circle
}

// --- for-in over an array ---

func lf_sum(arr : [4]int) : int {
    var total = 0
    for(var v in arr) {
        total = total + v
    }
    return total
}

// --- do-while ---

func lf_do_while(limit : int) : int {
    var i = 0
    do {
        i = i + 1
    } while(i < limit)
    return i
}

// --- typealias ---

type LfInt = int;

func lf_typealias(v : LfInt) : LfInt {
    return v + 1
}

// --- operator overloading on an aggregate ---

@direct_init
struct LfVec {
    var x : int
    var y : int
}

impl core::ops::Add<LfVec, LfVec> for LfVec {
    func add(&self, rhs : LfVec) : LfVec {
        return LfVec { x : self.x + rhs.x, y : self.y + rhs.y }
    }
}

func lf_add_vectors(a : LfVec, b : LfVec) : LfVec {
    return a + b
}

// --- index operator overloading ---

struct LfBox {
    var a : int
    var b : int
}

func take_ref(ptr : &int) : &int {
    return ptr
}

impl core::ops::Index<int, int> for LfBox {
    func index(&self, i : int) : &int {
        if(i == 0) {
            return take_ref(&self.a)
        } else {
            return take_ref(&self.b)
        }
    }
}

func lf_index_box(b : LfBox, i : int) : int {
    const r = b[i]
    return r
}

func test_language_features() {
    test("if value expression", () => {
        return lf_if_value(true) == 10 && lf_if_value(false) == 20
    })
    test("loop value expression", () => {
        return lf_loop_value(7) == 7 && lf_loop_value(0) == 0
    })
    test("switch value expression", () => {
        return lf_switch_value(0) == 100 && lf_switch_value(1) == 200 && lf_switch_value(9) == 300
    })
    test("pattern matching binds variant payload", () => {
        return lf_area(LfShape.Circle(3)) == 27 && lf_area(LfShape.Rect(4, 5)) == 20
    })
    test("is-expression on a variant", () => {
        return lf_is_circle(LfShape.Circle(1)) && !lf_is_circle(LfShape.Rect(1, 1))
    })
    test("for-in over an array", () => {
        var arr : [4]int = [1, 2, 3, 4]
        return lf_sum(arr) == 10
    })
    test("do-while executes the body at least once", () => {
        return lf_do_while(3) == 3 && lf_do_while(0) == 1
    })
    test("typealias resolves to the aliased type", () => {
        return lf_typealias(41) == 42
    })
    test("operator overloading on an aggregate", () => {
        var s = lf_add_vectors(LfVec { x : 1, y : 2 }, LfVec { x : 3, y : 4 })
        return s.x == 4 && s.y == 6
    })
    test("index operator overloading", () => {
        var b = LfBox { a : 7, b : 9 }
        return lf_index_box(b, 0) == 7 && lf_index_box(b, 1) == 9
    })
}
