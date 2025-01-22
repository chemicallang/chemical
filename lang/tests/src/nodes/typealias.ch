import "/test.ch"

typealias SimpleFunc = () => int;

func take_simple_func(simple : SimpleFunc) : int {
    return simple();
}

typealias SimpleFunc2 = () => void;

func take_simple_func2(simple : SimpleFunc2) {
    simple();
}

var my_secret_typealias_number = 0;

func test_typealias() {
    test("typealias to a function works - 1", () => {
        return take_simple_func(() => 674) == 674;
    })
    test("typealias to a function works - 2", () => {
        return take_simple_func(() => 837) == 837;
    })
    test("typealias to a function works - 3", () => {
        take_simple_func2(() => {
            my_secret_typealias_number = 2346
        })
        return my_secret_typealias_number == 2346
    })
}