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
}