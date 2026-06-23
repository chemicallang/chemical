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

}