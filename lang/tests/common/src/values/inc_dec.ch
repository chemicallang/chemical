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
    // Post-increment used as an array index must evaluate to the *old* value
    // (this is the `buf[bi++] = ...` pattern used by hex/base64 encoders).
    test("post increment as an array index uses the old value", () => {
        var buf : [4]char
        var bi = 0
        buf[bi++] = 'a'
        buf[bi++] = 'b'
        return bi == 2 && buf[0] == 'a' && buf[1] == 'b'
    })
    test("pre decrement as an array index uses the new value", () => {
        var buf : [4]char
        buf[0] = 'a'
        buf[1] = 'b'
        var bi = 2
        var c = buf[--bi]
        return bi == 1 && c == 'b'
    })
    test("hex encoding with a post-increment index writes all digits", () => {
        const hex = "0123456789ABCDEF"
        var buf : [16]char
        var bi = 0
        var val = 0x2728
        while(val > 0) { buf[bi++] = hex[val & 0xF]; val >>= 4 }
        // digits are written least-significant first: 8, 2, 7, 2
        return bi == 4 && buf[0] == '8' && buf[1] == '2' && buf[2] == '7' && buf[3] == '2'
    })
}