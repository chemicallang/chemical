func test_in_value() {
    test("in value works with constant expression in return", () => {
        return 'a' in 'a', 'b', 'c'
    })
    test("in value works with non constant expression in return", () => {
        var x = 'x'
        x = 'a'
        return x in 'a', 'b', 'c'
    })
    test("in value works with constant expression in const", () => {
        const x = 'a' in 'a', 'b', 'c'
        return x;
    })
    test("in value works with non constant expression in const", () => {
        var x = 'x'
        x = 'a'
        const y = x in 'a', 'b', 'c'
        return y;
    })
    test("in value works when negating", () => {
        return !('a' !in 'a', 'b', 'c');
    })
    test("in value works when negating", () => {
        var x = 'x'
        x = 'a'
        return !(x !in 'a', 'b', 'c');
    })
    test("in value works in an if statement - 1", () => {
        if('a' in 'a', 'b', 'c') {
            return true;
        } else {
            return false;
        }
    })
    test("in value works in an if statement - 2", () => {
        if('a' !in 'a', 'b', 'c') {
            return false;
        } else {
            return true;
        }
    })
    test("in value works in an if statement - 3", () => {
        var x = 'x'
        x = 'a'
        if(x in 'a', 'b', 'c') {
            return true;
        } else {
            return false;
        }
    })
    test("in value works in an if statement - 4", () => {
        var x = 'x'
        x = 'a'
        if(x !in 'a', 'b', 'c') {
            return false;
        } else {
            return true;
        }
    })
}