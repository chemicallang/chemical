// Iteration & For-Loop Negative Tests
// Tests that the compiler correctly rejects invalid iteration patterns.

@test
func neg_for_in_non_iterable_full(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    for(var x in 42) { }\n}\n"
    expect_compile_error(env, "for_in_non_iterable", ch, "expected container to implement")
}

@test
func neg_for_in_non_container(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    for(var x in \"hello\") { }\n}\n"
    expect_compile_error(env, "for_in_string", ch, "expected container to implement")
}

@test
func neg_for_in_unknown_size(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var p : *int = null\n    for(var x in p) { }\n}\n"
    expect_compile_error(env, "for_in_ptr", ch, "expected container to implement")
}

@test
func neg_reverse_iter_no_rev(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct MyColl {\n    var data : [3]int\n}\nimpl core::iterable::Iterable for MyColl {\n    func begin(&self) : *int { return &raw data[0] }\n    func valid(&self, it : *int) : bool { return it < &raw data[3] }\n    func current(&self, it : *int) : &int { return *it }\n    func next(&self, it : &mut *int) { *it = *it + 1 }\n}\nfunc main() {\n    var coll = MyColl { data : [1, 2, 3] }\n    for(var x in coll reversed) { }\n}\n"
    expect_compile_error(env, "reverse_iter_no_rev", ch, "reversed iteration requires")
}

@test
func neg_iter_current_not_ref(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct BadIter {\n    var data : [3]int\n}\nimpl core::iterable::Iterable for BadIter {\n    func begin(&self) : *int { return &raw data[0] }\n    func valid(&self, it : *int) : bool { return it < &raw data[3] }\n    func current(&self, it : *int) : int { return *it }\n    func next(&self, it : &mut *int) { *it = *it + 1 }\n}\nfunc main() {\n    var coll = BadIter { data : [1, 2, 3] }\n    for(var x in coll) { }\n}\n"
    expect_compile_error(env, "iter_current_not_ref", ch, "expected 'core::iterable::Iterable::current' return type to be a reference")
}

@test
func neg_array_size_not_known(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func main() {\n    var size = 10\n    var arr : [size]int\n}\n"
    expect_compile_error(env, "array_size_not_known", ch, "array size is not known at compile time")
}

@test
func neg_iter_linear_data_not_ptr(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct BadLinear {\n    var data : [3]int\n}\nimpl core::iterable::Linear for BadLinear {\n    func data(&self) : int { return 0 }\n    func size(&self) : int { return 3 }\n}\nfunc main() {\n    var coll = BadLinear { data : [1, 2, 3] }\n    for(var x in coll) { }\n}\n"
    expect_compile_error(env, "linear_data_not_ptr", ch, "expected 'core::iterable::Linear::data' return type to be a pointer")
}

@test
func neg_iter_chunked_wrong_return(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct BadChunked {\n    var data : [3]int\n}\nimpl core::iterable::Chunked for BadChunked {\n    func begin_chunks(&self) : *int { return &raw data[0] }\n    func valid_chunk(&self, it : *int) : bool { return true }\n    func current_chunk(&self, it : *int) : int { return 0 }\n    func next_chunk(&self, it : &mut *int) { }\n    func total_size(&self) : int { return 3 }\n}\nfunc main() {\n    var coll = BadChunked { data : [1, 2, 3] }\n    for(var x in coll) { }\n}\n"
    expect_compile_error(env, "chunked_wrong_return", ch, "expected 'core::iterable::Chunked::current_chunk'")
}
