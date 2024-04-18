import "test.ch"

func main() {
    test("4 + 2 / 2 == 5", []() => {
        return (4 + 2 / 2) == 5;
    });
    test("4 / 2 + 2 == 4", []() => {
        return (4 / 2 + 2) == 4;
    });
    test("3 * (4 + 2) - 8 / 2 == 5", []() => {
        return (4 + 2 / 2) == 5;
    });
    test("3 * (4 + 2) - 8 / 2 == 14", []() => {
        return (3 * (4 + 2) - 8 / 2) == 14;
    });
    test("8 / (2 + 2) * 3 - 1 == 5", []() => {
        return (8 / (2 + 2) * 3 - 1) == 5;
    });
    test("(5 + 3) * 2 - 4 / (1 + 1) == 14", []() => {
        return ((5 + 3) * 2 - 4 / (1 + 1)) == 14;
    });
    test("Simple addition and multiplication", []() => {
        return (2 + 3 * 4) == 14;
    });
    test("Addition, subtraction, multiplication", []() => {
        return (10 - 4 + 2 * 3) == 12;
    });
    test("Division and multiplication", []() => {
        return (20 / 4 * 2) == 10;
    });
    test("Brackets and multiplication", []() => {
        return ((5 + 3) * 2) == 16;
    });
    test("Brackets and division", []() => {
        return (8 / (2 + 2)) == 2;
    });
    test("Complex expression", []() => {
        return ((5 + 3) * 2 - 4 / (1 + 1)) == 14;
    });
    test("Nested brackets", []() => {
        return (((4 * 2) + 3) * (10 / 5)) == 22;
    });
    test("Expression with negative numbers", []() => {
        return (-3 * (-4 + 2)) == 6;
    });
    test("Decimal number with integer addition", []() => {
        return 4 + 1.5 == 5.5;
    });
    test("Expression with decimal numbers", []() => {
        return (1.5 * (4 + 1.5)) == 8.25;
    });
    test("Expression with variables", []() => {
        var a = 5;
        var b = 3;
        return ((a + b) * 2 - 4 / (1 + 1)) == 14;
    });
    test("Expression with modulo", []() => {
        return (10 % 3 == 1);
    });
    test("Expression with mixed operations", []() => {
        return (5 + 3) * 2 - 4 / (1 + 1) == 14;
    });
    test("Longer expression with mixed operations", []() => {
        return (2 * (3 + 4) / 2 - (7 % 3) + 5) == 11;
    });
    test("Expression with nested brackets and subtraction", []() => {
        return (10 - (2 * (3 + 4) / 2) + (7 % 3) - 5) == -1;
    });
    test("Expression with multiple division and subtraction", []() => {
        return (100 / 5 / 2 - 4 / 2 - 3) == 5;
    });
    test("Expression with consecutive multiplication", []() => {
        return (2 * 3 * 4 * 5) == 120;
    });
    test("Expression with consecutive division", []() => {
        return (100 / 5 / 2) == 10;
    });
    test("Expression with consecutive addition", []() => {
        return (1 + 2 + 3 + 4 + 5) == 15;
    });
    test("Expression with consecutive subtraction", []() => {
        return (20 - 5 - 4 - 3 - 2 - 1) == 5;
    });
    test("Expression with mixed operations and negative numbers", []() => {
        return (10 - (-3 * (4 + 2)) + 8 / 2) == 32;
    });
    test("Expression with complex nested operations", []() => {
        return ((10 * (5 + (3 * 2) - 4 / 2)) - (2 * (7 % 3))) == 88;
    });
    test("Expression with repeated brackets", []() => {
        return (((((2 + 3) * 4) / 2) - 1) + ((((8 / 2) + 1) * 3) - 2)) == 22;
    });
    // TODO test power expression
    // TODO support negative braced expressions like -(1 + 1)
}