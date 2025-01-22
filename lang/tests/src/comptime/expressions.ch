import "../test.ch"

func test_comptime_expressions() {
    test("comptime: 4 + 2 / 2 == 5", () => {
        return comptime { (4 + 2 / 2) == 5 }
    });
    test("comptime: 4 / 2 + 2 == 4", () => {
        return comptime { (4 / 2 + 2) == 4 }
    });
    test("comptime: 3 * (4 + 2) - 8 / 2 == 5", () => {
        return comptime { (4 + 2 / 2) == 5 }
    });
    test("comptime: 3 * (4 + 2) - 8 / 2 == 14", () => {
        return comptime { (3 * (4 + 2) - 8 / 2) == 14 }
    });
    test("comptime: 8 / (2 + 2) * 3 - 1 == 5", () => {
        return comptime { (8 / (2 + 2) * 3 - 1) == 5 }
    });
    test("comptime: (5 + 3) * 2 - 4 / (1 + 1) == 14", () => {
        return comptime { ((5 + 3) * 2 - 4 / (1 + 1)) == 14 }
    });
    test("comptime: single braced value", () => {
        return comptime { (5 + (5)) == 10 }
    })
    test("comptime: Simple addition and multiplication", () => {
        return comptime { (2 + 3 * 4) == 14 }
    });
    test("comptime: Addition, subtraction, multiplication", () => {
        return comptime { (10 - 4 + 2 * 3) == 12 }
    });
    test("comptime: Division and multiplication", () => {
        return comptime { (20 / 4 * 2) == 10 }
    });
    test("comptime: Brackets and multiplication", () => {
        return comptime { ((5 + 3) * 2) == 16 }
    });
    test("comptime: Brackets and division", () => {
        return comptime { (8 / (2 + 2)) == 2 }
    });
    test("comptime: Complex expression", () => {
        return comptime { ((5 + 3) * 2 - 4 / (1 + 1)) == 14 }
    });
    test("comptime: Nested brackets", () => {
        return comptime { (((4 * 2) + 3) * (10 / 5)) == 22 }
    });
    test("comptime: Expression with negative numbers", () => {
        return comptime { (-3 * (-4 + 2)) == 6 }
    });
    test("comptime: double with integer addition", () => {
        return comptime { 4 + 1.5 == 5.5 }
    });
    test("comptime: Expression with decimal numbers", () => {
        return comptime { (1.5 * (4 + 1.5)) == 8.25 }
    });
    test("comptime: Expression with variables", () => {
        var a = 5;
        var b = 3;
        return ((a + b) * 2 - 4 / (1 + 1)) == 14;
    });
    test("comptime: Expression with modulo", () => {
        return comptime { (10 % 3 == 1) }
    });
    test("comptime: Expression with mixed operations", () => {
        return comptime { (5 + 3) * 2 - 4 / (1 + 1) == 14 }
    });
    test("comptime: Longer expression with mixed operations", () => {
        return comptime { (2 * (3 + 4) / 2 - (7 % 3) + 5) == 11 }
    });
    test("comptime: Expression with nested brackets and subtraction", () => {
        return comptime { (10 - (2 * (3 + 4) / 2) + (7 % 3) - 5) == -1 }
    });
    test("comptime: Expression with multiple division and subtraction", () => {
        return comptime { (100 / 5 / 2 - 4 / 2 - 3) == 5 }
    });
    test("comptime: Expression with consecutive multiplication", () => {
        return comptime { (2 * 3 * 4 * 5) == 120 }
    });
    test("comptime: Expression with consecutive division", () => {
        return comptime { (100 / 5 / 2) == 10 }
    });
    test("comptime: Expression with consecutive addition", () => {
        return comptime { (1 + 2 + 3 + 4 + 5) == 15 }
    });
    test("comptime: Expression with consecutive subtraction", () => {
        return comptime { (20 - 5 - 4 - 3 - 2 - 1) == 5 }
    });
    test("comptime: Expression with mixed operations and negative numbers", () => {
        return comptime { (10 - (-3 * (4 + 2)) + 8 / 2) == 32 }
    });
    test("comptime: Expression with complex nested operations", () => {
        return comptime { ((10 * (5 + (3 * 2) - 4 / 2)) - (2 * (7 % 3))) == 88 }
    });
    test("comptime: Expression with repeated brackets", () => {
        return comptime { (((((2 + 3) * 4) / 2) - 1) + ((((8 / 2) + 1) * 3) - 2)) == 22 }
    });
    test("comptime: Negative braced expression", () => {
        return comptime { -(2 + 3) == -5 }
    });
}