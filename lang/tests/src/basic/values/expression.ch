func ret_log_expr_1() : bool {
    return (false || false) && !(false && false)
}

func test_bodmas() {
    test("4 + 2 / 2 == 5", () => {
        return (4 + 2 / 2) == 5;
    });
    test("4 / 2 + 2 == 4", () => {
        return (4 / 2 + 2) == 4;
    });
    test("3 * (4 + 2) - 8 / 2 == 5", () => {
        return (4 + 2 / 2) == 5;
    });
    test("3 * (4 + 2) - 8 / 2 == 14", () => {
        return (3 * (4 + 2) - 8 / 2) == 14;
    });
    test("8 / (2 + 2) * 3 - 1 == 5", () => {
        return (8 / (2 + 2) * 3 - 1) == 5;
    });
    test("(5 + 3) * 2 - 4 / (1 + 1) == 14", () => {
        return ((5 + 3) * 2 - 4 / (1 + 1)) == 14;
    });
    test("single braced value", () => {
        return (5 + (5)) == 10;
    })
    test("Simple addition and multiplication", () => {
        return (2 + 3 * 4) == 14;
    });
    test("Addition, subtraction, multiplication", () => {
        return (10 - 4 + 2 * 3) == 12;
    });
    test("Division and multiplication", () => {
        return (20 / 4 * 2) == 10;
    });
    test("Brackets and multiplication", () => {
        return ((5 + 3) * 2) == 16;
    });
    test("Brackets and division", () => {
        return (8 / (2 + 2)) == 2;
    });
    test("Complex expression", () => {
        return ((5 + 3) * 2 - 4 / (1 + 1)) == 14;
    });
    test("Nested brackets", () => {
        return (((4 * 2) + 3) * (10 / 5)) == 22;
    });
    test("Expression with negative numbers", () => {
        return (-3 * (-4 + 2)) == 6;
    });
    test("double with integer addition", () => {
        return 4 + 1.5 == 5.5;
    });
    test("Expression with decimal numbers", () => {
        return (1.5 * (4 + 1.5)) == 8.25;
    });
    test("Expression with variables", () => {
        var a = 5;
        var b = 3;
        return ((a + b) * 2 - 4 / (1 + 1)) == 14;
    });
    test("Expression with modulo", () => {
        return (10 % 3 == 1);
    });
    test("Expression with mixed operations", () => {
        return (5 + 3) * 2 - 4 / (1 + 1) == 14;
    });
    test("Longer expression with mixed operations", () => {
        return (2 * (3 + 4) / 2 - (7 % 3) + 5) == 11;
    });
    test("Expression with nested brackets and subtraction", () => {
        return (10 - (2 * (3 + 4) / 2) + (7 % 3) - 5) == -1;
    });
    test("Expression with multiple division and subtraction", () => {
        return (100 / 5 / 2 - 4 / 2 - 3) == 5;
    });
    test("Expression with consecutive multiplication", () => {
        return (2 * 3 * 4 * 5) == 120;
    });
    test("Expression with consecutive division", () => {
        return (100 / 5 / 2) == 10;
    });
    test("Expression with consecutive addition", () => {
        return (1 + 2 + 3 + 4 + 5) == 15;
    });
    test("Expression with consecutive subtraction", () => {
        return (20 - 5 - 4 - 3 - 2 - 1) == 5;
    });
    test("Expression with mixed operations and negative numbers", () => {
        return (10 - (-3 * (4 + 2)) + 8 / 2) == 32;
    });
    test("Expression with complex nested operations", () => {
        return ((10 * (5 + (3 * 2) - 4 / 2)) - (2 * (7 % 3))) == 88;
    });
    test("Expression with repeated brackets", () => {
        return (((((2 + 3) * 4) / 2) - 1) + ((((8 / 2) + 1) * 3) - 2)) == 22;
    });
    test("Negative braced expression", () => {
        return -(2 + 3) == -5;
    });
    test("pointer math in expressions work", () => {
        var i = 22;
        const p = &i;
        const j = p + 1;
        const k = j - 1;
        return *k == 22;
    })
    test("logical right shift", () => {
        var value : uint = 0b10010010; // 146 in decimal
        var expected : uint = 0b00100100; // 36 in decimal
        var result : uint = value >> 2u; // Logical right shift (unsigned)
        return result == expected;
    })
    test("arithmetic right shift", () => {
        var value : int = -8; // 0xFFFFFFF8 in 32-bit signed representation
        var expected : int = -2; // Right shift retains the sign
        var result : int = value >> 2; // Arithmetic right shift (signed)
        return result == expected;
    })
    test("logical left shift", () => {
        var value : uint = 0b00010010; // 18 in decimal
        var expected : uint = 0b01001000; // 72 in decimal
        var result : uint = value << 2u; // Logical left shift (unsigned)
        return result == expected;
    })
    test("arithmetic left shift", () => {
        var value : int = -8; // -8 in signed
        var expected : int = -32; // Left shift retains the sign
        var result : int = value << 2; // Arithmetic left shift
        return result == expected;
    })
    test("parenthesized expressions can be casted", () => {
        var v = (10 + 20) as ushort
        return v == 30
    })
    test("second value in expression can be on next line, if it contains operator on previous line", () => {
        var v = 33
        var j = 44
        if(v == 33 ||
           j == 44
        ) {
            return true;
        } else {
            return false;
        }
    })
    test("nested logical expressions work - 1", () => {
        return ret_log_expr_1() == false;
    })
    test("nested logical expressions work - 2", () => {
        const res = (false || false) && !(false && false)
        return !res;
    })
    test("nested logical expressions work - 3", () => {
        var res = (false || false) && !(false && false)
        return !res;
    })
}