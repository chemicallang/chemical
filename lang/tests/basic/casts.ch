import "../test.ch"

func test_casts() {
    test("test long value can be truncated", () => {
        var num1 = 30;
        var num2 : long = 16;
        return (num1 > (num2 as int));
    })
    test("test int value can be extended", () => {
        var num1 : int = 30;
        var num2 : long = 16;
        return ((num1 as long) > num2);
    })
    test("test cast without parenthesis works - 1", () => {
        var num1 = 30;
        var num2 : long = 16;
        return (num1 > num2 as int);
    })
    test("test cast without parenthesis works - 2", () => {
        var num1 : int = 30;
        var num2 : long = 16;
        return (num1 as long > num2);
    })
    test("cast double to an integer", () => {
        var num = 1.0 as double
        var num2 = num as int
        return num2 == 1;
    })
    test("cast float to a double", () => {
        var num = 1.0f as float
        var num2 = num as int
        return num2 == 1;
    })
    test("cast int to a double", () => {
        var num = 1
        var num2 = num as double
        return num2 == 1.0;
    })
    test("cast int to a float", () => {
        var num = 1
        var num2 = num as float
        return num2 == 1.0f;
    })
}