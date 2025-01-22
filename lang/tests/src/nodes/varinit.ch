import "/test.ch"

@comptime
const glob_ct_const = 100 + 300

const glob_const = 300 + 500

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
    test("can assign to a non initialized variable", () => {
        var x : int
        x = 6;
        return x == 6;
    })
    test("can initialize a typed variable", () => {
        var x : int = 5;
        return x == 5;
    })
    test("global comptime constants work", () => {
        return glob_ct_const == 400;
    })
    test("global constants work", () => {
        return glob_const == 800;
    })
    test("local constants work as well", () => {
        const something = 1200 + 400
        return something == 1600
    })
}