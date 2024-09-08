import "../test.ch"

@extern
public func check_external_sum(a : int, b : int) : int

if(compiler::is_clang_based()) {

    @extern
    @cpp
    public func check_external_cpp_sum(a : int, b : int) : int;

}

func test_external_functions() {
    test("test external sum function is available", () => {
        return check_external_sum(80, 20) == 100;
    })
    if(compiler::is_clang_based()) {
        test("test can call function from C++", () => {
            // C++ adds dummy 3 to confuse you
            return check_external_cpp_sum(80, 20) == 103;
        })
    }
}