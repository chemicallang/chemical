import "../test.ch"

@extern
func check_external_sum(a : int, b : int) : int

func test_external_functions() {
    test("test external sum function is available", () => {
        return check_external_sum(80, 20) == 100;
    })
}