@extern
public func check_external_sum(a : int, b : int) : int

if(intrinsics::is_clang()) {

    @extern
    @cpp
    public func check_external_cpp_sum(a : int, b : int) : int;

}

// TODO: @extern
// public var exposed_constant_213 : int

func test_external_functions() {
    test("external sum function is available", () => {
        return check_external_sum(80, 20) == 100;
    })
    test("downloaded module example_sum works", () => {
        return example_sum(120, 4) == 124
    })
    comptime if(intrinsics::is_clang()) {
        test("can call function from C++", () => {
            // C++ adds dummy 3 to confuse you
            return check_external_cpp_sum(80, 20) == 103;
        })
    }
    // TODO: test("chemical external variables / constants work with no mangle and extern", () => {
    //     return exposed_constant_213 == 9182
    // })
}