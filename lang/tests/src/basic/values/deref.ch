struct DerefCopyablePoint {
    var a : int
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

    test("dereferencing trivially copyable struct constant pointer causes copy", () => {
        var d = DerefCopyablePoint { a : 30 }
        const j = &d
        var x = *j // this would cause a copy
        x.a = 40
        return x.a == 40 && d.a == 30
    })


}