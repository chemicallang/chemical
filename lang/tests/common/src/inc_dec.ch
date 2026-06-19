func passed_inc_dec(value : int) : int {
    return value;
}

func test_inc_dec() {
    test("post incrementing / decrementing as a node - 1", () => {
        var i = 1;
        i++
        return i == 2
    })
    test("post incrementing / decrementing as a node - 2", () => {
        var i = 1;
        i--
        return i == 0
    })
    test("pre incrementing / decrementing as a node - 1", () => {
        var i = 1;
        ++i
        return i == 2
    })
    test("pre incrementing / decrementing as a node - 2", () => {
        var i = 1;
        --i
        return i == 0
    })
    test("post incrementing / decrementing as a value - 1", () => {
        var i = 1;
        const result = i++ == 1
        return result && i == 2;
    })
    test("post incrementing / decrementing as a value - 2", () => {
        var i = 1;
        const result = i-- == 1
        return result && i == 0
    })
    test("pre incrementing / decrementing as a value - 1", () => {
        var i = 1;
        const result = ++i == 2
        return result && i == 2;
    })
    test("pre incrementing / decrementing as a value - 2", () => {
        var i = 1;
        const result = --i == 0
        return result && i == 0
    })
    test("can pass post inc / dec value to functions - 1", () => {
        var i = 1;
        return passed_inc_dec(i++) == 1 && i == 2
    })
    test("can pass post inc / dec value to functions - 2", () => {
        var i = 1;
        return passed_inc_dec(i--) == 1 && i == 0
    })
    test("can pass pre inc / dec value to functions - 1", () => {
        var i = 1;
        return passed_inc_dec(++i) == 2 && i == 2
    })
    test("can pass pre inc / dec value to functions - 2", () => {
        var i = 1;
        return passed_inc_dec(--i) == 0 && i == 0
    })
    test("post inc / dec value works in loop - 1", () => {
        var j = 0;
        for(var i = 0; i < 5; i++) {
            j++
        }
        return j == 5
    })
    test("post inc / dec value works in loop - 2", () => {
        var j = 0;
        for(var i = 5; i > 0; i--) {
            j++
        }
        return j == 5
    })
    test("post inc / dec value works in loop - 1", () => {
        var j = 0;
        for(var i = 0; i < 5; ++i) {
            j++
        }
        return j == 5
    })
    test("post inc / dec value works in loop - 2", () => {
        var j = 0;
        for(var i = 5; i > 0; --i) {
            j++
        }
        return j == 5
    })
    test("post inc dec after minus works", () => {
        var i = 33;
        var j = -i++
        return i == 34 && j == -33
    })
}