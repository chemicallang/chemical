import "../test.ch"

func test_var_init() {
    test("can initialize normal variables", () => {
        var x = 5;
        return x == 5;
    })
    test("can modify variables", () => {
        var x = 5;
        x = 6;
        return x == 6;
    });
    /**
    test("can assign to a non initialized variable", () => {
        var x : int
        x = 6;
        return x == 6;
    })
    **/
    test("can initialize a typed variable", () => {
        var x : int = 5;
        return x == 5;
    })
}