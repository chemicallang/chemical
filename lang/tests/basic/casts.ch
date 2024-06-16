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
    test("test cast without parenthesis works - 3", () => {
        var d = 1.0 as double
        var fl = d as float
        var i = (fl as int) == 1
        return i;
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
    test("cast a float to a double", () => {
        var fl = 1.0f
        var d = fl as double
        return ((d as int) == 1)
    })
    test("cast a double to a float", () => {
        var d = 1.0 as double
        var fl = d as float
        return ((fl as int) == 1)
    })
    test("cast a float to a double", () => {
        var fl = 1.0f
        return ((fl as double) == 1.0)
    })
    test("cast a double to a float", () => {
        var d = 1.0 as double
        return ((d as float) == 1.0f)
    })
}