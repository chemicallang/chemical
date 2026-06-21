
func test_comptime_intrinsics() {
    test("module scope is as expected", () => {
        var scope_name = std::string_view(intrinsics::get_module_scope())
        var exp_scope_name = std::string_view("")
        return scope_name.equals(&exp_scope_name)
    })
    test("module name is as expected", () => {
        var module_name = std::string_view(intrinsics::get_module_name())
        var exp_module_name = std::string_view("main")
        return module_name.equals(&exp_module_name)
    })
}