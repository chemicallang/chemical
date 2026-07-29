func test_if() {

    test("single-line if without braces: only first stmt is conditional", () => {
        var a = 0; var b = 0
        if(true) a = 1; b = 2
        var ok1 = (a == 1 && b == 2)
        a = 0; b = 0
        if(false) a = 3; b = 4
        var ok2 = (a == 0 && b == 4)
        return ok1 && ok2;
    })

}
