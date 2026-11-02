using namespace std;

// Slice-view patterns are refutable: exact patterns only match a view whose
// runtime length equals the pattern length. These tests live in the lib suite
// because `[T]` is the std::span alias.

@test
func test_slice_view_pattern_exact(env : &mut TestEnv) {
    var values : [3]int = [1, 2, 3]
    var view : [int] = values
    if(var [a, b, c] = view) {
        if(a != 1 || b != 2 || c != 3) {
            env.error("slice pattern bound the wrong elements")
        }
        return
    }
    env.error("exact slice pattern did not match a 3-element view")
}

@test
func test_slice_view_pattern_length_mismatch(env : &mut TestEnv) {
    var values : [2]int = [1, 2]
    var view : [int] = values
    if(var [a, b, c] = view) {
        env.error("3-element slice pattern matched a 2-element view")
        return
    }
}
