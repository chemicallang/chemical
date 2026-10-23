// Copyright (c) Chemical Language Foundation 2026.

using std::string_view;
using std::string;

func test_clone_impls() {
    test("int clone", () => {
        var x = 41
        return x.clone() == 41
    })
    test("bool clone", () => {
        var x = true
        return x.clone()
    })
    test("string_view clone", () => {
        var s = string_view("hello")
        var t = s.clone()
        return t.size() == 5 && t.get(0) == 'h'
    })
    test("string deep clone", () => {
        var s = string("hello")
        var t = s.clone()
        return t.size() == 5 && t.get(0) == 'h'
    })
}
