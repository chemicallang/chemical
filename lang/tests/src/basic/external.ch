@extern
public func check_external_sum(a : int, b : int) : int

if(intrinsics::is_clang()) {

    @extern
    @cpp
    public func check_external_cpp_sum(a : int, b : int) : int;

}

func test_external_functions() {
    test("external sum function is available", () => {
        return check_external_sum(80, 20) == 100;
    })
    if(intrinsics::is_clang()) {
        test("can call function from C++", () => {
            // C++ adds dummy 3 to confuse you
            return check_external_cpp_sum(80, 20) == 103;
        })
    }
}