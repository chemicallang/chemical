struct DerefCopyablePoint {
    var a : int
}

func take_deref_copyable_point(d : DerefCopyablePoint) : int {
    return d.a
}

func take_deref_copyable_point_m(d : DerefCopyablePoint) : int {
    d.a = 938
    return d.a
}

func take_deref_copyable_ref_point(d : &DerefCopyablePoint) : int {
    return d.a
}

func take_deref_copyable_ref_point_m(d : &mut DerefCopyablePoint) : int {
    d.a = 938
    return d.a
}

struct DerefStoredPoint {
    var d : DerefCopyablePoint
}

struct DerefWidePoint {
    var a : int
    var b : int
    var c : i64
}

func take_deref_wide_point(p : DerefWidePoint) : int {
    return p.a + p.b
}

variant DerefStoredPointVariant {
    Some(d : DerefCopyablePoint)
    None()
}

func test_dereferences() {

    test("dereferencing integer variable pointer works", () => {
        var i = 10
        var j = &i
        return *j == 10
    })

    test("dereferencing integer variable constant pointer works", () => {
        var i = 10
        const j = &i
        return *j == 10
    })

    test("dereferencing pointer to integer works", () => {
        var i = 10
        return *&i == 10
    })

    test("dereferencing trivially copyable struct variable pointer works", () => {
        var d = DerefCopyablePoint { a : 30 }
        var j = &d
        var x = *j // this would cause a copy
        return x.a == 30
    })

    test("dereferencing trivially copyable struct variable pointer causes copy", () => {
        var d = DerefCopyablePoint { a : 30 }
        var j = &d
        var x = *j // this would cause a copy
        x.a = 40
        return x.a == 40 && d.a == 30
    })

    test("dereferencing trivially copyable struct constant pointer works", () => {
        var d = DerefCopyablePoint { a : 30 }
        const j = &d
        var x = *j // this would cause a copy
        return x.a == 30
    })

    test("dereferencing trivially copyable struct variable pointer into a function call reference works", () => {
        var d = DerefCopyablePoint { a : 30 }
        var j = &d
        return take_deref_copyable_ref_point(&*j) == 30
    })

    test("dereferencing trivially copyable struct variable pointer into a function call reference causes a copy", () => {
        var d = DerefCopyablePoint { a : 30 }
        var j = &mut d
        return d.a == 30 && take_deref_copyable_ref_point_m(&mut *j) != 30
    })

    test("dereferencing trivially copyable struct variable pointer into a function call works", () => {
        var d = DerefCopyablePoint { a : 30 }
        var j = &d
        return take_deref_copyable_point(*j) == 30
    })

    test("dereferencing trivially copyable struct variable pointer into a function call causes a copy", () => {
        var d = DerefCopyablePoint { a : 30 }
        var j = &d
        return d.a == 30 && take_deref_copyable_point_m(*j) != 30
    })

    test("dereferencing trivially copyable struct constant pointer causes copy", () => {
        var d = DerefCopyablePoint { a : 30 }
        const j = &d
        var x = *j // this would cause a copy
        x.a = 40
        return x.a == 40 && d.a == 30
    })

    test("dereferencing trivially copyable struct variable pointer into a struct works", () => {
        var d = DerefCopyablePoint { a : 30 }
        var j = &d
        var d2 = DerefStoredPoint { d : *j }
        return d2.d.a == 30
    })

    test("dereferencing trivially copyable struct constant pointer into a struct works", () => {
        var d = DerefCopyablePoint { a : 30 }
        const j = &d
        var d2 = DerefStoredPoint { d : *j }
        return d2.d.a == 30
    })

    test("dereferencing trivially copyable struct variable pointer into a struct causes copy", () => {
        var d = DerefCopyablePoint { a : 30 }
        var j = &d
        var d2 = DerefStoredPoint { d : *j }
        d2.d.a = 90;
        return d2.d.a == 90 && d.a == 30
    })

    test("dereferencing trivially copyable struct variable pointer into an array works", () => {
        var d = DerefCopyablePoint { a : 30 }
        var j = &d
        var arr : [1]DerefCopyablePoint = [ *j ]
        return arr[0].a == 30
    })

    test("dereferencing trivially copyable struct constant pointer into an array works", () => {
        var d = DerefCopyablePoint { a : 30 }
        const j = &d
        var arr : [1]DerefCopyablePoint = [ *j ]
        return arr[0].a == 30
    })

    test("dereferencing trivially copyable struct variable pointer into an array causes copy", () => {
        var d = DerefCopyablePoint { a : 30 }
        var j = &d
        var arr : [1]DerefCopyablePoint = [ *j ]
        arr[0].a = 736
        return arr[0].a == 736 && d.a == 30
    })

    test("dereferencing trivially copyable struct variable pointer into a variant works", () => {
        var d = DerefCopyablePoint { a : 30 }
        var j = &d
        var d2 = DerefStoredPointVariant.Some(*j)
        var Some(dx) = d2 else unreachable
        return dx.a == 30
    })

    test("dereferencing trivially copyable struct constant pointer into a variant works", () => {
        var d = DerefCopyablePoint { a : 30 }
        const j = &d
        var d2 = DerefStoredPointVariant.Some(*j)
        var Some(dx) = d2 else unreachable
        return dx.a == 30
    })

    test("dereferencing trivially copyable struct variable pointer into a variant causes copy", () => {
        var d = DerefCopyablePoint { a : 30 }
        var j = &d
        var d2 = DerefStoredPointVariant.Some(*j)
        var Some(dx) = d2 else unreachable
        dx.a = 90;
        return dx.a == 90 && d.a == 30
    })

    test("dereferencing a wide trivially copyable struct pointer copies every field", () => {
        var d = DerefWidePoint { a : 30, b : 12, c : 4611686018427387904 }
        var j = &d
        var x = *j // this would cause a copy
        return x.a == 30 && x.b == 12 && x.c == 4611686018427387904
    })

    test("dereferencing a wide trivially copyable struct pointer causes an independent copy", () => {
        var d = DerefWidePoint { a : 30, b : 12, c : 4611686018427387904 }
        var j = &d
        var x = *j // this would cause a copy
        x.a = 1
        x.b = 2
        x.c = 3
        return x.a == 1 && x.b == 2 && x.c == 3 && d.a == 30 && d.b == 12 &&
               d.c == 4611686018427387904
    })

    test("dereferencing a wide trivially copyable struct pointer into a function call copies every field", () => {
        var d = DerefWidePoint { a : 30, b : 12, c : 4611686018427387904 }
        var j = &d
        return take_deref_wide_point(*j) == 42
    })

    test("dereferencing a wide trivially copyable struct pointer into an array of structs copies every field", () => {
        var d = DerefWidePoint { a : 30, b : 12, c : 4611686018427387904 }
        var j = &d
        var arr : [2]DerefWidePoint = [ *j, *j ]
        arr[1].c = 7
        return arr[0].a == 30 && arr[0].b == 12 && arr[0].c == 4611686018427387904 &&
               arr[1].c == 7 && d.c == 4611686018427387904
    })

}