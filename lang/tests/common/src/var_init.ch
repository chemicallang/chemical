// Copyright (c) Chemical Language Foundation 2026.
// Tests moved from native nodes/varinit.ch — only constants tests work in interpreter

comptime const glob_ct_const = 100 + 300

const glob_const = 800

func test_var_init() {
    test("global comptime constants work", () => {
        return glob_ct_const == 400;
    })
    test("global constants work", () => {
        return glob_const == 800;
    })
}




